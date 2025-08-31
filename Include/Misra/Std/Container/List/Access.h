/// file      : Access.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// copyright : Copyright (c) 2025, Siddharth Mishra, All rights reserved.
///
/// Different list accessor helper macros.
///

#ifndef MISRA_STD_CONTAINER_LIST_OPS_H
#define MISRA_STD_CONTAINER_LIST_OPS_H

///
/// Swap items at given indices.
///
/// l[in,out] : List to swap items in.
/// idx1[in]  : Index/Position of first item.
/// idx1[in]  : Index/Position of second item.
///
/// SUCCESS: `v` on success
/// FAILURE: NULL
///
#define ListSwapItems(l, idx1, idx2) ((TYPE_OF(l))swap_list(GENERIC_LIST(l), sizeof(LIST_DATA_TYPE(l)), (idx1), (idx2)))

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

///
/// Data in node at given index in given list
///
/// l[in]   : List to get data from.
/// idx[in] : Index to get data at.
///
/// SUCCESS: Data from node in list at given index.
/// FAILURE: Emtpy object.
///
#define ListAt(l, idx) (ListPtrAt((l), (idx)) ? *ListPtrAt((l), (idx)) : (LIST_DATA_TYPE(l) {0}))

///
/// Value at first node in list
///
/// SUCCESS: Data in head node in list.
/// FAILURE: Emtpy object.
///
#define ListFirst(l) ListAt((l), 0)

///
/// Value at last node in list
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
#define ListNodeAt(l, idx) *((LIST_NODE_TYPE(l) *)(node_at_list(GENERIC_LIST(l), sizeof(LIST_DATA_TYPE(l)), (idx))))

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
#define ListNodeLast(l)  ListNodeAt((l), (l)->length - 1)

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

#endif // MISRA_STD_CONTAINER_LIST_OPS_H
