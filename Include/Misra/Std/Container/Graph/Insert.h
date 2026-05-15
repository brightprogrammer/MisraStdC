/// file      : std/container/graph/insert.h
/// author    : Generated following Misra project patterns
/// This is free and unencumbered software released into the public domain.
///
/// Insert helpers for Graph.

#ifndef MISRA_STD_CONTAINER_GRAPH_INSERT_H
#define MISRA_STD_CONTAINER_GRAPH_INSERT_H

#include "Type.h"
#include "Private.h"


///
/// Add a new node to the graph, taking ownership of `lval`.
/// L-value form: when the graph has no `copy_init` handler, the node payload
/// is moved out of `lval` (source zeroed) on success. With a deep-copy handler
/// the source is left untouched.
///
/// g[in,out] : Graph handle.
/// lval[in]  : Addressable node payload to insert.
///
/// SUCCESS : Returns the new node's stable `GraphNodeId` (non-zero). A new
///           slot has been allocated (or a freed slot reused with a bumped
///           generation); `live_count` grows by one. The node's outgoing
///           and incoming adjacency lists are empty. When the graph has
///           no `copy_init` handler, `lval` has been zeroed (payload
///           ownership transferred into the slot); otherwise `lval` is
///           unchanged.
/// FAILURE : Returns `0` on allocation failure (payload buffer or slot
///           growth). The graph and `lval` are unchanged.
///
/// USAGE:
///   GraphNodeId id = GraphAddNodeL(&g, payload);
///   if (!id) { /* recover */ }
///
/// TAGS: Graph, AddNode, LValue, Ownership
///
#define GraphAddNodeL(g, lval)                                                                                         \
    (ValidateGraph(g),                                                                                                 \
     CHECK_TYPE_EQUIVALENCE(TYPE_OF(lval), GRAPH_NODE_TYPE(g)),                                                        \
     graph_push_node_owned(GENERIC_GRAPH(g), &(lval), sizeof(GRAPH_NODE_TYPE(g))))

///
/// Add a new node to the graph from an r-value expression. The source is
/// treated as a temporary value; nothing is zeroed on the caller side.
///
/// g[in,out] : Graph handle.
/// rval[in]  : Node payload expression.
///
/// SUCCESS : Returns the new node's stable `GraphNodeId` (non-zero). A new
///           slot has been allocated (or a freed slot reused with a bumped
///           generation); `live_count` grows by one. The new node's
///           adjacency lists are empty. The source expression is untouched.
/// FAILURE : Returns `0` on allocation failure. The graph is unchanged.
///
/// USAGE:
///   GraphNodeId id = GraphAddNodeR(&g, StrZ("Alpha"));
///
/// TAGS: Graph, AddNode, RValue
///
#define GraphAddNodeR(g, rval)                                                                                         \
    (ValidateGraph(g),                                                                                                 \
     CHECK_TYPE_CONVERTIBLE(GRAPH_NODE_TYPE(g), rval),                                                                 \
     graph_push_node(GENERIC_GRAPH(g), &LVAL_AS(GRAPH_NODE_TYPE(g), rval), sizeof(GRAPH_NODE_TYPE(g))))

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
/// SUCCESS : Returns `true`. The edge entry has been appended to the
///           outgoing-neighbour list of `from` and to the reverse
///           predecessor list of `to`; `edge_count` grows by one.
/// FAILURE : Returns `false` on allocation failure for either side of the
///           adjacency entry. The graph is unchanged. A reference to a
///           non-live `from` or `to` node id is a caller bug and aborts
///           via `LOG_FATAL`.
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
/// FAILURE : Does not return - aborts via `LOG_FATAL` / `Abort`.
///
/// TAGS: Graph, Add, Must, Abort
///
#define GraphMustAddNodeL(g, lval)                                                                                     \
    do {                                                                                                               \
        if (!GraphAddNodeL((g), (lval))) {                                                                             \
            LOG_FATAL("GraphMustAddNodeL failed");                                                                     \
        }                                                                                                              \
    } while (0)

///
/// Aborting variant of `GraphAddNodeR`. See that macro for parameter
/// semantics and success-state effects.
///
/// SUCCESS : Returns to the caller. The underlying `GraphAddNodeR` call
///           succeeded; see `GraphAddNodeR` for the post-state.
/// FAILURE : Does not return - aborts via `LOG_FATAL` / `Abort` when
///           the underlying `GraphAddNodeR` call returns `false`.
///
/// TAGS: Graph, Must, Abort
///
#define GraphMustAddNodeR(g, rval)                                                                                     \
    do {                                                                                                               \
        if (!GraphAddNodeR((g), (rval))) {                                                                             \
            LOG_FATAL("GraphMustAddNodeR failed");                                                                     \
        }                                                                                                              \
    } while (0)

///
/// Aborting variant of `GraphAddNode`. See that macro for parameter
/// semantics and success-state effects.
///
/// SUCCESS : Returns to the caller. The underlying `GraphAddNode` call
///           succeeded; see `GraphAddNode` for the post-state.
/// FAILURE : Does not return - aborts via `LOG_FATAL` / `Abort` when
///           the underlying `GraphAddNode` call returns `false`.
///
/// TAGS: Graph, Must, Abort
///
#define GraphMustAddNode(g, lval) GraphMustAddNodeL((g), (lval))

///
/// Aborting variant of `GraphAddEdge`. See that macro for parameter
/// semantics and success-state effects.
///
/// SUCCESS : Returns to the caller. The underlying `GraphAddEdge` call
///           succeeded; see `GraphAddEdge` for the post-state.
/// FAILURE : Does not return - aborts via `LOG_FATAL` / `Abort` when
///           the underlying `GraphAddEdge` call returns `false`.
///
/// TAGS: Graph, Must, Abort
///
#define GraphMustAddEdge(g, from, to)                                                                                  \
    do {                                                                                                               \
        if (!GraphAddEdge((g), (from), (to))) {                                                                        \
            LOG_FATAL("GraphMustAddEdge failed");                                                                      \
        }                                                                                                              \
    } while (0)

#endif // MISRA_STD_CONTAINER_GRAPH_INSERT_H
