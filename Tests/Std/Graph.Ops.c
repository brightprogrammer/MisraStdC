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
    result = result && (GraphInDegree(&graph, a) == 1);
    result = result && (GraphInDegree(&graph, c) == 0);
    result = result && GraphHasEdge(&graph, c, a);
    result = result && (GraphPredecessorAt(&graph, a, 0) == c);

    GraphNodeId d = GraphAddNodeR(&graph, 40);

    result = result && (GraphNodeIdIndex(d) == GraphNodeIdIndex(b));
    result = result && (GraphNodeIdGeneration(d) == (GraphNodeIdGeneration(b) + 1));
    result = result && (GraphNodeData(&graph, GraphGetNode(&graph, d)) == 40);
    result = result && !GraphNodeVisited(GraphGetNode(&graph, d));

    GraphDeinit(&graph);
    return result;
}

static bool test_graph_query_and_unmark_node_deletion(void) {
    WriteFmt("Testing Graph node mark query and unmark\n");

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit();

    GraphNodeId a    = GraphAddNodeR(&graph, 10);
    GraphNode   node = GraphGetNode(&graph, a);

    bool result = !GraphNodeMarkedForDeletion(node);
    result      = result && GraphMarkNodeForDeletion(node);
    result      = result && GraphNodeMarkedForDeletion(node);
    result      = result && !GraphMarkNodeForDeletion(node);
    result      = result && GraphUnmarkNodeForDeletion(node);
    result      = result && !GraphNodeMarkedForDeletion(node);
    result      = result && !GraphUnmarkNodeForDeletion(node);
    result      = result && (GraphCommitChanges(&graph) == 0);
    result      = result && GraphContainsNode(&graph, a);
    result      = result && (GraphNodeCount(&graph) == 1);

    GraphDeinit(&graph);
    return result;
}

static bool test_graph_mark_edge_for_removal(void) {
    WriteFmt("Testing GraphMarkEdgeForRemoval and deferred edge commit\n");

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit();

    GraphNodeId a = GraphAddNodeR(&graph, 10);
    GraphNodeId b = GraphAddNodeR(&graph, 20);
    GraphNodeId c = GraphAddNodeR(&graph, 30);

    GraphAddEdge(&graph, a, b);
    GraphAddEdge(&graph, a, c);
    GraphAddEdge(&graph, b, c);

    GraphForeachNode(&graph, node) {
        if (GraphNodeGetId(node) == a) {
            GraphNodeForeachNeighbor(node, neighbor) {
                if (GraphNodeGetId(neighbor) == b) {
                    (void)GraphMarkEdgeForRemoval(&graph, GraphNodeGetId(node), GraphNodeGetId(neighbor));
                }
            }
        }
    }

    bool result = GraphHasEdge(&graph, a, b);
    result      = result && GraphMarkEdgeForRemoval(&graph, b, c);
    result      = result && !GraphMarkEdgeForRemoval(&graph, b, c);
    result      = result && !GraphMarkEdgeForRemoval(&graph, c, b);

    u64 committed = GraphCommitChanges(&graph);

    result = result && (committed == 2);
    result = result && !GraphHasEdge(&graph, a, b);
    result = result && GraphHasEdge(&graph, a, c);
    result = result && !GraphHasEdge(&graph, b, c);
    result = result && (GraphEdgeCount(&graph) == 1);
    result = result && (GraphOutDegree(&graph, a) == 1);
    result = result && (GraphInDegree(&graph, b) == 0);
    result = result && (GraphInDegree(&graph, c) == 1);
    result = result && (GraphNeighborAt(&graph, a, 0) == c);
    result = result && (GraphPredecessorAt(&graph, c, 0) == a);

    GraphDeinit(&graph);
    return result;
}

static bool test_graph_query_and_unmark_edge_removal(void) {
    WriteFmt("Testing Graph edge mark query and unmark\n");

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit();

    GraphNodeId a = GraphAddNodeR(&graph, 10);
    GraphNodeId b = GraphAddNodeR(&graph, 20);

    GraphAddEdge(&graph, a, b);

    bool result = !GraphEdgeMarkedForRemoval(&graph, a, b);
    result      = result && GraphMarkEdgeForRemoval(&graph, a, b);
    result      = result && GraphEdgeMarkedForRemoval(&graph, a, b);
    result      = result && !GraphMarkEdgeForRemoval(&graph, a, b);
    result      = result && GraphUnmarkEdgeForRemoval(&graph, a, b);
    result      = result && !GraphEdgeMarkedForRemoval(&graph, a, b);
    result      = result && !GraphUnmarkEdgeForRemoval(&graph, a, b);
    result      = result && (GraphCommitChanges(&graph) == 0);
    result      = result && GraphHasEdge(&graph, a, b);
    result      = result && (GraphEdgeCount(&graph) == 1);

    GraphDeinit(&graph);
    return result;
}

static bool test_graph_partial_unmark_of_multiple_edge_removals(void) {
    WriteFmt("Testing partial unmark of multiple pending edge removals\n");

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit();

    GraphNodeId a = GraphAddNodeR(&graph, 10);
    GraphNodeId b = GraphAddNodeR(&graph, 20);
    GraphNodeId c = GraphAddNodeR(&graph, 30);

    GraphAddEdge(&graph, a, b);
    GraphAddEdge(&graph, a, c);

    bool result = GraphMarkEdgeForRemoval(&graph, a, b);
    result      = result && GraphMarkEdgeForRemoval(&graph, a, c);
    result      = result && GraphEdgeMarkedForRemoval(&graph, a, b);
    result      = result && GraphEdgeMarkedForRemoval(&graph, a, c);
    result      = result && GraphUnmarkEdgeForRemoval(&graph, a, b);
    result      = result && !GraphEdgeMarkedForRemoval(&graph, a, b);
    result      = result && GraphEdgeMarkedForRemoval(&graph, a, c);
    result      = result && (GraphCommitChanges(&graph) == 1);
    result      = result && GraphHasEdge(&graph, a, b);
    result      = result && !GraphHasEdge(&graph, a, c);
    result      = result && (GraphOutDegree(&graph, a) == 1);
    result      = result && (GraphInDegree(&graph, b) == 1);
    result      = result && (GraphInDegree(&graph, c) == 0);

    GraphDeinit(&graph);
    return result;
}

static bool test_graph_self_loop_edge_removal(void) {
    WriteFmt("Testing deferred removal of self-loop edge\n");

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit();

    GraphNodeId a = GraphAddNodeR(&graph, 10);

    bool result = GraphAddEdge(&graph, a, a);
    result      = result && (GraphOutDegree(&graph, a) == 1);
    result      = result && (GraphInDegree(&graph, a) == 1);
    result      = result && (GraphNeighborAt(&graph, a, 0) == a);
    result      = result && (GraphPredecessorAt(&graph, a, 0) == a);
    result      = result && GraphMarkEdgeForRemoval(&graph, a, a);
    result      = result && (GraphCommitChanges(&graph) == 1);
    result      = result && (GraphEdgeCount(&graph) == 0);
    result      = result && (GraphOutDegree(&graph, a) == 0);
    result      = result && (GraphInDegree(&graph, a) == 0);

    GraphDeinit(&graph);
    return result;
}

static bool test_graph_stale_node_handle_after_commit_deadend(void) {
    WriteFmt("Testing stale GraphNode handle after commit (should abort)\n");

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit();

    GraphNodeId a    = GraphAddNodeR(&graph, 10);
    GraphNode   node = GraphGetNode(&graph, a);

    (void)GraphMarkNodeForDeletion(node);
    (void)GraphCommitChanges(&graph);
    (void)GraphNodeVisit(node);

    GraphDeinit(&graph);
    return false;
}

int main(void) {
    TestFunction tests[] = {
        test_graph_node_visit_scratch_state,
        test_graph_mark_delete_commit_and_reuse,
        test_graph_query_and_unmark_node_deletion,
        test_graph_mark_edge_for_removal,
        test_graph_query_and_unmark_edge_removal,
        test_graph_partial_unmark_of_multiple_edge_removals,
        test_graph_self_loop_edge_removal,
    };
    TestFunction deadend_tests[] = {
        test_graph_stale_node_handle_after_commit_deadend,
    };

    WriteFmt("[INFO] Starting Graph.Ops tests\n\n");
    return run_test_suite(
        tests,
        (int)(sizeof(tests) / sizeof(tests[0])),
        deadend_tests,
        (int)(sizeof(deadend_tests) / sizeof(deadend_tests[0])),
        "Graph.Ops"
    );
}
