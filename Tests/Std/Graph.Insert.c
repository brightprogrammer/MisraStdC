#include <Misra/Std/Allocator/Debug.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Container/Graph.h>
#include <Misra/Std/Log.h>
#include <Misra/Types.h>

#include "../Util/TestRunner.h"

// Conditionally-failing deep-copy initializer for int payloads. When
// `g_fail_copy` is set the copy fails (returns false), which drives
// `graph_push_node` into its copy-failure cleanup branch. Otherwise it
// performs a plain bitwise int copy and succeeds.
static bool g_fail_copy = false;

static bool flaky_int_copy(void *dst, const void *src, const Allocator *alloc) {
    (void)alloc;
    if (g_fail_copy) {
        return false;
    }
    *(int *)dst = *(const int *)src;
    return true;
}

// Lean DebugAllocator config used by the leak-detecting tests: no trace
// capture / overflow / freed-history bookkeeping, just live-count tracking.
static DebugAllocator make_lean_debug_allocator(void) {
    DebugAllocatorConfig cfg = {.capture_traces = false, .detect_overflow = false, .track_freed_history = false};
    return DebugAllocatorInitWith(cfg);
}

static bool test_graph_add_node_semantics(void) {
    WriteFmt("Testing GraphAddNode semantics\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph  = GraphInit(&alloc);
    int      owned  = 42;
    int      shared = 7;

    GraphNodeId id0 = GraphAddNodeL(&graph, owned);
    GraphNodeId id1 = GraphAddNodeR(&graph, shared);

    bool result = GraphNodeIdIndex(id0) == 0 && GraphNodeIdGeneration(id0) == 1 && GraphNodeIdIndex(id1) == 1;
    result      = result && owned == 0 && shared == 7;
    result      = result && GraphNodeAt(&graph, id0) == 42;
    result      = result && GraphNodeAt(&graph, id1) == 7;

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_graph_add_edge_dedup(void) {
    WriteFmt("Testing GraphAddEdge deduplication\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    GraphNodeId a = GraphAddNodeR(&graph, 1);
    GraphNodeId b = GraphAddNodeR(&graph, 2);
    GraphNodeId c = GraphAddNodeR(&graph, 3);

    bool result = GraphAddEdge(&graph, a, b);
    result      = result && GraphAddEdge(&graph, a, c);
    result      = result && !GraphAddEdge(&graph, a, b);
    result      = result && GraphEdgeCount(&graph) == 2;
    result      = result && GraphOutDegree(&graph, a) == 2;
    result      = result && GraphInDegree(&graph, a) == 0;
    result      = result && GraphInDegree(&graph, b) == 1;
    result      = result && GraphInDegree(&graph, c) == 1;
    result      = result && GraphNeighborAt(&graph, a, 0) == b;
    result      = result && GraphNeighborAt(&graph, a, 1) == c;
    result      = result && GraphPredecessorAt(&graph, b, 0) == a;
    result      = result && GraphPredecessorAt(&graph, c, 0) == a;

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_graph_self_loop_and_predecessor_order(void) {
    WriteFmt("Testing Graph self-loop handling and predecessor order\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    GraphNodeId a = GraphAddNodeR(&graph, 1);
    GraphNodeId b = GraphAddNodeR(&graph, 2);
    GraphNodeId c = GraphAddNodeR(&graph, 3);

    bool result = GraphAddEdge(&graph, a, a);
    result      = result && GraphAddEdge(&graph, b, a);
    result      = result && GraphAddEdge(&graph, c, a);
    result      = result && !GraphAddEdge(&graph, a, a);
    result      = result && (GraphEdgeCount(&graph) == 3);
    result      = result && (GraphOutDegree(&graph, a) == 1);
    result      = result && (GraphInDegree(&graph, a) == 3);
    result      = result && (GraphNeighborAt(&graph, a, 0) == a);
    result      = result && (GraphPredecessorAt(&graph, a, 0) == a);
    result      = result && (GraphPredecessorAt(&graph, a, 1) == b);
    result      = result && (GraphPredecessorAt(&graph, a, 2) == c);

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// reserve_graph: a reserve that grows slot capacity must bump the mutation
// epoch (515: != -> ==, 516: removed graph_bump_mutation_epoch). The epoch is
// caller-observable via GraphMutationEpoch and is what traversal helpers use to
// detect concurrent structural mutation.
static bool test_graph_reserve_growth_bumps_epoch(void) {
    WriteFmt("Testing GraphReserve growth bumps the mutation epoch\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    (void)GraphAddNodeR(&graph, 1);

    size old_capacity = VecCapacity(&graph.slots);
    u64  old_epoch    = GraphMutationEpoch(&graph);

    bool grew = GraphReserve(&graph, 1024);

    bool result = grew;
    result      = result && (VecCapacity(&graph.slots) > old_capacity);
    result      = result && (GraphMutationEpoch(&graph) > old_epoch);

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// graph_push_node (564: removed graph_bump_mutation_epoch on the successful
// slot-reuse path). A successful add that reuses a freed slot must bump the
// mutation epoch, observable via GraphMutationEpoch.
static bool test_graph_reuse_add_bumps_epoch(void) {
    WriteFmt("Testing successful slot-reuse add bumps the mutation epoch\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    GraphNodeId a = GraphAddNodeR(&graph, 10);
    GraphNodeId b = GraphAddNodeR(&graph, 20);
    (void)a;

    (void)GraphMarkNodeForDeletion(GraphGetNode(&graph, b));
    (void)GraphCommitChanges(&graph);

    u64         old_epoch = GraphMutationEpoch(&graph);
    GraphNodeId reused    = GraphAddNodeR(&graph, 99);

    bool result = (GraphNodeIdIndex(reused) == GraphNodeIdIndex(b));
    result      = result && (GraphMutationEpoch(&graph) > old_epoch);

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// graph_push_node (559: removed graph_push_free_index on the slot-reuse
// copy-failure path). When a reuse add fails in copy_init, the freed slot must
// be returned to the free list so the next successful add reuses the SAME slot
// index. Removing the push leaks the index, so the next add allocates a fresh
// slot at a different index.
static bool test_graph_failed_reuse_returns_slot_to_free_list(void) {
    WriteFmt("Testing failed reuse add returns the slot to the free list\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInitWithDeepCopy(flaky_int_copy, NULL, &alloc);

    GraphNodeId a = GraphAddNodeR(&graph, 10);
    GraphNodeId b = GraphAddNodeR(&graph, 20);
    (void)a;

    (void)GraphMarkNodeForDeletion(GraphGetNode(&graph, b));
    (void)GraphCommitChanges(&graph);

    g_fail_copy           = true;
    GraphNodeId failed_id = GraphAddNodeR(&graph, 30);
    g_fail_copy           = false;

    GraphNodeId reused = GraphAddNodeR(&graph, 40);

    bool result = (failed_id == 0);
    result      = result && (GraphNodeIdIndex(reused) == GraphNodeIdIndex(b));
    result      = result && (GraphNodeAt(&graph, reused) == 40);
    result      = result && (GraphNodeCount(&graph) == 2);

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// graph_push_node (557: visit_count reset corrupted from 0 to 42 on the
// slot-reuse copy-failure cleanup). A freed slot must carry visit_count 0; the
// validator rejects a free slot that retains a visit count. We force a deep
// re-validation (a subsequent successful edge insert re-dirties the validation
// bit) so the corrupted free slot is rejected.
static bool test_graph_failed_reuse_resets_visit_count(void) {
    WriteFmt("Testing failed reuse add resets the freed slot visit count\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInitWithDeepCopy(flaky_int_copy, NULL, &alloc);

    GraphNodeId a = GraphAddNodeR(&graph, 10);
    GraphNodeId b = GraphAddNodeR(&graph, 20);
    GraphNodeId c = GraphAddNodeR(&graph, 30);

    (void)GraphMarkNodeForDeletion(GraphGetNode(&graph, b));
    (void)GraphCommitChanges(&graph);

    g_fail_copy = true;
    (void)GraphAddNodeR(&graph, 40);
    g_fail_copy = false;

    // Re-dirty the validator so the next ValidateGraph deep-scans every slot.
    bool result = GraphAddEdge(&graph, a, c);

    ValidateGraph(&graph);

    result = result && GraphContainsNode(&graph, a) && GraphContainsNode(&graph, c);
    result = result && !GraphContainsNode(&graph, b);

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// ===========================================================================
// 583:9 cxx_remove_void_call -- graph_push_node GROW-path copy-failure
// cleanup must free the freshly-allocated node-data buffer.
//
// On the slot-grow path, when copy_init fails, line 583
// graph_free_node_data(slot.data) releases the payload buffer before the add
// returns 0. Removing it leaks that buffer. We pre-reserve so the failing add
// does NOT reallocate slot storage, isolating the node-data alloc/free as the
// only allocator activity, and assert the live count is net-zero across the
// failing add. Routed through a DebugAllocator.
static bool test_push_grow_copy_failure_frees_node_data(void) {
    WriteFmt("Testing grow-path copy failure frees the node-data buffer (no leak)\n");

    DebugAllocator dbg = make_lean_debug_allocator();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInitWithDeepCopy(flaky_int_copy, NULL, &dbg);

    // Pre-reserve so the failing add appends a slot WITHOUT reallocating the
    // slot vector; the node-data buffer is then the only allocation in flight.
    bool result = GraphReserve(&graph, 8);

    size before = DebugAllocatorLiveCount(&dbg);

    g_fail_copy           = true;
    GraphNodeId failed_id = GraphAddNodeR(&graph, 42);
    g_fail_copy           = false;

    size after = DebugAllocatorLiveCount(&dbg);

    result = result && (failed_id == 0);
    // Real code frees the node-data buffer on the failure path; mutant leaks it.
    result = result && (after == before);
    result = result && (GraphNodeCount(&graph) == 0);

    GraphDeinit(&graph);
    DebugAllocatorDeinit(&dbg);
    return result;
}

// ===========================================================================
// 551:13 cxx_remove_void_call -- graph_push_node REUSE-path copy-failure
// cleanup must free the freshly-allocated node-data buffer.
//
// On the slot-reuse path, when copy_init fails, line 551
// graph_free_node_data(slot_ptr->data) releases the payload before returning
// 0. Removing it leaks the buffer. We delete+commit a node to seed a free
// slot, then drive a failing reuse add and assert the live count is net-zero.
static bool test_push_reuse_copy_failure_frees_node_data(void) {
    WriteFmt("Testing reuse-path copy failure frees the node-data buffer (no leak)\n");

    DebugAllocator dbg = make_lean_debug_allocator();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInitWithDeepCopy(flaky_int_copy, NULL, &dbg);

    GraphNodeId a = GraphAddNodeR(&graph, 10);
    GraphNodeId b = GraphAddNodeR(&graph, 20);
    (void)a;

    // Delete + commit b so its slot lands on the free list (reuse target).
    bool result = GraphMarkNodeForDeletion(GraphGetNode(&graph, b));
    result      = result && (GraphCommitChanges(&graph) == 1);

    size before = DebugAllocatorLiveCount(&dbg);

    g_fail_copy           = true;
    GraphNodeId failed_id = GraphAddNodeR(&graph, 30); // reuses b's slot, copy fails
    g_fail_copy           = false;

    size after = DebugAllocatorLiveCount(&dbg);

    result = result && (failed_id == 0);
    // Real code frees the node-data buffer; mutant leaks it.
    result = result && (after == before);
    result = result && (GraphNodeCount(&graph) == 1);

    GraphDeinit(&graph);
    DebugAllocatorDeinit(&dbg);
    return result;
}

int main(void) {
    TestFunction tests[] = {
        test_graph_add_node_semantics,
        test_graph_add_edge_dedup,
        test_graph_self_loop_and_predecessor_order,
        test_graph_reserve_growth_bumps_epoch,
        test_graph_reuse_add_bumps_epoch,
        test_graph_failed_reuse_returns_slot_to_free_list,
        test_graph_failed_reuse_resets_visit_count,
        test_push_grow_copy_failure_frees_node_data,
        test_push_reuse_copy_failure_frees_node_data,
    };

    WriteFmt("[INFO] Starting Graph.Insert tests\n\n");
    return run_test_suite(tests, (int)(sizeof(tests) / sizeof(tests[0])), NULL, 0, "Graph.Insert");
}
