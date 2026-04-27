/// file      : std/container/graph/private.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Internal runtime helpers for Graph.

#ifndef MISRA_STD_CONTAINER_GRAPH_PRIVATE_H
#define MISRA_STD_CONTAINER_GRAPH_PRIVATE_H

#include "Type.h"

#ifdef __cplusplus
extern "C" {
#endif

    bool         graph_neighbors_init_copy(void *dst, void *src);
    void         graph_neighbors_deinit(void *copy);
    void         validate_graph(const GenericGraph *graph);
    void         deinit_graph(GenericGraph *graph, size item_size);
    void         clear_graph(GenericGraph *graph, size item_size);
    void         reserve_graph(GenericGraph *graph, size item_size, size n);
    GraphNodeId  graph_push_node(GenericGraph *graph, const void *item_data, size item_size);
    GraphNodeId  graph_push_node_owned(GenericGraph *graph, void *item_data, size item_size);
    GraphNeighbors *graph_out_neighbors_ptr(GenericGraph *graph, GraphNodeId node_id);
    size         graph_out_degree(GenericGraph *graph, GraphNodeId node_id);
    bool         graph_has_edge(GenericGraph *graph, GraphNodeId from, GraphNodeId to);
    bool         graph_add_edge(GenericGraph *graph, GraphNodeId from, GraphNodeId to);

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_GRAPH_PRIVATE_H
