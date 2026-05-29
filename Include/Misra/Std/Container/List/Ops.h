/// file      : std/container/list/ops.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
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
/// SUCCESS : Returns to the caller. The list length is now 0, head and tail
///           are NULL, and every previously-stored node has been freed.
///           When `copy_deinit` is configured it has been invoked on each
///           previously-stored element.
/// FAILURE : Function cannot fail.
///
/// TAGS: List, Ops, Clear
///
#define ListClear(l) clear_list(GENERIC_LIST(l), sizeof(LIST_DATA_TYPE(l)))

///
/// Sort the list using a quicksort over a temporary contiguous buffer of the
/// element values, then write the sorted order back into the list nodes.
///
/// l[in,out]   : List to be sorted.
/// compare[in] : Comparator returning a three-way ordering: negative when
///               `a < b`, zero when equal, positive when `a > b`.
///
/// SUCCESS : Returns `true`. Elements are now in non-decreasing order
///           according to `compare`. The list length and node count are
///           unchanged; the scratch buffer has been released.
/// FAILURE : Returns `false` if the temporary contiguous buffer cannot be
///           allocated. The list order and contents are unchanged.
///
/// TAGS: List, Ops, Sort
///
#define ListSort(l, compare) list_sort(GENERIC_LIST(l), sizeof(LIST_DATA_TYPE(l)), (compare))

///
/// Aborting variant of `ListSort`. Calls `LOG_FATAL` if the scratch buffer
/// cannot be allocated.
///
/// SUCCESS : Returns to the caller.
/// FAILURE : Does not return - aborts via `LOG_FATAL` / `Abort`.
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
/// SUCCESS : Returns to the caller. The previous head is now the tail and
///           vice versa; every `next` / `prev` link has been flipped. The
///           list length is unchanged.
/// FAILURE : Function cannot fail.
///
/// TAGS: List, Ops, Reverse
///
#define ListReverse(l) reverse_list(GENERIC_LIST(l), sizeof(LIST_DATA_TYPE(l)))

#endif // MISRA_STD_CONTAINER_LIST_OPS_H
