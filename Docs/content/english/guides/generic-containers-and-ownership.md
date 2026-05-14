---
title: "Generic Containers and Ownership"
date: 2026-04-19
description: "How MisraStdC uses macros for generic containers, and why l-value vs r-value insertion matters."
authors:
  - siddharth-mishra
tags:
  - containers
  - generics
  - ownership
---

One of the core ideas in MisraStdC is that generic containers should feel reusable without requiring a code generator.

The implementation approach is macro-based, but the runtime model is shared.

## How The Generic Model Works

Template-style APIs such as `Vec(T)`, `List(T)`, `Iter(T)`, and `Pair(xT, yT)` expand through macros.

The important part is what happens after expansion:

- the call site determines the concrete type information
- the macro forwards size and layout details to shared helpers in `Source/`
- the runtime code performs the actual storage management and operations

That means the project gets a generic programming style without introducing a separate template compiler or code generation phase.

## Practical Consequences

This model has a few consequences that matter when writing real code.

### Distinct Expanded Types

Each `Vec(T)` expansion is its own anonymous type.

If you want to reuse it across declarations, create a `typedef`:

```c
typedef Vec(int) IntVec;
typedef List(Str) StrList;
```

### Nested Macro Types Need Wrapping

If a nested macro type contains commas, wrap it so the outer macro sees it as one argument:

```c
typedef Vec(T(Pair(i32, Str))) PairVec;
```

Without that wrapper, the preprocessor will split the inner commas as separate macro arguments.

## Ownership Is An API-Level Concept

MisraStdC makes ownership transfer visible in insertion APIs.

That is the reason for the `...L` and `...R` distinction in many container operations.

### L-Value Insertion

The `...L` forms are about explicit ownership transfer.

If the container does not have a deep-copy callback, it may take ownership by zeroing the source l-value after insertion. The intent is that only one live owner remains.

This is useful when building values in a temporary variable and moving them into a container.

### R-Value Insertion

The `...R` forms are less strict. They let the caller treat the insertion more like a normal value-style operation.

That does not make ownership disappear. It just means the API is not trying to annotate the transfer as aggressively.

## Allocator Ownership

Every dynamically-sized container in MisraStdC carries an `Allocator *` field
internally. Containers do not own their allocator and never deinit it; the
allocator must outlive every container that references it.

The recommended way to bind a container to an allocator is the `Scope` macro:

```c
#include <Misra/Std/Allocator/Default.h>

Scope(alloc, DefaultAllocator) {
    Vec(int) v = VecInit();         // implicit MisraScope pool
    Vec(int) w = VecInit(alloc);    // explicit named pool
    ...
    VecDeinit(&v);
    VecDeinit(&w);
}   // allocator destroyed automatically at block exit
```

`Scope(name, AllocType)` constructs two typed allocator instances on the stack:

- `name` - the user-visible pool, available to the caller and any helper
  the caller hands the allocator to.
- `MisraScope` - an internal pool that the zero-arg form of every `*Init()`
  macro picks up implicitly.

Both are deinit'd together when control leaves the block. Outside a `Scope`,
calling `VecInit()` with no argument fails to compile, which is the safety net
that prevents accidental dependence on a hidden global heap.

For helpers that take an `Allocator *` parameter, use `ScopeWith(alloc)` to
borrow the caller's allocator into the same `MisraScope` slot without
constructing a new one. `return` and `goto` that leave a `Scope` block
skip the deinit step and leak the allocator - a C-level limitation with no
portable workaround. Use `ExitScope` (an alias for `break`) when you need to
leave early but still run the cleanup.

The four other allocator types - `PageAllocator`, `ArenaAllocator`,
`SlabAllocator`, `BudgetAllocator` - all work the same way: declare on the
stack, hand to containers (or `Scope`), deinit at the end of their useful
lifetime.

## Why This Matters

A lot of C bugs are not algorithmic bugs. They are lifetime bugs:

- double ownership of mutable state
- cleanup responsibilities that are implied rather than stated
- container insertion code that silently copies in one place and silently moves in another

MisraStdC tries to make those transitions more visible in the function names and call sites.

It is a stricter style than plain C utility code, but it makes large container-heavy code easier to reason about.

## Where This Connects To The Rest Of The Library

The same philosophy shows up in the newer `Int` and `Float` APIs too.

The public surface is being pushed toward generic front doors:

- one `IntCompare`, not a pile of public compare overload names
- one `IntFrom`, not a user-facing family of construction helpers by exact scalar type
- one `FloatFrom`, `FloatAdd`, `FloatSub`, and related arithmetic entry points

The common theme is simple:

Users should learn the API they are meant to call, not the implementation fan-out behind it.
