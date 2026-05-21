/// file      : std/allocator/heap.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Bitmap-backed heap allocator. Every user page contains ONLY user
/// data -- no header, no `next` pointer, no magic. Per-page metadata
/// (the bitmap of which slots are in use, and the page address) lives
/// in allocator-owned descriptor arrays acquired separately from
/// PageAllocator.
///
/// Sizes route into one of 17 size classes plus an XL passthrough.
/// One slot size per user page (no co-located sub-bins): a page that
/// holds 128-byte slots only holds 128-byte slots, never mixed with
/// 256-byte slots on the same page. This trades the old "mixed page"
/// pack density for the right to add many more class sizes without
/// the per-page alignment-waste cascading.
///
/// Class set (bytes):
///
///   S  -- 16, 24, 32, 48, 64
///   M  -- 80, 96, 128, 160, 192, 256, 384, 512
///   L  -- 768, 1024, 1536, 2048
///   XL -- anything larger (one page-aligned mmap per allocation)
///
/// HEAP_PAGE_SIZE is fixed at 4096. macOS-aarch64 has 16 KiB OS pages
/// and asks PageAllocator for one OS page per grow; that mmap is then
/// carved into HEAP_PAGES_PER_OS_PAGE heap pages, each with its own
/// descriptor. The reclaim-when-empty path skips macOS-aarch64 for
/// now (a heap page can't be returned without its mmap siblings).
///
/// Every per-page descriptor stores: page base, class index, in-use
/// slot count, and a 256-bit bitmap (the widest any class needs).
/// The "S/M/L" tier names are a documentation convenience; internally
/// every descriptor is the same type and lives in a single sorted-by-
/// address array, so free is one binary search regardless of class.

#ifndef MISRA_STD_ALLOCATOR_HEAP_H
#define MISRA_STD_ALLOCATOR_HEAP_H

#include <Misra/Std/Allocator.h>
#include <Misra/Std/Allocator/Page.h>

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

// Number of S/M/L size classes. Indexed 0..HEAP_NUM_CLASSES-1.
// XL is "class HEAP_NUM_CLASSES" by convention -- it has its own
// descriptor type and its own array.
#define HEAP_NUM_CLASSES 17u

// Max slots in any single class's page. The 16-byte class fits 256
// slots in 4 KiB; bitmaps are sized to cover this and unused tail
// bits are pre-set to 1 at descriptor init so ctz never reports them.
#define HEAP_MAX_SLOTS_PER_PAGE 256u
#define HEAP_BITMAP_WORDS       4u

#ifdef __cplusplus
extern "C" {
#endif

    /// Per-heap-page descriptor. One per user page across every S/M/L
    /// size class. Unified type keeps the per-class arrays from
    /// multiplying. 48 bytes, naturally aligned for the u64 bitmap.
    ///
    /// FIELDS:
    /// - page       : User page base (4 KiB-aligned within an OS page).
    ///                Sort key for the allocator's descriptor array.
    /// - bitmap     : Per-slot in-use mask, 1 bit per slot, LSB-first
    ///                within each word. Unused tail bits (slots >=
    ///                class slot count) are pre-set to 1 at insert
    ///                time so the alloc-side ctz(~word) never finds
    ///                them as free.
    /// - used_count : Live slot count. Reclaim when 0 (and the class
    ///                still has another warm page parked).
    /// - class_idx  : Index into heap_class_size[] (0..HEAP_NUM_CLASSES-1).
    typedef struct HeapPage {
        void *page;
        u64   bitmap[HEAP_BITMAP_WORDS];
        u16   used_count;
        u8    class_idx;
        u8    _pad;
    } HeapPage;

    typedef struct HeapPageXL {
        void *page;
        u32   num_pages; // in HEAP_PAGE_SIZE units
    } HeapPageXL;

    /// HeapAllocator carries one unified descriptor array for every
    /// S/M/L page (sorted by page address for binary-search on free)
    /// plus a separate XL array. `class_count` tracks how many pages
    /// of each class are live, used by the reclaim path to keep one
    /// warm page per class.
    struct HeapAllocator {
        Allocator     base;
        PageAllocator page;

        HeapPage *pages;
        u32       pages_len;
        u32       pages_cap;

        HeapPageXL *xl;
        u32         xl_len;
        u32         xl_cap;

        u32 class_count[HEAP_NUM_CLASSES];
    };

    void *heap_allocator_allocate(Allocator *self, size bytes, i8 zeroed);
    i8    heap_allocator_resize(Allocator *self, void *ptr, size new_size);
    void *heap_allocator_remap(Allocator *self, void *ptr, size new_size);
    size  heap_allocator_deallocate(Allocator *self, void *ptr);

    ///
    /// Release every user page and descriptor array owned by `self`,
    /// across every size class. The descriptor arrays themselves are
    /// also released through the embedded `PageAllocator`, then the
    /// struct is zeroed so any post-deinit dispatch trips
    /// `ValidateAllocator` on the cleared `__magic`.
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

// Designated init for the class_count[] array. Done as a macro so
// init macros below stay readable. C99 allows omitted designators
// for elements; everything not named here implicitly zero-initialises.
#define HEAP_CLASS_COUNT_ZERO {0}

#define HeapAllocatorInit()                                                                                            \
    ((HeapAllocator) {                                                                                                 \
        .base =                                                                                                        \
            {.allocate    = heap_allocator_allocate,                                                                   \
                   .resize      = heap_allocator_resize,                                                                     \
                   .remap       = heap_allocator_remap,                                                                      \
                   .deallocate  = heap_allocator_deallocate,                                                                 \
                   .alignment   = 1,                                                                                         \
                   .effort      = ALLOCATOR_EFFORT_ONCE,                                                                     \
                   .retry_limit = 0,                                                                                         \
                   .__magic     = HEAP_ALLOCATOR_MAGIC},                                                                         \
        .page        = PageAllocatorInit(),                                                                            \
        .pages       = NULL,                                                                                           \
        .pages_len   = 0,                                                                                              \
        .pages_cap   = 0,                                                                                              \
        .xl          = NULL,                                                                                           \
        .xl_len      = 0,                                                                                              \
        .xl_cap      = 0,                                                                                              \
        .class_count = HEAP_CLASS_COUNT_ZERO                                                                           \
    })

#define HeapAllocatorInitAligned(N)                                                                                    \
    ((HeapAllocator) {                                                                                                 \
        .base =                                                                                                        \
            {.allocate    = heap_allocator_allocate,                                                                   \
                   .resize      = heap_allocator_resize,                                                                     \
                   .remap       = heap_allocator_remap,                                                                      \
                   .deallocate  = heap_allocator_deallocate,                                                                 \
                   .alignment   = (N) ? (N) : 1,                                                                             \
                   .effort      = ALLOCATOR_EFFORT_ONCE,                                                                     \
                   .retry_limit = 0,                                                                                         \
                   .__magic     = HEAP_ALLOCATOR_MAGIC},                                                                         \
        .page        = PageAllocatorInit(),                                                                            \
        .pages       = NULL,                                                                                           \
        .pages_len   = 0,                                                                                              \
        .pages_cap   = 0,                                                                                              \
        .xl          = NULL,                                                                                           \
        .xl_len      = 0,                                                                                              \
        .xl_cap      = 0,                                                                                              \
        .class_count = HEAP_CLASS_COUNT_ZERO                                                                           \
    })

#endif // MISRA_STD_ALLOCATOR_HEAP_H
