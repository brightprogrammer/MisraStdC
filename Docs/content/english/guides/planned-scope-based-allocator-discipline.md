---
title: "Planned Refactor: Scope-Based Allocator Discipline"
date: 2026-05-14
description: "An AI-generated summary of the design direction for MisraStdC's allocator API, where the public surface is gated behind a lexical `Scope` macro and `*Alloc`-suffixed forms are removed from public headers."
authors:
  - siddharth-mishra
tags:
  - design
  - allocators
  - scope
  - macros
  - lifetimes
---

{{< notice "note" >}}
This article is an AI-generated summary of a design discussion about the next stage of the MisraStdC allocator refactor.
It is a snapshot of the planned direction, not a statement that the implementation is already complete.
The summary builds on top of the design recorded in *Planned Refactor: Fallible APIs, Typed Macros, and Allocators*.
{{< /notice >}}

The fallible-APIs / typed-allocator refactor landed the foundational pieces: typed allocator structs (`HeapAllocator`, `PageAllocator`, `ArenaAllocator`, `PoolAllocator`), allocator-owning containers, and fallible runtime helpers. What remained was the question of *how* the public API should expose the choice of allocator to callers.

That question is what this document is about. The conclusion is a three-tier architecture with a lexically scoped allocator handle. There are no library globals, no thread-local storage, and no `*Alloc`-suffixed function in the public headers.

## Why This Refactor Is Happening

The earlier work added an `Allocator *` field to every container. That part was right. What was missing was a clean answer for how callers supply the allocator at construction time.

Several intermediate designs were considered and rejected:

- a process-global `DefaultAllocator()` accessor returning a static instance - rejected for being a library global with shared mutable state and an implicit thread-safety contract the rest of the library does not need
- thread-local "current allocator" storage manipulated by a `WithAllocator` block - rejected for two reasons: it is a hidden global in the threading sense, and action-at-a-distance through helper functions makes call-site behavior unpredictable
- threading `Allocator *alloc` through every public function signature - works but creates substantial API churn, doubles the cognitive load at every call site, and provides little incremental safety once containers already own their allocators

The landed direction takes a different shape. Allocator lifetime becomes *lexically* scoped. A `Scope(name, AllocType) { ... }` macro creates an allocator on the stack, exposes it for the duration of the block, and destroys it automatically when the block ends. Code outside any `Scope` cannot call the allocator-aware front-door macros - the compiler enforces this through ordinary identifier resolution.

The point is not novelty. The point is matching the lifetime story the user already has to think about anyway, and making the compiler refuse to let them forget.

## The Three Tiers

The allocator-aware API is organized in three tiers, only one of which is publicly visible.

### Tier 1: Public Scope-aware macros

These are the names callers see at every site that creates a new container, parses external input, or otherwise allocates fresh memory. They are macros, they take no allocator argument, and they read an identifier named `MisraScope` that is only declared inside `Scope`-introduced blocks.

Examples of the intended shape:

```c
Scope(lifetimeA, DefaultAllocator) {
    Vec(int) v          = VecInit();
    Str      line       = StrInitFromCstr("a,b,c", 5);
    BitVec   flags      = BitVecFromStr("10110");
    Float    pi         = FloatFrom(3.14);
    Strs     parts      = StrSplit(&line);

    my_helper(&v, lifetimeA);
}
```

Calling these macros outside any `Scope` is a compile error. The compiler diagnostic points at an undeclared identifier (`MisraScope`), which makes the rule self-teaching after the first encounter.

### Tier 2: Operations on existing objects

Operations that act on already-constructed objects do not need `Scope`. They use the allocator that the object already carries internally. Examples include `VecPushBack`, `StrDeinit`, `BitVecSet`, `IntAdd`, `SysMutexLock`, `SysProcDestroy`. These work anywhere - inside a `Scope`, outside a `Scope`, in helper functions, in test bodies.

The cleaner phrasing for the split between tier 1 and tier 2 is: **`Scope` governs birth. The object governs life and death.**

### Tier 3: Private `_alloc` primitives

The actual implementations live in tier 3 and are private to the library. They are ordinary functions with snake_case names ending in `_alloc`, take an explicit `Allocator *` parameter, and live behind per-module `Private.h` headers (for example `Include/Misra/Std/Container/Vec/Private.h`). User code is not expected to include those headers.

Tier 1 macros are typically one-line wrappers that forward to tier 3 with `MisraScope` substituted for the allocator argument:

```c
#define VecInit()                       vec_init_alloc(MisraScope)
#define StrInitFromCstr(cstr, len)      str_init_from_cstr_alloc((cstr), (len), MisraScope)
#define BitVecFromStr(zstr)             bitvec_from_str_alloc((zstr), MisraScope)
```

This keeps the public surface ergonomic, keeps the implementation surface explicit and testable, and gives the library a single place to change the API contract if the design needs to evolve.

## The Scope Macro

`Scope` and its companions are small. They expand into a pair of nested `for` loops in the same family of trick already used by `VecForeach` and similar iteration macros.

```c
#define MisraScope __misra_scope_alloc

#define Scope(name, AllocType)                                                 \
    for (AllocType _scope_a_##name = AllocType##Init(),                        \
                  *_scope_loop_##name = &_scope_a_##name;                      \
         _scope_loop_##name;                                                    \
         AllocType##Deinit(&_scope_a_##name),                                   \
         _scope_loop_##name = NULL)                                             \
        for (Allocator *name              = &_scope_a_##name.base,             \
                      *MisraScope        = name,                                \
                      *_scope_done_##name = name;                                \
             _scope_done_##name;                                                 \
             _scope_done_##name = NULL)

#define ScopeWith(alloc_ptr)                                                   \
    for (Allocator *MisraScope = (alloc_ptr),                                  \
                  *_scope_with_done = MisraScope;                              \
         _scope_with_done;                                                      \
         _scope_with_done = NULL)

#define ExitScope break
```

The outer `for` declares the typed allocator instance (`_scope_a_<name>`) and a sentinel pointer (`_scope_loop_<name>`). Its increment expression destroys the allocator and trips the sentinel, ending the loop. The inner `for` exposes three identifiers to the user body:

- `name` - an `Allocator *` named by the caller, for handing the allocator to helpers as a function argument
- `MisraScope` - an `Allocator *` used implicitly by the tier-1 macros
- `_scope_done_<name>` - an internal sentinel that ends the inner loop

The initial design considered making `name` and `MisraScope` point at *separate* allocator instances so that library scratch and user-driven allocations would land in disjoint pools. That separation was rejected because both pools would die together at `Scope` exit, which collapses every concrete benefit (fragmentation isolation, separable accounting, independent failure injection) without removing the cost (2x typed-allocator state on the stack, 2x mmap surface, double the cognitive load). `name` and `MisraScope` therefore alias the same pointer in the landed design. If isolation becomes useful later, it can be reintroduced as an opt-in `ScopeIsolated` variant without changing the call-site syntax of plain `Scope`.

### Control flow inside a Scope

The for-loop expansion gives the user body the natural control-flow story:

| User writes | Outcome |
| --- | --- |
| Normal fall-through | Inner loop sentinel trips, outer loop runs deinit, scope exits cleanly |
| `break` (or `ExitScope`) at scope top level | Inner loop exits, outer loop runs deinit, scope exits cleanly |
| `continue` at scope top level | Inner loop increment trips the sentinel, same path as `break` |
| `return` from inside the scope | Function returns immediately, deinit is skipped, the allocator leaks |
| `goto` to a label outside the scope | Same as `return`: deinit skipped, allocator leaks |

The `return` and `goto` cases are a C-level limitation that cannot be papered over portably (`__attribute__((cleanup))` works on GCC and Clang but not MSVC). They are documented as known footguns.

### ScopeWith for helpers

Helpers that need to allocate but receive their allocator from a caller use `ScopeWith` to expose the caller-supplied pointer as `MisraScope` without taking ownership:

```c
void my_helper(Vec(int) *v, Allocator *lifetimeA) {
    ScopeWith(lifetimeA) {
        Str scratch = StrInitFromCstr("hi", 2);
        StrDeinit(&scratch);
    }
}
```

`ScopeWith` does not create or destroy anything. It only introduces the binding. This is the mechanism by which an allocator travels across function-call boundaries while keeping the tier-1 macro ergonomic on the helper side.

### Nesting

Both `Scope` and `ScopeWith` rely on ordinary C identifier shadowing for nesting:

```c
Scope(outer, DefaultAllocator) {
    Vec(int) o = VecInit();

    Scope(inner, ArenaAllocator) {
        Vec(int) i = VecInit();         // uses inner's allocator
        my_helper(&i, inner);
    }                                    // arena destroyed, MisraScope = outer again

    VecPushBack(&o, 42);                 // o's embedded allocator still alive
}                                        // default destroyed, everything inside gone
```

The inner `Scope` introduces a fresh `_scope_a_inner`, `inner`, `MisraScope`, and `_scope_done_inner`. Inside the inner block, `MisraScope` refers to the inner allocator. After the inner block, the outer block's identifiers are visible again.

## Why No Library Globals, Restated

The properties that fall out of the lexical design, that were the motivation for rejecting the alternatives, are:

- **No process-global state.** Every `Scope` instance lives on the stack of the function that wrote `Scope(...)`. There is no `static` or thread-local storage anywhere in the allocator path.
- **Thread safety is automatic.** Each thread's `Scope` creates its own stack-local allocator. No lock is needed because no state is shared.
- **No action at a distance.** A helper function called from inside a `Scope` cannot see the caller's `MisraScope` - the identifier is not in the helper's lexical scope. The helper must either take an explicit `Allocator *` parameter and use `ScopeWith`, or open its own `Scope`. Implicit propagation across function-call boundaries is impossible.
- **"Forgot to enter a Scope" is a compile error.** Misuse produces a diagnostic at the point of misuse rather than a runtime surprise.

These four properties hold simultaneously. None of the rejected designs achieved all four.

## Migration Plan

The refactor will land in small commits in this order:

1. **Design document.** This file. Lands first, before any code, as a guide for users and contributors who will see follow-up commits land later.

2. **Tier-3 private `_alloc` primitives.** Most of the work already exists in the in-progress branch state - it is the same as the explicit-allocator API the earlier stash had been building, just renamed to snake_case and moved into per-module `Private.h` headers.

3. **Scope, ScopeWith, ExitScope, MisraScope macros.** Added to `Include/Misra/Std/Allocator.h`. Roughly thirty lines of macro plumbing.

4. **Tier-1 macros wired to one container, end-to-end (pilot).** `Vec` is the natural choice because most other containers depend on it. A single test exercises `Scope` + `VecInit` + `VecPushBack` + scope exit, including the helper / `ScopeWith` composition.

5. **Fan out to remaining containers.** `List`, `Map`, `Graph`, `BitVec`, `Str`, `Int`, `Float` in that rough order.

6. **System utilities and parsers.** `Sys`, `Sys/Mutex`, `Sys/Proc`, `Sys/Dir`, `Std/File`, `Std/Log`, `Std/Io`, `Parsers/JSON`, `Parsers/KvConfig`. These were the call sites where the original explicit-threading attempt got stuck.

7. **Tests, Bin, Fuzz harnesses.** Call sites converted from explicit-allocator forms (`VecInit(&a)`) to scoped forms (`Scope(a, DefaultAllocator) { VecInit(); }`). Tests that exercised the older fallback allocator path are removed or rewritten.

8. **AGENTS.md update.** A short section documenting the three-tier layout and the `Scope` discipline, so contributors know which tier they are extending when they add new functionality.

Each step builds green before the next one starts.

## What Is Not Changing

Several pieces of the existing design stay as they are.

### Containers continue to own their allocators

The `Allocator *allocator` field on `GenericVec`, `BitVec`, `GenericList`, `GenericMap`, and `GenericGraph` is unchanged. Containers continue to carry their allocator with them, which is what makes tier-2 operations work without any scope.

### Fallible API surface stays

The `Must...` versus propagating naming distinction described in the earlier refactor document is unchanged. Tier-1 macros expand to fallible tier-3 calls and propagate failure; `Must...` wrappers continue to abort on failure.

### Allocator implementations stay

`HeapAllocator`, `PageAllocator`, `ArenaAllocator`, and `PoolAllocator` are not changing shape. Only their lifecycle in user code is changing.

### `*Aligned(...)` builders stay

Allocator builders such as `HeapAllocatorAligned(N)` remain the way callers express stronger alignment requirements. These compose with `Scope` naturally because `Scope` takes a type, not an instance.

## Decisions Captured Up Front

A small number of design questions have already been settled and are recorded here so future readers do not relitigate them.

### `*Alloc`-suffixed functions are not part of the public API

The original explicit-threading plan exposed `VecInitAlloc(alloc)`, `StrInitFromCstrAlloc(cstr, len, alloc)`, and so on as public names. The Scope-based design hides these. Tier 3 keeps them, snake_cased and gated behind `Private.h`. Public users only see tier-1 macros.

The reason is uniformity. With both surfaces public, every call site became a choice between two equivalent forms, and the project would have to document when to use each. With only the Scope-based surface public, there is one way to construct, one mental model, and the lifetime discipline is forced rather than optional.

### Helpers do not auto-inherit the caller's scope

A helper function called from inside `Scope` does not see the caller's `MisraScope`. It must take an `Allocator *` parameter explicitly. This is by construction - the lexical scope of `MisraScope` ends at the boundary of the function that wrote `Scope(...)`.

This was a positive design property, not a limitation. The alternative (TLS-based current-allocator) gives helpers automatic access to the caller's allocator, which makes it impossible to read a helper in isolation and know what it allocates from. Forcing the parameter to be explicit at function boundaries makes allocation behavior visible at every call site that crosses a function call.

### `Scope` aliases instead of separating user / internal pools

`name` and `MisraScope` point at the same allocator. The earlier dual-pool design is recorded above and was rejected because the two pools would always co-terminate, neutralizing every concrete benefit while keeping all of the costs. An opt-in `ScopeIsolated` variant is reserved as an extension point for the day a real workload demonstrates that the separation pays off.

### `ExitScope` is `break`

`ExitScope` is a one-line `#define` to `break`. Like any C `break`, it escapes only the nearest enclosing loop. If a caller writes `ExitScope` from inside a user `for` or `while` that is itself inside a `Scope`, the user loop ends and the surrounding `Scope` continues. To escape both, the caller exits the inner loop first, then `ExitScope`. This matches familiar C semantics rather than introducing a new escape mechanism, and it avoids macro-hygiene problems that arise from synthesizing labels.

### Initial allocator type is a type name, not an instance

`Scope(name, AllocType)` takes the *type* of the allocator (`DefaultAllocator`, `ArenaAllocator`, ...). The instance is constructed by the macro. Callers who already have an allocator on hand use `ScopeWith(existing_pointer)` instead. Splitting the two macros keeps each one focused.

## Closing View

The earlier refactor made the library's failure model explicit and put the allocator into the type system. This refactor takes the next step and makes the *lifetime* of the allocator explicit at the call site.

The intended end state is:

- the public way to construct anything is `Scope(...) { ... }`
- the compiler rejects allocations that are not inside a `Scope`
- helpers carry allocators across function boundaries as explicit parameters
- the allocator never lives in process-global or thread-local storage
- the runtime cost is one stack-local allocator instance per `Scope` and one for-loop's worth of bookkeeping

The library keeps every property the earlier refactors added. It also keeps its bias toward strictness, in the same way that the fallible refactor kept caller bugs fatal and made runtime failure propagatable. Scope-based allocator discipline is the natural extension: caller bugs around lifetime become compile errors instead of runtime use-after-free.
