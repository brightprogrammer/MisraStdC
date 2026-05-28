/// file      : std/container/graph.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Generic directed graph implementation

#include <Misra/Std/Container/Graph.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Memory.h>
#include <Misra/Sys.h>


#define GRAPH_SLOT_OCCUPIED ((u32)1u << 0)
#define GRAPH_SLOT_MARKED   ((u32)1u << 1)

static GraphNodeId graph_make_node_id(u32 index, u32 generation) {
    return (((u64)generation) << 32) | (u64)index;
}

static bool graph_alignment_is_pow2(u64 alignment) {
    return alignment && ((alignment & (alignment - 1)) == 0);
}

static void graph_validate_alignment(const GenericGraph *graph) {
    u64 alignment = graph->allocator->alignment;

    if (!alignment) {
        LOG_FATAL("Invalid graph allocator alignment. Did you initialize the graph before use?");
    }

    if ((alignment > 1) && !graph_alignment_is_pow2(alignment)) {
        LOG_FATAL("Graph allocator alignment must be 1 or a power of two");
    }
}

static void graph_validate_slot_limit(const GenericGraph *graph) {
    if (VecLen(&graph->slots) > (u64)UINT32_MAX) {
        LOG_FATAL("Graph exceeded maximum supported slot count");
    }
}

static bool graph_slot_is_occupied(const GenericGraphSlot *slot) {
    return (slot->flags & GRAPH_SLOT_OCCUPIED) != 0;
}

static bool graph_slot_is_marked(const GenericGraphSlot *slot) {
    return (slot->flags & GRAPH_SLOT_MARKED) != 0;
}

static void graph_validate_node_index_raw(const GenericGraph *graph, u32 index) {
    if ((u64)index >= VecLen(&graph->slots)) {
        LOG_FATAL("graph node id out of bounds");
    }
}

static GenericGraphSlot *graph_slot_ptr_raw(GenericGraph *graph, u32 index) {
    return VecPtrAt(&graph->slots, index);
}

static const GenericGraphSlot *graph_slot_ptr_const_raw(const GenericGraph *graph, u32 index) {
    return VecPtrAt((GraphSlots *)&graph->slots, index);
}

static void graph_validate_node_id(const GenericGraph *graph, GraphNodeId node_id) {
    u32                     index;
    u32                     generation;
    const GenericGraphSlot *slot;

    index      = GraphNodeIdIndex(node_id);
    generation = GraphNodeIdGeneration(node_id);

    if (!generation) {
        LOG_FATAL("graph node id has invalid generation");
    }

    graph_validate_node_index_raw(graph, index);
    slot = graph_slot_ptr_const_raw(graph, index);

    if (!graph_slot_is_occupied(slot)) {
        LOG_FATAL("graph node id refers to a free slot");
    }

    if (slot->generation != generation) {
        LOG_FATAL("graph node id is stale");
    }
}

static GenericGraphSlot *graph_require_live_slot(GenericGraph *graph, GraphNodeId node_id) {
    graph_validate_node_id(graph, node_id);
    return graph_slot_ptr_raw(graph, GraphNodeIdIndex(node_id));
}

static const GenericGraphSlot *graph_require_live_slot_const(const GenericGraph *graph, GraphNodeId node_id) {
    graph_validate_node_id(graph, node_id);
    return graph_slot_ptr_const_raw(graph, GraphNodeIdIndex(node_id));
}

static GraphNode graph_validate_node_handle(GraphNode node) {
    GenericGraph *graph;

    graph = GENERIC_GRAPH(node._graph_);
    if (!graph) {
        LOG_FATAL("invalid graph node handle");
    }

    ValidateGraph(graph);
    graph_validate_node_id(graph, node._id_);
    return node;
}

static void *graph_alloc_node_data(GenericGraph *graph, size item_size) {
    graph_validate_alignment(graph);
    return AllocatorAlloc(graph->allocator, item_size, true);
}

static void graph_free_node_data(GenericGraph *graph, void *data, size item_size) {
    if (!data) {
        return;
    }

    if (graph->copy_deinit) {
        graph->copy_deinit(data, graph->allocator);
    } else {
        MemSet(data, 0, item_size);
    }

    AllocatorFree(graph->allocator, data);
}

static bool graph_copy_node_data(GenericGraph *graph, void *dst, const void *src, size item_size) {
    if (graph->copy_init) {
        return graph->copy_init(dst, src, graph->allocator);
    }

    MemCopy(dst, src, item_size);
    return true;
}

static void graph_bump_mutation_epoch(GenericGraph *graph) {
    graph->mutation_epoch += 1;
}

static void graph_ensure_slot_generation_available(GenericGraphSlot *slot) {
    if (slot->generation == UINT32_MAX) {
        LOG_FATAL("graph slot generation exhausted");
    }
}

static void graph_release_slot(GenericGraph *graph, GenericGraphSlot *slot, size item_size) {
    if (!graph_slot_is_occupied(slot)) {
        return;
    }

    graph_free_node_data(graph, slot->data, item_size);
    slot->data = NULL;

    deinit_vec(GENERIC_VEC(&slot->out_neighbors), sizeof(GraphNodeId));
    slot->out_neighbors = VecInitT(slot->out_neighbors, graph->allocator);
    deinit_vec(GENERIC_VEC(&slot->in_neighbors), sizeof(GraphNodeId));
    slot->in_neighbors = VecInitT(slot->in_neighbors, graph->allocator);

    slot->visit_count = 0;
    slot->flags       = 0;

    graph_ensure_slot_generation_available(slot);
    slot->generation += 1;
}

static void graph_push_free_index(GenericGraph *graph, u32 index) {
    if (!insert_range_into_vec(
            GENERIC_VEC(&graph->free_indices),
            (const u8 *)&index,
            sizeof(u32),
            VecLen(&graph->free_indices),
            1
        )) {
        LOG_FATAL("Graph failed to record a reusable slot index");
    }
}

static u32 graph_take_free_index(GenericGraph *graph) {
    u32 index;

    if (!VecLen(&graph->free_indices)) {
        LOG_FATAL("expected at least one free graph slot");
    }

    VecPopBack(&graph->free_indices, &index);
    return index;
}

static bool graph_neighbors_contains(const GraphNeighbors *neighbors, GraphNodeId node_id) {
    size idx;

    for (idx = 0; idx < VecLen(neighbors); idx++) {
        if (VecAt(neighbors, idx) == node_id) {
            return true;
        }
    }

    return false;
}

static size graph_find_neighbor_index(const GraphNeighbors *neighbors, GraphNodeId node_id) {
    size idx;

    for (idx = 0; idx < VecLen(neighbors); idx++) {
        if (VecAt(neighbors, idx) == node_id) {
            return idx;
        }
    }

    return SIZE_MAX;
}

static size graph_find_pending_edge_removal_index(const GenericGraph *graph, GraphNodeId from, GraphNodeId to) {
    size idx;

    for (idx = 0; idx < VecLen(&graph->pending_edge_removals); idx++) {
        const GraphPendingEdgeRemoval *pending =
            VecPtrAt((GraphPendingEdgeRemovals *)&graph->pending_edge_removals, idx);
        if ((pending->from == from) && (pending->to == to)) {
            return idx;
        }
    }

    return SIZE_MAX;
}

static bool graph_remove_edge_now(GenericGraph *graph, GraphNodeId from, GraphNodeId to) {
    GraphNeighbors *out_neighbors;
    GraphNeighbors *in_neighbors;
    size            out_idx;
    size            in_idx;

    out_neighbors = graph_out_neighbors_ptr(graph, from);
    out_idx       = graph_find_neighbor_index(out_neighbors, to);
    if (out_idx == SIZE_MAX) {
        return false;
    }

    in_neighbors = graph_in_neighbors_ptr(graph, to);
    in_idx       = graph_find_neighbor_index(in_neighbors, from);
    if (in_idx == SIZE_MAX) {
        LOG_FATAL("Graph reverse adjacency is inconsistent during edge removal");
    }

    remove_range_vec(GENERIC_VEC(out_neighbors), NULL, sizeof(GraphNodeId), out_idx, 1);
    remove_range_vec(GENERIC_VEC(in_neighbors), NULL, sizeof(GraphNodeId), in_idx, 1);
    graph->edge_count -= 1;
    return true;
}

static size graph_remove_marked_outgoing_edges(GenericGraph *graph, GraphNodeId from) {
    size            removed   = 0;
    GraphNeighbors *neighbors = graph_out_neighbors_ptr(graph, from);
    size            idx       = 0;

    while (idx < VecLen(neighbors)) {
        GraphNodeId             neighbor_id = VecAt(neighbors, idx);
        const GenericGraphSlot *slot;

        graph_validate_node_index_raw(graph, GraphNodeIdIndex(neighbor_id));
        slot = graph_slot_ptr_const_raw(graph, GraphNodeIdIndex(neighbor_id));

        if (graph_slot_is_occupied(slot) && !graph_slot_is_marked(slot) &&
            (slot->generation == GraphNodeIdGeneration(neighbor_id))) {
            idx += 1;
        } else {
            if (!graph_remove_edge_now(graph, from, neighbor_id)) {
                LOG_FATAL("Graph failed to remove marked outgoing edge");
            }
            removed += 1;
        }
    }

    return removed;
}

static size graph_remove_all_outgoing_edges(GenericGraph *graph, GraphNodeId from) {
    size            removed   = 0;
    GraphNeighbors *neighbors = graph_out_neighbors_ptr(graph, from);

    while (VecLen(neighbors)) {
        GraphNodeId to = VecLast(neighbors);
        if (!graph_remove_edge_now(graph, from, to)) {
            LOG_FATAL("Graph failed to remove outgoing edge during node deletion");
        }
        removed += 1;
    }

    return removed;
}

void validate_graph(const GenericGraph *graph) {
    u64 live_count     = 0;
    u64 out_edge_count = 0;
    u64 in_edge_count  = 0;
    u64 marked_count   = 0;
    u64 slot_index;
    u64 free_index_i;

    if (!graph) {
        LOG_FATAL("Expected a valid Graph pointer");
    }

    if (graph->__magic != GRAPH_MAGIC) {
        LOG_FATAL("Graph is uninitialized or corrupted");
    }

    // Graph has no stack-init form, so a NULL allocator on a magic-OK
    // handle means corruption between init and use. Surface it before
    // dereferencing the method table.
    if (!graph->allocator) {
        LOG_FATAL("Graph allocator pointer is NULL");
    }

    if (!graph->allocator->allocate || !graph->allocator->resize || !graph->allocator->remap ||
        !graph->allocator->deallocate) {
        LOG_FATAL("Graph allocator is not fully configured");
    }

    validate_vec((const GenericVec *)&graph->slots);
    validate_vec((const GenericVec *)&graph->free_indices);
    validate_vec((const GenericVec *)&graph->pending_edge_removals);
    graph_validate_alignment(graph);
    graph_validate_slot_limit(graph);

    if (graph->live_count > VecLen(&graph->slots)) {
        LOG_FATAL("Graph live node count exceeds slot count");
    }

    if (graph->pending_delete_count > graph->live_count) {
        LOG_FATAL("Graph pending delete count exceeds live node count");
    }

    if ((graph->live_count + VecLen(&graph->free_indices)) != VecLen(&graph->slots)) {
        LOG_FATAL("Graph slot accounting is inconsistent");
    }

    for (slot_index = 0; slot_index < VecLen(&graph->slots); slot_index++) {
        const GenericGraphSlot *slot    = VecPtrAt((GraphSlots *)&graph->slots, slot_index);
        GraphNodeId             self_id = graph_make_node_id((u32)slot_index, slot->generation);
        u64                     neighbor_i;

        ValidateVec(&slot->out_neighbors);
        ValidateVec(&slot->in_neighbors);

        if (graph_slot_is_occupied(slot)) {
            if (!slot->data) {
                LOG_FATAL("Occupied graph slot has NULL payload");
            }

            if (!slot->generation) {
                LOG_FATAL("Occupied graph slot has invalid generation");
            }

            live_count     += 1;
            out_edge_count += VecLen(&slot->out_neighbors);
            in_edge_count  += VecLen(&slot->in_neighbors);
            marked_count   += graph_slot_is_marked(slot) ? 1 : 0;

            for (neighbor_i = 0; neighbor_i < VecLen(&slot->out_neighbors); neighbor_i++) {
                GraphNodeId             neighbor_id = VecAt(&slot->out_neighbors, neighbor_i);
                const GenericGraphSlot *target_slot;

                graph_validate_node_id(graph, neighbor_id);
                target_slot = graph_require_live_slot_const(graph, neighbor_id);
                if (!graph_neighbors_contains(&target_slot->in_neighbors, self_id)) {
                    LOG_FATAL("Graph reverse adjacency is missing predecessor entry");
                }
            }

            for (neighbor_i = 0; neighbor_i < VecLen(&slot->in_neighbors); neighbor_i++) {
                GraphNodeId             predecessor_id = VecAt(&slot->in_neighbors, neighbor_i);
                const GenericGraphSlot *source_slot;

                graph_validate_node_id(graph, predecessor_id);
                source_slot = graph_require_live_slot_const(graph, predecessor_id);
                if (!graph_neighbors_contains(&source_slot->out_neighbors, self_id)) {
                    LOG_FATAL("Graph reverse adjacency is missing outgoing edge");
                }
            }
        } else {
            if (slot->data) {
                LOG_FATAL("Free graph slot retains payload pointer");
            }

            if (slot->visit_count != 0) {
                LOG_FATAL("Free graph slot retains visit count");
            }

            if (VecLen(&slot->out_neighbors) != 0) {
                LOG_FATAL("Free graph slot retains outgoing edges");
            }

            if (VecLen(&slot->in_neighbors) != 0) {
                LOG_FATAL("Free graph slot retains incoming edges");
            }
        }
    }

    for (free_index_i = 0; free_index_i < VecLen(&graph->free_indices); free_index_i++) {
        u32 index = VecAt(&graph->free_indices, free_index_i);
        if ((u64)index >= VecLen(&graph->slots)) {
            LOG_FATAL("Graph free slot index out of bounds");
        }

        if (graph_slot_is_occupied(VecPtrAt((GraphSlots *)&graph->slots, index))) {
            LOG_FATAL("Graph free index points to an occupied slot");
        }
    }

    for (free_index_i = 0; free_index_i < VecLen(&graph->pending_edge_removals); free_index_i++) {
        const GraphPendingEdgeRemoval *pending =
            VecPtrAt((GraphPendingEdgeRemovals *)&graph->pending_edge_removals, free_index_i);
        const GraphNeighbors *neighbors;

        graph_validate_node_id(graph, pending->from);
        graph_validate_node_id(graph, pending->to);
        neighbors = &graph_require_live_slot_const(graph, pending->from)->out_neighbors;

        if (!graph_neighbors_contains(neighbors, pending->to)) {
            LOG_FATAL("Graph pending edge removal refers to a missing edge");
        }
    }

    if (graph->live_count != live_count) {
        LOG_FATAL("Graph live node count is inconsistent");
    }

    if ((graph->edge_count != out_edge_count) || (graph->edge_count != in_edge_count)) {
        LOG_FATAL("Graph edge count is inconsistent");
    }

    if (graph->pending_delete_count != marked_count) {
        LOG_FATAL("Graph pending delete count is inconsistent");
    }
}

void deinit_graph(GenericGraph *graph, size item_size) {
    ValidateGraph(graph);

    clear_graph(graph, item_size);
    deinit_vec(GENERIC_VEC(&graph->slots), sizeof(GenericGraphSlot));
    deinit_vec(GENERIC_VEC(&graph->free_indices), sizeof(u32));
    deinit_vec(GENERIC_VEC(&graph->pending_edge_removals), sizeof(GraphPendingEdgeRemoval));

    MemSet(graph, 0, sizeof(*graph));
}

void clear_graph(GenericGraph *graph, size item_size) {
    u64 slot_index;

    ValidateGraph(graph);

    for (slot_index = 0; slot_index < VecLen(&graph->slots); slot_index++) {
        GenericGraphSlot *slot = VecPtrAt(&graph->slots, slot_index);
        if (graph_slot_is_occupied(slot)) {
            graph_release_slot(graph, slot, item_size);
        } else {
            deinit_vec(GENERIC_VEC(&slot->out_neighbors), sizeof(GraphNodeId));
            slot->out_neighbors = VecInitT(slot->out_neighbors, graph->allocator);
            deinit_vec(GENERIC_VEC(&slot->in_neighbors), sizeof(GraphNodeId));
            slot->in_neighbors = VecInitT(slot->in_neighbors, graph->allocator);
            slot->visit_count  = 0;
            slot->flags        = 0;
        }
    }

    clear_vec(GENERIC_VEC(&graph->free_indices), sizeof(u32));
    clear_vec(GENERIC_VEC(&graph->pending_edge_removals), sizeof(GraphPendingEdgeRemoval));
    for (slot_index = 0; slot_index < VecLen(&graph->slots); slot_index++) {
        u32 index = (u32)slot_index;
        graph_push_free_index(graph, index);
    }

    graph->live_count           = 0;
    graph->edge_count           = 0;
    graph->pending_delete_count = 0;
    graph_bump_mutation_epoch(graph);
}

bool reserve_graph(GenericGraph *graph, size item_size, size n) {
    size old_capacity;
    bool success;

    (void)item_size;

    ValidateGraph(graph);
    old_capacity = VecCapacity(&graph->slots);

    success = reserve_vec(GENERIC_VEC(&graph->free_indices), sizeof(u32), n) &&
              reserve_vec(GENERIC_VEC(&graph->slots), sizeof(GenericGraphSlot), n);
    if (!success) {
        return false;
    }

    if (VecCapacity(&graph->slots) != old_capacity) {
        graph_bump_mutation_epoch(graph);
    }

    return true;
}

GraphNodeId graph_push_node(GenericGraph *graph, const void *item_data, size item_size) {
    GraphNodeId       node_id;
    GenericGraphSlot  slot;
    GenericGraphSlot *slot_ptr;
    u32               slot_index;

    if (!graph || !item_data || !item_size) {
        LOG_FATAL("invalid arguments");
    }

    ValidateGraph(graph);

    if (VecLen(&graph->free_indices)) {
        slot_index = graph_take_free_index(graph);
        slot_ptr   = graph_slot_ptr_raw(graph, slot_index);
        if (graph_slot_is_occupied(slot_ptr)) {
            LOG_FATAL("graph free slot unexpectedly occupied");
        }

        slot_ptr->out_neighbors = VecInitT(slot_ptr->out_neighbors, graph->allocator);
        slot_ptr->in_neighbors  = VecInitT(slot_ptr->in_neighbors, graph->allocator);
        slot_ptr->data          = graph_alloc_node_data(graph, item_size);
        if (!slot_ptr->data) {
            graph_push_free_index(graph, slot_index);
            return 0;
        }
        slot_ptr->visit_count = 0;
        slot_ptr->flags       = GRAPH_SLOT_OCCUPIED;
        if (!graph_copy_node_data(graph, slot_ptr->data, item_data, item_size)) {
            graph_free_node_data(graph, slot_ptr->data, item_size);
            slot_ptr->data = NULL;
            deinit_vec(GENERIC_VEC(&slot_ptr->out_neighbors), sizeof(GraphNodeId));
            slot_ptr->out_neighbors = VecInitT(slot_ptr->out_neighbors, graph->allocator);
            deinit_vec(GENERIC_VEC(&slot_ptr->in_neighbors), sizeof(GraphNodeId));
            slot_ptr->in_neighbors = VecInitT(slot_ptr->in_neighbors, graph->allocator);
            slot_ptr->visit_count  = 0;
            slot_ptr->flags        = 0;
            graph_push_free_index(graph, slot_index);
            return 0;
        }

        graph->live_count += 1;
        graph_bump_mutation_epoch(graph);
        return graph_make_node_id(slot_index, slot_ptr->generation);
    }

    graph_validate_slot_limit(graph);
    if (!reserve_graph(graph, item_size, VecLen(&graph->slots) + 1)) {
        return 0;
    }

    slot.out_neighbors = VecInitT(slot.out_neighbors, graph->allocator);
    slot.in_neighbors  = VecInitT(slot.in_neighbors, graph->allocator);
    slot.data          = graph_alloc_node_data(graph, item_size);
    if (!slot.data) {
        return 0;
    }
    slot.visit_count = 0;
    slot.generation  = 1;
    slot.flags       = GRAPH_SLOT_OCCUPIED;
    if (!graph_copy_node_data(graph, slot.data, item_data, item_size)) {
        graph_free_node_data(graph, slot.data, item_size);
        return 0;
    }

    if (!insert_range_into_vec(
            GENERIC_VEC(&graph->slots),
            (const u8 *)&slot,
            sizeof(GenericGraphSlot),
            VecLen(&graph->slots),
            1
        )) {
        graph_free_node_data(graph, slot.data, item_size);
        deinit_vec(GENERIC_VEC(&slot.out_neighbors), sizeof(GraphNodeId));
        deinit_vec(GENERIC_VEC(&slot.in_neighbors), sizeof(GraphNodeId));
        return 0;
    }

    slot_index = (u32)(VecLen(&graph->slots) - 1);
    node_id    = graph_make_node_id(slot_index, 1);

    graph->live_count += 1;
    graph_bump_mutation_epoch(graph);
    return node_id;
}

GraphNodeId graph_push_node_owned(GenericGraph *graph, void *item_data, size item_size) {
    GraphNodeId node_id;

    if (!graph || !item_data || !item_size) {
        LOG_FATAL("invalid arguments");
    }

    node_id = graph_push_node(graph, item_data, item_size);

    if (node_id && !graph->copy_init) {
        MemSet(item_data, 0, item_size);
    }

    return node_id;
}

bool graph_contains_node(GenericGraph *graph, GraphNodeId node_id) {
    u32               index;
    u32               generation;
    GenericGraphSlot *slot;

    if (!graph) {
        LOG_FATAL("invalid arguments");
    }

    ValidateGraph(graph);

    index      = GraphNodeIdIndex(node_id);
    generation = GraphNodeIdGeneration(node_id);

    if (!generation || ((u64)index >= VecLen(&graph->slots))) {
        return false;
    }

    slot = graph_slot_ptr_raw(graph, index);
    return graph_slot_is_occupied(slot) && (slot->generation == generation);
}

GraphNode graph_get_node(GenericGraph *graph, GraphNodeId node_id) {
    GraphNode node;

    ValidateGraph(graph);
    graph_validate_node_id(graph, node_id);

    node._graph_ = graph;
    node._id_    = node_id;
    return node;
}

void *graph_node_ptr_at(GenericGraph *graph, GraphNodeId node_id) {
    return graph_require_live_slot(graph, node_id)->data;
}

void *graph_node_data_ptr_checked(GenericGraph *graph, GraphNode node) {
    GraphNode validated = graph_validate_node_handle(node);

    if (GENERIC_GRAPH(validated._graph_) != graph) {
        LOG_FATAL("graph node handle does not belong to the provided graph");
    }

    return graph_node_ptr_at(graph, validated._id_);
}

GraphNeighbors *graph_out_neighbors_ptr(GenericGraph *graph, GraphNodeId node_id) {
    return &graph_require_live_slot(graph, node_id)->out_neighbors;
}

GraphNeighbors *graph_in_neighbors_ptr(GenericGraph *graph, GraphNodeId node_id) {
    return &graph_require_live_slot(graph, node_id)->in_neighbors;
}

size graph_out_degree(GenericGraph *graph, GraphNodeId node_id) {
    return VecLen(graph_out_neighbors_ptr(graph, node_id));
}

size graph_in_degree(GenericGraph *graph, GraphNodeId node_id) {
    return VecLen(graph_in_neighbors_ptr(graph, node_id));
}

GraphNodeId graph_neighbor_at(GenericGraph *graph, GraphNodeId from, size neighbor_idx) {
    GraphNeighbors *neighbors;

    ValidateGraph(graph);
    graph_validate_node_id(graph, from);
    neighbors = graph_out_neighbors_ptr(graph, from);

    if (neighbor_idx >= VecLen(neighbors)) {
        LOG_FATAL("graph neighbor index out of bounds");
    }

    return VecAt(neighbors, neighbor_idx);
}

GraphNodeId graph_predecessor_at(GenericGraph *graph, GraphNodeId to, size predecessor_idx) {
    GraphNeighbors *neighbors;

    ValidateGraph(graph);
    graph_validate_node_id(graph, to);
    neighbors = graph_in_neighbors_ptr(graph, to);

    if (predecessor_idx >= VecLen(neighbors)) {
        LOG_FATAL("graph predecessor index out of bounds");
    }

    return VecAt(neighbors, predecessor_idx);
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
    GraphNeighbors *out_neighbors;
    GraphNeighbors *in_neighbors;

    ValidateGraph(graph);
    graph_validate_node_id(graph, from);
    graph_validate_node_id(graph, to);

    out_neighbors = graph_out_neighbors_ptr(graph, from);
    if (graph_neighbors_contains(out_neighbors, to)) {
        return false;
    }

    in_neighbors = graph_in_neighbors_ptr(graph, to);
    if (!insert_range_into_vec(
            GENERIC_VEC(out_neighbors),
            (const u8 *)&to,
            sizeof(GraphNodeId),
            VecLen(out_neighbors),
            1
        )) {
        return false;
    }
    if (!insert_range_into_vec(
            GENERIC_VEC(in_neighbors),
            (const u8 *)&from,
            sizeof(GraphNodeId),
            VecLen(in_neighbors),
            1
        )) {
        remove_range_vec(GENERIC_VEC(out_neighbors), NULL, sizeof(GraphNodeId), VecLen(out_neighbors) - 1, 1);
        return false;
    }
    graph->edge_count += 1;
    graph_bump_mutation_epoch(graph);
    return true;
}

u64 graph_node_visit(GraphNode node) {
    GenericGraph     *graph;
    GenericGraphSlot *slot;

    node  = graph_validate_node_handle(node);
    graph = GENERIC_GRAPH(node._graph_);
    slot  = graph_require_live_slot(graph, node._id_);

    if (slot->visit_count == UINT64_MAX) {
        LOG_FATAL("graph node visit count overflow");
    }

    slot->visit_count += 1;
    return slot->visit_count;
}

void graph_node_unvisit(GraphNode node) {
    GenericGraph     *graph;
    GenericGraphSlot *slot;

    node  = graph_validate_node_handle(node);
    graph = GENERIC_GRAPH(node._graph_);
    slot  = graph_require_live_slot(graph, node._id_);

    slot->visit_count = 0;
}

u64 graph_node_visit_count(GraphNode node) {
    GenericGraph           *graph;
    const GenericGraphSlot *slot;

    node  = graph_validate_node_handle(node);
    graph = GENERIC_GRAPH(node._graph_);
    slot  = graph_require_live_slot_const(graph, node._id_);
    return slot->visit_count;
}

bool graph_node_visited(GraphNode node) {
    return graph_node_visit_count(node) > 0;
}

bool graph_mark_node_for_deletion(GraphNode node) {
    GenericGraph     *graph;
    GenericGraphSlot *slot;

    node  = graph_validate_node_handle(node);
    graph = GENERIC_GRAPH(node._graph_);
    slot  = graph_require_live_slot(graph, node._id_);

    if (graph_slot_is_marked(slot)) {
        return false;
    }

    slot->flags                 |= GRAPH_SLOT_MARKED;
    graph->pending_delete_count += 1;
    return true;
}

bool graph_node_marked_for_deletion(GraphNode node) {
    GenericGraph           *graph;
    const GenericGraphSlot *slot;

    node  = graph_validate_node_handle(node);
    graph = GENERIC_GRAPH(node._graph_);
    slot  = graph_require_live_slot_const(graph, node._id_);

    return graph_slot_is_marked(slot);
}

bool graph_unmark_node_for_deletion(GraphNode node) {
    GenericGraph     *graph;
    GenericGraphSlot *slot;

    node  = graph_validate_node_handle(node);
    graph = GENERIC_GRAPH(node._graph_);
    slot  = graph_require_live_slot(graph, node._id_);

    if (!graph_slot_is_marked(slot)) {
        return false;
    }

    slot->flags                 &= ~GRAPH_SLOT_MARKED;
    graph->pending_delete_count -= 1;
    return true;
}

bool graph_mark_edge_for_removal(GenericGraph *graph, GraphNodeId from, GraphNodeId to) {
    GraphPendingEdgeRemoval pending;

    ValidateGraph(graph);
    graph_validate_node_id(graph, from);
    graph_validate_node_id(graph, to);

    if (!graph_has_edge(graph, from, to)) {
        return false;
    }

    if (graph_find_pending_edge_removal_index(graph, from, to) != SIZE_MAX) {
        return false;
    }

    pending.from = from;
    pending.to   = to;
    return insert_range_into_vec(
        GENERIC_VEC(&graph->pending_edge_removals),
        (const u8 *)&pending,
        sizeof(GraphPendingEdgeRemoval),
        VecLen(&graph->pending_edge_removals),
        1
    );
}

bool graph_edge_marked_for_removal(GenericGraph *graph, GraphNodeId from, GraphNodeId to) {
    ValidateGraph(graph);
    graph_validate_node_id(graph, from);
    graph_validate_node_id(graph, to);

    return graph_find_pending_edge_removal_index(graph, from, to) != SIZE_MAX;
}

bool graph_unmark_edge_for_removal(GenericGraph *graph, GraphNodeId from, GraphNodeId to) {
    size idx;

    ValidateGraph(graph);
    graph_validate_node_id(graph, from);
    graph_validate_node_id(graph, to);

    idx = graph_find_pending_edge_removal_index(graph, from, to);
    if (idx == SIZE_MAX) {
        return false;
    }

    remove_range_vec(GENERIC_VEC(&graph->pending_edge_removals), NULL, sizeof(GraphPendingEdgeRemoval), idx, 1);
    return true;
}

u64 graph_commit_changes(GenericGraph *graph, size item_size) {
    u64 removed_count = 0;
    u64 slot_index;
    u64 edge_idx;
    u64 explicit_edge_removal_count;

    ValidateGraph(graph);

    if (!graph->pending_delete_count && !VecLen(&graph->pending_edge_removals)) {
        return 0;
    }

    explicit_edge_removal_count = VecLen(&graph->pending_edge_removals);
    for (edge_idx = 0; edge_idx < VecLen(&graph->pending_edge_removals); edge_idx++) {
        GraphPendingEdgeRemoval *pending = VecPtrAt(&graph->pending_edge_removals, edge_idx);
        (void)graph_remove_edge_now(graph, pending->from, pending->to);
    }
    clear_vec(GENERIC_VEC(&graph->pending_edge_removals), sizeof(GraphPendingEdgeRemoval));

    for (slot_index = 0; slot_index < VecLen(&graph->slots); slot_index++) {
        GenericGraphSlot *slot = VecPtrAt(&graph->slots, slot_index);
        if (graph_slot_is_occupied(slot) && graph_slot_is_marked(slot)) {
            GraphNodeId marked_id = graph_make_node_id((u32)slot_index, slot->generation);
            (void)graph_remove_all_outgoing_edges(graph, marked_id);
        }
    }

    for (slot_index = 0; slot_index < VecLen(&graph->slots); slot_index++) {
        GenericGraphSlot *slot = VecPtrAt(&graph->slots, slot_index);
        if (graph_slot_is_occupied(slot) && !graph_slot_is_marked(slot)) {
            GraphNodeId live_id = graph_make_node_id((u32)slot_index, slot->generation);
            (void)graph_remove_marked_outgoing_edges(graph, live_id);
        }
    }

    for (slot_index = 0; slot_index < VecLen(&graph->slots); slot_index++) {
        GenericGraphSlot *slot = VecPtrAt(&graph->slots, slot_index);
        if (graph_slot_is_occupied(slot) && graph_slot_is_marked(slot)) {
            if (VecLen(&slot->out_neighbors) || VecLen(&slot->in_neighbors)) {
                LOG_FATAL("Graph marked node retained incident edges before release");
            }
            graph_release_slot(graph, slot, item_size);
            graph_push_free_index(graph, (u32)slot_index);
            graph->live_count           -= 1;
            graph->pending_delete_count -= 1;
            removed_count               += 1;
        }
    }

    graph_bump_mutation_epoch(graph);
    return removed_count + explicit_edge_removal_count;
}

GenericGraphNodeIter graph_node_iter_begin(GenericGraph *graph) {
    GenericGraphNodeIter iter;

    ValidateGraph(graph);

    iter.graph                   = graph;
    iter.slot_index              = 0;
    iter.expected_mutation_epoch = graph->mutation_epoch;
    return iter;
}

bool graph_node_iter_next(GenericGraphNodeIter *iter, GraphNode *out_node) {
    if (!iter || !out_node) {
        LOG_FATAL("invalid arguments");
    }

    if (!iter->graph) {
        return false;
    }

    ValidateGraph(iter->graph);
    if (iter->expected_mutation_epoch != iter->graph->mutation_epoch) {
        LOG_FATAL("graph structure changed during node iteration");
    }

    while (iter->slot_index < VecLen(&iter->graph->slots)) {
        u32               index  = (u32)iter->slot_index;
        GenericGraphSlot *slot   = VecPtrAt(&iter->graph->slots, iter->slot_index);
        iter->slot_index        += 1;

        if (graph_slot_is_occupied(slot)) {
            out_node->_graph_ = iter->graph;
            out_node->_id_    = graph_make_node_id(index, slot->generation);
            return true;
        }
    }

    return false;
}

GenericGraphNeighborIter graph_neighbor_iter_begin(GraphNode node) {
    GenericGraphNeighborIter iter;
    GenericGraph            *graph;

    node  = graph_validate_node_handle(node);
    graph = GENERIC_GRAPH(node._graph_);

    iter.graph                   = graph;
    iter.source_id               = node._id_;
    iter.neighbor_index          = 0;
    iter.expected_mutation_epoch = graph->mutation_epoch;
    return iter;
}

bool graph_neighbor_iter_next(GenericGraphNeighborIter *iter, GraphNode *out_node) {
    GraphNeighbors *neighbors;

    if (!iter || !out_node) {
        LOG_FATAL("invalid arguments");
    }

    if (!iter->graph) {
        return false;
    }

    ValidateGraph(iter->graph);
    if (iter->expected_mutation_epoch != iter->graph->mutation_epoch) {
        LOG_FATAL("graph structure changed during neighbor iteration");
    }

    graph_validate_node_id(iter->graph, iter->source_id);
    neighbors = graph_out_neighbors_ptr(iter->graph, iter->source_id);

    while (iter->neighbor_index < VecLen(neighbors)) {
        GraphNodeId neighbor_id  = VecAt(neighbors, iter->neighbor_index);
        iter->neighbor_index    += 1;

        graph_validate_node_id(iter->graph, neighbor_id);

        out_node->_graph_ = iter->graph;
        out_node->_id_    = neighbor_id;
        return true;
    }

    return false;
}

GenericGraphPredecessorIter graph_predecessor_iter_begin(GraphNode node) {
    GenericGraphPredecessorIter iter;
    GenericGraph               *graph;

    node  = graph_validate_node_handle(node);
    graph = GENERIC_GRAPH(node._graph_);

    iter.graph                   = graph;
    iter.target_id               = node._id_;
    iter.predecessor_index       = 0;
    iter.expected_mutation_epoch = graph->mutation_epoch;
    return iter;
}

bool graph_predecessor_iter_next(GenericGraphPredecessorIter *iter, GraphNode *out_node) {
    GraphNeighbors *neighbors;

    if (!iter || !out_node) {
        LOG_FATAL("invalid arguments");
    }

    if (!iter->graph) {
        return false;
    }

    ValidateGraph(iter->graph);
    if (iter->expected_mutation_epoch != iter->graph->mutation_epoch) {
        LOG_FATAL("graph structure changed during predecessor iteration");
    }

    graph_validate_node_id(iter->graph, iter->target_id);
    neighbors = graph_in_neighbors_ptr(iter->graph, iter->target_id);

    while (iter->predecessor_index < VecLen(neighbors)) {
        GraphNodeId predecessor_id  = VecAt(neighbors, iter->predecessor_index);
        iter->predecessor_index    += 1;

        graph_validate_node_id(iter->graph, predecessor_id);

        out_node->_graph_ = iter->graph;
        out_node->_id_    = predecessor_id;
        return true;
    }

    return false;
}
