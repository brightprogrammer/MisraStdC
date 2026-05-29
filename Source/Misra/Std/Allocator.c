/// file      : std/allocator.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Generic allocator dispatch entry points. The concrete allocator
/// implementations (Page, Heap, Arena, Slab, Budget, Debug) live next
/// to this file under `Allocator/`; this file only routes through the
/// function-pointer table on the `Allocator` base and applies the
/// `effort` / `retry_limit` retry policy on top.
///
/// Stats accounting is NOT done here. Each typed allocator updates
/// its own `base.stats.*` inline at allocate / deallocate / resize /
/// remap success / failure points, so both the typed-direct call
/// (`AllocatorAlloc(&heap, ...)`) and the dyn dispatch below produce
/// identical counter movement on the same workload. Readers consume
/// stats through the `Allocator*` accessor macros declared in
/// `Allocator.h` (e.g. `AllocatorBytesInUse(a)`); there is no
/// dispatched `AllocatorGetStats(...)` function.

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
    if (!self->allocate || !self->resize || !self->remap || !self->deallocate) {
        LOG_FATAL("Allocator missing required function pointers");
    }
    if (!allocator_alignment_is_pow2(self->alignment)) {
        LOG_FATAL("Allocator alignment must be a power of two > 0");
    }
}

void *AllocatorAlloc_dyn(Allocator *self, size bytes, i8 zeroed) {
    ValidateAllocator(self);

    size  attempts = allocator_attempt_limit(self);
    void *ptr      = NULL;
    for (size try_idx = 0; try_idx < attempts; try_idx++) {
        ptr = self->allocate(self, bytes, zeroed);
        if (ptr) {
            break;
        }
    }
    return ptr;
}

i8 AllocatorResize_dyn(Allocator *self, void *ptr, size new_size) {
    ValidateAllocator(self);
    // Resize requires a real allocation and a real new size. Anything
    // degenerate falls outside the in-place contract (caller should
    // route through AllocatorAlloc / AllocatorFree / AllocatorRemap
    // for those cases).
    if (!ptr || new_size == 0) {
        return 0;
    }
    return self->resize(self, ptr, new_size);
}

void *AllocatorRemap_dyn(Allocator *self, void *ptr, size new_size) {
    ValidateAllocator(self);

    size  attempts = allocator_attempt_limit(self);
    void *new_ptr  = NULL;
    for (size try_idx = 0; try_idx < attempts; try_idx++) {
        new_ptr = self->remap(self, ptr, new_size);
        if (new_ptr || new_size == 0) {
            break;
        }
    }
    return new_ptr;
}

void AllocatorFree_dyn(Allocator *self, void *ptr) {
    if (!ptr) {
        return;
    }
    ValidateAllocator(self);
    (void)self->deallocate(self, ptr);
}

#if FEATURE_ALLOC_STATS
void AllocatorResetStats(Allocator *self) {
    ValidateAllocator(self);
    u64 in_use  = self->stats.bytes_in_use;
    self->stats = (AllocatorStats) {0};
    // Preserve outstanding-allocation accounting so subsequent peak
    // tracking starts from current usage, not zero.
    self->stats.bytes_in_use      = in_use;
    self->stats.peak_bytes_in_use = in_use;
}
#endif
