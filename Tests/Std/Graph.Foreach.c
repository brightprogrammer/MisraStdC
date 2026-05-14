#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Container/Graph.h>
#include <Misra/Std/Container/Map.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Memory.h>

#include "../Util/TestRunner.h"

static u64 zstr_hash(const void *data, u32 size) {
    const char          *str  = *(const char *const *)data;
    const unsigned char *ptr  = (const unsigned char *)str;
    u64                  hash = 1469598103934665603ULL;
    (void)size;

    while (*ptr) {
        hash ^= (u64)(*ptr++);
        hash *= 1099511628211ULL;
    }

    return hash;
}

static i32 zstr_compare_ptr(const void *lhs, const void *rhs) {
    const char *a = *(const char *const *)lhs;
    const char *b = *(const char *const *)rhs;
    return ZstrCompare(a, b);
}

static u64 node_id_hash(const void *data, u32 size) {
    u64 x = *(const GraphNodeId *)data;
    (void)size;

    x ^= x >> 33;
    x *= 0xff51afd7ed558ccdULL;
    x ^= x >> 33;
    x *= 0xc4ceb9fe1a85ec53ULL;
    x ^= x >> 33;
    return x;
}

static i32 node_id_compare(const void *lhs, const void *rhs) {
    GraphNodeId a = *(const GraphNodeId *)lhs;
    GraphNodeId b = *(const GraphNodeId *)rhs;
    return (a > b) - (a < b);
}

typedef Graph(Str)                 CityGraph;
typedef Map(const char *, GraphNodeId) CityIndex;

static GraphNodeId city_add_intersection(CityGraph *graph, CityIndex *index, const char *name, DefaultAllocator *alloc) {
    GraphNodeId id = GraphAddNodeR(graph, StrZ(name, alloc));

    MapInsertR(index, name, id);
    return id;
}

static void city_reset_visits(CityGraph *graph) {
    GraphForeachNode(graph, node) {
        GraphNodeUnvisit(node);
    }
}

static bool city_reachable_from(GraphNode node, GraphNodeId goal_id) {
    if (GraphNodeVisitCount(node) > 0) {
        return false;
    }

    GraphNodeVisit(node);
    if (GraphNodeGetId(node) == goal_id) {
        return true;
    }

    GraphNodeForeachNeighbor(node, neighbor) {
        if (city_reachable_from(neighbor, goal_id)) {
            return true;
        }
    }

    return false;
}

static bool city_reachable(CityGraph *graph, CityIndex *index, const char *from, const char *to) {
    GraphNodeId *from_id = MapTryGetPtr(index, from);
    GraphNodeId *to_id   = MapTryGetPtr(index, to);

    if (!from_id || !to_id) {
        return false;
    }

    city_reset_visits(graph);
    return city_reachable_from(GraphGetNode(graph, *from_id), *to_id);
}

static bool test_graph_city_reachability(void) {
    WriteFmt("Testing GraphForeachNode and GraphNodeForeachNeighbor for reachability\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    CityGraph graph = GraphInitWithDeepCopy(NULL, str_deinit, &alloc);
    CityIndex index = MapInitWithDeepCopy(zstr_hash, zstr_compare_ptr, zstr_init_clone, zstr_deinit, NULL, NULL, &alloc);

    GraphNodeId alpha = city_add_intersection(&graph, &index, "Alpha", &alloc);
    GraphNodeId beta  = city_add_intersection(&graph, &index, "Beta", &alloc);
    GraphNodeId gamma = city_add_intersection(&graph, &index, "Gamma", &alloc);
    GraphNodeId delta = city_add_intersection(&graph, &index, "Delta", &alloc);
    GraphNodeId echo  = city_add_intersection(&graph, &index, "Echo", &alloc);

    GraphAddEdge(&graph, alpha, beta);
    GraphAddEdge(&graph, beta, gamma);
    GraphAddEdge(&graph, gamma, delta);
    GraphAddEdge(&graph, gamma, echo);
    GraphAddEdge(&graph, echo, beta);

    bool result = city_reachable(&graph, &index, "Alpha", "Delta");
    result      = result && city_reachable(&graph, &index, "Echo", "Gamma");
    result      = result && !city_reachable(&graph, &index, "Delta", "Alpha");
    result      = result && !city_reachable(&graph, &index, "Unknown", "Alpha");

    city_reset_visits(&graph);
    GraphForeachNode(&graph, node) {
        result = result && (GraphNodeVisitCount(node) == 0);
    }

    MapDeinit(&index);
    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_graph_foreach_with_external_map_counts(void) {
    WriteFmt("Testing nested foreach with external count tracking map\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    typedef Map(GraphNodeId, u64) CountMap;

    IntGraph graph  = GraphInit(&alloc);
    CountMap counts = MapInit(node_id_hash, node_id_compare, &alloc);

    GraphNodeId a = GraphAddNodeR(&graph, 1);
    GraphNodeId b = GraphAddNodeR(&graph, 2);
    GraphNodeId c = GraphAddNodeR(&graph, 3);
    GraphNodeId d = GraphAddNodeR(&graph, 4);

    GraphAddEdge(&graph, a, b);
    GraphAddEdge(&graph, a, c);
    GraphAddEdge(&graph, b, d);
    GraphAddEdge(&graph, c, d);

    GraphForeachNode(&graph, node) {
        (void)MapEnsurePtr(&counts, GraphNodeGetId(node), 0);
        GraphNodeForeachNeighbor(node, neighbor) {
            u64 *count = MapEnsurePtr(&counts, GraphNodeGetId(neighbor), 0);
            *count += 1;
        }
    }

    bool result = *MapTryGetPtr(&counts, a) == 0;
    result      = result && *MapTryGetPtr(&counts, b) == 1;
    result      = result && *MapTryGetPtr(&counts, c) == 1;
    result      = result && *MapTryGetPtr(&counts, d) == 2;

    MapDeinit(&counts);
    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_graph_foreach_predecessors(void) {
    WriteFmt("Testing GraphNodeForeachPredecessor\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    GraphNodeId a = GraphAddNodeR(&graph, 1);
    GraphNodeId b = GraphAddNodeR(&graph, 2);
    GraphNodeId c = GraphAddNodeR(&graph, 3);
    GraphNodeId d = GraphAddNodeR(&graph, 4);

    GraphAddEdge(&graph, a, d);
    GraphAddEdge(&graph, b, d);
    GraphAddEdge(&graph, c, d);
    GraphAddEdge(&graph, a, b);

    u64 predecessor_sum = 0;
    u64 predecessor_count = 0;

    GraphNodeForeachPredecessor(GraphGetNode(&graph, d), predecessor) {
        predecessor_sum += GraphNodeData(&graph, predecessor);
        predecessor_count += 1;
    }

    bool result = (predecessor_count == 3);
    result      = result && (predecessor_sum == 6);
    result      = result && (GraphInDegree(&graph, d) == 3);
    result      = result && (GraphInDegree(&graph, a) == 0);

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_graph_node_iteration_rejects_structural_mutation_deadend(void) {
    WriteFmt("Testing GraphForeachNode rejects structural mutation (should abort)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    GraphAddNodeR(&graph, 1);
    GraphAddNodeR(&graph, 2);

    GraphForeachNode(&graph, node) {
        (void)node;
        (void)GraphAddNodeR(&graph, 3);
    }

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

static bool test_graph_neighbor_iteration_rejects_structural_mutation_deadend(void) {
    WriteFmt("Testing GraphNodeForeachNeighbor rejects structural mutation (should abort)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    GraphNodeId a = GraphAddNodeR(&graph, 1);
    GraphNodeId b = GraphAddNodeR(&graph, 2);
    GraphNodeId c = GraphAddNodeR(&graph, 3);

    GraphAddEdge(&graph, a, b);

    GraphNodeForeachNeighbor(GraphGetNode(&graph, a), neighbor) {
        (void)neighbor;
        (void)GraphAddEdge(&graph, a, c);
    }

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

static bool test_graph_predecessor_iteration_rejects_structural_mutation_deadend(void) {
    WriteFmt("Testing GraphNodeForeachPredecessor rejects structural mutation (should abort)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef Graph(int) IntGraph;
    IntGraph graph = GraphInit(&alloc);

    GraphNodeId a = GraphAddNodeR(&graph, 1);
    GraphNodeId b = GraphAddNodeR(&graph, 2);
    GraphNodeId c = GraphAddNodeR(&graph, 3);
    GraphNodeId d = GraphAddNodeR(&graph, 4);

    GraphAddEdge(&graph, a, c);
    GraphAddEdge(&graph, b, c);

    GraphNodeForeachPredecessor(GraphGetNode(&graph, c), predecessor) {
        (void)predecessor;
        (void)GraphAddEdge(&graph, d, c);
    }

    GraphDeinit(&graph);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

int main(void) {
    TestFunction tests[] = {
        test_graph_city_reachability,
        test_graph_foreach_with_external_map_counts,
        test_graph_foreach_predecessors,
    };
    TestFunction deadend_tests[] = {
        test_graph_node_iteration_rejects_structural_mutation_deadend,
        test_graph_neighbor_iteration_rejects_structural_mutation_deadend,
        test_graph_predecessor_iteration_rejects_structural_mutation_deadend,
    };

    WriteFmt("[INFO] Starting Graph.Foreach tests\n\n");
    return run_test_suite(
        tests,
        (int)(sizeof(tests) / sizeof(tests[0])),
        deadend_tests,
        (int)(sizeof(deadend_tests) / sizeof(deadend_tests[0])),
        "Graph.Foreach"
    );
}
