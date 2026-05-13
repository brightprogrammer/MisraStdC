/// file      : std/container/graph/init.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Initializers for Graph.

#ifndef MISRA_STD_CONTAINER_GRAPH_INIT_H
#define MISRA_STD_CONTAINER_GRAPH_INIT_H

#include "Type.h"
#include "Private.h"

///
/// Initialize graph. Default node alignment is 1.
/// An allocator can be passed as the final optional argument. If omitted, DefaultAllocator() is used.
///
/// USAGE:
///   Graph(int) graph = GraphInit();
///   Graph(int) arena_graph = GraphInit(arena_allocator);
///
/// TAGS: Graph, Init, Directed
///
#define GRAPH_INIT_HAS_ARGS_IMPL(_0, _1, count, ...) count
#define GRAPH_INIT_HAS_ARGS(...) GRAPH_INIT_HAS_ARGS_IMPL(__VA_OPT__(,) __VA_ARGS__, 1, 0, 0)
#define GraphInit(...) CONCAT(GraphInit_, GRAPH_INIT_HAS_ARGS(__VA_ARGS__))(__VA_ARGS__)
#define GraphInit_0() GRAPH_INIT_ALIGNED_WITH_DEEP_COPY_VALUE(NULL, NULL, 1, DefaultAllocator())
#define GraphInit_1(alloc) GRAPH_INIT_ALIGNED_WITH_DEEP_COPY_VALUE(NULL, NULL, 1, (alloc))

///
/// Initialize given graph. Default node alignment is 1.
///
/// g[in]     : Variable or type of a graph to be initialized.
/// alloc[in] : Optional allocator copied into the graph. If omitted, DefaultAllocator() is used.
///
/// TAGS: Graph, Init, Directed
///
#define GRAPH_INIT_T_HAS_ARGS_IMPL(_1, _2, count, ...) count
#define GRAPH_INIT_T_HAS_ARGS(...) GRAPH_INIT_T_HAS_ARGS_IMPL(__VA_ARGS__, 2, 1, 0)
#define GraphInitT(...) CONCAT(GraphInitT_, GRAPH_INIT_T_HAS_ARGS(__VA_ARGS__))(__VA_ARGS__)
#define GraphInitT_1(g) GraphInitAlignedWithDeepCopyT((g), NULL, NULL, 1)
#define GraphInitT_2(g, alloc) GraphInitAlignedWithDeepCopyT((g), NULL, NULL, 1, (alloc))

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
#define GRAPH_INIT_WITH_DEEP_COPY_HAS_ARGS(...) GRAPH_INIT_WITH_DEEP_COPY_HAS_ARGS_IMPL(__VA_ARGS__, 3, 2, 1, 0)
#define GraphInitWithDeepCopy(...)                                                                                    \
    CONCAT(GraphInitWithDeepCopy_, GRAPH_INIT_WITH_DEEP_COPY_HAS_ARGS(__VA_ARGS__))(__VA_ARGS__)
#define GraphInitWithDeepCopy_2(ci, cd) GRAPH_INIT_ALIGNED_WITH_DEEP_COPY_VALUE((ci), (cd), 1, DefaultAllocator())
#define GraphInitWithDeepCopy_3(ci, cd, alloc) GRAPH_INIT_ALIGNED_WITH_DEEP_COPY_VALUE((ci), (cd), 1, (alloc))

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
#define GRAPH_INIT_WITH_DEEP_COPY_T_HAS_ARGS(...) GRAPH_INIT_WITH_DEEP_COPY_T_HAS_ARGS_IMPL(__VA_ARGS__, 4, 3, 2, 1, 0)
#define GraphInitWithDeepCopyT(...)                                                                                   \
    CONCAT(GraphInitWithDeepCopyT_, GRAPH_INIT_WITH_DEEP_COPY_T_HAS_ARGS(__VA_ARGS__))(__VA_ARGS__)
#define GraphInitWithDeepCopyT_3(g, ci, cd) GraphInitAlignedWithDeepCopyT((g), (ci), (cd), 1)
#define GraphInitWithDeepCopyT_4(g, ci, cd, alloc) GraphInitAlignedWithDeepCopyT((g), (ci), (cd), 1, (alloc))

///
/// Initialize graph with explicit node alignment.
///
/// aln[in]   : Alignment used for graph-owned node payload allocations.
/// alloc[in] : Optional allocator copied into the graph. If omitted, DefaultAllocator() is used.
///
/// TAGS: Graph, Init, Alignment, Directed
///
#define GRAPH_INIT_ALIGNED_HAS_ARGS_IMPL(_1, _2, count, ...) count
#define GRAPH_INIT_ALIGNED_HAS_ARGS(...) GRAPH_INIT_ALIGNED_HAS_ARGS_IMPL(__VA_ARGS__, 2, 1, 0)
#define GraphInitAligned(...) CONCAT(GraphInitAligned_, GRAPH_INIT_ALIGNED_HAS_ARGS(__VA_ARGS__))(__VA_ARGS__)
#define GraphInitAligned_1(aln) GRAPH_INIT_ALIGNED_WITH_DEEP_COPY_VALUE(NULL, NULL, (aln), DefaultAllocator())
#define GraphInitAligned_2(aln, alloc) GRAPH_INIT_ALIGNED_WITH_DEEP_COPY_VALUE(NULL, NULL, (aln), (alloc))

///
/// Initialize given graph with explicit node alignment.
///
/// g[in]     : Variable or type of a graph to be initialized.
/// aln[in]   : Alignment used for graph-owned node payload allocations.
/// alloc[in] : Optional allocator copied into the graph. If omitted, DefaultAllocator() is used.
///
/// TAGS: Graph, Init, Alignment, Directed
///
#define GRAPH_INIT_ALIGNED_T_HAS_ARGS_IMPL(_1, _2, _3, count, ...) count
#define GRAPH_INIT_ALIGNED_T_HAS_ARGS(...) GRAPH_INIT_ALIGNED_T_HAS_ARGS_IMPL(__VA_ARGS__, 3, 2, 1, 0)
#define GraphInitAlignedT(...) CONCAT(GraphInitAlignedT_, GRAPH_INIT_ALIGNED_T_HAS_ARGS(__VA_ARGS__))(__VA_ARGS__)
#define GraphInitAlignedT_2(g, aln) GraphInitAlignedWithDeepCopyT((g), NULL, NULL, (aln))
#define GraphInitAlignedT_3(g, aln, alloc) GraphInitAlignedWithDeepCopyT((g), NULL, NULL, (aln), (alloc))

///
/// Initialize graph with deep-copy callbacks and explicit node alignment.
///
/// ci[in]    : Optional deep-copy callback for nodes.
/// cd[in]    : Optional deinit callback for nodes.
/// aln[in]   : Alignment used for graph-owned node payload allocations.
/// alloc[in] : Optional allocator copied into the graph. If omitted, DefaultAllocator() is used.
///
/// TAGS: Graph, Init, DeepCopy, Alignment, Directed
///
#define GRAPH_INIT_ALIGNED_WITH_DEEP_COPY_HAS_ARGS_IMPL(_1, _2, _3, _4, count, ...) count
#define GRAPH_INIT_ALIGNED_WITH_DEEP_COPY_HAS_ARGS(...)                                                               \
    GRAPH_INIT_ALIGNED_WITH_DEEP_COPY_HAS_ARGS_IMPL(__VA_ARGS__, 4, 3, 2, 1, 0)
#define GraphInitAlignedWithDeepCopy(...)                                                                             \
    CONCAT(GraphInitAlignedWithDeepCopy_, GRAPH_INIT_ALIGNED_WITH_DEEP_COPY_HAS_ARGS(__VA_ARGS__))(__VA_ARGS__)
#define GraphInitAlignedWithDeepCopy_3(ci, cd, aln)                                                                   \
    GRAPH_INIT_ALIGNED_WITH_DEEP_COPY_VALUE((ci), (cd), (aln), DefaultAllocator())
#define GraphInitAlignedWithDeepCopy_4(ci, cd, aln, alloc)                                                            \
    GRAPH_INIT_ALIGNED_WITH_DEEP_COPY_VALUE((ci), (cd), (aln), (alloc))

#define GRAPH_INIT_ALIGNED_WITH_DEEP_COPY_VALUE(ci, cd, aln, alloc)                                                   \
    {.slots                = VecInit((alloc)),                                                                        \
     .free_indices         = VecInit((alloc)),                                                                        \
     .pending_edge_removals = VecInit((alloc)),                                                                       \
     .copy_init            = (GenericCopyInit)(ci),                                                                   \
     .copy_deinit          = (GenericCopyDeinit)(cd),                                                                 \
     .live_count           = 0,                                                                                       \
     .edge_count           = 0,                                                                                       \
     .pending_delete_count = 0,                                                                                       \
     .mutation_epoch       = 0,                                                                                       \
     .alignment            = (aln),                                                                                   \
     .allocator            = AllocatorBind((alloc)),                                                                  \
     .type_anchor          = NULL,                                                                                    \
     .__magic              = MISRA_GRAPH_MAGIC}

#define GRAPH_INIT_ALIGNED_WITH_DEEP_COPY_T_HAS_ARGS_IMPL(_1, _2, _3, _4, _5, count, ...) count
#define GRAPH_INIT_ALIGNED_WITH_DEEP_COPY_T_HAS_ARGS(...)                                                             \
    GRAPH_INIT_ALIGNED_WITH_DEEP_COPY_T_HAS_ARGS_IMPL(__VA_ARGS__, 5, 4, 3, 2, 1, 0)

#ifdef __cplusplus
#    define GraphInitAlignedWithDeepCopyT(...)                                                                         \
        CONCAT(GraphInitAlignedWithDeepCopyT_, GRAPH_INIT_ALIGNED_WITH_DEEP_COPY_T_HAS_ARGS(__VA_ARGS__))(__VA_ARGS__)
#    define GraphInitAlignedWithDeepCopyT_4(g, ci, cd, aln) (TYPE_OF(g) GraphInitAlignedWithDeepCopy((ci), (cd), (aln)))
#    define GraphInitAlignedWithDeepCopyT_5(g, ci, cd, aln, alloc)                                                     \
        (TYPE_OF(g) GraphInitAlignedWithDeepCopy((ci), (cd), (aln), (alloc)))
#else
///
/// Initialize given graph with deep-copy callbacks and explicit node alignment.
///
/// g[in]     : Variable or type of a graph to be initialized.
/// ci[in]    : Optional deep-copy callback for nodes.
/// cd[in]    : Optional deinit callback for nodes.
/// aln[in]   : Alignment used for graph-owned node payload allocations.
/// alloc[in] : Optional allocator copied into the graph. If omitted, DefaultAllocator() is used.
///
/// TAGS: Graph, Init, DeepCopy, Alignment, Directed
///
#    define GraphInitAlignedWithDeepCopyT(...)                                                                         \
        CONCAT(GraphInitAlignedWithDeepCopyT_, GRAPH_INIT_ALIGNED_WITH_DEEP_COPY_T_HAS_ARGS(__VA_ARGS__))(__VA_ARGS__)
#    define GraphInitAlignedWithDeepCopyT_4(g, ci, cd, aln) ((TYPE_OF(g))GraphInitAlignedWithDeepCopy((ci), (cd), (aln)))
#    define GraphInitAlignedWithDeepCopyT_5(g, ci, cd, aln, alloc)                                                     \
        ((TYPE_OF(g))GraphInitAlignedWithDeepCopy((ci), (cd), (aln), (alloc)))
#endif

///
/// Deinitialize graph and all owned node payloads and adjacency storage.
///
/// g[in,out] : Graph to deinitialize.
///
/// TAGS: Graph, Deinit, Memory, Directed
///
#define GraphDeinit(g) deinit_graph(GENERIC_GRAPH(g), sizeof(GRAPH_NODE_TYPE(g)))

#endif // MISRA_STD_CONTAINER_GRAPH_INIT_H
