/// file      : std/container/graph/insert.h
/// author    : Generated following Misra project patterns
/// This is free and unencumbered software released into the public domain.
///
/// Insert helpers for Graph.

#ifndef MISRA_STD_CONTAINER_GRAPH_INSERT_H
#define MISRA_STD_CONTAINER_GRAPH_INSERT_H

#include "Type.h"
#include "Private.h"

#include <stdio.h>

void SysAbort(void);

#if defined(MISRA_ENFORCE_TYPE_SAFETY) && MISRA_ENFORCE_TYPE_SAFETY
#    define GRAPH_TYPECHECK_NODE_L(g, node) ((void)sizeof(char[_Generic(&(node), GRAPH_NODE_TYPE(g) * : 1, default : -1)]))
#    define GRAPH_TYPECHECK_NODE_R(g, node) ((void)sizeof((GRAPH_NODE_TYPE(g)[]){(node)}))
#else
#    define GRAPH_TYPECHECK_NODE_L(g, node) ((void)0)
#    define GRAPH_TYPECHECK_NODE_R(g, node) ((void)0)
#endif

#define GRAPH_ABORT(message) graph_abort_insert_operation(__func__, __LINE__, (message))

static inline void graph_abort_insert_operation(const char *function, int line, const char *message) {
    fprintf(stderr, "FATAL [%s:%d] %s\n", function, line, message);
    SysAbort();
}

static inline GraphNodeId graph_require_inserted_node(GraphNodeId node_id, const char *message) {
    if (!node_id) {
        GRAPH_ABORT(message);
    }

    return node_id;
}

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

#define GraphMustAddNodeL(g, lval) graph_require_inserted_node(GraphAddNodeL((g), (lval)), "GraphMustAddNodeL failed")

#define GraphMustAddNodeR(g, rval) graph_require_inserted_node(GraphAddNodeR((g), (rval)), "GraphMustAddNodeR failed")

#define GraphMustAddNode(g, lval) GraphMustAddNodeL((g), (lval))

#define GraphMustAddEdge(g, from, to)                                                                                  \
    do {                                                                                                               \
        if (!GraphAddEdge((g), (from), (to))) {                                                                        \
            GRAPH_ABORT("GraphMustAddEdge failed");                                                                    \
        }                                                                                                              \
    } while (0)

#endif // MISRA_STD_CONTAINER_GRAPH_INSERT_H
