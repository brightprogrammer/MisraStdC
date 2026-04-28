/// file      : std/graph.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Generic directed graph implementation

#include <Misra/Std/Container/Graph.h>
#include <Misra/Std/Log.h>
#include <Misra/Sys.h>

#include <stdlib.h>

#define MISRA_GRAPH_SLOT_OCCUPIED ((u32)1u << 0)
#define MISRA_GRAPH_SLOT_MARKED   ((u32)1u << 1)

static GraphNodeId graph_make_node_id(u32 index, u32 generation) {
    return (((u64)generation) << 32) | (u64)index;
}

static bool graph_alignment_is_pow2(u64 alignment) {
    return alignment && ((alignment & (alignment - 1)) == 0);
}

static void graph_validate_alignment(const GenericGraph *graph) {
    if (!graph->alignment) {
        LOG_FATAL("Invalid graph alignment. Did you initialize the graph before use?");
    }

    if ((graph->alignment > 1) && !graph_alignment_is_pow2(graph->alignment)) {
        LOG_FATAL("Graph alignment must be 1 or a power of two");
    }
}

static void graph_validate_slot_limit(const GenericGraph *graph) {
    if (graph->slots.length > (u64)UINT32_MAX) {
        LOG_FATAL("Graph exceeded maximum supported slot count");
    }
}

static bool graph_slot_is_occupied(const GenericGraphSlot *slot) {
    return (slot->flags & MISRA_GRAPH_SLOT_OCCUPIED) != 0;
}

static bool graph_slot_is_marked(const GenericGraphSlot *slot) {
    return (slot->flags & MISRA_GRAPH_SLOT_MARKED) != 0;
}

static void graph_validate_node_index_raw(const GenericGraph *graph, u32 index) {
    if ((u64)index >= graph->slots.length) {
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

    graph = GENERIC_GRAPH(node.__graph);
    if (!graph) {
        LOG_FATAL("invalid graph node handle");
    }

    ValidateGraph(graph);
    graph_validate_node_id(graph, node.__id);
    return node;
}

static void *graph_alloc_node_data(const GenericGraph *graph, size item_size) {
    void *ptr;

    graph_validate_alignment(graph);

    if (graph->alignment <= sizeof(void *)) {
        ptr = calloc(item_size, 1);
        if (!ptr) {
            LOG_SYS_FATAL("calloc() failed");
        }
        return ptr;
    }

    {
        size alignment = (size)graph->alignment;
        size alloc_size = ALIGN_UP_POW2(item_size, alignment);

        ptr = aligned_alloc(alignment, alloc_size);
        if (!ptr) {
            LOG_SYS_FATAL("aligned_alloc() failed");
        }
        memset(ptr, 0, alloc_size);
    }

    return ptr;
}

static void graph_free_node_data(GenericGraph *graph, void *data, size item_size) {
    if (!data) {
        return;
    }

    if (graph->copy_deinit) {
        graph->copy_deinit(data);
    } else {
        memset(data, 0, item_size);
    }

    free(data);
}

static void graph_copy_node_data(GenericGraph *graph, void *dst, const void *src, size item_size) {
    if (graph->copy_init) {
        graph->copy_init(dst, (void *)src);
    } else {
        memcpy(dst, src, item_size);
    }
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
    slot->out_neighbors = VecInitT(slot->out_neighbors);

    slot->visit_count = 0;
    slot->flags       = 0;

    graph_ensure_slot_generation_available(slot);
    slot->generation += 1;
}

static void graph_push_free_index(GenericGraph *graph, u32 index) {
    insert_range_into_vec(GENERIC_VEC(&graph->free_indices), (char *)&index, sizeof(u32), graph->free_indices.length, 1);
}

static u32 graph_take_free_index(GenericGraph *graph) {
    u32 index;

    if (!graph->free_indices.length) {
        LOG_FATAL("expected at least one free graph slot");
    }

    index = VecLast(&graph->free_indices);
    graph->free_indices.length -= 1;
    if (graph->free_indices.data) {
        VecAt(&graph->free_indices, graph->free_indices.length) = 0;
    }

    return index;
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

static size graph_find_neighbor_index(const GraphNeighbors *neighbors, GraphNodeId node_id) {
    size idx;

    for (idx = 0; idx < neighbors->length; idx++) {
        if (VecAt(neighbors, idx) == node_id) {
            return idx;
        }
    }

    return SIZE_MAX;
}

static size graph_find_pending_edge_removal_index(const GenericGraph *graph, GraphNodeId from, GraphNodeId to) {
    size idx;

    for (idx = 0; idx < graph->pending_edge_removals.length; idx++) {
        const GraphPendingEdgeRemoval *pending = VecPtrAt((GraphPendingEdgeRemovals *)&graph->pending_edge_removals, idx);
        if ((pending->from == from) && (pending->to == to)) {
            return idx;
        }
    }

    return SIZE_MAX;
}

static bool graph_remove_edge_now(GenericGraph *graph, GraphNodeId from, GraphNodeId to) {
    GraphNeighbors *neighbors;
    size            idx;

    neighbors = graph_out_neighbors_ptr(graph, from);
    idx       = graph_find_neighbor_index(neighbors, to);

    if (idx == SIZE_MAX) {
        return false;
    }

    remove_range_vec(GENERIC_VEC(neighbors), NULL, sizeof(GraphNodeId), idx, 1);
    graph->edge_count -= 1;
    return true;
}

static size graph_remove_marked_targets_from_neighbors(GenericGraph *graph, GraphNeighbors *neighbors) {
    size removed = 0;
    size write   = 0;
    size read;

    for (read = 0; read < neighbors->length; read++) {
        GraphNodeId             neighbor_id = VecAt(neighbors, read);
        const GenericGraphSlot *slot;

        graph_validate_node_index_raw(graph, GraphNodeIdIndex(neighbor_id));
        slot = graph_slot_ptr_const_raw(graph, GraphNodeIdIndex(neighbor_id));

        if (graph_slot_is_occupied(slot) && !graph_slot_is_marked(slot) &&
            (slot->generation == GraphNodeIdGeneration(neighbor_id))) {
            if (write != read) {
                VecAt(neighbors, write) = neighbor_id;
            }
            write += 1;
        } else {
            removed += 1;
        }
    }

    while (write < neighbors->length) {
        VecAt(neighbors, write) = 0;
        write += 1;
    }

    neighbors->length -= removed;
    if (neighbors->data) {
        VecAt(neighbors, neighbors->length) = 0;
    }

    return removed;
}

void validate_graph(const GenericGraph *graph) {
    u64 live_count   = 0;
    u64 edge_count   = 0;
    u64 marked_count = 0;
    u64 slot_index;
    u64 free_index_i;

    if (!graph) {
        LOG_FATAL("Expected a valid Graph pointer");
    }

    if (graph->__magic != MISRA_GRAPH_MAGIC) {
        LOG_FATAL("Graph is uninitialized or corrupted");
    }

    validate_vec((const GenericVec *)&graph->slots);
    validate_vec((const GenericVec *)&graph->free_indices);
    validate_vec((const GenericVec *)&graph->pending_edge_removals);
    graph_validate_alignment(graph);
    graph_validate_slot_limit(graph);

    if (graph->live_count > graph->slots.length) {
        LOG_FATAL("Graph live node count exceeds slot count");
    }

    if (graph->pending_delete_count > graph->live_count) {
        LOG_FATAL("Graph pending delete count exceeds live node count");
    }

    if ((graph->live_count + graph->free_indices.length) != graph->slots.length) {
        LOG_FATAL("Graph slot accounting is inconsistent");
    }

    for (slot_index = 0; slot_index < graph->slots.length; slot_index++) {
        const GenericGraphSlot *slot = VecPtrAt((GraphSlots *)&graph->slots, slot_index);

        ValidateVec(&slot->out_neighbors);

        if (graph_slot_is_occupied(slot)) {
            if (!slot->data) {
                LOG_FATAL("Occupied graph slot has NULL payload");
            }

            if (!slot->generation) {
                LOG_FATAL("Occupied graph slot has invalid generation");
            }

            live_count += 1;
            edge_count += slot->out_neighbors.length;
            marked_count += graph_slot_is_marked(slot) ? 1 : 0;
        } else {
            if (slot->data) {
                LOG_FATAL("Free graph slot retains payload pointer");
            }

            if (slot->visit_count != 0) {
                LOG_FATAL("Free graph slot retains visit count");
            }

            if (slot->out_neighbors.length != 0) {
                LOG_FATAL("Free graph slot retains outgoing edges");
            }
        }
    }

    for (free_index_i = 0; free_index_i < graph->free_indices.length; free_index_i++) {
        u32 index = VecAt(&graph->free_indices, free_index_i);
        if ((u64)index >= graph->slots.length) {
            LOG_FATAL("Graph free slot index out of bounds");
        }

        if (graph_slot_is_occupied(VecPtrAt((GraphSlots *)&graph->slots, index))) {
            LOG_FATAL("Graph free index points to an occupied slot");
        }
    }

    for (free_index_i = 0; free_index_i < graph->pending_edge_removals.length; free_index_i++) {
        const GraphPendingEdgeRemoval *pending = VecPtrAt((GraphPendingEdgeRemovals *)&graph->pending_edge_removals, free_index_i);
        const GraphNeighbors         *neighbors;

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

    if (graph->edge_count != edge_count) {
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

    graph->copy_init            = NULL;
    graph->copy_deinit          = NULL;
    graph->live_count           = 0;
    graph->edge_count           = 0;
    graph->pending_delete_count = 0;
    graph->mutation_epoch       = 0;
    graph->alignment            = 0;
    graph->type_anchor          = NULL;
    graph->__magic              = 0;
}

void clear_graph(GenericGraph *graph, size item_size) {
    u64 slot_index;

    ValidateGraph(graph);

    for (slot_index = 0; slot_index < graph->slots.length; slot_index++) {
        GenericGraphSlot *slot = VecPtrAt(&graph->slots, slot_index);
        if (graph_slot_is_occupied(slot)) {
            graph_release_slot(graph, slot, item_size);
        } else {
            deinit_vec(GENERIC_VEC(&slot->out_neighbors), sizeof(GraphNodeId));
            slot->out_neighbors = VecInitT(slot->out_neighbors);
            slot->visit_count   = 0;
            slot->flags         = 0;
        }
    }

    clear_vec(GENERIC_VEC(&graph->free_indices), sizeof(u32));
    clear_vec(GENERIC_VEC(&graph->pending_edge_removals), sizeof(GraphPendingEdgeRemoval));
    for (slot_index = 0; slot_index < graph->slots.length; slot_index++) {
        u32 index = (u32)slot_index;
        insert_range_into_vec(GENERIC_VEC(&graph->free_indices), (char *)&index, sizeof(u32), graph->free_indices.length, 1);
    }

    graph->live_count           = 0;
    graph->edge_count           = 0;
    graph->pending_delete_count = 0;
    graph_bump_mutation_epoch(graph);
}

void reserve_graph(GenericGraph *graph, size item_size, size n) {
    size old_capacity;

    (void)item_size;

    ValidateGraph(graph);
    old_capacity = graph->slots.capacity;

    reserve_vec(GENERIC_VEC(&graph->slots), sizeof(GenericGraphSlot), n);
    if (graph->slots.capacity != old_capacity) {
        graph_bump_mutation_epoch(graph);
    }
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

    if (graph->free_indices.length) {
        slot_index = graph_take_free_index(graph);
        slot_ptr   = graph_slot_ptr_raw(graph, slot_index);
        if (graph_slot_is_occupied(slot_ptr)) {
            LOG_FATAL("graph free slot unexpectedly occupied");
        }

        slot_ptr->out_neighbors = VecInitT(slot_ptr->out_neighbors);
        slot_ptr->data          = graph_alloc_node_data(graph, item_size);
        slot_ptr->visit_count   = 0;
        slot_ptr->flags         = MISRA_GRAPH_SLOT_OCCUPIED;
        graph_copy_node_data(graph, slot_ptr->data, item_data, item_size);

        graph->live_count += 1;
        graph_bump_mutation_epoch(graph);
        return graph_make_node_id(slot_index, slot_ptr->generation);
    }

    graph_validate_slot_limit(graph);

    slot.out_neighbors = VecInitT(slot.out_neighbors);
    slot.data          = graph_alloc_node_data(graph, item_size);
    slot.visit_count   = 0;
    slot.generation    = 1;
    slot.flags         = MISRA_GRAPH_SLOT_OCCUPIED;
    graph_copy_node_data(graph, slot.data, item_data, item_size);

    insert_range_into_vec(GENERIC_VEC(&graph->slots), (char *)&slot, sizeof(GenericGraphSlot), graph->slots.length, 1);

    slot_index = (u32)(graph->slots.length - 1);
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

    if (!graph->copy_init) {
        memset(item_data, 0, item_size);
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

    if (!generation || ((u64)index >= graph->slots.length)) {
        return false;
    }

    slot = graph_slot_ptr_raw(graph, index);
    return graph_slot_is_occupied(slot) && (slot->generation == generation);
}

GraphNode graph_get_node(GenericGraph *graph, GraphNodeId node_id) {
    GraphNode node;

    ValidateGraph(graph);
    graph_validate_node_id(graph, node_id);

    node.__graph = graph;
    node.__id    = node_id;
    return node;
}

void *graph_node_ptr_at(GenericGraph *graph, GraphNodeId node_id) {
    return graph_require_live_slot(graph, node_id)->data;
}

void *graph_node_data_ptr_checked(GenericGraph *graph, GraphNode node) {
    GraphNode validated = graph_validate_node_handle(node);

    if (GENERIC_GRAPH(validated.__graph) != graph) {
        LOG_FATAL("graph node handle does not belong to the provided graph");
    }

    return graph_node_ptr_at(graph, validated.__id);
}

GraphNeighbors *graph_out_neighbors_ptr(GenericGraph *graph, GraphNodeId node_id) {
    return &graph_require_live_slot(graph, node_id)->out_neighbors;
}

size graph_out_degree(GenericGraph *graph, GraphNodeId node_id) {
    return graph_out_neighbors_ptr(graph, node_id)->length;
}

GraphNodeId graph_neighbor_at(GenericGraph *graph, GraphNodeId from, size neighbor_idx) {
    GraphNeighbors *neighbors;

    ValidateGraph(graph);
    graph_validate_node_id(graph, from);
    neighbors = graph_out_neighbors_ptr(graph, from);

    if (neighbor_idx >= neighbors->length) {
        LOG_FATAL("graph neighbor index out of bounds");
    }

    return VecAt(neighbors, neighbor_idx);
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
    graph_bump_mutation_epoch(graph);
    return true;
}

u64 graph_node_visit(GraphNode node) {
    GenericGraph     *graph;
    GenericGraphSlot *slot;

    node  = graph_validate_node_handle(node);
    graph = GENERIC_GRAPH(node.__graph);
    slot  = graph_require_live_slot(graph, node.__id);

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
    graph = GENERIC_GRAPH(node.__graph);
    slot  = graph_require_live_slot(graph, node.__id);

    slot->visit_count = 0;
}

u64 graph_node_visit_count(GraphNode node) {
    GenericGraph           *graph;
    const GenericGraphSlot *slot;

    node  = graph_validate_node_handle(node);
    graph = GENERIC_GRAPH(node.__graph);
    slot  = graph_require_live_slot_const(graph, node.__id);
    return slot->visit_count;
}

bool graph_node_visited(GraphNode node) {
    return graph_node_visit_count(node) > 0;
}

bool graph_mark_node_for_deletion(GraphNode node) {
    GenericGraph     *graph;
    GenericGraphSlot *slot;

    node  = graph_validate_node_handle(node);
    graph = GENERIC_GRAPH(node.__graph);
    slot  = graph_require_live_slot(graph, node.__id);

    if (graph_slot_is_marked(slot)) {
        return false;
    }

    slot->flags |= MISRA_GRAPH_SLOT_MARKED;
    graph->pending_delete_count += 1;
    return true;
}

bool graph_node_marked_for_deletion(GraphNode node) {
    GenericGraph           *graph;
    const GenericGraphSlot *slot;

    node  = graph_validate_node_handle(node);
    graph = GENERIC_GRAPH(node.__graph);
    slot  = graph_require_live_slot_const(graph, node.__id);

    return graph_slot_is_marked(slot);
}

bool graph_unmark_node_for_deletion(GraphNode node) {
    GenericGraph     *graph;
    GenericGraphSlot *slot;

    node  = graph_validate_node_handle(node);
    graph = GENERIC_GRAPH(node.__graph);
    slot  = graph_require_live_slot(graph, node.__id);

    if (!graph_slot_is_marked(slot)) {
        return false;
    }

    slot->flags &= ~MISRA_GRAPH_SLOT_MARKED;
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
    insert_range_into_vec(
        GENERIC_VEC(&graph->pending_edge_removals),
        (char *)&pending,
        sizeof(GraphPendingEdgeRemoval),
        graph->pending_edge_removals.length,
        1
    );

    return true;
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

    if (!graph->pending_delete_count && !graph->pending_edge_removals.length) {
        return 0;
    }

    explicit_edge_removal_count = graph->pending_edge_removals.length;
    for (edge_idx = 0; edge_idx < graph->pending_edge_removals.length; edge_idx++) {
        GraphPendingEdgeRemoval *pending = VecPtrAt(&graph->pending_edge_removals, edge_idx);
        (void)graph_remove_edge_now(graph, pending->from, pending->to);
    }
    clear_vec(GENERIC_VEC(&graph->pending_edge_removals), sizeof(GraphPendingEdgeRemoval));

    for (slot_index = 0; slot_index < graph->slots.length; slot_index++) {
        GenericGraphSlot *slot = VecPtrAt(&graph->slots, slot_index);
        if (graph_slot_is_occupied(slot) && graph_slot_is_marked(slot)) {
            graph->edge_count -= slot->out_neighbors.length;
            deinit_vec(GENERIC_VEC(&slot->out_neighbors), sizeof(GraphNodeId));
            slot->out_neighbors = VecInitT(slot->out_neighbors);
        }
    }

    for (slot_index = 0; slot_index < graph->slots.length; slot_index++) {
        GenericGraphSlot *slot = VecPtrAt(&graph->slots, slot_index);
        if (graph_slot_is_occupied(slot) && !graph_slot_is_marked(slot)) {
            graph->edge_count -= graph_remove_marked_targets_from_neighbors(graph, &slot->out_neighbors);
        }
    }

    for (slot_index = 0; slot_index < graph->slots.length; slot_index++) {
        GenericGraphSlot *slot = VecPtrAt(&graph->slots, slot_index);
        if (graph_slot_is_occupied(slot) && graph_slot_is_marked(slot)) {
            graph_release_slot(graph, slot, item_size);
            graph_push_free_index(graph, (u32)slot_index);
            graph->live_count -= 1;
            graph->pending_delete_count -= 1;
            removed_count += 1;
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

    while (iter->slot_index < iter->graph->slots.length) {
        u32               index = (u32)iter->slot_index;
        GenericGraphSlot *slot  = VecPtrAt(&iter->graph->slots, iter->slot_index);
        iter->slot_index += 1;

        if (graph_slot_is_occupied(slot)) {
            out_node->__graph = iter->graph;
            out_node->__id    = graph_make_node_id(index, slot->generation);
            return true;
        }
    }

    return false;
}

GenericGraphNeighborIter graph_neighbor_iter_begin(GraphNode node) {
    GenericGraphNeighborIter iter;
    GenericGraph            *graph;

    node  = graph_validate_node_handle(node);
    graph = GENERIC_GRAPH(node.__graph);

    iter.graph                   = graph;
    iter.source_id               = node.__id;
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

    while (iter->neighbor_index < neighbors->length) {
        GraphNodeId neighbor_id = VecAt(neighbors, iter->neighbor_index);
        iter->neighbor_index += 1;

        graph_validate_node_id(iter->graph, neighbor_id);

        out_node->__graph = iter->graph;
        out_node->__id    = neighbor_id;
        return true;
    }

    return false;
}
