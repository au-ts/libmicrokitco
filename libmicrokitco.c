#include <microkit.h>
#include <stdint.h>

#include "libmicrokitco.h"
#include "coallocator.h"
#include "libhostedqueue/libhostedqueue.h"
#include "libco/libco.h"

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

    // non-zero means prioritised
    int prioritised;

    unsigned char stack_memory[DEFAULT_COSTACK_SIZE];
} co_tcb_t;

// "TCB" of the thread that called microkit_cothread_init(), called the root thread.
typedef struct {
    cothread_t cothread;
    co_state_t state;
    microkit_channel blocked_on;
    int prioritised;
} root_tcb_t;

struct cothreads_control {
    int max_cothreads;

    // active cothreads, exclusive of root thread
    int num_cothreads;

    // currently running cothread handle
    microkit_cothread_t running;

    // root thread that called microkit_cothread_init()
    root_tcb_t root;

    allocator_t mem_allocator;

    // array of cothreads, first index is left unused to signify root thread
    co_tcb_t* tcbs;

    // all of these are queues of `microkit_cothread_t`
    // queue of free cothread handles
    hosted_queue_t free_handle_queue;
    // queues for scheduling
    hosted_queue_t priority_queue;
    hosted_queue_t non_priority_queue;

    int init_success;
};

int microkit_cothread_init(void *backing_memory, unsigned int mem_size, int max_cothreads, co_control_t *co_controller){
    co_controller->init_success = 0;

    if (!backing_memory || max_cothreads <= 1) {
        return MICROKITCO_ERR_INVALID_ARGS;
    }

    co_controller->num_cothreads = 0;
    co_controller->max_cothreads = max_cothreads;

    // memory allocator for the library
    if (co_allocator_init(backing_memory, mem_size, &co_controller->mem_allocator) != 0) {
        return MICROKITCO_ERR_NOMEM;
    }

    allocator_t *allocator = &co_controller->mem_allocator;

    // allocate all the buffers we could possibly need, also checks if backing_memory is large enough
    void *tcbs_array = allocator->alloc(allocator, sizeof(co_tcb_t) * max_cothreads);
    void *free_handle_queue_mem = allocator->alloc(allocator, sizeof(microkit_cothread_t) * max_cothreads);
    void *priority_queue_mem = allocator->alloc(allocator, sizeof(microkit_cothread_t) * max_cothreads);
    void *non_priority_queue_mem = allocator->alloc(allocator, sizeof(microkit_cothread_t) * max_cothreads);
    if (!tcbs_array || !free_handle_queue_mem || !priority_queue_mem || !non_priority_queue_mem) {
        return MICROKITCO_ERR_NOMEM;
    }

    memzero(tcbs_array, sizeof(co_tcb_t) * max_cothreads);
    co_controller->tcbs = tcbs_array;

    // initialise the root thread's handle;
    co_controller->root.cothread = co_active();
    co_controller->root.state = cothread_running;
    co_controller->root.prioritised = 1;

    // initialise the queues
    int err = hostedqueue_init(&co_controller->free_handle_queue, free_handle_queue_mem, sizeof(microkit_cothread_t), max_cothreads);
    if (err != LIBHOSTEDQUEUE_ERR_INVALID_ARGS) {
        return MICROKITCO_ERR_OP_FAIL;
    }
    err = hostedqueue_init(&co_controller->priority_queue, priority_queue_mem, sizeof(microkit_cothread_t), max_cothreads);
    if (err != LIBHOSTEDQUEUE_ERR_INVALID_ARGS) {
        return MICROKITCO_ERR_OP_FAIL;
    }
    err = hostedqueue_init(&co_controller->non_priority_queue, non_priority_queue_mem, sizeof(microkit_cothread_t), max_cothreads);
    if (err != LIBHOSTEDQUEUE_ERR_INVALID_ARGS) {
        return MICROKITCO_ERR_OP_FAIL;
    }

    // enqueue all the free IDs
    for (microkit_cothread_t i = 1; i < max_cothreads; i++) {
        if (hostedqueue_push(&co_controller->free_handle_queue, &i) != LIBHOSTEDQUEUE_NOERR) {
            return MICROKITCO_ERR_OP_FAIL;
        }
    }

    co_controller->init_success = 1;
    return MICROKITCO_NOERR;
}

microkit_cothread_t microkit_cothread_spawn(void (*cothread_entrypoint)(void), int prioritised, co_control_t *co_controller) {
    if (!co_controller->init_success) {
        return MICROKITCO_ERR_NOT_INITIALISED;
    }
    if (!cothread_entrypoint) {
        return MICROKITCO_ERR_INVALID_ARGS;
    }

    hosted_queue_t *free_handle_queue = &co_controller->free_handle_queue;
    microkit_cothread_t new;
    int pop_err = hostedqueue_pop(free_handle_queue, &new);

    if (pop_err != LIBHOSTEDQUEUE_NOERR) {
        return MICROKITCO_ERR_MAX_COTHREADS_REACHED;
    }

    unsigned char *costack = co_controller->tcbs[new].stack_memory;
    memzero(costack, DEFAULT_COSTACK_SIZE);
    co_controller->tcbs[new].cothread = co_derive(costack, DEFAULT_COSTACK_SIZE, cothread_entrypoint);
    co_controller->tcbs[new].prioritised = prioritised;
    co_controller->tcbs[new].state = cothread_ready;

    return new;
}

int microkit_cothread_switch(microkit_cothread_t cothread, co_control_t *co_controller) {
    if (!co_controller->init_success) {
        return MICROKITCO_ERR_NOT_INITIALISED;
    }
    if (cothread >= co_controller->max_cothreads || cothread < 0) {
        return MICROKITCO_ERR_INVALID_HANDLE;
    }
    if (cothread == co_controller->running) {
        return MICROKITCO_ERR_INVALID_HANDLE;
    }
    if (co_controller->tcbs[cothread].state == cothread_not_active) {
        return MICROKITCO_ERR_INVALID_HANDLE;
    }

    if (co_controller->tcbs[cothread].state == cothread_blocked) {
        return MICROKITCO_ERR_DEST_BLOCKED;
    } else {
        co_controller->tcbs[co_controller->running].state = cothread_ready;
        co_controller->tcbs[cothread].state = cothread_running;
        co_controller->running = cothread;
        co_switch(co_controller->tcbs[cothread].cothread);
        return MICROKITCO_NOERR;
    }
}

// pop a ready thread from a given scheduling queue, discard any destroyed thread 
// that result from microkit_cothread_destroy_specific()
microkit_cothread_t internal_pop_from_queue(hosted_queue_t *sched_queue, co_tcb_t* tcbs) {
    while (true) {
        microkit_cothread_t next_choice;
        int peek_err = hostedqueue_pop(sched_queue, &next_choice);
        if (peek_err == LIBHOSTEDQUEUE_ERR_EMPTY) {
            return -1;
        } else if (peek_err == LIBHOSTEDQUEUE_NOERR) {
            // queue not empty
            if (tcbs[next_choice].state == cothread_not_active) {
                // head thread has been destroyed, continue looking.
                continue;
            } else {
                // got a ready thread!
                return next_choice;
            }
        } else {
            // catch other errs
            return -1;
        }
    }
}
// Pick a ready thread
microkit_cothread_t internal_schedule(co_control_t *co_controller) {
    microkit_cothread_t next;

    hosted_queue_t *priority_queue = &co_controller->priority_queue;
    hosted_queue_t *non_priority_queue = &co_controller->non_priority_queue;
    co_tcb_t* tcbs = co_controller->tcbs;

    next = internal_pop_from_queue(priority_queue, tcbs);
    if (next == -1) {
        next = internal_pop_from_queue(non_priority_queue, tcbs);
    }

    return next;
}

void microkit_cothread_wait(microkit_channel wake_on, co_control_t *co_controller) {
    co_controller->tcbs[co_controller->running].state = cothread_blocked;
    co_controller->tcbs[co_controller->running].blocked_on = wake_on;
    microkit_channel next = internal_schedule(co_controller);
    microkit_cothread_switch(next, co_controller);
}

void microkit_cothread_yield(co_control_t *co_controller) {
    microkit_channel next = internal_schedule(co_controller);
    co_controller->tcbs[co_controller->running].state = cothread_ready;
    microkit_cothread_switch(next, co_controller);
}

void microkit_cothread_destroy(co_control_t *co_controller) {

}

// umm???
void microkit_cothread_destroy_me(co_control_t *co_controller) {

}