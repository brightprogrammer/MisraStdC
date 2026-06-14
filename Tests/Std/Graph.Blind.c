#include <Misra.h>
#include <Misra/Std/Allocator/Debug.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Container/Graph.h>
#include <Misra/Std/Log.h>
#include <Misra/Types.h>

#include "../Util/TestRunner.h"

// Build a raw GraphNodeId from an explicit (index, generation) pair. A
// GraphNodeId is (generation << 32) | index.
static GraphNodeId make_raw_node_id(u32 index, u32 generation) {
    return ((GraphNodeId)generation << 32) | (GraphNodeId)index;
}

// Lean DebugAllocator config used by the leak-detecting tests: no trace
// capture / overflow / freed-history bookkeeping, just live-count tracking.
static DebugAllocator make_lean_debug_allocator(void) {
    DebugAllocatorConfig cfg = {.capture_traces = false, .detect_overflow = false, .track_freed_history = false};
    return DebugAllocatorInitWith(cfg);
}

// Conditionally-failing deep-copy initializer for int payloads. When
// g_fail_copy is set, copy fails (false), driving graph_push_node into its
// copy-failure cleanup branch; otherwise a plain bitwise int copy succeeds.
static bool g_fail_copy = false;

static bool flaky_int_copy(void *dst, const void *src, const Allocator *alloc) {
    (void)alloc;
    if (g_fail_copy) {
        return false;
    }
    *(int *)dst = *(const int *)src;
    return true;
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

// ===========================================================================
// 211:23 cxx_lt_to_le -- graph_find_neighbor_index loop bound.
//
// `for (idx = 0; idx < VecLen(neighbors); idx++)` scans an adjacency list for
// a target id. The `<=` mutant reads one element past the logical end
// (VecAt(neighbors, len)), unchecked. The function is reached through
// graph_remove_edge_now (out-side find at line 241). When the out-side find
// over-scans and reports a spurious match, the in-side find runs on an empty
// reverse list, returns SIZE_MAX, and trips the "reverse adjacency is
// inconsistent" abort (line 249).
//
// We stage that exact condition: a->b edge gives a.out=[b] (buffer holds b),
// b.in=[a]. We mark the edge for removal (pending {a,b}), then surgically set
// a.out.length=0 and b.in.length=0 (leaving the stale b in a.out's buffer at
// index 0) and edge_count=0, so the graph looks edge-free while the pending
// removal still names a->b. On commit, graph_remove_edge_now(a, b):
//   real (`<`): a.out length 0 -> find returns SIZE_MAX -> returns false,
//               benign no-op; the commit completes.
//   mutant (`<=`): reads index 0 (stale b) -> out_idx 0 -> in-side find on
//               empty b.in -> SIZE_MAX -> LOG_FATAL.
// NORMAL: real completes; the mutant aborts (killing the process).
static bool test_find_neighbor_index_no_overscan(void) {
    WriteFmt("Testing graph_find_neighbor_index does not overscan past the adjacency end\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    GraphNodeId a = GraphAddNodeR(&graph, 10);
    GraphNodeId b = GraphAddNodeR(&graph, 20);

    // a.out = [b]; b.in = [a]; the buffers physically hold those ids.
    bool result = GraphAddEdge(&graph, a, b);

    // Stage a pending removal for a->b (the edge is present so this validates).
    result = result && GraphMarkEdgeForRemoval(&graph, a, b);

    // intentional bypass: no public surface stages this exact state. Drop the
    // logical lengths to 0 (stale b remains in a.out's buffer at index 0) so
    // the graph looks edge-free while the pending removal still names a->b.
    GenericGraphSlot *slot_a     = (GenericGraphSlot *)VecPtrAt(&GENERIC_GRAPH(&graph)->slots, GraphNodeIdIndex(a));
    GenericGraphSlot *slot_b     = (GenericGraphSlot *)VecPtrAt(&GENERIC_GRAPH(&graph)->slots, GraphNodeIdIndex(b));
    slot_a->out_neighbors.length = 0;
    slot_b->in_neighbors.length  = 0;
    GENERIC_GRAPH(&graph)->edge_count = 0;

    // Clear the validated bit so commit's entry validation skips the deep scan
    // that would otherwise reject the hand-staged inconsistency.
    GENERIC_GRAPH(&graph)->__magic &= ~MAGIC_VALIDATED_BIT;

    // Real: out-side find returns SIZE_MAX (len 0) -> remove is a no-op; commit
    // returns the explicit-removal count (1). Mutant: over-scan finds stale b,
    // in-side find fails -> abort.
    u64 committed = GraphCommitChanges(&graph);
    result        = result && (committed == 1);

    // Restore a consistent edge so GraphDeinit walks a sane graph.
    slot_a->out_neighbors.length       = 1;
    slot_b->in_neighbors.length        = 1;
    GENERIC_GRAPH(&graph)->edge_count  = 1;
    GENERIC_GRAPH(&graph)->__magic    &= ~MAGIC_VALIDATED_BIT;

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// ===========================================================================
// 978:29 cxx_lt_to_le -- graph_node_iter_next slot-walk bound.
//
// `while (iter->slot_index < VecLen(&iter->graph->slots))` walks every slot.
// The `<=` mutant reads slot[len] (one past the end). We hide an occupied
// slot at index==len by shrinking the slot-array length by one; real code
// stops before it, the mutant reads it as occupied and yields an extra node.
// Caller-observable via the GraphForeachNode visit count.
static bool test_node_iter_no_overscan_extra_slot(void) {
    WriteFmt("Testing GraphForeachNode does not over-walk the slot array\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    (void)GraphAddNodeR(&graph, 10);
    (void)GraphAddNodeR(&graph, 20);

    // Run the deep validator once on the consistent graph so it clears the
    // memoized validated bit; subsequent ValidateGraph calls skip the deep
    // accounting scan that would otherwise reject the length tweak below.
    ValidateGraph(&graph);

    // intentional bypass: no public setter shrinks the slot array. Hiding the
    // last (occupied) slot makes its index equal the new slot count, so the
    // `<` bound excludes it but the `<=` mutant reads it.
    graph.slots.length -= 1;

    u64 visited = 0;
    GraphForeachNode(&graph, node) {
        (void)node;
        visited += 1;
    }

    // Restore the length before teardown so GraphDeinit walks a consistent
    // slot array.
    graph.slots.length += 1;

    // Real code visits only the one slot still within the shrunken length;
    // the mutant visits the hidden occupied slot too.
    bool result = (visited == 1);

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// ===========================================================================
// 927:37 cxx_lt_to_le -- graph_commit_changes pass-2 slot-walk bound
// (`occupied && !marked` -> remove marked outgoing edges of live nodes).
//
// The `<=` mutant reads slot[len]. We hide an occupied, UNMARKED slot at
// index==len; the mutant's pass-2 enters it and calls
// graph_remove_marked_outgoing_edges with an out-of-range id, which routes
// through graph_validate_node_index_raw and aborts. Real code stops before
// the hidden slot. A separate marked node (a) gives commit real work; c (the
// hidden slot) stays live + unmarked under real code.
// NORMAL: real completes (a deleted, c retained); the mutant aborts.
static bool test_commit_pass2_no_overscan(void) {
    WriteFmt("Testing commit pass-2 does not over-walk the slot array\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    GraphNodeId a = GraphAddNodeR(&graph, 10);
    GraphNodeId b = GraphAddNodeR(&graph, 20);
    GraphNodeId c = GraphAddNodeR(&graph, 30);
    (void)b;

    // Mark a (index 0, in range after the shrink) so the commit has a node to
    // delete (pending_delete_count > 0, so it does not early-return).
    (void)GraphMarkNodeForDeletion(GraphGetNode(&graph, a));

    // Clear the memoized validated bit on a consistent graph so commit's entry
    // ValidateGraph skips the deep accounting scan.
    ValidateGraph(&graph);

    // Hide the last slot (c, occupied + unmarked) at index == new length.
    graph.slots.length             -= 1;
    GENERIC_GRAPH(&graph)->__magic &= ~MAGIC_VALIDATED_BIT;

    u64 removed = GraphCommitChanges(&graph);

    // Restore the hidden slot (real code never touched it).
    graph.slots.length             += 1;
    GENERIC_GRAPH(&graph)->__magic &= ~MAGIC_VALIDATED_BIT;

    bool result = (removed == 1);
    result      = result && !GraphContainsNode(&graph, a);
    result      = result && GraphContainsNode(&graph, c);

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// ===========================================================================
// 919:37 cxx_lt_to_le -- graph_commit_changes pass-1 slot-walk bound
// (`occupied && marked` -> strip all outgoing edges of marked nodes).
// 935:37 cxx_lt_to_le -- graph_commit_changes pass-3 slot-walk bound
// (`occupied && marked` -> release the marked slots).
//
// Both passes gate on `occupied && marked`. We hide an occupied, MARKED slot
// at index==len. Pass-1's mutant enters it first and calls
// graph_remove_all_outgoing_edges with an out-of-range id, aborting via
// graph_validate_node_index_raw (this is also what would catch the 935
// over-walk were pass-1 not present). A second, in-range marked node (a)
// keeps commit from early-returning and gives the real passes legitimate
// work; the hidden marked c stays live under real code (it is never reached).
// NORMAL: real completes (a deleted; c still marked + live); the mutant aborts.
static bool test_commit_marked_passes_no_overscan(void) {
    WriteFmt("Testing commit marked passes do not over-walk the slot array\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    GraphNodeId a = GraphAddNodeR(&graph, 10);
    GraphNodeId b = GraphAddNodeR(&graph, 20);
    GraphNodeId c = GraphAddNodeR(&graph, 30);
    (void)b;

    // Mark both a (in range after the shrink) and c (the slot we hide at
    // index == len). pending_delete_count = 2 so commit does real work.
    (void)GraphMarkNodeForDeletion(GraphGetNode(&graph, a));
    (void)GraphMarkNodeForDeletion(GraphGetNode(&graph, c));

    // Clear the memoized validated bit on a consistent graph.
    ValidateGraph(&graph);

    // Hide c (occupied + marked) at index == new length.
    graph.slots.length             -= 1;
    GENERIC_GRAPH(&graph)->__magic &= ~MAGIC_VALIDATED_BIT;

    u64 removed = GraphCommitChanges(&graph);

    // Restore the hidden slot (real code never reached it; c is still marked).
    graph.slots.length             += 1;
    GENERIC_GRAPH(&graph)->__magic &= ~MAGIC_VALIDATED_BIT;

    // Real: only a was deleted; c remains live (and still marked).
    bool result = (removed == 1);
    result      = result && !GraphContainsNode(&graph, a);
    result      = result && GraphContainsNode(&graph, c);
    result      = result && GraphNodeMarkedForDeletion(GraphGetNode(&graph, c));

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

int main(void) {
    TestFunction tests[] = {
        test_push_grow_copy_failure_frees_node_data,
        test_push_reuse_copy_failure_frees_node_data,
        test_node_iter_no_overscan_extra_slot,
        test_find_neighbor_index_no_overscan,
        test_commit_pass2_no_overscan,
        test_commit_marked_passes_no_overscan,
    };
    TestFunction deadend_tests[] = {
        test_validate_node_id_rejects_free_slot_matching_generation_deadend,
    };

    WriteFmt("[INFO] Starting Graph.Blind tests\n\n");
    return run_test_suite(
        tests,
        (int)(sizeof(tests) / sizeof(tests[0])),
        deadend_tests,
        (int)(sizeof(deadend_tests) / sizeof(deadend_tests[0])),
        "Graph.Blind"
    );
}
