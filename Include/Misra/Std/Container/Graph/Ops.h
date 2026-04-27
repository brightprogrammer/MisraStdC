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
/// Apply all pending deletion marks and remove incident edges.
///
/// Deleted slots remain reusable and future nodes may reuse their slot indices with
/// fresh generations.
///
/// g[in,out] : Graph to commit.
///
/// SUCCESS: Number of nodes deleted by this commit.
/// FAILURE: Does not return on invalid graph.
///
/// TAGS: Graph, Node, Delete, Commit
///
#define GraphCommitChanges(g) graph_commit_changes(GENERIC_GRAPH(g), sizeof(GRAPH_NODE_TYPE(g)))

#endif // MISRA_STD_CONTAINER_GRAPH_OPS_H
