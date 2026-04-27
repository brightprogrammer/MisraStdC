/// file      : std/graph.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Generic directed graph implementation

#include <Misra/Std/Container/Graph.h>
#include <Misra/Std/Log.h>

static void graph_validate_node_id(const GenericGraph *graph, GraphNodeId node_id) {
    if (node_id >= graph->nodes.length) {
        LOG_FATAL("graph node id out of bounds");
    }
}

static bool graph_neighbors_contains(const GraphNeighbors *neighbors, GraphNodeId node_id) {
    size idx;

    for (idx = 0; idx < neighbors->length; idx++) {
        if (VecAt(neighbors, idx) == node_id) {
            return true;
        }
    }

    return false;
}

bool graph_neighbors_init_copy(void *dst, void *src) {
    GraphNeighbors *dst_neighbors;
    GraphNeighbors *src_neighbors;

    if (!dst || !src) {
        LOG_FATAL("invalid arguments");
    }

    dst_neighbors = dst;
    src_neighbors = src;

    ValidateVec(src_neighbors);

    *dst_neighbors = VecInitAlignedWithDeepCopyT(
        *dst_neighbors,
        src_neighbors->copy_init,
        src_neighbors->copy_deinit,
        src_neighbors->alignment
    );

    if (src_neighbors->length) {
        reserve_vec(GENERIC_VEC(dst_neighbors), sizeof(GraphNodeId), src_neighbors->length);
        insert_range_into_vec(
            GENERIC_VEC(dst_neighbors),
            (char *)src_neighbors->data,
            sizeof(GraphNodeId),
            0,
            src_neighbors->length
        );
    }

    return true;
}

void graph_neighbors_deinit(void *copy) {
    GraphNeighbors *neighbors;

    if (!copy) {
        LOG_FATAL("invalid arguments");
    }

    neighbors = copy;
    ValidateVec(neighbors);
    deinit_vec(GENERIC_VEC(neighbors), sizeof(GraphNodeId));
}

void validate_graph(const GenericGraph *graph) {
    if (!graph) {
        LOG_FATAL("Expected a valid Graph pointer");
    }

    if (graph->__magic != MISRA_GRAPH_MAGIC) {
        LOG_FATAL("Graph is uninitialized or corrupted");
    }

    validate_vec((const GenericVec *)&graph->nodes);
    validate_vec((const GenericVec *)&graph->out_neighbors);

    if (graph->nodes.length != graph->out_neighbors.length) {
        LOG_FATAL("Graph node storage and adjacency storage are out of sync");
    }
}

void deinit_graph(GenericGraph *graph, size item_size) {
    ValidateGraph(graph);

    clear_graph(graph, item_size);
    deinit_vec(GENERIC_VEC(&graph->nodes), item_size);
    deinit_vec(GENERIC_VEC(&graph->out_neighbors), sizeof(GraphNeighbors));
    graph->edge_count = 0;
}

void clear_graph(GenericGraph *graph, size item_size) {
    ValidateGraph(graph);

    clear_vec(GENERIC_VEC(&graph->out_neighbors), sizeof(GraphNeighbors));
    clear_vec(GENERIC_VEC(&graph->nodes), item_size);
    graph->edge_count = 0;
}

void reserve_graph(GenericGraph *graph, size item_size, size n) {
    ValidateGraph(graph);

    reserve_vec(GENERIC_VEC(&graph->nodes), item_size, n);
    reserve_vec(GENERIC_VEC(&graph->out_neighbors), sizeof(GraphNeighbors), n);
}

GraphNodeId graph_push_node(GenericGraph *graph, const void *item_data, size item_size) {
    GraphNeighbors neighbors;
    GraphNodeId    node_id;

    if (!graph || !item_data || !item_size) {
        LOG_FATAL("invalid arguments");
    }

    ValidateGraph(graph);

    node_id = graph->nodes.length;
    insert_range_into_vec(GENERIC_VEC(&graph->nodes), (char *)item_data, item_size, graph->nodes.length, 1);

    neighbors = VecInitT(neighbors);
    insert_range_into_vec(
        GENERIC_VEC(&graph->out_neighbors),
        (char *)&neighbors,
        sizeof(GraphNeighbors),
        graph->out_neighbors.length,
        1
    );

    return node_id;
}

GraphNodeId graph_push_node_owned(GenericGraph *graph, void *item_data, size item_size) {
    GraphNodeId node_id;

    if (!graph || !item_data || !item_size) {
        LOG_FATAL("invalid arguments");
    }

    node_id = graph_push_node(graph, item_data, item_size);

    if (!graph->nodes.copy_init) {
        memset(item_data, 0, item_size);
    }

    return node_id;
}

GraphNeighbors *graph_out_neighbors_ptr(GenericGraph *graph, GraphNodeId node_id) {
    GraphNeighbors *neighbors;

    ValidateGraph(graph);
    graph_validate_node_id(graph, node_id);

    neighbors = VecPtrAt(&graph->out_neighbors, node_id);
    ValidateVec(neighbors);
    return neighbors;
}

size graph_out_degree(GenericGraph *graph, GraphNodeId node_id) {
    return graph_out_neighbors_ptr(graph, node_id)->length;
}

bool graph_has_edge(GenericGraph *graph, GraphNodeId from, GraphNodeId to) {
    GraphNeighbors *neighbors;

    ValidateGraph(graph);
    graph_validate_node_id(graph, from);
    graph_validate_node_id(graph, to);

    neighbors = graph_out_neighbors_ptr(graph, from);
    return graph_neighbors_contains(neighbors, to);
}

bool graph_add_edge(GenericGraph *graph, GraphNodeId from, GraphNodeId to) {
    GraphNeighbors *neighbors;

    ValidateGraph(graph);
    graph_validate_node_id(graph, from);
    graph_validate_node_id(graph, to);

    neighbors = graph_out_neighbors_ptr(graph, from);
    if (graph_neighbors_contains(neighbors, to)) {
        return false;
    }

    insert_range_into_vec(GENERIC_VEC(neighbors), (char *)&to, sizeof(GraphNodeId), neighbors->length, 1);
    graph->edge_count += 1;
    return true;
}
