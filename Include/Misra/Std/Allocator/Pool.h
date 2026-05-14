/// file      : std/allocator/pool.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Fixed-size slot allocator (object pool) backed by page-mapped chunks.
/// Every allocation must be exactly the configured slot size; `allocate`
/// and `deallocate` run in O(1) using an intrusive free list. Best fit for
/// linked-list nodes, graph slots, and other uniform-size workloads.

#ifndef MISRA_STD_ALLOCATOR_POOL_H
#define MISRA_STD_ALLOCATOR_POOL_H

#include <Misra/Std/Allocator.h>

#ifdef __cplusplus
extern "C" {
#endif

    ///
    /// Create a pool allocator that hands out fixed-size slots.
    /// `slot_size` is recorded on the descriptor and verified on every
    /// allocation request. The pool grows by mapping fresh page-backed
    /// chunks through `PageAllocator` when the free list is empty.
    ///
    /// slot_size[in] : Size of each allocation in bytes. Must be > 0.
    ///                 Sizes above `UINT32_MAX` are clamped down.
    ///
    /// SUCCESS: Returns a pool allocator descriptor.
    /// FAILURE: Function cannot fail at creation. Out-of-memory shows up
    ///          as a NULL from the first `AllocatorAlloc` call.
    ///
    /// USAGE:
    ///   Allocator node_pool = PoolAllocator(sizeof(MyNode));
    ///   List(MyNode) list = ListInit(node_pool);
    ///
    /// TAGS: Allocator, Pool, Slot, FreeList, Initialization
    ///
    Allocator PoolAllocator(size slot_size);

    ///
    /// Create a pool allocator with a custom alignment floor.
    /// Slots are at least `alignment`-aligned. The effective slot size is
    /// padded up to a multiple of `alignment` so the intrusive free-list
    /// pointer can be stored inside each slot.
    ///
    /// slot_size[in] : Size of each allocation in bytes.
    /// alignment[in] : Required slot alignment (power of two).
    ///
    /// SUCCESS: Returns a configured pool allocator descriptor.
    /// FAILURE: Function cannot fail at creation.
    ///
    /// TAGS: Allocator, Pool, Slot, Aligned, Initialization
    ///
    Allocator PoolAllocatorAligned(size slot_size, size alignment);

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_ALLOCATOR_POOL_H
