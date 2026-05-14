/// file      : std/allocator/pool.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Per-descriptor fixed-size slot pool. Every allocation must match the
/// configured slot size; alloc/free are O(1) via an intrusive free list.
/// State is inline; the embedded `PageAllocator` provides backing slabs.

#ifndef MISRA_STD_ALLOCATOR_POOL_H
#define MISRA_STD_ALLOCATOR_POOL_H

#include <Misra/Std/Allocator.h>
#include <Misra/Std/Allocator/Page.h>

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct PoolChunk    PoolChunk;
    typedef struct PoolFreeSlot PoolFreeSlot;

    typedef struct {
        Allocator     base;
        PoolChunk    *head;
        PoolChunk    *tail;
        PoolFreeSlot *free_head;
        size          slot_size;
        size          slots_per_chunk;
        PageAllocator page;
    } PoolAllocator;

    void *pool_allocator_allocate(Allocator *self, size bytes, bool zeroed);
    void *pool_allocator_reallocate(Allocator *self, void *ptr, size old_size, size new_size);
    void  pool_allocator_deallocate(Allocator *self, void *ptr, size bytes);

    void PoolAllocatorDeinit(PoolAllocator *self);

#ifdef __cplusplus
}
#endif

#define MISRA_POOL_DEFAULT_CHUNK_SLOTS 256u

///
/// Initialize a `PoolAllocator` with the given slot size. Slot size is
/// padded internally so each slot holds the intrusive free-list pointer.
///
#define PoolAllocatorInit(slot_size_bytes)                                                                             \
    ((PoolAllocator) {.base            = {.allocate    = pool_allocator_allocate,                                      \
                                          .reallocate  = pool_allocator_reallocate,                                    \
                                          .deallocate  = pool_allocator_deallocate,                                    \
                                          .alignment   = 1,                                                            \
                                          .effort      = ALLOCATOR_EFFORT_ONCE,                                        \
                                          .retry_limit = 0,                                                            \
                                          .flags       = 0},                                                           \
                      .head            = NULL,                                                                          \
                      .tail            = NULL,                                                                          \
                      .free_head       = NULL,                                                                          \
                      .slot_size       = (slot_size_bytes),                                                              \
                      .slots_per_chunk = MISRA_POOL_DEFAULT_CHUNK_SLOTS,                                                \
                      .page            = PageAllocatorInit()})

///
/// Initialize a `PoolAllocator` with a custom alignment floor.
///
#define PoolAllocatorInitAligned(slot_size_bytes, alignment_value)                                                     \
    ((PoolAllocator) {.base            = {.allocate    = pool_allocator_allocate,                                      \
                                          .reallocate  = pool_allocator_reallocate,                                    \
                                          .deallocate  = pool_allocator_deallocate,                                    \
                                          .alignment   = (alignment_value) ? (alignment_value) : 1,                    \
                                          .effort      = ALLOCATOR_EFFORT_ONCE,                                        \
                                          .retry_limit = 0,                                                            \
                                          .flags       = 0},                                                           \
                      .head            = NULL,                                                                          \
                      .tail            = NULL,                                                                          \
                      .free_head       = NULL,                                                                          \
                      .slot_size       = (slot_size_bytes),                                                              \
                      .slots_per_chunk = MISRA_POOL_DEFAULT_CHUNK_SLOTS,                                                \
                      .page            = PageAllocatorInit()})

#endif // MISRA_STD_ALLOCATOR_POOL_H
