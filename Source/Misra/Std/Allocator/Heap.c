/// file      : std/allocator/heap.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Bitmap-backed heap allocator. See Heap.h for the design notes.
///
/// Every slot has exactly two states: FREE (bit = 0) and IN_USE
/// (bit = 1). Alloc transitions FREE -> IN_USE; Free transitions
/// IN_USE -> FREE. Both transitions verify the precondition before
/// mutating the bitmap -- alloc must find a 0 bit (else bitmap
/// corruption), free must find a 1 bit (else double-free). EVERY
/// failed precondition aborts via LOG_FATAL with a backtrace -- a
/// bad free is a caller-side memory-safety bug, not a recoverable
/// error condition, and continuing past it would leave the program
/// in undefined state.
///
/// User pages are opaque after Alloc returns. No metadata is written
/// through the user pointer for the rest of its life.

#include <Misra/Std/Allocator/Heap.h>
#include <Misra/Std/Allocator/Page.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Memory.h>

// HEAP_PAGES_PER_OS_PAGE is in Heap.h. The OS page size derived from
// it is the unit we ask PageAllocator for on each grow.
#define HEAP_OS_PAGE_SIZE (HEAP_PAGE_SIZE * HEAP_PAGES_PER_OS_PAGE)

// =============================================================================
// Self-validation.

// Fast path: magic on the allocator + magic on the embedded
// PageAllocator. Catches uninitialised / post-deinit / _Generic
// mismatch escape. Always on.
//
// Full path (FEATURE_HEAP_VALIDATE_FULL): adds vtable / alignment /
// per-class invariant cross-checks plus a forced volatile read of
// each descriptor array's first byte, to surface freed or torn
// metadata at the dispatch boundary. Costs ~22 ns / dispatch on x86.
static inline void heap_validate_self_fast(const Allocator *self) {
    if (!self) {
        LOG_FATAL("HeapAllocator: NULL self");
    }
    if (self->__magic != HEAP_ALLOCATOR_MAGIC) {
        LOG_FATAL("type-confusion: allocator passed to heap_allocator_* is not a HeapAllocator");
    }
    const HeapAllocator *h = (const HeapAllocator *)self;
    if (h->page.base.__magic != PAGE_ALLOCATOR_MAGIC) {
        LOG_FATAL("HeapAllocator: embedded PageAllocator has bad magic");
    }
}

#if FEATURE_HEAP_VALIDATE_FULL
static void heap_validate_self_full(const Allocator *self) {
    heap_validate_self_fast(self);
    if (!self->allocate || !self->resize || !self->remap || !self->deallocate) {
        LOG_FATAL("HeapAllocator: vtable function pointer is NULL");
    }
    if (self->alignment == 0 || (self->alignment & (self->alignment - 1)) != 0) {
        LOG_FATAL("HeapAllocator: alignment {} is not a positive power of two", (u64)self->alignment);
    }
    const HeapAllocator *h = (const HeapAllocator *)self;

#    define HEAP_CHECK_CLASS(_ptr, _len, _cap, _name)                                                                  \
        do {                                                                                                           \
            if ((_len) > (_cap)) {                                                                                     \
                LOG_FATAL("HeapAllocator: " _name "_len {} exceeds " _name "_cap {}", (u64)(_len), (u64)(_cap));       \
            }                                                                                                          \
            if (((_ptr) == NULL) != ((_cap) == 0)) {                                                                   \
                LOG_FATAL("HeapAllocator: " _name " / " _name "_cap mismatch ({x} / {})", (u64)(_ptr), (u64)(_cap));   \
            }                                                                                                          \
            if ((_len) > 0 && !(_ptr)) {                                                                               \
                LOG_FATAL("HeapAllocator: " _name "_len {} with NULL " _name, (u64)(_len));                            \
            }                                                                                                          \
            if ((_ptr)) {                                                                                              \
                (void)(*(const volatile char *)(const void *)(_ptr));                                                  \
            }                                                                                                          \
        } while (0)

    HEAP_CHECK_CLASS(h->s, h->s_len, h->s_cap, "s");
    HEAP_CHECK_CLASS(h->m, h->m_len, h->m_cap, "m");
    HEAP_CHECK_CLASS(h->l, h->l_len, h->l_cap, "l");
    HEAP_CHECK_CLASS(h->xl, h->xl_len, h->xl_cap, "xl");

#    undef HEAP_CHECK_CLASS
}
#endif

#if FEATURE_HEAP_VALIDATE_FULL
#    define heap_validate_self(self) heap_validate_self_full(self)
#else
#    define heap_validate_self(self) heap_validate_self_fast(self)
#endif

// Smallest bin slot is 16-byte aligned by construction. Stronger
// alignment demands bypass bins and route to the XL list (page-aligned).
static bool heap_alignment_demands_passthrough(Allocator *self) {
    return self->alignment > 16;
}

// =============================================================================
// Descriptor-array helpers. Per-class arrays are page-backed (acquired
// from PageAllocator) and stay sorted by `page` address so free() can
// binary-search them. The first field of every descriptor type is a
// `void *page`, which is the sort key.

#define HEAP_DESC_INITIAL_CAP 16u

static bool heap_grow_array(HeapAllocator *heap, void **arr_ptr, u32 *cap_ptr, u32 entry_size) {
    u32 old_cap = *cap_ptr;
    // u32 doubling overflow guard.
    if (old_cap > ((u32)-1) / 2u) {
        return false;
    }
    u32   new_cap   = old_cap ? old_cap * 2u : HEAP_DESC_INITIAL_CAP;
    size  new_bytes = (size)new_cap * (size)entry_size;
    void *fresh     = AllocatorAlloc(&heap->page.base, new_bytes, true);
    if (!fresh)
        return false;
    if (*arr_ptr && old_cap) {
        MemCopy(fresh, *arr_ptr, (size)old_cap * (size)entry_size);
        AllocatorFree(&heap->page.base, *arr_ptr);
    }
    *arr_ptr = fresh;
    *cap_ptr = new_cap;
    return true;
}

static u32
    heap_insert_sorted(HeapAllocator *heap, void **arr_ptr, u32 *len_ptr, u32 *cap_ptr, u32 entry_size, void *entry) {
    if (*len_ptr == *cap_ptr && !heap_grow_array(heap, arr_ptr, cap_ptr, entry_size)) {
        return (u32)-1;
    }
    char *base    = (char *)*arr_ptr;
    u64   new_key = (u64) * (void **)entry;
    u32   idx     = *len_ptr;
    while (idx > 0) {
        u64 prev = (u64) * (void **)(base + (idx - 1) * entry_size);
        if (prev < new_key)
            break;
        idx -= 1;
    }
    if (idx < *len_ptr) {
        MemMove(base + (idx + 1) * entry_size, base + idx * entry_size, (size)(*len_ptr - idx) * entry_size);
    }
    MemCopy(base + idx * entry_size, entry, entry_size);
    *len_ptr += 1;
    return idx;
}

static void heap_remove_at(void *arr, u32 *len_ptr, u32 idx, u32 entry_size) {
    char *base = (char *)arr;
    if (idx + 1 < *len_ptr) {
        MemMove(base + idx * entry_size, base + (idx + 1) * entry_size, (size)(*len_ptr - idx - 1) * entry_size);
    }
    *len_ptr -= 1;
}

static u32 heap_find_by_page(const void *arr, u32 len, u32 entry_size, void *page_addr) {
    const char *base = (const char *)arr;
    u64         key  = (u64)page_addr;
    u32         lo = 0, hi = len;
    while (lo < hi) {
        u32 mid    = lo + ((hi - lo) >> 1);
        u64 mid_pg = (u64) * (void *const *)(base + mid * entry_size);
        if (mid_pg == key)
            return mid;
        if (mid_pg < key)
            lo = mid + 1;
        else
            hi = mid;
    }
    return (u32)-1;
}

// MSVC: no GCC/Clang builtins; use _BitScanForward[64] from <intrin.h>.
// Both branches assume x != 0 (the callers all gate on `if (inv != 0)`
// or its equivalent before invoking these helpers).
#if defined(_MSC_VER) && !defined(__clang__)
#    include <intrin.h>
static u32 ctz64(u64 x) {
    unsigned long idx;
    _BitScanForward64(&idx, x);
    return (u32)idx;
}
static u32 ctz32(u32 x) {
    unsigned long idx;
    _BitScanForward(&idx, x);
    return (u32)idx;
}
#else
static u32 ctz64(u64 x) {
    return (u32)__builtin_ctzll(x);
}
static u32 ctz32(u32 x) {
    return (u32)__builtin_ctz(x);
}
#endif

// =============================================================================
// Class S. Three sub-bins (16/32/64), each with its own bitmap field.

// Bit-width-safe masks. Shifting by the full width of the type is
// undefined in C; the count == width case must produce all-ones via a
// different path. Used by both the find-free helpers below and the
// max_slots guards in class M/L (where count is always < 32, so the
// guard is a no-op there).
static u64 mask_low_u64(u32 count) {
    return count >= 64u ? ~(u64)0 : ((u64)1 << count) - 1u;
}
static u32 mask_low_u32(u32 count) {
    return count >= 32u ? ~(u32)0 : ((u32)1 << count) - 1u;
}

// Find first free bit in `bitmap` masked to `count` bits. Returns -1 if none.
static i32 heap_find_free_bit_64(u64 bitmap, u32 count) {
    u64 inv = ~bitmap & mask_low_u64(count);
    return inv ? (i32)ctz64(inv) : -1;
}
static i32 heap_find_free_bit_32(u32 bitmap, u32 count) {
    u32 inv = ~bitmap & mask_low_u32(count);
    return inv ? (i32)ctz32(inv) : -1;
}

static void *heap_alloc_s(HeapAllocator *heap, u32 slot_size) {
    // Scan existing class-S pages for a free slot in the sub-bin
    // corresponding to slot_size. State assertion: ctz finds a 0 bit
    // by construction, so the bit we set must be 0 before. If it isn't,
    // bitmap memory has been corrupted -- abort.
    for (u32 i = 0; i < heap->s_len; i++) {
        HeapPageS *d = &heap->s[i];

        if (slot_size == 16) {
            i32 bit = heap_find_free_bit_64(d->bitmap_16, HEAP_S_16_COUNT);
            if (bit < 0)
                continue;
            if (d->bitmap_16 & ((u64)1 << bit)) {
                LOG_FATAL("HeapAllocator S/16 bitmap corruption at page {x} bit {}", (u64)d->page, (u64)bit);
            }
            d->bitmap_16 |= ((u64)1 << bit);
            return (char *)d->page + HEAP_S_16_OFFSET + (u32)bit * 16u;
        }
        if (slot_size == 32) {
            i32 bit = heap_find_free_bit_32(d->bitmap_32, HEAP_S_32_COUNT);
            if (bit < 0)
                continue;
            if (d->bitmap_32 & ((u32)1 << bit)) {
                LOG_FATAL("HeapAllocator S/32 bitmap corruption at page {x} bit {}", (u64)d->page, (u64)bit);
            }
            d->bitmap_32 |= ((u32)1 << bit);
            return (char *)d->page + HEAP_S_32_OFFSET + (u32)bit * 32u;
        }
        /* slot_size == 64 */
        i32 bit = heap_find_free_bit_32(d->bitmap_64, HEAP_S_64_COUNT);
        if (bit < 0)
            continue;
        if (d->bitmap_64 & ((u32)1 << bit)) {
            LOG_FATAL("HeapAllocator S/64 bitmap corruption at page {x} bit {}", (u64)d->page, (u64)bit);
        }
        d->bitmap_64 |= ((u32)1 << bit);
        return (char *)d->page + HEAP_S_64_OFFSET + (u32)bit * 64u;
    }

    // No existing page has space -- grow. Allocate one OS page (4 KiB on
    // most platforms, 16 KiB on macOS aarch64), carve it into
    // HEAP_PAGES_PER_OS_PAGE heap pages, register one descriptor per
    // heap page. Take the first slot from the lowest-address (first)
    // new descriptor.
    void *base = AllocatorAlloc(&heap->page.base, HEAP_OS_PAGE_SIZE, false);
    if (!base)
        return NULL;
    u32 first_idx = (u32)-1;
    for (u32 i = 0; i < HEAP_PAGES_PER_OS_PAGE; i++) {
        void     *page_i = (char *)base + i * HEAP_PAGE_SIZE;
        HeapPageS desc   = {.page = page_i, .bitmap_16 = 0, .bitmap_32 = 0, .bitmap_64 = 0};
        u32 idx = heap_insert_sorted(heap, (void **)&heap->s, &heap->s_len, &heap->s_cap, sizeof(HeapPageS), &desc);
        if (idx == (u32)-1) {
            // Roll back any descriptors already inserted from this base.
            for (u32 j = 0; j < i; j++) {
                void *p_j  = (char *)base + j * HEAP_PAGE_SIZE;
                u32   ix_j = heap_find_by_page(heap->s, heap->s_len, sizeof(HeapPageS), p_j);
                if (ix_j != (u32)-1)
                    heap_remove_at(heap->s, &heap->s_len, ix_j, sizeof(HeapPageS));
            }
            AllocatorFree(&heap->page.base, base);
            return NULL;
        }
        if (i == 0)
            first_idx = idx;
    }
    HeapPageS *d = &heap->s[first_idx];
    if (slot_size == 16) {
        d->bitmap_16 |= 1u;
        return (char *)base + HEAP_S_16_OFFSET;
    }
    if (slot_size == 32) {
        d->bitmap_32 |= 1u;
        return (char *)base + HEAP_S_32_OFFSET;
    }
    d->bitmap_64 |= 1u;
    return (char *)base + HEAP_S_64_OFFSET;
}

// idx is the descriptor index the caller already resolved via
// heap_recover_size; we skip the duplicate heap_find_by_page that
// the freed-page lookup would otherwise repeat.
static void heap_free_s(HeapAllocator *heap, void *ptr, u32 slot_size, u32 idx) {
    void      *page_base = (void *)((u64)ptr & ~(u64)(HEAP_PAGE_SIZE - 1u));
    HeapPageS *d         = &heap->s[idx];
    u64        off       = (u64)ptr - (u64)page_base;

    u32 region_off, region_count;
    if (slot_size == 16) {
        region_off   = HEAP_S_16_OFFSET;
        region_count = HEAP_S_16_COUNT;
    } else if (slot_size == 32) {
        region_off   = HEAP_S_32_OFFSET;
        region_count = HEAP_S_32_COUNT;
    } else {
        region_off   = HEAP_S_64_OFFSET;
        region_count = HEAP_S_64_COUNT;
    }

    if (off < region_off || off >= region_off + (u64)slot_size * region_count) {
        LOG_FATAL("heap_free: ptr {x} not in S/{} region", (u64)ptr, (u64)slot_size);
        return;
    }
    u64 in_region = off - region_off;
    if (in_region % slot_size != 0) {
        LOG_FATAL("heap_free: misaligned ptr {x} for slot size {}", (u64)ptr, (u64)slot_size);
        return;
    }
    u32 bit = (u32)(in_region / slot_size);

    // State assertion: the bit must currently be IN_USE (1). Otherwise
    // this is a double-free.
    if (slot_size == 16) {
        if (!(d->bitmap_16 & ((u64)1 << bit))) {
            LOG_FATAL("heap_free: double-free of {x} (S/16 bit {})", (u64)ptr, (u64)bit);
            return;
        }
        d->bitmap_16 &= ~((u64)1 << bit);
        return;
    }
    if (slot_size == 32) {
        if (!(d->bitmap_32 & ((u32)1 << bit))) {
            LOG_FATAL("heap_free: double-free of {x} (S/32 bit {})", (u64)ptr, (u64)bit);
            return;
        }
        d->bitmap_32 &= ~((u32)1 << bit);
        return;
    }
    if (!(d->bitmap_64 & ((u32)1 << bit))) {
        LOG_FATAL("heap_free: double-free of {x} (S/64 bit {})", (u64)ptr, (u64)bit);
        return;
    }
    d->bitmap_64 &= ~((u32)1 << bit);
}

// =============================================================================
// Class M. Three sub-bins (128/256/512) packed into one u16 bitmap.

static void *heap_alloc_m(HeapAllocator *heap, u32 slot_size) {
    u32 region_off, region_count, region_shift;
    if (slot_size == 128) {
        region_off   = HEAP_M_128_OFFSET;
        region_count = HEAP_M_128_COUNT;
        region_shift = HEAP_M_128_SHIFT;
    } else if (slot_size == 256) {
        region_off   = HEAP_M_256_OFFSET;
        region_count = HEAP_M_256_COUNT;
        region_shift = HEAP_M_256_SHIFT;
    } else {
        region_off   = HEAP_M_512_OFFSET;
        region_count = HEAP_M_512_COUNT;
        region_shift = HEAP_M_512_SHIFT;
    }

    for (u32 i = 0; i < heap->m_len; i++) {
        HeapPageM *d   = &heap->m[i];
        u32        sub = (~(u32)d->bitmap >> region_shift) & (((u32)1 << region_count) - 1u);
        if (!sub)
            continue;
        u32 bit  = ctz32(sub);
        u32 mask = (u32)1 << (bit + region_shift);
        if (d->bitmap & (u16)mask) {
            LOG_FATAL(
                "HeapAllocator M/{} bitmap corruption at page {x} bit {}",
                (u64)slot_size,
                (u64)d->page,
                (u64)bit
            );
        }
        d->bitmap |= (u16)mask;
        return (char *)d->page + region_off + bit * slot_size;
    }

    void *base = AllocatorAlloc(&heap->page.base, HEAP_OS_PAGE_SIZE, false);
    if (!base)
        return NULL;
    u32 first_idx = (u32)-1;
    for (u32 i = 0; i < HEAP_PAGES_PER_OS_PAGE; i++) {
        void     *page_i = (char *)base + i * HEAP_PAGE_SIZE;
        HeapPageM desc   = {.page = page_i, .bitmap = 0};
        u32 idx = heap_insert_sorted(heap, (void **)&heap->m, &heap->m_len, &heap->m_cap, sizeof(HeapPageM), &desc);
        if (idx == (u32)-1) {
            for (u32 j = 0; j < i; j++) {
                void *p_j  = (char *)base + j * HEAP_PAGE_SIZE;
                u32   ix_j = heap_find_by_page(heap->m, heap->m_len, sizeof(HeapPageM), p_j);
                if (ix_j != (u32)-1)
                    heap_remove_at(heap->m, &heap->m_len, ix_j, sizeof(HeapPageM));
            }
            AllocatorFree(&heap->page.base, base);
            return NULL;
        }
        if (i == 0)
            first_idx = idx;
    }
    heap->m[first_idx].bitmap |= (u16)((u32)1 << region_shift);
    return (char *)base + region_off;
}

// idx is the descriptor index the caller already resolved via
// heap_recover_size; we skip the duplicate heap_find_by_page that
// the freed-page lookup would otherwise repeat.
static void heap_free_m(HeapAllocator *heap, void *ptr, u32 slot_size, u32 idx) {
    void      *page_base = (void *)((u64)ptr & ~(u64)(HEAP_PAGE_SIZE - 1u));
    HeapPageM *d         = &heap->m[idx];
    u64        off       = (u64)ptr - (u64)page_base;

    u32 region_off, region_count, region_shift;
    if (slot_size == 128) {
        region_off   = HEAP_M_128_OFFSET;
        region_count = HEAP_M_128_COUNT;
        region_shift = HEAP_M_128_SHIFT;
    } else if (slot_size == 256) {
        region_off   = HEAP_M_256_OFFSET;
        region_count = HEAP_M_256_COUNT;
        region_shift = HEAP_M_256_SHIFT;
    } else {
        region_off   = HEAP_M_512_OFFSET;
        region_count = HEAP_M_512_COUNT;
        region_shift = HEAP_M_512_SHIFT;
    }

    if (off < region_off || off >= region_off + (u64)slot_size * region_count) {
        LOG_FATAL("heap_free: ptr {x} not in M/{} region", (u64)ptr, (u64)slot_size);
        return;
    }
    u64 in_region = off - region_off;
    if (in_region % slot_size != 0) {
        LOG_FATAL("heap_free: misaligned ptr {x} for slot size {}", (u64)ptr, (u64)slot_size);
        return;
    }
    u32 bit  = (u32)(in_region / slot_size);
    u16 mask = (u16)((u32)1 << (bit + region_shift));
    if (!(d->bitmap & mask)) {
        LOG_FATAL("heap_free: double-free of {x} (M/{} bit {})", (u64)ptr, (u64)slot_size, (u64)bit);
        return;
    }
    d->bitmap &= (u16)~mask;
}

// =============================================================================
// Class L. Two sub-bins (1024/2048) packed into one u8 bitmap.

static void *heap_alloc_l(HeapAllocator *heap, u32 slot_size) {
    u32 region_off, region_count, region_shift;
    if (slot_size == 1024) {
        region_off   = HEAP_L_1024_OFFSET;
        region_count = HEAP_L_1024_COUNT;
        region_shift = HEAP_L_1024_SHIFT;
    } else {
        region_off   = HEAP_L_2048_OFFSET;
        region_count = HEAP_L_2048_COUNT;
        region_shift = HEAP_L_2048_SHIFT;
    }

    for (u32 i = 0; i < heap->l_len; i++) {
        HeapPageL *d   = &heap->l[i];
        u32        sub = (~(u32)d->bitmap >> region_shift) & (((u32)1 << region_count) - 1u);
        if (!sub)
            continue;
        u32 bit  = ctz32(sub);
        u32 mask = (u32)1 << (bit + region_shift);
        if (d->bitmap & (u8)mask) {
            LOG_FATAL(
                "HeapAllocator L/{} bitmap corruption at page {x} bit {}",
                (u64)slot_size,
                (u64)d->page,
                (u64)bit
            );
        }
        d->bitmap |= (u8)mask;
        return (char *)d->page + region_off + bit * slot_size;
    }

    void *base = AllocatorAlloc(&heap->page.base, HEAP_OS_PAGE_SIZE, false);
    if (!base)
        return NULL;
    u32 first_idx = (u32)-1;
    for (u32 i = 0; i < HEAP_PAGES_PER_OS_PAGE; i++) {
        void     *page_i = (char *)base + i * HEAP_PAGE_SIZE;
        HeapPageL desc   = {.page = page_i, .bitmap = 0};
        u32 idx = heap_insert_sorted(heap, (void **)&heap->l, &heap->l_len, &heap->l_cap, sizeof(HeapPageL), &desc);
        if (idx == (u32)-1) {
            for (u32 j = 0; j < i; j++) {
                void *p_j  = (char *)base + j * HEAP_PAGE_SIZE;
                u32   ix_j = heap_find_by_page(heap->l, heap->l_len, sizeof(HeapPageL), p_j);
                if (ix_j != (u32)-1)
                    heap_remove_at(heap->l, &heap->l_len, ix_j, sizeof(HeapPageL));
            }
            AllocatorFree(&heap->page.base, base);
            return NULL;
        }
        if (i == 0)
            first_idx = idx;
    }
    heap->l[first_idx].bitmap |= (u8)((u32)1 << region_shift);
    return (char *)base + region_off;
}

// idx is the descriptor index the caller already resolved via
// heap_recover_size; we skip the duplicate heap_find_by_page that
// the freed-page lookup would otherwise repeat.
static void heap_free_l(HeapAllocator *heap, void *ptr, u32 slot_size, u32 idx) {
    void      *page_base = (void *)((u64)ptr & ~(u64)(HEAP_PAGE_SIZE - 1u));
    HeapPageL *d         = &heap->l[idx];
    u64        off       = (u64)ptr - (u64)page_base;

    u32 region_off, region_count, region_shift;
    if (slot_size == 1024) {
        region_off   = HEAP_L_1024_OFFSET;
        region_count = HEAP_L_1024_COUNT;
        region_shift = HEAP_L_1024_SHIFT;
    } else {
        region_off   = HEAP_L_2048_OFFSET;
        region_count = HEAP_L_2048_COUNT;
        region_shift = HEAP_L_2048_SHIFT;
    }

    if (off < region_off || off >= region_off + (u64)slot_size * region_count) {
        LOG_FATAL("heap_free: ptr {x} not in L/{} region", (u64)ptr, (u64)slot_size);
        return;
    }
    u64 in_region = off - region_off;
    if (in_region % slot_size != 0) {
        LOG_FATAL("heap_free: misaligned ptr {x} for slot size {}", (u64)ptr, (u64)slot_size);
        return;
    }
    u32 bit  = (u32)(in_region / slot_size);
    u8  mask = (u8)((u32)1 << (bit + region_shift));
    if (!(d->bitmap & mask)) {
        LOG_FATAL("heap_free: double-free of {x} (L/{} bit {})", (u64)ptr, (u64)slot_size, (u64)bit);
        return;
    }
    d->bitmap &= (u8)~mask;
}

// =============================================================================
// Class XL. One page-aligned region per allocation. No bitmap; the
// descriptor existence is the in-use bit.

static void *heap_alloc_xl(HeapAllocator *heap, size bytes, i8 zeroed) {
    // num_pages is u32 on the descriptor; refuse requests beyond u32 pages.
    size pages_needed = (bytes + HEAP_PAGE_SIZE - 1u) / HEAP_PAGE_SIZE;
    if (pages_needed > (size)(u32)-1) {
        return NULL;
    }
    u32   num_pages = (u32)pages_needed;
    size  full      = (size)num_pages * HEAP_PAGE_SIZE;
    void *page      = AllocatorAlloc(&heap->page.base, full, zeroed);
    if (!page)
        return NULL;
    HeapPageXL desc = {.page = page, .num_pages = num_pages};
    if (heap_insert_sorted(heap, (void **)&heap->xl, &heap->xl_len, &heap->xl_cap, sizeof(HeapPageXL), &desc) ==
        (u32)-1) {
        AllocatorFree(&heap->page.base, page);
        return NULL;
    }
    return page;
}

// Release the XL allocation at descriptor index `idx`. Caller has
// already validated that `idx` is in range and that the user pointer
// matches the descriptor base. Returns bytes released.
static size heap_free_xl_at(HeapAllocator *heap, u32 idx) {
    HeapPageXL *e     = &heap->xl[idx];
    size        bytes = (size)e->num_pages * HEAP_PAGE_SIZE;
    AllocatorFree(&heap->page.base, e->page);
    heap_remove_at(heap->xl, &heap->xl_len, idx, sizeof(HeapPageXL));
    return bytes;
}

// =============================================================================
// Public alloc / free / resize / remap dispatch.

// Map a request size to its rounded-up slot size, or 0 for XL.
static u32 heap_slot_size_for(size n) {
    if (n <= 16)
        return 16u;
    if (n <= 32)
        return 32u;
    if (n <= 64)
        return 64u;
    if (n <= 128)
        return 128u;
    if (n <= 256)
        return 256u;
    if (n <= 512)
        return 512u;
    if (n <= 1024)
        return 1024u;
    if (n <= 2048)
        return 2048u;
    return 0u;
}

void *heap_allocator_allocate(Allocator *self, size bytes, i8 zeroed) {
    heap_validate_self(self);
    HeapAllocator *heap = (HeapAllocator *)self;
    if (!bytes)
        return NULL;
    if (heap_alignment_demands_passthrough(self) || bytes > 2048) {
        return heap_alloc_xl(heap, bytes, zeroed);
    }

    u32   slot = heap_slot_size_for(bytes);
    void *out;
    if (slot <= 64)
        out = heap_alloc_s(heap, slot);
    else if (slot <= 512)
        out = heap_alloc_m(heap, slot);
    else
        out = heap_alloc_l(heap, slot);

    if (out && zeroed)
        MemSet(out, 0, slot);
    return out;
}

// Look up the slot size for `ptr` and write its owning descriptor
// index into *idx_out. Returns 0 if foreign (idx_out is left as
// (u32)-1). The four class arrays are disjoint -- a page lives in
// exactly one class -- so first match wins, and the index is into
// the array of whichever class the returned size identifies:
// xl for size > 2048, l for 1024 / 2048, m for 128 / 256 / 512,
// s for 16 / 32 / 64. Callers consume the index to skip a second
// heap_find_by_page in the matching free path.
static size heap_recover_size(HeapAllocator *heap, void *ptr, u32 *idx_out) {
    *idx_out = (u32)-1;
    if (!ptr)
        return 0;

    void *page_base = (void *)((u64)ptr & ~(u64)(HEAP_PAGE_SIZE - 1u));

    u32 i = heap_find_by_page(heap->xl, heap->xl_len, sizeof(HeapPageXL), page_base);
    if (i != (u32)-1) {
        *idx_out = i;
        return (size)heap->xl[i].num_pages * HEAP_PAGE_SIZE;
    }

    u64 off = (u64)ptr - (u64)page_base;

    i = heap_find_by_page(heap->l, heap->l_len, sizeof(HeapPageL), page_base);
    if (i != (u32)-1) {
        *idx_out = i;
        return (off < HEAP_L_2048_OFFSET) ? 1024u : 2048u;
    }
    i = heap_find_by_page(heap->m, heap->m_len, sizeof(HeapPageM), page_base);
    if (i != (u32)-1) {
        *idx_out = i;
        if (off < HEAP_M_256_OFFSET)
            return 128u;
        if (off < HEAP_M_512_OFFSET)
            return 256u;
        return 512u;
    }
    i = heap_find_by_page(heap->s, heap->s_len, sizeof(HeapPageS), page_base);
    if (i != (u32)-1) {
        *idx_out = i;
        if (off < HEAP_S_32_OFFSET)
            return 16u;
        if (off < HEAP_S_64_OFFSET)
            return 32u;
        return 64u;
    }
    return 0;
}

size heap_allocator_deallocate(Allocator *self, void *ptr) {
    heap_validate_self(self);
    HeapAllocator *heap = (HeapAllocator *)self;
    if (!ptr)
        return 0;

    u32  idx;
    size slot = heap_recover_size(heap, ptr, &idx);
    if (slot == 0) {
        LOG_FATAL("heap_free: foreign ptr {x} (not in any class list)", (u64)ptr);
        return 0;
    }
    if (slot > 2048) {
        HeapPageXL *e = &heap->xl[idx];
        if (ptr != e->page) {
            LOG_FATAL("heap_free: mid-allocation ptr {x} (XL base {x})", (u64)ptr, (u64)e->page);
            return 0;
        }
        return heap_free_xl_at(heap, idx);
    }
    if (slot <= 64)
        heap_free_s(heap, ptr, (u32)slot, idx);
    else if (slot <= 512)
        heap_free_m(heap, ptr, (u32)slot, idx);
    else
        heap_free_l(heap, ptr, (u32)slot);
    return slot;
}

i8 heap_allocator_resize(Allocator *self, void *ptr, size new_size) {
    heap_validate_self(self);
    HeapAllocator *heap = (HeapAllocator *)self;
    if (!ptr || new_size == 0)
        return 0;

    u32  idx;
    size cur = heap_recover_size(heap, ptr, &idx);
    if (cur == 0)
        return 0; // foreign ptr -- can't resize without knowing the slot

    if (cur > 2048) {
        // XL: page count must match for in-place.
        u32 op = heap->xl[idx].num_pages;
        u32 np = (u32)((new_size + HEAP_PAGE_SIZE - 1u) / HEAP_PAGE_SIZE);
        return op == np ? 1 : 0;
    }
    if (heap_alignment_demands_passthrough(self) || new_size > 2048) {
        return 0; // mixed-class transitions cannot be in-place
    }
    return (size)heap_slot_size_for(new_size) == cur ? 1 : 0;
}

void *heap_allocator_remap(Allocator *self, void *ptr, size new_size) {
    heap_validate_self(self);
    HeapAllocator *heap = (HeapAllocator *)self;

    if (new_size == 0) {
        if (ptr)
            heap_allocator_deallocate(self, ptr);
        return NULL;
    }
    if (!ptr)
        return heap_allocator_allocate(self, new_size, false);
    if (heap_allocator_resize(self, ptr, new_size))
        return ptr;

    // Need the old allocation size to bound the copy. Same lookup the
    // resize and free paths use; foreign ptr aborts in deallocate.
    u32  idx;
    size cur = heap_recover_size(heap, ptr, &idx);
    if (cur == 0) {
        LOG_FATAL("heap_remap: foreign ptr {x}", (u64)ptr);
        return NULL;
    }
    void *fresh = heap_allocator_allocate(self, new_size, false);
    if (!fresh)
        return NULL;
    MemCopy(fresh, ptr, cur < new_size ? cur : new_size);
    heap_allocator_deallocate(self, ptr);
    return fresh;
}

// =============================================================================
// Deinit. Unmap every user page and free every descriptor array.

// Each OS-page mmap produces HEAP_PAGES_PER_OS_PAGE descriptors but
// only one munmap. The descriptor whose page is OS-page-aligned owns
// the unmap; the rest share the mapping.
#define IS_GROUP_LEADER(page_addr) (((u64)(page_addr) & ((u64)HEAP_OS_PAGE_SIZE - 1u)) == 0u)

void HeapAllocatorDeinit(HeapAllocator *self) {
    if (!self)
        return;

    for (u32 i = 0; i < self->s_len; i++) {
        if (IS_GROUP_LEADER(self->s[i].page))
            AllocatorFree(&self->page.base, self->s[i].page);
    }
    if (self->s_cap)
        AllocatorFree(&self->page.base, self->s);

    for (u32 i = 0; i < self->m_len; i++) {
        if (IS_GROUP_LEADER(self->m[i].page))
            AllocatorFree(&self->page.base, self->m[i].page);
    }
    if (self->m_cap)
        AllocatorFree(&self->page.base, self->m);

    for (u32 i = 0; i < self->l_len; i++) {
        if (IS_GROUP_LEADER(self->l[i].page))
            AllocatorFree(&self->page.base, self->l[i].page);
    }
    if (self->l_cap)
        AllocatorFree(&self->page.base, self->l);

    for (u32 i = 0; i < self->xl_len; i++)
        AllocatorFree(&self->page.base, self->xl[i].page);
    if (self->xl_cap)
        AllocatorFree(&self->page.base, self->xl);

    MemSet(self, 0, sizeof(*self));
}
