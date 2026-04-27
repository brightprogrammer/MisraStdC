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
/// Number of nodes currently stored in graph.
///
/// g[in] : Graph to query.
///
/// SUCCESS: Node count.
/// FAILURE: Function cannot fail.
///
/// TAGS: Graph, Node, Count, Query
///
#define GraphNodeCount(g) VecLen(&((g)->nodes))

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
/// Check whether graph contains no nodes.
///
/// g[in] : Graph to query.
///
/// SUCCESS: `true` when graph has zero nodes.
/// FAILURE: `false`
///
/// TAGS: Graph, Empty, Query
///
#define GraphEmpty(g) (GraphNodeCount(g) == 0)

///
/// Access node payload at given node id.
///
/// g[in]       : Graph to query.
/// node_id[in] : Node id to access.
///
/// TAGS: Graph, Node, Access
///
#define GraphNodeAt(g, node_id) VecAt(&((g)->nodes), (node_id))

///
/// Get pointer to node payload at given node id.
///
/// g[in,out]   : Graph to query.
/// node_id[in] : Node id to access.
///
/// TAGS: Graph, Node, Access, Pointer
///
#define GraphNodePtrAt(g, node_id) VecPtrAt(&((g)->nodes), (node_id))

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

#endif // MISRA_STD_CONTAINER_GRAPH_ACCESS_H
