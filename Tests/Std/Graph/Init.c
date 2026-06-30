#include <Misra/Std/Allocator/Debug.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Std/Allocator/Heap.h>
#include <Misra/Std/Container/Graph.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Log.h>

#include "../../Util/TestRunner.h"

// A test passes iff `ok` held AND the DebugAllocator has no outstanding
// allocations after the test released everything it owns.
#define LEAK_CLEAN(dbg) (DebugAllocatorLiveCount(&(dbg)) == 0 && DebugAllocatorLiveBytes(&(dbg)) == 0)

static bool test_graph_reserve_clear(void) {
    WriteFmt("Testing GraphReserve and GraphClear\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    GraphReserve(&graph, 8);
    ValidateGraph(&graph);

    bool        result            = VecCapacity(&graph.slots) >= 8;
    GraphNodeId first_id          = GraphAddNodeR(&graph, 10);
    GraphNodeId second_id         = GraphAddNodeR(&graph, 20);
    GraphNodeId third_id          = GraphAddNodeR(&graph, 30);
    u64         slot_count        = VecLen(&graph.slots);
    size        slot_capacity     = VecCapacity(&graph.slots);
    u32         first_generation  = GraphNodeIdGeneration(first_id);
    u32         second_generation = GraphNodeIdGeneration(second_id);
    u32         third_generation  = GraphNodeIdGeneration(third_id);
    u64         slot_index;

    result = result && GraphAddEdge(&graph, first_id, second_id);
    result = result && GraphAddEdge(&graph, second_id, third_id);
    result = result && GraphAddEdge(&graph, third_id, first_id);
    result = result && GraphAddEdge(&graph, third_id, third_id);
    result = result && (GraphNodeVisit(GraphGetNode(&graph, first_id)) == 1);
    result = result && GraphMarkNodeForDeletion(GraphGetNode(&graph, second_id));
    result = result && GraphMarkEdgeForRemoval(&graph, third_id, third_id);

    GraphClear(&graph);

    result = result && GraphNodeCount(&graph) == 0 && GraphEdgeCount(&graph) == 0 && GraphEmpty(&graph);
    result = result && !GraphContainsNode(&graph, first_id) && !GraphContainsNode(&graph, second_id);
    result = result && !GraphContainsNode(&graph, third_id);
    // intentional bypass: no public accessors for `slots`, `free_indices`,
    // `pending_delete_count`, or `pending_edge_removals`; this test asserts
    // on the graph's private bookkeeping after GraphClear, which the public
    // surface does not expose.
    result = result && VecLen(&graph.slots) == slot_count && VecLen(&graph.free_indices) == slot_count;
    result = result && VecCapacity(&graph.slots) == slot_capacity && VecCapacity(&graph.free_indices) >= slot_count;
    result = result && graph.pending_delete_count == 0 && VecLen(&graph.pending_edge_removals) == 0;

    for (slot_index = 0; slot_index < VecLen(&graph.slots); slot_index++) {
        // `graph.slots` is the typed `Vec(GraphSlot(int))`, so iterate via
        // the runtime-shared layout to avoid an anonymous-struct annotation.
        GenericGraphSlot *slot = (GenericGraphSlot *)VecPtrAt(&GENERIC_GRAPH(&graph)->slots, slot_index);
        result                 = result && (slot->data == NULL);
        result                 = result && (slot->visit_count == 0);
        result                 = result && (slot->flags == 0);
        result                 = result && (VecLen(&slot->out_neighbors) == 0);
        result                 = result && (VecLen(&slot->in_neighbors) == 0);
    }

    result = result && (VecPtrAt(&graph.slots, GraphNodeIdIndex(first_id))->generation == (first_generation + 1));
    result = result && (VecPtrAt(&graph.slots, GraphNodeIdIndex(second_id))->generation == (second_generation + 1));
    result = result && (VecPtrAt(&graph.slots, GraphNodeIdIndex(third_id))->generation == (third_generation + 1));

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_graph_node_deep_copy(void) {
    WriteFmt("Testing Graph node deep-copy\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(Str) StrGraph;
    StrGraph    graph = GraphInitWithDeepCopy(str_init_copy, str_deinit, &alloc);
    Str         name  = StrInitFromZstr("alpha", &alloc);
    GraphNodeId node_id;
    GraphNode   node;
    Str        *stored_name;

    node_id     = GraphAddNodeL(&graph, name);
    node        = GraphGetNode(&graph, node_id);
    stored_name = GraphNodeDataPtr(&graph, node);

    bool result = GraphNodeIdIndex(node_id) == 0 && StrBegin(&name) != NULL && GraphNodeCount(&graph) == 1 &&
                  ZstrCompare(StrBegin(stored_name), "alpha") == 0 && StrBegin(stored_name) != StrBegin(&name);

    StrDeinit(&name);
    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_graph_node_owned_str_rvalue(void) {
    WriteFmt("Testing Graph node owned Str r-value insertion\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(Str) StrGraph;
    StrGraph    graph = GraphInitWithDeepCopy(NULL, str_deinit, &alloc);
    GraphNodeId node_id;
    GraphNode   node;
    Str        *stored_name;

    node_id     = GraphAddNodeR(&graph, StrZ("alpha", &alloc));
    node        = GraphGetNode(&graph, node_id);
    stored_name = GraphNodeDataPtr(&graph, node);

    bool result = GraphNodeIdIndex(node_id) == 0 && GraphNodeCount(&graph) == 1 && StrBegin(stored_name) != NULL &&
                  ZstrCompare(StrBegin(stored_name), "alpha") == 0;

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_graph_init_optional_allocator(void) {
    WriteFmt("Testing Graph init optional allocator\n");

    typedef Graph(Str) StrGraph;

    // intentional bypass: no public setter on `Allocator` for effort /
    // retry_limit -- pre-seeded directly so the inheritance path below
    // can be observed end-to-end.
    DefaultAllocator alloc = DefaultAllocatorInit();
    alloc.base.retry_limit = 31;

    HeapAllocator aligned_8     = HeapAllocatorInitAligned(8);
    aligned_8.base.retry_limit  = 31;
    HeapAllocator aligned_16    = HeapAllocatorInitAligned(16);
    aligned_16.base.retry_limit = 31;
    HeapAllocator aligned_32    = HeapAllocatorInitAligned(32);
    aligned_32.base.retry_limit = 31;
    HeapAllocator aligned_64    = HeapAllocatorInitAligned(64);
    aligned_64.base.retry_limit = 31;

    StrGraph graph_a = GraphInit(&alloc);
    StrGraph graph_b = GraphInitT(graph_b, &alloc);
    StrGraph graph_c = GraphInitWithDeepCopy(str_init_copy, str_deinit, &alloc);
    StrGraph graph_d = GraphInitWithDeepCopyT(graph_d, str_init_copy, str_deinit, &alloc);
    StrGraph graph_e = GraphInit(&aligned_8);
    StrGraph graph_f = GraphInitT(graph_f, &aligned_16);
    StrGraph graph_g = GraphInitWithDeepCopy(str_init_copy, str_deinit, &aligned_32);
    StrGraph graph_h = GraphInitWithDeepCopyT(graph_h, str_init_copy, str_deinit, &aligned_64);

    // intentional bypass: the inner `slots` / `free_indices` /
    // `pending_edge_removals` vectors are private graph fields with no
    // public accessor -- they are read directly here to verify the
    // top-level allocator is propagated all the way down to the inner
    // storage of the deep-copy / typed-init variants.
    bool result = (GraphAllocator(&graph_a)->retry_limit == 31) && (GraphAllocator(&graph_b)->retry_limit == 31);
    result = result && (GraphAllocator(&graph_c)->retry_limit == 31) && (GraphAllocator(&graph_d)->retry_limit == 31);
    result = result && (GraphAllocator(&graph_e)->retry_limit == 31) && (GraphAllocator(&graph_f)->retry_limit == 31);
    result = result && (GraphAllocator(&graph_g)->retry_limit == 31) && (GraphAllocator(&graph_h)->retry_limit == 31);
    result = result && (GraphAllocator(&graph_e)->alignment == 8) && (GraphAllocator(&graph_f)->alignment == 16);
    result = result && (GraphAllocator(&graph_g)->alignment == 32) && (GraphAllocator(&graph_h)->alignment == 64);
    result = result && (GraphCopyInit(&graph_c) == (GenericCopyInit)str_init_copy);
    result = result && (GraphCopyDeinit(&graph_d) == (GenericCopyDeinit)str_deinit);
    result = result && (graph_h.slots.allocator->retry_limit == 31);
    result = result && (graph_h.free_indices.allocator->retry_limit == 31);
    result = result && (graph_h.pending_edge_removals.allocator->retry_limit == 31);

    GraphDeinit(&graph_a);
    GraphDeinit(&graph_b);
    GraphDeinit(&graph_c);
    GraphDeinit(&graph_d);
    GraphDeinit(&graph_e);
    GraphDeinit(&graph_f);
    GraphDeinit(&graph_g);
    GraphDeinit(&graph_h);

    HeapAllocatorDeinit(&aligned_64);
    HeapAllocatorDeinit(&aligned_32);
    HeapAllocatorDeinit(&aligned_16);
    HeapAllocatorDeinit(&aligned_8);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// GraphReserve must actually reserve storage. Mutant: reserve_graph's success
// expression is replaced so the underlying reserve_vec calls never run, yet the
// op reports success; capacity then never grows. Real code grows slot capacity
// to at least the requested count.
static bool test_graph_reserve_grows_capacity(void) {
    WriteFmt("Testing GraphReserve actually grows slot capacity\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    bool result = GraphReserve(&graph, 64);
    result      = result && (VecCapacity(&graph.slots) >= 64);

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// deinit_graph, line 463: deinit_vec(&graph->pending_edge_removals, ...)
//
// The pending_edge_removals Vec gains heap backing when an edge is marked for
// removal. clear_graph clears it (keeping the backing via clear_vec), so at line
// 463 the backing is still outstanding. Removing the line-463 deinit leaks it.
// Mark (but do not commit) an edge removal so a backing exists at teardown.
static bool test_deinit_frees_pending_edge_removals_backing_no_leak(void) {
    WriteFmt("Testing GraphDeinit frees the pending-edge-removals backing (no leak)\n");

    DebugAllocator dbg = DebugAllocatorInit();

    typedef Graph(int) IntGraph;
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
    TestFunction tests[] = {
        test_graph_reserve_clear,
        test_graph_node_deep_copy,
        test_graph_node_owned_str_rvalue,
        test_graph_init_optional_allocator,
        test_graph_reserve_grows_capacity,
        test_deinit_frees_pending_edge_removals_backing_no_leak,
    };

    WriteFmt("[INFO] Starting Graph.Init tests\n\n");
    return run_test_suite(tests, (int)(sizeof(tests) / sizeof(tests[0])), NULL, 0, "Graph.Init");
}
