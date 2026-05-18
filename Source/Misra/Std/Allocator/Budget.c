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

static void budget_validate_self(const Allocator *self) {
    if (!self || self->__magic != BUDGET_ALLOCATOR_MAGIC) {
        LOG_FATAL("type-confusion: allocator passed to budget_allocator_* is not a BudgetAllocator");
    }
}

static size budget_round_up(size value, size alignment) {
    return (value + (alignment - 1)) & ~(alignment - 1);
}

static bool budget_alignment_is_pow2(size alignment) {
    return alignment != 0 && ((alignment & (alignment - 1)) == 0);
}

// ---------------------------------------------------------------------------
// Bitmap helpers. Each u64 word covers 64 slots.

#if defined(_MSC_VER) && !defined(__clang__)
#    include <intrin.h>
static u32 ctz64(u64 x) {
    unsigned long idx;
    _BitScanForward64(&idx, x);
    return (u32)idx;
}
#else
static u32 ctz64(u64 x) {
    return (u32)__builtin_ctzll(x);
}
#endif

// Returns first 0 bit globally across all words, or -1 if none.
static i64 budget_first_free_bit(const u64 *bitmap, u32 words, size cap) {
    for (u32 w = 0; w < words; w++) {
        u64 inv = ~bitmap[w];
        if (inv == 0)
            continue;
        u32  bit    = ctz64(inv);
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

i8 budget_allocator_resize(Allocator *self, void *ptr, size old_size, size new_size) {
    budget_validate_self(self);
    BudgetAllocator *bp = (BudgetAllocator *)self;
    (void)ptr;
    (void)old_size;
    return new_size <= bp->slot_size ? 1 : 0;
}

void *budget_allocator_remap(Allocator *self, void *ptr, size old_size, size new_size) {
    budget_validate_self(self);
    BudgetAllocator *bp = (BudgetAllocator *)self;
    (void)old_size;
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
// Init. Carves the user buffer into [bitmap | pad | slots]. The bitmap
// is sized for the upper bound of how many slots could fit if there
// were no bitmap; in practice we then have fewer slots than the bitmap
// covers (the tail bits stay forever 0, harmlessly).

static BudgetAllocator budget_build(void *buf_in, size buf_bytes, size slot_size, size alignment) {
    BudgetAllocator empty = {0};
    if (!buf_in || !slot_size)
        return empty;

    if (!budget_alignment_is_pow2(alignment))
        alignment = sizeof(void *);
    if (alignment < sizeof(void *))
        alignment = sizeof(void *);

    size padded_slot = budget_round_up(slot_size, alignment);

    // Bitmap lives at the front of the buffer, u64-aligned.
    u64  buf_addr        = (u64)buf_in;
    u64  bitmap_addr     = (buf_addr + 7u) & ~(u64)7u;
    size bitmap_head_pad = (size)(bitmap_addr - buf_addr);
    if (bitmap_head_pad >= buf_bytes)
        return empty;

    size avail_after_pad = buf_bytes - bitmap_head_pad;

    // Upper bound on slot count (if bitmap took zero bytes). Used to
    // size the bitmap; the final slot count is recomputed once the
    // bitmap-size is known and may be a few slots less.
    size max_slots_ub = avail_after_pad / padded_slot;
    if (max_slots_ub == 0)
        return empty;

    size bitmap_bytes = ((max_slots_ub + 63u) / 64u) * 8u;
    if (bitmap_bytes >= avail_after_pad)
        return empty;

    // Slots come after the bitmap, aligned to slot alignment.
    u64  slot_addr_raw     = bitmap_addr + bitmap_bytes;
    u64  slot_addr_aligned = (slot_addr_raw + (u64)(alignment - 1)) & ~(u64)(alignment - 1);
    size slot_head_pad     = (size)(slot_addr_aligned - slot_addr_raw);
    if (bitmap_bytes + slot_head_pad >= avail_after_pad)
        return empty;

    size slot_count = (avail_after_pad - bitmap_bytes - slot_head_pad) / padded_slot;
    if (slot_count == 0)
        return empty;

    char *bitmap = (char *)(void *)bitmap_addr;
    MemSet(bitmap, 0, bitmap_bytes);

    return (BudgetAllocator) {
        .base =
            {.allocate    = budget_allocator_allocate,
                   .resize      = budget_allocator_resize,
                   .remap       = budget_allocator_remap,
                   .deallocate  = budget_allocator_deallocate,
                   .alignment   = alignment,
                   .effort      = ALLOCATOR_EFFORT_ONCE,
                   .retry_limit = 0,
                   .__magic     = BUDGET_ALLOCATOR_MAGIC},
        .buf          = (char *)buf_in,
        .buf_bytes    = buf_bytes,
        .bitmap       = (u64 *)(void *)bitmap,
        .bitmap_words = (u32)(bitmap_bytes / 8u),
        .slots        = (char *)(void *)slot_addr_aligned,
        .slot_size    = padded_slot,
        .slot_count   = slot_count,
    };
}

BudgetAllocator BudgetAllocatorInit(void *buf, size buf_bytes, size slot_size) {
    return budget_build(buf, buf_bytes, slot_size, sizeof(void *));
}

BudgetAllocator BudgetAllocatorInitAligned(void *buf, size buf_bytes, size slot_size, size alignment) {
    return budget_build(buf, buf_bytes, slot_size, alignment);
}

void BudgetAllocatorDeinit(BudgetAllocator *self) {
    if (!self)
        return;
    // The caller still owns `buf`; just wipe our header so any
    // post-deinit dispatch trips ValidateAllocator on zero __magic.
    MemSet(self, 0, sizeof(*self));
}
