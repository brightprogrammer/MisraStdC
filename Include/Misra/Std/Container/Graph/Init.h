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
/// Initialize an empty shallow-copy `Graph`. Inside a `Scope` block the allocator argument
/// may be omitted; otherwise pass a typed allocator handle.
///
/// SUCCESS : Returns a live, empty graph with NULL copy callbacks (bitwise-copy semantics).
/// FAILURE : Cannot fail at construction; first allocation failures surface from later inserts.
///
/// TAGS: Graph, Init, Construct
///
#define GraphInit(...)               OVERLOAD(GraphInit, __VA_ARGS__)
#define GraphInit_0()                GraphInitWithDeepCopy_3(NULL, NULL, MisraScope)
#define GraphInit_1(typed_alloc_ptr) GraphInitWithDeepCopy_3(NULL, NULL, typed_alloc_ptr)

///
/// Type-flavoured `GraphInit` for a typedef'd `Graph(T)`; the first argument names the
/// target graph type so the literal carries the correct compound-literal type.
///
/// SUCCESS : Returns a typed empty graph value, allocator-bound, callbacks NULL.
/// FAILURE : Cannot fail at construction; allocation failures surface from later inserts.
///
/// TAGS: Graph, Init, Typed, Construct
///
#define GraphInitT(g, ...)               OVERLOAD(GraphInitT, g, __VA_ARGS__)
#define GraphInitT_1(g)                  GraphInitWithDeepCopyT_4((g), NULL, NULL, MisraScope)
#define GraphInitT_2(g, typed_alloc_ptr) GraphInitWithDeepCopyT_4((g), NULL, NULL, typed_alloc_ptr)

///
/// Initialize an empty deep-copy `Graph`. `ci` is invoked on every push to clone the payload
/// into graph-owned storage; `cd` is invoked on remove/clear/deinit to release that storage.
///
/// ci[in]              : `GenericCopyInit` callback (may be NULL for bitwise copy).
/// cd[in]              : `GenericCopyDeinit` callback (may be NULL when `ci` is NULL).
/// typed_alloc_ptr[in] : Typed allocator handle, optional inside `Scope`.
///
/// SUCCESS : Returns a live, empty deep-copy graph wired to the supplied callbacks.
/// FAILURE : Cannot fail at construction; allocation failures surface from later inserts.
///
/// TAGS: Graph, Init, DeepCopy, Construct
///
#define GraphInitWithDeepCopy(...)                       OVERLOAD(GraphInitWithDeepCopy, __VA_ARGS__)
#define GraphInitWithDeepCopy_2(ci, cd)                  GRAPH_INIT_WITH_DEEP_COPY_VALUE((ci), (cd), MisraScope)
#define GraphInitWithDeepCopy_3(ci, cd, typed_alloc_ptr) GRAPH_INIT_WITH_DEEP_COPY_VALUE((ci), (cd), typed_alloc_ptr)

///
/// Type-flavoured `GraphInitWithDeepCopy` — same contract, but the first argument names the
/// target `Graph(T)` so the literal carries the typed compound-literal cast.
///
/// SUCCESS : Returns a typed deep-copy graph wired to the supplied callbacks.
/// FAILURE : Cannot fail at construction; allocation failures surface from later inserts.
///
/// TAGS: Graph, Init, DeepCopy, Typed, Construct
///
#define GraphInitWithDeepCopyT(g, ...) OVERLOAD(GraphInitWithDeepCopyT, g, __VA_ARGS__)
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

///
/// Release every node, every edge list, and the backing slot storage of `g`. If the graph
/// was created with a deep-copy deinit callback, that callback is invoked on each live node
/// payload before the slot is freed.
///
/// g[in,out] : Graph to deinitialize. Must not be used until reinitialized.
///
/// SUCCESS : All node payloads released, all edge bookkeeping freed; `g` left in the zeroed
///           post-deinit state.
/// FAILURE : Cannot fail; aborts on a corrupted magic via the validator.
///
/// TAGS: Graph, Deinit, Memory
///
#define GraphDeinit(g) deinit_graph(GENERIC_GRAPH(g), sizeof(GRAPH_NODE_TYPE(g)))

#endif // MISRA_STD_CONTAINER_GRAPH_INIT_H
