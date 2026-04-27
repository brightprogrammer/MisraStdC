/// file      : std/container/graph/type.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Generic directed graph type definition

#ifndef MISRA_STD_CONTAINER_GRAPH_TYPE_H
#define MISRA_STD_CONTAINER_GRAPH_TYPE_H

#include <Misra/Std/Container/Vec.h>
#include <Misra/Types.h>

///
/// Stable identifier used to refer to a graph node.
///
/// TAGS: Graph, Node, Id
///
typedef u64 GraphNodeId;

///
/// Outgoing neighbor list for a single graph node.
///
/// Each item is a `GraphNodeId` of a node reachable by one directed edge.
///
/// TAGS: Graph, Edge, Neighbor, Vec
///
typedef Vec(GraphNodeId) GraphNeighbors;

///
/// Internal vector storing one outgoing neighbor list per node.
///
/// TAGS: Graph, Neighbor, Vec, Internal
///
typedef Vec(GraphNeighbors) GraphAdjacencyLists;

typedef struct {
    GenericVec          nodes;
    GraphAdjacencyLists out_neighbors;
    u64                 edge_count;
    u64                 __magic;
} GenericGraph;

#define GENERIC_GRAPH(g) ((GenericGraph *)(void *)(g))

///
/// Typesafe directed graph definition.
///
/// Node payloads are stored in insertion order and are referred to by their
/// stable zero-based `GraphNodeId`. Edges are directed and stored as outgoing
/// adjacency lists of those node ids.
///
/// NOTE: Like the other generic containers in this project, each `Graph(T)`
///       expansion creates a distinct anonymous type. Prefer a `typedef`
///       when you need to reuse the same graph type across APIs.
///
/// USAGE:
///   typedef Graph(int) IntGraph;
///   IntGraph graph = GraphInit();
///
/// FIELDS:
/// - nodes         : Node payload storage. Do not index directly. Use graph access helpers.
/// - out_neighbors : One outgoing adjacency list per node.
/// - edge_count    : Number of directed edges currently stored in graph.
///
/// TAGS: Graph, Generic, Directed, AdjacencyList
///
#define Graph(T)                                                                                                       \
    struct {                                                                                                           \
        Vec(T)             nodes;                                                                                      \
        GraphAdjacencyLists out_neighbors;                                                                             \
        u64                edge_count;                                                                                 \
        u64                __magic;                                                                                    \
    }

#define GRAPH_NODE_TYPE(g) VEC_DATATYPE(&((g)->nodes))

#define MISRA_GRAPH_MAGIC MISRA_MAKE_NEW_MAGIC_VALUE("digrph00")

///
/// Validate whether a given `Graph` object is valid.
/// Aborts if provided graph is uninitialized or corrupted.
///
/// g[in] : Pointer to `Graph` object to validate.
///
/// SUCCESS: Continue execution, meaning given graph is most probably valid.
/// FAILURE: `abort`
///
#define ValidateGraph(g) validate_graph((const GenericGraph *)GENERIC_GRAPH(g))

#endif // MISRA_STD_CONTAINER_GRAPH_TYPE_H
