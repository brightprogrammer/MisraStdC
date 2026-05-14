/// file      : std/allocator/heap.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Per-descriptor binned heap allocator. Small allocations (<= 2 KiB) come
/// from power-of-two free-list bins; large allocations pass straight
/// through to an embedded `PageAllocator`. No library global state - every
/// `HeapAllocator` value owns its own bins and chunk list inline.

#ifndef MISRA_STD_ALLOCATOR_HEAP_H
#define MISRA_STD_ALLOCATOR_HEAP_H

#include <Misra/Std/Allocator.h>
#include <Misra/Std/Allocator/Page.h>

#define HEAP_MIN_BIN_LOG 4u                                          // smallest bin: 2^4 = 16 bytes
#define HEAP_MAX_BIN_LOG 11u                                         // largest bin:  2^11 = 2048 bytes
#define HEAP_NUM_BINS    (HEAP_MAX_BIN_LOG - HEAP_MIN_BIN_LOG + 1u)

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct HeapFreeSlot  HeapFreeSlot;
    typedef struct HeapPageChunk HeapPageChunk;

    ///
    /// Typed binned heap allocator. State is inline:
    /// - `bins`        : free-list heads for each power-of-two size class.
    /// - `chunks_head` : singly-linked list of page-backed chunks this heap owns.
    /// - `page`        : embedded `PageAllocator` used to acquire new chunks.
    ///
    typedef struct HeapAllocator {
        Allocator      base;
        HeapFreeSlot  *bins[HEAP_NUM_BINS];
        HeapPageChunk *chunks_head;
        PageAllocator  page;
    } HeapAllocator;

    void *heap_allocator_allocate(Allocator *self, size bytes, bool zeroed);
    void *heap_allocator_reallocate(Allocator *self, void *ptr, size old_size, size new_size);
    void  heap_allocator_deallocate(Allocator *self, void *ptr, size bytes);

    ///
    /// Release every page chunk currently owned by `self`. After this call
    /// the heap is back to its post-`HeapAllocatorInit` state and any pointer
    /// previously returned by `AllocatorAlloc` through this heap is invalid.
    ///
    /// self[in,out] : Heap allocator instance.
    ///
    /// SUCCESS: Function returns. Owned pages have been unmapped.
    /// FAILURE: No action when `self` is NULL.
    ///
    /// TAGS: Allocator, Heap, Cleanup
    ///
    void HeapAllocatorDeinit(HeapAllocator *self);

#ifdef __cplusplus
}
#endif

///
/// Initialize a `HeapAllocator` with default alignment (1, meaning "no
/// stronger than the backing page allocator's natural alignment").
/// Use as a designated-initializer:
///
///     HeapAllocator heap = HeapAllocatorInit();
///     Vec(int) v = VecInit(&heap);
///     ...
///     VecDeinit(&v);
///     HeapAllocatorDeinit(&heap);
///
#define HeapAllocatorInit()                                                                                            \
    ((HeapAllocator) {.base        = {.allocate    = heap_allocator_allocate,                                          \
                                      .reallocate  = heap_allocator_reallocate,                                        \
                                      .deallocate  = heap_allocator_deallocate,                                        \
                                      .alignment   = 1,                                                                \
                                      .effort      = ALLOCATOR_EFFORT_ONCE,                                            \
                                      .retry_limit = 0,                                                                \
                                      .flags       = 0},                                                               \
                      .bins        = {0},                                                                              \
                      .chunks_head = NULL,                                                                              \
                      .page        = PageAllocatorInit()})

///
/// Initialize a `HeapAllocator` with a custom alignment floor.
/// Over-page-aligned requests bypass the bin path and go straight to the
/// embedded `PageAllocator`.
///
#define HeapAllocatorInitAligned(N)                                                                                    \
    ((HeapAllocator) {.base        = {.allocate    = heap_allocator_allocate,                                          \
                                      .reallocate  = heap_allocator_reallocate,                                        \
                                      .deallocate  = heap_allocator_deallocate,                                        \
                                      .alignment   = (N) ? (N) : 1,                                                    \
                                      .effort      = ALLOCATOR_EFFORT_ONCE,                                            \
                                      .retry_limit = 0,                                                                \
                                      .flags       = 0},                                                               \
                      .bins        = {0},                                                                              \
                      .chunks_head = NULL,                                                                              \
                      .page        = PageAllocatorInit()})

#endif // MISRA_STD_ALLOCATOR_HEAP_H
