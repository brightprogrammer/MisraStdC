/// file      : std/container/graph/foreach.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Iteration helpers for Graph.

#ifndef MISRA_STD_CONTAINER_GRAPH_FOREACH_H
#define MISRA_STD_CONTAINER_GRAPH_FOREACH_H

#include "Access.h"

///
/// Iterate over each live node of given graph.
///
/// The loop variable is a `GraphNode` handle carrying the owning graph plus a stable
/// node id. Marking nodes for deletion is allowed during iteration, but structural
/// changes such as adding nodes, adding edges, clearing, or committing changes will
/// invalidate the traversal and abort on the next iteration step.
///
/// g[in,out] : Graph to iterate over.
/// node[in]  : Name of the `GraphNode` loop variable.
///
/// TAGS: Graph, Foreach, Node, Iteration
///
#define GraphForeachNode(g, node)                                                                                      \
    for (TYPE_OF(g) UNPL(pg) = (g); UNPL(pg); UNPL(pg) = NULL)                                                         \
        if ((ValidateGraph(UNPL(pg)), 1) && GraphNodeCount(UNPL(pg)) > 0)                                              \
            for (GenericGraphNodeIter UNPL(iter) = graph_node_iter_begin(GENERIC_GRAPH(UNPL(pg))); UNPL(iter).graph;  \
                 UNPL(iter).graph = NULL)                                                                              \
                for (GraphNode node = {0}; graph_node_iter_next(&UNPL(iter), &node);)

///
/// Iterate over each outgoing neighbor of a node handle.
///
/// The neighbor loop variable is also a `GraphNode` handle.
///
/// node[in]      : Source node handle.
/// neighbor[in]  : Name of the `GraphNode` loop variable used for outgoing neighbors.
///
/// TAGS: Graph, Foreach, Neighbor, Iteration
///
#define GraphNodeForeachNeighbor(node, neighbor)                                                                       \
    for (GraphNode UNPL(src_node) = (node); UNPL(src_node).__graph; UNPL(src_node).__graph = NULL)                    \
        for (GenericGraphNeighborIter UNPL(iter) = graph_neighbor_iter_begin(UNPL(src_node)); UNPL(iter).graph;       \
             UNPL(iter).graph = NULL)                                                                                  \
            for (GraphNode neighbor = {0}; graph_neighbor_iter_next(&UNPL(iter), &neighbor);)

///
/// Iterate over each incoming predecessor of a node handle.
///
/// The predecessor loop variable is also a `GraphNode` handle.
///
/// node[in]          : Destination node handle.
/// predecessor[in]   : Name of the `GraphNode` loop variable used for incoming predecessors.
///
/// TAGS: Graph, Foreach, Predecessor, Iteration
///
#define GraphNodeForeachPredecessor(node, predecessor)                                                                \
    for (GraphNode UNPL(dst_node) = (node); UNPL(dst_node).__graph; UNPL(dst_node).__graph = NULL)                   \
        for (GenericGraphPredecessorIter UNPL(iter) = graph_predecessor_iter_begin(UNPL(dst_node));                  \
             UNPL(iter).graph;                                                                                        \
             UNPL(iter).graph = NULL)                                                                                 \
            for (GraphNode predecessor = {0}; graph_predecessor_iter_next(&UNPL(iter), &predecessor);)

#endif // MISRA_STD_CONTAINER_GRAPH_FOREACH_H
