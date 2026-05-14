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
/// TAGS: Vec, Ops, Sort
///
#define VecSort(v, compare) (qsort_vec(GENERIC_VEC(v), sizeof(VEC_DATATYPE(v)), (GenericCompare)(compare)))

///
/// Reverse the elements of the vector in place.
///
/// v[in,out] : Vector handle.
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
/// TAGS: Vec, Ops, Swap
///
#define VecSwapItems(v, idx1, idx2) (swap_vec(GENERIC_VEC(v), sizeof(VEC_DATATYPE(v)), (idx1), (idx2)))


#endif // MISRA_STD_CONTAINER_VEC_OPS_H
