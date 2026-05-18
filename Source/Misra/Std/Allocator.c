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
    if (!self->allocate || !self->resize || !self->remap || !self->deallocate) {
        LOG_FATAL("Allocator missing required function pointers");
    }
    if (!allocator_alignment_is_pow2(self->alignment)) {
        LOG_FATAL("Allocator alignment must be a power of two > 0");
    }
}

#if FEATURE_ALLOC_STATS
static void allocator_stats_on_alloc(Allocator *self, size bytes) {
    self->stats.allocations     += 1;
    self->stats.bytes_requested += (u64)bytes;
    self->stats.bytes_in_use    += (u64)bytes;
    if (self->stats.bytes_in_use > self->stats.peak_bytes_in_use) {
        self->stats.peak_bytes_in_use = self->stats.bytes_in_use;
    }
}

static void allocator_stats_on_free(Allocator *self, size bytes) {
    self->stats.deallocations += 1;
    if ((u64)bytes <= self->stats.bytes_in_use) {
        self->stats.bytes_in_use -= (u64)bytes;
    } else {
        self->stats.bytes_in_use = 0;
    }
}
#endif

void *AllocatorAlloc(Allocator *self, size bytes, i8 zeroed) {
    ValidateAllocator(self);

    size  attempts = allocator_attempt_limit(self);
    void *ptr      = NULL;
    for (size try_idx = 0; try_idx < attempts; try_idx++) {
        ptr = self->allocate(self, bytes, zeroed);
        if (ptr) {
            break;
        }
    }
#if FEATURE_ALLOC_STATS
    if (ptr) {
        allocator_stats_on_alloc(self, bytes);
    } else {
        self->stats.failed_allocations += 1;
    }
#endif
    return ptr;
}

#if FEATURE_ALLOC_STATS
// Bookkeeping for a successful resize/remap. bytes_in_use deltas are
// tracked inside the underlying allocator (via internal alloc/free
// for the move case, and as a no-op for true in-place). Dispatch
// just counts the operation and accumulates requested bytes.
static void allocator_stats_on_realloc(Allocator *self, size new_size) {
    self->stats.reallocations   += 1;
    self->stats.bytes_requested += (u64)new_size;
    if (self->stats.bytes_in_use > self->stats.peak_bytes_in_use) {
        self->stats.peak_bytes_in_use = self->stats.bytes_in_use;
    }
}
#endif

i8 AllocatorResize(Allocator *self, void *ptr, size new_size) {
    ValidateAllocator(self);
    // Resize requires a real allocation and a real new size. Anything
    // degenerate falls outside the in-place contract (caller should
    // route through AllocatorAlloc / AllocatorFree / AllocatorRemap
    // for those cases).
    if (!ptr || new_size == 0) {
        return 0;
    }
    i8 ok = self->resize(self, ptr, new_size);
#if FEATURE_ALLOC_STATS
    if (ok) {
        allocator_stats_on_realloc(self, new_size);
    }
#endif
    return ok;
}

void *AllocatorRemap(Allocator *self, void *ptr, size new_size) {
    ValidateAllocator(self);

    size  attempts = allocator_attempt_limit(self);
    void *new_ptr  = NULL;
    for (size try_idx = 0; try_idx < attempts; try_idx++) {
        new_ptr = self->remap(self, ptr, new_size);
        if (new_ptr || new_size == 0) {
            break;
        }
    }
#if FEATURE_ALLOC_STATS
    if (new_size == 0) {
        if (ptr) {
            // remap(ptr, 0) is a free of ptr.
            self->stats.deallocations += 1;
        }
        // remap(NULL, 0) is the trivial no-op: nothing freed,
        // nothing allocated, nothing failed -- no counter moves.
        // Made explicit so future readers don't read "no else
        // clause" as a forgotten case.
    } else if (new_ptr) {
        allocator_stats_on_realloc(self, new_size);
    } else {
        // new_size > 0 and impl returned NULL: alloc-via-remap
        // failed, or remap of an existing ptr failed.
        self->stats.failed_allocations += 1;
    }
#endif
    return new_ptr;
}

void *AllocatorRealloc(Allocator *self, void *ptr, size new_size) {
    // Convenience cascade: try in-place first (cheap if the allocator
    // can do it -- no copy, no free, pointer stays valid), fall back
    // to remap on failure. Callers that need to know whether the
    // pointer moved should use AllocatorResize / AllocatorRemap
    // directly. ValidateAllocator runs inside each sub-call.
    if (ptr && new_size > 0 && AllocatorResize(self, ptr, new_size)) {
        return ptr;
    }
    return AllocatorRemap(self, ptr, new_size);
}

void AllocatorFree(Allocator *self, void *ptr) {
    if (!ptr) {
        return;
    }
    ValidateAllocator(self);
    size freed = self->deallocate(self, ptr);
#if FEATURE_ALLOC_STATS
    allocator_stats_on_free(self, freed);
#else
    (void)freed;
#endif
}

#if FEATURE_ALLOC_STATS
AllocatorStats AllocatorGetStats(const Allocator *self) {
    ValidateAllocator(self);
    return self->stats;
}

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
