// @billn: needs some cleaning up

#include <microkit.h>
#include <stdint.h>
#include <stdarg.h>
#include <sel4/sel4.h>

#include "libmicrokitco.h"
#include "coallocator.h"
#include "libhostedqueue/libhostedqueue.h"
#include "libco/libco.h"

#define SCHEDULER_NULL_CHOICE -1

// Error handling
const char *err_strs[] = {
    "libmicrokitco: no error.\n",
    "libmicrokitco: not initialised.\n",
    "libmicrokitco: invalid cothread handle.\n",

    "libmicrokitco: init(): already initialised.\n",
    "libmicrokitco: init(): co-stack size too small.\n",
    "libmicrokitco: init(): max_cothreads too small.\n",
    "libmicrokitco: init(): cannot initialise private memory allocator.\n",
    "libmicrokitco: init(): failed to allocate memory for TCB array from private mem allocator.\n",
    "libmicrokitco: init(): failed to allocate memory for join map from private mem allocator.\n",
    "libmicrokitco: init(): failed to allocate memory for free handles queue from private mem allocator.\n",
    "libmicrokitco: init(): failed to allocate memory for scheduling queue from private mem allocator.\n",
    "libmicrokitco: init(): got a NULL co-stack pointer.\n",
    "libmicrokitco: init(): failed to initialise free handles queue.\n",
    "libmicrokitco: init(): failed to initialise scheduling queue.\n",
    "libmicrokitco: init(): failed to fill free handles queue.\n",

    "libmicrokitco: recv_ntfn(): no cothreads are blocked on this channel.\n",

    "libmicrokitco: spawn(): client entrypoint is NULL.\n",
    "libmicrokitco: spawn(): number of arguments is negative.\n",
    "libmicrokitco: spawn(): number of arguments is greater than maximum.\n",
    "libmicrokitco: spawn(): maximum amount of cothreads reached.\n",
    "libmicrokitco: spawn(): cannot schedule the new cothread.\n",

    "libmicrokitco: get_arg(): called from root thread.\n",
    "libmicrokitco: get_arg(): argument index is negative.\n",
    "libmicrokitco: get_arg(): argument index is out of bound.\n",

    "libmicrokitco: mark_ready(): subject cothread is already ready.\n",
    "libmicrokitco: mark_ready(): cannot mark self as ready.\n",
    "libmicrokitco: mark_ready(): cannot schedule subject cothread.\n",

    "libmicrokitco: switch(): cannot switch to self.\n",

    "libmicrokitco: wait(): invalid channel.\n",

    "libmicrokitco: yield(): cannot schedule caller.\n",

    "libmicrokitco: destroy_specific(): cannot release free handle back into queue.\n",
    "libmicrokitco: destroy_specific(): caller cannot destroy self.\n",

    "libmicrokitco: join(): cannot join to root PD thread as it does not return!.\n",
    "libmicrokitco: join(): detected deadlock.\n",
};

// Return a string of human friendly error message.
const char *microkit_cothread_pretty_error(co_err_t err_num) {
    int abs_err = err_num * -1;
    if (err_num < 0 || abs_err >= co_num_errors) {
        return "libmicrokitco: unknown error!\n";
    } else {
        return err_strs[abs_err];
    }
}


// Business logic
typedef enum cothread_state {
    // this id is not being used
    cothread_not_active = 0,

    cothread_initialised = 1, // but not ready
    cothread_blocked_on_channel = 2,
    cothread_blocked_on_join = 3,
    cothread_ready = 4,
    cothread_running = 5,
} co_state_t;

typedef struct {
    cothread_t cothread;
    client_entry_t client_entry;

    co_state_t state;
    
    // the next tcb that is blocked on the same channel or joined on the same cothread as this tcb.
    // -1 means this tcb is the tail.

    // only checked when state == cothread_blocked_on_channel.
    microkit_cothread_t next_blocked_on_same_channel;

    // only checked when state == cothread_blocked_on_join.
    microkit_cothread_t next_joined_on_same_cothread;


    // the cothread that this cothread is waiting on to return
    microkit_cothread_t joined_on;
    // retval of cothread that this TCB joined on
    size_t joined_retval;

    size_t this_retval;
    // zero if this TCB never returned before
    int retval_valid;


    // unused for root thread at the first index of co_tcb_t* tcbs;
    uintptr_t stack_left;

    int num_priv_args;
    size_t priv_args[MAXIMUM_CO_ARGS];
} co_tcb_t;

typedef struct {
    microkit_cothread_t head;
    microkit_cothread_t tail;
} blocked_list_t;

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
    hosted_queue_t free_handle_queue;
    hosted_queue_t scheduling_queue;

    // a map of linked list on what cothreads are blocked on which channel.
    blocked_list_t blocked_channel_map[MICROKIT_MAX_CHANNELS];

    // a map of linked list on what cothreads are joined to which cothread.
    blocked_list_t *joined_map;

    int init_success;
};

// each PD can only have one "instance" of this library running.
static co_control_t co_controller = {
    .init_success = 0
};

size_t microkit_cothread_derive_memsize(int max_cothreads) {
    // Includes root PD thread.
    int real_max_cothreads = max_cothreads + 1;

    size_t tcb_table_size = sizeof(co_tcb_t) * real_max_cothreads;
    size_t joined_table_size = sizeof(blocked_list_t) * real_max_cothreads;

    // Scheduling and free handle queue 
    size_t queues_size = (sizeof(microkit_cothread_t) * 2) * real_max_cothreads;

    return tcb_table_size + joined_table_size + queues_size;
}

co_err_t microkit_cothread_init(uintptr_t controller_memory, int co_stack_size, int max_cothreads, ...) {
    // We don't allow skipping error checking here for safety because this can only be ran once per PD.
    if (co_controller.init_success) {
        return co_err_init_already_initialised;
    }
    if (co_stack_size < MINIMUM_STACK_SIZE) {
        return co_err_init_stack_too_small;
    }
    if (max_cothreads < 1) {
        return co_err_init_max_cothreads_too_small;
    }

    co_controller.co_stack_size = co_stack_size;

    // Includes root thread.
    co_controller.num_cothreads = 1;
    int real_max_cothreads = max_cothreads + 1;
    co_controller.max_cothreads = real_max_cothreads;

    // This part will VMFault on write if your memory is not large enough.
    size_t derived_mem_size = microkit_cothread_derive_memsize(max_cothreads);
    if (co_allocator_init((void *) controller_memory, derived_mem_size, &co_controller.mem_allocator) != 0) {
        return co_err_init_allocator_init_fail;
    }
    allocator_t *allocator = &co_controller.mem_allocator;

    void *tcbs_array = allocator->alloc(allocator, sizeof(co_tcb_t) * real_max_cothreads);
    void *joined_array = allocator->alloc(allocator, sizeof(blocked_list_t) * real_max_cothreads);
    void *free_handle_queue_mem = allocator->alloc(allocator, sizeof(microkit_cothread_t) * real_max_cothreads);
    void *scheduling_queue_mem = allocator->alloc(allocator, sizeof(microkit_cothread_t) * real_max_cothreads);
    if (!tcbs_array) {
        return co_err_init_tcbs_alloc_fail;
    }
    if (!joined_array) {
        return co_err_init_join_alloc_fail;
    }
    if (!free_handle_queue_mem) {
        return co_err_init_free_handles_alloc_fail;
    }
    if (!scheduling_queue_mem) {
        return co_err_init_sched_alloc_fail;
    }

    co_controller.tcbs = tcbs_array;
    co_controller.joined_map = joined_array;
    for (int i = 0; i < real_max_cothreads; i++) {
        co_controller.tcbs[i].state = cothread_not_active;
    }

    // parses all the valid stack memory regions 
    va_list ap;
    va_start (ap, max_cothreads);
    for (int i = 0; i < max_cothreads; i++) {
        co_controller.tcbs[i + 1].stack_left = va_arg(ap, uintptr_t);

        if (!co_controller.tcbs[i + 1].stack_left) {
            return co_err_init_co_stack_null;
        }

        // sanity check the stacks, crash if stack not as big as we think
        char *stack = (char *) co_controller.tcbs[i + 1].stack_left;
        stack[0] = 0;
        stack[co_stack_size - 1] = 0;
    }
    va_end(ap);

    // initialise the root thread's handle;
    co_controller.tcbs[0].cothread = co_active();
    co_controller.tcbs[0].state = cothread_running;
    co_controller.tcbs[0].stack_left = 0;
    co_controller.tcbs[0].next_blocked_on_same_channel = -1;
    memzero(co_controller.tcbs[0].priv_args, sizeof(size_t) * MAXIMUM_CO_ARGS);
    co_controller.running = 0;

    // initialise the queues
    int err_hq = hostedqueue_init(&co_controller.free_handle_queue, free_handle_queue_mem, sizeof(microkit_cothread_t), real_max_cothreads);
    int err_sq = hostedqueue_init(&co_controller.scheduling_queue, scheduling_queue_mem, sizeof(microkit_cothread_t), real_max_cothreads);
    if (err_hq != LIBHOSTEDQUEUE_NOERR) {
        return co_err_init_free_handles_init_fail;
    }
    if (err_sq != LIBHOSTEDQUEUE_NOERR) {
        return co_err_init_sched_init_fail;
    }

    // enqueue all the free cothread handle IDs but exclude the root thread.
    for (microkit_cothread_t i = 1; i < real_max_cothreads; i++) {
        if (hostedqueue_push(&co_controller.free_handle_queue, &i) != LIBHOSTEDQUEUE_NOERR) {
            return co_err_init_free_handles_populate_fail;
        }
    }

    // initialise the blocked table
    for (int i = 0; i < MICROKIT_MAX_CHANNELS; i++) {
        co_controller.blocked_channel_map[i].head = -1;
        co_controller.blocked_channel_map[i].tail = -1;
    }

    // initialised the joined table
    for (int j = 0; j < real_max_cothreads; j++) {
        co_controller.joined_map[j].head = -1;
        co_controller.joined_map[j].tail = -1;
    }

    co_controller.init_success = 1;
    return co_no_err;
}

co_err_t microkit_cothread_recv_ntfn(microkit_channel ch) {
    if (co_controller.running) {
        // called from a cothread context
        panic();
    }

    #if !defined(LIBMICROKITCO_UNSAFE)
        if (!co_controller.init_success) {
            return co_err_generic_not_initialised;
        }
    #endif

    if (co_controller.blocked_channel_map[ch].head == -1) {
        return co_err_recv_ntfn_no_blocked;
    } else {
        microkit_cothread_t cur = co_controller.blocked_channel_map[ch].head;

        // iterate over the blocked linked list, unblocks all the cothreads and schedule them
        while (cur != -1) {
            co_err_t err = microkit_cothread_mark_ready(cur);
            if (err != co_no_err) {
                return err;
            }

            microkit_cothread_t next = co_controller.tcbs[cur].next_blocked_on_same_channel;
            co_controller.tcbs[cur].next_blocked_on_same_channel = -1;
            cur = next;
        }
 
        co_controller.blocked_channel_map[ch].head = -1;
        co_controller.blocked_channel_map[ch].tail = -1;

        // run the cothreads
        microkit_cothread_yield();

        return co_no_err;
    }
}

void internal_destroy_me();
void internal_entry_return(microkit_cothread_t cothread, size_t retval) {
    // Save retval in TCB to handle case where thread is joined after it returns
    co_controller.tcbs[cothread].retval_valid = 1;
    co_controller.tcbs[cothread].this_retval = retval;

    // Wake all the joined cothreads on this cothread (if any)
    microkit_cothread_t cur = co_controller.joined_map[cothread].head;
    while (cur != -1) {
        co_err_t err = microkit_cothread_mark_ready(cur);
        if (err != co_no_err) {
            microkit_internal_crash(err);
        }
        co_controller.tcbs[cur].joined_retval = retval;

        microkit_cothread_t next = co_controller.tcbs[cur].next_joined_on_same_cothread;
        co_controller.tcbs[cur].next_joined_on_same_cothread = -1;
        cur = next;
    }

    co_controller.joined_map[cothread].head = -1;
    co_controller.joined_map[cothread].tail = -1;

    internal_destroy_me();
}
void cothread_entry_wrapper() {
    // Execute the client entry point
    size_t retval = co_controller.tcbs[co_controller.running].client_entry();

    // After the client entry point return, wake any joined cothreads and clean up after ourselves.
    internal_entry_return(co_controller.running, retval);

    // should not get here.
    // UB in libco if cothread returns normally!
    panic();
}

co_err_t microkit_cothread_spawn(client_entry_t client_entry, ready_status_t ready, microkit_cothread_t *ret, int num_args, ...) {
    #if !defined LIBMICROKITCO_UNSAFE
        if (!co_controller.init_success) {
            return co_err_generic_not_initialised;
        }
        if (!client_entry) {
            return co_err_spawn_client_entry_null;
        }
        if (num_args < 0) {
            return co_err_spawn_num_args_negative;
        }
        if (num_args > MAXIMUM_CO_ARGS) {
            return co_err_spawn_num_args_too_much;
        }
    #endif

    hosted_queue_t *free_handle_queue = &co_controller.free_handle_queue;
    microkit_cothread_t new;
    int pop_err = hostedqueue_pop(free_handle_queue, &new);

    if (pop_err != LIBHOSTEDQUEUE_NOERR) {
        return co_err_spawn_max_cothreads_reached;
    }

    unsigned char *costack = (unsigned char *) co_controller.tcbs[new].stack_left;
    memzero(costack, co_controller.co_stack_size);
    co_controller.tcbs[new].client_entry = client_entry;
    co_controller.tcbs[new].cothread = co_derive(costack, co_controller.co_stack_size, cothread_entry_wrapper);
    co_controller.tcbs[new].state = cothread_initialised;
    co_controller.tcbs[new].next_blocked_on_same_channel = -1;
    co_controller.tcbs[new].next_joined_on_same_cothread = -1;
    co_controller.tcbs[new].joined_on = -1;
    co_controller.tcbs[new].retval_valid = 0;
    co_controller.tcbs[new].num_priv_args = num_args;
    memzero(co_controller.tcbs[new].priv_args, sizeof(size_t) * MAXIMUM_CO_ARGS);

    va_list ap;
    va_start (ap, num_args);
    for (int i = 0; i < num_args; i++) {
        co_controller.tcbs[new].priv_args[i] = va_arg(ap, size_t);
    }
    va_end(ap);

    if (ready) {
        int err = microkit_cothread_mark_ready(new);
        if (err != co_no_err) {
            co_controller.tcbs[new].state = cothread_not_active;
            hostedqueue_push(free_handle_queue, &new);
            return co_err_spawn_cannot_schedule;
        }
    }

    *ret = new;
    return co_no_err;
}

co_err_t microkit_cothread_get_arg(int nth, size_t *ret) {
    #if !defined(LIBMICROKITCO_UNSAFE)
        if (!co_controller.init_success) {
            return co_err_generic_not_initialised;
        }
        if (!co_controller.running) {
            return co_err_get_arg_called_from_root;
        }
        if (nth >= MAXIMUM_CO_ARGS) {
            return co_err_get_arg_nth_is_greater_than_max;
        }
        if (nth < 0) {
            return co_err_get_arg_nth_is_negative;
        }
    #endif

    *ret = co_controller.tcbs[co_controller.running].priv_args[nth];
    return co_no_err;
}

co_err_t microkit_cothread_mark_ready(microkit_cothread_t cothread) {
    #if !defined(LIBMICROKITCO_UNSAFE)
        if (!co_controller.init_success) {
            return co_err_generic_not_initialised;
        }
        if (cothread >= co_controller.max_cothreads || cothread < 0 || co_controller.tcbs[cothread].state == cothread_not_active) {
            return co_err_generic_invalid_handle;
        }
        if (co_controller.tcbs[cothread].state == cothread_ready) {
            return co_err_mark_ready_already_ready;
        }
        if (co_controller.running == cothread) {
            return co_err_mark_ready_cannot_mark_self;
        }
    #endif

    hosted_queue_t *sched_queue = &co_controller.scheduling_queue;
    int push_err = hostedqueue_push(sched_queue, &cothread);
    co_state_t old_state = co_controller.tcbs[cothread].state;
    co_controller.tcbs[cothread].state = cothread_ready;

    if (push_err != LIBHOSTEDQUEUE_NOERR) {
        co_controller.tcbs[cothread].state = old_state;
        return co_err_mark_ready_cannot_schedule;
    } else {
        return co_no_err;
    }
}

co_err_t microkit_cothread_switch(microkit_cothread_t cothread) {
    #if !defined(LIBMICROKITCO_UNSAFE)
        if (!co_controller.init_success) {
            return co_err_generic_not_initialised;
        }
        if (cothread >= co_controller.max_cothreads || cothread < 0) {
            return co_err_generic_invalid_handle;
        }
        if (cothread == co_controller.running) {
            return co_err_switch_to_self;
        }
        if (co_controller.tcbs[cothread].state == cothread_not_active) {
            return co_err_generic_invalid_handle;
        }
    #endif

    co_controller.tcbs[co_controller.running].state = cothread_ready;
    hosted_queue_t *sched_queue = &co_controller.scheduling_queue;
    int push_err = hostedqueue_push(sched_queue, &co_controller.running);

    #if !defined(LIBMICROKITCO_UNSAFE)
        if (push_err != LIBHOSTEDQUEUE_NOERR) {
            // cannot schedule caller, something horrifically wrong with implementation.
            panic();
        }
    #endif

    co_controller.tcbs[cothread].state = cothread_running;
    co_controller.running = cothread;
    co_switch(co_controller.tcbs[cothread].cothread);
    return co_no_err;
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
    co_tcb_t* tcbs = co_controller.tcbs;
    return internal_pop_from_queue(&co_controller.scheduling_queue, tcbs);
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

co_err_t microkit_cothread_wait(microkit_channel wake_on) {
    #if !defined(LIBMICROKITCO_UNSAFE)
        if (!co_controller.init_success) {
            return co_err_generic_not_initialised;
        }
        if (wake_on >= MICROKIT_MAX_CHANNELS) {
            return co_err_wait_invalid_channel;
        }
    #endif

    co_controller.tcbs[co_controller.running].state = cothread_blocked_on_channel;
    
    // push into the waiting linked list (if any)
    if (co_controller.blocked_channel_map[wake_on].head == -1) {
        co_controller.blocked_channel_map[wake_on].head = co_controller.running;
    } else {
        co_controller.tcbs[co_controller.blocked_channel_map[wake_on].tail].next_blocked_on_same_channel = co_controller.running;
    }
    co_controller.blocked_channel_map[wake_on].tail = co_controller.running;
    co_controller.tcbs[co_controller.running].next_blocked_on_same_channel = -1;

    internal_go_next();
    return co_no_err;
}

void microkit_cothread_yield() {
    #if !defined(LIBMICROKITCO_UNSAFE)
        if (!co_controller.init_success) {
            return;
        }
    #endif

    // Caller get pushed onto the appropriate scheduling queue.
    hosted_queue_t *sched_queue = &co_controller.scheduling_queue;
    hostedqueue_push(sched_queue, &co_controller.running);
    co_controller.tcbs[co_controller.running].state = cothread_ready;

    // If the scheduling queues are empty beforehand, the caller just get runned again.
    internal_go_next();
}

void internal_destroy_me() {
    #if !defined(LIBMICROKITCO_UNSAFE)
        if (!co_controller.init_success) {
            return;
        }
    #endif

    if (co_controller.running == 0) {
        return;
    }

    int err = microkit_cothread_destroy_specific(co_controller.running);

    #if !defined(LIBMICROKITCO_UNSAFE)
        if (err != co_no_err) {
            panic();
        }
    #endif

    // Should not return
    panic();
}

co_err_t microkit_cothread_destroy_specific(microkit_cothread_t cothread) {
    #if !defined(LIBMICROKITCO_UNSAFE)
        if (!co_controller.init_success) {
            return co_err_generic_not_initialised;
        }
        if (cothread >= co_controller.max_cothreads || cothread < 0) {
            return co_err_generic_invalid_handle;
        }
    #endif

    if (co_controller.running == 0) {
        // cannot destroy root thread
        panic();
    }
    if (hostedqueue_push(&co_controller.free_handle_queue, &cothread) != LIBHOSTEDQUEUE_NOERR) {
        return co_err_destroy_specific_cannot_release_handle;
    }
    co_controller.tcbs[cothread].state = cothread_not_active;

    if (cothread == co_controller.running) {
        internal_go_next();
    }

    return co_no_err;
}

int internal_detect_deadlock(microkit_cothread_t caller, microkit_cothread_t joinee) {
    microkit_cothread_t cur = co_controller.tcbs[joinee].joined_on;
    while (cur != -1) {
        if (cur == caller) {
            return 1;
        } else {
            cur = co_controller.tcbs[cur].joined_on;
        }
    }

    return 0;
}

co_err_t microkit_cothread_join(microkit_cothread_t cothread, size_t *retval) {
    #if !defined(LIBMICROKITCO_UNSAFE)
        if (!co_controller.init_success) {
            return co_err_generic_not_initialised;
        }
        if (cothread >= co_controller.max_cothreads || cothread < 0) {
            return co_err_generic_invalid_handle;
        }
    #endif

    if (co_controller.tcbs[cothread].retval_valid) {
        // Cothread returned before join
        co_controller.tcbs[cothread].retval_valid = 0;
        *retval = co_controller.tcbs[cothread].this_retval;
        return co_no_err;
    }

    if (co_controller.tcbs[cothread].state == cothread_not_active) {
        return co_err_generic_not_initialised;
    }
    if (cothread == 0) {
        return co_err_join_cannot_join_to_root_thread;
    }

    if (cothread == co_controller.running || internal_detect_deadlock(co_controller.running, cothread)) {
        return co_err_join_deadlock_detected;
    }

    co_controller.tcbs[co_controller.running].state = cothread_blocked_on_join;
    
    // push into the waiting linked list (if any)
    if (co_controller.joined_map[cothread].head == -1) {
        co_controller.joined_map[cothread].head = co_controller.running;
    } else {
        co_controller.tcbs[co_controller.joined_map[cothread].tail].next_joined_on_same_cothread = co_controller.running;
    }
    co_controller.joined_map[cothread].tail = co_controller.running;
    co_controller.tcbs[co_controller.running].next_joined_on_same_cothread = -1;
    co_controller.tcbs[co_controller.running].joined_on = cothread;

    internal_go_next();
    
    *retval = co_controller.tcbs[co_controller.running].joined_retval;

    return co_no_err;
}
