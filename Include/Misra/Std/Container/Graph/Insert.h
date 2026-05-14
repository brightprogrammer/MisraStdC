/// file      : std/container/graph/insert.h
/// author    : Generated following Misra project patterns
/// This is free and unencumbered software released into the public domain.
///
/// Insert helpers for Graph.

#ifndef MISRA_STD_CONTAINER_GRAPH_INSERT_H
#define MISRA_STD_CONTAINER_GRAPH_INSERT_H

#include "Type.h"
#include "Private.h"

#if defined(MISRA_ENFORCE_TYPE_SAFETY) && MISRA_ENFORCE_TYPE_SAFETY
#    define GRAPH_TYPECHECK_NODE_L(g, node) ((void)sizeof(char[_Generic(&(node), GRAPH_NODE_TYPE(g) * : 1, default : -1)]))
#    define GRAPH_TYPECHECK_NODE_R(g, node) ((void)sizeof((GRAPH_NODE_TYPE(g)[]){(node)}))
#else
#    define GRAPH_TYPECHECK_NODE_L(g, node) ((void)0)
#    define GRAPH_TYPECHECK_NODE_R(g, node) ((void)0)
#endif

#define GraphAddNodeL(g, lval)                                                                                         \
    (ValidateGraph(g),                                                                                                \
     GRAPH_TYPECHECK_NODE_L((g), (lval)),                                                                            \
     graph_push_node_owned(GENERIC_GRAPH(g), &(lval), sizeof(GRAPH_NODE_TYPE(g))))

#define GraphAddNodeR(g, rval)                                                                                         \
    (ValidateGraph(g),                                                                                                \
     GRAPH_TYPECHECK_NODE_R((g), (rval)),                                                                            \
     graph_push_node(GENERIC_GRAPH(g), &LVAL((GRAPH_NODE_TYPE(g))(rval)), sizeof(GRAPH_NODE_TYPE(g))))

#define GraphAddNode(g, lval) GraphAddNodeL((g), (lval))

#define GraphAddEdge(g, from, to) (ValidateGraph(g), graph_add_edge(GENERIC_GRAPH(g), (from), (to)))

#define GraphMustAddNodeL(g, lval)                                                                                     \
    do {                                                                                                               \
        if (!GraphAddNodeL((g), (lval))) {                                                                             \
            LOG_FATAL("GraphMustAddNodeL failed");                                                                     \
        }                                                                                                              \
    } while (0)

#define GraphMustAddNodeR(g, rval)                                                                                     \
    do {                                                                                                               \
        if (!GraphAddNodeR((g), (rval))) {                                                                             \
            LOG_FATAL("GraphMustAddNodeR failed");                                                                     \
        }                                                                                                              \
    } while (0)

#define GraphMustAddNode(g, lval) GraphMustAddNodeL((g), (lval))

#define GraphMustAddEdge(g, from, to)                                                                                  \
    do {                                                                                                               \
        if (!GraphAddEdge((g), (from), (to))) {                                                                        \
            LOG_FATAL("GraphMustAddEdge failed");                                                                      \
        }                                                                                                              \
    } while (0)

#endif // MISRA_STD_CONTAINER_GRAPH_INSERT_H
