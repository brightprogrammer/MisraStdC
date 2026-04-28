---
title: "Working With Graphs"
date: 2026-04-27
description: "How to use MisraStdC's generic directed graph for reachability, CFGs, dependency analysis, and graph rewrites."
authors:
  - siddharth-mishra
tags:
  - graph
  - containers
  - analysis
  - traversal
---

`Graph(T)` is the container you use when the problem is not "store a list of items" but "store a topology and run passes over it".

That usually means one of these:

- reachability analysis
- control-flow graphs
- dependency graphs
- graph rewrites that need stable traversal before destructive mutation

The graph container is generic, but it is not "object graph" generic in the OOP sense. It is closer to the graph style used by compilers, static analyzers, and systems libraries:

- nodes have stable ids while they are live
- node payloads are generic
- edges are directed
- traversal is explicit
- deletion is deferred through mark-then-commit

## The Data Model

At the type level, a graph is just:

```c
typedef Graph(MyNodeType) MyGraph;
```

Each live node has:

- a user payload of type `T`
- an outgoing adjacency list
- an incoming adjacency list
- a scratch visit count
- a stable node id while the node remains live

The important part is that node identity is not the payload.

That matters because these are all valid node payloads:

- `Str`
- `BasicBlock *`
- `Module *`
- a small by-value struct such as `Intersection`

Two nodes can carry equal payload values and still be distinct graph nodes.

## Node Ids And Node Handles

The graph exposes two related but different concepts:

- `GraphNodeId`
- `GraphNode`

### `GraphNodeId`

`GraphNodeId` is the stable public id for a live node.

Internally it packs:

- a slot index
- a generation

That lets the graph reuse storage after deletion without silently treating an old id as if it still referred to the same logical node.

Practical rule:

- live id: valid for access and traversal
- deleted id after `GraphCommitChanges(...)`: stale, do not use with live-node APIs

### `GraphNode`

`GraphNode` is the traversal handle used by the foreach macros and handle-based helpers.

Use it when you want to:

- read the payload through `GraphNodeData(...)`
- access the node id through `GraphNodeGetId(...)`
- update the scratch visit count
- iterate neighbors or predecessors

The usual pattern is:

```c
GraphForeachNode(&graph, node) {
    WriteFmtLn("node id = {}", GraphNodeGetId(node));
}
```

## Traversal Model

The graph exposes both directions explicitly.

Forward traversal:

- `GraphOutDegree(...)`
- `GraphNeighborAt(...)`
- `GraphNodeForeachNeighbor(node, neighbor)`

Reverse traversal:

- `GraphInDegree(...)`
- `GraphPredecessorAt(...)`
- `GraphNodeForeachPredecessor(node, predecessor)`

That reverse path is maintained incrementally by the graph. It is not computed lazily by scanning every edge on demand.

That is the right tradeoff for the workloads this container is meant for:

- CFG predecessor queries
- dependency backtracking
- reverse reachability
- graph rewrite passes

## Reachability With Named Intersections

A city map is a good example because it shows the split between topology and external lookup.

You normally want:

- graph topology for roads
- node payloads for intersection data
- a side index from name to node id

```c
typedef Graph(Str) CityGraph;
typedef Map(const char *, GraphNodeId) CityIndex;
```

Then:

- initialize the graph with `GraphInitWithDeepCopy(NULL, StrDeinit)`
- add the intersection name with `GraphAddNodeR(&graph, StrZ(name))`
- store `name -> node_id` in a `Map`
- connect intersections with `GraphAddEdge(...)`

That means the graph is responsible for connectivity, while the map handles name lookup.

### Why The Name Index Lives Outside The Graph

Because names are not special. They are just one possible payload.

For another workload, the payload might instead be:

- `BasicBlock *`
- `Module *`
- `TaskId`

The graph should stay neutral and let the application choose how payload lookup works.

## A Simple Reachability Pattern

For small one-off traversals, the graph-owned scratch visit count is enough:

```c
static bool reachable_from(GraphNode node, GraphNodeId goal_id) {
    if (GraphNodeVisitCount(node) > 0) {
        return false;
    }

    GraphNodeVisit(node);
    if (GraphNodeGetId(node) == goal_id) {
        return true;
    }

    GraphNodeForeachNeighbor(node, neighbor) {
        if (reachable_from(neighbor, goal_id)) {
            return true;
        }
    }

    return false;
}
```

Before starting a new traversal, reset visits:

```c
GraphForeachNode(&graph, node) {
    GraphNodeUnvisit(node);
}
```

This is intentionally lightweight. It is good for:

- DFS-style reachability
- one traversal at a time
- graph-local scratch state

It is not the only way to track traversal state.

## External Side State

For more serious analysis, use your own side tables.

Typical examples:

- `Map(GraphNodeId, u64)` for sparse counters
- `Vec(u64)` keyed by slot index for dense side state
- domain-specific arrays for colors, timestamps, distances, dominators, or worklist flags

This is the important caveat:

- `GraphNodeId` generation changes when a slot is reused
- `GraphNodeIndex(node)` can repeat across delete-and-reuse commits

So if you key a dense side array by slot index, you must either:

- reset it when deleted slots can be reused
- or pair it with generation-aware logic

That is not a bug in the graph. It is part of the storage contract.

## CFGs And Dependency Graphs

`Graph(T)` is a good fit for control-flow or dependency analysis because both node payload styles are supported well.

### CFG By Pointer

Most compiler pipelines already own their basic blocks elsewhere.

In that case, store pointers:

```c
typedef Graph(BasicBlock *) Cfg;
```

Why pointer payload is often right here:

- the graph should not own the whole IR object model
- the payload is large and already managed elsewhere
- graph passes usually care about connectivity first and block metadata second

Then predecessor traversal becomes natural:

```c
GraphNode block = GraphGetNode(&cfg, block_id);

GraphNodeForeachPredecessor(block, pred) {
    BasicBlock *pred_bb = GraphNodeData(&cfg, pred);
    (void)pred_bb;
}
```

### Dependency Graph By Value Or Pointer

For dependency analysis, either payload form can be fine:

- `Graph(Str)` if the graph owns module names directly
- `Graph(Module *)` if a larger module table owns the real objects

The graph does not force one style. That choice belongs to the surrounding system.

## Deferred Deletion

The container does not support arbitrary destructive mutation during active traversal.

That is deliberate.

Instead, the graph uses a two-stage model:

1. mark nodes or edges
2. commit changes

Available operations:

- `GraphMarkNodeForDeletion(node)`
- `GraphUnmarkNodeForDeletion(node)`
- `GraphNodeMarkedForDeletion(node)`
- `GraphMarkEdgeForRemoval(g, from, to)`
- `GraphUnmarkEdgeForRemoval(g, from, to)`
- `GraphEdgeMarkedForRemoval(g, from, to)`
- `GraphCommitChanges(g)`

This is useful for rewrite passes such as:

- "remove all dead blocks after analysis"
- "remove selected dependency edges after pruning"
- "mark this subgraph now, actually delete it after traversal"

### Why Not Delete Immediately?

Because immediate structural mutation during iterator-driven traversal creates fragile rules very quickly:

- iteration order changes under your feet
- neighbor arrays shift while you are still walking them
- users end up depending on subtle invalidation behavior

Mark-then-commit avoids that.

## What Is Safe During Traversal

Inside the graph foreach APIs, the intended operations are:

- read payloads
- update payloads
- use `GraphNodeVisit(...)` and `GraphNodeUnvisit(...)`
- mark and unmark nodes for deletion
- mark and unmark edges for removal

What is not allowed:

- adding nodes
- adding edges
- clearing the graph
- committing changes

Those are structural mutations, and the iterators will abort if they detect them.

## Important Pitfalls

### 1. Stale Ids Are Real Errors

If you delete a node and commit, the old id is not a "maybe false" query key. It is stale.

That means APIs that expect live ids, such as:

- `GraphNodeAt(...)`
- `GraphOutDegree(...)`
- `GraphInDegree(...)`
- `GraphNeighborAt(...)`
- `GraphPredecessorAt(...)`
- `GraphHasEdge(...)`

should only be called with ids you know are still live.

Use `GraphContainsNode(...)` when probing an old stored id.

### 2. `GraphNodeIndex(...)` Is Not A Permanent Identity

It is the slot index, not the full logical identity.

After delete-and-reuse:

- slot index may repeat
- generation changes

So index-keyed side tables need reset or generation-aware handling.

### 3. The Scratch Visit Count Is Shared Graph State

It is convenient, not magical.

If you need:

- multiple independent traversals
- nested algorithms with distinct state
- richer metadata than one counter

use your own side tables.

## A Good Mental Model

The clean way to think about `Graph(T)` is:

- the graph owns topology
- the graph owns the node payload `T`
- `GraphNodeId` is live-node identity
- `GraphNode` is the traversal handle
- predecessor and successor lists are maintained eagerly
- destructive edits are deferred
- richer analysis state belongs outside the graph

That model scales from toy reachability all the way up to CFG and dependency analysis without forcing the graph container to become an application framework.
