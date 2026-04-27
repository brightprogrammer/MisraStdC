/// file      : std/container/graph/memory.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Memory management helpers for Graph.

#ifndef MISRA_STD_CONTAINER_GRAPH_MEMORY_H
#define MISRA_STD_CONTAINER_GRAPH_MEMORY_H

#include "Type.h"
#include "Private.h"

///
/// Clear all nodes and edges but retain allocated storage.
///
/// Existing live node handles become invalid after clear. Internal slot storage is
/// retained so new nodes can reuse it with fresh generations.
///
/// g[in,out] : Graph to clear.
///
/// TAGS: Graph, Clear, Memory
///
#define GraphClear(g) clear_graph(GENERIC_GRAPH(g), sizeof(GRAPH_NODE_TYPE(g)))

///
/// Reserve capacity for at least `n` slots.
///
/// g[in,out] : Graph to reserve storage for.
/// n[in]     : Minimum number of slots expected.
///
/// TAGS: Graph, Reserve, Memory
///
#define GraphReserve(g, n) reserve_graph(GENERIC_GRAPH(g), sizeof(GRAPH_NODE_TYPE(g)), (n))

#endif // MISRA_STD_CONTAINER_GRAPH_MEMORY_H
