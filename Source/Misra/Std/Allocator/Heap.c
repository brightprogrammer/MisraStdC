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

static void heap_validate_self(const Allocator *self) {
    if (!self || self->__magic != HEAP_ALLOCATOR_MAGIC) {
        LOG_FATAL("type-confusion: allocator passed to heap_allocator_* is not a HeapAllocator");
    }
}

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
    u32   old_cap   = *cap_ptr;
    u32   new_cap   = old_cap ? old_cap * 2u : HEAP_DESC_INITIAL_CAP;
    size  new_bytes = (size)new_cap * (size)entry_size;
    void *fresh     = AllocatorAlloc(&heap->page.base, new_bytes, true);
    if (!fresh)
        return false;
    if (*arr_ptr && old_cap) {
        MemCopy(fresh, *arr_ptr, (size)old_cap * (size)entry_size);
        AllocatorFree(&heap->page.base, *arr_ptr, (size)old_cap * (size)entry_size);
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

static u32 ctz64(u64 x) {
    return (u32)__builtin_ctzll(x);
}
static u32 ctz32(u32 x) {
    return (u32)__builtin_ctz(x);
}

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
            AllocatorFree(&heap->page.base, base, HEAP_OS_PAGE_SIZE);
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

static void heap_free_s(HeapAllocator *heap, void *ptr, u32 slot_size) {
    void *page_base = (void *)((u64)ptr & ~(u64)(HEAP_PAGE_SIZE - 1u));
    u32   idx       = heap_find_by_page(heap->s, heap->s_len, sizeof(HeapPageS), page_base);
    if (idx == (u32)-1) {
        LOG_FATAL("heap_free: ptr {x} not in class-S (foreign or wrong size hint)", (u64)ptr);
        return;
    }
    HeapPageS *d   = &heap->s[idx];
    u64        off = (u64)ptr - (u64)page_base;

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
            AllocatorFree(&heap->page.base, base, HEAP_OS_PAGE_SIZE);
            return NULL;
        }
        if (i == 0)
            first_idx = idx;
    }
    heap->m[first_idx].bitmap |= (u16)((u32)1 << region_shift);
    return (char *)base + region_off;
}

static void heap_free_m(HeapAllocator *heap, void *ptr, u32 slot_size) {
    void *page_base = (void *)((u64)ptr & ~(u64)(HEAP_PAGE_SIZE - 1u));
    u32   idx       = heap_find_by_page(heap->m, heap->m_len, sizeof(HeapPageM), page_base);
    if (idx == (u32)-1) {
        LOG_FATAL("heap_free: ptr {x} not in class-M (foreign or wrong size hint)", (u64)ptr);
        return;
    }
    HeapPageM *d   = &heap->m[idx];
    u64        off = (u64)ptr - (u64)page_base;

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
            AllocatorFree(&heap->page.base, base, HEAP_OS_PAGE_SIZE);
            return NULL;
        }
        if (i == 0)
            first_idx = idx;
    }
    heap->l[first_idx].bitmap |= (u8)((u32)1 << region_shift);
    return (char *)base + region_off;
}

static void heap_free_l(HeapAllocator *heap, void *ptr, u32 slot_size) {
    void *page_base = (void *)((u64)ptr & ~(u64)(HEAP_PAGE_SIZE - 1u));
    u32   idx       = heap_find_by_page(heap->l, heap->l_len, sizeof(HeapPageL), page_base);
    if (idx == (u32)-1) {
        LOG_FATAL("heap_free: ptr {x} not in class-L (foreign or wrong size hint)", (u64)ptr);
        return;
    }
    HeapPageL *d   = &heap->l[idx];
    u64        off = (u64)ptr - (u64)page_base;

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
    u32   num_pages = (u32)((bytes + HEAP_PAGE_SIZE - 1u) / HEAP_PAGE_SIZE);
    size  full      = (size)num_pages * HEAP_PAGE_SIZE;
    void *page      = AllocatorAlloc(&heap->page.base, full, zeroed);
    if (!page)
        return NULL;
    HeapPageXL desc = {.page = page, .num_pages = num_pages};
    if (heap_insert_sorted(heap, (void **)&heap->xl, &heap->xl_len, &heap->xl_cap, sizeof(HeapPageXL), &desc) ==
        (u32)-1) {
        AllocatorFree(&heap->page.base, page, full);
        return NULL;
    }
    return page;
}

static void heap_free_xl(HeapAllocator *heap, void *ptr) {
    // Mask ptr down to its containing page so a mid-allocation pointer
    // inside the first page of an XL region still finds the descriptor,
    // and we can emit the right "mid-allocation" diagnostic rather than
    // misreporting it as foreign. A pointer in the 2nd+ page of a
    // multi-page XL allocation still misses (only the base page is
    // indexed) and is reported as foreign -- caller bug either way.
    void *page_base = (void *)((u64)ptr & ~(u64)(HEAP_PAGE_SIZE - 1u));
    u32   idx       = heap_find_by_page(heap->xl, heap->xl_len, sizeof(HeapPageXL), page_base);
    if (idx == (u32)-1) {
        LOG_FATAL("heap_free: foreign ptr {x} routed as XL", (u64)ptr);
        return;
    }
    HeapPageXL *e = &heap->xl[idx];
    if (ptr != e->page) {
        LOG_FATAL("heap_free: mid-allocation ptr {x} (XL base {x})", (u64)ptr, (u64)e->page);
        return;
    }
    AllocatorFree(&heap->page.base, e->page, (size)e->num_pages * HEAP_PAGE_SIZE);
    heap_remove_at(heap->xl, &heap->xl_len, idx, sizeof(HeapPageXL));
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

void heap_allocator_deallocate(Allocator *self, void *ptr, size bytes) {
    heap_validate_self(self);
    HeapAllocator *heap = (HeapAllocator *)self;
    if (!ptr)
        return;
    if (heap_alignment_demands_passthrough(self) || bytes > 2048) {
        heap_free_xl(heap, ptr);
        return;
    }

    u32 slot = heap_slot_size_for(bytes);
    if (slot <= 64)
        heap_free_s(heap, ptr, slot);
    else if (slot <= 512)
        heap_free_m(heap, ptr, slot);
    else
        heap_free_l(heap, ptr, slot);
}

i8 heap_allocator_resize(Allocator *self, void *ptr, size old_size, size new_size) {
    heap_validate_self(self);
    (void)ptr;
    if (heap_alignment_demands_passthrough(self) || old_size > 2048 || new_size > 2048) {
        if (old_size > 2048 && new_size > 2048) {
            u32 op = (u32)((old_size + HEAP_PAGE_SIZE - 1u) / HEAP_PAGE_SIZE);
            u32 np = (u32)((new_size + HEAP_PAGE_SIZE - 1u) / HEAP_PAGE_SIZE);
            return op == np ? 1 : 0;
        }
        return 0;
    }
    return heap_slot_size_for(old_size) == heap_slot_size_for(new_size) ? 1 : 0;
}

void *heap_allocator_remap(Allocator *self, void *ptr, size old_size, size new_size) {
    heap_validate_self(self);
    if (new_size == 0) {
        if (ptr)
            heap_allocator_deallocate(self, ptr, old_size);
        return NULL;
    }
    if (!ptr)
        return heap_allocator_allocate(self, new_size, false);
    if (heap_allocator_resize(self, ptr, old_size, new_size))
        return ptr;
    void *fresh = heap_allocator_allocate(self, new_size, false);
    if (!fresh)
        return NULL;
    MemCopy(fresh, ptr, old_size < new_size ? old_size : new_size);
    heap_allocator_deallocate(self, ptr, old_size);
    return fresh;
}

// =============================================================================
// Deinit. Unmap every user page and free every descriptor array.

// On macOS aarch64 each OS-page mmap produces HEAP_PAGES_PER_OS_PAGE
// descriptors, but only one munmap is owed per mmap. Only the
// descriptor whose page address is OS-page-aligned (the "group leader")
// fires the munmap; the others share the mapping and skip the free.
// On 4 KiB-page systems HEAP_PAGES_PER_OS_PAGE == 1 and every
// descriptor is its own group of one -- this loop reduces to the
// simple "one munmap per descriptor" case.
#define IS_GROUP_LEADER(page_addr) (((u64)(page_addr) & ((u64)HEAP_OS_PAGE_SIZE - 1u)) == 0u)

void HeapAllocatorDeinit(HeapAllocator *self) {
    if (!self)
        return;

    for (u32 i = 0; i < self->s_len; i++) {
        if (IS_GROUP_LEADER(self->s[i].page))
            AllocatorFree(&self->page.base, self->s[i].page, HEAP_OS_PAGE_SIZE);
    }
    if (self->s_cap)
        AllocatorFree(&self->page.base, self->s, (size)self->s_cap * sizeof(HeapPageS));

    for (u32 i = 0; i < self->m_len; i++) {
        if (IS_GROUP_LEADER(self->m[i].page))
            AllocatorFree(&self->page.base, self->m[i].page, HEAP_OS_PAGE_SIZE);
    }
    if (self->m_cap)
        AllocatorFree(&self->page.base, self->m, (size)self->m_cap * sizeof(HeapPageM));

    for (u32 i = 0; i < self->l_len; i++) {
        if (IS_GROUP_LEADER(self->l[i].page))
            AllocatorFree(&self->page.base, self->l[i].page, HEAP_OS_PAGE_SIZE);
    }
    if (self->l_cap)
        AllocatorFree(&self->page.base, self->l, (size)self->l_cap * sizeof(HeapPageL));

    for (u32 i = 0; i < self->xl_len; i++)
        AllocatorFree(&self->page.base, self->xl[i].page, (size)self->xl[i].num_pages * HEAP_PAGE_SIZE);
    if (self->xl_cap)
        AllocatorFree(&self->page.base, self->xl, (size)self->xl_cap * sizeof(HeapPageXL));

    MemSet(self, 0, sizeof(*self));
}
