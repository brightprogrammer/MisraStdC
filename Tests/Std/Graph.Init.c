#include <Misra/Std/Container/Graph.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Log.h>

#include "../Util/TestRunner.h"

static bool test_graph_reserve_clear(void) {
    WriteFmt("Testing GraphReserve and GraphClear\n");

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit();

    GraphReserve(&graph, 8);
    ValidateGraph(&graph);

    bool result = graph.nodes.capacity >= 8 && graph.out_neighbors.capacity >= 8;

    GraphAddNodeR(&graph, 10);
    GraphAddNodeR(&graph, 20);
    result = result && GraphAddEdge(&graph, 0, 1);

    size node_capacity      = graph.nodes.capacity;
    size neighbor_capacity  = graph.out_neighbors.capacity;

    GraphClear(&graph);

    result = result && GraphNodeCount(&graph) == 0 && GraphEdgeCount(&graph) == 0;
    result = result && graph.nodes.capacity == node_capacity;
    result = result && graph.out_neighbors.capacity == neighbor_capacity;

    GraphDeinit(&graph);
    return result;
}

static bool test_graph_node_deep_copy(void) {
    WriteFmt("Testing Graph node deep-copy\n");

    typedef Graph(Str) StrGraph;
    StrGraph graph = GraphInitWithDeepCopy(StrInitCopy, StrDeinit);
    Str      name  = StrInitFromZstr("alpha");

    GraphNodeId node_id = GraphAddNodeL(&graph, name);

    bool result = node_id == 0 && name.data != NULL && GraphNodeCount(&graph) == 1 &&
                  ZstrCompare(GraphNodePtrAt(&graph, 0)->data, "alpha") == 0 &&
                  GraphNodePtrAt(&graph, 0)->data != name.data;

    StrDeinit(&name);
    GraphDeinit(&graph);
    return result;
}

int main(void) {
    TestFunction tests[] = {
        test_graph_reserve_clear,
        test_graph_node_deep_copy,
    };

    WriteFmt("[INFO] Starting Graph.Init tests\n\n");
    return run_test_suite(tests, (int)(sizeof(tests) / sizeof(tests[0])), NULL, 0, "Graph.Init");
}
