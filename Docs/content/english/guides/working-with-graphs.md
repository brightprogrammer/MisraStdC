---
title: "Working With Graphs"
date: 2026-04-27
description: "Build, walk, and modify a directed graph with Graph(T)."
authors:
  - siddharth-mishra
tags:
  - graph
  - containers
  - traversal
---

> **Note**: This post was drafted by an AI assistant under direction from the author. It is not first-hand writing; the design choices it describes are real, the prose explaining them is generated. Treat the technical content as the design talking, and the framing as a translation layer.

`Graph(T)` stores a directed graph with payloads of type `T` on each node. Use it for reachability checks, control-flow graphs, dependency graphs, and any pass that walks a topology before mutating it.

Each node has a stable id while it's alive, edges are directed, and reverse walks (going backwards along edges) are as cheap as forward walks.

## A small graph

```c
typedef Graph(Str) CityGraph;

Scope(lt, DefaultAllocator) {
    CityGraph g = GraphInitWithDeepCopy(NULL, StrDeinit);

    GraphNodeId a = GraphAddNodeR(&g, StrZ("alice"));
    GraphNodeId b = GraphAddNodeR(&g, StrZ("bob"));
    GraphNodeId c = GraphAddNodeR(&g, StrZ("carol"));

    GraphAddEdge(&g, a, b);
    GraphAddEdge(&g, b, c);

    GraphForeachNode(&g, node) {
        WriteFmtLn("node {} = {}", GraphNodeGetId(node), GraphNodeData(&g, node));
    }

    GraphDeinit(&g);
}
```

## Ids vs handles

- `GraphNodeId` is a stable id. Use it to remember a node across calls (in a map, in another data structure, in an algorithm's worklist).
- `GraphNode` is a handle you get from a traversal macro. Use it inside the loop body to read the payload, walk neighbors, or update the scratch visit count.

```c
GraphNode node = GraphGetNode(&g, some_id);
Str     *name  = GraphNodeData(&g, node);
```

## Walking neighbors

Forward and reverse walks have parallel APIs:

```c
GraphNodeForeachNeighbor(node, succ) {
    // edges out of node
}

GraphNodeForeachPredecessor(node, pred) {
    // edges into node
}
```

## Reachability with the built-in visit counter

Every node carries a small counter you can use to mark whether you've already seen it on the current walk. It's there to make simple traversals quick to write:

```c
static bool reaches(GraphNode node, GraphNodeId goal) {
    if (GraphNodeVisitCount(node) > 0) return false;
    GraphNodeVisit(node);

    if (GraphNodeGetId(node) == goal) return true;

    GraphNodeForeachNeighbor(node, next) {
        if (reaches(next, goal)) return true;
    }
    return false;
}
```

Reset before reusing:

```c
GraphForeachNode(&g, n) GraphNodeUnvisit(n);
```

The counter is one piece of shared state on the graph, so only one traversal at a time can use it. For nested or parallel walks, keep your own side table (`Map(GraphNodeId, u32)` or a flat `Vec(u32)` keyed by node index).

## Removing nodes and edges

Removal is two-step. Mark inside the traversal; commit after:

```c
GraphForeachNode(&g, n) {
    if (should_drop(n)) GraphMarkNodeForDeletion(n);
}
GraphCommitChanges(&g);
```

Available marks: `GraphMarkNodeForDeletion`, `GraphUnmarkNodeForDeletion`, `GraphMarkEdgeForRemoval`, `GraphUnmarkEdgeForRemoval`. `GraphCommitChanges` applies them all at once.

Inside a `GraphForeachNode` body, you can mark, unmark, and read/write payloads. Adding nodes, adding edges, or calling `GraphCommitChanges` mid-traversal will abort.

## Pitfalls

- **Stale ids.** After `GraphCommitChanges`, ids of removed nodes are no longer valid. Don't pass them to `GraphNodeAt`, `GraphOutDegree`, etc. Use `GraphContainsNode(&g, id)` to probe an id you're not sure about.
- **Slot indices are not identities.** Internally the graph keeps nodes in a flat array; `GraphNodeIndex(node)` is the position in that array. When a node is removed and the slot is later reused for a new node, the index repeats. The full `GraphNodeId` also carries a *generation* number that goes up every time a slot is reused, so two `GraphNodeId` values for the same slot at different times are not equal. If you key a `Vec` by slot index, reset the entries you care about after any commit that may have reused slots, or just key on `GraphNodeId` instead.
