/// file      : std/container/graph/init.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Initializers for Graph.

#ifndef MISRA_STD_CONTAINER_GRAPH_INIT_H
#define MISRA_STD_CONTAINER_GRAPH_INIT_H

#include "Private.h"
#include "Type.h"

#define GraphInit(...)               MISRA_OVERLOAD(GraphInit, __VA_ARGS__)
#define GraphInit_0()                GraphInitWithDeepCopy_3(NULL, NULL, MisraScope)
#define GraphInit_1(typed_alloc_ptr) GraphInitWithDeepCopy_3(NULL, NULL, typed_alloc_ptr)

#define GraphInitT(g, ...)               MISRA_OVERLOAD(GraphInitT, g, __VA_ARGS__)
#define GraphInitT_1(g)                  GraphInitWithDeepCopyT_4((g), NULL, NULL, MisraScope)
#define GraphInitT_2(g, typed_alloc_ptr) GraphInitWithDeepCopyT_4((g), NULL, NULL, typed_alloc_ptr)

#define GraphInitWithDeepCopy(...)                       MISRA_OVERLOAD(GraphInitWithDeepCopy, __VA_ARGS__)
#define GraphInitWithDeepCopy_2(ci, cd)                  GRAPH_INIT_WITH_DEEP_COPY_VALUE((ci), (cd), MisraScope)
#define GraphInitWithDeepCopy_3(ci, cd, typed_alloc_ptr) GRAPH_INIT_WITH_DEEP_COPY_VALUE((ci), (cd), typed_alloc_ptr)

#define GraphInitWithDeepCopyT(g, ...) MISRA_OVERLOAD(GraphInitWithDeepCopyT, g, __VA_ARGS__)
#ifdef __cplusplus
#    define GraphInitWithDeepCopyT_3(g, ci, cd) (TYPE_OF(g) GRAPH_INIT_WITH_DEEP_COPY_VALUE((ci), (cd), MisraScope))
#    define GraphInitWithDeepCopyT_4(g, ci, cd, typed_alloc_ptr)                                                       \
        (TYPE_OF(g) GRAPH_INIT_WITH_DEEP_COPY_VALUE((ci), (cd), typed_alloc_ptr))
#else
#    define GraphInitWithDeepCopyT_3(g, ci, cd) ((TYPE_OF(g))GRAPH_INIT_WITH_DEEP_COPY_VALUE((ci), (cd), MisraScope))
#    define GraphInitWithDeepCopyT_4(g, ci, cd, typed_alloc_ptr)                                                       \
        ((TYPE_OF(g))GRAPH_INIT_WITH_DEEP_COPY_VALUE((ci), (cd), typed_alloc_ptr))
#endif

#define GRAPH_INIT_WITH_DEEP_COPY_VALUE(ci, cd, typed_alloc_ptr)                                                       \
    {.slots                 = VecInit_1(typed_alloc_ptr),                                                              \
     .free_indices          = VecInit_1(typed_alloc_ptr),                                                              \
     .pending_edge_removals = VecInit_1(typed_alloc_ptr),                                                              \
     .copy_init             = (GenericCopyInit)(ci),                                                                   \
     .copy_deinit           = (GenericCopyDeinit)(cd),                                                                 \
     .live_count            = 0,                                                                                       \
     .edge_count            = 0,                                                                                       \
     .pending_delete_count  = 0,                                                                                       \
     .mutation_epoch        = 0,                                                                                       \
     .allocator             = ALLOCATOR_OF(typed_alloc_ptr),                                                           \
     .__magic               = GRAPH_MAGIC}

#define GraphDeinit(g) deinit_graph(GENERIC_GRAPH(g), sizeof(GRAPH_NODE_TYPE(g)))

#endif // MISRA_STD_CONTAINER_GRAPH_INIT_H
