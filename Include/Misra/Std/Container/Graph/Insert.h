/// file      : std/container/graph/insert.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Insert helpers for Graph.

#ifndef MISRA_STD_CONTAINER_GRAPH_INSERT_H
#define MISRA_STD_CONTAINER_GRAPH_INSERT_H

#include "Type.h"
#include "Private.h"

///
/// Add a node using l-value semantics and return its node id.
///
/// NOTE: Ownership of node payload is transferred to graph when node copy-init
///       is not set. In that case the provided l-value is zeroed after insertion.
///
/// g[in,out] : Graph to modify.
/// lval[in]  : Node payload l-value to insert.
///
/// SUCCESS: Node id assigned to inserted node.
/// FAILURE: Does not return on invalid arguments.
///
/// TAGS: Graph, Node, Insert, LValue
///
#define GraphAddNodeL(g, lval)                                                                                         \
    ((void)(&((GRAPH_NODE_TYPE(g)[]){(lval)})[0]),                                                                    \
     graph_push_node_owned(GENERIC_GRAPH(g), &(lval), sizeof(GRAPH_NODE_TYPE(g))))

///
/// Add a node using r-value semantics and return its node id.
///
/// g[in,out] : Graph to modify.
/// rval[in]  : Node payload r-value to insert.
///
/// SUCCESS: Node id assigned to inserted node.
/// FAILURE: Does not return on invalid arguments.
///
/// TAGS: Graph, Node, Insert, RValue
///
#define GraphAddNodeR(g, rval)                                                                                        \
    graph_push_node(                                                                                                  \
        GENERIC_GRAPH(g),                                                                                             \
        &((GRAPH_NODE_TYPE(g)[]){(rval)})[0],                                                                         \
        sizeof(GRAPH_NODE_TYPE(g))                                                                                    \
    )

///
/// Add a node using l-value semantics and return its node id.
///
/// TAGS: Graph, Node, Insert, Ownership
///
#define GraphAddNode(g, lval) GraphAddNodeL((g), (lval))

///
/// Add a directed edge `from -> to`.
///
/// Duplicate edges are ignored and return `false`.
///
/// g[in,out] : Graph to modify.
/// from[in]  : Source node id.
/// to[in]    : Destination node id.
///
/// SUCCESS: `true` when a new edge was inserted.
/// FAILURE: `false` when edge already existed.
///
/// TAGS: Graph, Edge, Insert, Directed
///
#define GraphAddEdge(g, from, to) graph_add_edge(GENERIC_GRAPH(g), (from), (to))

#endif // MISRA_STD_CONTAINER_GRAPH_INSERT_H
