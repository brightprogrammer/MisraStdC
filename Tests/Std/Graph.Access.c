#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Std/Container/Graph.h>
#include <Misra/Std/Log.h>

#include "../Util/TestRunner.h"

static bool test_graph_access_helpers(void) {
    WriteFmt("Testing Graph access helpers\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

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
    result      = result && GraphInDegree(&graph, a) == 1;
    result      = result && GraphInDegree(&graph, b) == 1;
    result      = result && GraphInDegree(&graph, c) == 1;
    result      = result && GraphNeighborAt(&graph, a, 0) == b && GraphNeighborAt(&graph, a, 1) == c;
    result      = result && GraphNeighborAt(&graph, c, 0) == a;
    result      = result && GraphPredecessorAt(&graph, a, 0) == c;
    result      = result && GraphPredecessorAt(&graph, b, 0) == a;
    result      = result && GraphPredecessorAt(&graph, c, 0) == a;

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_graph_has_edge_query(void) {
    WriteFmt("Testing GraphHasEdge\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(const char *) ZstrGraph;
    ZstrGraph graph = GraphInit(&alloc);

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
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_graph_cross_graph_node_handle_deadend(void) {
    WriteFmt("Testing GraphNodeData rejects foreign graph node handles (should abort)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph_a = GraphInit(&alloc);
    IntGraph graph_b = GraphInit(&alloc);
    GraphNode node   = GraphGetNode(&graph_a, GraphAddNodeR(&graph_a, 10));

    (void)GraphNodeData(&graph_b, node);

    GraphDeinit(&graph_b);
    GraphDeinit(&graph_a);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

static bool test_graph_predecessor_access_oob_deadend(void) {
    WriteFmt("Testing GraphPredecessorAt out-of-bounds access (should abort)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    GraphNodeId a = GraphAddNodeR(&graph, 10);
    GraphNodeId b = GraphAddNodeR(&graph, 20);

    GraphAddEdge(&graph, a, b);
    (void)GraphPredecessorAt(&graph, a, 0);

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

static bool test_graph_neighbor_access_oob_deadend(void) {
    WriteFmt("Testing GraphNeighborAt out-of-bounds access (should abort)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    GraphNodeId a = GraphAddNodeR(&graph, 10);
    GraphNodeId b = GraphAddNodeR(&graph, 20);

    GraphAddEdge(&graph, a, b);
    (void)GraphNeighborAt(&graph, b, 0);

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

int main(void) {
    TestFunction tests[] = {
        test_graph_access_helpers,
        test_graph_has_edge_query,
    };
    TestFunction deadend_tests[] = {
        test_graph_cross_graph_node_handle_deadend,
        test_graph_predecessor_access_oob_deadend,
        test_graph_neighbor_access_oob_deadend,
    };

    WriteFmt("[INFO] Starting Graph.Access tests\n\n");
    return run_test_suite(
        tests,
        (int)(sizeof(tests) / sizeof(tests[0])),
        deadend_tests,
        (int)(sizeof(deadend_tests) / sizeof(deadend_tests[0])),
        "Graph.Access"
    );
}
