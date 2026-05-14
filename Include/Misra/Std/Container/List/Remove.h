/// file      : Remove.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// copyright : Copyright (c) 2025, Siddharth Mishra, All rights reserved.
///
/// List remove helpers
///

#ifndef MISRA_STD_CONTAINER_LIST_REMOVE_H
#define MISRA_STD_CONTAINER_LIST_REMOVE_H

///
/// Remove the element at `idx` and optionally move its value out to `val`.
///
/// l[in,out] : List handle.
/// val[out]  : Optional destination for the removed element. Pass `NULL` to
///             discard it.
/// idx[in]   : Position in [0, length).
///
/// TAGS: List, Remove
///
#define ListRemove(l, val, idx) remove_range_list(GENERIC_LIST(l), (val), sizeof(LIST_DATA_TYPE(l)), (idx), 1)

///
/// Remove the first element of the list and optionally store its value.
///
/// l[in,out] : List handle.
/// val[out]  : Optional destination for the popped element.
///
/// TAGS: List, Remove, Pop, Front
///
#define ListPopFront(l, val) ListRemove((l), (val), 0);

///
/// Remove the last element of the list and optionally store its value.
///
/// l[in,out] : List handle.
/// val[out]  : Optional destination for the popped element.
///
/// TAGS: List, Remove, Pop, Back
///
#define ListPopBack(l, val) ListRemove((l), (val), (l)->length - 1)

///
/// Remove `count` elements starting at `start` and optionally copy them out.
///
/// l[in,out] : List handle.
/// rd[out]   : Optional destination buffer of at least `count` slots. Pass
///             `NULL` to discard the removed elements.
/// start[in] : First removed index.
/// count[in] : Number of elements to remove.
///
/// TAGS: List, Remove, Range
///
#define ListRemoveRange(l, rd, start, count)                                                                           \
    remove_range_list(GENERIC_LIST(l), (rd), sizeof(LIST_DATA_TYPE(l)), (start), (count))

///
/// Delete the last element of the list.
///
/// l[in,out] : List handle.
///
/// TAGS: List, Delete, Back
///
#define ListDeleteLast(l) ListPopBack((l), NULL)

///
/// Delete the element at `idx`.
///
/// l[in,out] : List handle.
/// idx[in]   : Position in [0, length).
///
/// TAGS: List, Delete
///
#define ListDelete(l, idx) ListRemove((l), NULL, (idx))

///
/// Delete `count` elements starting at `start`.
///
/// l[in,out] : List handle.
/// start[in] : First deleted index.
/// count[in] : Number of elements to delete.
///
/// TAGS: List, Delete, Range
///
#define ListDeleteRange(l, start, count) ListRemoveRange((l), NULL, (start), (count))

#endif // MISRA_STD_CONTAINER_LIST_REMOVE_H
