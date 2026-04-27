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

    bool        result           = graph.slots.capacity >= 8;
    GraphNodeId first_id         = GraphAddNodeR(&graph, 10);
    GraphNodeId second_id        = GraphAddNodeR(&graph, 20);
    u64         slot_count       = graph.slots.length;
    size        slot_capacity    = graph.slots.capacity;

    result = result && GraphAddEdge(&graph, first_id, second_id);

    GraphClear(&graph);

    result = result && GraphNodeCount(&graph) == 0 && GraphEdgeCount(&graph) == 0 && GraphEmpty(&graph);
    result = result && !GraphContainsNode(&graph, first_id) && !GraphContainsNode(&graph, second_id);
    result = result && graph.slots.length == slot_count && graph.free_indices.length == slot_count;
    result = result && graph.slots.capacity == slot_capacity && graph.free_indices.capacity >= slot_count;

    GraphDeinit(&graph);
    return result;
}

static bool test_graph_node_deep_copy(void) {
    WriteFmt("Testing Graph node deep-copy\n");

    typedef Graph(Str) StrGraph;
    StrGraph   graph = GraphInitWithDeepCopy(StrInitCopy, StrDeinit);
    Str        name  = StrInitFromZstr("alpha");
    GraphNodeId node_id;
    GraphNode   node;
    Str        *stored_name;

    node_id     = GraphAddNodeL(&graph, name);
    node        = GraphGetNode(&graph, node_id);
    stored_name = GraphNodeDataPtr(&graph, node);

    bool result = GraphNodeIdIndex(node_id) == 0 && name.data != NULL && GraphNodeCount(&graph) == 1 &&
                  ZstrCompare(stored_name->data, "alpha") == 0 && stored_name->data != name.data;

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
