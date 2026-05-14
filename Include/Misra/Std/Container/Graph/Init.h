/// file      : std/container/graph/init.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Initializers for Graph.

#ifndef MISRA_STD_CONTAINER_GRAPH_INIT_H
#define MISRA_STD_CONTAINER_GRAPH_INIT_H

#include "Private.h"
#include "Type.h"

///
/// Initialize graph. Node payload alignment is derived from the allocator's
/// `alignment` field.
/// An allocator can be passed as the final optional argument. If omitted, DefaultAllocator() is used.
/// To request stronger alignment for node payloads, pass an allocator built via
/// `HeapAllocatorAligned(n)` (or set the allocator's `alignment` field directly).
///
/// USAGE:
///   Graph(int) graph = GraphInit();
///   Graph(int) arena_graph = GraphInit(arena_allocator);
///   Graph(SimdNode) wide_graph = GraphInit(HeapAllocatorAligned(32));
///
/// TAGS: Graph, Init, Directed
///
#define GRAPH_INIT_HAS_ARGS_IMPL(_0, _1, count, ...) count
#define GRAPH_INIT_HAS_ARGS(...)                     GRAPH_INIT_HAS_ARGS_IMPL(__VA_OPT__(, ) __VA_ARGS__, 1, 0, 0)
#define GraphInit(...)                               CONCAT(GraphInit_, GRAPH_INIT_HAS_ARGS(__VA_ARGS__))(__VA_ARGS__)
#define GraphInit_0()                                GRAPH_INIT_WITH_DEEP_COPY_VALUE(NULL, NULL, DefaultAllocator())
#define GraphInit_1(alloc)                           GRAPH_INIT_WITH_DEEP_COPY_VALUE(NULL, NULL, (alloc))

///
/// Initialize given graph.
///
/// g[in]     : Variable or type of a graph to be initialized.
/// alloc[in] : Optional allocator copied into the graph. If omitted, DefaultAllocator() is used.
///
/// TAGS: Graph, Init, Directed
///
#define GRAPH_INIT_T_HAS_ARGS_IMPL(_1, _2, count, ...) count
#define GRAPH_INIT_T_HAS_ARGS(...)                     GRAPH_INIT_T_HAS_ARGS_IMPL(__VA_ARGS__, 2, 1, 0)
#define GraphInitT(...)                                CONCAT(GraphInitT_, GRAPH_INIT_T_HAS_ARGS(__VA_ARGS__))(__VA_ARGS__)
#define GraphInitT_1(g)                                GraphInitWithDeepCopyT((g), NULL, NULL)
#define GraphInitT_2(g, alloc)                         GraphInitWithDeepCopyT((g), NULL, NULL, (alloc))

///
/// Initialize graph with deep-copy callbacks for node payloads.
///
/// ci[in]    : Optional deep-copy callback for nodes.
/// cd[in]    : Optional deinit callback for nodes.
/// alloc[in] : Optional allocator copied into the graph. If omitted, DefaultAllocator() is used.
///
/// TAGS: Graph, Init, DeepCopy, Directed
///
#define GRAPH_INIT_WITH_DEEP_COPY_HAS_ARGS_IMPL(_1, _2, _3, count, ...) count
#define GRAPH_INIT_WITH_DEEP_COPY_HAS_ARGS(...)                         GRAPH_INIT_WITH_DEEP_COPY_HAS_ARGS_IMPL(__VA_ARGS__, 3, 2, 1, 0)
#define GraphInitWithDeepCopy(...)                                                                                     \
    CONCAT(GraphInitWithDeepCopy_, GRAPH_INIT_WITH_DEEP_COPY_HAS_ARGS(__VA_ARGS__))(__VA_ARGS__)
#define GraphInitWithDeepCopy_2(ci, cd)        GRAPH_INIT_WITH_DEEP_COPY_VALUE((ci), (cd), DefaultAllocator())
#define GraphInitWithDeepCopy_3(ci, cd, alloc) GRAPH_INIT_WITH_DEEP_COPY_VALUE((ci), (cd), (alloc))

///
/// Initialize given graph with deep-copy callbacks for node payloads.
///
/// g[in]     : Variable or type of a graph to be initialized.
/// ci[in]    : Optional deep-copy callback for nodes.
/// cd[in]    : Optional deinit callback for nodes.
/// alloc[in] : Optional allocator copied into the graph. If omitted, DefaultAllocator() is used.
///
/// TAGS: Graph, Init, DeepCopy, Directed
///
#define GRAPH_INIT_WITH_DEEP_COPY_T_HAS_ARGS_IMPL(_1, _2, _3, _4, count, ...) count
#define GRAPH_INIT_WITH_DEEP_COPY_T_HAS_ARGS(...)                             GRAPH_INIT_WITH_DEEP_COPY_T_HAS_ARGS_IMPL(__VA_ARGS__, 4, 3, 2, 1, 0)

#ifdef __cplusplus
#    define GraphInitWithDeepCopyT(...)                                                                                \
        CONCAT(GraphInitWithDeepCopyT_, GRAPH_INIT_WITH_DEEP_COPY_T_HAS_ARGS(__VA_ARGS__))(__VA_ARGS__)
#    define GraphInitWithDeepCopyT_3(g, ci, cd)        (TYPE_OF(g) GraphInitWithDeepCopy((ci), (cd)))
#    define GraphInitWithDeepCopyT_4(g, ci, cd, alloc) (TYPE_OF(g) GraphInitWithDeepCopy((ci), (cd), (alloc)))
#else
#    define GraphInitWithDeepCopyT(...)                                                                                \
        CONCAT(GraphInitWithDeepCopyT_, GRAPH_INIT_WITH_DEEP_COPY_T_HAS_ARGS(__VA_ARGS__))(__VA_ARGS__)
#    define GraphInitWithDeepCopyT_3(g, ci, cd)        ((TYPE_OF(g))GraphInitWithDeepCopy((ci), (cd)))
#    define GraphInitWithDeepCopyT_4(g, ci, cd, alloc) ((TYPE_OF(g))GraphInitWithDeepCopy((ci), (cd), (alloc)))
#endif

#define GRAPH_INIT_WITH_DEEP_COPY_VALUE(ci, cd, alloc)                                                                 \
    {.slots                 = VecInit((alloc)),                                                                        \
     .free_indices          = VecInit((alloc)),                                                                        \
     .pending_edge_removals = VecInit((alloc)),                                                                        \
     .copy_init             = (GenericCopyInit)(ci),                                                                   \
     .copy_deinit           = (GenericCopyDeinit)(cd),                                                                 \
     .live_count            = 0,                                                                                       \
     .edge_count            = 0,                                                                                       \
     .pending_delete_count  = 0,                                                                                       \
     .mutation_epoch        = 0,                                                                                       \
     .allocator             = AllocatorBind((alloc)),                                                                  \
     .__magic               = MISRA_GRAPH_MAGIC}

///
/// Deinit graph by freeing all allocations and tearing down all node payloads.
///
/// g[in,out] : Pointer to `Graph` to be deinited.
///
/// TAGS: Graph, Deinit, Cleanup
///
#define GraphDeinit(g) deinit_graph(GENERIC_GRAPH(g), sizeof(GRAPH_NODE_TYPE(g)))

#endif // MISRA_STD_CONTAINER_GRAPH_INIT_H
