/*
 * Copyright 2024, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <microkit.h>

// A simple fixed capacity circular queue of `microkit_cothread_ref_t`s.

// "Hosted" meaning the user of the library provides the memory.

#define ITEM_TYPE microkit_cothread_ref_t

// return codes:
#define LIBHOSTEDQUEUE_NOERR 0
#define LIBHOSTEDQUEUE_ERR_INVALID_ARGS 1
#define LIBHOSTEDQUEUE_ERR_FULL 2
#define LIBHOSTEDQUEUE_ERR_EMPTY 3

typedef struct {
    int capacity;
    int items;

    // points to item at front
    int head;
    // next available index for insert, i.e. exclusive of last item
    int tail;
} hosted_queue_t;

static inline int hostedqueue_init(hosted_queue_t *queue_controller, const int capacity) {
    if (capacity < 1) {
        return LIBHOSTEDQUEUE_ERR_INVALID_ARGS;
    }

    queue_controller->capacity = capacity;
    queue_controller->items = 0;
    queue_controller->head = 0;
    queue_controller->tail = 0;
    return LIBHOSTEDQUEUE_NOERR;
}

static inline int hostedqueue_peek(const hosted_queue_t *queue_controller, const ITEM_TYPE *queue_memory, const void *ret) {
    if (!queue_controller->items) {
        return LIBHOSTEDQUEUE_ERR_EMPTY;
    }

    *((ITEM_TYPE *) ret) = ((ITEM_TYPE *) queue_memory)[queue_controller->head];

    return LIBHOSTEDQUEUE_NOERR;
}

static inline int hostedqueue_pop(hosted_queue_t *queue_controller, ITEM_TYPE *queue_memory, void *ret) {
    int err = hostedqueue_peek(queue_controller, queue_memory, ret);
    if (err == LIBHOSTEDQUEUE_NOERR) {
        queue_controller->head += 1;
        queue_controller->head %= queue_controller->capacity;
        queue_controller->items -= 1;
    }
    return err;
}

static inline int hostedqueue_push(hosted_queue_t *queue_controller, ITEM_TYPE *queue_memory, const void *item) {
    if (queue_controller->items == queue_controller->capacity) {
        return LIBHOSTEDQUEUE_ERR_FULL;
    }

    ((ITEM_TYPE *) queue_memory)[queue_controller->tail] = *((int *) item);

    queue_controller->items += 1;
    queue_controller->tail += 1;
    queue_controller->tail %= queue_controller->capacity;

    return LIBHOSTEDQUEUE_NOERR;
}
