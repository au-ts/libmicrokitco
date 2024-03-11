#include <microkit.h>
#include <stdint.h>
#include <stdarg.h>
#include <sel4/sel4.h>

#include "libmicrokitco.h"
#include "coallocator.h"
#include "libhostedqueue/libhostedqueue.h"
#include "libco/libco.h"

#define SCHEDULER_NULL_CHOICE -1
#define MINIMUM_STACK_SIZE 0x1000

typedef enum cothread_state {
    // this id is not being used
    cothread_not_active = 0,

    cothread_initialised = 1, // but not ready
    cothread_blocked = 2,
    cothread_ready = 3,
    cothread_running = 4,
} co_state_t;

typedef struct {
    cothread_t cothread;

    co_state_t state;
    microkit_channel blocked_on;

    // non-zero means prioritised
    int prioritised;

    uintptr_t stack_left;
} co_tcb_t;

struct cothreads_control {
    // array of cothreads, first index is root thread AND len(tcbs) == (max_cothreads + 1)
    co_tcb_t* tcbs;

    // both is inclusive of root thread!!!
    int max_cothreads;
    // active cothreads
    int num_cothreads;

    int co_stack_size;

    // currently running cothread handle
    microkit_cothread_t running;

    allocator_t mem_allocator;

    // all of these are queues of `microkit_cothread_t`
    // queue of free cothread handles
    hosted_queue_t free_handle_queue;
    // queues for scheduling
    hosted_queue_t priority_queue;
    hosted_queue_t non_priority_queue;

    int init_success;
};

static co_control_t co_controller = {
    .init_success = 0
};

co_err_t microkit_cothread_init(uintptr_t controller_memory, int co_stack_size, int max_cothreads, ...) {
    // Err checking cannot be disabled by the preprocessor define here for safety because this can only be ran once per PD.
    if (co_controller.init_success) {
        return MICROKITCO_ERR_ALREADY_INITIALISED;
    }
    if (co_stack_size < MINIMUM_STACK_SIZE || max_cothreads < 1) {
        return MICROKITCO_ERR_INVALID_ARGS;
    }

    co_controller.num_cothreads = 1;
    // Includes root thread.
    int real_max_cothreads = max_cothreads + 1;
    co_controller.max_cothreads = real_max_cothreads;

    co_controller.co_stack_size = co_stack_size;

    // memory allocator for the library
    unsigned int derived_mem_size = (sizeof(co_tcb_t) * real_max_cothreads) + ((sizeof(microkit_cothread_t) * 3) * real_max_cothreads);
    if (co_allocator_init((void *) controller_memory, derived_mem_size, &co_controller.mem_allocator) != 0) {
        return MICROKITCO_ERR_NOMEM;
    }
    allocator_t *allocator = &co_controller.mem_allocator;

    // allocate all the buffers we could possibly need, also checks if controller_memory is large enough
    void *tcbs_array = allocator->alloc(allocator, sizeof(co_tcb_t) * real_max_cothreads);
    void *free_handle_queue_mem = allocator->alloc(allocator, sizeof(microkit_cothread_t) * real_max_cothreads);
    void *priority_queue_mem = allocator->alloc(allocator, sizeof(microkit_cothread_t) * real_max_cothreads);
    void *non_priority_queue_mem = allocator->alloc(allocator, sizeof(microkit_cothread_t) * real_max_cothreads);
    if (!tcbs_array || !free_handle_queue_mem || !priority_queue_mem || !non_priority_queue_mem) {
        return MICROKITCO_ERR_NOMEM;
    }
    co_controller.tcbs = tcbs_array;
    for (int i = 0; i < real_max_cothreads; i++) {
        co_controller.tcbs[i].state = cothread_not_active;
    }

    // parses all the valid stack memory regions 
    // TODO, check that we actually have `max_cothreads` extra args.
    va_list ap;
    va_start (ap, max_cothreads);
    for (int i = 0; i < max_cothreads; i++) {
        co_controller.tcbs[i + 1].stack_left = va_arg(ap, uintptr_t);
    }
    va_end(ap);

    // initialise the root thread's handle;
    co_controller.tcbs[0].cothread = co_active();
    co_controller.tcbs[0].state = cothread_running;
    co_controller.tcbs[0].prioritised = 1;
    co_controller.tcbs[0].stack_left = 0;
    co_controller.running = 0;

    // initialise the queues
    int err = hostedqueue_init(&co_controller.free_handle_queue, free_handle_queue_mem, sizeof(microkit_cothread_t), real_max_cothreads);
    if (err != LIBHOSTEDQUEUE_NOERR) {
        return MICROKITCO_ERR_OP_FAIL;
    }
    err = hostedqueue_init(&co_controller.priority_queue, priority_queue_mem, sizeof(microkit_cothread_t), real_max_cothreads);
    if (err != LIBHOSTEDQUEUE_NOERR) {
        return MICROKITCO_ERR_OP_FAIL;
    }
    err = hostedqueue_init(&co_controller.non_priority_queue, non_priority_queue_mem, sizeof(microkit_cothread_t), real_max_cothreads);
    if (err != LIBHOSTEDQUEUE_NOERR) {
        return MICROKITCO_ERR_OP_FAIL;
    }

    // enqueue all the free cothread handle IDs. Exclude root thread.
    for (microkit_cothread_t i = 1; i < real_max_cothreads; i++) {
        if (hostedqueue_push(&co_controller.free_handle_queue, &i) != LIBHOSTEDQUEUE_NOERR) {
            return MICROKITCO_ERR_OP_FAIL;
        }
    }

    co_controller.init_success = 1;
    return MICROKITCO_NOERR;
}

co_err_t microkit_cothread_recv_ntfn(microkit_channel ch) {
    // TODO: this could be faster
    for (microkit_cothread_t i = 0; i < co_controller.max_cothreads; i++) {
        if (co_controller.tcbs[i].state == cothread_blocked) {
            if (co_controller.tcbs[i].blocked_on == ch) {
                co_controller.tcbs[i].state = cothread_ready;
                hosted_queue_t *sched_queue;
                if (!co_controller.tcbs[i].prioritised) {
                    sched_queue = &co_controller.non_priority_queue;
                } else {
                    sched_queue = &co_controller.priority_queue;
                }

                hostedqueue_push(sched_queue, &i);
                return MICROKITCO_NOERR;
            }
        }
    }
    return MICROKITCO_ERR_OP_FAIL;
}

co_err_t microkit_cothread_prioritise(microkit_cothread_t subject) {
    #if !defined(LIBMICROKITCO_UNSAFE)
        if (!co_controller.init_success) {
            return MICROKITCO_ERR_NOT_INITIALISED;
        }
        if (subject >= co_controller.max_cothreads || subject < 0) {
            return MICROKITCO_ERR_INVALID_HANDLE;
        }
    #endif

    co_controller.tcbs[subject].prioritised = 1;
    return MICROKITCO_NOERR;
}

co_err_t microkit_cothread_deprioritise(microkit_cothread_t subject) {
    #if !defined(LIBMICROKITCO_UNSAFE)
        if (!co_controller.init_success) {
            return MICROKITCO_ERR_NOT_INITIALISED;
        }
        if (subject >= co_controller.max_cothreads || subject < 0) {
            return MICROKITCO_ERR_INVALID_HANDLE;
        }
    #endif

    co_controller.tcbs[subject].prioritised = 0;
    return MICROKITCO_NOERR;
}

microkit_cothread_t microkit_cothread_spawn(void (*cothread_entrypoint)(void), int prioritised, int ready) {
    #if !defined LIBMICROKITCO_UNSAFE
        if (!co_controller.init_success) {
            return MICROKITCO_ERR_NOT_INITIALISED;
        }
        if (!cothread_entrypoint) {
            return MICROKITCO_ERR_INVALID_ARGS;
        }
    #endif

    hosted_queue_t *free_handle_queue = &co_controller.free_handle_queue;
    microkit_cothread_t new;
    int pop_err = hostedqueue_pop(free_handle_queue, &new);

    if (pop_err != LIBHOSTEDQUEUE_NOERR) {
        return MICROKITCO_ERR_MAX_COTHREADS_REACHED;
    }

    unsigned char *costack = (unsigned char *) co_controller.tcbs[new].stack_left;
    memzero(costack, co_controller.co_stack_size);
    co_controller.tcbs[new].cothread = co_derive(costack, co_controller.co_stack_size, cothread_entrypoint);
    co_controller.tcbs[new].prioritised = prioritised;
    co_controller.tcbs[new].state = cothread_initialised;

    hosted_queue_t *sched_queue;
    if (ready) {
        int err = microkit_cothread_mark_ready(new);
        if (err != MICROKITCO_NOERR) {
            co_controller.tcbs[new].state = cothread_not_active;
            return MICROKITCO_ERR_OP_FAIL;
        }
    }

    return new;
}

co_err_t microkit_cothread_mark_ready(microkit_cothread_t cothread) {
    #if !defined(LIBMICROKITCO_UNSAFE)
        if (!co_controller.init_success) {
            return MICROKITCO_ERR_NOT_INITIALISED;
        }
        if (cothread >= co_controller.max_cothreads || cothread < 0) {
            return MICROKITCO_ERR_INVALID_HANDLE;
        }
    #endif

    if (co_controller.tcbs[cothread].state != cothread_initialised) {
        return MICROKITCO_ERR_OP_FAIL;
    }

    hosted_queue_t *sched_queue;
    if (co_controller.tcbs[cothread].prioritised) {
        sched_queue = &co_controller.priority_queue;
    } else {
        sched_queue = &co_controller.non_priority_queue;
    }
    int push_err = hostedqueue_push(sched_queue, &cothread);
    co_controller.tcbs[cothread].state = cothread_ready;

    if (push_err != LIBHOSTEDQUEUE_NOERR) {
        return MICROKITCO_ERR_OP_FAIL;
    } else {
        return MICROKITCO_NOERR;
    }
}

co_err_t microkit_cothread_switch(microkit_cothread_t cothread) {
    #if !defined(LIBMICROKITCO_UNSAFE)
        if (!co_controller.init_success) {
            return MICROKITCO_ERR_NOT_INITIALISED;
        }
        if (cothread >= co_controller.max_cothreads || cothread < 0) {
            return MICROKITCO_ERR_INVALID_HANDLE;
        }
        if (cothread == co_controller.running) {
            return MICROKITCO_ERR_INVALID_HANDLE;
        }
        if (co_controller.tcbs[cothread].state == cothread_not_active) {
            return MICROKITCO_ERR_INVALID_HANDLE;
        }
    #endif

    co_controller.tcbs[co_controller.running].state = cothread_ready;
    hosted_queue_t *sched_queue;
    if (co_controller.tcbs[co_controller.running].prioritised) {
        sched_queue = &co_controller.priority_queue;
    } else {
        sched_queue = &co_controller.non_priority_queue;
    }
    int push_err = hostedqueue_push(sched_queue, &co_controller.running);

    co_controller.tcbs[cothread].state = cothread_running;
    co_controller.running = cothread;
    co_switch(co_controller.tcbs[cothread].cothread);
    return MICROKITCO_NOERR;
}

// pop a ready thread from a given scheduling queue, discard any destroyed thread 
microkit_cothread_t internal_pop_from_queue(hosted_queue_t *sched_queue, co_tcb_t* tcbs) {
    while (true) {
        microkit_cothread_t next_choice;
        int peek_err = hostedqueue_pop(sched_queue, &next_choice);
        if (peek_err == LIBHOSTEDQUEUE_ERR_EMPTY) {
            return SCHEDULER_NULL_CHOICE;
        } else if (peek_err == LIBHOSTEDQUEUE_NOERR) {
            // queue not empty
            if (tcbs[next_choice].state == cothread_ready) {
                return next_choice;
            }
        } else {
            // catch other errs
            panic();
        }
    }
}
// Pick a ready thread
microkit_cothread_t internal_schedule() {
    microkit_cothread_t next;

    hosted_queue_t *priority_queue = &co_controller.priority_queue;
    hosted_queue_t *non_priority_queue = &co_controller.non_priority_queue;
    co_tcb_t* tcbs = co_controller.tcbs;

    next = internal_pop_from_queue(priority_queue, tcbs);
    if (next == SCHEDULER_NULL_CHOICE) {
        next = internal_pop_from_queue(non_priority_queue, tcbs);
        // next can still be SCHEDULER_NULL_CHOICE!
    }

    return next;
}
// Switch to the next ready thread, also handle cases where there is no ready thread.
void internal_go_next() {
    microkit_cothread_t next = internal_schedule();
    if (next == SCHEDULER_NULL_CHOICE) {
        // no ready thread in the queue, go back to root execution thread to receive notifications
        next = 0;
    }

    co_controller.tcbs[next].state = cothread_running;
    co_controller.running = next;
    co_switch(co_controller.tcbs[next].cothread);
}

void microkit_cothread_wait(microkit_channel wake_on) {
    co_controller.tcbs[co_controller.running].state = cothread_blocked;
    co_controller.tcbs[co_controller.running].blocked_on = wake_on;
    internal_go_next();
}

void microkit_cothread_yield() {
    // Caller get pushed onto the appropriate scheduling queue.
    hosted_queue_t *sched_queue;
    if (co_controller.tcbs[co_controller.running].prioritised) {
        sched_queue = &co_controller.priority_queue;
    } else {
        sched_queue = &co_controller.non_priority_queue;
    }

    hostedqueue_push(sched_queue, &co_controller.running);
    co_controller.tcbs[co_controller.running].state = cothread_ready;

    // If the scheduling queues are empty beforehand, the caller just get runned again.
    internal_go_next();
}

void microkit_cothread_destroy_me() {
    int err = microkit_cothread_destroy_specific(co_controller.running);

    #if !defined(LIBMICROKITCO_UNSAFE)
        if (err != MICROKITCO_NOERR) {
            panic();
        }
    #endif

    internal_go_next();    
}

co_err_t microkit_cothread_destroy_specific(microkit_cothread_t cothread) {
    #if !defined(LIBMICROKITCO_UNSAFE)
        if (!co_controller.init_success) {
            return MICROKITCO_ERR_NOT_INITIALISED;
        }
        if (cothread >= co_controller.max_cothreads || cothread < 0) {
            return MICROKITCO_ERR_INVALID_HANDLE;
        }
    #endif

    if (co_controller.running == 0) {
        panic();
    }
    if (hostedqueue_push(&co_controller.free_handle_queue, &cothread) != LIBHOSTEDQUEUE_NOERR) {
        return MICROKITCO_ERR_OP_FAIL;
    }
    co_controller.tcbs[cothread].state = cothread_not_active;

    return MICROKITCO_NOERR;
}
