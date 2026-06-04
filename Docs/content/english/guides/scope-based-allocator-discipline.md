---
title: "Scope-Based Allocator Discipline"
date: 2026-05-14
description: "Use Scope blocks to give your containers an allocator with a clean lifetime."
authors:
  - siddharth-mishra
tags:
  - allocators
  - scope
  - lifetimes
---

> **Note**: This post was drafted by an AI assistant under direction from the author. It is not first-hand writing; the design choices it describes are real, the prose explaining them is generated. Treat the technical content as the design talking, and the framing as a translation layer.

Containers in MisraStdC need an allocator. You give them one by writing the construction code inside a `Scope` block. The block creates the allocator on entry, exposes it for the body, and destroys it on exit.

## Minimum example

```c
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Container/Vec.h>

void demo(void) {
    Scope(lifetime, DefaultAllocator) {
        Vec(int) v = VecInit();
        VecPushBack(&v, 1);
        VecPushBack(&v, 2);
        VecDeinit(&v);
    }   // allocator destroyed here
}
```

That's everything for the common case. `VecInit()` (no arguments) reaches for the allocator the block put in scope. Outside any `Scope`, the same call is a compile error — the identifier it relies on is not in scope. The compiler diagnostic will point at an undeclared identifier named `MisraScope`; that is the rule announcing itself.

## Passing the allocator to a helper

Sometimes a helper needs to allocate too. Give it the allocator as a parameter, and let the helper open its own `ScopeWith` block to plug that pointer into the same machinery:

```c
void my_helper(Vec(int) *v, Allocator *alloc) {
    ScopeWith(alloc) {
        Str scratch = StrInitFromCstr("hi", 2);
        // ... use scratch ...
        StrDeinit(&scratch);
    }
}

void caller(void) {
    Scope(lifetime, DefaultAllocator) {
        Vec(int) v = VecInit();
        my_helper(&v, lifetime);
    }
}
```

`Scope` creates the allocator. `ScopeWith` borrows one that someone else owns. The helper does not free anything; the caller's `Scope` still owns the lifetime.

## Control flow

| You write | What happens |
| --- | --- |
| Normal fall-through to the end of the block | Allocator destroyed cleanly. |
| `break` or `ExitScope` | Allocator destroyed cleanly. |
| `continue` at the top level of the block | Same as `break`. |
| `return` from inside the block | **Allocator leaks.** Body skipped past cleanup. |
| `goto` to a label outside the block | **Allocator leaks.** Same reason. |

`return` and `goto` are the only two ways to leave a scope without cleanup. If you need an early exit, use `ExitScope` instead.

## Nesting

You can put a `Scope` inside another `Scope`:

```c
Scope(outer, DefaultAllocator) {
    Vec(int) o = VecInit();

    Scope(inner, ArenaAllocator) {
        Vec(int) i = VecInit();   // uses inner
        VecDeinit(&i);
    }                              // inner destroyed; outer back in charge

    VecPushBack(&o, 42);           // outer still alive
    VecDeinit(&o);
}                                  // outer destroyed
```

Inside `inner`, `VecInit()` uses the inner allocator. After `inner` ends, the outer one is back in scope. Standard C identifier shadowing — no special rule.

## What lives inside a Scope vs not

- **Construction:** new `Vec`, `Str`, `Map`, `Graph`, `BitVec`, `Int`, `Float`, parsed input, etc. — these go inside a `Scope`.
- **Operations on existing objects:** `VecPushBack`, `StrDeinit`, `BitVecSet`, `MapInsertR`, etc. — these work anywhere. The container already remembers its allocator.

A short way to say it: *Scope governs birth. The object governs life and death.*
