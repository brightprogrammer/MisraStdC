#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Container/Graph.h>
#include <Misra/Std/Log.h>

#include "../../Util/TestRunner.h"

static bool test_graph_node_visit_scratch_state(void) {
    WriteFmt("Testing Graph node scratch visit state\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    GraphNodeId a    = GraphAddNodeR(&graph, 10);
    GraphNode   node = GraphGetNode(&graph, a);

    bool result = !GraphNodeVisited(node) && (GraphNodeVisitCount(node) == 0);

    GraphNodeVisit(node);
    GraphNodeVisit(node);

    result = result && GraphNodeVisited(node) && (GraphNodeVisitCount(node) == 2);

    GraphNodeUnvisit(node);
    result = result && !GraphNodeVisited(node) && (GraphNodeVisitCount(node) == 0);

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_graph_mark_delete_commit_and_reuse(void) {
    WriteFmt("Testing GraphMarkNodeForDeletion and GraphCommitChanges\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    GraphNodeId a = GraphAddNodeR(&graph, 10);
    GraphNodeId b = GraphAddNodeR(&graph, 20);
    GraphNodeId c = GraphAddNodeR(&graph, 30);

    GraphAddEdge(&graph, a, b);
    GraphAddEdge(&graph, b, c);
    GraphAddEdge(&graph, c, a);

    GraphForeachNode(&graph, node) {
        if (GraphNodeData(&graph, node) == 20) {
            GraphMarkNodeForDeletion(node);
        }
    }

    bool result  = GraphContainsNode(&graph, b) && (graph.pending_delete_count == 1);
    u64  removed = GraphCommitChanges(&graph);

    result = result && (removed == 1);
    result = result && (GraphNodeCount(&graph) == 2);
    result = result && (GraphEdgeCount(&graph) == 1);
    result = result && !GraphContainsNode(&graph, b);
    result = result && (GraphOutDegree(&graph, a) == 0);
    result = result && (GraphInDegree(&graph, a) == 1);
    result = result && (GraphInDegree(&graph, c) == 0);
    result = result && GraphHasEdge(&graph, c, a);
    result = result && (GraphPredecessorAt(&graph, a, 0) == c);

    GraphNodeId d = GraphAddNodeR(&graph, 40);

    result = result && (GraphNodeIdIndex(d) == GraphNodeIdIndex(b));
    result = result && (GraphNodeIdGeneration(d) == (GraphNodeIdGeneration(b) + 1));
    result = result && (GraphNodeData(&graph, GraphGetNode(&graph, d)) == 40);
    result = result && !GraphNodeVisited(GraphGetNode(&graph, d));

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_graph_query_and_unmark_node_deletion(void) {
    WriteFmt("Testing Graph node mark query and unmark\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    GraphNodeId a    = GraphAddNodeR(&graph, 10);
    GraphNode   node = GraphGetNode(&graph, a);

    bool result = !GraphNodeMarkedForDeletion(node);
    result      = result && GraphMarkNodeForDeletion(node);
    result      = result && GraphNodeMarkedForDeletion(node);
    result      = result && !GraphMarkNodeForDeletion(node);
    result      = result && GraphUnmarkNodeForDeletion(node);
    result      = result && !GraphNodeMarkedForDeletion(node);
    result      = result && !GraphUnmarkNodeForDeletion(node);
    result      = result && (GraphCommitChanges(&graph) == 0);
    result      = result && GraphContainsNode(&graph, a);
    result      = result && (GraphNodeCount(&graph) == 1);

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_graph_mark_edge_for_removal(void) {
    WriteFmt("Testing GraphMarkEdgeForRemoval and deferred edge commit\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    GraphNodeId a = GraphAddNodeR(&graph, 10);
    GraphNodeId b = GraphAddNodeR(&graph, 20);
    GraphNodeId c = GraphAddNodeR(&graph, 30);

    GraphAddEdge(&graph, a, b);
    GraphAddEdge(&graph, a, c);
    GraphAddEdge(&graph, b, c);

    GraphForeachNode(&graph, node) {
        if (GraphNodeGetId(node) == a) {
            GraphNodeForeachNeighbor(node, neighbor) {
                if (GraphNodeGetId(neighbor) == b) {
                    (void)GraphMarkEdgeForRemoval(&graph, GraphNodeGetId(node), GraphNodeGetId(neighbor));
                }
            }
        }
    }

    bool result = GraphHasEdge(&graph, a, b);
    result      = result && GraphMarkEdgeForRemoval(&graph, b, c);
    result      = result && !GraphMarkEdgeForRemoval(&graph, b, c);
    result      = result && !GraphMarkEdgeForRemoval(&graph, c, b);

    u64 committed = GraphCommitChanges(&graph);

    result = result && (committed == 2);
    result = result && !GraphHasEdge(&graph, a, b);
    result = result && GraphHasEdge(&graph, a, c);
    result = result && !GraphHasEdge(&graph, b, c);
    result = result && (GraphEdgeCount(&graph) == 1);
    result = result && (GraphOutDegree(&graph, a) == 1);
    result = result && (GraphInDegree(&graph, b) == 0);
    result = result && (GraphInDegree(&graph, c) == 1);
    result = result && (GraphNeighborAt(&graph, a, 0) == c);
    result = result && (GraphPredecessorAt(&graph, c, 0) == a);

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_graph_query_and_unmark_edge_removal(void) {
    WriteFmt("Testing Graph edge mark query and unmark\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    GraphNodeId a = GraphAddNodeR(&graph, 10);
    GraphNodeId b = GraphAddNodeR(&graph, 20);

    GraphAddEdge(&graph, a, b);

    bool result = !GraphEdgeMarkedForRemoval(&graph, a, b);
    result      = result && GraphMarkEdgeForRemoval(&graph, a, b);
    result      = result && GraphEdgeMarkedForRemoval(&graph, a, b);
    result      = result && !GraphMarkEdgeForRemoval(&graph, a, b);
    result      = result && GraphUnmarkEdgeForRemoval(&graph, a, b);
    result      = result && !GraphEdgeMarkedForRemoval(&graph, a, b);
    result      = result && !GraphUnmarkEdgeForRemoval(&graph, a, b);
    result      = result && (GraphCommitChanges(&graph) == 0);
    result      = result && GraphHasEdge(&graph, a, b);
    result      = result && (GraphEdgeCount(&graph) == 1);

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_graph_partial_unmark_of_multiple_edge_removals(void) {
    WriteFmt("Testing partial unmark of multiple pending edge removals\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    GraphNodeId a = GraphAddNodeR(&graph, 10);
    GraphNodeId b = GraphAddNodeR(&graph, 20);
    GraphNodeId c = GraphAddNodeR(&graph, 30);

    GraphAddEdge(&graph, a, b);
    GraphAddEdge(&graph, a, c);

    bool result = GraphMarkEdgeForRemoval(&graph, a, b);
    result      = result && GraphMarkEdgeForRemoval(&graph, a, c);
    result      = result && GraphEdgeMarkedForRemoval(&graph, a, b);
    result      = result && GraphEdgeMarkedForRemoval(&graph, a, c);
    result      = result && GraphUnmarkEdgeForRemoval(&graph, a, b);
    result      = result && !GraphEdgeMarkedForRemoval(&graph, a, b);
    result      = result && GraphEdgeMarkedForRemoval(&graph, a, c);
    result      = result && (GraphCommitChanges(&graph) == 1);
    result      = result && GraphHasEdge(&graph, a, b);
    result      = result && !GraphHasEdge(&graph, a, c);
    result      = result && (GraphOutDegree(&graph, a) == 1);
    result      = result && (GraphInDegree(&graph, b) == 1);
    result      = result && (GraphInDegree(&graph, c) == 0);

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_graph_self_loop_edge_removal(void) {
    WriteFmt("Testing deferred removal of self-loop edge\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    GraphNodeId a = GraphAddNodeR(&graph, 10);

    bool result = GraphAddEdge(&graph, a, a);
    result      = result && (GraphOutDegree(&graph, a) == 1);
    result      = result && (GraphInDegree(&graph, a) == 1);
    result      = result && (GraphNeighborAt(&graph, a, 0) == a);
    result      = result && (GraphPredecessorAt(&graph, a, 0) == a);
    result      = result && GraphMarkEdgeForRemoval(&graph, a, a);
    result      = result && (GraphCommitChanges(&graph) == 1);
    result      = result && (GraphEdgeCount(&graph) == 0);
    result      = result && (GraphOutDegree(&graph, a) == 0);
    result      = result && (GraphInDegree(&graph, a) == 0);

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_graph_edge_removal_and_node_deletion_overlap(void) {
    WriteFmt("Testing overlap between pending edge removal and node deletion\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    GraphNodeId a = GraphAddNodeR(&graph, 10);
    GraphNodeId b = GraphAddNodeR(&graph, 20);
    GraphNodeId c = GraphAddNodeR(&graph, 30);
    GraphNodeId d = GraphAddNodeR(&graph, 40);

    GraphAddEdge(&graph, a, b);
    GraphAddEdge(&graph, b, c);
    GraphAddEdge(&graph, d, b);

    bool result = GraphMarkEdgeForRemoval(&graph, a, b);
    result      = result && GraphMarkNodeForDeletion(GraphGetNode(&graph, b));
    result      = result && GraphEdgeMarkedForRemoval(&graph, a, b);
    result      = result && GraphNodeMarkedForDeletion(GraphGetNode(&graph, b));
    result      = result && (GraphCommitChanges(&graph) == 2);
    result      = result && !GraphContainsNode(&graph, b);
    result      = result && (GraphNodeCount(&graph) == 3);
    result      = result && (GraphEdgeCount(&graph) == 0);
    result      = result && (GraphOutDegree(&graph, a) == 0);
    result      = result && (GraphOutDegree(&graph, d) == 0);
    result      = result && (GraphInDegree(&graph, c) == 0);
    result      = result && (GraphInDegree(&graph, a) == 0);
    result      = result && (GraphInDegree(&graph, d) == 0);

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_graph_external_indexed_state_requires_reset_on_reuse(void) {
    WriteFmt("Testing external slot-indexed state across delete and reuse\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    GraphNodeId a         = GraphAddNodeR(&graph, 10);
    GraphNodeId b         = GraphAddNodeR(&graph, 20);
    u64         counts[2] = {0};

    counts[GraphNodeIdIndex(a)] = 11;
    counts[GraphNodeIdIndex(b)] = 29;

    bool result = GraphMarkNodeForDeletion(GraphGetNode(&graph, b));
    result      = result && (GraphCommitChanges(&graph) == 1);
    result      = result && !GraphContainsNode(&graph, b);

    GraphNodeId reused = GraphAddNodeR(&graph, 99);

    result = result && (GraphNodeIdIndex(reused) == GraphNodeIdIndex(b));
    result = result && (GraphNodeIdGeneration(reused) == (GraphNodeIdGeneration(b) + 1));
    result = result && (counts[GraphNodeIdIndex(reused)] == 29);

    counts[GraphNodeIdIndex(reused)] = 0;
    result                           = result && (counts[GraphNodeIdIndex(reused)] == 0);
    result                           = result && (counts[GraphNodeIdIndex(a)] == 11);
    result                           = result && (GraphNodeAt(&graph, reused) == 99);

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_graph_stale_node_handle_after_commit_deadend(void) {
    WriteFmt("Testing stale GraphNode handle after commit (should abort)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    GraphNodeId a    = GraphAddNodeR(&graph, 10);
    GraphNode   node = GraphGetNode(&graph, a);

    (void)GraphMarkNodeForDeletion(node);
    (void)GraphCommitChanges(&graph);
    (void)GraphNodeVisit(node);

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// Mutant 258:272:cxx_add_assign_to_sub_assign -- the `idx += 1` advance in
// graph_remove_marked_outgoing_edges. With `idx -= 1` the unsigned index
// underflows after the first kept edge, so the loop exits before reaching a
// later edge that points at a marked node. That marked node then still has an
// incoming edge when the commit's release loop runs, tripping the
// "marked node retained incident edges" fatal. On real code commit succeeds
// and leaves the live edge intact, so this is a NORMAL test asserting the
// caller-observable post-commit graph shape.
static bool test_graph_commit_keeps_live_edge_before_removing_marked_edge(void) {
    WriteFmt("Testing commit keeps a live out-edge ordered before a marked-target edge\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    GraphNodeId a = GraphAddNodeR(&graph, 10);
    GraphNodeId b = GraphAddNodeR(&graph, 20);
    GraphNodeId c = GraphAddNodeR(&graph, 30);

    // a.out = [c, b] -- the kept edge (a->c) comes first, the edge to the
    // marked node (a->b) comes second, so the loop must advance past the kept
    // edge to reach and drop a->b.
    GraphAddEdge(&graph, a, c);
    GraphAddEdge(&graph, a, b);

    GraphMarkNodeForDeletion(GraphGetNode(&graph, b));

    u64 committed = GraphCommitChanges(&graph);

    bool result = (committed == 1);
    result      = result && !GraphContainsNode(&graph, b);
    result      = result && GraphContainsNode(&graph, a);
    result      = result && GraphContainsNode(&graph, c);
    result      = result && GraphHasEdge(&graph, a, c);
    result      = result && (GraphOutDegree(&graph, a) == 1);
    result      = result && (GraphEdgeCount(&graph) == 1);
    result      = result && (GraphNeighborAt(&graph, a, 0) == c);
    result      = result && (GraphNodeCount(&graph) == 2);

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Clear with a pre-existing free slot must reset that slot's visit_count to 0;
// a corrupted (non-zero) free-slot visit_count is rejected by the next
// validation. Mutant: clear_graph sets the free-slot visit_count to a constant
// (42), so the next valid op aborts. Real code keeps it at 0 and the op
// succeeds.
static bool test_graph_clear_resets_free_slot_visit_count(void) {
    WriteFmt("Testing GraphClear resets free-slot visit_count\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    GraphNodeId a = GraphAddNodeR(&graph, 10);
    GraphNodeId b = GraphAddNodeR(&graph, 20);

    // Delete b so its slot is free at the time GraphClear walks it.
    bool result = GraphMarkNodeForDeletion(GraphGetNode(&graph, b));
    result      = result && (GraphCommitChanges(&graph) == 1);
    result      = result && !GraphContainsNode(&graph, b);
    (void)a;

    GraphClear(&graph);

    // A valid op after clear runs ValidateGraph; on the mutant the pre-free
    // slot carries a bogus visit_count and validation aborts here.
    GraphNodeId c = GraphAddNodeR(&graph, 30);
    result        = result && GraphContainsNode(&graph, c);
    result        = result && (GraphNodeCount(&graph) == 1);

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Clear must empty free_indices before repopulating it for every slot; if the
// old entries are retained, the (live + free) == slot accounting breaks and the
// next validation aborts. Mutant: clear_graph drops the clear_vec on
// free_indices. Setup needs a non-empty free_indices at clear time (one prior
// deletion).
static bool test_graph_clear_empties_free_indices(void) {
    WriteFmt("Testing GraphClear empties free_indices before repopulating\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    GraphNodeId a = GraphAddNodeR(&graph, 10);
    GraphNodeId b = GraphAddNodeR(&graph, 20);
    (void)a;

    // One deletion leaves a single entry in free_indices going into clear.
    bool result = GraphMarkNodeForDeletion(GraphGetNode(&graph, b));
    result      = result && (GraphCommitChanges(&graph) == 1);

    GraphClear(&graph);

    // Valid op runs ValidateGraph; on the mutant free_indices is over-long and
    // slot accounting is inconsistent, so validation aborts before this runs.
    GraphNodeId c = GraphAddNodeR(&graph, 30);
    result        = result && GraphContainsNode(&graph, c);
    result        = result && (GraphNodeCount(&graph) == 1);

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// NORMAL: a commit that runs while a previously-freed (free-list) slot still
// exists must succeed and return the right count. The pass-2 loop in
// graph_commit_changes guards each slot with `occupied && !marked`; if the
// occupied half is forced true (mutant at 929), the loop processes the free
// slot and graph_validate_node_id aborts on "free slot". Real code skips it.
static bool test_commit_with_free_slot_present_succeeds(void) {
    WriteFmt("Testing commit succeeds while a free-list slot is present\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    GraphNodeId a = GraphAddNodeR(&graph, 10);
    GraphNodeId b = GraphAddNodeR(&graph, 20);
    GraphNodeId c = GraphAddNodeR(&graph, 30);

    GraphAddEdge(&graph, a, c);

    // First commit frees b's slot (free-list entry, generation bumped).
    bool result  = GraphMarkNodeForDeletion(GraphGetNode(&graph, b));
    u64  removed = GraphCommitChanges(&graph);
    result       = result && (removed == 1);
    result       = result && !GraphContainsNode(&graph, b);

    // Second commit: a free slot (b's) is present. The marked/unmarked passes
    // iterate every slot; the free slot must be skipped, not validated.
    result  = result && GraphMarkNodeForDeletion(GraphGetNode(&graph, a));
    removed = GraphCommitChanges(&graph);

    result = result && (removed == 1);
    result = result && !GraphContainsNode(&graph, a);
    result = result && GraphContainsNode(&graph, c);
    result = result && (GraphNodeCount(&graph) == 1);
    result = result && (GraphEdgeCount(&graph) == 0);
    result = result && (GraphInDegree(&graph, c) == 0);

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// DEADEND: graph_edge_marked_for_removal validates `from` (mutant at 878
// removes it). A stale `from` with a valid `to` must abort. Without the
// validation graph_find_pending_edge_removal_index just returns "not found".
static bool test_edge_marked_stale_from_deadend(void) {
    WriteFmt("Testing GraphEdgeMarkedForRemoval rejects a stale from id (should abort)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    GraphNodeId a = GraphAddNodeR(&graph, 10);
    GraphNodeId b = GraphAddNodeR(&graph, 20);

    (void)GraphMarkNodeForDeletion(GraphGetNode(&graph, a));
    (void)GraphCommitChanges(&graph);
    (void)GraphAddNodeR(&graph, 99); // reuse a's slot, a is now stale

    (void)GraphEdgeMarkedForRemoval(&graph, a, b);

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// DEADEND: graph_edge_marked_for_removal validates `to` (mutant at 879). A
// valid `from` with a stale `to` must abort.
static bool test_edge_marked_stale_to_deadend(void) {
    WriteFmt("Testing GraphEdgeMarkedForRemoval rejects a stale to id (should abort)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    GraphNodeId a = GraphAddNodeR(&graph, 10);
    GraphNodeId b = GraphAddNodeR(&graph, 20);

    (void)GraphMarkNodeForDeletion(GraphGetNode(&graph, b));
    (void)GraphCommitChanges(&graph);
    (void)GraphAddNodeR(&graph, 99); // reuse b's slot, b is now stale

    (void)GraphEdgeMarkedForRemoval(&graph, a, b);

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// DEADEND: graph_unmark_edge_for_removal validates `from` (mutant at 888). A
// stale `from` with a valid `to` must abort. Without it
// graph_find_pending_edge_removal_index returns SIZE_MAX and the call returns
// false rather than aborting.
static bool test_unmark_edge_stale_from_deadend(void) {
    WriteFmt("Testing GraphUnmarkEdgeForRemoval rejects a stale from id (should abort)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    GraphNodeId a = GraphAddNodeR(&graph, 10);
    GraphNodeId b = GraphAddNodeR(&graph, 20);

    (void)GraphMarkNodeForDeletion(GraphGetNode(&graph, a));
    (void)GraphCommitChanges(&graph);
    (void)GraphAddNodeR(&graph, 99); // reuse a's slot, a is now stale

    (void)GraphUnmarkEdgeForRemoval(&graph, a, b);

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// DEADEND: graph_unmark_edge_for_removal validates `to` (mutant at 889). A
// valid `from` with a stale `to` must abort.
static bool test_unmark_edge_stale_to_deadend(void) {
    WriteFmt("Testing GraphUnmarkEdgeForRemoval rejects a stale to id (should abort)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    GraphNodeId a = GraphAddNodeR(&graph, 10);
    GraphNodeId b = GraphAddNodeR(&graph, 20);

    (void)GraphMarkNodeForDeletion(GraphGetNode(&graph, b));
    (void)GraphCommitChanges(&graph);
    (void)GraphAddNodeR(&graph, 99); // reuse b's slot, b is now stale

    (void)GraphUnmarkEdgeForRemoval(&graph, a, b);

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return false;
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
        test_graph_node_visit_scratch_state,
        test_graph_mark_delete_commit_and_reuse,
        test_graph_query_and_unmark_node_deletion,
        test_graph_mark_edge_for_removal,
        test_graph_query_and_unmark_edge_removal,
        test_graph_partial_unmark_of_multiple_edge_removals,
        test_graph_self_loop_edge_removal,
        test_graph_edge_removal_and_node_deletion_overlap,
        test_graph_external_indexed_state_requires_reset_on_reuse,
        test_graph_commit_keeps_live_edge_before_removing_marked_edge,
        test_graph_clear_resets_free_slot_visit_count,
        test_graph_clear_empties_free_indices,
        test_commit_with_free_slot_present_succeeds,
        test_find_neighbor_index_no_overscan,
        test_commit_pass2_no_overscan,
        test_commit_marked_passes_no_overscan,
    };
    TestFunction deadend_tests[] = {
        test_graph_stale_node_handle_after_commit_deadend,
        test_edge_marked_stale_from_deadend,
        test_edge_marked_stale_to_deadend,
        test_unmark_edge_stale_from_deadend,
        test_unmark_edge_stale_to_deadend,
    };

    WriteFmt("[INFO] Starting Graph.Ops tests\n\n");
    return run_test_suite(
        tests,
        (int)(sizeof(tests) / sizeof(tests[0])),
        deadend_tests,
        (int)(sizeof(deadend_tests) / sizeof(deadend_tests[0])),
        "Graph.Ops"
    );
}
