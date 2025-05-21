/// file      : std/container/vec/private.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// copyright : Copyright (c) 2025, Siddharth Mishra, All rights reserved.
///
/// Different types of operations on vector

#ifndef MISRA_STD_CONTAINER_VEC_OPS_H
#define MISRA_STD_CONTAINER_VEC_OPS_H

// clang-format off
#include "Type.h"
#include "Private.h"
// clang-format on

///
/// Sort given vector with given comparator using quicksort algorithm.
///
/// v[in,out]  : Vector to be sorted.
/// compare[in] : Compare function. Signature and behaviour must be similar to that of `strcmp`.
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define VecSort(v, compare) (qsort_vec(GENERIC_VEC(v), sizeof(VEC_DATATYPE(v)), (GenericCompare)(compare)))

///
/// Reverse contents of this vector.
///
/// v[in,out] : Vector to be reversed.
///
/// SUCCESS : `v`
/// FAILURE : Does not return on failure
///
#define VecReverse(v) (reverse_vec(GENERIC_VEC(v), sizeof(VEC_DATATYPE(v))))

///
/// Swap items at given indices.
///
/// v[in,out] : Vector to swap items in.
/// idx1[in]  : Index/Position of first item.
/// idx1[in]  : Index/Position of second item.
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define VecSwapItems(v, idx1, idx2) (swap_vec(GENERIC_VEC(v), sizeof(VEC_DATATYPE(v)), (idx1), (idx2)))


#endif // MISRA_STD_CONTAINER_VEC_OPS_H
