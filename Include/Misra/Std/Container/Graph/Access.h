/// file      : std/container/graph/access.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Access helpers for Graph.

#ifndef MISRA_STD_CONTAINER_GRAPH_ACCESS_H
#define MISRA_STD_CONTAINER_GRAPH_ACCESS_H

#include "Type.h"
#include "Private.h"

///
/// Number of live nodes currently stored in graph. Includes nodes marked
/// for deletion but not yet committed.
///
/// g[in] : Graph to query.
///
/// SUCCESS : Returns the live-node count as a `u64`. The graph is not
///           modified.
/// FAILURE : Function cannot fail.
///
/// TAGS: Graph, Node, Count, Query
///
#define GraphNodeCount(g) ((void)0, (g)->live_count)

///
/// Number of directed edges currently stored in graph. Includes edges
/// marked for removal but not yet committed.
///
/// g[in] : Graph to query.
///
/// SUCCESS : Returns the edge count as a `u64`. The graph is not modified.
/// FAILURE : Function cannot fail.
///
/// TAGS: Graph, Edge, Count, Query
///
#define GraphEdgeCount(g) ((void)0, (g)->edge_count)

///
/// Structural-mutation counter. Bumped by node/edge insertions, deletions,
/// and commits; traversal helpers snapshot it to detect concurrent mutation.
///
/// g[in] : Graph to query.
///
/// TAGS: Graph, Access, Epoch, Mutation
///
#define GraphMutationEpoch(g) ((void)0, (g)->mutation_epoch)

///
/// Allocator backing the graph's slots, adjacency vectors, and node payloads.
///
/// g[in] : Graph to query.
///
/// TAGS: Graph, Access, Allocator
///
#define GraphAllocator(g) ((void)0, (g)->allocator)

///
/// Deep-copy `init` callback wired into the graph for node payloads, or
/// `NULL` if the graph was initialised without deep-copy semantics.
///
/// g[in] : Graph to query.
///
/// TAGS: Graph, Access, DeepCopy
///
#define GraphCopyInit(g) ((void)0, (g)->copy_init)

///
/// Deep-copy `deinit` callback wired into the graph for node payloads, or
/// `NULL` if the graph was initialised without deep-copy semantics.
///
/// g[in] : Graph to query.
///
/// TAGS: Graph, Access, DeepCopy
///
#define GraphCopyDeinit(g) ((void)0, (g)->copy_deinit)

///
/// Check whether graph contains no live nodes.
///
/// g[in] : Graph to query.
///
/// SUCCESS : Returns `true` when `live_count == 0`. The graph is not
///           modified.
/// FAILURE : Returns `false` when the graph holds at least one live node.
///
/// TAGS: Graph, Empty, Query
///
#define GraphEmpty(g) (GraphNodeCount(g) == 0)

///
/// Check whether graph currently contains the provided node id.
///
/// Marked nodes still count as present until `GraphCommitChanges` is called.
/// This is the safe probe to use before deciding whether an old stored id is still live.
///
/// g[in]       : Graph to query.
/// node_id[in] : Node id to check.
///
/// SUCCESS : Returns `true` when `node_id` currently refers to a live node
///           (slot occupied, generation matching, and not yet committed
///           for deletion). The graph is not modified.
/// FAILURE : Returns `false` when the id refers to a freed or stale slot
///           (mismatched generation). The graph is not modified.
///
/// TAGS: Graph, Node, Contains, Query
///
#define GraphContainsNode(g, node_id) graph_contains_node(GENERIC_GRAPH(g), (node_id))

///
/// Get a traversal handle for a live node id.
///
/// g[in,out]   : Graph owning the node.
/// node_id[in] : Live node id to wrap as a `GraphNode`.
///
/// SUCCESS : Returns a `GraphNode` handle that resolves back to the same
///           slot and generation. The graph is not modified. The handle is
///           valid until the slot is committed for deletion.
/// FAILURE : Does not return - aborts via `LOG_FATAL` for an invalid or
///           stale node id (caller bug).
///
/// TAGS: Graph, Node, Handle, Access
///
#define GraphGetNode(g, node_id) graph_get_node(GENERIC_GRAPH(g), (node_id))

///
/// Get the stable node id carried by a traversal handle.
///
/// node[in] : `GraphNode` handle.
///
/// SUCCESS : Returns the stable `GraphNodeId` (slot index + generation).
///           The handle is not modified.
/// FAILURE : Function cannot fail.
///
/// TAGS: Graph, Node, Id, Handle
///
#define GraphNodeGetId(node) ((void)0, (node).__id)

///
/// Get the slot index encoded in a traversal handle.
///
/// node[in] : `GraphNode` handle.
///
/// SUCCESS : Returns the slot-index portion (without generation bits) of
///           the node id, useful as a key into external storage. The
///           handle is not modified.
/// FAILURE : Function cannot fail.
///
/// WARN: Slot indices are intentionally reusable after `GraphCommitChanges`.
///       External arrays keyed only by `GraphNodeIndex(node)` must be reset when
///       deleted slots can be reused, or paired with generation-aware logic.
///
/// TAGS: Graph, Node, Index, Handle
///
#define GraphNodeIndex(node) GraphNodeIdIndex(GraphNodeGetId(node))

///
/// Access node payload at given node id.
///
/// g[in]       : Graph to query.
/// node_id[in] : Node id to access.
///
/// SUCCESS : Returns the payload as a value of type `GRAPH_NODE_TYPE(g)`.
///           The graph is not modified.
/// FAILURE : Does not return - aborts via `LOG_FATAL` for an invalid or
///           stale node id (caller bug).
///
/// TAGS: Graph, Node, Access
///
#define GraphNodeAt(g, node_id) (*(GRAPH_NODE_TYPE(g) *)graph_node_ptr_at(GENERIC_GRAPH(g), (node_id)))

///
/// Get pointer to node payload at given node id.
///
/// g[in,out]   : Graph to query.
/// node_id[in] : Node id to access.
///
/// SUCCESS : Returns a pointer of type `GRAPH_NODE_TYPE(g) *` to the
///           payload slot. The graph is not modified. The pointer is
///           valid until the slot is committed for deletion.
/// FAILURE : Does not return - aborts via `LOG_FATAL` for an invalid or
///           stale node id (caller bug).
///
/// TAGS: Graph, Node, Access, Pointer
///
#define GraphNodePtrAt(g, node_id) ((GRAPH_NODE_TYPE(g) *)graph_node_ptr_at(GENERIC_GRAPH(g), (node_id)))

///
/// Access payload through a `GraphNode` traversal handle.
///
/// g[in]       : Graph owning the node.
/// node[in]    : `GraphNode` handle to access.
///
/// SUCCESS : Returns the payload as a value of type `GRAPH_NODE_TYPE(g)`.
///           The graph is not modified. The handle is also validated
///           against `g` so a handle from a different graph aborts.
/// FAILURE : Does not return - aborts via `LOG_FATAL` for an invalid
///           handle or graph/handle mismatch (caller bug).
///
/// TAGS: Graph, Node, Access, Handle
///
#define GraphNodeData(g, node) (*(GRAPH_NODE_TYPE(g) *)graph_node_data_ptr_checked(GENERIC_GRAPH(g), (node)))

///
/// Get payload pointer through a `GraphNode` traversal handle.
///
/// g[in,out] : Graph owning the node.
/// node[in]  : `GraphNode` handle to access.
///
/// SUCCESS : Returns a pointer of type `GRAPH_NODE_TYPE(g) *` to the
///           payload slot referenced by the handle. The graph is not
///           modified. The pointer is valid until the slot is committed
///           for deletion.
/// FAILURE : Does not return - aborts via `LOG_FATAL` for an invalid
///           handle or graph/handle mismatch (caller bug).
///
/// TAGS: Graph, Node, Access, Pointer, Handle
///
#define GraphNodeDataPtr(g, node) ((GRAPH_NODE_TYPE(g) *)graph_node_data_ptr_checked(GENERIC_GRAPH(g), (node)))

///
/// Number of outgoing neighbors for a node. Includes edges marked for
/// removal but not yet committed.
///
/// g[in]       : Graph to query.
/// node_id[in] : Node id whose out-degree is requested.
///
/// SUCCESS : Returns the out-degree as a `u64`. The graph is not modified.
/// FAILURE : Does not return - aborts via `LOG_FATAL` for an invalid or
///           stale node id (caller bug).
///
/// TAGS: Graph, Edge, Degree, Query
///
#define GraphOutDegree(g, node_id) graph_out_degree(GENERIC_GRAPH(g), (node_id))

///
/// Number of incoming neighbors for a node. Includes edges marked for
/// removal but not yet committed.
///
/// g[in]       : Graph to query.
/// node_id[in] : Node id whose in-degree is requested.
///
/// SUCCESS : Returns the in-degree as a `u64`. The graph is not modified.
/// FAILURE : Does not return - aborts via `LOG_FATAL` for an invalid or
///           stale node id (caller bug).
///
/// TAGS: Graph, Edge, Degree, Query, Incoming
///
#define GraphInDegree(g, node_id) graph_in_degree(GENERIC_GRAPH(g), (node_id))

///
/// Access outgoing neighbor id at given offset.
///
/// g[in]            : Graph to query.
/// node_id[in]      : Source node id.
/// neighbor_idx[in] : Index in outgoing neighbor list, [0, out_degree).
///
/// SUCCESS : Returns the `GraphNodeId` at the requested position in the
///           outgoing adjacency list. The graph is not modified.
/// FAILURE : Does not return - aborts via `LOG_FATAL` for an invalid or
///           stale `node_id` or out-of-bounds `neighbor_idx`.
///
/// TAGS: Graph, Edge, Neighbor, Access
///
#define GraphNeighborAt(g, node_id, neighbor_idx) graph_neighbor_at(GENERIC_GRAPH(g), (node_id), (neighbor_idx))

///
/// Access incoming predecessor id at given offset.
///
/// g[in]               : Graph to query.
/// node_id[in]         : Destination node id.
/// predecessor_idx[in] : Index in incoming predecessor list, [0, in_degree).
///
/// SUCCESS : Returns the `GraphNodeId` at the requested position in the
///           reverse predecessor list. The graph is not modified.
/// FAILURE : Does not return - aborts via `LOG_FATAL` for an invalid or
///           stale `node_id` or out-of-bounds `predecessor_idx`.
///
/// TAGS: Graph, Edge, Predecessor, Access
///
#define GraphPredecessorAt(g, node_id, predecessor_idx)                                                                \
    graph_predecessor_at(GENERIC_GRAPH(g), (node_id), (predecessor_idx))

///
/// Check whether graph contains directed edge `from -> to`.
///
/// g[in]    : Graph to query.
/// from[in] : Source node id.
/// to[in]   : Destination node id.
///
/// SUCCESS: `true` when the directed edge exists.
/// FAILURE: `false`
///
/// WARN: `from` and `to` must both be live node ids. This query treats stale ids
///       as programmer error and aborts instead of quietly collapsing them to "not found".
///
/// TAGS: Graph, Edge, Query, Search
///
#define GraphHasEdge(g, from, to) graph_has_edge(GENERIC_GRAPH(g), (from), (to))

///
/// Get the current scratch visit count for a node handle.
///
/// node[in] : `GraphNode` handle to query.
///
/// SUCCESS: Current scratch visit count for the node.
/// FAILURE: Does not return on invalid node handle.
///
/// NOTE: This count is graph-owned scratch state. It is convenient for simple traversals,
///       but it is not a substitute for richer application-owned side tables.
///
/// TAGS: Graph, Node, Visit, Count, Query
///
#define GraphNodeVisitCount(node) graph_node_visit_count((node))

///
/// Check whether a node has been visited at least once.
///
/// node[in] : `GraphNode` handle to query.
///
/// SUCCESS: `true` when `GraphNodeVisitCount(node) > 0`.
/// FAILURE: `false`
///
/// TAGS: Graph, Node, Visit, Query
///
#define GraphNodeVisited(node) graph_node_visited((node))

#endif // MISRA_STD_CONTAINER_GRAPH_ACCESS_H
