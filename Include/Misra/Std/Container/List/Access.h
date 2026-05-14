/// file      : Access.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// copyright : Copyright (c) 2025, Siddharth Mishra, All rights reserved.
///
/// Different list accessor helper macros.
///

#ifndef MISRA_STD_CONTAINER_LIST_ACCESS_H
#define MISRA_STD_CONTAINER_LIST_ACCESS_H

///
/// Number of items in list.
///
/// l[in] : List to query.
///
/// SUCCESS: Length of list.
/// FAILURE: Function cannot fail.
///
/// TAGS: List, Length, Query
///
#define ListLen(l) ((l)->length)

///
/// Check whether list has no items.
///
/// l[in] : List to query.
///
/// SUCCESS: `true` when list length is 0.
/// FAILURE: `false`
///
/// TAGS: List, Empty, Query
///
#define ListEmpty(l) (ListLen(l) == 0)

///
/// Swap the payloads of two list nodes in place.
///
/// l[in,out] : List handle.
/// idx1[in]  : First index in [0, length).
/// idx2[in]  : Second index in [0, length).
///
/// SUCCESS : Returns to the caller. The data payloads at `idx1` and `idx2`
///           have been exchanged byte-for-byte. Node identity and node
///           order are unchanged - only the values they carry move.
/// FAILURE : Function cannot fail. Either index being out of range is a
///           caller bug and aborts via `LOG_FATAL`.
///
/// TAGS: List, Access, Swap
///
#define ListSwapItems(l, idx1, idx2) swap_list(GENERIC_LIST(l), sizeof(LIST_DATA_TYPE(l)), (idx1), (idx2))

///
/// Pointer to data in node at given index in given list
///
/// l[in]   : List to get data from.
/// idx[in] : Index to get data at.
///
/// SUCCESS: Pointer to data from node in list at given index
/// FAILURE: `NULL`
///
#define ListPtrAt(l, idx) ((LIST_DATA_TYPE(l) *)item_ptr_at_list(GENERIC_LIST(l), sizeof(LIST_DATA_TYPE(l)), (idx)))

#ifdef __cplusplus
#    define ListAt(l, idx) (ListPtrAt((l), (idx)) ? *ListPtrAt((l), (idx)) : (LIST_DATA_TYPE(l) {0}))
#else
///
/// Data in node at given index in given list
/// This is a more expensive call. Fetches pointer to data twice and then dereferences.
/// Better use ListPtrAt instead.
///
/// l[in]   : List to get data from.
/// idx[in] : Index to get data at.
///
/// SUCCESS: Data from node in list at given index.
/// FAILURE: Emtpy object.
///
#    define ListAt(l, idx) (ListPtrAt((l), (idx)) ? *ListPtrAt((l), (idx)) : ((LIST_DATA_TYPE(l)) {0}))
#endif

///
/// Find the first item equal to the searched value.
///
/// NOTE: `item_ptr` must point to a value comparable with list elements.
///       Use `&LVAL(expr)` when searching with a temporary expression.
///
/// l[in]        : List to search.
/// item_ptr[in] : Pointer to searched value.
/// compare[in]  : Comparator returning `0` for equality.
///
/// SUCCESS: Index of first matching item.
/// FAILURE: `SIZE_MAX`
///
/// TAGS: List, Find, Search, Compare
///
#define ListFind(l, item_ptr, compare) find_idx_list(GENERIC_LIST(l), (item_ptr), sizeof(LIST_DATA_TYPE(l)), (compare))

///
/// Check whether list contains a matching item.
///
/// l[in]        : List to search.
/// item_ptr[in] : Pointer to searched value.
/// compare[in]  : Comparator returning `0` for equality.
///
/// SUCCESS: `true` when a matching item exists.
/// FAILURE: `false`
///
/// TAGS: List, Contains, Search, Compare
///
#define ListContains(l, item_ptr, compare) (ListFind((l), (item_ptr), (compare)) != SIZE_MAX)

///
/// Value at first node in list
/// This is a more expensive call. Fetches pointer to data twice and then dereferences.
/// Better use ListPtrAt instead.
///
/// SUCCESS: Data in head node in list.
/// FAILURE: Emtpy object.
///
#define ListFirst(l) ListAt((l), 0)

///
/// Value at last node in list
/// This is a more expensive call. Fetches pointer to data twice and then dereferences.
/// Better use ListPtrAt instead.
///
/// SUCCESS: Data in tail node in list.
/// FAILURE: Emtpy object.
///
#define ListLast(l) ListAt((l), (l)->length - 1)

///
/// Reference to node at given index in list
///
/// l[in]   : List to get node from
/// idx[in] : Index to fetch node at.
///
/// SUCCESS: Reference to node in given list at given index.
/// FAILURE: `NULL`
///
#define ListNodePtrAt(l, idx) ((LIST_NODE_TYPE(l) *)(node_at_list(GENERIC_LIST(l), sizeof(LIST_DATA_TYPE(l)), (idx))))

///
/// Reference to head node in list
///
/// l[in] : List to get node from
///
/// SUCCESS: Reference to head node.
/// FAILURE: `NULL`
///
#define ListNodeBegin(l) ListNodePtrAt((l), 0)

///
/// Reference to tail node in list
///
/// l[in] : List to get node from
///
/// SUCCESS: Reference to head node.
/// FAILURE: `NULL`
///
#define ListNodeEnd(l) ListNodePtrAt((l), (l)->length - 1)

///
/// Node at given index in list
///
/// l[in]   : List to get node from
/// idx[in] : Index to fetch node at.
///
/// SUCCESS: Node in given list at given index.
/// FAILURE: Empty node struct.
///
#define ListNodeAt(l, idx) (*((LIST_NODE_TYPE(l) *)(node_at_list(GENERIC_LIST(l), sizeof(LIST_DATA_TYPE(l)), (idx)))))

///
/// Head node in list.
///
/// l[in] : List to get node from
///
/// SUCCESS: Head node.
/// FAILURE: Empty node struct.
///
#define ListNodeFirst(l) ListNodeAt((l), 0)

///
/// Tail node in list.
///
/// l[in] : List to get node from
///
/// SUCCESS: Tail node.
/// FAILURE: Empty node struct.
///
#define ListNodeLast(l) ListNodeAt((l), (l)->length - 1)

///
/// Get item after (next to) given list item
///
/// item[in] : List node to get next node of, in the list.
///
/// SUCCESS: Node next to given `item` in list.
/// FAILURE: `NULL`
///
#define ListNodeNext(item) ((TYPE_OF(item))((item) ? (item)->next : NULL))

///
/// Get item before (prev to) given list item
///
/// item[in] : List node to get previous node of, in the list.
///
/// SUCCESS: Node before given `item` in list.
/// FAILURE: `NULL`
///
#define ListNodePrev(item) ((TYPE_OF(item))((item) ? (item)->prev : NULL))

///
/// Get item relative to given node.
///
/// item[in] : List node to get previous node of, in the list.
/// ridx[in] : Relative index +ve or -ve.
///
/// If relative index exceeds the bounds of list, then NULL is returned.
///
/// SUCCESS: Node relative to given `item` in list.
/// FAILURE: `NULL` or abort
///
#define ListNodeRelative(base_node, ridx) get_node_relative_to_list_node(GENERIC_LIST_NODE(base_node), (i64)(ridx))

#endif // MISRA_STD_CONTAINER_LIST_ACCESS_H
