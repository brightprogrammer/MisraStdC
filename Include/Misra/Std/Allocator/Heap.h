/// file      : std/allocator/heap.h
/// author    : Generated during allocator refactor
/// This is free and unencumbered software released into the public domain.
///
/// Heap allocator descriptor helpers.

#ifndef MISRA_STD_ALLOCATOR_HEAP_H
#define MISRA_STD_ALLOCATOR_HEAP_H

#include <Misra/Std/Allocator.h>

#ifdef __cplusplus
extern "C" {
#endif

    ///
    /// Create an allocator descriptor backed by the process heap.
    /// The returned allocator uses `malloc`/`realloc`/`free` semantics. Its
    /// `alignment` field defaults to `_Alignof(max_align_t)`, matching the
    /// alignment guarantee of `malloc`. Callers that need a stronger alignment
    /// should use `HeapAllocatorAligned(alignment)` or set the `alignment`
    /// field directly.
    ///
    /// SUCCESS: Returns a heap-backed allocator descriptor.
    /// FAILURE: Function cannot fail.
    ///
    /// TAGS: Allocator, Heap, Initialization, Memory
    ///
    Allocator HeapAllocator(void);

    ///
    /// Create a heap-backed allocator descriptor with a custom alignment.
    /// All allocations issued through the returned descriptor are aligned to
    /// `alignment` bytes. `alignment` must be a power of two; passing `0`
    /// falls back to the default `_Alignof(max_align_t)`.
    ///
    /// alignment[in] : Required alignment in bytes (power of two).
    ///
    /// USAGE:
    ///   Allocator simd_alloc = HeapAllocatorAligned(32);
    ///   Vec(SimdVec) v = VecInit(simd_alloc);
    ///
    /// SUCCESS: Returns a heap-backed allocator descriptor with the requested
    ///          alignment.
    /// FAILURE: Function cannot fail (non-pow2 alignment surfaces as an
    ///          allocation failure at use-time).
    ///
    /// TAGS: Allocator, Heap, Aligned, Initialization
    ///
    Allocator HeapAllocatorAligned(size alignment);

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_ALLOCATOR_HEAP_H
