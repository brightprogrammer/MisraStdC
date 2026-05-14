#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Allocator/Heap.h>
#include <Misra/Std/Container/Graph.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Log.h>

#include "../Util/TestRunner.h"

static bool test_graph_reserve_clear(void) {
    WriteFmt("Testing GraphReserve and GraphClear\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    GraphReserve(&graph, 8);
    ValidateGraph(&graph);

    bool        result            = graph.slots.capacity >= 8;
    GraphNodeId first_id          = GraphAddNodeR(&graph, 10);
    GraphNodeId second_id         = GraphAddNodeR(&graph, 20);
    GraphNodeId third_id          = GraphAddNodeR(&graph, 30);
    u64         slot_count        = graph.slots.length;
    size        slot_capacity     = graph.slots.capacity;
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
    result = result && graph.slots.length == slot_count && graph.free_indices.length == slot_count;
    result = result && graph.slots.capacity == slot_capacity && graph.free_indices.capacity >= slot_count;
    result = result && graph.pending_delete_count == 0 && graph.pending_edge_removals.length == 0;

    for (slot_index = 0; slot_index < graph.slots.length; slot_index++) {
        // `graph.slots` is the typed `Vec(GraphSlot(int))`, so iterate via
        // the runtime-shared layout to avoid an anonymous-struct annotation.
        GenericGraphSlot *slot = (GenericGraphSlot *)VecPtrAt(&GENERIC_GRAPH(&graph)->slots, slot_index);
        result                 = result && (slot->data == NULL);
        result                 = result && (slot->visit_count == 0);
        result                 = result && (slot->flags == 0);
        result                 = result && (slot->out_neighbors.length == 0);
        result                 = result && (slot->in_neighbors.length == 0);
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
    StrGraph    graph = GraphInitWithDeepCopy(StrInitCopyAlloc, StrDeinitAlloc, &alloc);
    Str         name  = StrInitFromZstr("alpha", &alloc);
    GraphNodeId node_id;
    GraphNode   node;
    Str        *stored_name;

    node_id     = GraphAddNodeL(&graph, name);
    node        = GraphGetNode(&graph, node_id);
    stored_name = GraphNodeDataPtr(&graph, node);

    bool result = GraphNodeIdIndex(node_id) == 0 && name.data != NULL && GraphNodeCount(&graph) == 1 &&
                  ZstrCompare(stored_name->data, "alpha") == 0 && stored_name->data != name.data;

    StrDeinit(&name);
    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_graph_node_owned_str_rvalue(void) {
    WriteFmt("Testing Graph node owned Str r-value insertion\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(Str) StrGraph;
    StrGraph    graph = GraphInitWithDeepCopy(NULL, StrDeinitAlloc, &alloc);
    GraphNodeId node_id;
    GraphNode   node;
    Str        *stored_name;

    node_id     = GraphAddNodeR(&graph, StrZ("alpha", &alloc));
    node        = GraphGetNode(&graph, node_id);
    stored_name = GraphNodeDataPtr(&graph, node);

    bool result = GraphNodeIdIndex(node_id) == 0 && GraphNodeCount(&graph) == 1 && stored_name->data != NULL &&
                  ZstrCompare(stored_name->data, "alpha") == 0;

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_graph_init_optional_allocator(void) {
    WriteFmt("Testing Graph init optional allocator\n");

    typedef Graph(Str) StrGraph;

    DefaultAllocator alloc = DefaultAllocatorInit();
    alloc.base.retry_limit = 31;

    HeapAllocator aligned_8 = HeapAllocatorInitAligned(8);
    aligned_8.base.retry_limit = 31;
    HeapAllocator aligned_16 = HeapAllocatorInitAligned(16);
    aligned_16.base.retry_limit = 31;
    HeapAllocator aligned_32 = HeapAllocatorInitAligned(32);
    aligned_32.base.retry_limit = 31;
    HeapAllocator aligned_64 = HeapAllocatorInitAligned(64);
    aligned_64.base.retry_limit = 31;

    StrGraph graph_a = GraphInit(&alloc);
    StrGraph graph_b = GraphInitT(graph_b, &alloc);
    StrGraph graph_c = GraphInitWithDeepCopy(StrInitCopyAlloc, StrDeinitAlloc, &alloc);
    StrGraph graph_d = GraphInitWithDeepCopyT(graph_d, StrInitCopyAlloc, StrDeinitAlloc, &alloc);
    StrGraph graph_e = GraphInit(&aligned_8);
    StrGraph graph_f = GraphInitT(graph_f, &aligned_16);
    StrGraph graph_g = GraphInitWithDeepCopy(StrInitCopyAlloc, StrDeinitAlloc, &aligned_32);
    StrGraph graph_h = GraphInitWithDeepCopyT(graph_h, StrInitCopyAlloc, StrDeinitAlloc, &aligned_64);

    bool result = (graph_a.allocator->retry_limit == 31) && (graph_b.allocator->retry_limit == 31);
    result      = result && (graph_c.allocator->retry_limit == 31) && (graph_d.allocator->retry_limit == 31);
    result      = result && (graph_e.allocator->retry_limit == 31) && (graph_f.allocator->retry_limit == 31);
    result      = result && (graph_g.allocator->retry_limit == 31) && (graph_h.allocator->retry_limit == 31);
    result      = result && (graph_e.allocator->alignment == 8) && (graph_f.allocator->alignment == 16);
    result      = result && (graph_g.allocator->alignment == 32) && (graph_h.allocator->alignment == 64);
    result      = result && (graph_c.copy_init == (GenericCopyInit)StrInitCopyAlloc);
    result      = result && (graph_d.copy_deinit == (GenericCopyDeinit)StrDeinitAlloc);
    result      = result && (graph_h.slots.allocator->retry_limit == 31);
    result      = result && (graph_h.free_indices.allocator->retry_limit == 31);
    result      = result && (graph_h.pending_edge_removals.allocator->retry_limit == 31);

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

int main(void) {
    TestFunction tests[] = {
        test_graph_reserve_clear,
        test_graph_node_deep_copy,
        test_graph_node_owned_str_rvalue,
        test_graph_init_optional_allocator,
    };

    WriteFmt("[INFO] Starting Graph.Init tests\n\n");
    return run_test_suite(tests, (int)(sizeof(tests) / sizeof(tests[0])), NULL, 0, "Graph.Init");
}
