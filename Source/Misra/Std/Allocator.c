/// file      : std/allocator.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Generic allocator dispatch entry points. The concrete allocator
/// implementations (Heap, Page, Arena, Pool) live next to this file under
/// `Allocator/`; this file only routes through the function-pointer table
/// on the `Allocator` base and applies the `effort`/`retry_limit` retry
/// policy on top.

#include <Misra/Std/Allocator.h>
#include <Misra/Std/Log.h>

static size allocator_attempt_limit(const Allocator *self) {
    if (!self) {
        return 1;
    }
    switch (self->effort) {
        case ALLOCATOR_EFFORT_RETRY :
        case ALLOCATOR_EFFORT_RETRY_FALLBACK :
            return self->retry_limit ? (size)(self->retry_limit + 1) : 2;
        case ALLOCATOR_EFFORT_ONCE :
        default :
            return 1;
    }
}

static bool allocator_alignment_is_pow2(size alignment) {
    return alignment != 0 && ((alignment & (alignment - 1)) == 0);
}

void ValidateAllocator(const Allocator *self) {
    if (!self) {
        LOG_FATAL("NULL allocator pointer");
    }
    if (self->__magic == 0u) {
        LOG_FATAL("Allocator uninitialized (__magic is zero)");
    }
    if (!self->allocate || !self->reallocate || !self->deallocate) {
        LOG_FATAL("Allocator missing required function pointers");
    }
    if (!allocator_alignment_is_pow2(self->alignment)) {
        LOG_FATAL("Allocator alignment must be a power of two > 0");
    }
}

void *AllocatorAlloc(Allocator *self, size bytes, i8 zeroed) {
    ValidateAllocator(self);

    size  attempts = allocator_attempt_limit(self);
    void *ptr      = NULL;
    for (size try_idx = 0; try_idx < attempts; try_idx++) {
        ptr = self->allocate(self, bytes, zeroed);
        if (ptr) {
            return ptr;
        }
    }
    return NULL;
}

void *AllocatorRealloc(Allocator *self, void *ptr, size old_size, size new_size) {
    ValidateAllocator(self);

    size  attempts = allocator_attempt_limit(self);
    void *new_ptr  = NULL;
    for (size try_idx = 0; try_idx < attempts; try_idx++) {
        new_ptr = self->reallocate(self, ptr, old_size, new_size);
        if (new_ptr || new_size == 0) {
            return new_ptr;
        }
    }
    return NULL;
}

void AllocatorFree(Allocator *self, void *ptr, size bytes) {
    if (!ptr) {
        return;
    }
    ValidateAllocator(self);
    self->deallocate(self, ptr, bytes);
}
