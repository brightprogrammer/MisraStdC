/// file      : std/container/graph/init.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Initializers for Graph.

#ifndef MISRA_STD_CONTAINER_GRAPH_INIT_H
#define MISRA_STD_CONTAINER_GRAPH_INIT_H

#include "Private.h"
#include "Type.h"

#define GraphInit(typed_alloc_ptr)    GraphInitWithDeepCopy(NULL, NULL, typed_alloc_ptr)
#define GraphInitT(g, typed_alloc_ptr) GraphInitWithDeepCopyT((g), NULL, NULL, typed_alloc_ptr)

#define GraphInitWithDeepCopy(ci, cd, typed_alloc_ptr) GRAPH_INIT_WITH_DEEP_COPY_VALUE((ci), (cd), typed_alloc_ptr)

#ifdef __cplusplus
#    define GraphInitWithDeepCopyT(g, ci, cd, typed_alloc_ptr)                                                         \
        (TYPE_OF(g) GraphInitWithDeepCopy((ci), (cd), typed_alloc_ptr))
#else
#    define GraphInitWithDeepCopyT(g, ci, cd, typed_alloc_ptr)                                                         \
        ((TYPE_OF(g))GraphInitWithDeepCopy((ci), (cd), typed_alloc_ptr))
#endif

#define GRAPH_INIT_WITH_DEEP_COPY_VALUE(ci, cd, typed_alloc_ptr)                                                       \
    {.slots                 = VecInit(typed_alloc_ptr),                                                                \
     .free_indices          = VecInit(typed_alloc_ptr),                                                                \
     .pending_edge_removals = VecInit(typed_alloc_ptr),                                                                \
     .copy_init             = (GenericCopyInit)(ci),                                                                   \
     .copy_deinit           = (GenericCopyDeinit)(cd),                                                                 \
     .live_count            = 0,                                                                                       \
     .edge_count            = 0,                                                                                       \
     .pending_delete_count  = 0,                                                                                       \
     .mutation_epoch        = 0,                                                                                       \
     .allocator             = ALLOCATOR_OF(typed_alloc_ptr),                                                           \
     .__magic               = MISRA_GRAPH_MAGIC}

#define GraphDeinit(g) deinit_graph(GENERIC_GRAPH(g), sizeof(GRAPH_NODE_TYPE(g)))

#endif // MISRA_STD_CONTAINER_GRAPH_INIT_H
