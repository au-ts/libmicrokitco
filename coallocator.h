#pragma once

#include "util.h"

// A very crude memory allocator for the coroutine library.

typedef struct allocator {
    // memory is allocated bottom UP
    void *backing_memory;
    word_t size;

    // next available address
    word_t brk;

    void *(*alloc)(struct allocator *, word_t);
} allocator_t;

void *co_alloc(allocator_t *allocator, word_t size) {
    if (size < 1 || allocator->brk + size >= allocator->size + (word_t) allocator->backing_memory) {
        return 0;
    } else {
        word_t ret = allocator->brk;
        allocator->brk += size;
        return (void *) ret;
    }
}

int co_allocator_init(void *backing_memory, int size, allocator_t *allocator) {
    if (!backing_memory || size < 1) {
        return 1;
    }

    // memzero(backing_memory, size);

    allocator->backing_memory = backing_memory;
    allocator->size = size;
    allocator->brk = (word_t) backing_memory;
    allocator->alloc = co_alloc;
    return 0;
}


