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
#include <Misra/Std/Log.h>
#include <Misra/Std/Memory.h>

#include "_Os.h"

// HEAP_PAGES_PER_OS_PAGE is in Heap.h. The OS page size derived from
// it is the unit we ask the kernel for on each grow.
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
//
// Two-tier check on every API entry. The fast tier is unconditional;
// the deep tier is cached via a one-bit dirty marker stashed in the
// MSB of `__magic` (see HEAP_MAGIC_VALIDATED_BIT in Heap.h).
//
//   bit clear -> deep-check verified since last structural mutation
//   bit set   -> deep checks must re-run
//
// Only mutations that could break the deep-check invariants (page
// array growth, sorted insert/remove, reclaim) set the bit. Per-slot
// ops (bitmap flips, used_count++/--) leave it alone -- those touch
// fields the deep check doesn't inspect, so they're guaranteed-safe
// to skip the re-validation.

// Cast through void* to drop const for the cache-bit write. The
// mutation is observably a no-op (caches a freshly-verified
// validation result); the underlying storage is the allocator's
// own __magic field which it owns.
#define HEAP_MARK_DIRTY(h) \
    ((HeapAllocator *)(h))->base.__magic |= HEAP_MAGIC_VALIDATED_BIT

// Fast path: NULL check + magic check on the allocator. Catches
// uninitialised / post-deinit / _Generic mismatch escape. Always on,
// FORCE_INLINEd into every caller. Masks the dirty bit before comparing
// magic so neither cached state fails type-confusion detection.
static FORCE_INLINE void heap_validate_self_fast(const HeapAllocator *h) {
    if (!h) {
        LOG_FATAL("HeapAllocator: NULL self");
    }
    if ((h->base.__magic & ~HEAP_MAGIC_VALIDATED_BIT) != HEAP_ALLOCATOR_MAGIC) {
        LOG_FATAL("type-confusion: allocator passed to heap_allocator_* is not a HeapAllocator");
    }
}

#if FEATURE_HEAP_VALIDATE_FULL
// Split: _inner does the actual deep checks. The dispatch wrapper
// below decides whether to call it based on the cache bit.
static void heap_validate_self_full_inner(const HeapAllocator *h) {
    if (!h->base.allocate || !h->base.resize || !h->base.remap || !h->base.deallocate) {
        LOG_FATAL("HeapAllocator: vtable function pointer is NULL");
    }
    if (h->base.alignment == 0 || (h->base.alignment & (h->base.alignment - 1)) != 0) {
        LOG_FATAL("HeapAllocator: alignment {} is not a positive power of two", (u64)h->base.alignment);
    }
    if (h->pages_count > h->pages_cap) {
        LOG_FATAL("HeapAllocator: pages_count {} exceeds pages_cap {}", (u64)h->pages_count, (u64)h->pages_cap);
    }
    if ((h->pages == NULL) != (h->pages_cap == 0)) {
        LOG_FATAL("HeapAllocator: pages / pages_cap mismatch ({x} / {})", (u64)h->pages, (u64)h->pages_cap);
    }
    if (h->pages_cap != 0 && (h->pages_cap & (h->pages_cap - 1)) != 0) {
        LOG_FATAL("HeapAllocator: pages_cap {} is not a power of two", (u64)h->pages_cap);
    }
    if (h->xl_len > h->xl_cap) {
        LOG_FATAL("HeapAllocator: xl_len {} exceeds xl_cap {}", (u64)h->xl_len, (u64)h->xl_cap);
    }
    if ((h->xl == NULL) != (h->xl_cap == 0)) {
        LOG_FATAL("HeapAllocator: xl / xl_cap mismatch ({x} / {})", (u64)h->xl, (u64)h->xl_cap);
    }
    if (h->pages) {
        (void)(*(const volatile u8 *)(const void *)h->pages);
    }
    if (h->xl) {
        (void)(*(const volatile u8 *)(const void *)h->xl);
    }
}

static FORCE_INLINE void heap_validate_self_full(const HeapAllocator *h) {
    heap_validate_self_fast(h);
    if (!(h->base.__magic & HEAP_MAGIC_VALIDATED_BIT)) {
        return;  // cache hit: invariants verified since last structural mutation
    }
    heap_validate_self_full_inner(h);
    // Mark clean. Cast through void* to write through const view.
    ((HeapAllocator *)(void *)h)->base.__magic &= ~HEAP_MAGIC_VALIDATED_BIT;
}
#endif

#if FEATURE_HEAP_VALIDATE_FULL
#    define heap_validate_self(self) heap_validate_self_full(self)
#else
#    define heap_validate_self(self) heap_validate_self_fast(self)
#endif

// Smallest bin slot is 16-byte aligned by construction. Stronger
// alignment demands bypass bins and route to the XL list (page-aligned).
static bool heap_alignment_demands_passthrough(const HeapAllocator *h) {
    return h->base.alignment > 16;
}

// =============================================================================
// Descriptor-storage helpers. The `pages` table is a hash table over
// S/M/L user pages keyed by `page`; the `xl` table is a sorted-by-page
// array for the rare XL path. Both back their storage with direct
// os_page_map mmaps -- no intermediate allocator. The first field of
// every descriptor type is a `void *page` (the lookup key for pages,
// the sort key for xl).

#define HEAP_DESC_INITIAL_CAP 16u

// Grows the XL descriptor array (used for HeapPageXL only -- the
// classed-pages table has its own hash-table grow path). The mmap
// size is rounded up to a whole OS page, so the unmap on later
// grow / deinit must round the prior cap*entry_size up the same way
// -- a bare `old_cap * entry_size` would leave the trailing page
// bytes mapped.
static bool heap_grow_array(HeapAllocator *heap, void **arr_ptr, u32 *cap_ptr, u32 entry_size) {
    u32 old_cap = *cap_ptr;
    if (old_cap > ((u32)-1) / 2u) {
        return false;
    }
    u32  want_cap   = old_cap ? old_cap * 2u : HEAP_DESC_INITIAL_CAP;
    size want_bytes = (size)want_cap * (size)entry_size;
    size new_bytes  = os_page_round_up(want_bytes);
    u32  new_cap    = (u32)(new_bytes / entry_size);
    void *fresh = os_page_map(new_bytes);
    if (!fresh)
        return false;
    if (*arr_ptr && old_cap) {
        MemCopy(fresh, *arr_ptr, (size)old_cap * (size)entry_size);
        os_page_unmap(*arr_ptr, os_page_round_up((size)old_cap * (size)entry_size));
    }
    *arr_ptr = fresh;
    *cap_ptr = new_cap;
    HEAP_MARK_DIRTY(heap);  // xl pointer + cap changed
    return true;
}

static u32
    heap_insert_sorted(HeapAllocator *heap, void **arr_ptr, u32 *len_ptr, u32 *cap_ptr, u32 entry_size, void *entry) {
    if (*len_ptr == *cap_ptr && !heap_grow_array(heap, arr_ptr, cap_ptr, entry_size)) {
        return (u32)-1;
    }
    u8   *base    = (u8 *)*arr_ptr;
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
    HEAP_MARK_DIRTY(heap);  // xl_len changed
    return idx;
}

// Threaded `heap` through so we can flip the dirty bit on the cache.
static void heap_remove_at(HeapAllocator *heap, void *arr, u32 *len_ptr, u32 idx, u32 entry_size) {
    u8 *base = (u8 *)arr;
    if (idx + 1 < *len_ptr) {
        MemMove(base + idx * entry_size, base + (idx + 1) * entry_size, (size)(*len_ptr - idx - 1) * entry_size);
    }
    *len_ptr -= 1;
    HEAP_MARK_DIRTY(heap);  // xl_len changed
}

static u32 heap_find_by_page(const void *arr, u32 len, u32 entry_size, void *page_addr) {
    const u8 *base = (const u8 *)arr;
    u64       key  = (u64)page_addr;
    u32       lo   = 0;
    u32       hi   = len;
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

// =============================================================================
// pages[]: open-addressed hash table on (page_base -> HeapPage descriptor).
//
// Linear probing, capacity is a power of two (mask = cap - 1), empty
// bucket sentinel is page == NULL. Deletes use back-shift so there are
// no tombstones; consequently `heap_hash_lookup` can stop the moment it
// sees an empty bucket.
//
// Sized to keep load factor below 50% (grow when count*2 > cap). At
// that load, expected probes-on-hit is around 1.5 -- significantly
// cheaper than the bsearch's log2(N) iterations on the old sorted-array
// path, and insert/remove are O(1) bucket writes instead of MemMove
// tail shifts.
// =============================================================================

#define HEAP_PAGES_INITIAL_CAP 16u // power of two, must be >= 1

static FORCE_INLINE u32 heap_hash_bucket(u64 page_base, u32 mask) {
    // Page bases are 4 KiB-aligned u64s. Multiply by the 64-bit golden
    // ratio to spread the high address bits, take the top half, mask.
    u64 h = (page_base >> 12) * 0x9E3779B97F4A7C15ULL;
    return (u32)(h >> 32) & mask;
}

static FORCE_INLINE u32 heap_hash_lookup(const HeapPage *pages, u32 mask, void *page_base) {
    if (!pages) return HEAP_BUCKET_NONE;
    u32 idx = heap_hash_bucket((u64)page_base, mask);
    while (pages[idx].page) {
        if (pages[idx].page == page_base) return idx;
        idx = (idx + 1u) & mask;
    }
    return HEAP_BUCKET_NONE;
}

// Insert into a known-not-full table. Caller verified that `desc->page`
// is not already present. Does NOT touch the warm list; caller wires.
static FORCE_INLINE u32 heap_hash_insert_into(HeapPage *pages, u32 mask, const HeapPage *desc) {
    u32 idx = heap_hash_bucket((u64)desc->page, mask);
    while (pages[idx].page) {
        idx = (idx + 1u) & mask;
    }
    pages[idx] = *desc;
    return idx;
}

// Link `idx` at the head of `cls`'s warm list. Caller ensures the page
// isn't already linked.
static FORCE_INLINE void heap_warm_push(HeapAllocator *heap, u32 idx, u8 cls) {
    HeapPage *pages = heap->pages;
    u32       head  = heap->class_warm_head[cls];
    pages[idx].prev_warm = HEAP_BUCKET_NONE;
    pages[idx].next_warm = head;
    if (head != HEAP_BUCKET_NONE) {
        pages[head].prev_warm = idx;
    }
    heap->class_warm_head[cls] = idx;
}

// Unlink `idx` from `cls`'s warm list.
static FORCE_INLINE void heap_warm_unlink(HeapAllocator *heap, u32 idx, u8 cls) {
    HeapPage *pages = heap->pages;
    u32       prev  = pages[idx].prev_warm;
    u32       next  = pages[idx].next_warm;
    if (prev != HEAP_BUCKET_NONE) {
        pages[prev].next_warm = next;
    } else {
        heap->class_warm_head[cls] = next;
    }
    if (next != HEAP_BUCKET_NONE) {
        pages[next].prev_warm = prev;
    }
    pages[idx].prev_warm = HEAP_BUCKET_NONE;
    pages[idx].next_warm = HEAP_BUCKET_NONE;
}

// Knuth Algorithm R back-shift delete. Clears bucket `idx` and walks
// forward, sliding any entry whose natural bucket falls outside the
// (idx, cursor] window into the gap. Fixes warm-list pointers that
// reference moved buckets.
static void heap_hash_remove(HeapAllocator *heap, u32 idx) {
    HeapPage *pages = heap->pages;
    u32       mask  = heap->pages_cap - 1u;
    pages[idx].page = NULL;
    heap->pages_count -= 1u;
    u32 cursor = (idx + 1u) & mask;
    while (pages[cursor].page) {
        u32  natural = heap_hash_bucket((u64)pages[cursor].page, mask);
        bool can_move;
        if (cursor >= idx) {
            can_move = !(natural > idx && natural <= cursor);
        } else {
            // cursor wrapped past 0; idx is in the high half.
            can_move = !(natural > idx || natural <= cursor);
        }
        if (can_move) {
            pages[idx]         = pages[cursor];
            pages[cursor].page = NULL;
            HeapPage *moved    = &pages[idx];
            u8        cls      = moved->class_idx;
            if (moved->prev_warm != HEAP_BUCKET_NONE) {
                pages[moved->prev_warm].next_warm = idx;
            } else if (heap->class_warm_head[cls] == cursor) {
                heap->class_warm_head[cls] = idx;
            }
            if (moved->next_warm != HEAP_BUCKET_NONE) {
                pages[moved->next_warm].prev_warm = idx;
            }
            idx = cursor;
        }
        cursor = (cursor + 1u) & mask;
    }
}

// Resize (or initial allocate) to `new_cap` (power of two). Re-inserts
// every live entry and rebuilds the per-class warm lists from scratch
// since bucket indices change.
static bool heap_hash_resize(HeapAllocator *heap, u32 new_cap) {
    // Mapped bytes are rounded up to a whole OS page. `new_cap` is a
    // power of two and HeapPage is 64 bytes, so for OS page sizes 4 KiB
    // and 16 KiB the rounded length is uniquely determined by `new_cap`
    // -- no separate stored field is needed for the matching unmap on
    // a later grow.
    size      raw_bytes = (size)new_cap * sizeof(HeapPage);
    size      new_bytes = os_page_round_up(raw_bytes);
    HeapPage *fresh     = (HeapPage *)os_page_map(new_bytes);
    if (!fresh) return false;
    // The kernel zeros the mmap so HeapPage.page reads as NULL on every
    // bucket -- no MemSet needed.
    u32 new_mask = new_cap - 1u;

    // Rebuild warm heads from scratch as we reinsert.
    for (u32 c = 0; c < HEAP_NUM_CLASSES; c++) {
        heap->class_warm_head[c] = HEAP_BUCKET_NONE;
    }

    HeapPage *old_pages = heap->pages;
    u32       old_cap   = heap->pages_cap;
    for (u32 i = 0; i < old_cap; i++) {
        if (!old_pages[i].page) continue;
        HeapPage desc = old_pages[i];
        // Probe-insert into fresh.
        u32 ins = heap_hash_bucket((u64)desc.page, new_mask);
        while (fresh[ins].page) {
            ins = (ins + 1u) & new_mask;
        }
        u8 cls = desc.class_idx;
        if (desc.used_count < heap_class_slots[cls]) {
            // Has free slots -> belongs at warm-list head for its class.
            u32 head       = heap->class_warm_head[cls];
            desc.prev_warm = HEAP_BUCKET_NONE;
            desc.next_warm = head;
            fresh[ins]     = desc;
            if (head != HEAP_BUCKET_NONE) {
                fresh[head].prev_warm = ins;
            }
            heap->class_warm_head[cls] = ins;
        } else {
            // Full page -> not in any warm list.
            desc.prev_warm = HEAP_BUCKET_NONE;
            desc.next_warm = HEAP_BUCKET_NONE;
            fresh[ins]     = desc;
        }
    }

    if (old_pages) {
        size old_bytes = os_page_round_up((size)old_cap * sizeof(HeapPage));
        os_page_unmap(old_pages, old_bytes);
    }
    heap->pages     = fresh;
    heap->pages_cap = new_cap;
    HEAP_MARK_DIRTY(heap);
    return true;
}

// ===========================================================================
// recycle[]: per-allocator LIFO of reclaimed HEAP_OS_PAGE_SIZE mmaps.
//
// Replaces PageAllocator's retention pool for HeapAllocator's own
// consumption. Storage is itself mmap-backed: one OS page when first
// needed, doubled on overflow. heap_recycle_pop is called from the
// alloc-side fast path (heap_grow_class); heap_recycle_push is called
// from the free-side fast path (heap_reclaim_empty_page). Both O(1).
// ===========================================================================

// Grow the recycle storage if full. Returns false on OS mmap failure;
// the caller falls back to os_page_unmap of the reclaimed page.
static bool heap_recycle_reserve(HeapAllocator *heap) {
    if (heap->recycle_len < heap->recycle_cap) return true;
    u32  want_cap   = heap->recycle_cap ? heap->recycle_cap * 2u : 0u;
    size want_bytes = (size)want_cap * sizeof(void *);
    size new_bytes  = os_page_round_up(want_bytes ? want_bytes : sizeof(void *));
    u32  new_cap    = (u32)(new_bytes / sizeof(void *));
    void **fresh    = (void **)os_page_map(new_bytes);
    if (!fresh) return false;
    if (heap->recycle && heap->recycle_len) {
        MemCopy(fresh, heap->recycle, (size)heap->recycle_len * sizeof(void *));
    }
    if (heap->recycle) {
        os_page_unmap(heap->recycle, os_page_round_up((size)heap->recycle_cap * sizeof(void *)));
    }
    heap->recycle     = fresh;
    heap->recycle_cap = new_cap;
    HEAP_MARK_DIRTY(heap);
    return true;
}

// Try to park `page` (size HEAP_OS_PAGE_SIZE) in the recycle stack.
// Returns false on storage growth failure -- caller MUST then
// os_page_unmap to avoid leaking.
static FORCE_INLINE bool heap_recycle_push(HeapAllocator *heap, void *page) {
    if (!heap_recycle_reserve(heap)) return false;
    heap->recycle[heap->recycle_len++] = page;
    return true;
}

// Pop the most recently retained HEAP_OS_PAGE_SIZE region, or NULL if
// the stack is empty.
static FORCE_INLINE void *heap_recycle_pop(HeapAllocator *heap) {
    if (heap->recycle_len == 0u) return NULL;
    heap->recycle_len -= 1u;
    return heap->recycle[heap->recycle_len];
}

// Reclaim a now-empty heap page. Push it into the recycle pool so the
// next heap_grow_class for any class can reuse it without re-syscalling
// mmap; fall through to os_page_unmap if the pool's storage grow fails.
// Only safe when HEAP_PAGES_PER_OS_PAGE == 1; on macOS-aarch64 the user
// page is one of several sub-pages in one mmap and the sibling-page
// grouping logic isn't in this branch.
//
// Guard: keep at least one descriptor of this class so the next alloc
// of the same class still finds a warm page.
#if HEAP_PAGES_PER_OS_PAGE == 1u
static FORCE_INLINE void heap_reclaim_empty_page(HeapAllocator *heap, u32 idx) {
    HeapPage *d   = &heap->pages[idx];
    u8        cls = d->class_idx;
    if (heap->class_count[cls] <= 1u) {
        return; // keep one warm page per class
    }
    void *page = d->page;
    // An empty page (used_count == 0) is in its class's warm list. Unlink
    // before clearing the bucket so the back-shift fixups don't trip on a
    // stale link.
    heap_warm_unlink(heap, idx, cls);
    heap_hash_remove(heap, idx);
    heap->class_count[cls] -= 1u;
    if (!heap_recycle_push(heap, page)) {
        os_page_unmap(page, HEAP_OS_PAGE_SIZE);
    }
    HEAP_MARK_DIRTY(heap);
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

// Return the bucket index of a warm page for `cls`, or HEAP_BUCKET_NONE.
// O(1): warm-list head holds the most recently un-filled page for the
// class. The page is guaranteed to have at least one free slot because
// pages are unlinked from the warm list the moment they fill in
// heap_take_slot, and re-linked the moment they un-fill in
// heap_free_classed.
static FORCE_INLINE u32 heap_find_class_warm_idx(const HeapAllocator *heap, u8 cls) {
    return heap->class_warm_head[cls];
}

// Take the first free slot from the page at bucket `idx`. Bumps
// used_count and, if the page just filled, unlinks from the class warm
// list so the next alloc gets a different page.
static FORCE_INLINE void *heap_take_slot(HeapAllocator *heap, u32 idx, u8 cls) {
    HeapPage *d        = &heap->pages[idx];
    u8        bm_words = heap_class_bm_words[cls];
    for (u32 w = 0; w < bm_words; w++) {
        u64 inv = ~d->bitmap[w];
        if (inv == 0u) continue;
        u32 bit = CTZ64(inv);
        d->bitmap[w] |= ((u64)1 << bit);
        d->used_count += 1u;
        // Just filled this page -> unlink from warm list. Common case for
        // the LAST slot taken before the next page becomes the warm head.
        if (d->used_count == heap_class_slots[cls]) {
            heap_warm_unlink(heap, idx, cls);
        }
        u32 slot_idx = w * 64u + bit;
        return (u8 *)d->page + (size)slot_idx * heap_class_size[cls];
    }
    // Unreachable: caller verified there's a free slot.
    LOG_FATAL("HeapAllocator: take_slot on a full page (class {}, page {x})", (u64)cls, (u64)d->page);
    return NULL;
}

// Grow: map one OS page (or pop one from the recycle pool), carve it
// into HEAP_PAGES_PER_OS_PAGE heap pages, register a descriptor per
// heap page (all assigned `cls`) into the hash table and warm list.
// Returns the warm-list head bucket index, or HEAP_BUCKET_NONE on
// failure.
static u32 heap_grow_class(HeapAllocator *heap, u8 cls) {
    // Ensure the hash has room for the new entries at <50% load. Grow
    // (doubling, or initial alloc to HEAP_PAGES_INITIAL_CAP) before
    // requesting the OS page so a failure here doesn't strand an mmap.
    u32 need = heap->pages_count + HEAP_PAGES_PER_OS_PAGE;
    while (heap->pages_cap == 0u || need * 2u > heap->pages_cap) {
        u32 new_cap = heap->pages_cap ? heap->pages_cap * 2u : HEAP_PAGES_INITIAL_CAP;
        if (!heap_hash_resize(heap, new_cap)) {
            return HEAP_BUCKET_NONE;
        }
    }

    // Try the recycle pool before going to the kernel. Reclaimed pages
    // are the same HEAP_OS_PAGE_SIZE we'd ask the OS for; reusing them
    // skips a mmap/munmap syscall pair per alloc/free cycle.
    void *base = heap_recycle_pop(heap);
    if (!base) {
        base = os_page_map(HEAP_OS_PAGE_SIZE);
        if (!base) return HEAP_BUCKET_NONE;
    }
    // Recycled pages keep whatever bytes the previous user wrote into
    // them. We don't `MemSet` because the new descriptors carve the
    // page into slots whose contents are only inspected after the user
    // writes them. (The kernel-zeroed guarantee only applies to fresh
    // mmaps; honoring it here would defeat the recycling win.)

    u32 mask     = heap->pages_cap - 1u;
    u16 slots    = heap_class_slots[cls];
    u8  bm_words = heap_class_bm_words[cls];
    for (u32 i = 0; i < HEAP_PAGES_PER_OS_PAGE; i++) {
        void    *page_i = (u8 *)base + (size)i * HEAP_PAGE_SIZE;
        HeapPage desc   = {
              .page       = page_i,
              .bitmap     = {0, 0, 0, 0},
              .used_count = 0u,
              .class_idx  = cls,
              ._pad0      = 0u,
              .prev_warm  = HEAP_BUCKET_NONE,
              .next_warm  = HEAP_BUCKET_NONE,
        };
        heap_set_tail_bits(desc.bitmap, slots, bm_words);
        u32 idx = heap_hash_insert_into(heap->pages, mask, &desc);
        heap->pages_count += 1u;
        heap_warm_push(heap, idx, cls);
    }
    heap->class_count[cls] += HEAP_PAGES_PER_OS_PAGE;
    HEAP_MARK_DIRTY(heap);
    // Return the warm-list head, NOT the first inserted page. With
    // HEAP_PAGES_PER_OS_PAGE > 1 (macOS-aarch64) the LIFO push order
    // makes the last-inserted sibling the head, and heap_take_slot
    // operates on whichever page is at the head. Caller and the
    // warm-head walker MUST agree on the same page, otherwise free
    // and the next alloc land in different pages and the
    // "free-then-alloc-recycles" invariant breaks.
    return heap->class_warm_head[cls];
}

// =============================================================================
// Class XL. One OS-page-aligned region per allocation; descriptor
// existence is the in-use bit, no bitmap. Round the request up to a
// whole OS page and remember the page count so free can pass the exact
// mmap'd length to os_page_unmap.

static void *heap_alloc_xl(HeapAllocator *heap, size bytes, i8 zeroed) {
    size os_pages = (bytes + HEAP_OS_PAGE_SIZE - 1u) / HEAP_OS_PAGE_SIZE;
    if (os_pages > (size)(u32)-1) return NULL;
    size  total = os_pages * HEAP_OS_PAGE_SIZE;
    void *ptr   = os_page_map(total);
    if (!ptr) return NULL;
    // os_page_map returns kernel-zeroed memory so the `zeroed` flag is
    // already satisfied without an extra MemSet.
    (void)zeroed;
    HeapPageXL desc = {.page = ptr, .os_pages = (u32)os_pages};
    u32 idx = heap_insert_sorted(heap, (void **)&heap->xl, &heap->xl_len, &heap->xl_cap, sizeof(HeapPageXL), &desc);
    if (idx == (u32)-1) {
        os_page_unmap(ptr, total);
        return NULL;
    }
    return ptr;
}

static void heap_free_xl(HeapAllocator *heap, u32 idx) {
    void *ptr   = heap->xl[idx].page;
    size  total = (size)heap->xl[idx].os_pages * HEAP_OS_PAGE_SIZE;
    heap_remove_at(heap, heap->xl, &heap->xl_len, idx, sizeof(HeapPageXL));
    os_page_unmap(ptr, total);
}

// =============================================================================
// Public alloc / free / resize / remap dispatch.

void *heap_allocator_allocate(HeapAllocator *self, size bytes, i8 zeroed) {
    heap_validate_self(self);
    if (!bytes) return NULL;
    void *out;
    if (heap_alignment_demands_passthrough(self) || bytes > 2048u) {
        out = heap_alloc_xl(self, bytes, zeroed);
    } else {
        u8  cls = heap_class_idx_for(bytes);
        u32 idx = heap_find_class_warm_idx(self, cls);
        if (idx == HEAP_BUCKET_NONE) {
            idx = heap_grow_class(self, cls);
            if (idx == HEAP_BUCKET_NONE) {
#if FEATURE_ALLOC_STATS
                self->base.stats.failed_allocations += 1u;
#endif
                return NULL;
            }
        }
        out = heap_take_slot(self, idx, cls);
        if (out && zeroed) {
            MemSet(out, 0, heap_class_size[cls]);
        }
    }
#if FEATURE_ALLOC_STATS
    if (out) {
        self->base.stats.allocations     += 1u;
        self->base.stats.bytes_requested += (u64)bytes;
        self->base.stats.bytes_in_use    += (u64)bytes;
        if (self->base.stats.bytes_in_use > self->base.stats.peak_bytes_in_use) {
            self->base.stats.peak_bytes_in_use = self->base.stats.bytes_in_use;
        }
    } else {
        self->base.stats.failed_allocations += 1u;
    }
#endif
    return out;
}

// Look up the slot size for `ptr` and the descriptor index. XL still
// uses the sorted-array bsearch (rare path). S/M/L uses the hash table
// keyed on page base -- one probe on hit. Returns 0 on foreign ptr.
//
// `is_xl_out` distinguishes which structure `idx_out` indexes -- callers
// branch on it to choose the right free path.
static size heap_recover_size(HeapAllocator *heap, void *ptr, u32 *idx_out, bool *is_xl_out) {
    *idx_out   = HEAP_BUCKET_NONE;
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
        *idx_out   = i;
        *is_xl_out = true;
        return (size)heap->xl[i].os_pages * HEAP_OS_PAGE_SIZE;
    }

    u32 b = heap_hash_lookup(heap->pages, heap->pages_cap - 1u, page_base);
    if (b != HEAP_BUCKET_NONE) {
        *idx_out = b;
        return (size)heap_class_size[heap->pages[b].class_idx];
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
    u16 was = d->used_count;
    d->used_count = (u16)(was - 1u);

    // Just un-filled? Push back into the class warm list so the next
    // alloc sees it. The warm list is FIFO at the head, so this becomes
    // the next page handed out -- aggregating frees into the same warm
    // page minimises bucket churn on the alloc side.
    u16 slots = heap_class_slots[cls];
    if (was == slots) {
        heap_warm_push(heap, idx, cls);
    }

#if HEAP_PAGES_PER_OS_PAGE == 1u
    if (d->used_count == 0u) {
        heap_reclaim_empty_page(heap, idx);
    }
#endif
}

i8 heap_allocator_resize(HeapAllocator *self, void *ptr, size new_size) {
    heap_validate_self(self);
    u32  idx;
    bool is_xl;
    size cur = heap_recover_size(self, ptr, &idx, &is_xl);
    if (!cur) return 0;
    i8 ok;
    if (is_xl) {
        u32 op = self->xl[idx].os_pages;
        u32 np = (u32)((new_size + HEAP_OS_PAGE_SIZE - 1u) / HEAP_OS_PAGE_SIZE);
        ok     = (op == np) ? 1 : 0;
    } else if (heap_alignment_demands_passthrough(self) || new_size > 2048u) {
        ok = 0; // mixed-class transitions cannot be in-place
    } else {
        u8 new_cls = heap_class_idx_for(new_size);
        ok         = (heap_class_size[new_cls] == cur) ? 1 : 0;
    }
#if FEATURE_ALLOC_STATS
    if (ok) {
        self->base.stats.reallocations   += 1u;
        self->base.stats.bytes_requested += (u64)new_size;
        if (self->base.stats.bytes_in_use > self->base.stats.peak_bytes_in_use) {
            self->base.stats.peak_bytes_in_use = self->base.stats.bytes_in_use;
        }
    }
#endif
    return ok;
}

void *heap_allocator_remap(HeapAllocator *self, void *ptr, size new_size) {
    heap_validate_self(self);

    if (new_size == 0) {
        if (ptr) heap_allocator_deallocate(self, ptr);
        return NULL;
    }
    if (!ptr) return heap_allocator_allocate(self, new_size, true);

    u32  idx;
    bool is_xl;
    size cur = heap_recover_size(self, ptr, &idx, &is_xl);
    if (!cur) {
        LOG_FATAL("heap_remap: foreign or already-freed ptr {x}", (u64)ptr);
        return NULL;
    }

    // XL fast path: ask the kernel to resize the mapping in place. On
    // Linux this routes to `mremap(..., MREMAP_MAYMOVE)`, which can
    // grow a page-class region without touching the bytes -- the same
    // trick glibc / jemalloc use for their large-block realloc. The
    // kernel may move the region; if it does, the xl[] descriptor
    // gets re-inserted at the new ptr's sorted position. Darwin and
    // Windows don't expose this primitive (os_page_remap returns
    // NULL); those paths fall through to alloc-new + memcpy.
    void *result = NULL;
    if (is_xl) {
        size old_pages = (size)self->xl[idx].os_pages;
        size new_pages = (new_size + HEAP_OS_PAGE_SIZE - 1u) / HEAP_OS_PAGE_SIZE;
        if (new_pages == old_pages) {
            result = ptr;
            goto done;
        }
        if (new_pages > (size)(u32)-1) goto fallback;
        size  old_total = old_pages * HEAP_OS_PAGE_SIZE;
        size  new_total = new_pages * HEAP_OS_PAGE_SIZE;
        void *new_ptr   = os_page_remap(ptr, old_total, new_total);
        if (!new_ptr) goto fallback;
        if (new_ptr == ptr) {
            self->xl[idx].os_pages = (u32)new_pages;
            HEAP_MARK_DIRTY(self);
            result = ptr;
            goto done;
        }
        // Sorted array: position keyed on `.page`. The remapped region
        // sits at a new address, so we drop the old descriptor and
        // re-insert at the new position. `heap_insert_sorted` may
        // grow the array via heap_grow_array (which mmap-rounds the
        // backing storage); both transitions mark dirty.
        heap_remove_at(self, self->xl, &self->xl_len, idx, sizeof(HeapPageXL));
        HeapPageXL desc = {.page = new_ptr, .os_pages = (u32)new_pages};
        u32        ins  = heap_insert_sorted(
            self, (void **)&self->xl, &self->xl_len, &self->xl_cap, sizeof(HeapPageXL), &desc);
        if (ins == (u32)-1) {
            // xl[] capacity grow failed -- we already kept the kernel's
            // resized region, so unmap it and bail. Caller sees NULL.
            os_page_unmap(new_ptr, new_total);
            result = NULL;
            goto done;
        }
        result = new_ptr;
        goto done;
    }

fallback:
    // S/M/L binned allocations: the slot lives inside a multi-tenant
    // OS page; we can't ask the kernel to grow a single slot. Same
    // story for any path where os_page_remap declined. heap_allocator_allocate
    // bumps stats on its own; heap_allocator_deallocate bumps the free
    // counter. We skip the reallocation bump here -- the realloc is
    // realised as a discrete alloc + free pair below, and double-
    // counting it would mis-report bytes_in_use.
    {
        void *fresh = heap_allocator_allocate(self, new_size, false);
        if (!fresh) return NULL;
        size copy_bytes = cur < new_size ? cur : new_size;
        MemCopy(fresh, ptr, copy_bytes);
        (void)heap_allocator_deallocate(self, ptr);
        return fresh;
    }

done:
#if FEATURE_ALLOC_STATS
    if (result) {
        self->base.stats.reallocations   += 1u;
        self->base.stats.bytes_requested += (u64)new_size;
        if (self->base.stats.bytes_in_use > self->base.stats.peak_bytes_in_use) {
            self->base.stats.peak_bytes_in_use = self->base.stats.bytes_in_use;
        }
    } else {
        self->base.stats.failed_allocations += 1u;
    }
#endif
    return result;
}

size heap_allocator_deallocate(HeapAllocator *self, void *ptr) {
    heap_validate_self(self);
    if (!ptr) return 0;

    u32  idx;
    bool is_xl;
    size cur = heap_recover_size(self, ptr, &idx, &is_xl);
    if (!cur) {
        LOG_FATAL("heap_free: foreign or already-freed ptr {x}", (u64)ptr);
        return 0;
    }
    if (is_xl) {
        heap_free_xl(self, idx);
    } else {
        heap_free_classed(self, ptr, idx);
    }
#if FEATURE_ALLOC_STATS
    self->base.stats.deallocations += 1u;
    if ((u64)cur <= self->base.stats.bytes_in_use) {
        self->base.stats.bytes_in_use -= (u64)cur;
    } else {
        self->base.stats.bytes_in_use = 0u;
    }
#endif
    return cur;
}

void HeapAllocatorDeinit(HeapAllocator *self) {
    if (!self) return;
    // Release user pages first. The hash table is sparse; walk every
    // bucket and act only on occupied ones (page != NULL).
    if (self->pages) {
        for (u32 i = 0; i < self->pages_cap; i++) {
            void *p = self->pages[i].page;
            if (!p) continue;
            // On macOS-aarch64 multiple sibling descriptors share one
            // mmap; only release the LOW-address one in each group.
#if HEAP_PAGES_PER_OS_PAGE == 1u
            os_page_unmap(p, HEAP_OS_PAGE_SIZE);
#else
            if (((u64)p % HEAP_OS_PAGE_SIZE) == 0u) {
                os_page_unmap(p, HEAP_OS_PAGE_SIZE);
            }
#endif
        }
    }
    // XL allocations were mmap'd at OS-page granularity; pass the
    // recorded count back.
    for (u32 i = 0; i < self->xl_len; i++) {
        os_page_unmap(self->xl[i].page, (size)self->xl[i].os_pages * HEAP_OS_PAGE_SIZE);
    }
    // Drain the recycle pool itself.
    for (u32 i = 0; i < self->recycle_len; i++) {
        os_page_unmap(self->recycle[i], HEAP_OS_PAGE_SIZE);
    }
    // Release the bookkeeping arrays' own mmaps. Each was created via
    // heap_grow_array / heap_hash_resize / heap_recycle_reserve with a
    // page-rounded byte count; the same rounding recovers the unmap
    // size deterministically.
    if (self->pages)   os_page_unmap(self->pages,   os_page_round_up((size)self->pages_cap   * sizeof(HeapPage)));
    if (self->xl)      os_page_unmap(self->xl,      os_page_round_up((size)self->xl_cap      * sizeof(HeapPageXL)));
    if (self->recycle) os_page_unmap(self->recycle, os_page_round_up((size)self->recycle_cap * sizeof(void *)));
    MemSet(self, 0, sizeof(*self));
}
