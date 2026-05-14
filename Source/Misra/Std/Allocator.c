/// file      : std/allocator.c
/// author    : Generated during allocator refactor
/// This is free and unencumbered software released into the public domain.
///
/// Allocator configuration and generic helper functions.

#include <Misra/Std/Allocator.h>

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#if defined(_MSC_VER) || defined(__MSC_VER)
#    include <malloc.h>
#endif

static bool allocator_alignment_is_pow2(size alignment) {
    return alignment != 0 && ((alignment & (alignment - 1)) == 0);
}

static size heap_allocator_alignment(const Allocator *alloc) {
    if (alloc && alloc->alignment) {
        return alloc->alignment;
    }
    return (size) _Alignof(max_align_t);
}

static void *heap_allocator_raw_allocate(size bytes, size alignment) {
    if (bytes == 0) {
        return NULL;
    }

    if (alignment <= sizeof(void *)) {
        return malloc(bytes);
    }

    if (!allocator_alignment_is_pow2(alignment)) {
        return NULL;
    }

#if defined(_MSC_VER) || defined(__MSC_VER)
    return _aligned_malloc(bytes, alignment);
#else
    return aligned_alloc(alignment, ALIGN_UP_POW2(bytes, alignment));
#endif
}

static void heap_allocator_raw_deallocate(void *ptr, size alignment) {
    if (!ptr) {
        return;
    }

    if (alignment <= sizeof(void *)) {
        free(ptr);
        return;
    }

#if defined(_MSC_VER) || defined(__MSC_VER)
    _aligned_free(ptr);
#else
    free(ptr);
#endif
}

static void *heap_allocator_allocate(Allocator *alloc, size bytes, bool zeroed) {
    size  alignment = heap_allocator_alignment(alloc);
    void *ptr       = heap_allocator_raw_allocate(bytes, alignment);
    if (ptr && zeroed) {
        memset(ptr, 0, bytes);
    }
    return ptr;
}

static void *heap_allocator_reallocate(Allocator *alloc, void *ptr, size old_size, size new_size) {
    size  alignment = heap_allocator_alignment(alloc);
    void *new_ptr   = NULL;

    if (new_size == 0) {
        heap_allocator_raw_deallocate(ptr, alignment);
        return NULL;
    }

    if (alignment <= sizeof(void *)) {
        return realloc(ptr, new_size);
    }

    new_ptr = heap_allocator_raw_allocate(new_size, alignment);
    if (!new_ptr) {
        return NULL;
    }

    if (ptr) {
        memcpy(new_ptr, ptr, MIN2(old_size, new_size));
        heap_allocator_raw_deallocate(ptr, alignment);
    }

    return new_ptr;
}

static void heap_allocator_deallocate(Allocator *alloc, void *ptr, size bytes) {
    (void)bytes;
    heap_allocator_raw_deallocate(ptr, heap_allocator_alignment(alloc));
}

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

Allocator HeapAllocator(void) {
    return (Allocator) {
        .state        = NULL,
        .state_init   = NULL,
        .state_deinit = NULL,
        .allocate     = heap_allocator_allocate,
        .reallocate   = heap_allocator_reallocate,
        .deallocate   = heap_allocator_deallocate,
        .effort       = ALLOCATOR_EFFORT_ONCE,
        .retry_limit  = 0,
        .flags        = 0,
        .alignment    = (size) _Alignof(max_align_t),
    };
}

Allocator HeapAllocatorAligned(size alignment) {
    Allocator alloc = HeapAllocator();
    if (alignment) {
        alloc.alignment = alignment;
    }
    return alloc;
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

bool AllocatorEnsureState(Allocator *alloc) {
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

    if (!AllocatorEnsureState(alloc)) {
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

    if (!AllocatorEnsureState(alloc)) {
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
