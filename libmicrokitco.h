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
typedef int microkit_cothread_t;
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

#ifdef LIBMICROKITCO_PREEMPTIVE_UNBLOCK
#define MAX_NTFN_QUEUE 1
#endif

// Business logic
#define MICROKITCO_ROOT_THREAD 0
#define MINIMUM_STACK_SIZE 0x1000 // Minimum is page size
#define SCHEDULER_NULL_CHOICE -1

// ========== BEGIN ERROR HANDLING SECTION ==========

// Error numbers and their meanings
typedef enum {
    co_no_err,

    co_err_generic_not_initialised = 0,
    co_err_generic_invalid_handle,

    co_err_init_already_initialised,
    co_err_init_stack_too_small,
    co_err_init_num_costacks_not_equal_defined,
    co_err_init_co_stack_null,
    co_err_init_free_handles_init_fail,
    co_err_init_sched_init_fail,
    co_err_init_free_handles_populate_fail,

    co_err_my_arg_called_from_root_thread,

    co_err_recv_ntfn_no_blocked,
    co_err_recv_ntfn_already_queued,

    co_err_spawn_client_entry_null,
    co_err_spawn_num_args_negative,
    co_err_spawn_num_args_too_much,
    co_err_spawn_max_cothreads_reached,
    co_err_spawn_cannot_schedule,

    co_err_mark_ready_already_ready,
    co_err_mark_ready_cannot_mark_self,
    co_err_mark_ready_cannot_schedule,

    co_err_wait_invalid_channel,

    co_err_yield_cannot_schedule_caller,

    co_err_destroy_specific_cannot_release_handle,
    co_err_destroy_specific_cannot_destroy_self,

    co_err_join_cannot_join_to_root_thread,
    co_err_join_deadlock_detected,

    // this must be last to count how many errors combinations we have
    co_num_errors
} co_err_t;

// Returns a human friendly error message corresponding with the given err code.
const char *microkit_cothread_pretty_error(const co_err_t err_num);

// ========== END ERROR HANDLING SECTION ==========


// ========== BEGIN DATA TYPES SECTION ==========

// The form of client entrypoint function.
typedef void (*client_entry_t)();

typedef enum {
    // This id is not being used
    cothread_not_active = 0,

    cothread_initialised, // but not ready, transition to ready with mark_ready()
    cothread_blocked, // generic block, client decides the logic of when the cothread wake up.
    cothread_blocked_on_channel,
    cothread_blocked_on_join,
    cothread_ready,
    cothread_running,
} co_state_t;

typedef struct {
    // Thread local storage: context + stack
    void* local_storage;

    // Entrypoint for cothread
    client_entry_t client_entry;
    uintptr_t private_arg;

    // Current execution state
    co_state_t state;

    // The next tcb that is blocked on the same channel or joined on the same cothread as this tcb.
    // -1 means this tcb is the tail.
    // Only checked when state == cothread_blocked_on_channel.
    microkit_cothread_t next_blocked_on_same_channel;

    // Data for join() operations:
    // Only checked when state == cothread_blocked_on_join.
    microkit_cothread_t next_joined_on_same_cothread;
    // The cothread that this cothread is waiting on to return (if applicable)
    microkit_cothread_t joined_on;
} co_tcb_t;

// A linked list data structure that manage all cothreads blocking on a specific channel or joining on another cothread.
typedef struct {

#ifdef LIBMICROKITCO_PREEMPTIVE_UNBLOCK
    // True if preemptive unblock is opted-in AND a notification came in without any cothreads blocking on it.
    unsigned queued;
#endif

    microkit_cothread_t head;
    microkit_cothread_t tail;
} blocked_list_t;

typedef struct cothreads_control {
    int co_stack_size;
    microkit_cothread_t running;

    // All of these are queues of `microkit_cothread_t`
    hosted_queue_t free_handle_queue;
    hosted_queue_t scheduling_queue;

    // Blocks of memory for our data structures:

    // Array of cothreads, first index is root thread AND len(tcbs) == (max_cothreads + 1)
    co_tcb_t tcbs[MAX_THREADS];

    // Arrays for queues.
    microkit_cothread_t free_handle_queue_mem[MAX_THREADS];
    microkit_cothread_t scheduling_queue_mem[MAX_THREADS];

    // Map of linked list on what cothreads are joined to which cothread.
    blocked_list_t joined_map[MAX_THREADS];

    // Map of linked list on what cothreads are blocked on which channel.
    blocked_list_t blocked_channel_map[MICROKIT_MAX_CHANNELS];
} co_control_t;

#define LIBMICROKITCO_CONTROLLER_SIZE sizeof(co_control_t)

// ========== END DATA TYPES SECTION ==========


// ========== BEGIN API SECTION ==========

co_err_t microkit_cothread_init(const uintptr_t controller_memory_addr, const size_t co_stack_size, ...);

co_err_t microkit_cothread_spawn(const client_entry_t client_entry, const bool ready, microkit_cothread_t *handle_ret, uintptr_t private_arg);

co_err_t microkit_cothread_query_state(const microkit_cothread_t cothread, co_state_t *ret_state);

co_err_t microkit_cothread_free_handle_available(bool *ret_flag);

co_err_t microkit_cothread_my_handle(microkit_cothread_t *ret_handle);

co_err_t microkit_cothread_my_arg(uintptr_t *private_arg);

co_err_t microkit_cothread_recv_ntfn(const microkit_channel ch);

co_err_t microkit_cothread_mark_ready(const microkit_cothread_t cothread);

co_err_t microkit_cothread_wait_on_channel(const microkit_channel wake_on);

co_err_t microkit_cothread_join(const microkit_cothread_t cothread);

co_err_t microkit_cothread_block();

co_err_t microkit_cothread_yield();

co_err_t microkit_cothread_destroy(const microkit_cothread_t cothread);

// ========== END API SECTION ==========
