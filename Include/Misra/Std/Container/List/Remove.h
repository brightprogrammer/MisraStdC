/// file      : std/container/list/remove.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
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
///             discard it (the configured `copy_deinit` is invoked instead).
/// idx[in]   : Position in [0, length).
///
/// SUCCESS : Returns to the caller. The node at position `idx` is unlinked
///           and its allocator-owned storage freed; list length shrinks by
///           one. When `val` is non-NULL, the removed value is memcopied
///           into `*val` and ownership transfers to the caller. When
///           `val` is NULL and `copy_deinit` is configured, the handler is
///           invoked on the removed element.
/// FAILURE : Function cannot fail. An out-of-range `idx` is a caller bug
///           and aborts via `LOG_FATAL`.
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
/// SUCCESS : Returns to the caller. The head node is unlinked, its
///           allocator-owned storage freed; list length shrinks by one,
///           the next node becomes the new head. When `val` is non-NULL
///           the removed value is memcopied into `*val`.
/// FAILURE : Function cannot fail. Calling on an empty list is a caller
///           bug and aborts via `LOG_FATAL`.
///
/// TAGS: List, Remove, Pop, Front
///
#define ListPopFront(l, val) ListRemove((l), (val), 0)

///
/// Remove the last element of the list and optionally store its value.
///
/// l[in,out] : List handle.
/// val[out]  : Optional destination for the popped element.
///
/// SUCCESS : Returns to the caller. The tail node is unlinked, its
///           allocator-owned storage freed; list length shrinks by one,
///           the previous node becomes the new tail. When `val` is
///           non-NULL the removed value is memcopied into `*val`.
/// FAILURE : Function cannot fail. Calling on an empty list is a caller
///           bug and aborts via `LOG_FATAL`.
///
/// TAGS: List, Remove, Pop, Back
///
#define ListPopBack(l, val) ListRemove((l), (val), (l)->length - 1)

///
/// Remove `count` elements starting at `start` and optionally copy them out.
///
/// l[in,out] : List handle.
/// rd[out]   : Optional destination buffer of at least `count` slots. Pass
///             `NULL` to discard the removed elements (the configured
///             `copy_deinit` is invoked instead).
/// start[in] : First removed index.
/// count[in] : Number of elements to remove.
///
/// SUCCESS : Returns to the caller. `count` nodes starting at `start` are
///           unlinked and their storage freed; list length shrinks by
///           `count`. When `rd` is non-NULL the removed values are
///           memcopied into `*rd` in order. When `rd` is NULL and
///           `copy_deinit` is configured, the handler is invoked on each
///           removed element.
/// FAILURE : Function cannot fail. `start + count` exceeding `length` is
///           a caller bug and aborts via `LOG_FATAL`.
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
/// SUCCESS : Returns to the caller. The tail node is unlinked and freed;
///           list length shrinks by one. When `copy_deinit` is configured
///           it is invoked on the dropped element.
/// FAILURE : Function cannot fail. Calling on an empty list is a caller
///           bug and aborts via `LOG_FATAL`.
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
/// SUCCESS : Returns to the caller. The node at `idx` is unlinked and
///           freed; list length shrinks by one. When `copy_deinit` is
///           configured it is invoked on the dropped element.
/// FAILURE : Function cannot fail. An out-of-range `idx` is a caller bug
///           and aborts via `LOG_FATAL`.
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
/// SUCCESS : Returns to the caller. `count` nodes starting at `start` are
///           unlinked and freed; list length shrinks by `count`. When
///           `copy_deinit` is configured it is invoked on each dropped
///           element.
/// FAILURE : Function cannot fail. `start + count` exceeding `length` is
///           a caller bug and aborts via `LOG_FATAL`.
///
/// TAGS: List, Delete, Range
///
#define ListDeleteRange(l, start, count) ListRemoveRange((l), NULL, (start), (count))

#endif // MISRA_STD_CONTAINER_LIST_REMOVE_H
