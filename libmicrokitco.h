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

// No point to use this library if you only have the root PD thread.
#if LIBMICROKITCO_MAX_COTHREADS < 2
#error "libmicrokitco: max_cothreads must be greater or equal to 2."
#endif

// ========== BEGIN DATA TYPES SECTION ==========

#define LIBMICROKITCO_NULL_HANDLE -1

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
    co_tcb_t tcbs[LIBMICROKITCO_MAX_COTHREADS];

    // All of these are queues of `microkit_cothread_ref_t`
    hosted_queue_t free_handle_queue;
    hosted_queue_t scheduling_queue;

    // Arrays for queues.
    microkit_cothread_ref_t free_handle_queue_mem[LIBMICROKITCO_MAX_COTHREADS];
    microkit_cothread_ref_t scheduling_queue_mem[LIBMICROKITCO_MAX_COTHREADS];

    // Map of linked list on what cothreads are blocked on which channel.
    microkit_cothread_sem_t blocked_channel_map[MICROKIT_MAX_CHANNELS];
} co_control_t;

#define LIBMICROKITCO_CONTROLLER_SIZE sizeof(co_control_t)

// ========== END DATA TYPES SECTION ==========


// ========== BEGIN API SECTION ==========

void microkit_cothread_init(co_control_t *controller_memory_addr, const size_t co_stack_size, ...);

bool microkit_cothread_free_handle_available(microkit_cothread_ref_t *ret_handle);

microkit_cothread_ref_t microkit_cothread_spawn(const client_entry_t client_entry, void *private_arg);

void microkit_cothread_set_arg(const microkit_cothread_ref_t cothread, void *private_arg);

co_state_t microkit_cothread_query_state(const microkit_cothread_ref_t cothread);

microkit_cothread_ref_t microkit_cothread_my_handle(void);

void *microkit_cothread_my_arg(void);

void microkit_cothread_yield(void);

void microkit_cothread_destroy(const microkit_cothread_ref_t cothread);

// Generic blocking mechanism: a userland semaphore
void microkit_cothread_semaphore_init(microkit_cothread_sem_t *ret_sem);
void microkit_cothread_semaphore_wait(microkit_cothread_sem_t *sem);
void microkit_cothread_semaphore_signal(microkit_cothread_sem_t *sem);
bool microkit_cothread_semaphore_is_queue_empty(const microkit_cothread_sem_t *sem);
bool microkit_cothread_semaphore_is_set(const microkit_cothread_sem_t *sem);

// Microkit specific semaphore wrapper: blocking on channel
void microkit_cothread_wait_on_channel(const microkit_channel wake_on); 
void microkit_cothread_recv_ntfn(const microkit_channel ch);

// ========== END API SECTION ==========
