/// file      : std/memory.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Raw memory manipulation: compare / copy / move / set / generic sort.
/// String operations on NUL-terminated C strings live in `Misra/Std/Zstr.h`.

#ifndef MISRA_STD_MEMORY_H
#define MISRA_STD_MEMORY_H

#include <Misra/Std/Container/Common.h>
#include <Misra/Types.h>

///
/// Compare memory regions.
/// A zero byte count returns 0 without reading either pointer.
///
/// p1[in]  : First memory region.
/// p2[in]  : Second memory region.
/// n[in]   : Number of bytes to compare.
///
/// SUCCESS: Returns 0 if equal, <0 if p1<p2, >0 if p1>p2.
/// FAILURE: Aborts via LOG_FATAL when `n > 0` and either `p1` or `p2`
///          is NULL.
///
/// TAGS: Memory, Comparison
i32 MemCompare(const void *p1, const void *p2, size n);

///
/// Copy memory from source to destination.
/// A zero byte count returns `dst` without reading either pointer.
///
/// dst[out] : Destination memory region.
/// src[in]  : Source memory region.
/// n[in]    : Number of bytes to copy.
///
/// SUCCESS: Returns destination pointer.
/// FAILURE: Aborts via LOG_FATAL when `n > 0` and either `dst` or `src`
///          is NULL. Behaviour is undefined when the regions overlap;
///          use `MemMove` for that case.
///
/// TAGS: Memory, Copy
void *MemCopy(void *dst, const void *src, size n);

///
/// Move memory from source to destination, handling overlapping regions.
/// A zero byte count returns `dst` without reading either pointer.
///
/// dst[out] : Destination memory region.
/// src[in]  : Source memory region.
/// n[in]    : Number of bytes to move.
///
/// SUCCESS: Returns destination pointer.
/// FAILURE: Aborts via LOG_FATAL when `n > 0` and either `dst` or `src`
///          is NULL.
///
/// TAGS: Memory, Move
void *MemMove(void *dst, const void *src, size n);

///
/// Set memory region to a value.
/// A zero byte count returns `dst` without writing to it.
///
/// dst[out] : Destination memory region.
/// val[in]  : Byte value to write (low 8 bits used).
/// n[in]    : Number of bytes to set.
///
/// SUCCESS: Returns destination pointer.
/// FAILURE: Aborts via LOG_FATAL when `n > 0` and `dst` is NULL.
///
/// TAGS: Memory, Set
void *MemSet(void *dst, i32 val, size n);

///
/// Generic in-place sort over a flat array of fixed-size items.
/// Quicksort with median-of-three pivot, insertion-sort fallback for
/// small partitions, tail-iteration on the larger side to bound stack
/// depth at O(log n). The implementation is fully in-tree, so sort
/// behaviour is bit-for-bit identical across compilers and platforms.
///
/// base[in,out] : Pointer to the first element of the array.
/// n_items[in]  : Number of items in the array.
/// item_size[in]: Size of each item in bytes.
/// cmp[in]      : Comparator returning a three-way ordering: negative
///                when `a < b`, zero when equal, positive when `a > b`.
///
/// SUCCESS: Returns; the array is sorted in ascending order per `cmp`.
/// FAILURE: No failure mode. If `n_items < 2` or `cmp` is NULL the
///          call is a no-op.
///
/// TAGS: Memory, Sort
///
void MemSort(void *base, size n_items, size item_size, GenericCompare cmp);

#endif // MISRA_STD_MEMORY_H
