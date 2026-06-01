/// file      : std/allocator/heap.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Bitmap-backed heap allocator. Every user page contains ONLY user
/// data -- no header, no `next` pointer, no magic. Per-page metadata
/// (the bitmap of which slots are in use, and the page address) lives
/// in allocator-owned descriptor structures backed by direct OS page
/// mappings; HeapAllocator does not chain through any other in-tree
/// allocator.
///
/// Sizes route into one of 8 size classes plus an XL passthrough.
/// One slot size per user page (no co-located sub-bins): a page that
/// holds 128-byte slots only holds 128-byte slots, never mixed with
/// 256-byte slots on the same page. Every class is a power of two
/// that divides `HEAP_PAGE_SIZE` cleanly, so there is no per-page
/// alignment waste -- the bitmap covers the page exactly.
///
/// Class set (bytes):
///
///   16, 32, 64, 128, 256, 512, 1024, 2048
///   XL -- anything larger (one page-aligned mmap per allocation,
///         retained in `xl_freed[]` until the footprint-shrink
///         policy returns it to the kernel)
///
/// HEAP_PAGE_SIZE is fixed at 4096. macOS-aarch64 has 16 KiB OS pages
/// and asks the kernel for one OS page per grow; that mmap is then
/// carved into HEAP_PAGES_PER_OS_PAGE heap pages, each with its own
/// descriptor. The reclaim-when-empty path skips macOS-aarch64 for
/// now (a heap page can't be returned without its mmap siblings).
///
/// Every per-page descriptor stores: page base, class index, in-use
/// slot count, a 256-bit bitmap (the widest any class needs), and
/// warm-list links. The "S/M/L" tier names are a documentation
/// convenience; internally every binned descriptor lives in an
/// open-addressed hash table keyed by user-page base, so free is one
/// hash probe regardless of class. XL descriptors live in two
/// parallel flat arrays (`xl_in_use[]` for live regions, `xl_freed[]`
/// for retained mappings); alloc looks for a matching `os_pages`
/// entry in `xl_freed[]` (LIFO scan) before falling back to a fresh
/// mmap, and free is a swap-remove from `xl_in_use[]` + swap-push
/// onto `xl_freed[]`. Linear scan on free is O(N_live), faster than
/// the prior hash for the typical < 100 live XL regions because each
/// probe is a contiguous read.

#ifndef MISRA_STD_ALLOCATOR_HEAP_H
#define MISRA_STD_ALLOCATOR_HEAP_H

#include <Misra/Std/Allocator.h>

#define HEAP_PAGE_SIZE 4096u

// Most platforms have HEAP_PAGE_SIZE-sized OS pages. macOS aarch64
// has 16 KiB OS pages -- each mmap grow allocates one OS page and
// carves it into HEAP_PAGES_PER_OS_PAGE heap pages, registering one
// descriptor per heap page.
#if PLATFORM_DARWIN && ARCHITECTURE_AARCH64
#    define HEAP_PAGES_PER_OS_PAGE 4u
#else
#    define HEAP_PAGES_PER_OS_PAGE 1u
#endif

#define HEAP_ALLOCATOR_MAGIC MAKE_NEW_MAGIC_VALUE("heapallc")

// Structural-mutation sites set MAGIC_VALIDATED_BIT (see Misra/Types.h)
// so the next `heap_validate_self` call recomputes its cross-class
// invariants. For HeapAllocator those sites are pages hash table grow
// / rebuild, xl_in_use / xl_freed array grow, and recycle-pool grow.
// Per-slot ops (bitmap flips, used_count++/--, warm-list linkage,
// single-bucket insert/remove, XL array swap-remove / swap-push)
// leave the bit alone -- they don't touch fields the deep body
// inspects, so the memoized result stays valid.

// Number of binned size classes. Indexed 0..HEAP_NUM_CLASSES-1.
// XL is "class HEAP_NUM_CLASSES" by convention -- it has its own
// descriptor type and its own arrays.
#define HEAP_NUM_CLASSES 8u

// Max slots in any single class's page. The 16-byte class fits 256
// slots in 4 KiB; bitmaps are sized to cover this. Every class's
// slot count is a power of two that divides HEAP_PAGE_SIZE exactly,
// so the page itself has no wasted bytes. The bitmap word the slot
// count lives in may still carry tail bits (e.g. 32 slots = bits
// 0..31 of one 64-bit word); those are pre-set to 1 by
// `heap_set_tail_bits` so the alloc-side `ctz(~word)` never finds
// them as free.
#define HEAP_MAX_SLOTS_PER_PAGE 256u
#define HEAP_BITMAP_WORDS       4u

// Trigger the retention shrink policy when the allocator's footprint
// is at least this large AND the in-use ratio drops below the half-
// of-footprint mark. Below this threshold the allocator keeps every
// freed page retained -- AllocFreePair-style hot-reuse workloads
// stay hot and benchmarks at small sizes don't churn munmap/mmap.
// Real workloads holding tens of MiB or more get retention bled back
// to the kernel when their working set shrinks.
#define HEAP_FOOTPRINT_SHRINK_THRESHOLD (1u << 20) // 1 MiB

#ifdef __cplusplus
extern "C" {
#endif

    /// Sentinel for "no bucket" / "list end" in the hash table and warm
    /// lists. `0xFFFFFFFFu` is unreachable as a real bucket index since
    /// `pages_cap` is bounded well below 2^32.
#define HEAP_BUCKET_NONE ((u32) - 1)

    /// Per-heap-page descriptor. One per user page across every S/M/L
    /// size class. Lives in the HeapAllocator's hash table; the `page`
    /// field doubles as the bucket key (NULL == empty bucket). 64 bytes
    /// so each bucket occupies one cache line, keeping hash probes to
    /// a single load.
    ///
    /// FIELDS:
    /// - page       : User page base (4 KiB-aligned within an OS page).
    ///                Hash key. NULL means the bucket is empty.
    /// - bitmap     : Per-slot in-use mask, 1 bit per slot, LSB-first
    ///                within each word. Unused tail bits (slots >=
    ///                class slot count) are pre-set to 1 at insert
    ///                time so the alloc-side ctz(~word) never finds
    ///                them as free.
    /// - used_count : Live slot count. Reclaim when 0 (and the class
    ///                still has another warm page parked).
    /// - class_idx  : Size-class index in `0..HEAP_NUM_CLASSES-1`.
    /// - prev_warm  : Doubly-linked warm list, bucket index of the
    /// - next_warm    previous / next page in this class's warm list
    ///                (pages with `used_count < slots`). Both
    ///                `HEAP_BUCKET_NONE` when the page is full or
    ///                detached. The class's head is
    ///                `class_warm_head[class_idx]`.
    // Cache-line aligned via C11 `_Alignas` on the first member. The
    // pages[] hash probe is the hot path for every classed free and
    // grow; a 64-byte HeapPage means each probe touches exactly one
    // cache line. Without the alignment, the natural sizeof() (56)
    // leaves the tail of every bucket bleeding into the first 8 bytes
    // of the next cache line, doubling the probe cost on hit and
    // adding measurable latency to BM_AllocFreePair on small sizes.
    // This is the documented hardware-constraint carve-out in the
    // macro-hygiene section of CODING-CONVENTIONS.md.
    typedef struct HeapPage {
        _Alignas(64) void *page;
        u64                bitmap[HEAP_BITMAP_WORDS];
        u16                used_count;
        u8                 class_idx;
        u32                prev_warm;
        u32                next_warm;
    } HeapPage;

    /// Per-XL-region descriptor. One per allocation >2 KiB. XL
    /// allocations are page-aligned standalone mmaps with no bitmap
    /// and no sub-slotting; the descriptor's presence in
    /// `xl_in_use[]` IS the in-use bit, presence in `xl_freed[]` IS
    /// the retained bit. Both arrays are grown by doubling; live /
    /// retained lookup is a linear scan over the relevant array.
    ///
    /// FIELDS:
    /// - page     : Base of the user mmap (4 KiB-aligned).
    /// - os_pages : Mmap size in OS-page units (each unit is
    ///              `HEAP_PAGE_SIZE * HEAP_PAGES_PER_OS_PAGE` bytes).
    ///              Equal to the value passed to the kernel; free
    ///              unmaps exactly that many bytes.
    typedef struct HeapPageXL {
        void *page;
        u32   os_pages;
    } HeapPageXL;

    /// HeapAllocator owns:
    ///   - an open-addressed hash table of `HeapPage` descriptors keyed
    ///     by user-page base (`pages` / `pages_cap` / `pages_count`),
    ///   - a per-class doubly-linked warm list of pages with at least
    ///     one free slot (`class_warm_head`),
    ///   - two parallel descriptor arrays for XL allocations
    ///     (`xl_in_use` for live regions, `xl_freed` for retained),
    ///   - a shared LIFO retention pool of reclaimed binned (1-OS-page)
    ///     mmaps that any binned class can pop from (`recycle`),
    ///   - and a `retention_bytes` counter the shrink policy reads at
    ///     free time to decide when retained mappings should be
    ///     unmapped back to the kernel.
    ///
    /// Every region comes from a direct kernel page-mapping call (or a
    /// retention pop). There is no embedded PageAllocator; HeapAllocator
    /// talks to the kernel itself, so the type-erased
    /// `AllocatorAlloc_dyn` path is not invoked even for internal grow
    /// / reclaim work.
    struct HeapAllocator {
        Allocator base;

        /// Open-addressed hash table, linear probing, no tombstones
        /// (back-shift delete). `pages_cap` is a power of two; an empty
        /// bucket has `pages[i].page == NULL`. `pages_count` tracks
        /// occupancy for the >= 50% load grow check.
        HeapPage *pages;
        u32       pages_cap;
        u32       pages_count;

        /// Per-class head of the doubly-linked warm list. Bucket index
        /// of the first page in `class_warm_head[cls]`'s class that has
        /// `used_count < slots`, or `HEAP_BUCKET_NONE` when the class
        /// has no such page.
        u32 class_warm_head[HEAP_NUM_CLASSES];

        /// XL descriptor arrays. Each XL allocation occupies one
        /// `HeapPageXL` slot in `xl_in_use` while live; on free the
        /// descriptor moves to `xl_freed` (retention). Free-side
        /// lookup is a linear scan over `xl_in_use` -- O(N_live), but
        /// for typical XL counts (< 100) faster than the hash table
        /// it replaced because each probe is a contiguous read.
        ///
        /// Alloc looks for a matching `os_pages` entry in `xl_freed`
        /// first; on hit the descriptor is moved back to `xl_in_use`
        /// without any syscall. Misses fall through to `os_page_map`.
        ///
        /// Storage for both arrays is itself page-mapped and grown by
        /// doubling.
        HeapPageXL *xl_in_use;
        u32         xl_in_use_len;
        u32         xl_in_use_cap;
        HeapPageXL *xl_freed;
        u32         xl_freed_len;
        u32         xl_freed_cap;

        /// LIFO retention for binned (single-OS-page) heap pages.
        /// Shared across every binned class -- a 16-byte-class page
        /// that becomes empty can satisfy a future 2048-byte-class
        /// grow because the descriptor is re-initialised for the new
        /// class on pop. Storage is page-mapped and grown by doubling.
        void **recycle;
        u32    recycle_len;
        u32    recycle_cap;

        /// Sum of bytes currently held in the retention pools
        /// (`recycle` entries plus `xl_freed` entries). Compared
        /// against `base.footprint_bytes` at free time: when
        /// `footprint_bytes >= HEAP_FOOTPRINT_SHRINK_THRESHOLD` AND
        /// `retention_bytes > footprint_bytes / 2`, the allocator
        /// unmaps retained mappings until the new footprint is at
        /// most three-quarters of its old value. This is the
        /// "give memory back when the working set shrinks" policy.
        u64 retention_bytes;
    };

    // Typed entry points wired into `base` by `HeapAllocatorInit*`.
    // They are declared here so the init macro can name them in the
    // vtable cast; consumers should call the `AllocatorAlloc` /
    // `AllocatorResize` / `AllocatorRemap` / `AllocatorFree` macros
    // from `<Misra/Std/Allocator.h>` with a `HeapAllocator *`, which
    // `_Generic`-dispatches to these directly.

    ///
    /// Serve a bitmap-classed or XL-passthrough allocation. Routes the
    /// request to the matching size class (or the XL arrays for
    /// requests above the largest class), picks a free slot via
    /// `ctz` over the page bitmap, and stamps it in-use.
    ///
    /// SUCCESS: Returns a writable pointer to at least `bytes` bytes.
    ///          Zeroed when `zeroed` is non-zero. Subsequent allocs
    ///          may reuse the same page until its bitmap fills.
    /// FAILURE: Returns NULL when no warm page exists and the kernel
    ///          page-mapping call (or descriptor-table grow) fails.
    ///
    /// TAGS: Allocator, Heap, Memory, Allocation
    ///
    void *heap_allocator_allocate(HeapAllocator *self, size bytes, i8 zeroed);

    ///
    /// Try to grow / shrink a heap allocation in place. For classed
    /// slots this succeeds iff the new size still lands in the same
    /// size class; for XL allocations it succeeds iff the new size
    /// still rounds to the same OS-page count.
    ///
    /// SUCCESS: Returns 1. The pointer stays valid for `new_size`
    ///          bytes; the slot's class / mapping is unchanged.
    /// FAILURE: Returns 0 when the new size needs a different slot
    ///          class or page count. The allocation is unchanged.
    ///
    /// TAGS: Allocator, Heap, Memory, InPlace
    ///
    i8 heap_allocator_resize(HeapAllocator *self, void *ptr, size new_size);

    ///
    /// Resize a heap allocation with relocation allowed. May allocate
    /// a fresh slot (or XL mapping), copy the live bytes, and free
    /// the old slot.
    ///
    /// SUCCESS: Returns the (possibly moved) pointer. When `ptr` is
    ///          NULL this behaves like
    ///          `heap_allocator_allocate(self, new_size, true)` --
    ///          fresh allocations from a remap-NULL are zeroed. When
    ///          `new_size == 0` the allocation is freed and NULL is
    ///          returned.
    /// FAILURE: Returns NULL when the underlying allocation cannot
    ///          be served. The old allocation is left untouched.
    ///
    /// TAGS: Allocator, Heap, Memory, Reallocation
    ///
    void *heap_allocator_remap(HeapAllocator *self, void *ptr, size new_size);

    ///
    /// Free a heap-served allocation. Recovers the size class (or XL
    /// status) from the descriptor table, clears the slot's bitmap
    /// bit, and parks empty pages on the per-class warm list. Pages
    /// that emptied past the per-class warm-cap are recycled via the
    /// LIFO retention pool.
    ///
    /// SUCCESS: Returns the slot or XL-region byte count released
    ///          (what stats accounting sees).
    /// FAILURE: Aborts via `LOG_FATAL` when `ptr` is foreign to this
    ///          heap, has already been freed, or does not point at
    ///          a slot base.
    ///
    /// TAGS: Allocator, Heap, Memory, Deallocation
    ///
    size heap_allocator_deallocate(HeapAllocator *self, void *ptr);

    ///
    /// Release every user page and bookkeeping array owned by `self`,
    /// across every size class. The bookkeeping arrays themselves are
    /// released back to the kernel directly, then the struct is zeroed
    /// so any post-deinit dispatch trips `ValidateAllocator` on the
    /// cleared `__magic`.
    ///
    /// self[in,out] : HeapAllocator instance, or NULL.
    ///
    /// SUCCESS: Function returns. Every pointer previously handed out
    ///          by this heap is invalid; the struct is fully zeroed
    ///          and cannot be used until re-initialised.
    /// FAILURE: No action when `self` is NULL.
    ///
    /// TAGS: Allocator, Heap, Cleanup
    ///
    void HeapAllocatorDeinit(HeapAllocator *self);

#ifdef __cplusplus
}
#endif

///
/// Live S/M/L user-page count. The hash table's occupancy: how many
/// `HeapPage` descriptors currently track an OS page handed out by
/// this allocator. XL allocations are counted by `HeapAllocatorXlCount`
/// separately. Useful for sizing decisions and for tests that need
/// to observe grow / reclaim behaviour.
///
/// TAGS: Allocator, Heap, Query
///
#define HeapAllocatorPageCount(h) ((void)0, (h)->pages_count)

///
/// Live XL region count. One entry per allocation that exceeded the
/// 2 KiB classed-bin cap and was served as a standalone page-aligned
/// mmap.
///
/// TAGS: Allocator, Heap, Query
///
#define HeapAllocatorXlCount(h) ((void)0, (h)->xl_in_use_len)

// Initializer for class_warm_head[]: every entry HEAP_BUCKET_NONE.
// Written out explicitly because zero-init would mean "bucket 0 is the
// head" which is wrong. _Static_assert below catches HEAP_NUM_CLASSES
// drift.
#define HEAP_CLASS_WARM_HEAD_NONE                                                                                      \
    {HEAP_BUCKET_NONE,                                                                                                 \
     HEAP_BUCKET_NONE,                                                                                                 \
     HEAP_BUCKET_NONE,                                                                                                 \
     HEAP_BUCKET_NONE,                                                                                                 \
     HEAP_BUCKET_NONE,                                                                                                 \
     HEAP_BUCKET_NONE,                                                                                                 \
     HEAP_BUCKET_NONE,                                                                                                 \
     HEAP_BUCKET_NONE}
_Static_assert(HEAP_NUM_CLASSES == 8, "HEAP_CLASS_WARM_HEAD_NONE has 8 entries; sync with HEAP_NUM_CLASSES");

/// Construct a HeapAllocator with default alignment (`1`). Use as a
/// designated-initializer at any storage class. The allocator starts
/// empty: no descriptors, no XL regions, no recycled pages; all
/// kernel page mappings happen lazily on the first alloc per class.
///
///     HeapAllocator h = HeapAllocatorInit();
///     void *p = AllocatorAlloc(&h, 64, false);
///     AllocatorFree(&h, p);
///     HeapAllocatorDeinit(&h);
///
/// SUCCESS: Returns a fully-initialised `HeapAllocator` value. No OS
///          calls are made; first per-class alloc triggers the lazy
///          page-map.
/// FAILURE: Cannot fail at macro-expansion time.
///
/// TAGS: Allocator, Heap, Init
///
#define HeapAllocatorInit()                                                                                            \
    ((HeapAllocator) {                                                                                                 \
        .base =                                                                                                        \
            {.allocate        = (AllocatorAllocateFn)heap_allocator_allocate,                                          \
                   .resize          = (AllocatorResizeFn)heap_allocator_resize,                                              \
                   .remap           = (AllocatorRemapFn)heap_allocator_remap,                                                \
                   .deallocate      = (AllocatorDeallocateFn)heap_allocator_deallocate,                                      \
                   .alignment       = 1,                                                                                     \
                   .effort          = ALLOCATOR_EFFORT_ONCE,                                                                 \
                   .retry_limit     = 0,                                                                                     \
                   .__magic         = HEAP_ALLOCATOR_MAGIC | MAGIC_VALIDATED_BIT,                                                                  \
                   .footprint_bytes = 0},                                                                                    \
        .pages           = NULL,                                                                                       \
        .pages_cap       = 0,                                                                                          \
        .pages_count     = 0,                                                                                          \
        .class_warm_head = HEAP_CLASS_WARM_HEAD_NONE,                                                                  \
        .xl_in_use       = NULL,                                                                                       \
        .xl_in_use_len   = 0,                                                                                          \
        .xl_in_use_cap   = 0,                                                                                          \
        .xl_freed        = NULL,                                                                                       \
        .xl_freed_len    = 0,                                                                                          \
        .xl_freed_cap    = 0,                                                                                          \
        .recycle         = NULL,                                                                                       \
        .recycle_len     = 0,                                                                                          \
        .recycle_cap     = 0,                                                                                          \
        .retention_bytes = 0                                                                                           \
    })

/// Same as `HeapAllocatorInit()` but with a caller-supplied
/// `alignment` floor (`N`, in bytes). `N == 0` is silently coerced to
/// 1. Allocations whose effective alignment exceeds 16 are routed to
/// the XL passthrough so the bitmap-classed path can stay tied to
/// its 16-byte slot grid.
///
///     HeapAllocator h = HeapAllocatorInitAligned(64);
///
/// SUCCESS: Returns a fully-initialised `HeapAllocator` value with
///          the requested alignment floor recorded in `base.alignment`.
/// FAILURE: Cannot fail at macro-expansion time.
///
/// TAGS: Allocator, Heap, Init, Alignment
///
#define HeapAllocatorInitAligned(N)                                                                                    \
    ((HeapAllocator) {                                                                                                 \
        .base =                                                                                                        \
            {.allocate        = (AllocatorAllocateFn)heap_allocator_allocate,                                          \
                   .resize          = (AllocatorResizeFn)heap_allocator_resize,                                              \
                   .remap           = (AllocatorRemapFn)heap_allocator_remap,                                                \
                   .deallocate      = (AllocatorDeallocateFn)heap_allocator_deallocate,                                      \
                   .alignment       = (N) ? (N) : 1,                                                                         \
                   .effort          = ALLOCATOR_EFFORT_ONCE,                                                                 \
                   .retry_limit     = 0,                                                                                     \
                   .__magic         = HEAP_ALLOCATOR_MAGIC | MAGIC_VALIDATED_BIT,                                                                  \
                   .footprint_bytes = 0},                                                                                    \
        .pages           = NULL,                                                                                       \
        .pages_cap       = 0,                                                                                          \
        .pages_count     = 0,                                                                                          \
        .class_warm_head = HEAP_CLASS_WARM_HEAD_NONE,                                                                  \
        .xl_in_use       = NULL,                                                                                       \
        .xl_in_use_len   = 0,                                                                                          \
        .xl_in_use_cap   = 0,                                                                                          \
        .xl_freed        = NULL,                                                                                       \
        .xl_freed_len    = 0,                                                                                          \
        .xl_freed_cap    = 0,                                                                                          \
        .recycle         = NULL,                                                                                       \
        .recycle_len     = 0,                                                                                          \
        .recycle_cap     = 0,                                                                                          \
        .retention_bytes = 0                                                                                           \
    })

#endif // MISRA_STD_ALLOCATOR_HEAP_H
