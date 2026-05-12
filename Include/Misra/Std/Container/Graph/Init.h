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
///
/// USAGE:
///   Graph(int) graph = GraphInit();
///
/// TAGS: Graph, Init, Directed
///
#define GRAPH_INIT_HAS_ARGS_IMPL(_0, _1, count, ...) count
#define GRAPH_INIT_HAS_ARGS(...) GRAPH_INIT_HAS_ARGS_IMPL(__VA_OPT__(,) __VA_ARGS__, 1, 0, 0)
#define GraphInit(...) CONCAT(GraphInit_, GRAPH_INIT_HAS_ARGS(__VA_ARGS__))(__VA_ARGS__)
#define GraphInit_0() GraphInitAlignedWithDeepCopyAndAlloc(NULL, NULL, 1, DefaultAllocator())
#define GraphInit_1(alloc) GraphInitAlignedWithDeepCopyAndAlloc(NULL, NULL, 1, (alloc))

///
/// Initialize given graph. Default node alignment is 1.
///
/// g[in] : Variable or type of a graph to be initialized.
///
/// TAGS: Graph, Init, Directed
///
#define GraphInitT(g) GraphInitAlignedWithDeepCopyT((g), NULL, NULL, 1)

///
/// Initialize graph with deep-copy callbacks for node payloads.
///
/// ci[in] : Optional deep-copy callback for nodes.
/// cd[in] : Optional deinit callback for nodes.
///
/// TAGS: Graph, Init, DeepCopy, Directed
///
#define GraphInitWithDeepCopy(ci, cd) GraphInitAlignedWithDeepCopyAndAlloc((ci), (cd), 1, DefaultAllocator())
#define GraphInitWithDeepCopyAlloc(ci, cd, alloc) GraphInitAlignedWithDeepCopyAndAlloc((ci), (cd), 1, (alloc))

///
/// Initialize given graph with deep-copy callbacks for node payloads.
///
/// g[in]  : Variable or type of a graph to be initialized.
/// ci[in] : Optional deep-copy callback for nodes.
/// cd[in] : Optional deinit callback for nodes.
///
/// TAGS: Graph, Init, DeepCopy, Directed
///
#define GraphInitWithDeepCopyT(g, ci, cd) GraphInitAlignedWithDeepCopyT((g), (ci), (cd), 1)

///
/// Initialize graph with explicit node alignment.
///
/// aln[in] : Alignment used for graph-owned node payload allocations.
///
/// TAGS: Graph, Init, Alignment, Directed
///
#define GraphInitAligned(aln) GraphInitAlignedWithDeepCopyAndAlloc(NULL, NULL, (aln), DefaultAllocator())
#define GraphInitAlignedAlloc(aln, alloc) GraphInitAlignedWithDeepCopyAndAlloc(NULL, NULL, (aln), (alloc))

///
/// Initialize given graph with explicit node alignment.
///
/// g[in]   : Variable or type of a graph to be initialized.
/// aln[in] : Alignment used for graph-owned node payload allocations.
///
/// TAGS: Graph, Init, Alignment, Directed
///
#define GraphInitAlignedT(g, aln) GraphInitAlignedWithDeepCopyT((g), NULL, NULL, (aln))

///
/// Initialize graph with deep-copy callbacks and explicit node alignment.
///
/// ci[in]  : Optional deep-copy callback for nodes.
/// cd[in]  : Optional deinit callback for nodes.
/// aln[in] : Alignment used for graph-owned node payload allocations.
///
/// TAGS: Graph, Init, DeepCopy, Alignment, Directed
///
#define GraphInitAlignedWithDeepCopy(ci, cd, aln)                                                                     \
    GraphInitAlignedWithDeepCopyAndAlloc((ci), (cd), (aln), DefaultAllocator())

#define GraphInitAlignedWithDeepCopyAndAlloc(ci, cd, aln, alloc)                                                      \
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

#ifdef __cplusplus
#    define GraphInitAlignedWithDeepCopyT(g, ci, cd, aln) (TYPE_OF(g) GraphInitAlignedWithDeepCopy((ci), (cd), (aln)))
#    define GraphInitAlignedWithDeepCopyAllocT(g, ci, cd, aln, alloc)                                                  \
        (TYPE_OF(g) GraphInitAlignedWithDeepCopyAndAlloc((ci), (cd), (aln), (alloc)))
#else
///
/// Initialize given graph with deep-copy callbacks and explicit node alignment.
///
/// g[in]   : Variable or type of a graph to be initialized.
/// ci[in]  : Optional deep-copy callback for nodes.
/// cd[in]  : Optional deinit callback for nodes.
/// aln[in] : Alignment used for graph-owned node payload allocations.
///
/// TAGS: Graph, Init, DeepCopy, Alignment, Directed
///
#    define GraphInitAlignedWithDeepCopyT(g, ci, cd, aln) ((TYPE_OF(g))GraphInitAlignedWithDeepCopy((ci), (cd), (aln)))
#    define GraphInitAlignedWithDeepCopyAllocT(g, ci, cd, aln, alloc)                                                  \
        ((TYPE_OF(g))GraphInitAlignedWithDeepCopyAndAlloc((ci), (cd), (aln), (alloc)))
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
