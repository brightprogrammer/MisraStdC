/// file      : std/container/vec/remove.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// copyright : Copyright (c) 2025, Siddharth Mishra, All rights reserved.
///
/// Remove items from vector in different ways.

#ifndef MISRA_STD_CONTAINER_VEC_REMOVE_H
#define MISRA_STD_CONTAINER_VEC_REMOVE_H

// clang-format off
#include "Type.h"
#include "Private.h"
// clang-format on

///
/// Remove item from vector at given index and store in given pointer.
/// Order of elements is guaranteed to be preserved.
///
/// v[in,out] : Vector to remove item from.
/// ptr[out]  : Where removed item will be stored. If not provided then it's equivalent to
///             deleting the item at specified index.
/// idx[in]   : Index in vector to remove item from.
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define VecRemove(v, ptr, idx)                                                                                         \
    do {                                                                                                               \
        VEC_DATATYPE(v) *p = (ptr);                                                                                    \
        remove_range_vec(GENERIC_VEC(v), (char *)p, sizeof(VEC_DATATYPE(v)), (idx), 1);                                \
    } while (0)

///
/// Remove item from vector at given index and store in given pointer.
/// Order of elements inside vector is not guaranteed to be preserved.
/// The implementation is faster in some scenarios that `VecRemove`
///
/// v[in,out] : Vector to remove item from.
/// ptr[out]  : Where removed item will be stored. If not provided then it's equivalent to
///             deleting the item at specified index.
/// idx[in]   : Index in vector to remove item from.
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define VecRemoveFast(v, ptr, idx)                                                                                     \
    do {                                                                                                               \
        VEC_DATATYPE(v) *p = (ptr);                                                                                    \
        fast_remove_range_vec(GENERIC_VEC(v), (char *)(p), sizeof(VEC_DATATYPE(v)), (idx), 1);                         \
    } while (0)

///
/// Remove data from vector in given range [start, start + count)
/// Order of elements is guaranteed to be preserved.
///
/// v[in,out] : Vector to remove item from.
/// ptr[out]  : Where removed data will be stored. If not provided then it's equivalent to
///             deleting the items in specified range.
/// start[in] : Index in vector to removing items from.
/// count[in] : Number of items from starting index.
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define VecRemoveRange(v, ptr, start, count)                                                                           \
    do {                                                                                                               \
        VEC_DATATYPE(v) *p = (ptr);                                                                                    \
        remove_range_vec(GENERIC_VEC(v), (char *)p, sizeof(VEC_DATATYPE(v)), (start), (count));                        \
    } while (0)

///
/// Remove item from vector at given index and store in given pointer.
/// Order of elements inside vector is not guaranteed to be preserved.
/// The implementation is faster in some scenarios that `VecRemove`
///
/// v[in,out] : Vector to remove item from.
/// ptr[out]  : Where removed data will be stored. If not provided then it's equivalent to
///             deleting the items in specified range.
/// start[in] : Index in vector to removing items from.
/// count[in] : Number of items from starting index.
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define VecRemoveRangeFast(v, ptr, start, count)                                                                       \
    do {                                                                                                               \
        VEC_DATATYPE(v) *p = (ptr);                                                                                    \
        fast_remove_range_vec(GENERIC_VEC(v), (char *)p, sizeof(VEC_DATATYPE(v)), (start), (count));                   \
    } while (0)


///
/// Pop item from vector back.
///
/// v[in,out]  : Vector to pop item from.
/// ptr[out]   : Popped item will be stored here. Make sure this has sufficient memory
///              to store memcopied data. If no pointer is provided, then it's equivalent
///              to deleting item from last position.
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define VecPopBack(v, ptr) VecRemove((v), (ptr), (v)->length - 1)

///
/// Pop item from vector front.
///
/// v[in,out]  : Vector to pop item from.
/// ptr[out]   : Popped item will be stored here. Make sure this has sufficient memory
///              to store memcopied data. If no pointer is provided, then it's equivalent
///              to deleting item from last position.
///
#define VecPopFront(v, ptr) VecRemove((v), (ptr), 0)

///
/// Delete last item from vec
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define VecDeleteLast(v) VecPopBack((v), NULL)

///
/// Delete item at given index
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define VecDelete(v, idx) VecRemove((v), NULL, (idx))

///
/// Delete item at given index using faster implementation.
/// Order preservation is not guaranteed
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define VecDeleteFast(v, idx) VecRemoveFast((v), NULL, (idx))

///
/// Delete items in given range [start, start + count)
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define VecDeleteRange(v, start, count) VecRemoveRange((v), NULL, (start), (count))

///
/// Delete items in given range [start, start + count) using faster implementation.
/// Order preservation is not guaranteed
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define VecDeleteRangeFast(v, start, count) VecRemoveRangeFast((v), NULL, (start), (count))


#endif // MISRA_STD_CONTAINER_VEC_REMOVE_H
