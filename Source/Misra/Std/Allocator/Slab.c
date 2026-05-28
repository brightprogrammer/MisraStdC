/// file      : std/allocator/slab.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Fixed-size slot allocator. See Slab.h for the design rationale.
///
/// One OS page == one slab. Slabs hold pure user slots; bitmaps live
/// in a packed buffer owned by the allocator. Free is O(log N slabs)
/// via ptr-mask + bsearch; alloc walks bitmaps for a free bit. Slot
/// size is fixed at init, power-of-two in [16, PAGE_SIZE].
///
/// Slot state machine:
///     FREE -- Alloc --> IN_USE -- Free --> FREE
///     pre: bit==0       pre: bit==1
///     post: bit:=1      post: bit:=0

#include <Misra/Std/Allocator/Slab.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Memory.h>

#include "_Os.h"

// =============================================================================
// Self-validation.
//
// Fast path: magic on the allocator. Catches uninitialised /
// post-deinit / _Generic mismatch escape. Always on, FORCE_INLINEd
// into every caller -- without it gcc emits a standalone copy at -O3
// because the LOG_FATAL macro expansions count against the inline-cost
// heuristic, and the standalone copy then shows up as ~50% of
// self-time in profiles.
//
// Full path (FEATURE_HEAP_VALIDATE_FULL): adds vtable / alignment /
// slot_size sanity / slabs[]-vs-bitmaps consistency / power-of-two
// check on slot_size. Costs ~7 ns / dispatch when on.
static FORCE_INLINE void slab_validate_self_fast(const SlabAllocator *self) {
    if (!self) {
        LOG_FATAL("SlabAllocator: NULL self");
    }
    if (self->base.__magic != SLAB_ALLOCATOR_MAGIC) {
        LOG_FATAL("type-confusion: allocator passed to slab_allocator_* is not a SlabAllocator");
    }
}

#if FEATURE_HEAP_VALIDATE_FULL
static void slab_validate_self_full(const SlabAllocator *self) {
    slab_validate_self_fast(self);
    if (!self->base.allocate || !self->base.resize || !self->base.remap || !self->base.deallocate) {
        LOG_FATAL("SlabAllocator: vtable function pointer is NULL");
    }
    if (self->base.alignment == 0 || (self->base.alignment & (self->base.alignment - 1)) != 0) {
        LOG_FATAL("SlabAllocator: alignment {} is not a positive power of two", (u64)self->base.alignment);
    }
    if (self->slot_size == 0) {
        LOG_FATAL("SlabAllocator: slot_size is 0");
    }
    if ((self->slot_size & (self->slot_size - 1)) != 0) {
        LOG_FATAL("SlabAllocator: slot_size {} is not a power of two", (u64)self->slot_size);
    }
    if (self->slot_size < 16) {
        LOG_FATAL("SlabAllocator: slot_size {} below 16-byte minimum", (u64)self->slot_size);
    }
    if (self->slot_size_shift == 0 || ((size)1 << self->slot_size_shift) != self->slot_size) {
        LOG_FATAL(
            "SlabAllocator: slot_size_shift {} disagrees with slot_size {}",
            (u64)self->slot_size_shift,
            (u64)self->slot_size
        );
    }
    if ((self->slabs == NULL) != (self->slabs_cap == 0)) {
        LOG_FATAL("SlabAllocator: slabs / slabs_cap mismatch ({x} / {})", (u64)self->slabs, (u64)self->slabs_cap);
    }
    if ((self->bitmaps == NULL) != (self->slabs_cap == 0)) {
        LOG_FATAL("SlabAllocator: bitmaps / slabs_cap mismatch ({x} / {})", (u64)self->bitmaps, (u64)self->slabs_cap);
    }
    if (self->slabs_len > self->slabs_cap) {
        LOG_FATAL("SlabAllocator: slabs_len {} exceeds slabs_cap {}", (u64)self->slabs_len, (u64)self->slabs_cap);
    }
}
#endif

#if FEATURE_HEAP_VALIDATE_FULL
#    define slab_validate_self(self) slab_validate_self_full(self)
#else
#    define slab_validate_self(self) slab_validate_self_fast(self)
#endif

// =============================================================================
// Bookkeeping helpers.

#define SLAB_INITIAL_CAP 8u

// Returns the OS page size. os_page_size() caches its own result.
static FORCE_INLINE size slab_page_size(const SlabAllocator *slab) {
    (void)slab;
    return os_page_size();
}

// One-time setup performed lazily on the first slab grow (we don't
// know the OS page size at init time; os_page_size() queries it on
// first call). Computes bitmap_words_per_slab and aborts if slot_size
// violates the power-of-two-in-[16,PAGE_SIZE] contract.
static void slab_finalize_runtime_consts(SlabAllocator *slab) {
    size page_size = slab_page_size(slab);
    if (slab->slot_size < 16u) {
        LOG_FATAL("SlabAllocator: slot_size {} below 16-byte minimum", (u64)slab->slot_size);
    }
    if (slab->slot_size > page_size) {
        LOG_FATAL("SlabAllocator: slot_size {} exceeds page size {}", (u64)slab->slot_size, (u64)page_size);
    }
    if ((slab->slot_size & (slab->slot_size - 1)) != 0) {
        LOG_FATAL("SlabAllocator: slot_size {} is not a power of two", (u64)slab->slot_size);
    }
    if (slab->slot_size_shift == 0 || ((size)1u << slab->slot_size_shift) != slab->slot_size) {
        LOG_FATAL(
            "SlabAllocator: slot_size_shift {} disagrees with slot_size {}",
            (u64)slab->slot_size_shift,
            (u64)slab->slot_size
        );
    }
    size slots_per_slab = page_size >> slab->slot_size_shift;
    size words          = CEIL_DIV(slots_per_slab, 64u);
    if (words == 0u) {
        words = 1u; // one slot fits in one bitmap bit; still need one word.
    }
    if (words > 255u) {
        // u8 storage for bitmap_words_per_slab caps us at 255 words ==
        // 16320 slots per slab; way past anything sensible for our
        // slot-size range.
        LOG_FATAL("SlabAllocator: bitmap words per slab {} exceeds u8 range", (u64)words);
    }
    slab->bitmap_words_per_slab = (u8)words;
}

// Grow slabs[] + bitmaps[] capacity geometrically. Both arrays are
// mmap-backed directly. Mapped byte counts are rounded up to a whole OS
// page. The unmap size on the next grow (and in Deinit) is recovered as
// os_page_round_up(old_cap * entry_size) -- identical to the value
// passed to os_page_map here. Old contents are copied via `MemCopy`;
// old buffers are unmapped.
static bool slab_grow_caps(SlabAllocator *slab) {
    u32 old_cap = slab->slabs_cap;
    u32 new_cap = old_cap ? old_cap * 2u : SLAB_INITIAL_CAP;
    if (new_cap < old_cap) {
        return false; // u32 doubling overflow
    }

    size new_slabs_bytes   = os_page_round_up((size)new_cap * sizeof(void *));
    size new_bitmaps_bytes = os_page_round_up((size)new_cap * (size)slab->bitmap_words_per_slab * sizeof(u64));

    void **new_slabs = (void **)os_page_map(new_slabs_bytes);
    if (!new_slabs) {
        return false;
    }
    // os_page_map returns kernel-zeroed pages; bitmaps need zero
    // initialisation which is already satisfied.
    u64 *new_bitmaps = (u64 *)os_page_map(new_bitmaps_bytes);
    if (!new_bitmaps) {
        os_page_unmap(new_slabs, new_slabs_bytes);
        return false;
    }

    if (slab->slabs && old_cap) {
        MemCopy(new_slabs, slab->slabs, (size)old_cap * sizeof(void *));
        MemCopy(new_bitmaps, slab->bitmaps, (size)old_cap * (size)slab->bitmap_words_per_slab * sizeof(u64));
        os_page_unmap(slab->slabs, os_page_round_up((size)old_cap * sizeof(void *)));
        os_page_unmap(slab->bitmaps, os_page_round_up((size)old_cap * (size)slab->bitmap_words_per_slab * sizeof(u64)));
    }
    slab->slabs     = new_slabs;
    slab->bitmaps   = new_bitmaps;
    slab->slabs_cap = new_cap;
    return true;
}

// Insertion-sort a freshly-grown slab page into slabs[] (sorted by
// address ascending). Shifts both slabs[] and the parallel bitmaps[]
// entries to keep indices aligned. Returns the slab's new index, or
// (u32)-1 if grow-caps failed.
static u32 slab_insert_sorted(SlabAllocator *slab, void *page_base) {
    if (slab->slabs_len == slab->slabs_cap && !slab_grow_caps(slab)) {
        return (u32)-1;
    }
    // Binary search for insertion point.
    u32 lo = 0, hi = slab->slabs_len;
    while (lo < hi) {
        u32 mid = lo + (hi - lo) / 2u;
        if ((u64)slab->slabs[mid] < (u64)page_base) {
            lo = mid + 1u;
        } else {
            hi = mid;
        }
    }
    u32 ins     = lo;
    u32 bw      = slab->bitmap_words_per_slab;
    u32 to_move = slab->slabs_len - ins;
    if (to_move > 0u) {
        // Shift slabs[ins..len] right by one.
        MemMove(&slab->slabs[ins + 1u], &slab->slabs[ins], (size)to_move * sizeof(void *));
        // Shift bitmaps[ins..len] (each bw u64 words) right by one entry.
        MemMove(
            &slab->bitmaps[(size)(ins + 1u) * (size)bw],
            &slab->bitmaps[(size)ins * (size)bw],
            (size)to_move * (size)bw * sizeof(u64)
        );
    }
    slab->slabs[ins] = page_base;
    // Zero the new bitmap entry, then set the tail bits (bits >=
    // slots_per_slab) to 1 so ctz on the inverted bitmap never reports
    // them as free. For slot_size >= page_size/64 the bitmap has no
    // tail bits; for larger slot_size some of the low bits represent
    // valid slots and the rest are tail.
    u64 *bm = &slab->bitmaps[(size)ins * (size)bw];
    for (u32 w = 0; w < bw; w++) {
        bm[w] = 0u;
    }
    size slots_per_slab = slab_page_size(slab) >> slab->slot_size_shift;
    if (slots_per_slab < 64u) {
        bm[0] = ~(((u64)1 << slots_per_slab) - 1u);
    }
    slab->slabs_len++;
    return ins;
}

// Bsearch slabs[] for the slab whose page-base equals `page_base`.
// Returns (u32)-1 when not found.
static FORCE_INLINE u32 slab_find_by_page(const SlabAllocator *slab, void *page_base) {
    u32 lo = 0, hi = slab->slabs_len;
    while (lo < hi) {
        u32   mid    = lo + (hi - lo) / 2u;
        void *mid_pg = slab->slabs[mid];
        if (mid_pg == page_base) {
            return mid;
        }
        if ((u64)mid_pg < (u64)page_base) {
            lo = mid + 1u;
        } else {
            hi = mid;
        }
    }
    return (u32)-1;
}

// Map one OS page directly and install it as a new slab. On first ever
// grow, also runs the lazy runtime-constant init. Returns the slab's
// index, or (u32)-1 on failure.
static u32 slab_grow_one(SlabAllocator *slab) {
    if (slab->bitmap_words_per_slab == 0u) {
        slab_finalize_runtime_consts(slab);
    }
    size  page_size = slab_page_size(slab);
    void *page      = os_page_map(page_size);
    if (!page) {
        return (u32)-1;
    }
    u32 idx = slab_insert_sorted(slab, page);
    if (idx == (u32)-1) {
        os_page_unmap(page, page_size);
        return (u32)-1;
    }
    return idx;
}

// =============================================================================
// Public alloc / free / resize / remap.

void *slab_allocator_allocate(SlabAllocator *self, size bytes, i8 zeroed) {
    slab_validate_self(self);

    if (bytes == 0 || bytes > self->slot_size) {
        return NULL;
    }

    u32 bw = self->bitmap_words_per_slab;

    // Walk slabs in index order; for each, scan its bitmap words for a
    // free bit. First fit wins. Tail bits in the last word (if any)
    // are pre-set to 1 at slab-insert time, so ctz on the inverted
    // word can never spuriously hit them.
    //
    // The pre-redesign code re-read bm[w] here and LOG_FATAL'd if the
    // freshly-found bit was already set, defending against an
    // inconsistent-with-itself bitmap word. That check was a
    // self-consistency probe against rare corruption -- not a useful
    // safety net under the contiguous-bitmaps layout, where the only
    // way the bit can be set is if `CTZ64(~bm[w])` is broken (which
    // is platform code we trust). Free's double-free check (below)
    // still defends against the more useful real-world failure mode:
    // releasing a slot twice from user code.
    for (u32 i = 0; i < self->slabs_len; i++) {
        u64 *bm = &self->bitmaps[(size)i * (size)bw];
        for (u32 w = 0; w < bw; w++) {
            u64 inv = ~bm[w];
            if (inv == 0u) {
                continue;
            }
            u32 bit         = CTZ64(inv);
            bm[w]          |= ((u64)1 << bit);
            u32   slot_idx  = w * 64u + bit;
            void *slot      = (u8 *)self->slabs[i] + ((size)slot_idx << self->slot_size_shift);
            if (zeroed) {
                MemSet(slot, 0, self->slot_size);
            }
            return slot;
        }
    }

    // All slabs full -- grow and take the first slot of the new slab.
    u32 idx = slab_grow_one(self);
    if (idx == (u32)-1) {
        return NULL;
    }
    // bitmap_words_per_slab might have just been set by the first
    // grow; re-read.
    bw          = self->bitmap_words_per_slab;
    u64 *bm     = &self->bitmaps[(size)idx * (size)bw];
    bm[0]      |= 1u;
    void *slot  = self->slabs[idx];
    if (zeroed) {
        MemSet(slot, 0, self->slot_size);
    }
    return slot;
}

i8 slab_allocator_resize(SlabAllocator *self, void *ptr, size new_size) {
    slab_validate_self(self);
    (void)ptr;
    return new_size <= self->slot_size ? 1 : 0;
}

void *slab_allocator_remap(SlabAllocator *self, void *ptr, size new_size) {
    slab_validate_self(self);
    if (!ptr) {
        return slab_allocator_allocate(self, new_size, true);
    }
    if (new_size == 0) {
        slab_allocator_deallocate(self, ptr);
        return NULL;
    }
    return new_size <= self->slot_size ? ptr : NULL;
}

size slab_allocator_deallocate(SlabAllocator *self, void *ptr) {
    slab_validate_self(self);
    if (!ptr) {
        return 0;
    }

    // ptr -> owning slab via single mask + bsearch. `os_page_map`
    // returns page-aligned regions, so `ptr & ~(page_size - 1)` is
    // exactly the slab base address.
    size  page_size = slab_page_size(self);
    void *page_base = (void *)((u64)ptr & ~((u64)page_size - 1u));
    u32   idx       = slab_find_by_page(self, page_base);
    if (idx == (u32)-1) {
        LOG_FATAL("slab_free: foreign ptr {x} not in any slab", (u64)ptr);
        return 0;
    }
    // Offset within slab -> slot index via shift (slot_size is pow-of-2).
    size offset   = (size)((u64)ptr - (u64)page_base);
    size slot_idx = offset >> self->slot_size_shift;
    // The shift naturally enforces alignment: if the user passed a
    // mid-slot pointer, offset & (slot_size-1) is non-zero -> we
    // round it down and clear the wrong bit. Catch this explicitly.
    if ((offset & (self->slot_size - 1u)) != 0u) {
        LOG_FATAL("slab_free: misaligned ptr {x} (slot size {})", (u64)ptr, (u64)self->slot_size);
        return 0;
    }
    u32  bw   = self->bitmap_words_per_slab;
    u64 *bm   = &self->bitmaps[(size)idx * (size)bw];
    u32  w    = (u32)(slot_idx >> 6);
    u32  bit  = (u32)(slot_idx & 63u);
    u64  mask = (u64)1 << bit;
    if (!(bm[w] & mask)) {
        LOG_FATAL("slab_free: double-free of {x} (slot {})", (u64)ptr, (u64)slot_idx);
        return 0;
    }
    bm[w] &= ~mask;
    return self->slot_size;
}

void SlabAllocatorDeinit(SlabAllocator *self) {
    if (!self) {
        return;
    }
    // Unmap each slab page. Each was mapped as exactly one OS page.
    size page_size = os_page_size();
    for (u32 i = 0; i < self->slabs_len; i++) {
        os_page_unmap(self->slabs[i], page_size);
    }
    // Unmap the bookkeeping arrays. The exact mapped byte count is
    // recoverable as os_page_round_up(slabs_cap * entry_size), which
    // is identical to what slab_grow_caps passed to os_page_map.
    if (self->slabs) {
        os_page_unmap(self->slabs, os_page_round_up((size)self->slabs_cap * sizeof(void *)));
    }
    if (self->bitmaps) {
        os_page_unmap(
            self->bitmaps,
            os_page_round_up((size)self->slabs_cap * (size)self->bitmap_words_per_slab * sizeof(u64))
        );
    }
    MemSet(self, 0, sizeof(*self));
}
