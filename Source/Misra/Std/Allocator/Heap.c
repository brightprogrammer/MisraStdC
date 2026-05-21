/// file      : std/allocator/heap.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Bitmap-backed heap allocator. See Heap.h for the design notes.
///
/// Every slot has exactly two states: FREE (bit = 0) and IN_USE
/// (bit = 1). Alloc transitions FREE -> IN_USE; Free transitions
/// IN_USE -> FREE. Both transitions verify the precondition before
/// mutating the bitmap. EVERY failed precondition aborts via
/// LOG_FATAL with a backtrace -- a bad free is a caller-side
/// memory-safety bug, not a recoverable error condition, and
/// continuing past it would leave the program in undefined state.
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
// Class table. Indexed by class_idx (0..HEAP_NUM_CLASSES-1). Order matters:
// heap_class_idx_for() walks bins in ascending size, returning the first
// that fits the request.

static const u16 heap_class_size[HEAP_NUM_CLASSES] = {
    16u, 24u, 32u, 48u, 64u,                                          // S
    80u, 96u, 128u, 160u, 192u, 256u, 384u, 512u,                     // M
    768u, 1024u, 1536u, 2048u,                                        // L
};

// Slots per page per class = floor(HEAP_PAGE_SIZE / class_size). The
// per-class waste = HEAP_PAGE_SIZE - class_size * slots. Largest waste
// is the 1536 class (4096 / 1536 = 2 slots, 1024 B waste); within the
// S/M tier waste is <= 96 B per page.
static const u16 heap_class_slots[HEAP_NUM_CLASSES] = {
    256u, 170u, 128u, 85u, 64u,                                       // S
    51u, 42u, 32u, 25u, 21u, 16u, 10u, 8u,                            // M
    5u, 4u, 2u, 2u,                                                   // L
};

// ceil(slots / 64) for each class. Used to bound the bitmap word
// scan on alloc and to know how many words to pre-tail-mask on insert.
static const u8 heap_class_bm_words[HEAP_NUM_CLASSES] = {
    4u, 3u, 2u, 2u, 1u,                                               // S
    1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,                                   // M
    1u, 1u, 1u, 1u,                                                   // L
};

// Map a request size in bytes to a class index in [0, HEAP_NUM_CLASSES),
// or (u8)-1 for "this request is XL". Long if-ladder because of the
// non-uniform spacing between bins; gcc compiles this to a balanced
// decision tree at -O3.
static FORCE_INLINE u8 heap_class_idx_for(size n) {
    if (n <= 16u)   return 0u;
    if (n <= 24u)   return 1u;
    if (n <= 32u)   return 2u;
    if (n <= 48u)   return 3u;
    if (n <= 64u)   return 4u;
    if (n <= 80u)   return 5u;
    if (n <= 96u)   return 6u;
    if (n <= 128u)  return 7u;
    if (n <= 160u)  return 8u;
    if (n <= 192u)  return 9u;
    if (n <= 256u)  return 10u;
    if (n <= 384u)  return 11u;
    if (n <= 512u)  return 12u;
    if (n <= 768u)  return 13u;
    if (n <= 1024u) return 14u;
    if (n <= 1536u) return 15u;
    if (n <= 2048u) return 16u;
    return (u8)-1; // XL
}

// =============================================================================
// Self-validation.

// Fast path: magic on the allocator + magic on the embedded
// PageAllocator. Catches uninitialised / post-deinit / _Generic
// mismatch escape. Always on, FORCE_INLINEd into every caller.
static FORCE_INLINE void heap_validate_self_fast(const Allocator *self) {
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
    if (h->pages_len > h->pages_cap) {
        LOG_FATAL("HeapAllocator: pages_len {} exceeds pages_cap {}", (u64)h->pages_len, (u64)h->pages_cap);
    }
    if ((h->pages == NULL) != (h->pages_cap == 0)) {
        LOG_FATAL("HeapAllocator: pages / pages_cap mismatch ({x} / {})", (u64)h->pages, (u64)h->pages_cap);
    }
    if (h->xl_len > h->xl_cap) {
        LOG_FATAL("HeapAllocator: xl_len {} exceeds xl_cap {}", (u64)h->xl_len, (u64)h->xl_cap);
    }
    if ((h->xl == NULL) != (h->xl_cap == 0)) {
        LOG_FATAL("HeapAllocator: xl / xl_cap mismatch ({x} / {})", (u64)h->xl, (u64)h->xl_cap);
    }
    if (h->pages) {
        (void)(*(const volatile char *)(const void *)h->pages);
    }
    if (h->xl) {
        (void)(*(const volatile char *)(const void *)h->xl);
    }
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
// Descriptor-array helpers. Both `pages` (unified S/M/L) and `xl` are
// page-backed via the embedded PageAllocator, sorted by page address.
// The first field of every descriptor type is a `void *page`, the sort
// key.

#define HEAP_DESC_INITIAL_CAP 16u

static bool heap_grow_array(HeapAllocator *heap, void **arr_ptr, u32 *cap_ptr, u32 entry_size) {
    u32 old_cap = *cap_ptr;
    if (old_cap > ((u32)-1) / 2u) {
        return false;
    }
    u32  new_cap   = old_cap ? old_cap * 2u : HEAP_DESC_INITIAL_CAP;
    size new_bytes = (size)new_cap * (size)entry_size;
    void *fresh = AllocatorAlloc(&heap->page.base, new_bytes, false);
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
    u64   new_key = *(void **)entry == NULL ? 0 : (u64) * (void **)entry;
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

// Reclaim a now-empty heap page: hand the user page back to
// PageAllocator and remove the descriptor at `idx`. Only safe when
// HEAP_PAGES_PER_OS_PAGE == 1; on macOS-aarch64 the user page is one
// of several sub-pages in one mmap and the sibling-page grouping
// logic isn't in this branch.
//
// Guard: keep at least one descriptor of this class so the next alloc
// of the same class doesn't have to mmap a fresh page.
#if HEAP_PAGES_PER_OS_PAGE == 1u
static FORCE_INLINE void heap_reclaim_empty_page(HeapAllocator *heap, u32 idx) {
    HeapPage *d   = &heap->pages[idx];
    u8        cls = d->class_idx;
    if (heap->class_count[cls] <= 1u) {
        return; // keep one warm page per class
    }
    void *page = d->page;
    heap_remove_at(heap->pages, &heap->pages_len, idx, sizeof(HeapPage));
    heap->class_count[cls] -= 1u;
    AllocatorFree(&heap->page.base, page);
}
#endif

// =============================================================================
// Per-class page allocation / freeing.

// Mark the tail bits (slots >= class slot count) of `bm` as in-use so
// the alloc-side ctz on the inverted bitmap word never finds them.
// Words past `bm_words - 1` aren't touched (caller wrote 0 across
// them; we only need the partial last word).
static FORCE_INLINE void heap_set_tail_bits(u64 *bm, u32 slots, u8 bm_words) {
    u32 used_bits_in_last = slots & 63u;
    if (used_bits_in_last == 0u) {
        return; // slots is an exact multiple of 64; no tail.
    }
    u64 valid_mask = ((u64)1 << used_bits_in_last) - 1u;
    bm[bm_words - 1u] = ~valid_mask;
}

// Find a HeapPage descriptor for `cls` that has a free slot. Returns
// NULL if none. Linear scan over the unified array filtering by
// class_idx -- pages of the same class cluster in address order, so
// this is cache-friendly in practice.
static FORCE_INLINE HeapPage *heap_find_class_page_with_free(HeapAllocator *heap, u8 cls) {
    u8 bm_words = heap_class_bm_words[cls];
    for (u32 i = 0; i < heap->pages_len; i++) {
        HeapPage *d = &heap->pages[i];
        if (d->class_idx != cls) continue;
        // Quick reject: a full page has used_count == slots_per_class.
        if (d->used_count == heap_class_slots[cls]) continue;
        // Find first 0 bit; cheaper than scanning all words sequentially.
        for (u32 w = 0; w < bm_words; w++) {
            u64 inv = ~d->bitmap[w];
            if (inv == 0u) continue;
            // The class-specific tail bits are pre-set, so ctz on inv
            // is guaranteed to land in [0, slots) for this class.
            return d;
        }
    }
    return NULL;
}

// Bump the user-pointer for the first free slot in `d`, set its bit,
// increment used_count. Caller has already verified `d` has space.
static FORCE_INLINE void *heap_take_slot(HeapPage *d, u8 cls) {
    u8 bm_words = heap_class_bm_words[cls];
    for (u32 w = 0; w < bm_words; w++) {
        u64 inv = ~d->bitmap[w];
        if (inv == 0u) continue;
        u32 bit = CTZ64(inv);
        d->bitmap[w] |= ((u64)1 << bit);
        d->used_count += 1u;
        u32 slot_idx = w * 64u + bit;
        return (char *)d->page + (size)slot_idx * heap_class_size[cls];
    }
    // Unreachable: caller verified there's a free slot.
    LOG_FATAL("HeapAllocator: take_slot on a full page (class {}, page {x})", (u64)cls, (u64)d->page);
    return NULL;
}

// Grow: ask PageAllocator for one OS page, carve it into
// HEAP_PAGES_PER_OS_PAGE heap pages, register a descriptor per heap
// page (all assigned `cls`). Returns the index of the first new
// descriptor in pages[], or (u32)-1 on failure.
static u32 heap_grow_class(HeapAllocator *heap, u8 cls) {
    void *base = AllocatorAlloc(&heap->page.base, HEAP_OS_PAGE_SIZE, false);
    if (!base) return (u32)-1;

    u32 first_idx     = (u32)-1;
    u32 inserted      = 0;
    u16 slots         = heap_class_slots[cls];
    u8  bm_words      = heap_class_bm_words[cls];
    for (u32 i = 0; i < HEAP_PAGES_PER_OS_PAGE; i++) {
        void *page_i = (char *)base + (size)i * HEAP_PAGE_SIZE;
        HeapPage desc = {
            .page       = page_i,
            .bitmap     = {0, 0, 0, 0},
            .used_count = 0u,
            .class_idx  = cls,
            ._pad       = 0u,
        };
        heap_set_tail_bits(desc.bitmap, slots, bm_words);
        u32 idx = heap_insert_sorted(heap, (void **)&heap->pages, &heap->pages_len, &heap->pages_cap,
                                     sizeof(HeapPage), &desc);
        if (idx == (u32)-1) {
            // Roll back any descriptors already inserted from this base.
            for (u32 j = 0; j < inserted; j++) {
                void *p_j  = (char *)base + (size)j * HEAP_PAGE_SIZE;
                u32   ix_j = heap_find_by_page(heap->pages, heap->pages_len, sizeof(HeapPage), p_j);
                if (ix_j != (u32)-1) {
                    heap_remove_at(heap->pages, &heap->pages_len, ix_j, sizeof(HeapPage));
                }
            }
            AllocatorFree(&heap->page.base, base);
            return (u32)-1;
        }
        // first_idx tracks the LOW-address descriptor (page_i for i=0).
        if (i == 0u) first_idx = idx;
        // Earlier insertions may shift if this insert sorts ahead of
        // them. Recover first_idx by re-locating base after each step.
        // Simpler than tracking shifts manually.
        first_idx = heap_find_by_page(heap->pages, heap->pages_len, sizeof(HeapPage), base);
        inserted += 1u;
    }
    heap->class_count[cls] += HEAP_PAGES_PER_OS_PAGE;
    return first_idx;
}

// =============================================================================
// Class XL. One page-aligned region per allocation; descriptor existence
// is the in-use bit, no bitmap. Tracks the page count so free can pass
// the rounded size back to PageAllocator without re-querying.

static void *heap_alloc_xl(HeapAllocator *heap, size bytes, i8 zeroed) {
    size pages = (bytes + HEAP_PAGE_SIZE - 1u) / HEAP_PAGE_SIZE;
    if (pages > (size)(u32)-1) return NULL;
    size  total = pages * HEAP_PAGE_SIZE;
    void *ptr   = AllocatorAlloc(&heap->page.base, total, zeroed);
    if (!ptr) return NULL;
    HeapPageXL desc = {.page = ptr, .num_pages = (u32)pages};
    u32 idx = heap_insert_sorted(heap, (void **)&heap->xl, &heap->xl_len, &heap->xl_cap, sizeof(HeapPageXL), &desc);
    if (idx == (u32)-1) {
        AllocatorFree(&heap->page.base, ptr);
        return NULL;
    }
    return ptr;
}

static void heap_free_xl(HeapAllocator *heap, u32 idx) {
    void *ptr = heap->xl[idx].page;
    heap_remove_at(heap->xl, &heap->xl_len, idx, sizeof(HeapPageXL));
    AllocatorFree(&heap->page.base, ptr);
}

// =============================================================================
// Public alloc / free / resize / remap dispatch.

void *heap_allocator_allocate(Allocator *self, size bytes, i8 zeroed) {
    heap_validate_self(self);
    HeapAllocator *heap = (HeapAllocator *)self;
    if (!bytes) return NULL;
    if (heap_alignment_demands_passthrough(self) || bytes > 2048u) {
        return heap_alloc_xl(heap, bytes, zeroed);
    }

    u8 cls = heap_class_idx_for(bytes);
    // cls is in [0, HEAP_NUM_CLASSES) here because bytes <= 2048.

    HeapPage *d = heap_find_class_page_with_free(heap, cls);
    if (!d) {
        u32 idx = heap_grow_class(heap, cls);
        if (idx == (u32)-1) return NULL;
        d = &heap->pages[idx];
    }
    void *out = heap_take_slot(d, cls);
    if (out && zeroed) {
        MemSet(out, 0, heap_class_size[cls]);
    }
    return out;
}

// Look up the slot size for `ptr` and the descriptor index. For S/M/L
// pages the unified `pages` array gives both via a single bsearch by
// page base. For XL the per-page entry IS the descriptor. Returns 0
// if foreign (idx_out is left as (u32)-1).
//
// `is_xl_out` distinguishes which array `idx_out` indexes -- callers
// branch on it to choose the right free path.
static size heap_recover_size(HeapAllocator *heap, void *ptr, u32 *idx_out, bool *is_xl_out) {
    *idx_out  = (u32)-1;
    *is_xl_out = false;
    if (!ptr) return 0;

    void *page_base = (void *)((u64)ptr & ~(u64)(HEAP_PAGE_SIZE - 1u));

    // Try XL first: XL allocations are page-aligned but use the full
    // mapped region as the descriptor's `page` field. Match the
    // descriptor only when ptr equals its base exactly -- a
    // mid-allocation pointer (page_base of an interior heap page in
    // an XL region) must trip the foreign-ptr abort, not silently
    // fix up to the base.
    u32 i = heap_find_by_page(heap->xl, heap->xl_len, sizeof(HeapPageXL), page_base);
    if (i != (u32)-1 && ptr == heap->xl[i].page) {
        *idx_out  = i;
        *is_xl_out = true;
        return (size)heap->xl[i].num_pages * HEAP_PAGE_SIZE;
    }

    i = heap_find_by_page(heap->pages, heap->pages_len, sizeof(HeapPage), page_base);
    if (i != (u32)-1) {
        *idx_out = i;
        return (size)heap_class_size[heap->pages[i].class_idx];
    }

    return 0;
}

// Per-slot free path for S/M/L. Caller already resolved `idx` via
// heap_recover_size.
static void heap_free_classed(HeapAllocator *heap, void *ptr, u32 idx) {
    HeapPage *d         = &heap->pages[idx];
    u8        cls       = d->class_idx;
    u32       slot_size = heap_class_size[cls];
    void     *page_base = d->page;
    u64       off       = (u64)ptr - (u64)page_base;

    if (off >= (u64)slot_size * heap_class_slots[cls]) {
        LOG_FATAL("heap_free: ptr {x} past slot region (class size {})", (u64)ptr, (u64)slot_size);
        return;
    }
    if (off % slot_size != 0u) {
        LOG_FATAL("heap_free: misaligned ptr {x} for slot size {}", (u64)ptr, (u64)slot_size);
        return;
    }
    u32 slot_idx = (u32)(off / slot_size);
    u32 w        = slot_idx >> 6;
    u32 bit      = slot_idx & 63u;
    u64 mask     = (u64)1 << bit;
    if (!(d->bitmap[w] & mask)) {
        LOG_FATAL("heap_free: double-free of {x} (class size {}, slot {})", (u64)ptr, (u64)slot_size, (u64)slot_idx);
        return;
    }
    d->bitmap[w] &= ~mask;
    d->used_count -= 1u;

#if HEAP_PAGES_PER_OS_PAGE == 1u
    if (d->used_count == 0u) {
        heap_reclaim_empty_page(heap, idx);
    }
#endif
}

i8 heap_allocator_resize(Allocator *self, void *ptr, size new_size) {
    heap_validate_self(self);
    HeapAllocator *heap = (HeapAllocator *)self;
    u32   idx;
    bool  is_xl;
    size  cur = heap_recover_size(heap, ptr, &idx, &is_xl);
    if (!cur) return 0;
    if (is_xl) {
        u32 op = heap->xl[idx].num_pages;
        u32 np = (u32)((new_size + HEAP_PAGE_SIZE - 1u) / HEAP_PAGE_SIZE);
        return op == np ? 1 : 0;
    }
    if (heap_alignment_demands_passthrough(self) || new_size > 2048u) {
        return 0; // mixed-class transitions cannot be in-place
    }
    u8 new_cls = heap_class_idx_for(new_size);
    return heap_class_size[new_cls] == cur ? 1 : 0;
}

void *heap_allocator_remap(Allocator *self, void *ptr, size new_size) {
    heap_validate_self(self);
    HeapAllocator *heap = (HeapAllocator *)self;

    if (new_size == 0) {
        if (ptr) heap_allocator_deallocate(self, ptr);
        return NULL;
    }
    if (!ptr) return heap_allocator_allocate(self, new_size, true);

    // Resolve current size from the descriptor tables. We DON'T keep
    // the idx around: heap_allocator_allocate below may grow pages[],
    // and the sorted-insert MemMove invalidates every cached index.
    // Re-resolve via heap_allocator_deallocate which does its own
    // bsearch (one extra bsearch vs caching, in exchange for not
    // corrupting the wrong descriptor).
    u32  idx;
    bool is_xl;
    size cur = heap_recover_size(heap, ptr, &idx, &is_xl);
    if (!cur) {
        LOG_FATAL("heap_remap: foreign or already-freed ptr {x}", (u64)ptr);
        return NULL;
    }
    (void)idx;
    (void)is_xl;
    void *fresh = heap_allocator_allocate(self, new_size, false);
    if (!fresh) return NULL;
    size copy_bytes = cur < new_size ? cur : new_size;
    MemCopy(fresh, ptr, copy_bytes);
    (void)heap_allocator_deallocate(self, ptr);
    return fresh;
}

size heap_allocator_deallocate(Allocator *self, void *ptr) {
    heap_validate_self(self);
    HeapAllocator *heap = (HeapAllocator *)self;
    if (!ptr) return 0;

    u32  idx;
    bool is_xl;
    size cur = heap_recover_size(heap, ptr, &idx, &is_xl);
    if (!cur) {
        LOG_FATAL("heap_free: foreign or already-freed ptr {x}", (u64)ptr);
        return 0;
    }
    if (is_xl) {
        heap_free_xl(heap, idx);
    } else {
        heap_free_classed(heap, ptr, idx);
    }
    return cur;
}

void HeapAllocatorDeinit(HeapAllocator *self) {
    if (!self) return;
    // Release user pages first.
    for (u32 i = 0; i < self->pages_len; i++) {
        // On macOS-aarch64 multiple sibling descriptors share one mmap;
        // only call AllocatorFree for the LOW-address one in each group.
#if HEAP_PAGES_PER_OS_PAGE == 1u
        AllocatorFree(&self->page.base, self->pages[i].page);
#else
        if (((u64)self->pages[i].page % HEAP_OS_PAGE_SIZE) == 0u) {
            AllocatorFree(&self->page.base, self->pages[i].page);
        }
#endif
    }
    for (u32 i = 0; i < self->xl_len; i++) {
        AllocatorFree(&self->page.base, self->xl[i].page);
    }
    if (self->pages) AllocatorFree(&self->page.base, self->pages);
    if (self->xl)    AllocatorFree(&self->page.base, self->xl);
    PageAllocatorDeinit(&self->page);
    MemSet(self, 0, sizeof(*self));
}
