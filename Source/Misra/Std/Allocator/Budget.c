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

struct BudgetFreeSlot {
    int _unused;
};

// Relational invariants for BudgetAllocator. The caller-provided
// buffer is partitioned at init into a bitmap region followed by a
// slots region; the linked fields stay synchronized for the lifetime
// of the allocator. An init failure (buf too small, slot_size 0,
// etc.) yields a zero-initialized struct with __magic == 0 -- the
// magic check above already rejects it.
static void budget_validate_self(const Allocator *self) {
    if (!self) {
        LOG_FATAL("BudgetAllocator: NULL self");
    }
    if (self->__magic != BUDGET_ALLOCATOR_MAGIC) {
        LOG_FATAL("type-confusion: allocator passed to budget_allocator_* is not a BudgetAllocator");
    }
    if (!self->allocate || !self->resize || !self->remap || !self->deallocate) {
        LOG_FATAL("BudgetAllocator: vtable function pointer is NULL");
    }
    if (self->alignment == 0 || (self->alignment & (self->alignment - 1)) != 0) {
        LOG_FATAL("BudgetAllocator: alignment {} is not a positive power of two", (u64)self->alignment);
    }
    const BudgetAllocator *b = (const BudgetAllocator *)self;
    if (!b->buf || b->buf_bytes == 0) {
        LOG_FATAL("BudgetAllocator: NULL or zero-byte backing buffer");
    }
    if (!b->bitmap || b->bitmap_words == 0) {
        LOG_FATAL("BudgetAllocator: NULL or zero-word bitmap");
    }
    if (!b->slots || b->slot_count == 0) {
        LOG_FATAL("BudgetAllocator: NULL or zero-count slot region");
    }
    if (b->slot_size == 0) {
        LOG_FATAL("BudgetAllocator: slot_size is 0");
    }
    // Bitmap covers at least slot_count bits.
    if ((u64)b->bitmap_words * 64u < (u64)b->slot_count) {
        LOG_FATAL(
            "BudgetAllocator: bitmap_words {} too small for slot_count {} (need {})",
            (u64)b->bitmap_words,
            (u64)b->slot_count,
            (u64)((b->slot_count + 63u) / 64u)
        );
    }
    // Slots and bitmap both lie inside [buf, buf + buf_bytes).
    const char *buf_end = b->buf + b->buf_bytes;
    if ((const char *)b->bitmap < b->buf || (const char *)b->bitmap >= buf_end) {
        LOG_FATAL("BudgetAllocator: bitmap pointer outside buf region");
    }
    if (b->slots < b->buf || b->slots > buf_end) {
        LOG_FATAL("BudgetAllocator: slots pointer outside buf region");
    }
    if ((u64)b->slot_count * (u64)b->slot_size > (u64)(buf_end - b->slots)) {
        LOG_FATAL(
            "BudgetAllocator: slots region overruns buf (need {} bytes, have {})",
            (u64)b->slot_count * (u64)b->slot_size,
            (u64)(buf_end - b->slots)
        );
    }
    // Bitmap region must precede the slot region (init lays them out that way).
    if ((const char *)b->bitmap >= b->slots) {
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
// Public alloc / free / resize / remap.

void *budget_allocator_allocate(Allocator *self, size bytes, i8 zeroed) {
    budget_validate_self(self);
    BudgetAllocator *bp = (BudgetAllocator *)self;
    if (bytes == 0 || bytes > bp->slot_size)
        return NULL;

    i64 idx = budget_first_free_bit(bp->bitmap, bp->bitmap_words, bp->slot_count);
    if (idx < 0)
        return NULL;

    // State assertion: ctz found a 0 bit; if the corresponding bit is
    // somehow set, the bitmap memory has been corrupted.
    u32 w = (u32)((u64)idx >> 6);
    u32 b = (u32)((u64)idx & 63u);
    if (bp->bitmap[w] & ((u64)1 << b)) {
        LOG_FATAL("BudgetAllocator bitmap corruption: idx {} bit unexpectedly set", (u64)idx);
    }
    bp->bitmap[w] |= ((u64)1 << b);

    void *slot = bp->slots + (size)idx * bp->slot_size;
    if (zeroed)
        MemSet(slot, 0, bp->slot_size);
    return slot;
}

i8 budget_allocator_resize(Allocator *self, void *ptr, size new_size) {
    budget_validate_self(self);
    BudgetAllocator *bp = (BudgetAllocator *)self;
    (void)ptr;
    return new_size <= bp->slot_size ? 1 : 0;
}

void *budget_allocator_remap(Allocator *self, void *ptr, size new_size) {
    budget_validate_self(self);
    BudgetAllocator *bp = (BudgetAllocator *)self;
    if (!ptr)
        return budget_allocator_allocate(self, new_size, true);
    if (new_size == 0) {
        budget_allocator_deallocate(self, ptr);
        return NULL;
    }
    return new_size <= bp->slot_size ? ptr : NULL;
}

size budget_allocator_deallocate(Allocator *self, void *ptr) {
    budget_validate_self(self);
    BudgetAllocator *bp = (BudgetAllocator *)self;
    if (!ptr)
        return 0;

    char *p   = (char *)ptr;
    char *end = bp->slots + bp->slot_count * bp->slot_size;

    if (p < bp->slots || p >= end) {
        LOG_FATAL("budget_free: foreign ptr {x} not in slot region", (u64)p);
        return 0;
    }
    size off = (size)(p - bp->slots);
    if (off % bp->slot_size != 0) {
        LOG_FATAL("budget_free: misaligned ptr {x} (slot size {})", (u64)p, (u64)bp->slot_size);
        return 0;
    }
    size idx = off / bp->slot_size;
    u32  w   = (u32)(idx >> 6);
    u32  b   = (u32)(idx & 63u);
    if (!(bp->bitmap[w] & ((u64)1 << b))) {
        LOG_FATAL("budget_free: double-free of {x} (idx {})", (u64)p, (u64)idx);
        return 0;
    }
    bp->bitmap[w] &= ~((u64)1 << b);
    return bp->slot_size;
}

// ---------------------------------------------------------------------------
// Init lives entirely in the BudgetAllocatorInit / BudgetAllocatorInitAligned
// macros in Budget.h. The macros expand to a designated-initializer literal
// gated by ASSERT_OR_FATAL preconditions, with a MemSet call in the comma
// chain pre-zeroing the bitmap region inside the caller's buffer. Layout
// follows the same scheme the function form used: [bitmap | pad | slots].

void BudgetAllocatorDeinit(BudgetAllocator *self) {
    if (!self)
        return;
    // The caller still owns `buf`; just wipe our header so any
    // post-deinit dispatch trips ValidateAllocator on zero __magic.
    MemSet(self, 0, sizeof(*self));
}
