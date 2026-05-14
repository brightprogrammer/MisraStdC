#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Container/Graph.h>
#include <Misra/Std/Log.h>

#include "../Util/TestRunner.h"

static bool test_graph_add_node_semantics(void) {
    WriteFmt("Testing GraphAddNode semantics\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph  = GraphInit(&alloc);
    int      owned  = 42;
    int      shared = 7;

    GraphNodeId id0 = GraphAddNodeL(&graph, owned);
    GraphNodeId id1 = GraphAddNodeR(&graph, shared);

    bool result = GraphNodeIdIndex(id0) == 0 && GraphNodeIdGeneration(id0) == 1 && GraphNodeIdIndex(id1) == 1;
    result      = result && owned == 0 && shared == 7;
    result      = result && GraphNodeAt(&graph, id0) == 42;
    result      = result && GraphNodeAt(&graph, id1) == 7;

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_graph_add_edge_dedup(void) {
    WriteFmt("Testing GraphAddEdge deduplication\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    GraphNodeId a = GraphAddNodeR(&graph, 1);
    GraphNodeId b = GraphAddNodeR(&graph, 2);
    GraphNodeId c = GraphAddNodeR(&graph, 3);

    bool result = GraphAddEdge(&graph, a, b);
    result      = result && GraphAddEdge(&graph, a, c);
    result      = result && !GraphAddEdge(&graph, a, b);
    result      = result && GraphEdgeCount(&graph) == 2;
    result      = result && GraphOutDegree(&graph, a) == 2;
    result      = result && GraphInDegree(&graph, a) == 0;
    result      = result && GraphInDegree(&graph, b) == 1;
    result      = result && GraphInDegree(&graph, c) == 1;
    result      = result && GraphNeighborAt(&graph, a, 0) == b;
    result      = result && GraphNeighborAt(&graph, a, 1) == c;
    result      = result && GraphPredecessorAt(&graph, b, 0) == a;
    result      = result && GraphPredecessorAt(&graph, c, 0) == a;

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_graph_self_loop_and_predecessor_order(void) {
    WriteFmt("Testing Graph self-loop handling and predecessor order\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    GraphNodeId a = GraphAddNodeR(&graph, 1);
    GraphNodeId b = GraphAddNodeR(&graph, 2);
    GraphNodeId c = GraphAddNodeR(&graph, 3);

    bool result = GraphAddEdge(&graph, a, a);
    result      = result && GraphAddEdge(&graph, b, a);
    result      = result && GraphAddEdge(&graph, c, a);
    result      = result && !GraphAddEdge(&graph, a, a);
    result      = result && (GraphEdgeCount(&graph) == 3);
    result      = result && (GraphOutDegree(&graph, a) == 1);
    result      = result && (GraphInDegree(&graph, a) == 3);
    result      = result && (GraphNeighborAt(&graph, a, 0) == a);
    result      = result && (GraphPredecessorAt(&graph, a, 0) == a);
    result      = result && (GraphPredecessorAt(&graph, a, 1) == b);
    result      = result && (GraphPredecessorAt(&graph, a, 2) == c);

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

int main(void) {
    TestFunction tests[] = {
        test_graph_add_node_semantics,
        test_graph_add_edge_dedup,
        test_graph_self_loop_and_predecessor_order,
    };

    WriteFmt("[INFO] Starting Graph.Insert tests\n\n");
    return run_test_suite(tests, (int)(sizeof(tests) / sizeof(tests[0])), NULL, 0, "Graph.Insert");
}
