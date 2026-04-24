/// file      : std/container/vec/access.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)

#ifndef MISRA_STD_CONTAINER_VEC_ACCESS_H
#define MISRA_STD_CONTAINER_VEC_ACCESS_H

#include <Misra/Std/Utility/Iter.h>

#include "Type.h"

///
/// Vector implementation already manages alignment for stored objects.
/// It makes sure that objects of improper size are stored at alignment of 8 bytes each
/// to avoid UB.
///
/// v[in]   : Vector to get aligned offset for.
/// idx[in] : Index of element to get offset of
///
/// SUCCESS : Alignment address.
/// FAILURE : Does not return on failure
///
#define VecAlignedOffsetAt(v, idx) ((idx) * ALIGN_UP(sizeof(VEC_DATATYPE(v)), (v)->alignment))
///
/// Value at given index in a vector.
/// It's strongly recommended to always use this instead of directly accessing the data.
///
/// v[in]   : Vector to get data from
/// idx[in] : Index to get data at
///
#define VecAt(v, idx) ((VEC_DATATYPE(v) *)(VecAlignedOffsetAt((v), (idx)) + (char *)(v)->data))[0]

///
/// Value at given index in a vector.
/// It's strongly recommended to always use this instead of directly accessing the data.
///
/// v[in]   : Vector to get data from
/// idx[in] : Index to get data at
///
#define VecPtrAt(v, idx) ((VEC_DATATYPE(v) *)(VecAlignedOffsetAt((v), (idx)) + (char *)(v)->data))

///
/// Value of first element in vector.
///
/// v[in] : Vector to get first element of.
///
#define VecFirst(v) VecAt(v, 0)

///
/// Value of last element in vector.
///
/// v[in] : Vector to get last element of.
///
#define VecLast(v) VecAt(v, (v)->length - 1)

///
/// Pointer to first element in vector
///
/// v[in] : Vector to get beginning ptr of.
///
#define VecBegin(v) ((v)->data)

///
/// Pointer at the end (after last element) of vector
///
/// v[in] : Vector to get end of.
///
#define VecEnd(v) ((char *)(v)->data + VecAlignedOffsetAt((v), (v)->length))

///
/// Size of vector in bytes. Use this instead of multiplying
/// size of item with vector length!
///
/// v[in] : Vector to get length of
///
#define VecSize(v) VecAlignedOffsetAt(v, (v)->length)

///
/// Length of vector.
///
/// v[in] : Vector to get length of
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
