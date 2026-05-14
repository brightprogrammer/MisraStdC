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
#    define GRAPH_TYPECHECK_NODE_L(g, node)                                                                            \
        ((void)sizeof(char[_Generic(&(node), GRAPH_NODE_TYPE(g) *: 1, default: -1)]))
#    define GRAPH_TYPECHECK_NODE_R(g, node) ((void)sizeof((GRAPH_NODE_TYPE(g)[]) {(node)}))
#else
#    define GRAPH_TYPECHECK_NODE_L(g, node) ((void)0)
#    define GRAPH_TYPECHECK_NODE_R(g, node) ((void)0)
#endif

///
/// Add a new node to the graph, taking ownership of `lval`.
/// L-value form: when the graph has no `copy_init` handler, the node payload
/// is moved out of `lval` (source zeroed) on success. With a deep-copy handler
/// the source is left untouched.
///
/// g[in,out] : Graph handle.
/// lval[in]  : Addressable node payload to insert.
///
/// SUCCESS : Returns the new node's stable `GraphNodeId` (non-zero).
/// FAILURE : Returns `0` on allocation failure. The graph and `lval` are
///           unchanged.
///
/// USAGE:
///   GraphNodeId id = GraphAddNodeL(&g, payload);
///   if (!id) { /* recover */ }
///
/// TAGS: Graph, AddNode, LValue, Ownership
///
#define GraphAddNodeL(g, lval)                                                                                         \
    (ValidateGraph(g),                                                                                                 \
     GRAPH_TYPECHECK_NODE_L((g), (lval)),                                                                              \
     graph_push_node_owned(GENERIC_GRAPH(g), &(lval), sizeof(GRAPH_NODE_TYPE(g))))

///
/// Add a new node to the graph from an r-value expression. The source is
/// treated as a temporary value; nothing is zeroed on the caller side.
///
/// g[in,out] : Graph handle.
/// rval[in]  : Node payload expression.
///
/// SUCCESS : Returns the new node's stable `GraphNodeId` (non-zero).
/// FAILURE : Returns `0` on allocation failure.
///
/// USAGE:
///   GraphNodeId id = GraphAddNodeR(&g, StrZ("Alpha"));
///
/// TAGS: Graph, AddNode, RValue
///
#define GraphAddNodeR(g, rval)                                                                                         \
    (ValidateGraph(g),                                                                                                 \
     GRAPH_TYPECHECK_NODE_R((g), (rval)),                                                                              \
     graph_push_node(GENERIC_GRAPH(g), &LVAL((GRAPH_NODE_TYPE(g))(rval)), sizeof(GRAPH_NODE_TYPE(g))))

///
/// Default node-add alias for `GraphAddNodeL`.
///
#define GraphAddNode(g, lval) GraphAddNodeL((g), (lval))

///
/// Add a directed edge from one node to another. Both endpoints must be
/// existing node ids in the same graph.
///
/// g[in,out] : Graph handle.
/// from[in]  : Source `GraphNodeId`.
/// to[in]    : Destination `GraphNodeId`.
///
/// SUCCESS : `true`.
/// FAILURE : `false` on allocation failure for the edge entry.
///
/// TAGS: Graph, AddEdge, Directed
///
#define GraphAddEdge(g, from, to) (ValidateGraph(g), graph_add_edge(GENERIC_GRAPH(g), (from), (to)))

///
/// Aborting (`Must*`) variants of the fallible insertion macros above.
///
/// Each `GraphMustXxx(...)` is the statement-style do-while wrapper around
/// the matching `GraphXxx(...)` expression: it calls the underlying fallible
/// form and triggers `LOG_FATAL(...)` if the call fails. Unlike the plain
/// `Add` forms, the `Must` forms do not yield a node id - they are statements,
/// not expressions. Use these at API boundaries where allocation failure is
/// not recoverable for the caller. Otherwise prefer the propagating forms.
///
/// SUCCESS : Returns to the caller.
/// FAILURE : Does not return - aborts via `LOG_FATAL` / `SysAbort`.
///
/// TAGS: Graph, Add, Must, Abort
///
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
