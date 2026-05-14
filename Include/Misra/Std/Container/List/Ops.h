/// file      : Ops.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// copyright : Copyright (c) 2025, Siddharth Mishra, All rights reserved.
///
/// Misc list operations.
///

#ifndef MISRA_STD_CONTAINER_LIST_OPS_H
#define MISRA_STD_CONTAINER_LIST_OPS_H

#include "Private.h"

///
/// Set list length to 0.
///
/// list[in,out] : List to be cleared.
///
/// SUCCESS: `v`
/// FAILURE: NULL
///
#define ListClear(l) clear_list(GENERIC_LIST(l), sizeof(LIST_DATA_TYPE(l)))

///
/// Sort given list with given comparator using quicksort algorithm.
///
/// l[in,out]  : List to be sorted.
/// compare[in] : Compare function. Signature and behaviour must be similar to that of `strcmp`.
///
/// SUCCESS: Returns `v` on success.
/// FAILURE: Returns NULL otherwise.
///
#define ListSort(l, compare) qsort_list(GENERIC_LIST(l), sizeof(LIST_DATA_TYPE(l)), (compare))
#define ListMustSort(l, compare)                                                                                       \
    do {                                                                                                               \
        if (!ListSort((l), (compare))) {                                                                               \
            LOG_FATAL("ListMustSort failed");                                                                          \
        }                                                                                                              \
    } while (0)

///
/// Reverse contents of this list.
///
/// l[in,out] : List to be reversed.
///
/// SUCCESS: `v`
/// FAILURE: NULL
///
#define ListReverse(l) reverse_list(GENERIC_LIST(l), sizeof(LIST_DATA_TYPE(l)))

#endif // MISRA_STD_CONTAINER_LIST_OPS_H
