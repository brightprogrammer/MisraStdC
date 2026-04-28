#include <Misra/Std/Container/Graph.h>
#include <Misra/Std/Log.h>

#include "../Util/TestRunner.h"

static bool test_graph_type_defaults(void) {
    WriteFmt("Testing Graph defaults\n");

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit();

    ValidateGraph(&graph);

    bool result = GraphNodeCount(&graph) == 0 && GraphEdgeCount(&graph) == 0 && GraphEmpty(&graph) &&
                  graph.slots.data == NULL && graph.free_indices.data == NULL &&
                  graph.pending_edge_removals.data == NULL && graph.copy_init == NULL && graph.copy_deinit == NULL &&
                  graph.live_count == 0 && graph.pending_delete_count == 0 && graph.mutation_epoch == 0 &&
                  graph.alignment == 1 && graph.type_anchor == NULL;

    GraphDeinit(&graph);
    return result;
}

static bool test_graph_aligned_init_and_id_layout(void) {
    WriteFmt("Testing Graph aligned init and node id layout\n");

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInitAligned(32);

    GraphNodeId node_id = GraphAddNodeR(&graph, 11);
    GraphNode   node    = GraphGetNode(&graph, node_id);

    bool result = graph.alignment == 32 && GraphNodeIdIndex(node_id) == 0 && GraphNodeIdGeneration(node_id) == 1;
    result      = result && GraphNodeGetId(node) == node_id;
    result      = result && GraphNodeIndex(node) == 0;
    result      = result && GraphContainsNode(&graph, node_id);

    GraphDeinit(&graph);
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
