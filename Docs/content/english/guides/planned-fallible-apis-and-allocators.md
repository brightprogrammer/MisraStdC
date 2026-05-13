---
title: "Planned Refactor: Fallible APIs, Typed Macros, and Allocators"
date: 2026-05-12
description: "An AI-generated summary of the current design direction for the MisraStdC refactor around error propagation, typed macros, and allocator-backed containers."
authors:
  - siddharth-mishra
tags:
  - design
  - allocators
  - error-handling
  - containers
  - macros
---

{{< notice "note" >}}
This article is an AI-generated summary of an extended design discussion about the shape of an upcoming MisraStdC refactor.
It is a snapshot of the current plan, not a statement that the implementation is already complete.
The summary reflects the refactor direction as of commit `add5d0adaf591f63d27d6c2a79a0666a582a1162` on branch `allocator-fallible-container-refactor`.
{{< /notice >}}

The refactor described here is driven by one practical problem: the library needs to be usable in a real project where allocation failure, bad external data, and ownership boundaries all need to be handled deliberately instead of collapsing into process aborts too early.

The current library already has a strong opinion about programmer errors. That part is not being thrown away. What is changing is where the library draws the line between a caller bug and an ordinary runtime failure.

## Why This Refactor Is Happening

MisraStdC started with a "fail fast" bias and a large macro surface. That worked well for early experimentation because it kept call sites short and surfaced misuse immediately. It becomes limiting once the library is used as infrastructure inside a bigger program.

The current pain points are:

- allocation-heavy container operations often abort instead of returning failure
- typed statement macros do not propagate failure naturally
- ownership transfer logic is embedded into macro expansions
- custom allocator support needs a consistent way to bubble failure upward
- deep-copy behavior needs to follow the allocator of the destination object, not just whatever helper happens to run next

The goal of the refactor is not to make the library "soft" or permissive. The goal is to make the library stricter about programmer misuse and more disciplined about runtime failure.

## Error Propagation Policy

The new policy keeps a hard split between programmer error and normal failure.

### What stays fatal

These remain abort conditions:

- violated API preconditions
- null object pointers where a valid object is required
- out-of-bounds accesses caused by caller misuse
- using uninitialized or corrupted objects
- broken internal invariants

This preserves the existing philosophy that caller mistakes should fail at the source instead of teaching the library to silently tolerate misuse.

### What must propagate

These become ordinary failure paths:

- allocation failure
- allocator exhaustion
- parse failure from external data
- domain or range failures that are not caller contract violations
- deep-copy failures caused by allocation or owned subobject construction

This is the central behavioral change. A low-level operation may still end in an abort if the caller explicitly chooses a must-succeed API, but the library surface itself must make failure propagation possible.

## Public Naming Direction

The public naming rule for allocating and otherwise fallible operations is:

- the plain operation name propagates failure
- the `Must...` variant aborts on failure

Examples of the intended direction:

- `VecInsertL(...)` returns `bool`
- `VecMustInsertL(...)` aborts if insertion fails
- `VecReserve(...)` returns `bool`
- `VecMustReserve(...)` aborts if reserve fails
- `ListInsertL(...)` returns `bool`
- `ListMustInsertL(...)` aborts if insertion fails

This is an intentional choice. It makes failure propagation the default behavior and pushes abort semantics behind an explicit name.

## Macro-Based APIs Stay

The generic, type-safe macro front-end is not going away.

That matters because macros like `VecInsertL`, `VecInsertR`, `VecPushBack`, and similar container entry points are doing real API work:

- they enforce type expectations
- they distinguish l-value and r-value forms
- they encode ownership-transfer semantics
- they keep call sites compact and consistent

The refactor does not replace those macros with raw untyped helper calls. Instead, it changes what the macros expand to.

## Propagating Macros Become Expression-Shaped

Today many typed container operations are statement-style `do { ... } while (0)` macros. That shape is useful for side effects, but it is hostile to error propagation because it does not naturally evaluate to a result.

The planned change is:

- propagating forms become expression macros that evaluate to `bool`
- `Must...` forms remain statement wrappers that abort on failure

Conceptually, the desired call sites look like this:

```c
if (!VecInsertL(&values, item, idx)) {
    return false;
}

VecMustInsertL(&values, item, idx);
```

This removes the user-visible distinction between "macro API" and "function API" for failure handling.

## The Real Work Moves Into Fallible Helpers

The macros remain the typed front door, but they stop being the place where the full mutation logic lives.

Instead:

- runtime helpers perform the actual reserve, insert, move, copy, and cleanup work
- those helpers return `bool`
- the macros perform validation, type-checking, value-category adaptation, and then call the helper

This matters because success-sensitive behavior must be centralized.

For example, ownership-transfer forms must only zero the source object on success. If insertion fails, the source value must remain untouched. That rule is easier to enforce in a single helper path than in a scattered set of statement macros.

## Type Safety Becomes Explicit

The existing type-safety behavior stays, but it becomes more deliberate.

The plan is to introduce per-family `TypeCheck` macros that are expression-shaped and can be compiled out when desired.

The intended model is:

- when type-safety enforcement is enabled, `TypeCheck...` macros actively validate argument shape
- when disabled, they expand to no-ops

One practical way to do that is with a project-level feature flag such as `MISRA_ENFORCE_TYPE_SAFETY`, where the checks do real work only when the flag is enabled.

The reason for making this optional is performance. One of the practical ways to force compile-time type safety in these macro-heavy APIs is to route values through helpers such as `LVAL(...)`, which can materialize an extra temporary and therefore introduce an additional copy at runtime.

That overhead is useful during development because it helps prove that the call sites are correct. It is much less attractive in optimized builds where the code has already been validated and the extra copy does not carry its weight anymore.

So the intended workflow is:

- enable `MISRA_ENFORCE_TYPE_SAFETY` in debug builds
- use it to catch incorrect call sites early
- disable it in release builds to remove that overhead completely once the code is known to be correct

This keeps type checking independent from allocator design and storage layout.

The point is not just static neatness. It also lets the propagating macros stay clean:

- validate the object
- perform the type check
- create an addressable temporary only when needed
- call the fallible helper

## `L` and `R` Forms Still Matter

MisraStdC already distinguishes between l-value and r-value insertion forms, and that distinction is staying.

The reason is ownership.

For `L` forms:

- the container may take ownership if no deep-copy handler is configured
- source zeroing happens only if the operation succeeds

For `R` forms:

- the source expression is treated as a value
- there is no source ownership to zero out afterward

Failure propagation makes this distinction more important, not less important. A failed insertion must not partially consume an l-value source.

## What Is Not Changing

Several ideas were discussed and rejected as the base design.

### No fake element slots in normal storage

The refactor is not going to rely on hidden temporary storage inside logical container element space.

Rejected directions included:

- using `index == length` as a temporary slot
- using `index == -1` with shifted pointers
- embedding fake scratch storage into ordinary element layout

These ideas were rejected because they couple storage invariants to macro implementation details, complicate allocation and deallocation rules, and do not generalize cleanly across container families such as `List`, `Map`, and `Graph`.

### No storage-layout redesign just to support macro temporaries

The reason container macros exist is broader than "we need a temporary variable." They also encode type checks, ownership semantics, and l-value versus r-value behavior.

That means the right abstraction is not "hide a temp in every container object." The right abstraction is "keep the typed front-end and move runtime mutation into fallible helpers."

## Allocator Redesign

The allocator work is the second major part of the refactor.

### High-level goals

The allocator system is being introduced so MisraStdC can take control of its own memory-management policy instead of hard-coding ordinary heap behavior everywhere.

This is useful for:

- embedded targets
- arenas and region allocators
- custom slab or pool allocators
- retry or fallback allocation strategies
- explicit ownership of memory policy at object init time

### Allocators are passed by value

The planned API shape is that allocators are passed by value, not by pointer.

This makes the ownership semantics clearer:

- passing an allocator into `XXXInit(...)` does not transfer ownership of the allocator object
- the destination object stores its own copied allocator configuration
- the caller remains free to reuse the same allocator configuration elsewhere

### Configuration is copied, runtime state is not

This is an important design point.

When an object is initialized with an allocator:

- the allocator configuration is copied into the object
- the allocator runtime state is not copied from the source allocator instance
- the bound allocator inside the object gets its own state

The effect is that two objects initialized from the same allocator configuration can share policy while still having separate allocator runtime state.

### Allocator binding is immutable after init

Once an object is initialized with an allocator, that allocator binding is not supposed to change.

This keeps memory ownership rules stable over the lifetime of the object and prevents allocator drift after internal allocations have already been made.

### Optional allocator on init

The init APIs are intended to support both forms:

- `VecInit()`
- `VecInit(alloc)`

If no allocator is supplied, `DefaultAllocator()` will be used.

This keeps the common path convenient without giving up explicit allocator choice.

The same idea is intended to apply more broadly across object and container helper APIs:

- `StrInitFromCstr(text, len)`
- `StrInitFromCstr(text, len, alloc)`
- `BitVecFromStr("1010")`
- `BitVecFromStr("1010", alloc)`
- `StrInitCopy(&dst, &src)`
- `StrInitCopy(&dst, &src, alloc)`

The plain name stays the public surface. Passing an allocator becomes an optional override, not a separate public API family.

### Internal allocator use stays mostly hidden

The allocator policy is not supposed to leak into every helper call.

For internal-only usage of `Init`, `Clone`, and other allocation-capable helpers:

- the allocator should usually stay hidden from the public API
- internal clones should inherit the allocator of the source object
- internal temporaries should use the allocator of the object or result they are logically tied to
- `DefaultAllocator()` should only be used for truly standalone scratch work

That means the public contract stays simple while the internal implementation still preserves allocator correctness.

Examples of the intended rule:

- a public omitted-allocator `VecInit()` uses `DefaultAllocator()`
- `StrInitCopy(...)` inherits the allocator configuration of the source object unless an explicit override API exists
- an internal `Int` or `Float` temporary used during an operation should be allocated from the relevant object allocator, not from an arbitrary global default
- a scratch formatting buffer in I/O can use a transient hidden allocator because it does not become caller-owned state

The user should not have to care about allocator details for pure implementation scratch.

### Raw pointer ownership APIs are different

Raw-pointer APIs need a stricter rule.

If a function may allocate, reallocate, free, or replace storage through a caller-provided raw pointer, then allocator provenance is part of the API contract.

The intended policy is:

- borrowed input pointers do not need allocators
- purely internal scratch allocation does not need a public allocator parameter
- object APIs like `Vec`, `Map`, `Int`, and `Float` do not need extra allocator parameters because allocator binding is already part of object state
- raw-pointer ownership APIs must take the allocator responsible for that pointer if they may resize or free it

This matters for functions that work with caller-managed buffers such as `char **data` or `void **buffer`.

For those APIs:

- the allocator handling that pointer must be passed alongside it
- the function must use that allocator only for operations related to that pointer
- the function should not rebind that allocator
- the function should not stash local long-lived copies of that allocator for unrelated work

This keeps allocator provenance correct without forcing allocator details into places where the user does not need to see them.

The intended public shape for these APIs is still a single name, but with stricter argument rules.

For example, a raw-buffer helper such as `ReadCompleteFile(...)` is expected to support both forms:

- `ReadCompleteFile(path, &buffer, &file_size, &capacity)`
- `ReadCompleteFile(path, &buffer, &file_size, &capacity, &allocator)`

The contract is:

- if `*buffer == NULL` and no allocator is supplied, `DefaultAllocator()` is used
- if `*buffer == NULL` and an allocator is supplied, that allocator is used
- if `*buffer != NULL`, the allocator responsible for that buffer must be supplied
- omitting that allocator while asking the API to reuse or grow caller-owned storage is a caller bug and should be treated as fatal

That keeps the public name simple while still preserving allocator provenance for escaping memory.

### Prefer Library Utilities Internally

The refactor should also remove direct usage of standard C utility functions when MisraStdC already provides an equivalent helper.

This includes string helpers such as `strcmp` and `strlen`, which should be replaced with `ZstrCompare` and `ZstrLen`, and memory helpers such as `memcpy`, `memmove`, and `memset`, which should move toward `MemCopy`, `MemMove`, and `MemSet` outside the low-level memory implementation itself.

The reason is consistency rather than novelty. Once the library owns allocation, failure policy, and object semantics, code inside the library should route through the same utility layer that user code is expected to use. That makes later audits easier and keeps low-level behavior centralized.

## Which Objects Own Allocators

The allocator should live in root memory-owning objects, not in every nested wrapper type.

The intended owners are:

- `GenericVec`
- `BitVec`
- `GenericList`
- `GenericMap`
- `GenericGraph`

Derived or nested types should not duplicate allocator state:

- `Str` follows `Vec`
- `Int` follows `BitVec`
- `Float` follows `Int`

This keeps allocator ownership aligned with the objects that actually manage memory.

## Allocator Policy Versus API Policy

The allocator and the API layer have different jobs.

The allocator decides how hard it tries:

- allocate once
- retry
- use fallback strategies
- abort internally if configured that way

The API layer decides whether failure is propagated or treated as must-succeed.

In other words, this is a two-axis model:

- effort axis: how hard the allocator tries before giving up
- termination axis: whether the public API propagates failure or treats it as must-succeed

That means even when allocators have aggressive retry behavior, the public operation names still matter:

- `VecInsert...` should let failure bubble up
- `VecMustInsert...` should terminate when failure reaches that boundary

This separation is important because it lets the caller decide where the final abort boundary belongs.

## Copy Callbacks Must Become Allocator-Aware

The deep-copy hooks also need to change.

Right now, generic copy callbacks do not know which allocator they are supposed to use. That becomes a problem once containers can own objects under different memory policies.

The planned direction is:

- copy-init callbacks receive allocator context
- copy-deinit callbacks likely receive allocator context as well

The reason is straightforward: a destination container should initialize owned copies using its own allocator, not by implicitly inheriting whatever behavior happened to be available in the source object.

## What This Means For Container Internals

The internal helper layer for containers will need to change shape.

Operations such as these must become fallible:

- reserve
- resize when growth is required
- insert
- push-back and push-front
- range insert
- merge
- deep-copy paths

Once those helpers return `bool`, the public surface becomes straightforward:

- propagating macro calls helper and returns `bool`
- `Must...` macro calls the same helper and aborts on `false`

## Current Repository Baseline

This design summary is written against the following repository state:

- repository: `MisraStdC`
- branch: `master`
- commit: `73af67f468d17ce7c97352738049d68fccbccabc`
- tracked upstream at time of writing: `origin/master`

That commit already includes earlier work on:

- making several `Int` and `Float` failure paths recoverable
- adding optional error channels for ambiguous integer-return APIs
- simplifying generic dispatch for `Int` and `Float`

The allocator and macro-propagation work described here is the next larger design step, not something already implemented in that commit.

## Expected Direction Of Implementation

The likely implementation path is incremental.

The broad order is:

1. finalize allocator and callback types
2. convert the core container helpers to return failure
3. move typed macro semantics onto those helpers
4. add `Must...` wrappers for aborting behavior
5. roll the allocator binding through root memory-owning objects
6. update docs and tests around the new failure model

That order matters because the new allocator design is only useful if the container APIs can actually propagate failure.

## Closing View

This refactor is not mainly about adding features. It is about tightening the shape of the library.

The intended end state is:

- caller bugs still die loudly
- runtime failures can propagate
- typed generic macros stay ergonomic
- ownership transfer becomes easier to reason about on success and on failure
- allocator policy becomes an explicit part of object initialization

In short, MisraStdC keeps its existing bias toward strictness, but becomes stricter in the right place.
