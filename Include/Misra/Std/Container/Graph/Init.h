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
#define GraphInit() GraphInitAlignedWithDeepCopy(NULL, NULL, 1)

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
#define GraphInitWithDeepCopy(ci, cd) GraphInitAlignedWithDeepCopy((ci), (cd), 1)

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
/// aln[in] : Alignment used for stored node payloads.
///
/// TAGS: Graph, Init, Alignment, Directed
///
#define GraphInitAligned(aln) GraphInitAlignedWithDeepCopy(NULL, NULL, (aln))

///
/// Initialize given graph with explicit node alignment.
///
/// g[in]   : Variable or type of a graph to be initialized.
/// aln[in] : Alignment used for stored node payloads.
///
/// TAGS: Graph, Init, Alignment, Directed
///
#define GraphInitAlignedT(g, aln) GraphInitAlignedWithDeepCopyT((g), NULL, NULL, (aln))

///
/// Initialize graph with deep-copy callbacks and explicit node alignment.
///
/// ci[in]  : Optional deep-copy callback for nodes.
/// cd[in]  : Optional deinit callback for nodes.
/// aln[in] : Alignment used for stored node payloads.
///
/// TAGS: Graph, Init, DeepCopy, Alignment, Directed
///
#define GraphInitAlignedWithDeepCopy(ci, cd, aln)                                                                     \
    {.nodes         = VecInitAlignedWithDeepCopy((ci), (cd), (aln)),                                                  \
     .out_neighbors = VecInitWithDeepCopy(graph_neighbors_init_copy, graph_neighbors_deinit),                         \
     .edge_count    = 0,                                                                                              \
     .__magic       = MISRA_GRAPH_MAGIC}

#ifdef __cplusplus
#    define GraphInitAlignedWithDeepCopyT(g, ci, cd, aln) (TYPE_OF(g) GraphInitAlignedWithDeepCopy((ci), (cd), (aln)))
#else
///
/// Initialize given graph with deep-copy callbacks and explicit node alignment.
///
/// g[in]   : Variable or type of a graph to be initialized.
/// ci[in]  : Optional deep-copy callback for nodes.
/// cd[in]  : Optional deinit callback for nodes.
/// aln[in] : Alignment used for stored node payloads.
///
/// TAGS: Graph, Init, DeepCopy, Alignment, Directed
///
#    define GraphInitAlignedWithDeepCopyT(g, ci, cd, aln) ((TYPE_OF(g))GraphInitAlignedWithDeepCopy((ci), (cd), (aln)))
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
