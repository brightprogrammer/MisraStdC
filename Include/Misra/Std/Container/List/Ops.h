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
/// Remove all entries from the list. Length becomes 0; node storage is released
/// through the list's allocator. Element payloads are deinitialized via the
/// configured `copy_deinit` handler when present.
///
/// l[in,out] : List to be cleared.
///
/// TAGS: List, Ops, Clear
///
#define ListClear(l) clear_list(GENERIC_LIST(l), sizeof(LIST_DATA_TYPE(l)))

///
/// Sort the list using a quicksort over a temporary contiguous buffer of the
/// element values, then write the sorted order back into the list nodes.
///
/// l[in,out]   : List to be sorted.
/// compare[in] : Compare function with `strcmp`-style return (negative, zero,
///               positive).
///
/// SUCCESS : Returns `true`.
/// FAILURE : Returns `false` if the scratch buffer allocation fails. The list order
///           is unchanged in that case.
///
/// TAGS: List, Ops, Sort
///
#define ListSort(l, compare) qsort_list(GENERIC_LIST(l), sizeof(LIST_DATA_TYPE(l)), (compare))

///
/// Aborting variant of `ListSort`. Calls `LOG_FATAL` if the scratch buffer
/// cannot be allocated.
///
/// SUCCESS : Returns to the caller.
/// FAILURE : Does not return - aborts via `LOG_FATAL` / `SysAbort`.
///
/// TAGS: List, Ops, Sort, Must, Abort
///
#define ListMustSort(l, compare)                                                                                       \
    do {                                                                                                               \
        if (!ListSort((l), (compare))) {                                                                               \
            LOG_FATAL("ListMustSort failed");                                                                          \
        }                                                                                                              \
    } while (0)

///
/// Reverse the order of nodes in the list in place.
///
/// l[in,out] : List to be reversed.
///
/// TAGS: List, Ops, Reverse
///
#define ListReverse(l) reverse_list(GENERIC_LIST(l), sizeof(LIST_DATA_TYPE(l)))

#endif // MISRA_STD_CONTAINER_LIST_OPS_H
