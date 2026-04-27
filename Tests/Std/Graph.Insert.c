#include <Misra/Std/Container/Graph.h>
#include <Misra/Std/Log.h>

#include "../Util/TestRunner.h"

static bool test_graph_add_node_semantics(void) {
    WriteFmt("Testing GraphAddNode semantics\n");

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit();
    int      owned = 42;
    int      shared = 7;

    GraphNodeId id0 = GraphAddNodeL(&graph, owned);
    GraphNodeId id1 = GraphAddNodeR(&graph, shared);

    bool result = id0 == 0 && id1 == 1 && owned == 0 && shared == 7;
    result      = result && GraphNodeAt(&graph, 0) == 42;
    result      = result && GraphNodeAt(&graph, 1) == 7;

    GraphDeinit(&graph);
    return result;
}

static bool test_graph_add_edge_dedup(void) {
    WriteFmt("Testing GraphAddEdge deduplication\n");

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit();

    GraphAddNodeR(&graph, 1);
    GraphAddNodeR(&graph, 2);
    GraphAddNodeR(&graph, 3);

    bool result = GraphAddEdge(&graph, 0, 1);
    result      = result && GraphAddEdge(&graph, 0, 2);
    result      = result && !GraphAddEdge(&graph, 0, 1);
    result      = result && GraphEdgeCount(&graph) == 2;
    result      = result && GraphOutDegree(&graph, 0) == 2;
    result      = result && GraphNeighborAt(&graph, 0, 0) == 1;
    result      = result && GraphNeighborAt(&graph, 0, 1) == 2;

    GraphDeinit(&graph);
    return result;
}

int main(void) {
    TestFunction tests[] = {
        test_graph_add_node_semantics,
        test_graph_add_edge_dedup,
    };

    WriteFmt("[INFO] Starting Graph.Insert tests\n\n");
    return run_test_suite(tests, (int)(sizeof(tests) / sizeof(tests[0])), NULL, 0, "Graph.Insert");
}
