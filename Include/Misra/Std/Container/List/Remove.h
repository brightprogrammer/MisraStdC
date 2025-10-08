/// file      : Remove.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// copyright : Copyright (c) 2025, Siddharth Mishra, All rights reserved.
///
/// List remove helpers
///

#ifndef MISRA_STD_CONTAINER_LIST_REMOVE_H
#define MISRA_STD_CONTAINER_LIST_REMOVE_H

///
/// Remove item from list at given index and store in given pointer.
/// Order of elements is guaranteed to be preserved.
///
/// l[in,out] : List to remove item from.
/// val[out]  : Where removed item will be stored. If not provided then it's equivalent to
///             deleting the item at specified index.
/// idx[in]   : Index in list to remove item from.
///
/// SUCCESS : Returns `v` on success.
/// FAILURE : Returns NULL otherwise.
///
#define ListRemove(l, val, idx)                                                                                        \
    remove_range_list(GENERIC_LIST(l), (val), sizeof(LIST_DATA_TYPE(l)), ((l)->length - 1), 1)

///
/// Remove item from the very beginning of list.
///
/// l[in]   : List to push item into
/// val[in] : Pointer to value to be prepended
///
/// SUCCESS : Returns `v` the list itself on success.
/// FAILURE : Returns `NULL` otherwise.
///
#define ListPopFront(l, val) ListRemove((l), (val), 0);

///
/// Pop item from list back.
///
/// l[in,out]  : List to pop item from.
/// val[out]   : Popped item will be stored here. Make sure this has sufficient memory
///              to store memcopied data. If no pointer is provided, then it's equivalent
///              to deleting item from last position.
///
/// SUCCESS : Returns `v` on success
/// FAILURE : Returns NULL otherwise.
///
#define ListPopBack(l, val) ListRemove((l), (val), ((l) ? (l)->length - 1 : (size_t)-1));

///
/// Remove data from list in given range [start, start + count)
/// Order of elements is guaranteed to be preserved.
///
/// l[in,out] : List to remove item from.
/// rd[out]   : Where removed data will be stored. If not provided then it's equivalent to
///             deleting the items in specified range.
/// start[in] : Index in list to removing items from.
/// count[in] : Number of items from starting index.
///
/// SUCCESS : Returns `v` on success.
/// FAILURE : Returns NULL otherwise.
///
#define ListRemoveRange(l, rd, start, count)                                                                           \
    remove_range_list(GENERIC_LIST(l), (rd), sizeof(LIST_DATA_TYPE(l)), (start), (count))

///
/// Delete last item from list
///
#define ListDeleteLast(l) ListPop((l), NULL)

///
/// Delete item at given index
///
#define ListDelete(l, idx) ListRemove((l), NULL, (idx))

///
/// Delete items in given range [start, start + count)
///
#define ListDeleteRange(l, start, count) ListRemoveRange((l), NULL, (start), (count))

#endif // MISRA_STD_CONTAINER_LIST_REMOVE_H
