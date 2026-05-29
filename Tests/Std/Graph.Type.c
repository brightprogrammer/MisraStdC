#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Allocator/Heap.h>
#include <Misra/Std/Container/Graph.h>
#include <Misra/Std/Log.h>

#include "../Util/TestRunner.h"

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

int main(void) {
    TestFunction tests[] = {
        test_graph_type_defaults,
        test_graph_aligned_init_and_id_layout,
    };

    WriteFmt("[INFO] Starting Graph.Type tests\n\n");
    return run_test_suite(tests, (int)(sizeof(tests) / sizeof(tests[0])), NULL, 0, "Graph.Type");
}
