#include <microkit.h>
#include <libco.h>

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
    void *memory;
} co_tcb_t;

struct cothreads_control {
    int max_cothreads;

    // memory manager
    allocator_t mem_allocator;

    // scheduling stuff...
};

int microkit_cothread_init(void *backing_memory, int max_cothreads, co_control_t *co_controller) {
    if (!backing_memory || !max_cothreads) {
        return -1;
    }

    co_controller->max_cothreads = max_cothreads;
    co_controller->backing_memory = backing_memory;

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