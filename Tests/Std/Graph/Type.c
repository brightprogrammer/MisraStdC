#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Allocator/Heap.h>
#include <Misra/Std/Container/Graph.h>
#include <Misra/Std/Log.h>
#include <Misra/Types.h>

#include "../../Util/TestRunner.h"

// Fetch the type-erased slot for a node id (slots vec is shared-layout).
static GenericGraphSlot *mutant_slot(void *graph_handle, GraphNodeId id) {
    GenericGraph *g = GENERIC_GRAPH(graph_handle);
    return (GenericGraphSlot *)VecPtrAt(&g->slots, GraphNodeIdIndex(id));
}

static bool test_graph_type_defaults(void) {
    WriteFmt("Testing Graph defaults\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    ValidateGraph(&graph);

    // intentional bypass: the inner `slots` / `free_indices` /
    // `pending_edge_removals` Vec fields and the `pending_delete_count`
    // counter are private graph fields with no public accessor -- they
    // are read directly here to verify the default-constructed graph has
    // every internal cursor / table empty.
    bool result = GraphNodeCount(&graph) == 0 && GraphEdgeCount(&graph) == 0 && GraphEmpty(&graph) &&
                  VecBegin(&graph.slots) == NULL && VecBegin(&graph.free_indices) == NULL &&
                  VecBegin(&graph.pending_edge_removals) == NULL && GraphCopyInit(&graph) == NULL &&
                  GraphCopyDeinit(&graph) == NULL && graph.pending_delete_count == 0 &&
                  GraphMutationEpoch(&graph) == 0 && GraphAllocator(&graph)->alignment == 1;

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_graph_aligned_init_and_id_layout(void) {
    WriteFmt("Testing Graph aligned init and node id layout\n");

    HeapAllocator alloc = HeapAllocatorInitAligned(32);

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    GraphNodeId node_id = GraphAddNodeR(&graph, 11);
    GraphNode   node    = GraphGetNode(&graph, node_id);

    bool result = GraphAllocator(&graph)->alignment == 32 && GraphNodeIdIndex(node_id) == 0 &&
                  GraphNodeIdGeneration(node_id) == 1;
    result = result && GraphNodeGetId(node) == node_id;
    result = result && GraphNodeIndex(node) == 0;
    result = result && GraphContainsNode(&graph, node_id);

    GraphDeinit(&graph);
    HeapAllocatorDeinit(&alloc);
    return result;
}

// Mutant 375:28:cxx_add_assign_to_sub_assign -- `marked_count += ...` becomes
// `marked_count -= ...` in validate_graph's deep body. A valid graph that has
// a node marked for deletion would then underflow marked_count and fail the
// `pending_delete_count == marked_count` cross-check, aborting validation of a
// perfectly valid graph. We force the deep body to run (a structural mutation
// after the mark sets the validated bit while b stays marked) and assert that
// validating this valid graph does NOT abort. NORMAL test.
static bool test_graph_validate_passes_with_marked_node(void) {
    WriteFmt("Testing deep ValidateGraph accepts a valid graph that has a marked node\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    GraphNodeId a = GraphAddNodeR(&graph, 10);
    GraphNodeId b = GraphAddNodeR(&graph, 20);
    (void)a;

    GraphMarkNodeForDeletion(GraphGetNode(&graph, b));

    // A structural mutation that keeps b marked but re-arms the deep
    // validator (sets the validated bit), so the next ValidateGraph walks the
    // slots and recomputes marked_count.
    GraphNodeId d = GraphAddNodeR(&graph, 40);
    (void)d;

    ValidateGraph(&graph);

    bool result = (graph.pending_delete_count == 1);
    result      = result && (GraphNodeCount(&graph) == 3);

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Mutant 31:20:cxx_gt_to_le, 31:29:cxx_replace_scalar_call, and
// 340:5:cxx_remove_void_call -- all three let a non-power-of-two allocator
// alignment greater than 1 slip past graph_validate_alignment when it runs
// inside validate_graph's deep body. (We drive the deep-body path -- a direct
// ValidateGraph -- rather than an add, because every allocation also routes
// through ValidateAllocator, which independently rejects a non-pow2 alignment;
// the validator's own graph_validate_alignment call is the sole guard on this
// path.) We corrupt the bound allocator's alignment to 3 (a non-pow2 value
// > 1), re-arm the deep validator, and assert ValidateGraph aborts. DEADEND.
static bool test_graph_non_pow2_alignment_rejected_deadend(void) {
    WriteFmt("Testing ValidateGraph rejects a non-power-of-two allocator alignment (should abort)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    (void)GraphAddNodeR(&graph, 10);

    // intentional bypass: there is no public setter for an allocator's
    // alignment, and a non-pow2 alignment cannot arise from a normal init.
    // Write it directly to simulate a corrupted allocator and re-arm the deep
    // validator; graph_validate_alignment inside validate_graph is the only
    // guard on this (non-allocating) path.
    GENERIC_GRAPH(&graph)->__magic    |= MAGIC_VALIDATED_BIT;
    GraphAllocator(&graph)->alignment  = 3;

    ValidateGraph(&graph);

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// Mutant 337:5:cxx_remove_void_call -- removes `validate_vec(&graph->slots)`
// from validate_graph's deep body. A corrupted `slots` vector magic is then
// no longer caught here, and the downstream slot walk (VecLen / VecPtrAt) reads
// it without re-validating its magic. We corrupt the slots-vector magic, re-arm
// the deep validator, and assert ValidateGraph aborts.
//
// NOTE: we deliberately do NOT call GraphDeinit afterwards. GraphDeinit routes
// the corrupted slots vec through deinit_vec -> validate_vec, which would ALSO
// abort and thus mask the removed in-validator check (real code aborts inside
// ValidateGraph and never reaches deinit, so dropping it changes nothing for
// real code while removing the mask for the mutant). DEADEND.
static bool test_graph_validate_catches_corrupt_slots_vec_deadend(void) {
    WriteFmt("Testing ValidateGraph catches a corrupted slots vector (should abort)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    (void)GraphAddNodeR(&graph, 10);

    // intentional bypass: force the deep validator to run on the next call and
    // corrupt the private slots-vector magic, which only validate_graph's
    // validate_vec(&slots) would catch.
    GENERIC_GRAPH(&graph)->__magic       |= MAGIC_VALIDATED_BIT;
    GENERIC_GRAPH(&graph)->slots.__magic  = 0;

    ValidateGraph(&graph);

    return false;
}

// Mutant 338:5:cxx_remove_void_call -- removes
// `validate_vec(&graph->free_indices)`. A corrupted free-indices vector magic
// is then uncaught; the downstream free-index walk (VecLen / VecAt) reads it
// without checking magic. We omit GraphDeinit so its deinit_vec(&free_indices)
// does not mask the removed in-validator check (see slots-vec test). DEADEND.
static bool test_graph_validate_catches_corrupt_free_indices_vec_deadend(void) {
    WriteFmt("Testing ValidateGraph catches a corrupted free_indices vector (should abort)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    (void)GraphAddNodeR(&graph, 10);

    // intentional bypass: corrupt the private free_indices-vector magic that
    // only validate_graph's validate_vec(&free_indices) would catch.
    GENERIC_GRAPH(&graph)->__magic              |= MAGIC_VALIDATED_BIT;
    GENERIC_GRAPH(&graph)->free_indices.__magic  = 0;

    ValidateGraph(&graph);

    return false;
}

// Mutant 339:5:cxx_remove_void_call -- removes
// `validate_vec(&graph->pending_edge_removals)`. A corrupted
// pending-edge-removals vector magic is then uncaught. We omit GraphDeinit so
// its deinit_vec(&pending_edge_removals) does not mask the removed in-validator
// check (see slots-vec test). DEADEND.
static bool test_graph_validate_catches_corrupt_pending_edges_vec_deadend(void) {
    WriteFmt("Testing ValidateGraph catches a corrupted pending_edge_removals vector (should abort)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    (void)GraphAddNodeR(&graph, 10);

    // intentional bypass: corrupt the private pending_edge_removals-vector
    // magic that only validate_graph's validate_vec(&pending_edge_removals)
    // would catch.
    GENERIC_GRAPH(&graph)->__magic                       |= MAGIC_VALIDATED_BIT;
    GENERIC_GRAPH(&graph)->pending_edge_removals.__magic  = 0;

    ValidateGraph(&graph);

    return false;
}

// These tests target validate_graph's deep consistency body. Each one
// corrupts the graph's private bookkeeping through an intentional bypass
// (there is no public surface that produces an inconsistent graph) and
// then asserts that ValidateGraph aborts. The corruptions are crafted so
// that ONLY the targeted check fires: the edge counts stay balanced and
// the dual (in/out) reverse-adjacency loop accepts the corruption (via a
// duplicated-but-valid neighbor entry), leaving exactly one validator
// branch responsible for catching the inconsistency.

// ---- out-loop (slot.out_neighbors -> target.in_neighbors contains self) ----

// Corruption: edges a->b and c->b. Rewrite b.in_neighbors so it lists [c, c]
// instead of [a, c]. The in-loop accepts both c entries (c->b is real), counts
// stay balanced, but a->b's reverse predecessor (a) is now missing from
// b.in_neighbors -- caught only by the out-loop at 383 (and skipped wholesale
// if its index starts past the end). Kills 377:29 and 383:22.
static bool test_graph_out_reverse_missing_predecessor_deadend(void) {
    WriteFmt("Testing validate_graph out-loop catches missing reverse predecessor (should abort)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    GraphNodeId a = GraphAddNodeR(&graph, 10);
    GraphNodeId b = GraphAddNodeR(&graph, 20);
    GraphNodeId c = GraphAddNodeR(&graph, 30);

    GraphAddEdge(&graph, a, b);
    GraphAddEdge(&graph, c, b);

    GenericGraphSlot *b_slot = mutant_slot(&graph, b);
    // b.in_neighbors is [a, c]; overwrite the 'a' entry with 'c' -> [c, c].
    *VecPtrAt(&b_slot->in_neighbors, 0) = c;

    MAGIC_MARK_DIRTY(GENERIC_GRAPH(&graph));
    ValidateGraph(&graph);

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// Corruption: edges a->b, a->c, b->c. So a.out_neighbors = [b, c]. Rewrite
// c.in_neighbors from [a, b] to [b, b], dropping a. The reverse of a->b (index
// 0 of a.out) stays valid; only the reverse of a->c (index 1 of a.out) is
// broken. A loop that visits only index 0 misses it. Kills 377:87 (i++ -> i--).
static bool test_graph_out_reverse_second_neighbor_deadend(void) {
    WriteFmt("Testing validate_graph out-loop visits every out-neighbor (should abort)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    GraphNodeId a = GraphAddNodeR(&graph, 10);
    GraphNodeId b = GraphAddNodeR(&graph, 20);
    GraphNodeId c = GraphAddNodeR(&graph, 30);

    GraphAddEdge(&graph, a, b);
    GraphAddEdge(&graph, a, c);
    GraphAddEdge(&graph, b, c);

    GenericGraphSlot *c_slot = mutant_slot(&graph, c);
    // c.in_neighbors is [a, b]; overwrite the 'a' entry with 'b' -> [b, b].
    *VecPtrAt(&c_slot->in_neighbors, 0) = b;

    MAGIC_MARK_DIRTY(GENERIC_GRAPH(&graph));
    ValidateGraph(&graph);

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// ---- in-loop (slot.in_neighbors -> source.out_neighbors contains self) ----

// Corruption: edges b->a and b->c. So b.out_neighbors = [a, c]. Rewrite it to
// [c, c], dropping a. The out-loop accepts both c entries (b->c reverse is
// fine), counts stay balanced, but a.in_neighbors still lists b while b no
// longer lists a as an out-edge -- caught only by the in-loop at 394 (and
// skipped wholesale if its index starts past the end). Kills 388:29 and 394:22.
static bool test_graph_in_reverse_missing_outgoing_deadend(void) {
    WriteFmt("Testing validate_graph in-loop catches missing reverse outgoing edge (should abort)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    GraphNodeId a = GraphAddNodeR(&graph, 10);
    GraphNodeId b = GraphAddNodeR(&graph, 20);
    GraphNodeId c = GraphAddNodeR(&graph, 30);

    GraphAddEdge(&graph, b, a);
    GraphAddEdge(&graph, b, c);

    GenericGraphSlot *b_slot = mutant_slot(&graph, b);
    // b.out_neighbors is [a, c]; overwrite the 'a' entry with 'c' -> [c, c].
    *VecPtrAt(&b_slot->out_neighbors, 0) = c;

    MAGIC_MARK_DIRTY(GENERIC_GRAPH(&graph));
    ValidateGraph(&graph);

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// Corruption: edges b->a, c->a, c->b. So a.in_neighbors = [b, c]. Rewrite
// c.out_neighbors from [a, b] to [b, b], dropping a. The reverse-forward of
// a.in[0] (b->a) stays valid; only a.in[1] (c->a) is broken. A loop that
// visits only index 0 of in_neighbors misses it. Kills 388:86 (i++ -> i--).
static bool test_graph_in_reverse_second_predecessor_deadend(void) {
    WriteFmt("Testing validate_graph in-loop visits every predecessor (should abort)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    GraphNodeId a = GraphAddNodeR(&graph, 10);
    GraphNodeId b = GraphAddNodeR(&graph, 20);
    GraphNodeId c = GraphAddNodeR(&graph, 30);

    GraphAddEdge(&graph, b, a);
    GraphAddEdge(&graph, c, a);
    GraphAddEdge(&graph, c, b);

    GenericGraphSlot *c_slot = mutant_slot(&graph, c);
    // c.out_neighbors is [a, b]; overwrite the 'a' entry with 'b' -> [b, b].
    *VecPtrAt(&c_slot->out_neighbors, 0) = b;

    MAGIC_MARK_DIRTY(GENERIC_GRAPH(&graph));
    ValidateGraph(&graph);

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// ---- free-index loop (417-425) ----

// Corruption: after deleting one node, free_indices holds the freed slot index.
// Rewrite it to point at the surviving (occupied) slot index. Slot accounting
// (live + free == slots) still holds, so this is caught only by the free-index
// loop's "points to occupied slot" check at 423 -- and skipped if the loop
// index starts past the end. Kills 417:23.
static bool test_graph_free_index_points_to_occupied_deadend(void) {
    WriteFmt("Testing validate_graph free-index loop rejects occupied target (should abort)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    GraphNodeId a = GraphAddNodeR(&graph, 10);
    GraphNodeId b = GraphAddNodeR(&graph, 20);

    (void)GraphMarkNodeForDeletion(GraphGetNode(&graph, b));
    (void)GraphCommitChanges(&graph);

    GenericGraph *g = GENERIC_GRAPH(&graph);
    // free_indices now holds b's old slot index; redirect it to a (occupied).
    *VecPtrAt(&g->free_indices, 0) = GraphNodeIdIndex(a);

    MAGIC_MARK_DIRTY(g);
    ValidateGraph(&graph);

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// Corruption: set the free index equal to VecLen(slots), one past the last
// valid slot. The bounds check at 419 (index >= len) must reject it; weakening
// it to (index > len) lets this out-of-bounds index through. Kills 419:24.
static bool test_graph_free_index_out_of_bounds_deadend(void) {
    WriteFmt("Testing validate_graph free-index loop rejects out-of-bounds index (should abort)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    GraphNodeId a = GraphAddNodeR(&graph, 10);
    GraphNodeId b = GraphAddNodeR(&graph, 20);
    (void)a; // a keeps slot 0 occupied; only b is deleted to seed free_indices.

    (void)GraphMarkNodeForDeletion(GraphGetNode(&graph, b));
    (void)GraphCommitChanges(&graph);

    GenericGraph *g = GENERIC_GRAPH(&graph);
    // Point the free index exactly one past the last slot.
    *VecPtrAt(&g->free_indices, 0) = (u32)VecLen(&g->slots);

    MAGIC_MARK_DIRTY(g);
    ValidateGraph(&graph);

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// Corruption: delete two nodes so free_indices holds two entries, then point
// the SECOND free index at the surviving occupied slot. A loop visiting only
// the first free index misses it. Kills 417:85 (i++ -> i--).
static bool test_graph_free_index_second_entry_occupied_deadend(void) {
    WriteFmt("Testing validate_graph free-index loop visits every entry (should abort)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    GraphNodeId a = GraphAddNodeR(&graph, 10);
    GraphNodeId b = GraphAddNodeR(&graph, 20);
    GraphNodeId c = GraphAddNodeR(&graph, 30);

    (void)GraphMarkNodeForDeletion(GraphGetNode(&graph, b));
    (void)GraphMarkNodeForDeletion(GraphGetNode(&graph, c));
    (void)GraphCommitChanges(&graph);

    GenericGraph *g = GENERIC_GRAPH(&graph);
    // free_indices now holds two freed indices; redirect the last to a
    // (occupied) so only a loop that reaches the last entry catches it.
    *VecPtrAt(&g->free_indices, VecLen(&g->free_indices) - 1) = GraphNodeIdIndex(a);

    MAGIC_MARK_DIRTY(g);
    ValidateGraph(&graph);

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// ---- pending-edge-removal loop (428-440) ----

// Corruption: queue a->b for removal, then rewrite the pending entry's 'to'
// field to a live node d that a has no edge to. The "refers to a missing edge"
// check at 437 must fire; it is skipped if the loop index starts past the end.
// Kills 428:23.
static bool test_graph_pending_removal_missing_edge_deadend(void) {
    WriteFmt("Testing validate_graph pending-removal loop rejects missing edge (should abort)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    GraphNodeId a = GraphAddNodeR(&graph, 10);
    GraphNodeId b = GraphAddNodeR(&graph, 20);
    GraphNodeId d = GraphAddNodeR(&graph, 40);

    GraphAddEdge(&graph, a, b);
    (void)GraphMarkEdgeForRemoval(&graph, a, b);

    GenericGraph *g = GENERIC_GRAPH(&graph);
    // pending_edge_removals = [{a, b}]; redirect 'to' to d (no a->d edge).
    VecPtrAt(&g->pending_edge_removals, 0)->to = d;

    MAGIC_MARK_DIRTY(g);
    ValidateGraph(&graph);

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// Corruption: queue both a->b and a->c for removal, then break the SECOND
// pending entry (a->c) by redirecting its 'to' to d. A loop visiting only the
// first pending entry misses it. Kills 428:94 (i++ -> i--).
static bool test_graph_pending_removal_second_entry_deadend(void) {
    WriteFmt("Testing validate_graph pending-removal loop visits every entry (should abort)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    GraphNodeId a = GraphAddNodeR(&graph, 10);
    GraphNodeId b = GraphAddNodeR(&graph, 20);
    GraphNodeId c = GraphAddNodeR(&graph, 30);
    GraphNodeId d = GraphAddNodeR(&graph, 40);

    GraphAddEdge(&graph, a, b);
    GraphAddEdge(&graph, a, c);
    (void)GraphMarkEdgeForRemoval(&graph, a, b);
    (void)GraphMarkEdgeForRemoval(&graph, a, c);

    GenericGraph *g = GENERIC_GRAPH(&graph);
    // pending_edge_removals = [{a, b}, {a, c}]; break the second one.
    VecPtrAt(&g->pending_edge_removals, 1)->to = d;

    MAGIC_MARK_DIRTY(g);
    ValidateGraph(&graph);

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// DEADEND: a pending edge removal whose target edge no longer exists must be
// caught by ValidateGraph. Mutant: validate_graph replaces the
// graph_neighbors_contains result with a truthy constant, so the missing-edge
// check never fires. We corrupt the pending entry's target to a live node that
// is not an out-neighbor and force re-validation; real code aborts.
static bool test_graph_validate_rejects_missing_pending_edge_deadend(void) {
    WriteFmt("Testing ValidateGraph rejects pending removal of a missing edge (should abort)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    GraphNodeId a = GraphAddNodeR(&graph, 10);
    GraphNodeId b = GraphAddNodeR(&graph, 20);
    GraphNodeId c = GraphAddNodeR(&graph, 30);

    (void)GraphAddEdge(&graph, a, b);
    (void)GraphMarkEdgeForRemoval(&graph, a, b);

    // intentional bypass: no public setter rewrites a recorded pending edge
    // removal. Point the single pending entry's target at `c`, which is a live
    // node but not an out-neighbor of `a`, so the recorded removal now refers
    // to a non-existent edge.
    VecPtrAt(&graph.pending_edge_removals, 0)->to = c;

    // Re-arm the memoized deep validation so the next ValidateGraph re-walks
    // the pending list instead of short-circuiting on the validated bit.
    MAGIC_MARK_DIRTY(GENERIC_GRAPH(&graph));

    ValidateGraph(&graph);

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

int main(void) {
    TestFunction tests[] = {
        test_graph_type_defaults,
        test_graph_aligned_init_and_id_layout,
        test_graph_validate_passes_with_marked_node,
    };
    TestFunction deadend_tests[] = {
        test_graph_non_pow2_alignment_rejected_deadend,
        test_graph_validate_catches_corrupt_slots_vec_deadend,
        test_graph_validate_catches_corrupt_free_indices_vec_deadend,
        test_graph_validate_catches_corrupt_pending_edges_vec_deadend,
        test_graph_out_reverse_missing_predecessor_deadend,
        test_graph_out_reverse_second_neighbor_deadend,
        test_graph_in_reverse_missing_outgoing_deadend,
        test_graph_in_reverse_second_predecessor_deadend,
        test_graph_free_index_points_to_occupied_deadend,
        test_graph_free_index_second_entry_occupied_deadend,
        test_graph_free_index_out_of_bounds_deadend,
        test_graph_pending_removal_missing_edge_deadend,
        test_graph_pending_removal_second_entry_deadend,
        test_graph_validate_rejects_missing_pending_edge_deadend,
    };

    WriteFmt("[INFO] Starting Graph.Type tests\n\n");
    return run_test_suite(
        tests,
        (int)(sizeof(tests) / sizeof(tests[0])),
        deadend_tests,
        (int)(sizeof(deadend_tests) / sizeof(deadend_tests[0])),
        "Graph.Type"
    );
}
