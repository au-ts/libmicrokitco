/*
 * Copyright 2024, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <stddef.h>
#include <stdbool.h>

#include <libmicrokitco_opts.h>

// Cothread handle.
typedef int microkit_cothread_ref_t;
#include "libhostedqueue/libhostedqueue.h"

// This err is caught by the provided Makefile so we should never trigger this. But it's included
// in case the client want to compile the library manually.
#ifndef LIBMICROKITCO_MAX_COTHREADS
#error "libmicrokitco: max_cothreads must be known at compile time."
#endif

// No point to use this library if your max cothread is 0.
#if LIBMICROKITCO_MAX_COTHREADS < 1
#error "libmicrokitco: max_cothreads must be greater or equal to 1."
#endif

// MAX_THREADS includes the root thread whereas LIBMICROKITCO_MAX_COTHREADS does not
#define MAX_THREADS LIBMICROKITCO_MAX_COTHREADS + 1

// Business logic

// ========== BEGIN ERROR HANDLING SECTION ==========

// Error numbers and their meanings
typedef enum {
    co_no_err = 0,
    co_err_generic_invalid_handle,

    co_err_init_already_initialised,
    co_err_init_stack_too_small,
    co_err_init_num_costacks_not_equal_defined,
    co_err_init_co_stack_null,
    co_err_init_co_stack_overlap,
    co_err_init_free_handles_init_fail,
    co_err_init_sched_init_fail,
    co_err_init_free_handles_populate_fail,

    co_err_my_arg_called_from_root_thread,

    co_err_spawn_client_entry_null,
    co_err_spawn_max_cothreads_reached,
    co_err_spawn_cannot_schedule,

    co_err_yield_cannot_schedule_caller,

    co_err_destroy_cannot_destroy_root,
    co_err_destroy_cannot_release_handle,

    co_err_sem_sig_once_cannot_schedule_caller,
    co_err_sem_sig_once_already_set,

    co_err_wait_invalid_channel,

    co_err_recv_ntfn_called_from_non_root,

    // this must be last to count how many errors combinations we have
    co_num_errors
} co_err_t;

// Returns a human friendly error message corresponding with the given err code.
const char *microkit_cothread_pretty_error(const co_err_t err_num);

// ========== END ERROR HANDLING SECTION ==========


// ========== BEGIN DATA TYPES SECTION ==========

// The form of client entrypoint function.
typedef void (*client_entry_t)(void);

typedef enum {
    // This id is not being used
    cothread_not_active = 0,

    cothread_blocked,
    cothread_ready,
    cothread_running,
} co_state_t;

typedef void *cothread_t;

typedef struct {
    // Thread local storage: context + stack
    void *local_storage;
    cothread_t co_handle;

    // Entrypoint for cothread
    client_entry_t client_entry;
    void *private_arg;

    // Current execution state
    co_state_t state;

    microkit_cothread_ref_t next_blocked_on_same_event;
} co_tcb_t;

// A linked list data structure that manage all cothreads blocking on a specific sem/event.
typedef struct {
    // True if the sem is signaled without any cothread waiting on it.
    bool set;

    // First cothread waiting on this semaphore
    microkit_cothread_ref_t head;
    microkit_cothread_ref_t tail;
} microkit_cothread_sem_t;

typedef struct cothreads_control {
    int co_stack_size;
    microkit_cothread_ref_t running;

    // Array of cothreads, first index is root thread AND len(tcbs) == (max_cothreads + 1)
    co_tcb_t tcbs[MAX_THREADS];

    // All of these are queues of `microkit_cothread_ref_t`
    hosted_queue_t free_handle_queue;
    hosted_queue_t scheduling_queue;

    // Arrays for queues.
    microkit_cothread_ref_t free_handle_queue_mem[MAX_THREADS];
    microkit_cothread_ref_t scheduling_queue_mem[MAX_THREADS];

    // Map of linked list on what cothreads are blocked on which channel.
    microkit_cothread_sem_t blocked_channel_map[MICROKIT_MAX_CHANNELS];
} co_control_t;

#define LIBMICROKITCO_CONTROLLER_SIZE sizeof(co_control_t)

// ========== END DATA TYPES SECTION ==========


// ========== BEGIN API SECTION ==========

co_err_t microkit_cothread_init(co_control_t *controller_memory_addr, const size_t co_stack_size, ...);

co_err_t microkit_cothread_free_handle_available(bool *ret_flag, microkit_cothread_ref_t *ret_handle);

co_err_t microkit_cothread_spawn(const client_entry_t client_entry, void *private_arg, microkit_cothread_ref_t *handle_ret);

co_err_t microkit_cothread_set_arg(const microkit_cothread_ref_t cothread, void *private_arg);

co_err_t microkit_cothread_query_state(const microkit_cothread_ref_t cothread, co_state_t *ret_state);

co_err_t microkit_cothread_my_handle(microkit_cothread_ref_t *ret_handle);

co_err_t microkit_cothread_my_arg(void **ret_priv_arg);

co_err_t microkit_cothread_yield(void);

co_err_t microkit_cothread_destroy(const microkit_cothread_ref_t cothread);

// Generic blocking mechanism: a userland semaphore
co_err_t microkit_cothread_semaphore_init(microkit_cothread_sem_t *ret_sem);
co_err_t microkit_cothread_semaphore_wait(microkit_cothread_sem_t *sem);
co_err_t microkit_cothread_semaphore_signal(microkit_cothread_sem_t *sem);
bool microkit_cothread_semaphore_is_queue_empty(const microkit_cothread_sem_t *sem);
bool microkit_cothread_semaphore_is_set(const microkit_cothread_sem_t *sem);

// Microkit specific semaphore wrapper: blocking on channel
co_err_t microkit_cothread_wait_on_channel(const microkit_channel wake_on); 
co_err_t microkit_cothread_recv_ntfn(const microkit_channel ch);

// ========== END API SECTION ==========
