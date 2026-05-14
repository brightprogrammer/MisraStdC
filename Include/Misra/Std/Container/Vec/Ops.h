/// file      : std/container/vec/private.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Different types of operations on vector

#ifndef MISRA_STD_CONTAINER_VEC_OPS_H
#define MISRA_STD_CONTAINER_VEC_OPS_H

#include "Type.h"
#include "Private.h"

///
/// Sort the vector in place using a quicksort over the comparator. The
/// comparator must return a `strcmp`-style integer (negative, zero, positive).
///
/// v[in,out]   : Vector handle.
/// compare[in] : Comparator with `strcmp`-style return.
///
/// SUCCESS : Returns to the caller. Elements are now in non-decreasing
///           order according to `compare`. The vector length is unchanged.
/// FAILURE : Function cannot fail. A NULL comparator or invalid vector is
///           a caller bug and aborts via `LOG_FATAL`.
///
/// TAGS: Vec, Ops, Sort
///
#define VecSort(v, compare) (qsort_vec(GENERIC_VEC(v), sizeof(VEC_DATATYPE(v)), (GenericCompare)(compare)))

///
/// Reverse the elements of the vector in place.
///
/// v[in,out] : Vector handle.
///
/// SUCCESS : Returns to the caller. The element at index `i` is now the
///           element that was previously at index `length - 1 - i`. The
///           vector length is unchanged.
/// FAILURE : Function cannot fail.
///
/// TAGS: Vec, Ops, Reverse
///
#define VecReverse(v) (reverse_vec(GENERIC_VEC(v), sizeof(VEC_DATATYPE(v))))

///
/// Swap the elements at two given indices in place.
///
/// v[in,out] : Vector handle.
/// idx1[in]  : First index in [0, length).
/// idx2[in]  : Second index in [0, length).
///
/// SUCCESS : Returns to the caller. The values at `idx1` and `idx2` have
///           been exchanged byte-for-byte. The vector length and all other
///           elements are unchanged.
/// FAILURE : Function cannot fail. Either index being out of range is a
///           caller bug and aborts via `LOG_FATAL`.
///
/// TAGS: Vec, Ops, Swap
///
#define VecSwapItems(v, idx1, idx2) (swap_vec(GENERIC_VEC(v), sizeof(VEC_DATATYPE(v)), (idx1), (idx2)))


#endif // MISRA_STD_CONTAINER_VEC_OPS_H
