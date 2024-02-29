#pragma once

#define DEFAULT_COSTACK_SIZE 0x2000
#define NULL (void *) 0

#define MICROKITCO_ERR_COTHREAD_HANDLE -1

#define MICROKITCO_NOERR 0

#define MICROKITCO_ERR_INIT_INVALID_ARGS 1
#define MICROKITCO_ERR_INIT_MEMALLOC_INIT_FAIL 2
#define MICROKITCO_ERR_INIT_NOT_ENOUGH_MEMORY 3

typedef int microkit_cothread_t;
typedef struct cothreads_control co_control_t;

void co_panic() {
    char *panic_addr = (char *) NULL;
    *panic_addr = (char) 0;
}

void co_memset(void *mem, unsigned int size) {
    for (unsigned int i = 0; i < size; i++) {
        ((char *) mem)[i] = NULL;
    }
}

// Initialise the library's internal data structure. 
// Will need at least a circular queue for scheduling (to be described later), a table for storing cothreads' state
// and a large buffer for the cothreads' local memory.
// Expect the backing_memory to be (sizeof(internal data structure) + sizeof(cothread local mem) * max_cothreads) bytes large.
// backing_memory can be allocated statically as an MR.

// Details of backing_memory size requirements TBD.

// Return 0 on success.
int microkit_cothread_init(void *backing_memory, unsigned int mem_size, unsigned int max_cothreads, co_control_t *co_controller);


// Wrapper around co_derive(). Main job is to allocate from the pool of cothread local memory.
// Does not jump to the cothread.
// Returns a cothread handle, which is an index into the state table.
// Returns -1 on error, e.g. max_cothreads reached.
microkit_cothread_t microkit_cothread_spawn(void (*cothread_entrypoint)(void), co_control_t *co_controller);

// Explicitly switch to a another cothread.
void microkit_cothread_switch(microkit_cothread_t cothread, co_control_t *co_controller);

// A co_switch() in disguise. However, before actually switching, it poll for 
// any pending notifications and map the received notification to blocked cothreads. If 
// the mapping is unsuccessful, notified() is invoked to handle the problem. 

// Then for the calling cothread, register a blocked state within the library's internal data structure for this cothread.
// Finally, switch to another ready cothread, picked in a round-robin manner, i.e., all cothread have equal priority.

// Thus this model places the onus on the developer to ensure each cothread runs in a short 
// amount of time before yielding to ensure responsiveness.
void microkit_cothread_wait(microkit_channel wake_on, co_control_t *co_controller);

// Invoke the scheduler as described, but the calling cothread is "ready" instead of blocked.
void microkit_cothread_yield(co_control_t *co_controller);

// Destroys the calling cothread and return the cothread's local memory into the available pool.
// Then does similar things to microkit_cothread_wait() and pick the next cothread.
void microkit_cothread_destroy_me(co_control_t *co_controller);

void microkit_cothread_destroy_specific(microkit_cothread_t cothread, co_control_t *co_controller);


// Food for thoughts on improvements:
// - Ceiling of max cothreads can't be raised or lowered at runtime. Although might be fine for static systems?

// - As per docs of the original libco used in LionsOS (https://github.com/higan-emu/libco/blob/master/doc/usage.md#co_create),
//   coroutines that returns will result in undefined behaviour. Which places another responsibility on programmers to end
//   cothread in Microkit with either a microkit_cothread_wait() or microkit_cothread_destroy().

// - Priority (with a max heap in scheduler, then round-robin for same priority) might be worth implementing? 
//   Or we can just have a 2 level "prioritised" and "normal"/
//   E.g. client need the connection loop in a web server to be snappy.

// - Current memory model problematic for stack overrun since all the cothreads' memory are next to each others.