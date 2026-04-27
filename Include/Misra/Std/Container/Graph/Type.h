/// file      : std/container/graph/type.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Generic directed graph type definition

#ifndef MISRA_STD_CONTAINER_GRAPH_TYPE_H
#define MISRA_STD_CONTAINER_GRAPH_TYPE_H

#include <Misra/Std/Container/Vec.h>
#include <Misra/Types.h>

///
/// Stable identifier used to refer to a graph node.
///
/// Node ids are opaque 64-bit values. Internally they pack a per-slot generation
/// in the high 32 bits and a slot index in the low 32 bits.
///
/// TAGS: Graph, Node, Id
///
typedef u64 GraphNodeId;

///
/// Extract the slot index encoded in a node id.
///
/// id[in] : Graph node id.
///
/// SUCCESS: Low 32-bit slot index encoded in `id`.
/// FAILURE: Function cannot fail.
///
/// TAGS: Graph, Node, Id, Index
///
#define GraphNodeIdIndex(id) ((u32)((GraphNodeId)(id) & UINT32_MAX))

///
/// Extract the generation encoded in a node id.
///
/// id[in] : Graph node id.
///
/// SUCCESS: High 32-bit generation encoded in `id`.
/// FAILURE: Function cannot fail.
///
/// TAGS: Graph, Node, Id, Generation
///
#define GraphNodeIdGeneration(id) ((u32)(((GraphNodeId)(id) >> 32) & UINT32_MAX))

///
/// Opaque node handle used by traversal helpers.
///
/// This carries the owning graph pointer plus the stable node id. Users typically
/// obtain it from `GraphGetNode`, `GraphForeachNode`, or `GraphNodeForeachNeighbor`.
///
/// TAGS: Graph, Node, Handle
///
typedef struct {
    void       *__graph;
    GraphNodeId __id;
} GraphNode;

///
/// Outgoing neighbor list for a single graph node.
///
/// Each item is a `GraphNodeId` of a node reachable by one directed edge.
///
/// TAGS: Graph, Edge, Neighbor, Vec
///
typedef Vec(GraphNodeId) GraphNeighbors;

typedef struct {
    GraphNeighbors out_neighbors;
    void          *data;
    u64            visit_count;
    u32            generation;
    u32            flags;
} GenericGraphSlot;

typedef Vec(GenericGraphSlot) GraphSlots;
typedef Vec(u32)              GraphFreeIndices;

typedef struct {
    GraphSlots         slots;
    GraphFreeIndices   free_indices;
    GenericCopyInit    copy_init;
    GenericCopyDeinit  copy_deinit;
    u64                live_count;
    u64                edge_count;
    u64                pending_delete_count;
    u64                mutation_epoch;
    u64                alignment;
    void              *type_anchor;
    u64                __magic;
} GenericGraph;

#define GENERIC_GRAPH(g) ((GenericGraph *)(void *)(g))

///
/// Typesafe directed graph definition.
///
/// Node payloads are owned by the graph. Each live node occupies one internal slot
/// and is referred to by a stable generation/index `GraphNodeId`. Edges are directed
/// and stored as outgoing adjacency lists of those ids.
///
/// NOTE: Like the other generic containers in this project, each `Graph(T)`
///       expansion creates a distinct anonymous type. Prefer a `typedef`
///       when you need to reuse the same graph type across APIs.
///
/// USAGE:
///   typedef Graph(int) IntGraph;
///   IntGraph graph = GraphInit();
///
/// FIELDS:
/// - slots                : Internal slot storage for live and reusable nodes.
/// - free_indices         : Reusable slot indices populated by deletion/clear.
/// - copy_init            : Optional deep-copy callback for node payloads.
/// - copy_deinit          : Optional deinit callback for node payloads.
/// - live_count           : Number of currently live nodes.
/// - edge_count           : Number of directed edges currently stored.
/// - pending_delete_count : Number of nodes marked for deletion but not yet committed.
/// - mutation_epoch       : Structural mutation counter used by traversal helpers.
/// - alignment            : Alignment used for graph-owned node payload allocations.
/// - type_anchor          : Type anchor for generic node payload macros.
///
/// TAGS: Graph, Generic, Directed, Slot, Handle
///
#define Graph(T)                                                                                                       \
    struct {                                                                                                           \
        GraphSlots         slots;                                                                                      \
        GraphFreeIndices   free_indices;                                                                               \
        GenericCopyInit    copy_init;                                                                                  \
        GenericCopyDeinit  copy_deinit;                                                                                \
        u64                live_count;                                                                                 \
        u64                edge_count;                                                                                 \
        u64                pending_delete_count;                                                                       \
        u64                mutation_epoch;                                                                             \
        u64                alignment;                                                                                  \
        T                 *type_anchor;                                                                                \
        u64                __magic;                                                                                    \
    }

#define GRAPH_NODE_TYPE(g) TYPE_OF((g)->type_anchor[0])

#define MISRA_GRAPH_MAGIC MISRA_MAKE_NEW_MAGIC_VALUE("digrph01")

///
/// Validate whether a given `Graph` object is valid.
/// Aborts if provided graph is uninitialized or corrupted.
///
/// g[in] : Pointer to `Graph` object to validate.
///
/// SUCCESS: Continue execution, meaning given graph is most probably valid.
/// FAILURE: `abort`
///
#define ValidateGraph(g) validate_graph((const GenericGraph *)GENERIC_GRAPH(g))

#endif // MISRA_STD_CONTAINER_GRAPH_TYPE_H
