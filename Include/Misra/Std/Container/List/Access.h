/// file      : std/container/list/access.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
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
#define ListLen(l) ((void)0, (l)->length)

///
/// Allocator backing the list's nodes.
///
/// l[in] : List to query.
///
/// TAGS: List, Access, Allocator
///
#define ListAllocator(l) ((void)0, (l)->allocator)

///
/// Deep-copy `init` callback wired into the list, or `NULL` if the
/// list was initialised without deep-copy semantics.
///
/// l[in] : List to query.
///
/// TAGS: List, Access, DeepCopy
///
#define ListCopyInit(l) ((void)0, (l)->copy_init)

///
/// Deep-copy `deinit` callback wired into the list, or `NULL` if the
/// list was initialised without deep-copy semantics.
///
/// l[in] : List to query.
///
/// TAGS: List, Access, DeepCopy
///
#define ListCopyDeinit(l) ((void)0, (l)->copy_deinit)

///
/// Direct O(1) reference to the head node, or `NULL` when the list is
/// empty. Unlike `ListNodeBegin` (which aborts on empty), this is safe
/// to call on an empty list and is meant for invariant checks like
/// `ListHead(&l) == NULL`.
///
/// l[in] : List to query.
///
/// SUCCESS : Returns the head-node pointer.
/// FAILURE : Returns `NULL` when the list is empty. The list is not
///           modified.
///
/// TAGS: List, Access, Head
///
#define ListHead(l) ((void)0, (l)->head)

///
/// Direct O(1) reference to the tail node, or `NULL` when the list is
/// empty. Unlike `ListNodeEnd` (which aborts on empty), this is safe
/// to call on an empty list.
///
/// l[in] : List to query.
///
/// SUCCESS : Returns the tail-node pointer.
/// FAILURE : Returns `NULL` when the list is empty. The list is not
///           modified.
///
/// TAGS: List, Access, Tail
///
#define ListTail(l) ((void)0, (l)->tail)

///
/// Check whether list has no items.
///
/// l[in] : List to query.
///
/// SUCCESS : Returns `true` when the list length is 0. The list is not
///           modified.
/// FAILURE : Returns `false` when the list contains at least one node.
///           The list is not modified.
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
/// TAGS: List, Access, API
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
/// FAILURE: Empty object.
///
/// TAGS: List, Access, API
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
/// FAILURE: Empty object.
///
/// TAGS: List, First, Access
///
#define ListFirst(l) ListAt((l), 0)

///
/// Value at last node in list
/// This is a more expensive call. Fetches pointer to data twice and then dereferences.
/// Better use ListPtrAt instead.
///
/// SUCCESS: Data in tail node in list.
/// FAILURE: Empty object.
///
/// TAGS: List, Last, Access
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
/// TAGS: List, Node, Access
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
/// TAGS: List, Iterator, Begin, Node
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
/// TAGS: List, Node, Iterator, End
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
/// TAGS: List, Node, Access
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
/// TAGS: List, First, Node, Access
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
/// TAGS: List, Last, Node, Access
///
#define ListNodeLast(l) ListNodeAt((l), (l)->length - 1)

///
/// Payload pointer carried by a list node. Bare field read; the caller
/// owns the null-check decision (use `ListNodeNext` / `ListNodePrev`
/// chasing if NULL-tolerant traversal is wanted).
///
/// node[in] : List node to query.
///
/// TAGS: List, Node, Access, Data
///
#define ListNodeData(node) ((void)0, (node)->data)

///
/// Get item after (next to) given list item
///
/// item[in] : List node to get next node of, in the list.
///
/// SUCCESS: Node next to given `item` in list.
/// FAILURE: `NULL`
///
/// TAGS: List, Node, Access
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
/// TAGS: List, Node, Access
///
#define ListNodePrev(item) ((TYPE_OF(item))((item) ? (item)->prev : NULL))

///
/// Get the list node at signed offset `ridx` from `base_node`.
/// Positive `ridx` walks forward through `next` pointers; negative walks
/// backward through `prev` pointers. `ridx == 0` returns `base_node`.
///
/// base_node[in] : Anchor node.
/// ridx[in]      : Signed relative offset.
///
/// SUCCESS : Returns the node at offset `ridx`.
/// FAILURE : Returns `NULL` when the offset walks past either end of
///           the list.
///
/// TAGS: List, Node, Access
///
#define ListNodeRelative(base_node, ridx) get_node_relative_to_list_node(GENERIC_LIST_NODE(base_node), (i64)(ridx))

#endif // MISRA_STD_CONTAINER_LIST_ACCESS_H
