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
/// SUCCESS: Updated scratch visit count.
/// FAILURE: Does not return on invalid node handle.
///
/// TAGS: Graph, Node, Visit, Mutation
///
#define GraphNodeVisit(node) graph_node_visit((node))

///
/// Reset the scratch visit count of a node to zero.
///
/// node[in] : `GraphNode` handle to clear.
///
/// SUCCESS: Graph node scratch visit count becomes zero.
/// FAILURE: Does not return on invalid node handle.
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
/// SUCCESS: `true` when the node was newly marked.
/// FAILURE: `false` when the node was already marked.
///
/// TAGS: Graph, Node, Delete, Mark
///
#define GraphMarkNodeForDeletion(node) graph_mark_node_for_deletion((node))

///
/// Check whether a node is currently marked for deletion.
///
/// node[in] : `GraphNode` handle to query.
///
/// SUCCESS: `true` when the node is marked for deletion.
/// FAILURE: `false`
///
/// TAGS: Graph, Node, Delete, Query
///
#define GraphNodeMarkedForDeletion(node) graph_node_marked_for_deletion((node))

///
/// Remove a pending node-deletion mark before commit.
///
/// node[in] : `GraphNode` handle to unmark.
///
/// SUCCESS: `true` when a deletion mark was removed.
/// FAILURE: `false` when the node was not marked.
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
/// SUCCESS: `true` when the edge was newly marked.
/// FAILURE: `false` when the edge is absent or was already marked.
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
/// SUCCESS: `true` when the edge is pending removal.
/// FAILURE: `false`
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
/// SUCCESS: `true` when an edge-removal mark was removed.
/// FAILURE: `false` when the edge was not marked.
///
/// TAGS: Graph, Edge, Delete, Unmark
///
#define GraphUnmarkEdgeForRemoval(g, from, to) graph_unmark_edge_for_removal(GENERIC_GRAPH(g), (from), (to))

///
/// Apply all pending edge removals and node deletion marks.
///
/// Deleted slots remain reusable and future nodes may reuse their slot indices with
/// fresh generations.
///
/// g[in,out] : Graph to commit.
///
/// SUCCESS: Number of pending removals applied by this commit.
///          Explicit edge removals and node deletion marks are counted once each.
///          Incident edge cleanup caused by node deletion is not counted separately.
/// FAILURE: Does not return on invalid graph.
///
/// TAGS: Graph, Node, Delete, Commit
///
#define GraphCommitChanges(g) graph_commit_changes(GENERIC_GRAPH(g), sizeof(GRAPH_NODE_TYPE(g)))

#endif // MISRA_STD_CONTAINER_GRAPH_OPS_H
