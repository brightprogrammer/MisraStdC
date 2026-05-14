/// file      : std/allocator.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Allocator API entry points and default-binding helpers. The actual
/// allocator backends live next to this file under `Allocator/`.

#include <Misra/Std/Allocator.h>
#include <Misra/Std/Allocator/Private.h>

static size allocator_attempt_limit(const Allocator *alloc) {
    if (!alloc) {
        return 1;
    }

    switch (alloc->effort) {
        case ALLOCATOR_EFFORT_RETRY :
        case ALLOCATOR_EFFORT_RETRY_FALLBACK :
            return alloc->retry_limit ? (size)(alloc->retry_limit + 1) : 2;
        case ALLOCATOR_EFFORT_ONCE :
        default :
            return 1;
    }
}

Allocator AllocatorBind(Allocator alloc) {
    Allocator heap = HeapAllocator();

    if (!alloc.allocate) {
        alloc.allocate = heap.allocate;
    }
    if (!alloc.reallocate) {
        alloc.reallocate = heap.reallocate;
    }
    if (!alloc.deallocate) {
        alloc.deallocate = heap.deallocate;
    }

    alloc.state = NULL;
    return alloc;
}

bool allocator_ensure_state(Allocator *alloc) {
    if (!alloc) {
        return false;
    }
    if (!alloc->state_init || alloc->state) {
        return true;
    }
    return alloc->state_init(alloc);
}

void *AllocatorAlloc(Allocator *alloc, size bytes, bool zeroed) {
    size  attempts;
    size  try_idx;
    void *ptr = NULL;

    if (!alloc || !alloc->allocate) {
        return NULL;
    }

    if (!allocator_ensure_state(alloc)) {
        return NULL;
    }

    attempts = allocator_attempt_limit(alloc);
    for (try_idx = 0; try_idx < attempts; try_idx++) {
        ptr = alloc->allocate(alloc, bytes, zeroed);
        if (ptr) {
            return ptr;
        }
    }

    return NULL;
}

void *AllocatorRealloc(Allocator *alloc, void *ptr, size old_size, size new_size) {
    size  attempts;
    size  try_idx;
    void *new_ptr = NULL;

    if (!alloc || !alloc->reallocate) {
        return NULL;
    }

    if (!allocator_ensure_state(alloc)) {
        return NULL;
    }

    attempts = allocator_attempt_limit(alloc);
    for (try_idx = 0; try_idx < attempts; try_idx++) {
        new_ptr = alloc->reallocate(alloc, ptr, old_size, new_size);
        if (new_ptr || new_size == 0) {
            return new_ptr;
        }
    }

    return NULL;
}

void AllocatorFree(Allocator *alloc, void *ptr, size bytes) {
    if (!ptr || !alloc || !alloc->deallocate) {
        return;
    }

    alloc->deallocate(alloc, ptr, bytes);
}

void AllocatorUnbind(Allocator *alloc) {
    if (!alloc) {
        return;
    }

    if (alloc->state && alloc->state_deinit) {
        alloc->state_deinit(alloc);
    }

    alloc->state = NULL;
}
