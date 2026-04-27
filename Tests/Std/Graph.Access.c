#include <Misra/Std/Container/Graph.h>
#include <Misra/Std/Log.h>

#include "../Util/TestRunner.h"

static bool test_graph_access_helpers(void) {
    WriteFmt("Testing Graph access helpers\n");

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit();

    GraphNodeId a = GraphAddNodeR(&graph, 10);
    GraphNodeId b = GraphAddNodeR(&graph, 20);
    GraphNodeId c = GraphAddNodeR(&graph, 30);

    GraphAddEdge(&graph, a, b);
    GraphAddEdge(&graph, a, c);
    GraphAddEdge(&graph, c, a);

    ValidateGraph(&graph);

    int *node_b = GraphNodePtrAt(&graph, b);
    *node_b     = 25;

    GraphNeighbors *neighbors = GraphOutNeighborsPtr(&graph, a);
    GraphNodeId    *neighbor0 = GraphNeighborPtrAt(&graph, a, 0);
    GraphNodeId    *neighbor1 = GraphNeighborPtrAt(&graph, a, 1);

    bool result = GraphNodeCount(&graph) == 3 && GraphEdgeCount(&graph) == 3 && !GraphEmpty(&graph);
    result      = result && GraphNodeAt(&graph, b) == 25;
    result      = result && VecLen(neighbors) == 2;
    result      = result && GraphOutDegree(&graph, a) == 2;
    result      = result && *neighbor0 == b && *neighbor1 == c;
    result      = result && GraphNeighborAt(&graph, c, 0) == a;

    GraphDeinit(&graph);
    return result;
}

static bool test_graph_has_edge_query(void) {
    WriteFmt("Testing GraphHasEdge\n");

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit();

    GraphAddNodeR(&graph, 1);
    GraphAddNodeR(&graph, 2);
    GraphAddNodeR(&graph, 3);

    GraphAddEdge(&graph, 0, 1);
    GraphAddEdge(&graph, 1, 2);

    bool result = GraphHasEdge(&graph, 0, 1);
    result      = result && GraphHasEdge(&graph, 1, 2);
    result      = result && !GraphHasEdge(&graph, 2, 1);
    result      = result && !GraphHasEdge(&graph, 0, 2);

    GraphDeinit(&graph);
    return result;
}

int main(void) {
    TestFunction tests[] = {
        test_graph_access_helpers,
        test_graph_has_edge_query,
    };

    WriteFmt("[INFO] Starting Graph.Access tests\n\n");
    return run_test_suite(tests, (int)(sizeof(tests) / sizeof(tests[0])), NULL, 0, "Graph.Access");
}
