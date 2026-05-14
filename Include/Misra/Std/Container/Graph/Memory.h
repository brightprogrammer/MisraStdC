/// file      : std/container/graph/memory.h
/// author    : Generated following Misra project patterns
/// This is free and unencumbered software released into the public domain.
///
/// Memory management helpers for Graph.

#ifndef MISRA_STD_CONTAINER_GRAPH_MEMORY_H
#define MISRA_STD_CONTAINER_GRAPH_MEMORY_H

#include "Type.h"
#include "Private.h"

///
/// Remove all nodes and edges from the graph but keep allocated storage.
/// Node payloads are deinitialized via the configured `copy_deinit` handler
/// when present.
///
/// g[in,out] : Graph handle.
///
/// TAGS: Graph, Memory, Clear
///
#define GraphClear(g) clear_graph(GENERIC_GRAPH(g), sizeof(GRAPH_NODE_TYPE(g)))

///
/// Reserve space for at least `n` nodes in the graph.
/// Does not change the node count, only ensures capacity.
///
/// g[in,out] : Graph handle.
/// n[in]     : Minimum number of nodes the graph should accommodate.
///
/// SUCCESS : Returns `true`.
/// FAILURE : Returns `false` on allocation failure. The graph is unchanged.
///
/// TAGS: Graph, Memory, Reserve
///
#define GraphReserve(g, n) (ValidateGraph(g), reserve_graph(GENERIC_GRAPH(g), sizeof(GRAPH_NODE_TYPE(g)), (n)))

///
/// Aborting variant of `GraphReserve`. Calls `LOG_FATAL` on allocation failure.
///
/// SUCCESS : Returns to the caller.
/// FAILURE : Does not return - aborts via `LOG_FATAL` / `SysAbort`.
///
/// TAGS: Graph, Memory, Reserve, Must, Abort
///
#define GraphMustReserve(g, n)                                                                                         \
    do {                                                                                                               \
        if (!GraphReserve((g), (n))) {                                                                                 \
            LOG_FATAL("GraphMustReserve failed");                                                                      \
        }                                                                                                              \
    } while (0)

#endif // MISRA_STD_CONTAINER_GRAPH_MEMORY_H
