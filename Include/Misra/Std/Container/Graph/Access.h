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
/// Number of live nodes currently stored in graph.
///
/// g[in] : Graph to query.
///
/// SUCCESS: Live node count.
/// FAILURE: Function cannot fail.
///
/// TAGS: Graph, Node, Count, Query
///
#define GraphNodeCount(g) ((g)->live_count)

///
/// Number of directed edges currently stored in graph.
///
/// g[in] : Graph to query.
///
/// SUCCESS: Edge count.
/// FAILURE: Function cannot fail.
///
/// TAGS: Graph, Edge, Count, Query
///
#define GraphEdgeCount(g) ((g)->edge_count)

///
/// Check whether graph contains no live nodes.
///
/// g[in] : Graph to query.
///
/// SUCCESS: `true` when graph has zero live nodes.
/// FAILURE: `false`
///
/// TAGS: Graph, Empty, Query
///
#define GraphEmpty(g) (GraphNodeCount(g) == 0)

///
/// Check whether graph currently contains the provided node id.
///
/// Marked nodes still count as present until `GraphCommitChanges` is called.
///
/// g[in]       : Graph to query.
/// node_id[in] : Node id to check.
///
/// SUCCESS: `true` when `node_id` currently refers to a live node.
/// FAILURE: `false`
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
/// SUCCESS: `GraphNode` handle for the requested node.
/// FAILURE: Does not return on invalid node id.
///
/// TAGS: Graph, Node, Handle, Access
///
#define GraphGetNode(g, node_id) graph_get_node(GENERIC_GRAPH(g), (node_id))

///
/// Get the stable node id carried by a traversal handle.
///
/// node[in] : `GraphNode` handle.
///
/// SUCCESS: Stable `GraphNodeId` for the handle.
/// FAILURE: Function cannot fail.
///
/// TAGS: Graph, Node, Id, Handle
///
#define GraphNodeGetId(node) ((node).__id)

///
/// Get the slot index encoded in a traversal handle.
///
/// node[in] : `GraphNode` handle.
///
/// SUCCESS: Slot index portion of the node id.
/// FAILURE: Function cannot fail.
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
/// TAGS: Graph, Node, Access
///
#define GraphNodeAt(g, node_id) (*(GRAPH_NODE_TYPE(g) *)graph_node_ptr_at(GENERIC_GRAPH(g), (node_id)))

///
/// Get pointer to node payload at given node id.
///
/// g[in,out]   : Graph to query.
/// node_id[in] : Node id to access.
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
/// TAGS: Graph, Node, Access, Handle
///
#define GraphNodeData(g, node) (*(GRAPH_NODE_TYPE(g) *)graph_node_data_ptr_checked(GENERIC_GRAPH(g), (node)))

///
/// Get payload pointer through a `GraphNode` traversal handle.
///
/// g[in,out] : Graph owning the node.
/// node[in]  : `GraphNode` handle to access.
///
/// TAGS: Graph, Node, Access, Pointer, Handle
///
#define GraphNodeDataPtr(g, node) ((GRAPH_NODE_TYPE(g) *)graph_node_data_ptr_checked(GENERIC_GRAPH(g), (node)))

///
/// Get pointer to outgoing neighbor list for a node.
///
/// g[in,out]   : Graph to query.
/// node_id[in] : Node id whose outgoing adjacency list is requested.
///
/// SUCCESS: Pointer to outgoing neighbor vector.
/// FAILURE: Does not return on invalid node id.
///
/// TAGS: Graph, Edge, Neighbor, Access, Pointer
///
#define GraphOutNeighborsPtr(g, node_id) ((GraphNeighbors *)graph_out_neighbors_ptr(GENERIC_GRAPH(g), (node_id)))

///
/// Number of outgoing neighbors for a node.
///
/// g[in]       : Graph to query.
/// node_id[in] : Node id whose out-degree is requested.
///
/// SUCCESS: Out-degree of node.
/// FAILURE: Does not return on invalid node id.
///
/// TAGS: Graph, Edge, Degree, Query
///
#define GraphOutDegree(g, node_id) graph_out_degree(GENERIC_GRAPH(g), (node_id))

///
/// Access outgoing neighbor id at given offset.
///
/// g[in]            : Graph to query.
/// node_id[in]      : Source node id.
/// neighbor_idx[in] : Index in outgoing neighbor list.
///
/// TAGS: Graph, Edge, Neighbor, Access
///
#define GraphNeighborAt(g, node_id, neighbor_idx) VecAt(GraphOutNeighborsPtr((g), (node_id)), (neighbor_idx))

///
/// Get pointer to outgoing neighbor id at given offset.
///
/// g[in,out]        : Graph to query.
/// node_id[in]      : Source node id.
/// neighbor_idx[in] : Index in outgoing neighbor list.
///
/// TAGS: Graph, Edge, Neighbor, Access, Pointer
///
#define GraphNeighborPtrAt(g, node_id, neighbor_idx) VecPtrAt(GraphOutNeighborsPtr((g), (node_id)), (neighbor_idx))

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
