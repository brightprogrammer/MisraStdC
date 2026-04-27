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
/// g[in,out] : Graph to clear.
///
/// TAGS: Graph, Clear, Memory
///
#define GraphClear(g) clear_graph(GENERIC_GRAPH(g), sizeof(GRAPH_NODE_TYPE(g)))

///
/// Reserve capacity for at least `n` nodes and their adjacency-list slots.
///
/// g[in,out] : Graph to reserve storage for.
/// n[in]     : Minimum number of nodes expected.
///
/// TAGS: Graph, Reserve, Memory
///
#define GraphReserve(g, n) reserve_graph(GENERIC_GRAPH(g), sizeof(GRAPH_NODE_TYPE(g)), (n))

#endif // MISRA_STD_CONTAINER_GRAPH_MEMORY_H
