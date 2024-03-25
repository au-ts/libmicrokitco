#pragma once

#include <stddef.h>
#include "util.h"

// A very crude memory allocator for the coroutine library.

typedef struct allocator {
    // memory is allocated bottom UP
    void *backing_memory;
    size_t size;

    // next available address
    size_t brk;

    void *(*alloc)(struct allocator *, size_t);
} allocator_t;

void *co_alloc(allocator_t *allocator, size_t size) {
    if (size < 1 || allocator->brk + size > allocator->size + (size_t) allocator->backing_memory) {
        return 0;
    } else {
        size_t ret = allocator->brk;
        allocator->brk += size;
        return (void *) ret;
    }
}

int co_allocator_init(void *backing_memory, int size, allocator_t *allocator) {
    if (!backing_memory || size < 1) {
        return 1;
    }
    
    // quick sanity check
    unsigned char *mem = (unsigned char *) backing_memory;
    mem[0] = 0;
    mem[size - 1] = 0;

    allocator->backing_memory = backing_memory;
    allocator->size = size;
    allocator->brk = (size_t) backing_memory;
    allocator->alloc = co_alloc;
    return 0;
}


