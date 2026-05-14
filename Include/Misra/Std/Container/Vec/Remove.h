/// file      : std/container/vec/remove.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Remove items from vector in different ways.

#ifndef MISRA_STD_CONTAINER_VEC_REMOVE_H
#define MISRA_STD_CONTAINER_VEC_REMOVE_H

#include "Type.h"
#include "Private.h"

///
/// Remove the element at `idx` and optionally move its value out to `ptr`.
/// Order of trailing elements is preserved.
///
/// v[in,out] : Vector handle.
/// ptr[out]  : Optional destination for the removed element. Pass `NULL` to
///             discard it.
/// idx[in]   : Position in [0, length).
///
/// TAGS: Vec, Remove
///
#define VecRemove(v, ptr, idx)                                                                                         \
    do {                                                                                                               \
        if ((ptr) != NULL) {                                                                                           \
            const VEC_DATATYPE(v) __x = *(ptr);                                                                        \
            (void)__x;                                                                                                 \
        }                                                                                                              \
        VEC_DATATYPE(v) *p = (ptr);                                                                                    \
        remove_range_vec(GENERIC_VEC(v), (char *)p, sizeof(VEC_DATATYPE(v)), (idx), 1);                                \
    } while (0)

///
/// Remove the element at `idx` without preserving order: the previously-last
/// element is swapped into the removed slot. Faster than `VecRemove` because
/// no range shift is performed.
///
/// v[in,out] : Vector handle.
/// ptr[out]  : Optional destination for the removed element. Pass `NULL` to
///             discard it.
/// idx[in]   : Position in [0, length).
///
/// TAGS: Vec, Remove, Fast, Unordered
///
#define VecRemoveFast(v, ptr, idx)                                                                                     \
    do {                                                                                                               \
        if ((ptr) != NULL) {                                                                                           \
            const VEC_DATATYPE(v) __x = *(ptr);                                                                        \
            (void)__x;                                                                                                 \
        }                                                                                                              \
        VEC_DATATYPE(v) *p = (ptr);                                                                                    \
        fast_remove_range_vec(GENERIC_VEC(v), (char *)(p), sizeof(VEC_DATATYPE(v)), (idx), 1);                         \
    } while (0)

///
/// Remove `count` elements starting at `start` and optionally move them out to
/// the provided buffer. Order of remaining trailing elements is preserved.
///
/// v[in,out] : Vector handle.
/// ptr[out]  : Optional destination buffer of at least `count` aligned slots.
///             Pass `NULL` to discard the removed elements.
/// start[in] : First removed index.
/// count[in] : Number of elements to remove.
///
/// TAGS: Vec, Remove, Range
///
#define VecRemoveRange(v, ptr, start, count)                                                                           \
    do {                                                                                                               \
        if ((ptr) != NULL) {                                                                                           \
            const VEC_DATATYPE(v) __x = *(ptr);                                                                        \
            (void)__x;                                                                                                 \
        }                                                                                                              \
        VEC_DATATYPE(v) *p = (ptr);                                                                                    \
        remove_range_vec(GENERIC_VEC(v), (char *)p, sizeof(VEC_DATATYPE(v)), (start), (count));                        \
    } while (0)

///
/// Remove `count` elements starting at `start` without preserving order. The
/// removed slots are filled by swapping in elements from the tail.
///
/// v[in,out] : Vector handle.
/// ptr[out]  : Optional destination buffer. Pass `NULL` to discard.
/// start[in] : First removed index.
/// count[in] : Number of elements to remove.
///
/// TAGS: Vec, Remove, Range, Fast, Unordered
///
#define VecRemoveRangeFast(v, ptr, start, count)                                                                       \
    do {                                                                                                               \
        if ((ptr) != NULL) {                                                                                           \
            const VEC_DATATYPE(v) __x = *(ptr);                                                                        \
            (void)__x;                                                                                                 \
        }                                                                                                              \
        VEC_DATATYPE(v) *p = (ptr);                                                                                    \
        fast_remove_range_vec(GENERIC_VEC(v), (char *)p, sizeof(VEC_DATATYPE(v)), (start), (count));                   \
    } while (0)


///
/// Remove and optionally return the last element of the vector.
///
/// v[in,out] : Vector handle.
/// ptr[out]  : Optional destination for the popped element. Pass `NULL` to
///             just delete it.
///
/// TAGS: Vec, Remove, Pop, Back
///
#define VecPopBack(v, ptr) VecRemove((v), (ptr), (v)->length - 1)

///
/// Remove and optionally return the first element of the vector. Order of
/// trailing elements is preserved.
///
/// v[in,out] : Vector handle.
/// ptr[out]  : Optional destination for the popped element. Pass `NULL` to
///             just delete it.
///
/// TAGS: Vec, Remove, Pop, Front
///
#define VecPopFront(v, ptr) VecRemove((v), (ptr), 0)

///
/// Delete the last element of the vector.
///
/// v[in,out] : Vector handle.
///
/// TAGS: Vec, Delete, Back
///
#define VecDeleteLast(v) VecPopBack((v), (VEC_DATATYPE(v) *)NULL)

///
/// Delete the element at `idx`. Order of trailing elements is preserved.
///
/// v[in,out] : Vector handle.
/// idx[in]   : Position in [0, length).
///
/// TAGS: Vec, Delete
///
#define VecDelete(v, idx) VecRemove((v), (VEC_DATATYPE(v) *)NULL, (idx))

///
/// Delete the element at `idx` using the fast (order-not-preserving) path.
///
/// v[in,out] : Vector handle.
/// idx[in]   : Position in [0, length).
///
/// TAGS: Vec, Delete, Fast, Unordered
///
#define VecDeleteFast(v, idx) VecRemoveFast((v), (VEC_DATATYPE(v) *)NULL, (idx))

///
/// Delete `count` elements starting at `start`. Order of trailing elements is
/// preserved.
///
/// v[in,out] : Vector handle.
/// start[in] : First deleted index.
/// count[in] : Number of elements to delete.
///
/// TAGS: Vec, Delete, Range
///
#define VecDeleteRange(v, start, count) VecRemoveRange((v), (VEC_DATATYPE(v) *)NULL, (start), (count))

///
/// Delete `count` elements starting at `start` using the fast
/// (order-not-preserving) path.
///
/// v[in,out] : Vector handle.
/// start[in] : First deleted index.
/// count[in] : Number of elements to delete.
///
/// TAGS: Vec, Delete, Range, Fast, Unordered
///
#define VecDeleteRangeFast(v, start, count) VecRemoveRangeFast((v), (VEC_DATATYPE(v) *)NULL, (start), (count))


#endif // MISRA_STD_CONTAINER_VEC_REMOVE_H
