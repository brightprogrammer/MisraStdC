/// file      : std/container/graph/memory.h
/// author    : Generated following Misra project patterns
/// This is free and unencumbered software released into the public domain.
///
/// Memory management helpers for Graph.

#ifndef MISRA_STD_CONTAINER_GRAPH_MEMORY_H
#define MISRA_STD_CONTAINER_GRAPH_MEMORY_H

#include "Type.h"
#include "Private.h"

#define GraphClear(g) clear_graph(GENERIC_GRAPH(g), sizeof(GRAPH_NODE_TYPE(g)))

#define GraphReserve(g, n) (ValidateGraph(g), reserve_graph(GENERIC_GRAPH(g), sizeof(GRAPH_NODE_TYPE(g)), (n)))

#define GraphMustReserve(g, n)                                                                                         \
    do {                                                                                                               \
        if (!GraphReserve((g), (n))) {                                                                                 \
            LOG_FATAL("GraphMustReserve failed");                                                                      \
        }                                                                                                              \
    } while (0)

#endif // MISRA_STD_CONTAINER_GRAPH_MEMORY_H
