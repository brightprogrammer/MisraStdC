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

    typedef struct {
        GenericGraph *graph;
        u64           slot_index;
        u64           expected_mutation_epoch;
    } GenericGraphNodeIter;

    typedef struct {
        GenericGraph *graph;
        GraphNodeId   source_id;
        u64           neighbor_index;
        u64           expected_mutation_epoch;
    } GenericGraphNeighborIter;

    typedef struct {
        GenericGraph *graph;
        GraphNodeId   target_id;
        u64           predecessor_index;
        u64           expected_mutation_epoch;
    } GenericGraphPredecessorIter;

    void                    validate_graph(const GenericGraph *graph);
    void                    deinit_graph(GenericGraph *graph, size item_size);
    void                    clear_graph(GenericGraph *graph, size item_size);
    void                    reserve_graph(GenericGraph *graph, size item_size, size n);
    GraphNodeId             graph_push_node(GenericGraph *graph, const void *item_data, size item_size);
    GraphNodeId             graph_push_node_owned(GenericGraph *graph, void *item_data, size item_size);
    bool                    graph_contains_node(GenericGraph *graph, GraphNodeId node_id);
    GraphNode               graph_get_node(GenericGraph *graph, GraphNodeId node_id);
    void                   *graph_node_ptr_at(GenericGraph *graph, GraphNodeId node_id);
    void                   *graph_node_data_ptr_checked(GenericGraph *graph, GraphNode node);
    GraphNeighbors         *graph_out_neighbors_ptr(GenericGraph *graph, GraphNodeId node_id);
    GraphNeighbors         *graph_in_neighbors_ptr(GenericGraph *graph, GraphNodeId node_id);
    size                    graph_out_degree(GenericGraph *graph, GraphNodeId node_id);
    size                    graph_in_degree(GenericGraph *graph, GraphNodeId node_id);
    GraphNodeId             graph_neighbor_at(GenericGraph *graph, GraphNodeId from, size neighbor_idx);
    GraphNodeId             graph_predecessor_at(GenericGraph *graph, GraphNodeId to, size predecessor_idx);
    bool                    graph_has_edge(GenericGraph *graph, GraphNodeId from, GraphNodeId to);
    bool                    graph_add_edge(GenericGraph *graph, GraphNodeId from, GraphNodeId to);
    u64                     graph_node_visit(GraphNode node);
    void                    graph_node_unvisit(GraphNode node);
    u64                     graph_node_visit_count(GraphNode node);
    bool                    graph_node_visited(GraphNode node);
    bool                    graph_mark_node_for_deletion(GraphNode node);
    bool                    graph_node_marked_for_deletion(GraphNode node);
    bool                    graph_unmark_node_for_deletion(GraphNode node);
    bool                    graph_mark_edge_for_removal(GenericGraph *graph, GraphNodeId from, GraphNodeId to);
    bool                    graph_edge_marked_for_removal(GenericGraph *graph, GraphNodeId from, GraphNodeId to);
    bool                    graph_unmark_edge_for_removal(GenericGraph *graph, GraphNodeId from, GraphNodeId to);
    u64                     graph_commit_changes(GenericGraph *graph, size item_size);
    GenericGraphNodeIter    graph_node_iter_begin(GenericGraph *graph);
    bool                    graph_node_iter_next(GenericGraphNodeIter *iter, GraphNode *out_node);
    GenericGraphNeighborIter graph_neighbor_iter_begin(GraphNode node);
    bool                    graph_neighbor_iter_next(GenericGraphNeighborIter *iter, GraphNode *out_node);
    GenericGraphPredecessorIter graph_predecessor_iter_begin(GraphNode node);
    bool                        graph_predecessor_iter_next(GenericGraphPredecessorIter *iter, GraphNode *out_node);

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_GRAPH_PRIVATE_H
