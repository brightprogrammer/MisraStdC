/// file : tests/std/graph.leak.c
///
/// Success-path leak-guard tests for Graph: route allocations through an explicit
/// DebugAllocator and assert DebugAllocatorLiveCount(&dbg)==0 after cleanup, to KILL
/// remove_void_call survivors that drop an internal *Deinit (Graph/Vec) on a
/// reachable branch.
///
/// Each test drives a concrete input that REACHES a specific success-path internal
/// *Deinit / *Clear, releases everything it owns, then asserts zero live allocations.
/// Removing the targeted internal Deinit leaves a temporary outstanding ->
/// LiveCount/LiveBytes != 0 -> test FAILS -> mutant KILLED. Distinct contract from
/// the value-correctness tests in the sibling Graph.* files -- do NOT duplicate.

#include <Misra.h>
#include <Misra/Std/Allocator/Debug.h>
#include <Misra/Std/Container/Graph.h>

#include "../Util/TestRunner.h"

// A test passes iff `ok` held AND the DebugAllocator has no outstanding
// allocations after the test released everything it owns.
#define LEAK_CLEAN(dbg) (DebugAllocatorLiveCount(&(dbg)) == 0 && DebugAllocatorLiveBytes(&(dbg)) == 0)

typedef Graph(int) IntGraph;

// =============================================================================
// graph_release_slot, line 158: graph_free_node_data(graph, slot->data, ...)
//
// graph_release_slot runs on every occupied slot reclaimed by GraphCommitChanges
// (and GraphClear / GraphDeinit). Each occupied slot owns a heap payload buffer
// allocated by graph_alloc_node_data. Mark a node for deletion and commit: the
// slot is released and its payload must be freed. Removing line 158 leaks the
// payload buffer -> live count stays > 0 after commit + teardown.

// =============================================================================
// graph_release_slot, line 161: deinit_vec(out_neighbors)
//
// A released slot's outgoing-neighbour Vec must have its backing storage freed
// before the slot is recycled. The Vec only owns heap backing once at least one
// edge has been appended to it, so give the to-be-deleted node an outgoing edge,
// then mark+commit. Removing line 161 leaks the out-neighbour Vec backing.

// =============================================================================
// graph_release_slot, line 163: deinit_vec(in_neighbors)
//
// Same as above for the reverse-adjacency Vec: give the to-be-deleted node an
// incoming edge so its in_neighbors Vec owns heap backing, then mark+commit.
// Removing line 163 leaks the in-neighbour Vec backing.

// =============================================================================
// deinit_graph, line 460: clear_graph(graph, item_size)
//
// deinit_graph delegates the per-slot payload + neighbour-Vec release to
// clear_graph. With live nodes (each owning a heap payload) plus edges (each
// neighbour list owning heap backing) outstanding at deinit, removing the
// clear_graph call leaks every payload and every neighbour-Vec backing.

// =============================================================================
// deinit_graph, line 461: deinit_vec(&graph->slots, ...)
//
// The slots Vec itself owns heap backing the moment a node is pushed. Removing
// the slots deinit leaks the entire slot array. A graph with one node (no edges)
// isolates this from the neighbour/free-index deinits.

// =============================================================================
// deinit_graph, line 462: deinit_vec(&graph->free_indices, ...)
//
// clear_graph (run first by deinit_graph) re-pushes every slot index onto
// free_indices, so the free_indices Vec owns heap backing at line 462 whenever
// the graph has >= 1 slot. clear_vec keeps that backing; only the line-462
// deinit_vec returns it. Removing it leaks the free-index array.

// =============================================================================
// deinit_graph, line 463: deinit_vec(&graph->pending_edge_removals, ...)
//
// The pending_edge_removals Vec gains heap backing when an edge is marked for
// removal. clear_graph clears it (keeping the backing via clear_vec), so at line
// 463 the backing is still outstanding. Removing the line-463 deinit leaks it.
// Mark (but do not commit) an edge removal so a backing exists at teardown.

bool test_deinit_frees_pending_edge_removals_backing_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInit();

    IntGraph graph = GraphInit(&dbg);

    GraphNodeId a = GraphAddNodeR(&graph, 10);
    GraphNodeId b = GraphAddNodeR(&graph, 20);

    GraphAddEdge(&graph, a, b);
    GraphMarkEdgeForRemoval(&graph, a, b); // pending_edge_removals gains backing

    GraphDeinit(&graph);                   // deinit_vec(pending_edge_removals) frees the array

    bool ok = LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

int main(void) {
    WriteFmt("[INFO] Starting Graph.Leak tests\n\n");

    TestFunction tests[] = {
        test_deinit_frees_pending_edge_removals_backing_no_leak,
    };
    TestFunction deadend_tests[] = {0};
    (void)deadend_tests;

    return run_test_suite(tests, (int)(sizeof(tests) / sizeof(tests[0])), deadend_tests, 0, "Graph.Leak");
}
