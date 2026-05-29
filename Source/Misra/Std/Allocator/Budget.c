/// file      : std/allocator/budget.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Caller-buffer fixed-budget bitmap allocator.
///
/// The caller-provided buffer is partitioned into two disjoint regions:
///
///   [ bitmap ][ pad ][ slot 0 ][ slot 1 ] ... [ slot N-1 ]
///   ^                ^
///   buf              slots
///
/// The bitmap is allocator-owned metadata. The slots are user data.
/// User pointers returned by Alloc are always in the slot region; a
/// foreign or misaligned pointer fails the range / alignment check on
/// Free without the allocator ever dereferencing it. No metadata
/// (no `next` pointer, no magic, no header) is written through the
/// user pointer for the rest of its life.
///
/// Slot state machine:
///   FREE  -- Alloc -->  IN_USE -- Free -->  FREE
///   pre: bit==0          pre: bit==1
///   post: bit:=1         post: bit:=0
///   Bad Free (foreign / misaligned / double-free) -> rejected,
///   no state change.

#include <Misra/Std/Allocator/Budget.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Memory.h>

// Relational invariants for BudgetAllocator. The caller-provided
// buffer is partitioned at init into a bitmap region followed by a
// slots region; the linked fields stay synchronized for the lifetime
// of the allocator. The magic check is the first guard below; the
// remaining checks assume the struct is otherwise well-formed.
static void budget_validate_self(const BudgetAllocator *self) {
    if (!self) {
        LOG_FATAL("BudgetAllocator: NULL self");
    }
    if (self->base.__magic != BUDGET_ALLOCATOR_MAGIC) {
        LOG_FATAL("type-confusion: allocator passed to budget_allocator_* is not a BudgetAllocator");
    }
    if (!self->base.allocate || !self->base.resize || !self->base.remap || !self->base.deallocate) {
        LOG_FATAL("BudgetAllocator: vtable function pointer is NULL");
    }
    if (self->base.alignment == 0 || (self->base.alignment & (self->base.alignment - 1)) != 0) {
        LOG_FATAL("BudgetAllocator: alignment {} is not a positive power of two", (u64)self->base.alignment);
    }
    if (!self->buf || self->buf_bytes == 0) {
        LOG_FATAL("BudgetAllocator: NULL or zero-byte backing buffer");
    }
    if (!self->bitmap || self->bitmap_words == 0) {
        LOG_FATAL("BudgetAllocator: NULL or zero-word bitmap");
    }
    if (!self->slots || self->slot_count == 0) {
        LOG_FATAL("BudgetAllocator: NULL or zero-count slot region");
    }
    if (self->slot_size == 0) {
        LOG_FATAL("BudgetAllocator: slot_size is 0");
    }
    // Bitmap covers at least slot_count bits.
    if ((u64)self->bitmap_words * 64u < (u64)self->slot_count) {
        LOG_FATAL(
            "BudgetAllocator: bitmap_words {} too small for slot_count {} (need {})",
            (u64)self->bitmap_words,
            (u64)self->slot_count,
            (u64)((self->slot_count + 63u) / 64u)
        );
    }
    // Slots and bitmap both lie inside [buf, buf + buf_bytes).
    const u8 *buf_end = self->buf + self->buf_bytes;
    if ((const u8 *)self->bitmap < self->buf || (const u8 *)self->bitmap >= buf_end) {
        LOG_FATAL("BudgetAllocator: bitmap pointer outside buf region");
    }
    if (self->slots < self->buf || self->slots > buf_end) {
        LOG_FATAL("BudgetAllocator: slots pointer outside buf region");
    }
    if ((u64)self->slot_count * (u64)self->slot_size > (u64)(buf_end - self->slots)) {
        LOG_FATAL(
            "BudgetAllocator: slots region overruns buf (need {} bytes, have {})",
            (u64)self->slot_count * (u64)self->slot_size,
            (u64)(buf_end - self->slots)
        );
    }
    // Bitmap region must precede the slot region (init lays them out that way).
    if ((const u8 *)self->bitmap >= self->slots) {
        LOG_FATAL("BudgetAllocator: bitmap region must precede slot region");
    }
}

// ---------------------------------------------------------------------------
// Bitmap helpers. Each u64 word covers 64 slots.

// Returns first 0 bit globally across all words, or -1 if none.
static i64 budget_first_free_bit(const u64 *bitmap, u32 words, size cap) {
    for (u32 w = 0; w < words; w++) {
        u64 inv = ~bitmap[w];
        if (inv == 0)
            continue;
        u32  bit    = CTZ64(inv);
        size global = (size)w * 64u + bit;
        if (global >= cap)
            return -1; // the tail bits past cap are set; this can't happen on a clean bitmap
        return (i64)global;
    }
    return -1;
}

// ---------------------------------------------------------------------------
// Public alloc / resize / remap / free.

void *budget_allocator_allocate(BudgetAllocator *self, size bytes, i8 zeroed) {
    budget_validate_self(self);
    if (bytes == 0 || bytes > self->slot_size) {
#if FEATURE_ALLOC_STATS
        if (bytes != 0) {
            self->base.stats.failed_allocations += 1u;
        }
#endif
        return NULL;
    }

    i64 idx = budget_first_free_bit(self->bitmap, self->bitmap_words, self->slot_count);
    if (idx < 0) {
#if FEATURE_ALLOC_STATS
        self->base.stats.failed_allocations += 1u;
#endif
        return NULL;
    }

    // State assertion: ctz found a 0 bit; if the corresponding bit is
    // somehow set, the bitmap memory has been corrupted.
    u32 w = (u32)((u64)idx >> 6);
    u32 b = (u32)((u64)idx & 63u);
    if (self->bitmap[w] & ((u64)1 << b)) {
        LOG_FATAL("BudgetAllocator bitmap corruption: idx {} bit unexpectedly set", (u64)idx);
    }
    self->bitmap[w] |= ((u64)1 << b);

    void *slot = self->slots + (size)idx * self->slot_size;
    if (zeroed)
        MemSet(slot, 0, self->slot_size);
#if FEATURE_ALLOC_STATS
    // bytes_in_use tracks slot_size (what budget_allocator_deallocate
    // subtracts); bytes_requested keeps tracking the user's `bytes`.
    // Different units, by design -- see AllocatorStats doc in
    // Allocator.h.
    self->base.stats.allocations     += 1u;
    self->base.stats.bytes_requested += (u64)bytes;
    self->base.stats.bytes_in_use    += (u64)self->slot_size;
    if (self->base.stats.bytes_in_use > self->base.stats.peak_bytes_in_use) {
        self->base.stats.peak_bytes_in_use = self->base.stats.bytes_in_use;
    }
#endif
    return slot;
}

i8 budget_allocator_resize(BudgetAllocator *self, void *ptr, size new_size) {
    budget_validate_self(self);
    (void)ptr;
    i8 ok = (new_size <= self->slot_size) ? 1 : 0;
#if FEATURE_ALLOC_STATS
    if (ok) {
        // In-place resize does NOT move bytes_in_use (see AllocatorStats
        // doc in Allocator.h), so no peak refresh is possible here.
        self->base.stats.reallocations   += 1u;
        self->base.stats.bytes_requested += (u64)new_size;
    }
#endif
    return ok;
}

void *budget_allocator_remap(BudgetAllocator *self, void *ptr, size new_size) {
    budget_validate_self(self);
    if (!ptr)
        return budget_allocator_allocate(self, new_size, true);
    if (new_size == 0) {
        budget_allocator_deallocate(self, ptr);
        return NULL;
    }
    void *result = (new_size <= self->slot_size) ? ptr : NULL;
#if FEATURE_ALLOC_STATS
    if (result) {
        // In-place remap does NOT move bytes_in_use, so no peak refresh
        // is possible here.
        self->base.stats.reallocations   += 1u;
        self->base.stats.bytes_requested += (u64)new_size;
    } else {
        self->base.stats.failed_allocations += 1u;
    }
#endif
    return result;
}

size budget_allocator_deallocate(BudgetAllocator *self, void *ptr) {
    budget_validate_self(self);
    if (!ptr)
        return 0;

    u8 *p   = (u8 *)ptr;
    u8 *end = self->slots + self->slot_count * self->slot_size;

    if (p < self->slots || p >= end) {
        LOG_FATAL("budget_free: foreign ptr {x} not in slot region", (u64)p);
        return 0;
    }
    size off = (size)(p - self->slots);
    if (off % self->slot_size != 0) {
        LOG_FATAL("budget_free: misaligned ptr {x} (slot size {})", (u64)p, (u64)self->slot_size);
        return 0;
    }
    size idx = off / self->slot_size;
    u32  w   = (u32)(idx >> 6);
    u32  b   = (u32)(idx & 63u);
    if (!(self->bitmap[w] & ((u64)1 << b))) {
        LOG_FATAL("budget_free: double-free of {x} (idx {})", (u64)p, (u64)idx);
        return 0;
    }
    self->bitmap[w] &= ~((u64)1 << b);
#if FEATURE_ALLOC_STATS
    self->base.stats.deallocations += 1u;
    if ((u64)self->slot_size <= self->base.stats.bytes_in_use) {
        self->base.stats.bytes_in_use -= (u64)self->slot_size;
    } else {
        self->base.stats.bytes_in_use = 0u;
    }
#endif
    return self->slot_size;
}

// ---------------------------------------------------------------------------
// Init lives entirely in the BudgetAllocatorInit / BudgetAllocatorInitAligned
// macros in Budget.h. The macros expand to a designated-initializer literal
// gated by ASSERT_OR_FATAL preconditions, with a MemSet call in the comma
// chain pre-zeroing the bitmap region inside the caller's buffer. Layout is
// [bitmap | pad | slots] -- matching the validator's invariant checks above.

void BudgetAllocatorDeinit(BudgetAllocator *self) {
    if (!self)
        return;
    // The caller still owns `buf`; just wipe our header so any
    // post-deinit dispatch trips ValidateAllocator on zero __magic.
    MemSet(self, 0, sizeof(*self));
}
