/*
 * Copyright 2024, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <microkit.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>
#include <sel4/sel4.h>

#include "libmicrokitco.h"
#include <libco.h>

void microkit_cothread_panic() {
    char *panic_addr = (char *) NULL;
    *panic_addr = (char) 0;
}

// Error handling
const char *err_strs[] = {
    "libmicrokitco: generic: no error.\n",
    "libmicrokitco: generic: not initialised.\n",
    "libmicrokitco: generic: invalid cothread handle.\n",

    "libmicrokitco: init(): already initialised.\n",
    "libmicrokitco: init(): co-stack size too small.\n",
    "libmicrokitco: init(): num_costacks not equal to LIBMICROKITCO_MAX_COTHREADS defined in preprocessing.\n",
    "libmicrokitco: init(): got a NULL co-stack pointer.\n",
    "libmicrokitco: init(): overlapping stacks.\n",
    "libmicrokitco: init(): failed to initialise free handles queue.\n",
    "libmicrokitco: init(): failed to initialise scheduling queue.\n",
    "libmicrokitco: init(): failed to fill free handles queue.\n",

    "libmicrokitco: my_arg(): my_arg() called from the root thread.\n",

    "libmicrokitco: recv_ntfn(): no cothreads are blocked on this channel.\n",
    "libmicrokitco: recv_ntfn(): a previous notification is already queued on this channel. Ignoring this notification\n",

    "libmicrokitco: spawn(): client entrypoint is NULL.\n",
    "libmicrokitco: spawn(): number of arguments is negative.\n",
    "libmicrokitco: spawn(): number of arguments is greater than maximum.\n",
    "libmicrokitco: spawn(): maximum amount of cothreads reached.\n",
    "libmicrokitco: spawn(): cannot schedule the new cothread.\n",

    "libmicrokitco: mark_ready(): subject cothread is already ready.\n",
    "libmicrokitco: mark_ready(): cannot mark self as ready.\n",
    "libmicrokitco: mark_ready(): cannot schedule subject cothread.\n",

    "libmicrokitco: wait(): invalid channel.\n",

    "libmicrokitco: yield(): cannot schedule caller.\n",

    "libmicrokitco: destroy_specific(): cannot release free handle back into queue.\n",
    "libmicrokitco: destroy_specific(): caller cannot destroy self.\n",

    "libmicrokitco: join(): cannot join to root PD thread as it does not return!.\n",
    "libmicrokitco: join(): detected deadlock.\n",
};

// Return a string of human friendly error message.
const char *microkit_cothread_pretty_error(const co_err_t err_num) {
    if (err_num < 0 || err_num >= co_num_errors) {
        return "libmicrokitco: unknown error!\n";
    } else {
        return err_strs[err_num];
    }
}

// Business logic:

// each PD can only have one "instance" of this library running.
static co_control_t *co_controller = NULL;

co_err_t microkit_cothread_init(const uintptr_t controller_memory_addr, const size_t co_stack_size, ...) {
    // We don't allow skipping error checking here for safety because this can only be ran once per PD.
    if (co_controller != NULL) {
        return co_err_init_already_initialised;
    }
    if (co_stack_size < MINIMUM_STACK_SIZE) {
        return co_err_init_stack_too_small;
    }

    // This part will VMFault on write if the given memory is not large enough.
    memzero((void *) controller_memory_addr, LIBMICROKITCO_CONTROLLER_SIZE);
    co_controller = (co_control_t *) controller_memory_addr;
    co_controller->co_stack_size = co_stack_size;

    // Parses all the valid stack memory regions
    va_list ap;
    va_start(ap, co_stack_size);
    for (int i = 1; i <= LIBMICROKITCO_MAX_COTHREADS; i++) {
        co_controller->tcbs[i].local_storage = (void *) va_arg(ap, uintptr_t);

        if (!co_controller->tcbs[i].local_storage) {
            co_controller = NULL;
            return co_err_init_co_stack_null;
        }

        // sanity check the stacks, crash if stack not as big as we think
        // we only memzero the stack on cothread spawn.
        char *stack = (char *) co_controller->tcbs[i].local_storage;
        stack[0] = 0;
        stack[co_stack_size - 1] = 0;
    }
    va_end(ap);

    // Check that none of the stacks overlap
    for (int i = 1; i <= LIBMICROKITCO_MAX_COTHREADS; i++) {
        uintptr_t this_stack_start = (uintptr_t) co_controller->tcbs[i].local_storage;
        uintptr_t this_stack_end = this_stack_start + co_stack_size - 1;

        for (int j = 1; j <= LIBMICROKITCO_MAX_COTHREADS; j++) {
            if (j != i) {
                uintptr_t other_stack_start = (uintptr_t) co_controller->tcbs[j].local_storage;
                uintptr_t other_stack_end = other_stack_start + co_stack_size - 1;

                if (this_stack_start <= other_stack_end && other_stack_start <= this_stack_end) {
                    return co_err_init_co_stack_overlap;
                } 
            }
        }
    }

    // Initialise the root thread's handle;
    co_controller->tcbs[0].local_storage = NULL;
    co_controller->tcbs[0].co_handle = co_active();
    co_controller->tcbs[0].state = cothread_running;
    co_controller->tcbs[0].next_blocked_on_same_channel = -1;
    co_controller->tcbs[0].next_joined_on_same_cothread = -1;
    co_controller->tcbs[0].joined_on = -1;
    co_controller->running = 0;

    // Initialise the queues
    const int err_hq = hostedqueue_init(
        &co_controller->free_handle_queue,
        MAX_THREADS
    );
    const int err_sq = hostedqueue_init(
        &co_controller->scheduling_queue,
        MAX_THREADS
    );

    if (err_hq != LIBHOSTEDQUEUE_NOERR) {
        co_controller = NULL;
        return co_err_init_free_handles_init_fail;
    }
    if (err_sq != LIBHOSTEDQUEUE_NOERR) {
        co_controller = NULL;
        return co_err_init_sched_init_fail;
    }

    // Enqueue all the free cothread handle IDs but exclude the root thread.
    for (microkit_cothread_t i = 1; i < MAX_THREADS; i++) {
        if (hostedqueue_push(&co_controller->free_handle_queue, co_controller->free_handle_queue_mem, &i) != LIBHOSTEDQUEUE_NOERR) {
            co_controller = NULL;
            return co_err_init_free_handles_populate_fail;
        }
    }

    // Initialise the blocked table
    for (int i = 0; i < MICROKIT_MAX_CHANNELS; i++) {
        co_controller->blocked_channel_map[i].head = -1;
        co_controller->blocked_channel_map[i].tail = -1;

#ifdef LIBMICROKITCO_PREEMPTIVE_UNBLOCK
        co_controller->blocked_channel_map[i].queued = 0;
#endif

    }

    // Initialised the joined table
    for (int j = 0; j < MAX_THREADS; j++) {
        co_controller->joined_map[j].head = -1;
        co_controller->joined_map[j].tail = -1;
    }

    return co_no_err;
}

co_err_t microkit_cothread_query_state(const microkit_cothread_t cothread, co_state_t *ret_state) {
#if !defined(LIBMICROKITCO_UNSAFE)
    if (co_controller == NULL) {
        return co_err_generic_not_initialised;
    }
    if (cothread >= MAX_THREADS || cothread < 0) {
        return co_err_generic_invalid_handle;
    }
#endif

    *ret_state = co_controller->tcbs[cothread].state;
    return co_no_err;
}

co_err_t microkit_cothread_free_handle_available(bool *ret_flag, microkit_cothread_t *ret_handle) {
#if !defined(LIBMICROKITCO_UNSAFE)
    if (co_controller == NULL) {
        return co_err_generic_not_initialised;
    }
#endif
    *ret_flag = hostedqueue_peek(&co_controller->free_handle_queue, co_controller->free_handle_queue_mem, ret_handle) == LIBHOSTEDQUEUE_NOERR;
    return co_no_err;
}

co_err_t microkit_cothread_my_handle(microkit_cothread_t *ret_handle) {
#if !defined(LIBMICROKITCO_UNSAFE)
    if (co_controller == NULL) {
        return co_err_generic_not_initialised;
    }
#endif

    *ret_handle = co_controller->running;
    return co_no_err;
}

co_err_t microkit_cothread_my_arg(uintptr_t *ret_priv_arg) {
    #if !defined(LIBMICROKITCO_UNSAFE)
        if (co_controller == NULL) {
            return co_err_generic_not_initialised;
        }
        if (co_controller->running == 0) {
            return co_err_my_arg_called_from_root_thread;
        }
    #endif

    *ret_priv_arg = co_controller->tcbs[co_controller->running].private_arg;
    return co_no_err;
}

co_err_t microkit_cothread_set_arg(const microkit_cothread_t cothread, uintptr_t private_arg) {
    #if !defined(LIBMICROKITCO_UNSAFE)
        if (co_controller == NULL) {
            return co_err_generic_not_initialised;
        }
        if (co_controller->running == 0) {
            return co_err_my_arg_called_from_root_thread;
        }
        if (cothread >= MAX_THREADS || cothread < 0 || co_controller->tcbs[cothread].state == cothread_not_active) {
            return co_err_generic_invalid_handle;
        }
    #endif

    co_controller->tcbs[cothread].private_arg = private_arg;
    return co_no_err;
}

co_err_t microkit_cothread_recv_ntfn(const microkit_channel ch) {
#if !defined(LIBMICROKITCO_UNSAFE)
    if (co_controller == NULL) {
        return co_err_generic_not_initialised;
    }
#endif

    if (co_controller->running) {
        // Called from a cothread context
        microkit_cothread_panic();
    }

    microkit_cothread_t blocked_list_head = co_controller->blocked_channel_map[ch].head;
    microkit_cothread_t blocked_list_tail = co_controller->blocked_channel_map[ch].tail;

    // Delete the waiting list
    co_controller->blocked_channel_map[ch].head = -1;
    co_controller->blocked_channel_map[ch].tail = -1;

#ifdef LIBMICROKITCO_PREEMPTIVE_UNBLOCK
    if (blocked_list_head == -1) {
        if (co_controller->blocked_channel_map[ch].queued < MAX_NTFN_QUEUE) {
            co_controller->blocked_channel_map[ch].queued += 1;
            return co_no_err;
        } else if (co_controller->blocked_channel_map[ch].queued == MAX_NTFN_QUEUE) {
            return co_err_recv_ntfn_already_queued;
        }
    }
#else
    if (blocked_list_head == -1) {
        return co_err_recv_ntfn_no_blocked;
    }
#endif

#ifdef LIBMICROKITCO_RECV_NTFN_NO_FASTPATH
    if (0) {
#else
    if (blocked_list_head == blocked_list_tail) {
#endif
        // Fast-path: enter when there is only 1 cothread blocked on this channel.
        // Directly switch to the singular blocked cothread, bypassing the scheduling queue
        co_controller->tcbs[co_controller->running].state = cothread_ready;
        co_controller->tcbs[blocked_list_head].state = cothread_running;
        co_controller->running = blocked_list_head;
        co_switch(co_controller->tcbs[blocked_list_head].co_handle);
    } else {
        // Slow-path: 2 >= cothreads blocked on this channel.
        microkit_cothread_t cur = blocked_list_head;

        // Iterate over the blocked linked list, unblocks all the cothreads and schedule them
        while (cur != -1) {
            const co_err_t err = microkit_cothread_mark_ready(cur);
            if (err != co_no_err) {
                return err;
            }

            const microkit_cothread_t next = co_controller->tcbs[cur].next_blocked_on_same_channel;
            co_controller->tcbs[cur].next_blocked_on_same_channel = -1;
            cur = next;
        }

        // Run the cothreads
        microkit_cothread_yield();
    }

    return co_no_err;

}

static inline void internal_destroy_me();
static inline void internal_entry_return(microkit_cothread_t cothread) {
    // Wake all the joined cothreads on this cothread (if any)
    microkit_cothread_t cur = co_controller->joined_map[cothread].head;
    while (cur != -1) {
        const co_err_t err = microkit_cothread_mark_ready(cur);
        if (err != co_no_err) {
            microkit_internal_crash((seL4_Error) err);
        }

        microkit_cothread_t next = co_controller->tcbs[cur].next_joined_on_same_cothread;
        co_controller->tcbs[cur].next_joined_on_same_cothread = -1;
        cur = next;
    }

    // Erase the list after we are done waking up the cothreads
    co_controller->joined_map[cothread].head = -1;
    co_controller->joined_map[cothread].tail = -1;

    // Switch to another cothread. We never actually return internally.
    internal_destroy_me();
}

static inline void cothread_entry_wrapper() {
    // Execute the client entry point
    co_controller->tcbs[co_controller->running].client_entry();

    // After the client entry point return, wake any joined cothreads and clean up after ourselves.
    internal_entry_return(co_controller->running);

    // Should not get here.
    // UB in libco if cothread returns normally!
    microkit_cothread_panic();
}

co_err_t microkit_cothread_spawn(const client_entry_t client_entry, const bool ready, microkit_cothread_t *handle_ret, uintptr_t private_arg) {
#if !defined LIBMICROKITCO_UNSAFE
    if (co_controller == NULL) {
        return co_err_generic_not_initialised;
    }
    if (!client_entry) {
        return co_err_spawn_client_entry_null;
    }
#endif

    hosted_queue_t *free_handle_queue = &co_controller->free_handle_queue;
    microkit_cothread_t new;
    const int pop_err = hostedqueue_pop(free_handle_queue, co_controller->free_handle_queue_mem, &new);

    if (pop_err != LIBHOSTEDQUEUE_NOERR) {
        return co_err_spawn_max_cothreads_reached;
    }

    unsigned char *costack = (unsigned char *) co_controller->tcbs[new].local_storage;
    memzero(costack, co_controller->co_stack_size);
    co_controller->tcbs[new].client_entry = client_entry;
    co_controller->tcbs[new].private_arg = private_arg;
    co_controller->tcbs[new].co_handle = co_derive(costack, co_controller->co_stack_size, cothread_entry_wrapper);
    co_controller->tcbs[new].state = cothread_initialised;
    co_controller->tcbs[new].next_blocked_on_same_channel = -1;
    co_controller->tcbs[new].next_joined_on_same_cothread = -1;
    co_controller->tcbs[new].joined_on = -1;

    if (ready) {
        int err = microkit_cothread_mark_ready(new);
        if (err != co_no_err) {
            co_controller->tcbs[new].state = cothread_not_active;
            hostedqueue_push(free_handle_queue, co_controller->free_handle_queue_mem, &new);
            return co_err_spawn_cannot_schedule;
        }
    }

    *handle_ret = new;
    return co_no_err;
}

co_err_t microkit_cothread_mark_ready(const microkit_cothread_t cothread) {
#if !defined(LIBMICROKITCO_UNSAFE)
    if (co_controller == NULL) {
        return co_err_generic_not_initialised;
    }
    if (cothread >= MAX_THREADS || cothread < 0 || co_controller->tcbs[cothread].state == cothread_not_active) {
        return co_err_generic_invalid_handle;
    }
    if (co_controller->tcbs[cothread].state == cothread_ready) {
        return co_err_mark_ready_already_ready;
    }
    if (co_controller->running == cothread) {
        return co_err_mark_ready_cannot_mark_self;
    }
#endif

    hosted_queue_t *sched_queue = &co_controller->scheduling_queue;
    const int push_err = hostedqueue_push(sched_queue, co_controller->scheduling_queue_mem, &cothread);
    const co_state_t old_state = co_controller->tcbs[cothread].state;
    co_controller->tcbs[cothread].state = cothread_ready;

    if (push_err != LIBHOSTEDQUEUE_NOERR) {
        co_controller->tcbs[cothread].state = old_state;
        return co_err_mark_ready_cannot_schedule;
    } else {
        return co_no_err;
    }
}

// Pop a ready thread from a given scheduling queue, discard any destroyed thread
static inline microkit_cothread_t internal_pop_from_queue(hosted_queue_t *sched_queue, co_tcb_t* tcbs) {
    while (true) {
        microkit_cothread_t next_choice;
        const int peek_err = hostedqueue_pop(sched_queue, co_controller->scheduling_queue_mem, &next_choice);
        if (peek_err == LIBHOSTEDQUEUE_ERR_EMPTY) {
            return SCHEDULER_NULL_CHOICE;
        } else if (peek_err == LIBHOSTEDQUEUE_NOERR) {
            // queue not empty
            if (tcbs[next_choice].state == cothread_ready) {
                return next_choice;
            }
        } else {
            // catch other errs
            microkit_cothread_panic();
        }
    }
}

// Pick a ready thread
static inline microkit_cothread_t internal_schedule() {
    co_tcb_t* tcbs = co_controller->tcbs;
    return internal_pop_from_queue(&co_controller->scheduling_queue, tcbs);
}

// Switch to the next ready thread, also handle cases where there is no ready thread.
static inline void internal_go_next() {
    microkit_cothread_t next = internal_schedule();
    if (next == SCHEDULER_NULL_CHOICE) {
        // no ready thread in the queue, go back to root execution thread to receive notifications
        next = 0;
    }

    co_controller->tcbs[next].state = cothread_running;
    co_controller->running = next;
    co_switch(co_controller->tcbs[next].co_handle);
}

co_err_t microkit_cothread_wait_on_channel(const microkit_channel wake_on) {
#if !defined(LIBMICROKITCO_UNSAFE)
    if (co_controller == NULL) {
        return co_err_generic_not_initialised;
    }
    if (wake_on >= MICROKIT_MAX_CHANNELS) {
        return co_err_wait_invalid_channel;
    }
#endif

#ifdef LIBMICROKITCO_PREEMPTIVE_UNBLOCK
    if (co_controller->blocked_channel_map[wake_on].queued > 0) {
        co_controller->blocked_channel_map[wake_on].queued -= 1;
        return co_no_err;
    }
#endif

    co_controller->tcbs[co_controller->running].state = cothread_blocked_on_channel;

    // push into the waiting linked list (if any)
    if (co_controller->blocked_channel_map[wake_on].head == -1) {
        co_controller->blocked_channel_map[wake_on].head = co_controller->running;
    } else {
        const microkit_cothread_t tail_tcb = co_controller->blocked_channel_map[wake_on].tail;
        co_controller->tcbs[tail_tcb].next_blocked_on_same_channel = co_controller->running;
    }
    co_controller->blocked_channel_map[wake_on].tail = co_controller->running;
    co_controller->tcbs[co_controller->running].next_blocked_on_same_channel = -1;

    internal_go_next();
    return co_no_err;
}

co_err_t microkit_cothread_block() {
    co_controller->tcbs[co_controller->running].state = cothread_blocked;
    internal_go_next();
    return co_no_err;
}

co_err_t microkit_cothread_yield() {
#if !defined(LIBMICROKITCO_UNSAFE)
    if (co_controller == NULL) {
        return co_err_generic_not_initialised;
    }
#endif

    // Caller get pushed onto the appropriate scheduling queue.
    hosted_queue_t *sched_queue = &co_controller->scheduling_queue;
    hostedqueue_push(sched_queue, co_controller->scheduling_queue_mem, &co_controller->running);
    co_controller->tcbs[co_controller->running].state = cothread_ready;

    // If the scheduling queues are empty beforehand, the caller just get runned again.
    internal_go_next();

    return co_no_err;
}

// This function get executed when the client cothread returns. We updates the internal states of the library
// to indicate that the cothread's TCB is no longer used, and switch to the next ready cothread.
static inline void internal_destroy_me() {
#if !defined(LIBMICROKITCO_UNSAFE)
    if (co_controller == NULL) {
        return;
    }
#endif

    if (co_controller->running == 0) {
        return;
    }

    const int err = microkit_cothread_destroy(co_controller->running);

#if !defined(LIBMICROKITCO_UNSAFE)
    if (err != co_no_err) {
        microkit_cothread_panic();
    }
#endif

    // Should not return
    microkit_cothread_panic();
}

co_err_t microkit_cothread_destroy(const microkit_cothread_t cothread) {
#if !defined(LIBMICROKITCO_UNSAFE)
    if (co_controller == NULL) {
        return co_err_generic_not_initialised;
    }
    if (cothread >= MAX_THREADS || cothread < 0) {
        return co_err_generic_invalid_handle;
    }
#endif

    if (co_controller->running == 0) {
        // cannot destroy root thread
        microkit_cothread_panic();
    }
    if (hostedqueue_push(&co_controller->free_handle_queue, co_controller->free_handle_queue_mem, &cothread) != LIBHOSTEDQUEUE_NOERR) {
        return co_err_destroy_specific_cannot_release_handle;
    }
    co_controller->tcbs[cothread].state = cothread_not_active;

    if (cothread == co_controller->running) {
        internal_go_next();
    }

    return co_no_err;
}

static inline int internal_detect_deadlock(microkit_cothread_t caller, microkit_cothread_t joinee) {
    // depth first search to detect circular joins
    microkit_cothread_t cur = co_controller->tcbs[joinee].joined_on;
    while (cur != -1) {
        if (cur == caller) {
            return 1;
        } else {
            cur = co_controller->tcbs[cur].joined_on;
        }
    }

    return 0;
}

co_err_t microkit_cothread_join(const microkit_cothread_t cothread) {
#if !defined(LIBMICROKITCO_UNSAFE)
    if (co_controller == NULL) {
        return co_err_generic_not_initialised;
    }
    if (cothread >= MAX_THREADS || cothread < 0) {
        return co_err_generic_invalid_handle;
    }
#endif

    if (co_controller->tcbs[cothread].state == cothread_not_active) {
        return co_err_generic_not_initialised;
    }
    if (cothread == 0) {
        return co_err_join_cannot_join_to_root_thread;
    }

    if (cothread == co_controller->running || internal_detect_deadlock(co_controller->running, cothread)) {
        return co_err_join_deadlock_detected;
    }

    co_controller->tcbs[co_controller->running].state = cothread_blocked_on_join;

    // push into the waiting linked list (if any)
    if (co_controller->joined_map[cothread].head == -1) {
        co_controller->joined_map[cothread].head = co_controller->running;
    } else {
        const microkit_cothread_t tail_tcb = co_controller->joined_map[cothread].tail;
        co_controller->tcbs[tail_tcb].next_joined_on_same_cothread = co_controller->running;
    }
    co_controller->joined_map[cothread].tail = co_controller->running;
    co_controller->tcbs[co_controller->running].next_joined_on_same_cothread = -1;
    co_controller->tcbs[co_controller->running].joined_on = cothread;

    internal_go_next();

    return co_no_err;
}
