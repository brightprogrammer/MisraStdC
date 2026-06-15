/// file      : std/container/vec/access.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)

#ifndef MISRA_STD_CONTAINER_VEC_ACCESS_H
#define MISRA_STD_CONTAINER_VEC_ACCESS_H

#include <Misra/Std/Utility/Iter.h>

#include "Type.h"

///
/// Compute the byte offset of element `idx` from the start of the vector data
/// buffer. The per-element stride is `sizeof(VEC_DATATYPE(v))`, a compile-time
/// constant. Because `sizeof(T)` is always a multiple of `_Alignof(T)`,
/// contiguous elements stay naturally aligned without any padding. The
/// allocator governs only the alignment of the buffer base, never the stride.
///
/// v[in]   : Vector to query.
/// idx[in] : Element index.
///
/// TAGS: Vec, Access, Offset
///
#define VecAlignedOffsetAt(v, idx) ((idx) * sizeof(VEC_DATATYPE(v)))

///
/// Element at `idx` accessed by value. Use this rather than indexing `data`
/// directly so the canonical element stride is used.
///
/// v[in]   : Vector to query.
/// idx[in] : Index in [0, length).
///
/// TAGS: Vec, Access, Index
///
#define VecAt(v, idx) ((VEC_DATATYPE(v) *)(VecAlignedOffsetAt((v), (idx)) + (char *)(v)->data))[0]

///
/// Pointer to the element at `idx`. Use this rather than indexing `data`
/// directly so the canonical element stride is used.
///
/// v[in]   : Vector to query.
/// idx[in] : Index in [0, length).
///
/// TAGS: Vec, Access, Index, Pointer
///
#define VecPtrAt(v, idx) ((VEC_DATATYPE(v) *)(VecAlignedOffsetAt((v), (idx)) + (char *)(v)->data))

///
/// First element of the vector by value. Caller must ensure the vector is
/// non-empty.
///
/// v[in] : Vector to query.
///
/// TAGS: Vec, Access, First
///
#define VecFirst(v) VecAt(v, 0)

///
/// Last element of the vector by value. Caller must ensure the vector is
/// non-empty.
///
/// v[in] : Vector to query.
///
/// TAGS: Vec, Access, Last
///
#define VecLast(v) VecAt(v, (v)->length - 1)

///
/// Pointer to the first element of the vector. Equivalent to `v->data`.
///
/// v[in] : Vector to query.
///
/// TAGS: Vec, Access, Iterator, Begin
///
#define VecBegin(v) ((void)0, (v)->data)

///
/// Pointer one past the last element of the vector. Suitable as an iteration
/// sentinel for `[begin, end)` loops.
///
/// v[in] : Vector to query.
///
/// TAGS: Vec, Access, Iterator, End
///
#define VecEnd(v) ((VEC_DATATYPE(v) *)((char *)(v)->data + VecAlignedOffsetAt((v), (v)->length)))

///
/// Total used storage in bytes (`sizeof(element)` times length).
///
/// v[in] : Vector to query.
///
/// TAGS: Vec, Access, Size, Bytes
///
#define VecSize(v) VecAlignedOffsetAt(v, (v)->length)

///
/// Number of elements currently stored in the vector.
///
/// v[in] : Vector to query.
///
/// TAGS: Vec, Access, Length
///
#define VecLen(v) ((void)0, (v)->length)

///
/// Capacity in elements: the most the vector can hold before the next
/// reallocation. Always `>= VecLen(v)`.
///
/// v[in] : Vector to query.
///
/// TAGS: Vec, Access, Capacity
///
#define VecCapacity(v) ((void)0, (v)->capacity)

///
/// Allocator backing the vector's storage.
///
/// v[in] : Vector to query.
///
/// TAGS: Vec, Access, Allocator
///
#define VecAllocator(v) ((void)0, (v)->allocator)

///
/// Deep-copy `init` callback wired into the vector, or `NULL` if the
/// vector was initialised without deep-copy semantics.
///
/// v[in] : Vector to query.
///
/// TAGS: Vec, Access, DeepCopy
///
#define VecCopyInit(v) ((void)0, (v)->copy_init)

///
/// Deep-copy `deinit` callback wired into the vector, or `NULL` if the
/// vector was initialised without deep-copy semantics.
///
/// v[in] : Vector to query.
///
/// TAGS: Vec, Access, DeepCopy
///
#define VecCopyDeinit(v) ((void)0, (v)->copy_deinit)

///
/// Check whether vector has no elements.
///
/// v[in] : Vector to query.
///
/// SUCCESS : Returns `true` when the vector length is 0.
/// FAILURE : Returns `false` when the vector contains at least one element.
///           The vector is not modified.
///
/// TAGS: Vec, Empty, Query
///
#define VecEmpty(v) (VecLen(v) == 0)

///
/// Find the first element equal to the searched value.
///
/// NOTE: `item_ptr` must point to a value comparable with vector elements.
///       Use `&LVAL(expr)` when searching with a temporary expression.
///
/// v[in]        : Vector to search.
/// item_ptr[in] : Pointer to searched value.
/// compare[in]  : Comparator returning `0` for equality.
///
/// SUCCESS : Returns the index of the first matching element in [0, length).
///           The vector is not modified.
/// FAILURE : Returns `SIZE_MAX` when no element matches. The vector is not
///           modified.
///
/// TAGS: Vec, Find, Search, Compare
///
#define VecFind(v, item_ptr, compare)                                                                                  \
    find_idx_vec(GENERIC_VEC(v), (item_ptr), sizeof(VEC_DATATYPE(v)), (GenericCompare)(compare))

///
/// Check whether vector contains a matching element.
///
/// NOTE: `item_ptr` must point to a value comparable with vector elements.
///
/// v[in]        : Vector to search.
/// item_ptr[in] : Pointer to searched value.
/// compare[in]  : Comparator returning `0` for equality.
///
/// SUCCESS : Returns `true` when at least one matching element exists. The
///           vector is not modified.
/// FAILURE : Returns `false` when no element matches. The vector is not
///           modified.
///
/// TAGS: Vec, Contains, Search, Compare
///
#define VecContains(v, item_ptr, compare) (VecFind((v), (item_ptr), (compare)) != SIZE_MAX)

#endif // MISRA_STD_CONTAINER_VEC_ACCESS_H
