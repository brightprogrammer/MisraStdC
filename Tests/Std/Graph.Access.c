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
    GraphNode   node_b;
    GraphAddEdge(&graph, a, b);
    GraphAddEdge(&graph, a, c);
    GraphAddEdge(&graph, c, a);

    ValidateGraph(&graph);

    node_b = GraphGetNode(&graph, b);
    *GraphNodeDataPtr(&graph, node_b) = 25;

    bool result = GraphNodeCount(&graph) == 3 && GraphEdgeCount(&graph) == 3 && !GraphEmpty(&graph);
    result      = result && GraphContainsNode(&graph, a) && GraphContainsNode(&graph, b) && GraphContainsNode(&graph, c);
    result      = result && GraphNodeAt(&graph, b) == 25;
    result      = result && GraphNodeData(&graph, node_b) == 25;
    result      = result && GraphNodeGetId(node_b) == b;
    result      = result && GraphNodeIndex(node_b) == GraphNodeIdIndex(b);
    result      = result && GraphOutDegree(&graph, a) == 2;
    result      = result && GraphNeighborAt(&graph, a, 0) == b && GraphNeighborAt(&graph, a, 1) == c;
    result      = result && GraphNeighborAt(&graph, c, 0) == a;

    GraphDeinit(&graph);
    return result;
}

static bool test_graph_has_edge_query(void) {
    WriteFmt("Testing GraphHasEdge\n");

    typedef Graph(const char *) ZstrGraph;
    ZstrGraph graph = GraphInit();

    GraphNodeId red   = GraphAddNodeR(&graph, "red");
    GraphNodeId green = GraphAddNodeR(&graph, "green");
    GraphNodeId blue  = GraphAddNodeR(&graph, "blue");

    GraphAddEdge(&graph, red, green);
    GraphAddEdge(&graph, green, blue);

    bool result = GraphHasEdge(&graph, red, green);
    result      = result && GraphHasEdge(&graph, green, blue);
    result      = result && !GraphHasEdge(&graph, blue, green);
    result      = result && !GraphHasEdge(&graph, red, blue);
    result      = result && (ZstrCompare(*GraphNodePtrAt(&graph, red), "red") == 0);

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
