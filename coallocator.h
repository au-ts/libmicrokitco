#pragma once

#include <stdint.h>

// A very crude memory allocator for the coroutine library.
typedef struct allocator {
    void *backing_memory;
    uint64_t size;

    // next available address
    uint64_t brk;

    void *(*alloc)(allocator_t *, uint64_t);
    void *(*free)(allocator_t *, uint64_t);
} allocator_t;

void *alloc(allocator_t *allocator, uint64_t size) {
    if (allocator->brk + size > allocator->size) {
        return 0;
    } else {
        uint64_t ret = allocator->brk;
        allocator->brk += size;
        return (void *) ret;
    }
}

int allocator_init(void *backing_memory, uint64_t size, allocator_t *allocator) {
    if (!backing_memory || !size) {
        return 1;
    }

    for (uint64_t i = 0; i < size; i++) {
        ((uint8_t *) backing_memory)[i] = 0;
    }

    allocator->backing_memory = backing_memory;
    allocator->size = size;
    allocator->brk = 0;
    allocator->alloc = alloc;
    allocator->free = 0;
    return 0;
}


