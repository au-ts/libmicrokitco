#pragma once

#include "../util.h"

// A simple fixed capacity stack that grows upward.
// "Hosted" meaning the user of the library provides the memory.

#define LIBHOSTEDSTACK_NOERR 0
#define LIBHOSTEDSTACK_ERR_INVALID_ARGS 1
#define LIBHOSTEDSTACK_ERR_FULL 2
#define LIBHOSTEDSTACK_ERR_EMPTY 3

typedef struct {
    void *memory;
    int item_size;
    int capacity;
    int top;
} hosted_stack_t;

int hostedstack_init(hosted_stack_t *stack_controller, void *memory, int item_size, int capacity) {
    if (!memory || item_size < 1 || capacity < 1) {
        return LIBHOSTEDSTACK_ERR_INVALID_ARGS;
    }

    stack_controller->memory = memory;
    stack_controller->item_size = item_size;
    stack_controller->capacity = capacity;
    stack_controller->top = 0;
    return LIBHOSTEDSTACK_NOERR;
}

int hostedstack_peek(hosted_stack_t *stack_controller, void *ret) {
    if (stack_controller->top == 0) {
        return LIBHOSTEDSTACK_ERR_EMPTY;
    }

    int item_size = stack_controller->item_size;
    void *base = stack_controller->memory + (item_size * (stack_controller->top - 1));

    memcpy(ret, base, item_size);
    return LIBHOSTEDSTACK_NOERR;
}

int hostedstack_pop(hosted_stack_t *stack_controller, void *ret) {
    int err = hostedstack_peek(stack_controller, ret);
    if (err == LIBHOSTEDSTACK_NOERR) {
        stack_controller->top -= 1;
    }
    return err;
}

int hostedstack_push(hosted_stack_t *stack_controller, void *item) {
    int capacity = stack_controller->capacity;
    int top = stack_controller->capacity;

    if (capacity == top) {
        return LIBHOSTEDSTACK_ERR_FULL;
    }

    int item_size = stack_controller->item_size;
    void* base = stack_controller->memory + (item_size * top);
    memcpy(base, item, item_size);
    return LIBHOSTEDSTACK_NOERR;
}
