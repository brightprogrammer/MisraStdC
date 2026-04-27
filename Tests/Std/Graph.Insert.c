#include <Misra/Std/Container/Graph.h>
#include <Misra/Std/Log.h>

#include "../Util/TestRunner.h"

static bool test_graph_add_node_semantics(void) {
    WriteFmt("Testing GraphAddNode semantics\n");

    typedef Graph(int) IntGraph;
    IntGraph graph  = GraphInit();
    int      owned  = 42;
    int      shared = 7;

    GraphNodeId id0 = GraphAddNodeL(&graph, owned);
    GraphNodeId id1 = GraphAddNodeR(&graph, shared);

    bool result = GraphNodeIdIndex(id0) == 0 && GraphNodeIdGeneration(id0) == 1 && GraphNodeIdIndex(id1) == 1;
    result      = result && owned == 0 && shared == 7;
    result      = result && GraphNodeAt(&graph, id0) == 42;
    result      = result && GraphNodeAt(&graph, id1) == 7;

    GraphDeinit(&graph);
    return result;
}

static bool test_graph_add_edge_dedup(void) {
    WriteFmt("Testing GraphAddEdge deduplication\n");

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit();

    GraphNodeId a = GraphAddNodeR(&graph, 1);
    GraphNodeId b = GraphAddNodeR(&graph, 2);
    GraphNodeId c = GraphAddNodeR(&graph, 3);

    bool result = GraphAddEdge(&graph, a, b);
    result      = result && GraphAddEdge(&graph, a, c);
    result      = result && !GraphAddEdge(&graph, a, b);
    result      = result && GraphEdgeCount(&graph) == 2;
    result      = result && GraphOutDegree(&graph, a) == 2;
    result      = result && GraphNeighborAt(&graph, a, 0) == b;
    result      = result && GraphNeighborAt(&graph, a, 1) == c;

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
