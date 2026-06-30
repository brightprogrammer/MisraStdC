#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Std/Container/Graph.h>
#include <Misra/Std/Container/Map.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Memory.h>
#include <Misra/Types.h>

#include "../Util/TestRunner.h"

static u64 node_id_hash(const void *data, u32 size) {
    u64 x = *(const GraphNodeId *)data;
    (void)size;

    x ^= x >> 33;
    x *= 0xff51afd7ed558ccdULL;
    x ^= x >> 33;
    x *= 0xc4ceb9fe1a85ec53ULL;
    x ^= x >> 33;
    return x;
}

static i32 node_id_compare(const void *lhs, const void *rhs) {
    GraphNodeId a = *(const GraphNodeId *)lhs;
    GraphNodeId b = *(const GraphNodeId *)rhs;
    return (a > b) - (a < b);
}

typedef Graph(Str) CityGraph;
typedef Map(Str, GraphNodeId) CityIndex;

static GraphNodeId city_add_intersection(CityGraph *graph, CityIndex *index, const Str *name, DefaultAllocator *alloc) {
    GraphNodeId id = GraphAddNodeR(graph, StrInitFromCstr(StrBegin(name), StrLen(name), alloc));

    Str key_copy = StrInitFromCstr(StrBegin(name), StrLen(name), alloc);
    MapInsertR(index, key_copy, id);
    return id;
}

static void city_reset_visits(CityGraph *graph) {
    GraphForeachNode(graph, node) {
        GraphNodeUnvisit(node);
    }
}

static bool city_reachable_from(GraphNode node, GraphNodeId goal_id) {
    if (GraphNodeVisitCount(node) > 0) {
        return false;
    }

    GraphNodeVisit(node);
    if (GraphNodeGetId(node) == goal_id) {
        return true;
    }

    GraphNodeForeachNeighbor(node, neighbor) {
        if (city_reachable_from(neighbor, goal_id)) {
            return true;
        }
    }

    return false;
}

static bool city_reachable(CityGraph *graph, CityIndex *index, const Str *from, const Str *to) {
    GraphNodeId *from_id = MapGetFirstPtr(index, *from);
    GraphNodeId *to_id   = MapGetFirstPtr(index, *to);

    if (!from_id || !to_id) {
        return false;
    }

    city_reset_visits(graph);
    return city_reachable_from(GraphGetNode(graph, *from_id), *to_id);
}

static bool test_graph_city_reachability(void) {
    WriteFmt("Testing GraphForeachNode and GraphNodeForeachNeighbor for reachability\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    CityGraph graph = GraphInitWithDeepCopy(NULL, str_deinit, &alloc);
    CityIndex index = MapInitWithDeepCopy(str_hash, str_compare, str_init_copy, str_deinit, NULL, NULL, &alloc);

    // The helpers take const Str *. Build one stack Str per literal and
    // hand its address through; the helpers + Map / Graph deep-copy
    // callbacks own the resulting clones.
    Str s_alpha   = StrInitFromZstr("Alpha", &alloc);
    Str s_beta    = StrInitFromZstr("Beta", &alloc);
    Str s_gamma   = StrInitFromZstr("Gamma", &alloc);
    Str s_delta   = StrInitFromZstr("Delta", &alloc);
    Str s_echo    = StrInitFromZstr("Echo", &alloc);
    Str s_unknown = StrInitFromZstr("Unknown", &alloc);

    GraphNodeId alpha = city_add_intersection(&graph, &index, &s_alpha, &alloc);
    GraphNodeId beta  = city_add_intersection(&graph, &index, &s_beta, &alloc);
    GraphNodeId gamma = city_add_intersection(&graph, &index, &s_gamma, &alloc);
    GraphNodeId delta = city_add_intersection(&graph, &index, &s_delta, &alloc);
    GraphNodeId echo  = city_add_intersection(&graph, &index, &s_echo, &alloc);

    GraphAddEdge(&graph, alpha, beta);
    GraphAddEdge(&graph, beta, gamma);
    GraphAddEdge(&graph, gamma, delta);
    GraphAddEdge(&graph, gamma, echo);
    GraphAddEdge(&graph, echo, beta);

    bool result = city_reachable(&graph, &index, &s_alpha, &s_delta);
    result      = result && city_reachable(&graph, &index, &s_echo, &s_gamma);
    result      = result && !city_reachable(&graph, &index, &s_delta, &s_alpha);
    result      = result && !city_reachable(&graph, &index, &s_unknown, &s_alpha);

    StrDeinit(&s_alpha);
    StrDeinit(&s_beta);
    StrDeinit(&s_gamma);
    StrDeinit(&s_delta);
    StrDeinit(&s_echo);
    StrDeinit(&s_unknown);

    city_reset_visits(&graph);
    GraphForeachNode(&graph, node) {
        result = result && (GraphNodeVisitCount(node) == 0);
    }

    MapDeinit(&index);
    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_graph_foreach_with_external_map_counts(void) {
    WriteFmt("Testing nested foreach with external count tracking map\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    typedef Map(GraphNodeId, u64) CountMap;

    IntGraph graph  = GraphInit(&alloc);
    CountMap counts = MapInit(node_id_hash, node_id_compare, &alloc);

    GraphNodeId a = GraphAddNodeR(&graph, 1);
    GraphNodeId b = GraphAddNodeR(&graph, 2);
    GraphNodeId c = GraphAddNodeR(&graph, 3);
    GraphNodeId d = GraphAddNodeR(&graph, 4);

    GraphAddEdge(&graph, a, b);
    GraphAddEdge(&graph, a, c);
    GraphAddEdge(&graph, b, d);
    GraphAddEdge(&graph, c, d);

    GraphForeachNode(&graph, node) {
        (void)MapEnsurePtr(&counts, GraphNodeGetId(node), 0);
        GraphNodeForeachNeighbor(node, neighbor) {
            u64 *count  = MapEnsurePtr(&counts, GraphNodeGetId(neighbor), 0);
            *count     += 1;
        }
    }

    bool result = *MapGetFirstPtr(&counts, a) == 0;
    result      = result && *MapGetFirstPtr(&counts, b) == 1;
    result      = result && *MapGetFirstPtr(&counts, c) == 1;
    result      = result && *MapGetFirstPtr(&counts, d) == 2;

    MapDeinit(&counts);
    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_graph_foreach_predecessors(void) {
    WriteFmt("Testing GraphNodeForeachPredecessor\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    GraphNodeId a = GraphAddNodeR(&graph, 1);
    GraphNodeId b = GraphAddNodeR(&graph, 2);
    GraphNodeId c = GraphAddNodeR(&graph, 3);
    GraphNodeId d = GraphAddNodeR(&graph, 4);

    GraphAddEdge(&graph, a, d);
    GraphAddEdge(&graph, b, d);
    GraphAddEdge(&graph, c, d);
    GraphAddEdge(&graph, a, b);

    u64 predecessor_sum   = 0;
    u64 predecessor_count = 0;

    GraphNodeForeachPredecessor(GraphGetNode(&graph, d), predecessor) {
        predecessor_sum   += GraphNodeData(&graph, predecessor);
        predecessor_count += 1;
    }

    bool result = (predecessor_count == 3);
    result      = result && (predecessor_sum == 6);
    result      = result && (GraphInDegree(&graph, d) == 3);
    result      = result && (GraphInDegree(&graph, a) == 0);

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_graph_node_iteration_rejects_structural_mutation_deadend(void) {
    WriteFmt("Testing GraphForeachNode rejects structural mutation (should abort)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    GraphAddNodeR(&graph, 1);
    GraphAddNodeR(&graph, 2);

    GraphForeachNode(&graph, node) {
        (void)node;
        (void)GraphAddNodeR(&graph, 3);
    }

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

static bool test_graph_neighbor_iteration_rejects_structural_mutation_deadend(void) {
    WriteFmt("Testing GraphNodeForeachNeighbor rejects structural mutation (should abort)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    GraphNodeId a = GraphAddNodeR(&graph, 1);
    GraphNodeId b = GraphAddNodeR(&graph, 2);
    GraphNodeId c = GraphAddNodeR(&graph, 3);

    GraphAddEdge(&graph, a, b);

    GraphNodeForeachNeighbor(GraphGetNode(&graph, a), neighbor) {
        (void)neighbor;
        (void)GraphAddEdge(&graph, a, c);
    }

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

static bool test_graph_predecessor_iteration_rejects_structural_mutation_deadend(void) {
    WriteFmt("Testing GraphNodeForeachPredecessor rejects structural mutation (should abort)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    GraphNodeId a = GraphAddNodeR(&graph, 1);
    GraphNodeId b = GraphAddNodeR(&graph, 2);
    GraphNodeId c = GraphAddNodeR(&graph, 3);
    GraphNodeId d = GraphAddNodeR(&graph, 4);

    GraphAddEdge(&graph, a, c);
    GraphAddEdge(&graph, b, c);

    GraphNodeForeachPredecessor(GraphGetNode(&graph, c), predecessor) {
        (void)predecessor;
        (void)GraphAddEdge(&graph, d, c);
    }

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// GraphReserve must not falsely invalidate an in-progress traversal when it
// does not change capacity. Mutant: reserve_graph snapshots a constant (42)
// instead of the real old capacity, so a no-grow reserve still bumps the
// mutation epoch and aborts the active iterator. Real code: epoch unchanged,
// iteration completes over both nodes.
static bool test_graph_reserve_no_grow_keeps_iterator_valid(void) {
    WriteFmt("Testing in-capacity GraphReserve does not invalidate traversal\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    // Pre-grow so the live capacity is deterministic and not 42.
    GraphReserve(&graph, 4);

    (void)GraphAddNodeR(&graph, 10);
    (void)GraphAddNodeR(&graph, 20);

    u64 visited = 0;
    GraphForeachNode(&graph, node) {
        if (visited == 0) {
            // Reserve well within existing capacity: no realloc, so real code
            // leaves the epoch untouched and the iterator stays valid.
            (void)GraphReserve(&graph, 2);
        }
        visited += 1;
        (void)node;
    }

    bool result = (visited == 2);

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Kills 983:13 cxx_replace_scalar_call in graph_node_iter_next: the
// `graph_slot_is_occupied(slot)` guard is replaced with a forced-true value,
// so the forward node iterator would yield a node for every slot -- including
// the freed hole left behind by a committed deletion. The caller-observable
// outcome is the live-node count produced by GraphForeachNode: real code
// visits exactly the surviving nodes; the mutant visits the free slot too.
static bool test_graph_foreach_skips_freed_slot(void) {
    WriteFmt("Testing GraphForeachNode visits only occupied slots after a commit\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    GraphNodeId a = GraphAddNodeR(&graph, 10);
    GraphNodeId b = GraphAddNodeR(&graph, 20);
    GraphNodeId c = GraphAddNodeR(&graph, 30);
    (void)a;
    (void)c;

    // Delete the middle node so its slot becomes a free hole between two
    // live slots, then commit so the deletion materializes.
    (void)GraphMarkNodeForDeletion(GraphGetNode(&graph, b));
    (void)GraphCommitChanges(&graph);

    u64 visited = 0;
    GraphForeachNode(&graph, node) {
        (void)node;
        visited += 1;
    }

    bool result = (visited == 2);
    result      = result && (GraphNodeCount(&graph) == 2);

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Mutant 1030 (graph_neighbor_iter_next): the per-neighbor
// `graph_validate_node_id(iter->graph, neighbor_id)` is the only guard that
// rejects a corrupted out-neighbor id once the validator's deep body has been
// memoized away (validated bit cleared) and the mutation epoch is unchanged.
// Corrupt the single out-neighbor id to generation 0 *after* the iterator has
// begun (which runs the deep validation and clears the validated bit) but
// without bumping the epoch, so the only surviving defense is line 1030.
static bool test_neighbor_iter_validates_neighbor_id_deadend(void) {
    WriteFmt("Testing neighbor iteration rejects a corrupted neighbor id (should abort)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    GraphNodeId a = GraphAddNodeR(&graph, 10);
    GraphNodeId b = GraphAddNodeR(&graph, 20);

    GraphAddEdge(&graph, a, b);

    // Begin runs the deep validator (clears the validated bit) and snapshots
    // the current epoch.
    GenericGraphNeighborIter iter = graph_neighbor_iter_begin(GraphGetNode(&graph, a));

    // Intentional bypass: there is no public mutator for an individual
    // adjacency entry. Reach into `a`'s out_neighbors and rewrite the stored
    // neighbor id so its generation is 0 (an always-invalid generation),
    // without bumping the mutation epoch or re-flagging the validated bit.
    GenericGraphSlot *slot_a = (GenericGraphSlot *)VecPtrAt(&GENERIC_GRAPH(&graph)->slots, GraphNodeIdIndex(a));
    GraphNodeId      *entry  = VecPtrAt(&slot_a->out_neighbors, 0);
    *entry                   = (GraphNodeId)GraphNodeIdIndex(b);

    GraphNode out = {0};
    (void)graph_neighbor_iter_next(&iter, &out);

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// Mutant 1077 (graph_predecessor_iter_next): same shape on the predecessor
// path -- `graph_validate_node_id(iter->graph, predecessor_id)` is the only
// guard left once the deep validator is memoized away. Corrupt the in-neighbor
// id to generation 0 after begin.
static bool test_predecessor_iter_validates_predecessor_id_deadend(void) {
    WriteFmt("Testing predecessor iteration rejects a corrupted predecessor id (should abort)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    GraphNodeId a = GraphAddNodeR(&graph, 10);
    GraphNodeId b = GraphAddNodeR(&graph, 20);

    GraphAddEdge(&graph, a, b);

    GenericGraphPredecessorIter iter = graph_predecessor_iter_begin(GraphGetNode(&graph, b));

    // Intentional bypass: rewrite `b`'s sole in-neighbor (a) so its generation
    // is 0, leaving the epoch and validated bit untouched.
    GenericGraphSlot *slot_b = (GenericGraphSlot *)VecPtrAt(&GENERIC_GRAPH(&graph)->slots, GraphNodeIdIndex(b));
    GraphNodeId      *entry  = VecPtrAt(&slot_b->in_neighbors, 0);
    *entry                   = (GraphNodeId)GraphNodeIdIndex(a);

    GraphNode out = {0};
    (void)graph_predecessor_iter_next(&iter, &out);

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// Mutant 107:5:cxx_remove_void_call (graph_validate_node_handle): removes the
// `graph_validate_node_id(graph, node._id_)` that rejects a stale/invalid node
// handle. graph_neighbor_iter_begin is the load-bearing caller: it validates
// the handle, then captures node._id_ as the iterator's source_id WITHOUT a
// graph_require_live_slot re-validation. If the begin-time check is removed, a
// stale handle produces a live-looking iterator with no abort. We deliberately
// do NOT step the iterator (graph_neighbor_iter_next would re-validate
// source_id and mask the removal), so on real code begin aborts and on the
// mutant begin returns cleanly. DEADEND.
static bool test_neighbor_iter_begin_rejects_stale_handle_deadend(void) {
    WriteFmt("Testing graph_neighbor_iter_begin rejects a stale node handle (should abort)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    GraphNodeId a = GraphAddNodeR(&graph, 10);

    // Capture a handle, then delete + commit + reuse a's slot so the captured
    // id is stale (slot occupied, generation superseded).
    GraphNode stale = GraphGetNode(&graph, a);
    (void)GraphMarkNodeForDeletion(GraphGetNode(&graph, a));
    (void)GraphCommitChanges(&graph);
    (void)GraphAddNodeR(&graph, 99); // reuse a's slot at a higher generation

    // Real code: graph_validate_node_handle aborts on the stale id here.
    // Mutant: begin captures the stale source_id without aborting; we never
    // step the iterator, so no later guard fires.
    (void)graph_neighbor_iter_begin(stale);

    return false;
}

// graph_push_node line 604: bump of the mutation epoch on the slot-grow success
// path. The epoch is what node iteration captures and re-checks to abort on a
// structural mutation mid-traversal.
//
// IMPORTANT: a grow add that also changes the slot-vector CAPACITY routes
// through reserve_graph, which independently bumps the epoch (line 516) and so
// masks the removal of line 604. To isolate line 604 we pre-reserve spare
// capacity so the mid-iteration add appends a slot WITHOUT a capacity change:
// reserve_graph is then a no-op and line 604 is the only epoch bump on the path.
// Real code: epoch bumped, the next iterator step detects the mismatch and
// aborts. Mutant: no bump, no abort.
static bool test_node_iteration_rejects_grow_mutation_deadend(void) {
    WriteFmt("Testing GraphForeachNode rejects a no-realloc slot-growing mutation (should abort)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    // Pre-reserve enough capacity that appending a third slot does NOT realloc,
    // so reserve_graph leaves the epoch untouched and line 604 stands alone.
    (void)GraphReserve(&graph, 8);

    GraphAddNodeR(&graph, 1);
    GraphAddNodeR(&graph, 2);

    bool added = false;
    GraphForeachNode(&graph, node) {
        (void)node;
        if (!added) {
            added = true;
            // No free indices and spare capacity => grow path with no realloc.
            (void)GraphAddNodeR(&graph, 3);
        }
    }

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// DEADEND: clearing a graph mid-traversal must invalidate the active iterator.
// Mutant: clear_graph drops the mutation-epoch bump, so the iterator's snapshot
// still matches and the next step is not detected as a structural change. Real
// code aborts on the next iteration step.
static bool test_graph_clear_during_traversal_aborts_deadend(void) {
    WriteFmt("Testing GraphClear during traversal invalidates iterator (should abort)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    (void)GraphAddNodeR(&graph, 10);
    (void)GraphAddNodeR(&graph, 20);

    u64 visited = 0;
    GraphForeachNode(&graph, node) {
        (void)node;
        if (visited == 0) {
            // Structural mutation mid-iteration: the next iterator step must
            // detect the epoch change and abort.
            GraphClear(&graph);
        }
        visited += 1;
    }

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// Kills 949:5 cxx_remove_void_call in graph_commit_changes: the trailing
// graph_bump_mutation_epoch(graph) is deleted, so a commit that removes a
// node no longer advances the mutation epoch. A node iterator that is live
// across that commit must observe the structural change and abort; without
// the epoch bump it would silently keep iterating a mutated graph. Real code
// aborts on the next iteration step -> DEADEND.
static bool test_graph_commit_invalidates_live_iterator_deadend(void) {
    WriteFmt("Testing GraphCommitChanges invalidates a live node iterator (should abort)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    GraphNodeId a = GraphAddNodeR(&graph, 10);
    GraphNodeId b = GraphAddNodeR(&graph, 20);
    (void)a;

    GraphForeachNode(&graph, node) {
        (void)node;
        // Mark + commit a deletion mid-iteration. The commit bumps the
        // mutation epoch, so the iterator's next step must abort.
        (void)GraphMarkNodeForDeletion(GraphGetNode(&graph, b));
        (void)GraphCommitChanges(&graph);
    }

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return false;
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

int main(void) {
    TestFunction tests[] = {
        test_graph_city_reachability,
        test_graph_foreach_with_external_map_counts,
        test_graph_foreach_predecessors,
        test_graph_reserve_no_grow_keeps_iterator_valid,
        test_graph_foreach_skips_freed_slot,
        test_node_iter_no_overscan_extra_slot,
    };
    TestFunction deadend_tests[] = {
        test_graph_node_iteration_rejects_structural_mutation_deadend,
        test_graph_neighbor_iteration_rejects_structural_mutation_deadend,
        test_graph_predecessor_iteration_rejects_structural_mutation_deadend,
        test_neighbor_iter_validates_neighbor_id_deadend,
        test_predecessor_iter_validates_predecessor_id_deadend,
        test_neighbor_iter_begin_rejects_stale_handle_deadend,
        test_node_iteration_rejects_grow_mutation_deadend,
        test_graph_clear_during_traversal_aborts_deadend,
        test_graph_commit_invalidates_live_iterator_deadend,
    };

    WriteFmt("[INFO] Starting Graph.Foreach tests\n\n");
    return run_test_suite(
        tests,
        (int)(sizeof(tests) / sizeof(tests[0])),
        deadend_tests,
        (int)(sizeof(deadend_tests) / sizeof(deadend_tests[0])),
        "Graph.Foreach"
    );
}
