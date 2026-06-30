#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Std/Container/Graph.h>
#include <Misra/Std/Log.h>
#include <Misra/Types.h>

#include "../Util/TestRunner.h"

// Build a raw GraphNodeId from an explicit (index, generation) pair. A
// GraphNodeId is a u64 laid out as (generation << 32) | index -- see
// GraphNodeIdIndex / GraphNodeIdGeneration in Graph/Type.h.
static GraphNodeId make_raw_node_id(u32 index, u32 generation) {
    return ((GraphNodeId)generation << 32) | (GraphNodeId)index;
}

static bool test_graph_access_helpers(void) {
    WriteFmt("Testing Graph access helpers\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    GraphNodeId a = GraphAddNodeR(&graph, 10);
    GraphNodeId b = GraphAddNodeR(&graph, 20);
    GraphNodeId c = GraphAddNodeR(&graph, 30);
    GraphNode   node_b;
    GraphAddEdge(&graph, a, b);
    GraphAddEdge(&graph, a, c);
    GraphAddEdge(&graph, c, a);

    ValidateGraph(&graph);

    node_b                            = GraphGetNode(&graph, b);
    *GraphNodeDataPtr(&graph, node_b) = 25;

    bool result = GraphNodeCount(&graph) == 3 && GraphEdgeCount(&graph) == 3 && !GraphEmpty(&graph);
    result = result && GraphContainsNode(&graph, a) && GraphContainsNode(&graph, b) && GraphContainsNode(&graph, c);
    result = result && GraphNodeAt(&graph, b) == 25;
    result = result && GraphNodeData(&graph, node_b) == 25;
    result = result && GraphNodeGetId(node_b) == b;
    result = result && GraphNodeIndex(node_b) == GraphNodeIdIndex(b);
    result = result && GraphOutDegree(&graph, a) == 2;
    result = result && GraphInDegree(&graph, a) == 1;
    result = result && GraphInDegree(&graph, b) == 1;
    result = result && GraphInDegree(&graph, c) == 1;
    result = result && GraphNeighborAt(&graph, a, 0) == b && GraphNeighborAt(&graph, a, 1) == c;
    result = result && GraphNeighborAt(&graph, c, 0) == a;
    result = result && GraphPredecessorAt(&graph, a, 0) == c;
    result = result && GraphPredecessorAt(&graph, b, 0) == a;
    result = result && GraphPredecessorAt(&graph, c, 0) == a;

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_graph_has_edge_query(void) {
    WriteFmt("Testing GraphHasEdge\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(Zstr) ZstrGraph;
    ZstrGraph graph = GraphInit(&alloc);

    GraphNodeId red   = GraphAddNodeR(&graph, "red");
    GraphNodeId green = GraphAddNodeR(&graph, "green");
    GraphNodeId blue  = GraphAddNodeR(&graph, "blue");

    GraphAddEdge(&graph, red, green);
    GraphAddEdge(&graph, green, blue);

    bool result = GraphHasEdge(&graph, red, green);
    result      = result && GraphHasEdge(&graph, green, blue);
    result      = result && !GraphHasEdge(&graph, blue, green);
    result      = result && !GraphHasEdge(&graph, red, blue);
    result      = result && (ZstrCompare(*GraphNodePtrAt(&graph, red), "red") == 0);

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_graph_cross_graph_node_handle_deadend(void) {
    WriteFmt("Testing GraphNodeData rejects foreign graph node handles (should abort)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph  graph_a = GraphInit(&alloc);
    IntGraph  graph_b = GraphInit(&alloc);
    GraphNode node    = GraphGetNode(&graph_a, GraphAddNodeR(&graph_a, 10));

    (void)GraphNodeData(&graph_b, node);

    GraphDeinit(&graph_b);
    GraphDeinit(&graph_a);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

static bool test_graph_predecessor_access_oob_deadend(void) {
    WriteFmt("Testing GraphPredecessorAt out-of-bounds access (should abort)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    GraphNodeId a = GraphAddNodeR(&graph, 10);
    GraphNodeId b = GraphAddNodeR(&graph, 20);

    GraphAddEdge(&graph, a, b);
    (void)GraphPredecessorAt(&graph, a, 0);

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

static bool test_graph_neighbor_access_oob_deadend(void) {
    WriteFmt("Testing GraphNeighborAt out-of-bounds access (should abort)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    GraphNodeId a = GraphAddNodeR(&graph, 10);
    GraphNodeId b = GraphAddNodeR(&graph, 20);

    GraphAddEdge(&graph, a, b);
    (void)GraphNeighborAt(&graph, b, 0);

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// graph_contains_node line 638: the bounds guard is `index >= VecLen(slots)`.
// A node id whose index equals the slot count is out of bounds and must read
// as "not contained". The `>=`->`>` mutant treats index==count as in bounds and
// dereferences slot[count].
//
// A naive "id one past the last slot" probe does NOT kill the mutant: slot[len]
// (still within capacity) reads as zeroed/unoccupied, so the mutant returns the
// same `false`. To make the boundary observable we shrink the slot-array length
// by one so a genuinely OCCUPIED slot (b, with a matching generation) now sits
// at index == the new length. Real code: index >= len => not contained (false).
// Mutant: index > len is false, so it reads b's still-occupied slot and reports
// it as contained (true) -- a deterministic divergence.
static bool test_contains_node_rejects_index_equal_to_slot_count(void) {
    WriteFmt("Testing GraphContainsNode rejects index == slot count\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    GraphNodeId a = GraphAddNodeR(&graph, 10);
    GraphNodeId b = GraphAddNodeR(&graph, 20);

    bool result = GraphContainsNode(&graph, a);
    result      = result && GraphContainsNode(&graph, b);

    // intentional bypass: no public setter shrinks the slot array. Dropping one
    // from the length makes b's index (1) equal the new slot count (1) while b's
    // slot stays occupied just past the new logical end.
    graph.slots.length -= 1;

    // Real code rejects b as out of bounds; the `>`-mutant reads the occupied
    // slot and wrongly reports it contained.
    result = result && !GraphContainsNode(&graph, b);

    // Restore the length so GraphDeinit walks a consistent slot array.
    graph.slots.length += 1;

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// graph_contains_node line 643: the occupancy term `graph_slot_is_occupied(slot)`
// gates the generation comparison. After a node is deleted and committed, its
// slot is free but retains a (bumped) generation. A query id matching that free
// slot's current generation must read as "not contained" because the slot is
// unoccupied. The cxx_replace_scalar_call mutant forces the occupancy term to a
// truthy constant, so the free slot whose generation matches would wrongly
// report as contained.
static bool test_contains_node_free_slot_with_matching_generation(void) {
    WriteFmt("Testing GraphContainsNode rejects a freed slot whose generation matches\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    GraphNodeId a = GraphAddNodeR(&graph, 10);

    bool result = GraphContainsNode(&graph, a);

    // Delete + commit: slot 0 becomes free and its generation is bumped to
    // GraphNodeIdGeneration(a) + 1.
    result = result && GraphMarkNodeForDeletion(GraphGetNode(&graph, a));
    result = result && (GraphCommitChanges(&graph) == 1);
    result = result && !GraphContainsNode(&graph, a);

    // Construct an id that targets the now-free slot 0 with its current
    // generation. Real code: slot is unoccupied => not contained. Mutant:
    // occupancy term forced truthy + generation matches => wrongly contained.
    GraphNodeId free_slot_id = make_raw_node_id(GraphNodeIdIndex(a), GraphNodeIdGeneration(a) + 1);
    result                   = result && !GraphContainsNode(&graph, free_slot_id);

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// graph_validate_node_index_raw (51: >= -> >). The bounds check that rejects a
// node index equal to the slot count is load-bearing: weakening it to `>` lets
// an index one past the valid range through to a raw slot dereference. We
// corrupt the slot-array length so an existing live node's index now equals the
// slot count, then look it up: real code rejects (abort), the weakened check
// admits it.
static bool test_graph_node_index_equal_to_slot_count_deadend(void) {
    WriteFmt("Testing node index equal to slot count is rejected (should abort)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    GraphNodeId a = GraphAddNodeR(&graph, 10);
    GraphNodeId b = GraphAddNodeR(&graph, 20);
    (void)a;

    // Drop the validator's memoized bit so the upcoming GraphGetNode does not
    // re-run the deep slot-accounting scan (which would itself reject the
    // corruption below in both real and mutated builds).
    (void)GraphContainsNode(&graph, b);

    // intentional bypass: no public setter shrinks the slot array. Removing one
    // slot from the length makes `b`'s index equal the new slot count so the
    // bounds check in graph_validate_node_index_raw is exercised exactly at its
    // boundary.
    graph.slots.length -= 1;

    (void)GraphGetNode(&graph, b);

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// graph_get_node line 650: the only validation graph_get_node performs is
// graph_validate_node_id. Removing it lets a stale id mint a live-looking handle
// without aborting. Real code aborts on the stale id.
static bool test_get_node_rejects_stale_id_deadend(void) {
    WriteFmt("Testing GraphGetNode rejects a stale node id (should abort)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    GraphNodeId a = GraphAddNodeR(&graph, 10);

    (void)GraphMarkNodeForDeletion(GraphGetNode(&graph, a));
    (void)GraphCommitChanges(&graph);

    // a now refers to a freed slot with a stale generation.
    (void)GraphGetNode(&graph, a);

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// graph_has_edge line 720: validation of the destination `to`. The destination
// is only ever compared by value inside graph_neighbors_contains, which never
// validates it. Removing the `to` validation lets an invalid destination return
// a silent `false` instead of aborting on the caller bug.
static bool test_has_edge_rejects_invalid_destination_deadend(void) {
    WriteFmt("Testing GraphHasEdge rejects an invalid destination id (should abort)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    GraphNodeId a = GraphAddNodeR(&graph, 10);

    // index 5 does not exist; generation 1 is otherwise plausible.
    GraphNodeId bogus_to = make_raw_node_id(5, 1);

    (void)GraphHasEdge(&graph, a, bogus_to);

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// GraphNodeAt routes through graph_require_live_slot, whose only validation is
// the graph_validate_node_id call (mutant at 89 removes it). A stale id must
// abort. Without the validation it would read the live occupant's data and
// return without aborting.
static bool test_node_at_stale_id_deadend(void) {
    WriteFmt("Testing GraphNodeAt rejects a stale node id (should abort)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    GraphNodeId a = GraphAddNodeR(&graph, 10);

    (void)GraphMarkNodeForDeletion(GraphGetNode(&graph, a));
    (void)GraphCommitChanges(&graph);
    // Reuse a's slot at a higher generation; `a` is now stale.
    (void)GraphAddNodeR(&graph, 99);

    (void)GraphNodeAt(&graph, a);

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// ===========================================================================
// 79:10 cxx_replace_scalar_call -- graph_validate_node_id occupancy guard.
//
// `if (!graph_slot_is_occupied(slot))` rejects an id that points at a FREE
// slot. The mutant replaces the occupancy call value with a truthy constant,
// so `!42` is false and the "free slot" abort never fires. We craft an id
// that targets a freed slot but whose generation MATCHES the freed slot's
// current (bumped) generation, so the only guard that can reject it is the
// occupancy check at line 79 (the later generation check at 83 would pass).
// Real code aborts; mutant proceeds. DEADEND.
static bool test_validate_node_id_rejects_free_slot_matching_generation_deadend(void) {
    WriteFmt("Testing graph_validate_node_id rejects a free slot whose generation matches (abort)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    GraphNodeId a = GraphAddNodeR(&graph, 10);

    (void)GraphMarkNodeForDeletion(GraphGetNode(&graph, a));
    (void)GraphCommitChanges(&graph);

    // Slot a's index is now free with generation bumped to gen(a)+1. Build an
    // id with that exact generation: index in bounds, generation nonzero and
    // matching, slot unoccupied -> only the occupancy guard rejects it.
    GraphNodeId free_match = make_raw_node_id(GraphNodeIdIndex(a), GraphNodeIdGeneration(a) + 1);

    (void)GraphGetNode(&graph, free_match);

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

int main(void) {
    TestFunction tests[] = {
        test_graph_access_helpers,
        test_graph_has_edge_query,
        test_contains_node_rejects_index_equal_to_slot_count,
        test_contains_node_free_slot_with_matching_generation,
    };
    TestFunction deadend_tests[] = {
        test_graph_cross_graph_node_handle_deadend,
        test_graph_predecessor_access_oob_deadend,
        test_graph_neighbor_access_oob_deadend,
        test_graph_node_index_equal_to_slot_count_deadend,
        test_get_node_rejects_stale_id_deadend,
        test_has_edge_rejects_invalid_destination_deadend,
        test_node_at_stale_id_deadend,
        test_validate_node_id_rejects_free_slot_matching_generation_deadend,
    };

    WriteFmt("[INFO] Starting Graph.Access tests\n\n");
    return run_test_suite(
        tests,
        (int)(sizeof(tests) / sizeof(tests[0])),
        deadend_tests,
        (int)(sizeof(deadend_tests) / sizeof(deadend_tests[0])),
        "Graph.Access"
    );
}
