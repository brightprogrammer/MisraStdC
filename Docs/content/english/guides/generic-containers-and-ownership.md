---
title: "Generic Containers and Ownership"
date: 2026-04-19
description: "Type-parameterized containers in MisraStdC, and the L vs R insertion forms."
authors:
  - siddharth-mishra
tags:
  - containers
  - generics
  - ownership
---

> **Note**: This post was drafted by an AI assistant under direction from the author. It is not first-hand writing; the design choices it describes are real, the prose explaining them is generated. Treat the technical content as the design talking, and the framing as a translation layer.

`Vec(T)`, `List(T)`, `Map(K, V)`, `Graph(T)`, and `Pair(A, B)` are type-parameterized containers. You pick the element type at the declaration; the container handles storage and bookkeeping.

## Declaring a container type

```c
typedef Vec(int) IntVec;
typedef List(Str) StrList;
typedef Map(Str, i32) Counts;
```

If the element type itself contains a comma (most often a nested template), wrap it in `T(...)` so the preprocessor sees one argument:

```c
typedef Vec(T(Pair(i32, Str))) PairVec;
```

## L vs R: who owns the value after insertion

Most insertion APIs come in two forms. The difference is what happens to the *source* after the call.

```c
Scope(lt, DefaultAllocator) {
    Vec(int) v = VecInit();

    int x = 42;
    VecPushBackL(&v, x);     // L: container takes ownership of x
    // x is now zero.

    int y = 99;
    VecPushBackR(&v, y);     // R: container copies y
    // y is still 99.

    VecDeinit(&v);
}
```

L is for *move*: the source is left empty. R is for *copy*: the source is unchanged.

For plain `int`, the practical difference is small. It matters when the element owns memory of its own — a `Str`, an `Int`, a nested `Vec`. With L, you avoid duplicating the inner allocation; with R, the container makes its own deep copy through a registered `copy_init` callback.

The unsuffixed form (`VecPushBack`, `VecInsert`, `MapInsert`) is an alias for the L form. Reach for R explicitly when you want to keep the source around.

## A few rules to keep in mind

- **L requires a writable l-value.** A string literal or an arithmetic literal will not compile against `VecPushBackL`; it needs storage the macro can zero.
- **R takes anything assignable.** Literals, `const`-qualified sources, expressions — all fine.
- **Container-owning element types still need a copy callback.** When the element is a `Str` and you want R to deep-copy instead of bit-copy, register a `copy_init` / `copy_deinit` pair at init time (`VecInitWithDeepCopy`).

## Allocator

Every container carries an `Allocator *` field. You bind it at construction inside a `Scope` block — see *Scope-Based Allocator Discipline* for the full story. The container never destroys the allocator; the surrounding scope does.
