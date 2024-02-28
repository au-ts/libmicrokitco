#include <microkit.h>
#include <stdint.h>

#include "libmicrokitco.h"
#include "coallocator.h"
#include "./libco/libco.h"

extern char libmicrokitco_backing_mem[];

typedef enum cothread_state {
    cothread_blocked = 0,
    cothread_ready = 1,
    cothread_running = 2,
} co_state_t;

typedef struct cothread_tcb {
    cothread_t cothread;

    co_state_t state;
    microkit_channel blocked_on;

    char stack_memory[DEFAULT_COSTACK_SIZE];
} co_tcb_t;

// "TCB" of the thread that called microkit_cothread_init()
typedef struct cothread_tcb {
    cothread_t cothread;
    co_state_t state;
    microkit_channel blocked_on;
} tcb_t;

struct cothreads_control {
    int max_cothreads;

    // memory manager
    allocator_t mem_allocator;

    microkit_cothread_t running;
    // thread that called microkit_cothread_init()
    tcb_t root;
    // array of cothreads
    co_tcb_t* tcbs;

    // stack of free cothread IDs/handles
    microkit_cothread_t *free;
    // point to the next available stack slot, so this is 0 if stack is empty.
    unsigned int stack_top;

    // circ queue for scheduling
    microkit_cothread_t *sched;
    unsigned int front;
    unsigned int back;
};

int microkit_cothread_init(void *backing_memory, unsigned int mem_size, unsigned int max_cothreads, co_control_t *co_controller){
    if (!backing_memory || !max_cothreads) {
        return 1;
    }

    // TODO, check that the backing memory is enough to house all the cothreads stack and supporting DS.
    //       dont need to check that the memory is proper size cause the allocator checks for it.
    // TODO, figure out memory layout
    if (allocator_init(backing_memory, mem_size, &co_controller->mem_allocator) != 0) {
        return 1;
    }

    co_controller->root.cothread = co_active();
    co_controller->root.state = cothread_running;

    co_controller->tcbs = co_controller->mem_allocator.alloc(&co_controller->mem_allocator, sizeof(co_tcb_t) * max_cothreads);
    co_controller->max_cothreads = max_cothreads;

    return 0;
}

microkit_cothread_t microkit_cothread_spawn(void (*cothread_entrypoint)(void), co_control_t *co_controller) {

    return -1;
}

void microkit_cothread_switch(microkit_cothread_t cothread, co_control_t *co_controller) {
    if (cothread >= co_controller->max_cothreads) {
        copanic();
    }
    co_controller->tcbs[co_controller->running].state = cothread_ready;
    co_controller->tcbs[cothread].state = cothread_running;
    co_controller->running = cothread;
    co_switch(co_controller->tcbs[cothread].cothread);
}

void microkit_cothread_wait(microkit_channel wake_on, co_control_t *co_controller) {

}

void microkit_cothread_yield(co_control_t *co_controller) {

}

void microkit_cothread_destroy(co_control_t *co_controller) {

}

void microkit_cothread_destroy_me(co_control_t *co_controller) {

}