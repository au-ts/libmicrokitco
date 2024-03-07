#pragma once

#define DEFAULT_COSTACK_SIZE 0x2000

#define MICROKITCO_NOERR 0

#define MICROKITCO_ERR_INVALID_ARGS -1
#define MICROKITCO_ERR_INVALID_HANDLE -2
#define MICROKITCO_ERR_NOT_INITIALISED -3
#define MICROKITCO_ERR_NOMEM -4
#define MICROKITCO_ERR_OP_FAIL -5
#define MICROKITCO_ERR_DEST_NOT_READY -6
#define MICROKITCO_ERR_MAX_COTHREADS_REACHED -7

typedef int microkit_cothread_t;
typedef struct cothreads_control co_control_t;

// A cooperative multithreading library with 2-tier scheduling for use within Microkit.
// In essence, it allow mapping of multiple executing contexts (cothreads) into 1 protection domain.
// Then, each cothread can wait for an incoming notification from a channel, while it is waiting, other
// cothreads can execute.

// A cothread can be "prioritised" or not. All ready "prioritised" cothreads are picked to execute in round robin
// before non-prioritised cothreads get picked on.
// Cothreads should yield judiciously to ensure other cothreads are not starved.
// The library expects a large memory region (MR) allocated to it, see microkit_cothread_init().

// Unsafe: define `LIBMICROKITCO_UNSAFE` macro in your preprocessor to skip most pedantic error checking.


// Initialise the library's internal data structure.
// Will fail if max_cothreads is negative OR co_controller is null OR backing_memory is not at least
// (sizeof(co_tcb_t) * max_cothreads) + 3*(sizeof(microkit_cothread_t) * max_cothreads) bytes large!

// max_cothreads will be inclusive of the calling thread
// By default, the calling thread will be prioritised in scheduling.
// Return MICROKITCO_NOERR on success. 
int microkit_cothread_init(void *backing_memory, unsigned int mem_size, int max_cothreads, co_control_t *co_controller);

// IMPORTANT: Put this in your notified() to map incoming notifications to waiting cothreads!
// NOTE: This library will only return to the microkit main loop for receiving notification when there is no ready cothread.
// Returns MICROKITCO_ERR_OP_FAIL if the notification couldn't be mapped onto a waiting cothread.
int microkit_cothread_recv_ntfn(microkit_channel ch, co_control_t *co_controller);

// Allow client to select which scheduling queue a cothread is placed in. 
// No immediate effect if the cothread is already scheduled in a queue. 
int microkit_cothread_prioritise(microkit_cothread_t subject, co_control_t *co_controller);
int microkit_cothread_deprioritise(microkit_cothread_t subject, co_control_t *co_controller);

// Create a new cothread, which is prioritised in scheduling if `prioritised` is non-zero.
// Argument passing is handled with global variables.
// Returns a cothread handle.
// Returns -1 on error, e.g. max_cothreads reached.
microkit_cothread_t microkit_cothread_spawn(void (*cothread_entrypoint)(void), int prioritised, co_control_t *co_controller);

// Explicitly switch to a another cothread.
// If the destination is not in a ready state, returns MICROKITCO_ERR_DEST_NOT_READY or MICROKITCO_ERR_INVALID_HANDLE.
// Returns MICROKITCO_NOERR when the switch was successful AND the scheduler picked the calling thread in the future.
int microkit_cothread_switch(microkit_cothread_t cothread, co_control_t *co_controller);

// A co_switch() in disguise that switches to the next ready thread, if no thread is ready, switch to the root executing
// context for receiving notifications from the microkit main loop.

// Then for the calling cothread, register a blocked state within the library's internal data structure for this cothread.
void microkit_cothread_wait(microkit_channel wake_on, co_control_t *co_controller);

// Invoke the scheduler as described, but the calling cothread is switched to ready instead of blocked.
// However, if there is no ready cothread, the caller keeps running.
void microkit_cothread_yield(co_control_t *co_controller);

// Destroys the calling cothread and return the cothread's local memory into the available pool.
// Then invokes the scheduler to run the next cothread.
// Must be called when a coroutine finishes, Undefined Behaviour otherwise!
void microkit_cothread_destroy_me(co_control_t *co_controller);
// Should be sparingly used, because cothread might hold resources that needs free'ing.
int microkit_cothread_destroy_specific(microkit_cothread_t cothread, co_control_t *co_controller);

// Food for thoughts on improvements:
// HIGH PRIO - stack canary

// - Current memory model problematic for stack overrun since all the cothreads' memory are next to each others.

// - Ceiling of max cothreads can't be raised or lowered at runtime. Although might be fine for static systems?
