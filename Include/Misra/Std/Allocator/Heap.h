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
/// The allocator partitions sizes into four classes:
///
///   class S  -- 16 / 32 / 64 byte requests
///   class M  -- 128 / 256 / 512 byte requests
///   class L  -- 1024 / 2048 byte requests
///   class XL -- anything larger
///
/// One user page (HEAP_PAGE_SIZE bytes, fixed at 4096) is split into
/// three (S, M) or two (L) fixed-size sub-regions, each dedicated to
/// one of the class's sub-bin sizes. The exact split is laid out by
/// the HEAP_*_OFFSET / HEAP_*_COUNT macros below and never changes.
///
/// Class XL allocations occupy contiguous page-rounded mmap regions
/// and are tracked one-per-allocation.

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

// --- Class S layout (16/32/64) -------------------------------------------
// Three sub-regions, three separate bitmap fields.
#define HEAP_S_16_OFFSET 0u
#define HEAP_S_16_COUNT  64u
#define HEAP_S_32_OFFSET 1024u
#define HEAP_S_32_COUNT  32u
#define HEAP_S_64_OFFSET 2048u
#define HEAP_S_64_COUNT  32u

// --- Class M layout (128/256/512) ----------------------------------------
// Three sub-regions packed into one u16 bitmap.
#define HEAP_M_128_OFFSET 0u
#define HEAP_M_128_COUNT  8u
#define HEAP_M_128_SHIFT  0u
#define HEAP_M_256_OFFSET 1024u
#define HEAP_M_256_COUNT  4u
#define HEAP_M_256_SHIFT  8u
#define HEAP_M_512_OFFSET 2048u
#define HEAP_M_512_COUNT  4u
#define HEAP_M_512_SHIFT  12u

// --- Class L layout (1024/2048) ------------------------------------------
// Two sub-regions packed into one u8 bitmap.
#define HEAP_L_1024_OFFSET 0u
#define HEAP_L_1024_COUNT  2u
#define HEAP_L_1024_SHIFT  0u
#define HEAP_L_2048_OFFSET 2048u
#define HEAP_L_2048_COUNT  1u
#define HEAP_L_2048_SHIFT  2u

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct HeapPageS {
        void *page;      // user page; descriptor lives outside it
        u64   bitmap_16; // 64 bits, 1 = in use
        u32   bitmap_32; // 32 bits
        u32   bitmap_64; // 32 bits
    } HeapPageS;

    typedef struct HeapPageM {
        void *page;
        u16   bitmap; // 8+4+4 bits packed by HEAP_M_*_SHIFT
    } HeapPageM;

    typedef struct HeapPageL {
        void *page;
        u8    bitmap; // 2+1 bits packed by HEAP_L_*_SHIFT
    } HeapPageL;

    typedef struct HeapPageXL {
        void *page;
        u32   num_pages; // in HEAP_PAGE_SIZE units
    } HeapPageXL;

    /// HeapAllocator carries four descriptor arrays (one per class).
    /// Each array is page-backed via the embedded PageAllocator and
    /// stays sorted by page address so free() can find the descriptor
    /// for a given pointer in O(log N).
    struct HeapAllocator {
        Allocator     base;
        PageAllocator page;

        HeapPageS  *s;
        u32         s_len;
        u32         s_cap;
        HeapPageM  *m;
        u32         m_len;
        u32         m_cap;
        HeapPageL  *l;
        u32         l_len;
        u32         l_cap;
        HeapPageXL *xl;
        u32         xl_len;
        u32         xl_cap;
    };

    void *heap_allocator_allocate(Allocator *self, size bytes, i8 zeroed);
    i8    heap_allocator_resize(Allocator *self, void *ptr, size new_size);
    void *heap_allocator_remap(Allocator *self, void *ptr, size new_size);
    size  heap_allocator_deallocate(Allocator *self, void *ptr);

    ///
    /// Release every user page and descriptor array owned by `self`,
    /// across all four size classes (S/M/L/XL). The four descriptor
    /// arrays themselves are also released through the embedded
    /// `PageAllocator`, then the struct is zeroed so any post-deinit
    /// dispatch trips `ValidateAllocator` on the cleared `__magic`.
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
        .page   = PageAllocatorInit(),                                                                                 \
        .s      = NULL,                                                                                                \
        .s_len  = 0,                                                                                                   \
        .s_cap  = 0,                                                                                                   \
        .m      = NULL,                                                                                                \
        .m_len  = 0,                                                                                                   \
        .m_cap  = 0,                                                                                                   \
        .l      = NULL,                                                                                                \
        .l_len  = 0,                                                                                                   \
        .l_cap  = 0,                                                                                                   \
        .xl     = NULL,                                                                                                \
        .xl_len = 0,                                                                                                   \
        .xl_cap = 0                                                                                                    \
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
        .page   = PageAllocatorInit(),                                                                                 \
        .s      = NULL,                                                                                                \
        .s_len  = 0,                                                                                                   \
        .s_cap  = 0,                                                                                                   \
        .m      = NULL,                                                                                                \
        .m_len  = 0,                                                                                                   \
        .m_cap  = 0,                                                                                                   \
        .l      = NULL,                                                                                                \
        .l_len  = 0,                                                                                                   \
        .l_cap  = 0,                                                                                                   \
        .xl     = NULL,                                                                                                \
        .xl_len = 0,                                                                                                   \
        .xl_cap = 0                                                                                                    \
    })

#endif // MISRA_STD_ALLOCATOR_HEAP_H
