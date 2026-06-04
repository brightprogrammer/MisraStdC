---
title: "Fallible APIs and Allocators: the design"
date: 2026-05-12
description: "Where the library draws the line between a caller bug and a runtime failure."
authors:
  - siddharth-mishra
tags:
  - design
  - allocators
  - error-handling
---

> **Note**: This post was drafted by an AI assistant under direction from the author. It is not first-hand writing; the design choices it describes are real, the prose explaining them is generated. Treat the technical content as the design talking, and the framing as a translation layer.

MisraStdC splits failure into two kinds and treats them differently. This page explains the split and the naming convention that goes with it.

## Two kinds of failure

**Caller bug.** Out-of-bounds access, null pointer where one was promised, uninitialized object, broken invariant. The library does not try to recover. It aborts on the spot with a message and a stack trace.

**Runtime failure.** Allocation failure, parse failure on external data, allocator exhaustion. The library returns `false` and lets the caller decide what to do.

## The naming rule

For every operation that can fail at runtime, there are two public names:

- The plain name returns `bool`. You handle the result.
- The `Must` variant aborts on failure. You use it when failure is genuinely not recoverable at this layer.

```c
if (!VecInsertL(&values, item, idx)) {
    // handle the allocation failure
    return false;
}

VecMustInsertL(&values, item, idx);
// keeps going on success, aborts on failure
```

The plain name is the default. The `Must` form is the explicit opt-in to "treat failure as a programmer error from here up." Use it at the top of binaries or in test fixtures, not deep inside a library.

## L and R insertion forms

Insertion APIs come with `L` and `R` suffixes for ownership transfer behavior — see *Generic Containers and Ownership* for the full story. The failure model intersects them in one important way: the `L` form only zeros the source on success. A failed `L` insertion leaves the source untouched, so the caller can retry or hand it elsewhere.

## What about allocators

Containers carry an `Allocator *` field and bind it at construction. The lifetime story is told by the `Scope` macro — see *Scope-Based Allocator Discipline*. Once bound, the allocator does not move; if you need a different policy, you build a different container.

Five allocator backends ship with the library: `HeapAllocator`, `PageAllocator`, `ArenaAllocator`, `SlabAllocator`, `BudgetAllocator`. Pick one based on lifetime and growth pattern, not on capabilities — all five can back any container.

## What "landed" means today

The allocator half of this design is in: typed allocator structs, allocator-owning containers, per-type magic validation, the `Scope` discipline.

The fallible-API half is partway through. Some container operations still abort on allocation failure instead of returning `false`. If you find one without a `Must` peer, treat that as a piece that hasn't been converted yet — the public convention is that the plain name propagates.
