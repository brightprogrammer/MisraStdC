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
    GraphNodeId third_id         = GraphAddNodeR(&graph, 30);
    u64         slot_count       = graph.slots.length;
    size        slot_capacity    = graph.slots.capacity;
    u32         first_generation = GraphNodeIdGeneration(first_id);
    u32         second_generation = GraphNodeIdGeneration(second_id);
    u32         third_generation = GraphNodeIdGeneration(third_id);
    u64         slot_index;

    result = result && GraphAddEdge(&graph, first_id, second_id);
    result = result && GraphAddEdge(&graph, second_id, third_id);
    result = result && GraphAddEdge(&graph, third_id, first_id);
    result = result && GraphAddEdge(&graph, third_id, third_id);
    result = result && (GraphNodeVisit(GraphGetNode(&graph, first_id)) == 1);
    result = result && GraphMarkNodeForDeletion(GraphGetNode(&graph, second_id));
    result = result && GraphMarkEdgeForRemoval(&graph, third_id, third_id);

    GraphClear(&graph);

    result = result && GraphNodeCount(&graph) == 0 && GraphEdgeCount(&graph) == 0 && GraphEmpty(&graph);
    result = result && !GraphContainsNode(&graph, first_id) && !GraphContainsNode(&graph, second_id);
    result = result && !GraphContainsNode(&graph, third_id);
    result = result && graph.slots.length == slot_count && graph.free_indices.length == slot_count;
    result = result && graph.slots.capacity == slot_capacity && graph.free_indices.capacity >= slot_count;
    result = result && graph.pending_delete_count == 0 && graph.pending_edge_removals.length == 0;

    for (slot_index = 0; slot_index < graph.slots.length; slot_index++) {
        GenericGraphSlot *slot = VecPtrAt(&graph.slots, slot_index);
        result                 = result && (slot->data == NULL);
        result                 = result && (slot->visit_count == 0);
        result                 = result && (slot->flags == 0);
        result                 = result && (slot->out_neighbors.length == 0);
        result                 = result && (slot->in_neighbors.length == 0);
    }

    result = result && (VecPtrAt(&graph.slots, GraphNodeIdIndex(first_id))->generation == (first_generation + 1));
    result = result && (VecPtrAt(&graph.slots, GraphNodeIdIndex(second_id))->generation == (second_generation + 1));
    result = result && (VecPtrAt(&graph.slots, GraphNodeIdIndex(third_id))->generation == (third_generation + 1));

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
