#pragma once

// Error handling
typedef int co_err_t;

#define MICROKITCO_NOERR 0
#define MICROKITCO_ERR_INVALID_ARGS -1
#define MICROKITCO_ERR_INVALID_HANDLE -2
#define MICROKITCO_ERR_NOT_INITIALISED -3
#define MICROKITCO_ERR_NOMEM -4
#define MICROKITCO_ERR_OP_FAIL -5
#define MICROKITCO_ERR_DEST_NOT_READY -6
#define MICROKITCO_ERR_MAX_COTHREADS_REACHED -7
#define MICROKITCO_ERR_ALREADY_INITIALISED -8

#define ERR_COMBINATIONS 9

const char *err_strs[] = {
    "libmicrokitco: no error.\n",
    "libmicrokitco: invalid arguments.\n",
    "libmicrokitco: invalid cothread handle.\n",
    "libmicrokitco: library not initialised before usage.\n",
    "libmicrokitco: not enough memory internally.\n",
    "libmicrokitco: operation failed.\n", // catch all err
    "libmicrokitco: destination cothread not ready.\n",
    "libmicrokitco: cothreads ceiling reached.\n",
    "libmicrokitco: library already initialised.\n",
};

// Return a string of human friendly error message.
const char *microkit_cothread_pretty_error(co_err_t err_num) {
    int abs_err = err_num * -1;
    if (err_num > 0 || abs_err >= ERR_COMBINATIONS) {
        return "libmicrokitco: unknown error!\n";
    } else {
        return err_strs[abs_err];
    }
}


// Business logic
#define MICROKITCO_ROOT_THREAD 0

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

co_err_t microkit_cothread_spawn(void (*entry)(void), priority_level_t prioritised, ready_status_t ready, microkit_cothread_t *ret);

co_err_t microkit_cothread_mark_ready(microkit_cothread_t cothread);

co_err_t microkit_cothread_switch(microkit_cothread_t cothread);

// @billn: this needs some rethinking for case where client waits on an async api that fires a callback.
co_err_t microkit_cothread_wait(microkit_channel wake_on);

void microkit_cothread_yield();

void microkit_cothread_destroy_me();

co_err_t microkit_cothread_destroy_specific(microkit_cothread_t cothread);
