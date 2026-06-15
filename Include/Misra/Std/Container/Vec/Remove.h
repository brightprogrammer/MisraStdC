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
///             discard it (the configured `copy_deinit` is invoked instead).
/// idx[in]   : Position in [0, length).
///
/// SUCCESS : Returns to the caller. The vector length shrinks by one;
///           elements previously at indices > `idx` have shifted left by
///           one. When `ptr` is non-NULL, the removed value has been
///           bit-copied into `*ptr` (the slot is bit-copied; the
///           `copy_deinit` handler is NOT called - ownership transfers to
///           the caller). When `ptr` is NULL and `copy_deinit` is
///           configured, the handler is invoked on the removed element
///           before the slot is reclaimed.
/// FAILURE : Function cannot fail. An out-of-range `idx` is treated as a
///           caller bug and aborts via `LOG_FATAL`.
///
/// TAGS: Vec, Remove
///
#define VecRemove(v, ptr, idx)                                                                                         \
    do {                                                                                                               \
        VEC_DATATYPE(v) *p = (ptr);                                                                                    \
        remove_range_vec(GENERIC_VEC(v), p, sizeof(VEC_DATATYPE(v)), (idx), 1);                                        \
    } while (0)

///
/// Remove the element at `idx` without preserving order: the previously-last
/// element is swapped into the removed slot. Faster than `VecRemove` because
/// no range shift is performed.
///
/// v[in,out] : Vector handle.
/// ptr[out]  : Optional destination for the removed element. Pass `NULL` to
///             discard it (the configured `copy_deinit` is invoked instead).
/// idx[in]   : Position in [0, length).
///
/// SUCCESS : Returns to the caller. The vector length shrinks by one; the
///           previously-last element now occupies index `idx` (when `idx`
///           was not already the last index). When `ptr` is non-NULL, the
///           removed value has been bit-copied into `*ptr` and ownership
///           transfers to the caller. When `ptr` is NULL and `copy_deinit`
///           is configured, the handler is invoked on the removed element.
/// FAILURE : Function cannot fail. An out-of-range `idx` is treated as a
///           caller bug and aborts via `LOG_FATAL`.
///
/// TAGS: Vec, Remove, Fast, Unordered
///
#define VecRemoveFast(v, ptr, idx)                                                                                     \
    do {                                                                                                               \
        VEC_DATATYPE(v) *p = (ptr);                                                                                    \
        fast_remove_range_vec(GENERIC_VEC(v), (p), sizeof(VEC_DATATYPE(v)), (idx), 1);                                 \
    } while (0)

///
/// Remove `count` elements starting at `start` and optionally move them out to
/// the provided buffer. Order of remaining trailing elements is preserved.
///
/// v[in,out] : Vector handle.
/// ptr[out]  : Optional destination buffer of at least `count` element slots.
///             Pass `NULL` to discard the removed elements (the configured
///             `copy_deinit` is invoked instead).
/// start[in] : First removed index.
/// count[in] : Number of elements to remove.
///
/// SUCCESS : Returns to the caller. The vector length shrinks by `count`;
///           elements that previously sat at indices >= `start + count`
///           have shifted left by `count`. When `ptr` is non-NULL, the
///           removed values have been bit-copied into `*ptr` in order
///           (`copy_deinit` is not invoked - ownership transfers). When
///           `ptr` is NULL and `copy_deinit` is configured, the handler is
///           invoked on each removed element.
/// FAILURE : Function cannot fail. `start + count` exceeding `length` is a
///           caller bug and aborts via `LOG_FATAL`.
///
/// TAGS: Vec, Remove, Range
///
#define VecRemoveRange(v, ptr, start, count)                                                                           \
    do {                                                                                                               \
        VEC_DATATYPE(v) *p = (ptr);                                                                                    \
        remove_range_vec(GENERIC_VEC(v), p, sizeof(VEC_DATATYPE(v)), (start), (count));                                \
    } while (0)

///
/// Remove `count` elements starting at `start` without preserving order. The
/// removed slots are filled by swapping in elements from the tail.
///
/// v[in,out] : Vector handle.
/// ptr[out]  : Optional destination buffer. Pass `NULL` to discard (the
///             configured `copy_deinit` is invoked instead).
/// start[in] : First removed index.
/// count[in] : Number of elements to remove.
///
/// SUCCESS : Returns to the caller. The vector length shrinks by `count`;
///           the slots [start, start + count) are populated by elements
///           pulled from what was previously the tail (no defined order).
///           When `ptr` is non-NULL, the removed values have been bit-copied
///           into `*ptr` (ownership transfers, `copy_deinit` not invoked).
///           When `ptr` is NULL and `copy_deinit` is configured, the
///           handler is invoked on each removed element.
/// FAILURE : Function cannot fail. `start + count` exceeding `length` is a
///           caller bug and aborts via `LOG_FATAL`.
///
/// TAGS: Vec, Remove, Range, Fast, Unordered
///
#define VecRemoveRangeFast(v, ptr, start, count)                                                                       \
    do {                                                                                                               \
        VEC_DATATYPE(v) *p = (ptr);                                                                                    \
        fast_remove_range_vec(GENERIC_VEC(v), p, sizeof(VEC_DATATYPE(v)), (start), (count));                           \
    } while (0)


///
/// Remove and optionally return the last element of the vector.
///
/// v[in,out] : Vector handle.
/// ptr[out]  : Optional destination for the popped element. Pass `NULL` to
///             just delete it (the configured `copy_deinit` is invoked instead).
///
/// SUCCESS : Returns to the caller. The vector length shrinks by one. When
///           `ptr` is non-NULL the removed value is bit-copied into `*ptr`.
/// FAILURE : Function cannot fail. Calling on an empty vector is a caller
///           bug and aborts via `LOG_FATAL`.
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
///             just delete it (the configured `copy_deinit` is invoked instead).
///
/// SUCCESS : Returns to the caller. The vector length shrinks by one; all
///           remaining elements have shifted left by one. When `ptr` is
///           non-NULL the removed value is bit-copied into `*ptr`.
/// FAILURE : Function cannot fail. Calling on an empty vector is a caller
///           bug and aborts via `LOG_FATAL`.
///
/// TAGS: Vec, Remove, Pop, Front
///
#define VecPopFront(v, ptr) VecRemove((v), (ptr), 0)

///
/// Delete the last element of the vector.
///
/// v[in,out] : Vector handle.
///
/// SUCCESS : Returns to the caller. The vector length shrinks by one. When
///           `copy_deinit` is configured it is invoked on the dropped element.
/// FAILURE : Function cannot fail. Calling on an empty vector is a caller
///           bug and aborts via `LOG_FATAL`.
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
/// SUCCESS : Returns to the caller. The vector length shrinks by one;
///           elements previously at indices > `idx` have shifted left by
///           one. When `copy_deinit` is configured it is invoked on the
///           dropped element.
/// FAILURE : Function cannot fail. An out-of-range `idx` is a caller bug
///           and aborts via `LOG_FATAL`.
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
/// SUCCESS : Returns to the caller. The vector length shrinks by one; the
///           previously-last element now occupies `idx` (when `idx` was
///           not already the last index). When `copy_deinit` is configured
///           it is invoked on the dropped element.
/// FAILURE : Function cannot fail. An out-of-range `idx` is a caller bug
///           and aborts via `LOG_FATAL`.
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
/// SUCCESS : Returns to the caller. The vector length shrinks by `count`;
///           elements previously at indices >= `start + count` have shifted
///           left by `count`. When `copy_deinit` is configured it is
///           invoked on each dropped element.
/// FAILURE : Function cannot fail. `start + count` exceeding `length` is a
///           caller bug and aborts via `LOG_FATAL`.
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
/// SUCCESS : Returns to the caller. The vector length shrinks by `count`;
///           [start, start + count) is populated by elements pulled from
///           the tail (no defined order). When `copy_deinit` is configured
///           it is invoked on each dropped element.
/// FAILURE : Function cannot fail. `start + count` exceeding `length` is a
///           caller bug and aborts via `LOG_FATAL`.
///
/// TAGS: Vec, Delete, Range, Fast, Unordered
///
#define VecDeleteRangeFast(v, start, count) VecRemoveRangeFast((v), (VEC_DATATYPE(v) *)NULL, (start), (count))


#endif // MISRA_STD_CONTAINER_VEC_REMOVE_H
