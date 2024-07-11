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

void microkit_cothread_panic(uintptr_t err) {
    volatile char *fault_addr = (volatile char *) err;
    *fault_addr = 0;
}

#include <libco.h>

// Error handling
// On an unrecoverable error, such as bad argument or bugs in the implementation.
// libmicrokitco will crash the client PD with one of the following error code as
// the faulting virtual memory address.
typedef enum {
    reserved, // so that internal error code starts from 1 for easy identification.
    cannot_destroy_self_after_return,
    co_err_sem_sig_once_cannot_schedule_caller,
    destroy_cannot_destroy_root,
    destroy_cannot_release_handle,
    destroy_already_not_initialised,
    generic_invalid_handle,
    init_already_initialised,
    init_co_stack_null,
    init_co_stack_overlap,
    init_free_handles_init_fail,
    init_free_handles_populate_fail,
    init_num_costacks_not_equal_defined,
    init_sched_init_fail,
    init_stack_too_small,
    internal_pop_from_queue_cannot_pop,
    internal_pop_from_queue_found_non_ready_cothread_in_schedule_queue,
    my_arg_called_from_root,
    recv_ntfn_called_from_non_root_cothread,
    recv_ntfn_invalid_channel,
    spawn_cannot_schedule_new,
    spawn_client_entry_is_null,
    wait_on_channel_invalid_channel,
    yield_cannot_schedule_caller,
} internal_co_fatal_errors_t;

// =========== Business logic ===========

#define LIBMICROKITCO_ROOT_THREAD 0
#define MINIMUM_STACK_SIZE 0x1000 // Minimum is page size
#define SCHEDULER_NULL_CHOICE LIBMICROKITCO_NULL_HANDLE

// each PD can only have one "instance" of this library running.
static co_control_t *co_controller = NULL;

// =========== Helper functions ===========

// Pick a ready thread, essentially popping the first item from the scheduling queue.
static inline microkit_cothread_ref_t internal_schedule(void) {
    microkit_cothread_ref_t next_choice;
    const int peek_err = hostedqueue_pop(&co_controller->scheduling_queue, co_controller->scheduling_queue_mem, &next_choice);
    if (peek_err == LIBHOSTEDQUEUE_ERR_EMPTY) {
        return SCHEDULER_NULL_CHOICE;
    } else if (peek_err == LIBHOSTEDQUEUE_NOERR) {
        if (co_controller->tcbs[next_choice].state == cothread_ready || next_choice == LIBMICROKITCO_ROOT_THREAD) {
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
    microkit_cothread_ref_t next = internal_schedule();
    if (next == SCHEDULER_NULL_CHOICE) {
        // no ready thread in the queue, go back to root execution thread to receive notifications
        next = 0;
    }

    co_controller->tcbs[next].state = cothread_running;
    co_controller->running = next;
    co_switch(co_controller->tcbs[next].co_handle);
}

static inline void cothread_entry_wrapper(void) {
    // Execute the client entry point
    co_controller->tcbs[co_controller->running].client_entry();

    // Clean up after ourselves
    microkit_cothread_destroy(co_controller->running);

    // Should not return
    microkit_cothread_panic(cannot_destroy_self_after_return);
}

// =========== Semaphores ===========

void microkit_cothread_semaphore_init(microkit_cothread_sem_t *ret_sem) {
    ret_sem->head = LIBMICROKITCO_NULL_HANDLE;
    ret_sem->tail = LIBMICROKITCO_NULL_HANDLE;
    ret_sem->set = false;
}

void microkit_cothread_semaphore_wait(microkit_cothread_sem_t *sem) {
    if (sem->set) {
        sem->set = false;
    } else {
        co_controller->tcbs[co_controller->running].state = cothread_blocked;
        if (sem->head == LIBMICROKITCO_NULL_HANDLE) {
            sem->head = co_controller->running;
            sem->tail = co_controller->running;
        } else {
            co_controller->tcbs[sem->tail].next_blocked_on_same_event = co_controller->running;
            sem->tail = co_controller->running;
        }
        internal_go_next();
    }
}

void microkit_cothread_semaphore_signal(microkit_cothread_sem_t *sem) {
    if (microkit_cothread_semaphore_is_set(sem)) {
        return;
    }

    if (microkit_cothread_semaphore_is_queue_empty(sem)) {
        sem->set = true;
        return;
    }

    const microkit_cothread_ref_t head = sem->head;
    const microkit_cothread_ref_t next = co_controller->tcbs[head].next_blocked_on_same_event;

    // Schedule caller
    const int sched_err = hostedqueue_push(&co_controller->scheduling_queue, co_controller->scheduling_queue_mem, &co_controller->running);
    if (sched_err != LIBHOSTEDQUEUE_NOERR) {
        microkit_cothread_panic(co_err_sem_sig_once_cannot_schedule_caller);
    }

    // Move semaphore list
    sem->head = next;
    co_controller->tcbs[head].next_blocked_on_same_event = LIBMICROKITCO_NULL_HANDLE;
    
    if (next == LIBMICROKITCO_NULL_HANDLE) {
        // Reset semaphore if it's waiting queue is empty
        microkit_cothread_semaphore_init(sem);
    }

    // Directly switch to unblocked cothread
    co_controller->running = head;
    co_controller->tcbs[co_controller->running].state = cothread_running;
    co_switch(co_controller->tcbs[co_controller->running].co_handle);
}

bool microkit_cothread_semaphore_is_queue_empty(const microkit_cothread_sem_t *sem) {
    return sem->head == LIBMICROKITCO_NULL_HANDLE;
}

bool microkit_cothread_semaphore_is_set(const microkit_cothread_sem_t *sem) {
    return sem->set;
}

// =========== Public functions ===========

void microkit_cothread_init(co_control_t *controller_memory_addr, const size_t co_stack_size, ...) {
    if (co_controller != NULL) {
        microkit_cothread_panic(init_already_initialised);
    }
    if (co_stack_size < MINIMUM_STACK_SIZE) {
        microkit_cothread_panic(init_stack_too_small);
    }

    // This part will VMFault on write if the given memory is not large enough.
    memzero((void *) controller_memory_addr, LIBMICROKITCO_CONTROLLER_SIZE);
    co_controller = controller_memory_addr;
    co_controller->co_stack_size = co_stack_size;

    // Parses all the valid stack memory regions
    va_list ap;
    va_start(ap, co_stack_size);
    for (int i = 1; i < LIBMICROKITCO_MAX_COTHREADS; i++) {
        co_controller->tcbs[i].local_storage = (void *) va_arg(ap, uintptr_t);

        if (!co_controller->tcbs[i].local_storage) {
            microkit_cothread_panic(init_co_stack_null);
        }

        // sanity check the stacks, crash if stack not as big as we think
        // we only memzero the stack on cothread spawn.
        char *stack = (char *) co_controller->tcbs[i].local_storage;
        stack[0] = 0;
        stack[co_stack_size - 1] = 0;
    }
    va_end(ap);

    // Check that none of the stacks overlap
    for (int i = 1; i < LIBMICROKITCO_MAX_COTHREADS; i++) {
        uintptr_t this_stack_start = (uintptr_t) co_controller->tcbs[i].local_storage;
        uintptr_t this_stack_end = this_stack_start + co_stack_size - 1;

        for (int j = 1; j < LIBMICROKITCO_MAX_COTHREADS; j++) {
            if (j != i) {
                uintptr_t other_stack_start = (uintptr_t) co_controller->tcbs[j].local_storage;
                uintptr_t other_stack_end = other_stack_start + co_stack_size - 1;

                if (this_stack_start <= other_stack_end && other_stack_start <= this_stack_end) {
                    microkit_cothread_panic(init_co_stack_overlap);
                } 
            }
        }
    }

    // Initialise the root thread's handle;
    co_controller->tcbs[0].local_storage = NULL;
    co_controller->tcbs[0].co_handle = co_active();
    co_controller->tcbs[0].state = cothread_running;
    co_controller->tcbs[0].next_blocked_on_same_event = LIBMICROKITCO_NULL_HANDLE;
    co_controller->running = LIBMICROKITCO_ROOT_THREAD;

    // Initialise the queues
    const int err_hq = hostedqueue_init(
        &co_controller->free_handle_queue,
        LIBMICROKITCO_MAX_COTHREADS
    );
    const int err_sq = hostedqueue_init(
        &co_controller->scheduling_queue,
        LIBMICROKITCO_MAX_COTHREADS
    );

    if (err_hq != LIBHOSTEDQUEUE_NOERR) {
        microkit_cothread_panic(init_free_handles_init_fail);
    }
    if (err_sq != LIBHOSTEDQUEUE_NOERR) {
        microkit_cothread_panic(init_sched_init_fail);
    }

    // Enqueue all the free cothread handle IDs but exclude the root thread.
    for (microkit_cothread_ref_t i = 1; i < LIBMICROKITCO_MAX_COTHREADS; i++) {
        if (hostedqueue_push(&co_controller->free_handle_queue, co_controller->free_handle_queue_mem, &i) != LIBHOSTEDQUEUE_NOERR) {
            microkit_cothread_panic(init_free_handles_populate_fail);
        }
    }

    // Initialise the blocked table
    for (int i = 0; i < MICROKIT_MAX_CHANNELS; i++) {
        microkit_cothread_semaphore_init(&co_controller->blocked_channel_map[i]);
    }
}

bool microkit_cothread_free_handle_available(microkit_cothread_ref_t *ret_handle) {
    return hostedqueue_peek(&co_controller->free_handle_queue, co_controller->free_handle_queue_mem, ret_handle) == LIBHOSTEDQUEUE_NOERR;
}

microkit_cothread_ref_t microkit_cothread_spawn(const client_entry_t client_entry, void *private_arg) {
    if (!client_entry) {
        microkit_cothread_panic(spawn_client_entry_is_null);
    }

    hosted_queue_t *free_handle_queue = &co_controller->free_handle_queue;
    hosted_queue_t *scheduling_queue = &co_controller->scheduling_queue;
    microkit_cothread_ref_t new;
    const int pop_err = hostedqueue_pop(free_handle_queue, co_controller->free_handle_queue_mem, &new);

    if (pop_err != LIBHOSTEDQUEUE_NOERR) {
        return LIBMICROKITCO_NULL_HANDLE;
    }

    unsigned char *costack = (unsigned char *) co_controller->tcbs[new].local_storage;
    memzero(costack, co_controller->co_stack_size);
    co_controller->tcbs[new].client_entry = client_entry;
    co_controller->tcbs[new].private_arg = private_arg;
    co_controller->tcbs[new].co_handle = co_derive(costack, co_controller->co_stack_size, cothread_entry_wrapper);
    co_controller->tcbs[new].state = cothread_ready;
    co_controller->tcbs[new].next_blocked_on_same_event = LIBMICROKITCO_NULL_HANDLE;

    const int schedule_err = hostedqueue_push(scheduling_queue, co_controller->scheduling_queue_mem, &new);
    if (schedule_err != LIBHOSTEDQUEUE_NOERR) {
        microkit_cothread_panic(spawn_cannot_schedule_new);
        return LIBMICROKITCO_NULL_HANDLE;
    } else {
        return new;
    }
}

void microkit_cothread_set_arg(const microkit_cothread_ref_t cothread, void *private_arg) {
    if (cothread >= LIBMICROKITCO_MAX_COTHREADS || cothread < 0 || co_controller->tcbs[cothread].state == cothread_not_active) {
        microkit_cothread_panic(generic_invalid_handle);
    }

    co_controller->tcbs[cothread].private_arg = private_arg;
}

co_state_t microkit_cothread_query_state(const microkit_cothread_ref_t cothread) {
    if (cothread >= LIBMICROKITCO_MAX_COTHREADS || cothread < 0) {
        microkit_cothread_panic(generic_invalid_handle);
    }

    return co_controller->tcbs[cothread].state;
}

microkit_cothread_ref_t microkit_cothread_my_handle(void) {
    return co_controller->running;
}

void *microkit_cothread_my_arg(void) {
    if (co_controller->running == 0) {
        microkit_cothread_panic(my_arg_called_from_root);
    }

    return co_controller->tcbs[co_controller->running].private_arg;
}

void microkit_cothread_yield(void) {
    // Caller get pushed onto the appropriate scheduling queue.
    hosted_queue_t *sched_queue = &co_controller->scheduling_queue;
    const int sched_err = hostedqueue_push(sched_queue, co_controller->scheduling_queue_mem, &co_controller->running);
    if (sched_err != LIBHOSTEDQUEUE_NOERR) {
        microkit_cothread_panic(yield_cannot_schedule_caller);
    }
    
    co_controller->tcbs[co_controller->running].state = cothread_ready;

    // If the scheduling queues are empty beforehand, the caller just get runned again.
    internal_go_next();
}

void microkit_cothread_destroy(const microkit_cothread_ref_t cothread) {
    if (cothread >= LIBMICROKITCO_MAX_COTHREADS || cothread < 0) {
        microkit_cothread_panic(generic_invalid_handle);
    }

    if (co_controller->tcbs[cothread].state == cothread_not_active) {
        microkit_cothread_panic(destroy_already_not_initialised);
    }

    if (co_controller->running == 0) {
        // cannot destroy root thread
        microkit_cothread_panic(destroy_cannot_destroy_root);
    }

    if (hostedqueue_push(&co_controller->free_handle_queue, co_controller->free_handle_queue_mem, &cothread) != LIBHOSTEDQUEUE_NOERR) {
        microkit_cothread_panic(destroy_cannot_release_handle);
    } else {
        co_controller->tcbs[cothread].state = cothread_not_active;
        if (cothread == co_controller->running) {
            internal_go_next();
        }
    }
}

void microkit_cothread_wait_on_channel(const microkit_channel wake_on) {
    if (wake_on >= MICROKIT_MAX_CHANNELS) {
        microkit_cothread_panic(wait_on_channel_invalid_channel);
    }

    microkit_cothread_semaphore_wait(&co_controller->blocked_channel_map[wake_on]);
}

void microkit_cothread_recv_ntfn(const microkit_channel ch) {
    if (co_controller->running != LIBMICROKITCO_ROOT_THREAD) {
        microkit_cothread_panic(recv_ntfn_called_from_non_root_cothread);
    }
    if (ch >= MICROKIT_MAX_CHANNELS) {
        microkit_cothread_panic(recv_ntfn_invalid_channel);
    }

    microkit_cothread_semaphore_signal(&co_controller->blocked_channel_map[ch]);
}
