#include <Misra/Std/Container/Graph.h>
#include <Misra/Std/Log.h>

#include "../Util/TestRunner.h"

static bool test_graph_type_defaults(void) {
    WriteFmt("Testing Graph defaults\n");

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit();

    ValidateGraph(&graph);

    bool result = GraphNodeCount(&graph) == 0 && GraphEdgeCount(&graph) == 0 && GraphEmpty(&graph) &&
                  graph.nodes.data == NULL && graph.out_neighbors.data == NULL && graph.nodes.copy_init == NULL &&
                  graph.nodes.copy_deinit == NULL && graph.out_neighbors.copy_init == graph_neighbors_init_copy &&
                  graph.out_neighbors.copy_deinit == graph_neighbors_deinit && graph.nodes.alignment == 1;

    GraphDeinit(&graph);
    return result;
}

static bool test_graph_aligned_init(void) {
    WriteFmt("Testing Graph aligned init\n");

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInitAligned(32);

    ValidateGraph(&graph);

    bool result = graph.nodes.alignment == 32 && graph.out_neighbors.length == 0 && graph.edge_count == 0;

    GraphDeinit(&graph);
    return result;
}

int main(void) {
    TestFunction tests[] = {
        test_graph_type_defaults,
        test_graph_aligned_init,
    };

    WriteFmt("[INFO] Starting Graph.Type tests\n\n");
    return run_test_suite(tests, (int)(sizeof(tests) / sizeof(tests[0])), NULL, 0, "Graph.Type");
}
