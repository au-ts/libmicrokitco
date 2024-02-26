#include <microkit.h>
#include <stdint.h>

#include <libmicrokitco.h>
#include "coallocator.h"

extern char libmicrokitco_backing_mem[];

typedef enum cothread_state {
    cothread_blocked = 0,
    cothread_ready = 1,
    cothread_running = 2,
} co_state_t;

typedef struct cothread_tcb {
    co_state_t state;

    // This is checked if the thread is blocked.
    microkit_cothread_t blocked_on;

    void *stack_memory;
} co_tcb_t;

struct cothreads_control {
    int max_cothreads;

    // memory manager
    allocator_t mem_allocator;

    // scheduling stuff...
};

int microkit_cothread_init(void *backing_memory, uint64_t mem_size, int max_cothreads, co_control_t *co_controller) {
    if (!backing_memory || !max_cothreads) {
        return 1;
    }

    // TODO, check that the backing memory is enough to house all the cothreads stack and supporting DS.
    //       dont need to check that the memory is proper size cause the allocator checks for it.

    if (allocator_init(backing_memory, mem_size, &co_controller->mem_allocator) != 0) {
        return 1;
    }

    co_controller->max_cothreads = max_cothreads;

    // TODO, figure out how to structure the memory:
}

microkit_cothread_t microkit_cothread_spawn(void (*cothread_entrypoint)(void)) {

}

void microkit_cothread_switch(microkit_cothread_t cothread) {

}

void microkit_cothread_wait(microkit_channel wake_on) {

}

void microkit_cothread_yield() {

}

void microkit_cothread_destroy() {

}

void microkit_cothread_destroy_me() {

}