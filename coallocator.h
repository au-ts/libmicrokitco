#pragma once

#include <stdint.h>

// A very crude memory allocator for the coroutine library.

typedef struct allocator {
    // memory for client is allocated from bottom UP
    // memory for internal data structure is allocated top DOWN
    void *backing_memory;
    uint64_t size;

    // next available address
    uint64_t brk;

    void *(*alloc)(struct allocator *, unsigned int);
} allocator_t;

void *alloc(allocator_t *allocator, unsigned int size) {
    if (allocator->brk + size > allocator->size) {
        return 0;
    } else {
        uint64_t ret = allocator->brk;
        allocator->brk += size;
        return (void *) ret;
    }
}

int allocator_init(void *backing_memory, unsigned int size, allocator_t *allocator) {
    if (!backing_memory || !size) {
        return 1;
    }

    for (int i = 0; i < size; i++) {
        ((char *) backing_memory)[i] = 0;
    }

    allocator->backing_memory = backing_memory;
    allocator->size = size;
    allocator->brk = 0;
    allocator->alloc = alloc;
    return 0;
}


