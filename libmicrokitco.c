#include <microkit.h>
#include <stdint.h>

#include "libmicrokitco.h"
#include "coallocator.h"
#include "libhostedstack/libhostedstack.h"
#include "./libco/libco.h"

typedef enum cothread_state {
    // this id is not being used
    cothread_not_active = 0,

    cothread_blocked = 1,
    cothread_ready = 2,
    cothread_running = 3,
} co_state_t;

typedef struct {
    cothread_t cothread;

    co_state_t state;
    microkit_channel blocked_on;

    unsigned char stack_memory[DEFAULT_COSTACK_SIZE];
} co_tcb_t;

// "TCB" of the thread that called microkit_cothread_init()
typedef struct {
    cothread_t cothread;
    co_state_t state;
    microkit_channel blocked_on;
} tcb_t;

struct cothreads_control {
    int max_cothreads;
    // exclusive of root thread
    int num_cothreads;
    microkit_cothread_t running;

    // memory manager
    allocator_t mem_allocator;

    // root thread that called microkit_cothread_init()
    tcb_t root;
    microkit_channel root_blocked_on;
    // array of cothreads
    co_tcb_t* tcbs;

    // stack of free cothread handles
    hosted_stack_t handle_stack;
    // circular queue for scheduling
    // TODO
};

int microkit_cothread_init(void *backing_memory, unsigned int mem_size, unsigned int max_cothreads, co_control_t *co_controller){
    if (!backing_memory || !max_cothreads) {
        return MICROKITCO_ERR_INIT_INVALID_ARGS;
    }

    co_controller->num_cothreads = 0;
    co_controller->max_cothreads = max_cothreads;

    // memory allocator for the library
    if (co_allocator_init(backing_memory, mem_size, &co_controller->mem_allocator) != 0) {
        return MICROKITCO_ERR_INIT_MEMALLOC_INIT_FAIL;
    }

    // allocate all the buffers we could possibly need, also checks if backing_memory is large enough
    void *tcbs_array = co_controller->mem_allocator.alloc(&co_controller->mem_allocator, sizeof(co_tcb_t) * max_cothreads);
    void *stack_memory = co_controller->mem_allocator.alloc(&co_controller->mem_allocator, sizeof(microkit_cothread_t) * max_cothreads);
    void *queue_memory = co_controller->mem_allocator.alloc(&co_controller->mem_allocator, sizeof(microkit_cothread_t) * max_cothreads);

    // not enough memory;
    if (!tcbs_array || !stack_memory || !queue_memory) {
        return MICROKITCO_ERR_INIT_NOT_ENOUGH_MEMORY;
    }
    co_controller->tcbs = tcbs_array;

    // initialise the root thread's handle;
    co_controller->root.cothread = co_active();
    co_controller->root.state = cothread_running;

    // initialise our free id stack
    int err = hostedstack_init(&co_controller->handle_stack, stack_memory, sizeof(microkit_cothread_t), max_cothreads);
    if (err != LIBHOSTEDSTACK_NOERR) {
        return MICROKITCO_ERR_INIT_STACK_INIT_ERR;
    }

    // initialise our scheduling queue

    return MICROKITCO_NOERR;
}

microkit_cothread_t microkit_cothread_spawn(void (*cothread_entrypoint)(void), co_control_t *co_controller) {
    return -1;
}

void microkit_cothread_switch(microkit_cothread_t cothread, co_control_t *co_controller) {
    if (cothread >= co_controller->max_cothreads) {
        panic();
    }

    if (cothread == -1) {
        // No ready thread.
        // TODO
    }

    co_controller->tcbs[co_controller->running].state = cothread_ready;
    co_controller->tcbs[cothread].state = cothread_running;
    co_controller->running = cothread;
    co_switch(co_controller->tcbs[cothread].cothread);
}

// Pick a ready thread
microkit_cothread_t microkit_cothread_schedule(co_control_t *co_controller) {
    return -1;

    // pop scheduling queue
}

void microkit_cothread_wait(microkit_channel wake_on, co_control_t *co_controller) {
    co_controller->tcbs[co_controller->running].state = cothread_blocked;
    co_controller->tcbs[co_controller->running].blocked_on = wake_on;
    microkit_channel next = microkit_cothread_schedule(co_controller);
    microkit_cothread_switch(next, co_controller);
}

void microkit_cothread_yield(co_control_t *co_controller) {
    microkit_channel next = microkit_cothread_schedule(co_controller);
    co_controller->tcbs[co_controller->running].state = cothread_ready;
    microkit_cothread_switch(next, co_controller);
}

void microkit_cothread_destroy(co_control_t *co_controller) {

}

// umm???
void microkit_cothread_destroy_me(co_control_t *co_controller) {

}