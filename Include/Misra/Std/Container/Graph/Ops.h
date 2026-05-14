/// file      : std/container/graph/ops.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Operational helpers for Graph.

#ifndef MISRA_STD_CONTAINER_GRAPH_OPS_H
#define MISRA_STD_CONTAINER_GRAPH_OPS_H

#include "Access.h"

///
/// Increment the scratch visit count of a node.
///
/// node[in] : `GraphNode` handle to mark as visited.
///
/// SUCCESS : Returns the new visit count for the node (post-increment).
///           Only the scratch counter on the referenced slot is modified;
///           graph structure (live_count, edges, generations) is untouched.
/// FAILURE : Does not return - aborts via `LOG_FATAL` for an invalid or
///           stale node handle (caller bug).
///
/// NOTE: This is intentionally simple shared scratch state. Use external `Vec`, `Map`,
///       or domain-specific side tables when an algorithm needs more than one counter or bit.
///
/// TAGS: Graph, Node, Visit, Mutation
///
#define GraphNodeVisit(node) graph_node_visit((node))

///
/// Reset the scratch visit count of a node to zero.
///
/// node[in] : `GraphNode` handle to clear.
///
/// SUCCESS : Returns to the caller. The referenced slot's scratch visit
///           counter is now 0. Graph structure is untouched.
/// FAILURE : Does not return - aborts via `LOG_FATAL` for an invalid or
///           stale node handle (caller bug).
///
/// TAGS: Graph, Node, Visit, Reset
///
#define GraphNodeUnvisit(node) graph_node_unvisit((node))

///
/// Mark a node for deletion on the next `GraphCommitChanges`.
///
/// Marked nodes remain visible until commit. This operation is safe during graph
/// traversal and is intended for destructive passes that need stable iteration.
///
/// node[in] : `GraphNode` handle to mark.
///
/// SUCCESS : Returns `true`. The slot referenced by `node` is now flagged
///           as marked; the graph's pending-delete count grows by one.
///           `live_count` is unchanged - the node is still observable
///           through traversal and lookup until `GraphCommitChanges` runs.
/// FAILURE : Returns `false` when the node was already marked. The graph
///           is not modified.
///
/// TAGS: Graph, Node, Delete, Mark
///
#define GraphMarkNodeForDeletion(node) graph_mark_node_for_deletion((node))

///
/// Check whether a node is currently marked for deletion.
///
/// node[in] : `GraphNode` handle to query.
///
/// SUCCESS : Returns `true` when the slot's deletion mark is set. The graph
///           is not modified.
/// FAILURE : Returns `false` when the node is not marked. The graph is not
///           modified.
///
/// TAGS: Graph, Node, Delete, Query
///
#define GraphNodeMarkedForDeletion(node) graph_node_marked_for_deletion((node))

///
/// Remove a pending node-deletion mark before commit.
///
/// node[in] : `GraphNode` handle to unmark.
///
/// SUCCESS : Returns `true`. The deletion mark on the referenced slot has
///           been cleared; the graph's pending-delete count shrinks by one.
///           `live_count` is unchanged.
/// FAILURE : Returns `false` when the node was not marked. The graph is
///           not modified.
///
/// TAGS: Graph, Node, Delete, Unmark
///
#define GraphUnmarkNodeForDeletion(node) graph_unmark_node_for_deletion((node))

///
/// Mark a directed edge for removal on the next `GraphCommitChanges`.
///
/// This operation is safe during traversal and does not structurally mutate the graph
/// until commit.
///
/// g[in,out] : Graph owning the edge.
/// from[in]  : Source node id.
/// to[in]    : Destination node id.
///
/// SUCCESS : Returns `true`. The edge entry in the outgoing-neighbour list
///           of `from` (and matching predecessor entry on `to`) is flagged
///           as marked. `edge_count` is unchanged - the edge is still
///           observable through traversal until `GraphCommitChanges` runs.
/// FAILURE : Returns `false` when the edge is absent or was already
///           marked. The graph is not modified.
///
/// TAGS: Graph, Edge, Delete, Mark
///
#define GraphMarkEdgeForRemoval(g, from, to) graph_mark_edge_for_removal(GENERIC_GRAPH(g), (from), (to))

///
/// Check whether an edge is currently marked for removal.
///
/// g[in]    : Graph owning the edge.
/// from[in] : Source node id.
/// to[in]   : Destination node id.
///
/// SUCCESS : Returns `true` when the edge is flagged for removal but not
///           yet committed. The graph is not modified.
/// FAILURE : Returns `false` when the edge is absent or not marked. The
///           graph is not modified.
///
/// TAGS: Graph, Edge, Delete, Query
///
#define GraphEdgeMarkedForRemoval(g, from, to) graph_edge_marked_for_removal(GENERIC_GRAPH(g), (from), (to))

///
/// Remove a pending edge-removal mark before commit.
///
/// g[in,out] : Graph owning the edge.
/// from[in]  : Source node id.
/// to[in]    : Destination node id.
///
/// SUCCESS : Returns `true`. The removal mark on the matching edge entry
///           has been cleared. `edge_count` is unchanged.
/// FAILURE : Returns `false` when the edge was not marked. The graph is
///           not modified.
///
/// TAGS: Graph, Edge, Delete, Unmark
///
#define GraphUnmarkEdgeForRemoval(g, from, to) graph_unmark_edge_for_removal(GENERIC_GRAPH(g), (from), (to))

///
/// Apply all pending edge removals and node deletion marks.
///
/// Deleted slots remain reusable and future nodes may reuse their slot indices with
/// fresh generations.
/// Any deleted node id or `GraphNode` handle becomes invalid immediately after commit returns.
///
/// g[in,out] : Graph to commit.
///
/// SUCCESS : Returns the count of pending removals applied (explicit edge
///           removals plus node deletion marks; cascading edge cleanup
///           from node deletion is not counted separately). `live_count`
///           shrinks by the number of deleted nodes; `edge_count` shrinks
///           accordingly; pending-delete count drops to 0. Freed slot
///           indices are pushed onto the reuse list with incremented
///           generations; previously-stored payloads have been torn down
///           via `copy_deinit` (if configured). All previously-issued
///           node ids and `GraphNode` handles for the deleted nodes are
///           now stale.
/// FAILURE : Does not return - aborts via `LOG_FATAL` for an invalid graph
///           (caller bug).
///
/// INFO: This deferred mutation model is meant for passes that need stable traversal first
///       and destructive graph rewrites second.
///
/// TAGS: Graph, Node, Delete, Commit
///
#define GraphCommitChanges(g) graph_commit_changes(GENERIC_GRAPH(g), sizeof(GRAPH_NODE_TYPE(g)))

#endif // MISRA_STD_CONTAINER_GRAPH_OPS_H
