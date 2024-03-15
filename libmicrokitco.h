#pragma once

#define MICROKITCO_NOERR 0

#define MICROKITCO_ERR_INVALID_ARGS -1
#define MICROKITCO_ERR_INVALID_HANDLE -2
#define MICROKITCO_ERR_NOT_INITIALISED -3
#define MICROKITCO_ERR_NOMEM -4
#define MICROKITCO_ERR_OP_FAIL -5
#define MICROKITCO_ERR_DEST_NOT_READY -6
#define MICROKITCO_ERR_MAX_COTHREADS_REACHED -7
#define MICROKITCO_ERR_ALREADY_INITIALISED -8

typedef int microkit_cothread_t;
typedef struct cothreads_control co_control_t;
typedef int co_err_t;

co_err_t microkit_cothread_init(uintptr_t controller_memory, int co_stack_size, int max_cothreads, ...);

co_err_t microkit_cothread_recv_ntfn(microkit_channel ch);

co_err_t microkit_cothread_prioritise(microkit_cothread_t subject);
co_err_t microkit_cothread_deprioritise(microkit_cothread_t subject);

co_err_t microkit_cothread_spawn(void (*entry)(void), int prioritised, int ready, microkit_cothread_t *ret);

co_err_t microkit_cothread_mark_ready(microkit_cothread_t cothread);

co_err_t microkit_cothread_switch(microkit_cothread_t cothread);

// @billn: this needs some rethinking for case where client waits on an async api that fires a callback.
co_err_t microkit_cothread_wait(microkit_channel wake_on);

void microkit_cothread_yield();

void microkit_cothread_destroy_me();

co_err_t microkit_cothread_destroy_specific(microkit_cothread_t cothread);
