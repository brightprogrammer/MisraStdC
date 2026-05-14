/// file      : std/container/vec/access.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)

#ifndef MISRA_STD_CONTAINER_VEC_ACCESS_H
#define MISRA_STD_CONTAINER_VEC_ACCESS_H

#include <Misra/Std/Utility/Iter.h>

#include "Type.h"

///
/// Compute the aligned byte offset of element `idx` from the start of the
/// vector data buffer. The vector applies per-element alignment internally so
/// that arbitrarily-typed payloads are correctly aligned.
///
/// v[in]   : Vector to query.
/// idx[in] : Element index.
///
/// TAGS: Vec, Access, Alignment
///
#define VecAlignedOffsetAt(v, idx) ((idx) * ALIGN_UP(sizeof(VEC_DATATYPE(v)), (v)->alignment))

///
/// Element at `idx` accessed by value. Use this rather than indexing `data`
/// directly so element alignment is respected.
///
/// v[in]   : Vector to query.
/// idx[in] : Index in [0, length).
///
/// TAGS: Vec, Access, Index
///
#define VecAt(v, idx) ((VEC_DATATYPE(v) *)(VecAlignedOffsetAt((v), (idx)) + (char *)(v)->data))[0]

///
/// Pointer to the element at `idx`. Use this rather than indexing `data`
/// directly so element alignment is respected.
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
#define VecBegin(v) ((v)->data)

///
/// Pointer one past the last element of the vector. Suitable as an iteration
/// sentinel for `[begin, end)` loops.
///
/// v[in] : Vector to query.
///
/// TAGS: Vec, Access, Iterator, End
///
#define VecEnd(v) ((char *)(v)->data + VecAlignedOffsetAt((v), (v)->length))

///
/// Total used storage in bytes (aligned element size times length). Use this
/// rather than `sizeof(element) * length` because vector elements may be
/// padded for alignment.
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
#define VecLen(v) ((v)->length)

///
/// Check whether vector has no elements.
///
/// v[in] : Vector to query.
///
/// SUCCESS : `true` when vector length is 0.
/// FAILURE : `false`
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
/// SUCCESS : Index of first matching element.
/// FAILURE : `SIZE_MAX` if no element matches.
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
/// SUCCESS : `true` when a matching element exists.
/// FAILURE : `false`
///
/// TAGS: Vec, Contains, Search, Compare
///
#define VecContains(v, item_ptr, compare) (VecFind((v), (item_ptr), (compare)) != SIZE_MAX)

#endif // MISRA_STD_CONTAINER_VEC_ACCESS_H
