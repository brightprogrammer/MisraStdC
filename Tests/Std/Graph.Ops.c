#include <Misra/Std/Container/Graph.h>
#include <Misra/Std/Log.h>

#include "../Util/TestRunner.h"

static bool test_graph_node_visit_scratch_state(void) {
    WriteFmt("Testing Graph node scratch visit state\n");

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit();

    GraphNodeId a    = GraphAddNodeR(&graph, 10);
    GraphNode   node = GraphGetNode(&graph, a);

    bool result = !GraphNodeVisited(node) && (GraphNodeVisitCount(node) == 0);

    GraphNodeVisit(node);
    GraphNodeVisit(node);

    result = result && GraphNodeVisited(node) && (GraphNodeVisitCount(node) == 2);

    GraphNodeUnvisit(node);
    result = result && !GraphNodeVisited(node) && (GraphNodeVisitCount(node) == 0);

    GraphDeinit(&graph);
    return result;
}

static bool test_graph_mark_delete_commit_and_reuse(void) {
    WriteFmt("Testing GraphMarkNodeForDeletion and GraphCommitChanges\n");

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit();

    GraphNodeId a = GraphAddNodeR(&graph, 10);
    GraphNodeId b = GraphAddNodeR(&graph, 20);
    GraphNodeId c = GraphAddNodeR(&graph, 30);

    GraphAddEdge(&graph, a, b);
    GraphAddEdge(&graph, b, c);
    GraphAddEdge(&graph, c, a);

    GraphForeachNode(&graph, node) {
        if (GraphNodeData(&graph, node) == 20) {
            GraphMarkNodeForDeletion(node);
        }
    }

    bool result = GraphContainsNode(&graph, b) && (graph.pending_delete_count == 1);
    u64  removed = GraphCommitChanges(&graph);

    result = result && (removed == 1);
    result = result && (GraphNodeCount(&graph) == 2);
    result = result && (GraphEdgeCount(&graph) == 1);
    result = result && !GraphContainsNode(&graph, b);
    result = result && (GraphOutDegree(&graph, a) == 0);
    result = result && GraphHasEdge(&graph, c, a);

    GraphNodeId d = GraphAddNodeR(&graph, 40);

    result = result && (GraphNodeIdIndex(d) == GraphNodeIdIndex(b));
    result = result && (GraphNodeIdGeneration(d) == (GraphNodeIdGeneration(b) + 1));
    result = result && (GraphNodeData(&graph, GraphGetNode(&graph, d)) == 40);
    result = result && !GraphNodeVisited(GraphGetNode(&graph, d));

    GraphDeinit(&graph);
    return result;
}

int main(void) {
    TestFunction tests[] = {
        test_graph_node_visit_scratch_state,
        test_graph_mark_delete_commit_and_reuse,
    };

    WriteFmt("[INFO] Starting Graph.Ops tests\n\n");
    return run_test_suite(tests, (int)(sizeof(tests) / sizeof(tests[0])), NULL, 0, "Graph.Ops");
}
