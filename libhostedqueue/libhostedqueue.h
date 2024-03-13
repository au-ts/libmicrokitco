#pragma once

#include "../util.h"
#include <microkit.h>

// A simple fixed capacity circular queue.
// "Hosted" meaning the user of the library provides the memory.

// return codes:
#define LIBHOSTEDQUEUE_NOERR 0
#define LIBHOSTEDQUEUE_ERR_INVALID_ARGS 1
#define LIBHOSTEDQUEUE_ERR_FULL 2
#define LIBHOSTEDQUEUE_ERR_EMPTY 3

typedef struct {
    void *memory;
    int item_size;

    int capacity;
    int items;

    // points to item at front
    int front;
    // next available index for insert, i.e. exclusive of last item
    int back;
} hosted_queue_t;

// memory should be (item_size * capacity) bytes size large!
int hostedqueue_init(hosted_queue_t *queue_controller, void *memory, int item_size, int capacity) {
    if (!memory || item_size < 1 || capacity < 1) {
        return LIBHOSTEDQUEUE_ERR_INVALID_ARGS;
    }

    // make sure we have enough *valid* memory
    memzero(memory, item_size * capacity);

    queue_controller->memory = memory;
    queue_controller->item_size = item_size;
    queue_controller->capacity = capacity;
    queue_controller->items = 0;
    queue_controller->front = 0;
    queue_controller->back = 0;
    return LIBHOSTEDQUEUE_NOERR;
}

int hostedqueue_peek(hosted_queue_t *queue_controller, void *ret) {
    if (!queue_controller->items) {
        return LIBHOSTEDQUEUE_ERR_EMPTY;
    }

    int item_size = queue_controller->item_size;
    void *base = queue_controller->memory + (item_size * (queue_controller->front));

    memcpy(ret, base, item_size);
    return LIBHOSTEDQUEUE_NOERR;
}

int hostedqueue_pop(hosted_queue_t *queue_controller, void *ret) {
    int err = hostedqueue_peek(queue_controller, ret);
    if (err == LIBHOSTEDQUEUE_NOERR) {
        if (queue_controller->front == queue_controller->capacity - 1) {
            queue_controller->front = 0;
        } else {
            queue_controller->front += 1;
        }
        queue_controller->items -= 1;
    }
    return err;
}

int hostedqueue_push(hosted_queue_t *queue_controller, void *item) {
    if (queue_controller->items == queue_controller->capacity) {
        return LIBHOSTEDQUEUE_ERR_FULL;
    }

    queue_controller->items += 1;
    int item_size = queue_controller->item_size;
    void* base = queue_controller->memory + (item_size * queue_controller->back);
    memcpy(base, item, item_size);

    if (queue_controller->back == queue_controller->capacity - 1) {
        queue_controller->back = 0;
    } else {
        queue_controller->back += 1;
    }

    return LIBHOSTEDQUEUE_NOERR;
}

// TODO, write a short integration test
