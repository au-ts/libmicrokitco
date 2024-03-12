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

// Allow client to select which scheduling queue a cothread is placed in. 
// No immediate effect if the cothread is already scheduled in a queue. 
co_err_t microkit_cothread_prioritise(microkit_cothread_t subject);
co_err_t microkit_cothread_deprioritise(microkit_cothread_t subject);

co_err_t microkit_cothread_spawn(void (*entry)(void), int prioritised, int ready, microkit_cothread_t *ret);

co_err_t microkit_cothread_mark_ready(microkit_cothread_t cothread);

// Explicitly switch to a another cothread, bypassing the scheduler.
// If the destination is not in a ready state, returns MICROKITCO_ERR_DEST_NOT_READY or MICROKITCO_ERR_INVALID_HANDLE.
// Returns MICROKITCO_NOERR when the switch was successful AND the scheduler picked the calling thread in the future.
co_err_t microkit_cothread_switch(microkit_cothread_t cothread);

// A co_switch() in disguise that switches to the next ready thread, if no thread is ready, switch to the root executing
// context for receiving notifications from the microkit main loop.

co_err_t microkit_cothread_wait(microkit_channel wake_on);

void microkit_cothread_yield();

// Destroys the calling cothread and return the cothread's local memory into the available pool.
// Then invokes the scheduler to run the next cothread.
// Must be called when a coroutine finishes, Undefined Behaviour otherwise!
void microkit_cothread_destroy_me();
// Should be sparingly used, because cothread might hold resources that needs free'ing.
co_err_t microkit_cothread_destroy_specific(microkit_cothread_t cothread);
