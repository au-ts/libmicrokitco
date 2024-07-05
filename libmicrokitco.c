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

#pragma GCC diagnostic error "-Wpedantic"
#pragma GCC diagnostic push

#include <libco.h>

char microkit_cothread_panic(const uintptr_t err) {
    char *panic_addr = (char *) err;
    return *panic_addr;
}

// Error handling
typedef enum {
    reserved,
    cannot_release_our_handle_after_return,
    cannot_destroy_self_after_return,
    internal_pop_from_queue_cannot_pop,
    internal_pop_from_queue_found_non_ready_cothread_in_schedule_queue,
} internal_co_errors_t;

const char *external_err_strs[] = {
    "libmicrokitco: generic: no error.\n",
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

    "libmicrokitco: recv_ntfn(): called from cothread context.\n",
    "libmicrokitco: recv_ntfn(): this channel's semaphore is already set.\n",

    "libmicrokitco: spawn(): client entrypoint is NULL.\n",
    "libmicrokitco: spawn(): maximum amount of cothreads reached.\n",
    "libmicrokitco: spawn(): cannot schedule the new cothread.\n",

    "libmicrokitco: mark_ready(): subject cothread is already ready.\n",
    "libmicrokitco: mark_ready(): cannot mark self as ready.\n",
    "libmicrokitco: mark_ready(): cannot schedule subject cothread.\n",

    "libmicrokitco: semaphore_signal_once(): cannot schedule the unblocked cothread.\n",
    "libmicrokitco: semaphore_signal_all(): cannot schedule the unblocked cothreads.\n",

    "libmicrokitco: wait(): invalid channel.\n",

    "libmicrokitco: yield(): cannot schedule caller.\n",

    "libmicrokitco: destroy(): cannot destroy root thread.\n",
    "libmicrokitco: destroy(): cannot release free handle back into queue.\n",
};

// Return a string of human friendly error message.
const char *microkit_cothread_pretty_error(const co_err_t err_num) {
    if (err_num < 0 || err_num >= co_num_errors) {
        return "libmicrokitco: unknown error!\n";
    } else {
        return external_err_strs[err_num];
    }
}

// =========== Business logic ===========

#define LIBMICROKITCO_ROOT_THREAD 0
#define MINIMUM_STACK_SIZE 0x1000 // Minimum is page size
#define SCHEDULER_NULL_CHOICE -1
#define NULL_HANDLE -1

// each PD can only have one "instance" of this library running.
static co_control_t *co_controller = NULL;

// =========== Helper functions ===========

// Pick a ready thread, essentially popping the first item from the scheduling queue.
static inline microkit_cothread_t internal_schedule(void) {
    microkit_cothread_t next_choice;
    const int peek_err = hostedqueue_pop(&co_controller->scheduling_queue, co_controller->scheduling_queue_mem, &next_choice);
    if (peek_err == LIBHOSTEDQUEUE_ERR_EMPTY) {
        return SCHEDULER_NULL_CHOICE;
    } else if (peek_err == LIBHOSTEDQUEUE_NOERR) {
        if (co_controller->tcbs[next_choice].state == cothread_ready) {
            return next_choice;
        } else {
            microkit_cothread_panic(internal_pop_from_queue_found_non_ready_cothread_in_schedule_queue);
        }
    } else {
        // catch other errs
        microkit_cothread_panic(internal_pop_from_queue_cannot_pop);
    }
    return next_choice;
}

// Switch to the next ready thread, also handle cases where there is no ready thread.
static inline void internal_go_next(void) {
    microkit_cothread_t next = internal_schedule();
    if (next == SCHEDULER_NULL_CHOICE) {
        // no ready thread in the queue, go back to root execution thread to receive notifications
        next = 0;
    }

    co_controller->tcbs[next].state = cothread_running;
    co_controller->running = next;
    co_switch(co_controller->tcbs[next].co_handle);
}

// This function get executed when the client cothread returns. We updates the internal states of the library
// to indicate that the cothread's TCB is no longer used, and switch to the next ready cothread.
static inline void internal_destroy_me(void) {
    if (hostedqueue_push(&co_controller->free_handle_queue, 
                         co_controller->free_handle_queue_mem, 
                         &co_controller->running) != LIBHOSTEDQUEUE_NOERR)
    {
        microkit_cothread_panic(cannot_release_our_handle_after_return);
    }

    co_controller->tcbs[co_controller->running].state = cothread_not_active;
    internal_go_next();

    // Should not return
    microkit_cothread_panic(cannot_destroy_self_after_return);
}

static inline void cothread_entry_wrapper(void) {
    // Execute the client entry point
    co_controller->tcbs[co_controller->running].client_entry();

    internal_destroy_me();
}

// =========== Semaphores ===========

co_err_t microkit_cothread_semaphore_init(microkit_cothread_sem_t *ret_sem) {
    ret_sem->head = NULL_HANDLE;
    ret_sem->tail = NULL_HANDLE;
    ret_sem->set = false;
    return co_no_err;
}

// This internal version does not touch the state of the calling cothread, whereas the public version does!
static inline bool internal_sem_wait(microkit_cothread_sem_t *sem) {
    if (sem->set) {
        co_controller->tcbs[co_controller->running].state = cothread_running;
        sem->set = false;
        return co_no_err;
    } else {
        if (sem->head == NULL_HANDLE) {
            sem->head = co_controller->running;
            sem->tail = co_controller->running;
        } else {
            co_controller->tcbs[sem->tail].next_blocked_on_same_event = co_controller->running;
            sem->tail = co_controller->running;
        }
        internal_go_next();
        return co_no_err;
    }
}

co_err_t microkit_cothread_semaphore_wait(microkit_cothread_sem_t *sem) {
    co_controller->tcbs[co_controller->running].state = cothread_blocked_on_sem;
    return internal_sem_wait(sem);
}

// Mark 1 cothread as ready when a semaphore is signed.
static inline bool internal_sem_mark_cothread_ready(microkit_cothread_t subject) {
    const int sched_err = hostedqueue_push(&co_controller->scheduling_queue, co_controller->scheduling_queue_mem, &subject);
    if (sched_err != LIBHOSTEDQUEUE_NOERR) {
        return false;
    }
    co_controller->tcbs[subject].state = cothread_ready;
    return true;
}

static inline bool internal_sem_do_signal(microkit_cothread_sem_t *sem) {
    microkit_cothread_t head = sem->head;
    const microkit_cothread_t next = co_controller->tcbs[head].next_blocked_on_same_event;

    if (!internal_sem_mark_cothread_ready(head)) {
        return false;
    } else {
        co_controller->tcbs[head].next_blocked_on_same_event = NULL_HANDLE;
        if (next != NULL_HANDLE) {
            sem->head = next;
        } else {
            microkit_cothread_semaphore_init(sem);
        }
    }
    return true;
}

co_err_t microkit_cothread_semaphore_signal_once(microkit_cothread_sem_t *sem) {
    if (sem->set) {
        return co_err_sem_sig_once_already_set;
    } else if (sem->head == NULL_HANDLE) {
        sem->set = true;
        return true;
    } else {
        return internal_sem_do_signal(sem) ? co_no_err : co_err_sem_sig_once_cannot_schedule_ready_cothread;
    }
}

co_err_t microkit_cothread_semaphore_signal_all(microkit_cothread_sem_t *sem) {
    if (sem->set) {
        return co_err_sem_sig_once_already_set;
    } else if (sem->head == NULL_HANDLE) {
        sem->set = true;
    } else {
        while (!microkit_cothread_semaphore_is_queue_empty(sem)) {
            bool success = internal_sem_do_signal(sem);
            if (!success) {
                return co_err_sem_sig_all_cannot_schedule_ready_cothreads;
            }
        }
    }

    return co_no_err;
}

bool microkit_cothread_semaphore_is_queue_empty(const microkit_cothread_sem_t *sem) {
    return sem->head == NULL_HANDLE;
}

bool microkit_cothread_semaphore_is_set(const microkit_cothread_sem_t *sem) {
    return sem->set;
}

// =========== Public functions ===========

co_err_t microkit_cothread_init(const uintptr_t controller_memory_addr, const size_t co_stack_size, ...) {
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
    co_controller->tcbs[0].next_blocked_on_same_event = NULL_HANDLE;
    co_controller->running = LIBMICROKITCO_ROOT_THREAD;

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
        co_controller->blocked_channel_map[i].head = NULL_HANDLE;
        co_controller->blocked_channel_map[i].tail = NULL_HANDLE;
        co_controller->blocked_channel_map[i].set = false;
    }

    return co_no_err;
}

co_err_t microkit_cothread_free_handle_available(bool *ret_flag, microkit_cothread_t *ret_handle) {
    *ret_flag = hostedqueue_peek(&co_controller->free_handle_queue, co_controller->free_handle_queue_mem, ret_handle) == LIBHOSTEDQUEUE_NOERR;
    return co_no_err;
}

co_err_t microkit_cothread_spawn(const client_entry_t client_entry, const uintptr_t private_arg, microkit_cothread_t *handle_ret) {
    if (!client_entry) {
        return co_err_spawn_client_entry_null;
    }

    hosted_queue_t *free_handle_queue = &co_controller->free_handle_queue;
    hosted_queue_t *scheduling_queue = &co_controller->scheduling_queue;
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
    co_controller->tcbs[new].state = cothread_ready;
    co_controller->tcbs[new].next_blocked_on_same_event = NULL_HANDLE;

    const int schedule_err = hostedqueue_push(scheduling_queue, co_controller->scheduling_queue_mem, &new);
    if (schedule_err != LIBHOSTEDQUEUE_NOERR) {
        return co_err_spawn_cannot_schedule;
        hostedqueue_push(free_handle_queue, co_controller->free_handle_queue_mem, &new);
    }

    *handle_ret = new;
    return co_no_err;
}

co_err_t microkit_cothread_set_arg(const microkit_cothread_t cothread, const uintptr_t private_arg) {
    if (co_controller->running == 0) {
        return co_err_my_arg_called_from_root_thread;
    }
    if (cothread >= MAX_THREADS || cothread < 0 || co_controller->tcbs[cothread].state == cothread_not_active) {
        return co_err_generic_invalid_handle;
    }

    co_controller->tcbs[cothread].private_arg = private_arg;
    return co_no_err;
}

co_err_t microkit_cothread_query_state(const microkit_cothread_t cothread, co_state_t *ret_state) {
    if (cothread >= MAX_THREADS || cothread < 0) {
        return co_err_generic_invalid_handle;
    }

    *ret_state = co_controller->tcbs[cothread].state;
    return co_no_err;
}

co_err_t microkit_cothread_my_handle(microkit_cothread_t *ret_handle) {
    *ret_handle = co_controller->running;
    return co_no_err;
}

co_err_t microkit_cothread_my_arg(uintptr_t *ret_priv_arg) {
    if (co_controller->running == 0) {
        return co_err_my_arg_called_from_root_thread;
    }

    *ret_priv_arg = co_controller->tcbs[co_controller->running].private_arg;
    return co_no_err;
}

co_err_t microkit_cothread_yield(void) {
    // Caller get pushed onto the appropriate scheduling queue.
    hosted_queue_t *sched_queue = &co_controller->scheduling_queue;
    const int sched_err = hostedqueue_push(sched_queue, co_controller->scheduling_queue_mem, &co_controller->running);
    if (sched_err != LIBHOSTEDQUEUE_NOERR) {
        return co_err_yield_cannot_schedule_caller;
    }
    co_controller->tcbs[co_controller->running].state = cothread_ready;

    // If the scheduling queues are empty beforehand, the caller just get runned again.
    internal_go_next();

    return co_no_err;
}

co_err_t microkit_cothread_destroy(const microkit_cothread_t cothread) {
    if (cothread >= MAX_THREADS || cothread < 0) {
        return co_err_generic_invalid_handle;
    }

    if (co_controller->running == 0) {
        // cannot destroy root thread
        return co_err_destroy_cannot_destroy_root;
    }

    if (hostedqueue_push(&co_controller->free_handle_queue, co_controller->free_handle_queue_mem, &cothread) != LIBHOSTEDQUEUE_NOERR) {
        return co_err_destroy_cannot_release_handle;
    }

    co_controller->tcbs[cothread].state = cothread_not_active;

    if (cothread == co_controller->running) {
        internal_go_next();
    }

    return co_no_err;
}

co_err_t microkit_cothread_wait_on_channel(const microkit_channel wake_on) {
    if (wake_on >= MICROKIT_MAX_CHANNELS) {
        return co_err_wait_invalid_channel;
    }

    co_controller->tcbs[co_controller->running].state = cothread_blocked_on_channel;
    return internal_sem_wait(&co_controller->blocked_channel_map[wake_on]);
}

co_err_t microkit_cothread_recv_ntfn(const microkit_channel ch) {
    if (co_controller->running != LIBMICROKITCO_ROOT_THREAD) {
        return co_err_recv_ntfn_called_from_non_root;
    }

    microkit_cothread_sem_t* ch_sem = &co_controller->blocked_channel_map[ch];
    co_err_t err;

    if ((err = microkit_cothread_semaphore_signal_all(ch_sem)) != co_no_err) {
        return err;
    }

    return co_no_err;
}
