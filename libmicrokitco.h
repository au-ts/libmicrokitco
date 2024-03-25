#pragma once

#include <stddef.h>

// Error numbers and their meanings
typedef enum {
    co_no_err,

    co_err_generic_not_initialised,
    co_err_generic_invalid_handle,

    co_err_init_already_initialised,
    co_err_init_stack_too_small,
    co_err_init_max_cothreads_too_small,
    co_err_init_allocator_init_fail,
    co_err_init_tcbs_alloc_fail,
    co_err_init_free_handles_alloc_fail,
    co_err_init_prio_alloc_fail,
    co_err_init_non_prio_alloc_fail,
    co_err_init_co_stack_null,
    co_err_init_free_handles_init_fail,
    co_err_init_prio_init_fail,
    co_err_init_non_prio_init_fail,
    co_err_init_free_handles_populate_fail,

    co_err_recv_ntfn_no_blocked,

    // All of the below will not get thrown if you compile with LIBMICROKITCO_UNSAFE.
    co_err_spawn_client_entry_null,
    co_err_spawn_num_args_negative,
    co_err_spawn_num_args_too_much,
    co_err_spawn_max_cothreads_reached,
    co_err_spawn_cannot_schedule,

    co_err_get_arg_called_from_root,
    co_err_get_arg_nth_is_negative,
    co_err_get_arg_nth_is_greater_than_max,

    co_err_mark_ready_cannot_schedule,

    co_err_switch_to_self,

    co_err_wait_invalid_channel,

    co_err_yield_cannot_schedule_caller,

    co_err_destroy_specific_cannot_release_handle,

    // this must be last to "count" how many errors combinations we have
    co_num_errors
} co_err_t;

// Returns a human friendly error message corresponding with the given err code.
const char *microkit_cothread_pretty_error(co_err_t err_num);


// Business logic
#define MICROKITCO_ROOT_THREAD 0
#define MINIMUM_STACK_SIZE 0x1000
#define MAXIMUM_CO_ARGS 4

typedef void (*client_entry_t)(void);
typedef int microkit_cothread_t;
typedef struct cothreads_control co_control_t;

typedef enum {
    priority_false,
    priority_true
} priority_level_t;

typedef enum {
    ready_false,
    ready_true
} ready_status_t;

co_err_t microkit_cothread_init(uintptr_t controller_memory, int co_stack_size, int max_cothreads, ...);

co_err_t microkit_cothread_recv_ntfn(microkit_channel ch);

co_err_t microkit_cothread_prioritise(microkit_cothread_t subject);
co_err_t microkit_cothread_deprioritise(microkit_cothread_t subject);

co_err_t microkit_cothread_spawn(client_entry_t client_entry, priority_level_t prioritised, ready_status_t ready, microkit_cothread_t *ret, int num_args, ...);
co_err_t microkit_cothread_get_arg(int nth, size_t *ret);

co_err_t microkit_cothread_mark_ready(microkit_cothread_t cothread);

co_err_t microkit_cothread_switch(microkit_cothread_t cothread);

// @billn: this needs some rethinking for case where client waits on an async api that fires a callback.
co_err_t microkit_cothread_wait(microkit_channel wake_on);

void microkit_cothread_yield();

co_err_t microkit_cothread_destroy_specific(microkit_cothread_t cothread);

// race?
// co_err_t microkit_cothread_join(microkit_cothread_t cothread);
