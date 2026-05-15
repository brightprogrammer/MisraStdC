# MisraStdC

[![Linux Build](https://github.com/brightprogrammer/MisraStdC/actions/workflows/test-linux.yml/badge.svg?branch=master)](https://github.com/brightprogrammer/MisraStdC/actions/workflows/test-linux.yml)
[![macOS Build](https://github.com/brightprogrammer/MisraStdC/actions/workflows/test-macos.yml/badge.svg?branch=master)](https://github.com/brightprogrammer/MisraStdC/actions/workflows/test-macos.yml)
[![Windows Build](https://github.com/brightprogrammer/MisraStdC/actions/workflows/test-windows-msvc.yml/badge.svg?branch=master)](https://github.com/brightprogrammer/MisraStdC/actions/workflows/test-windows-msvc.yml)
[![Fuzzing](https://github.com/brightprogrammer/MisraStdC/actions/workflows/fuzz.yml/badge.svg?branch=master)](https://github.com/brightprogrammer/MisraStdC/actions/workflows/fuzz.yml)

A modern C11 library that borrows the parts of Rust, Zig, C++, and Python that
actually pay off, and stitches them into plain C without bringing along a
runtime, code generator, or template compiler. Generic containers, formatted
I/O, arbitrary-precision arithmetic, JSON and key-value parsing, cross-platform
system utilities — all opt-out via build-time feature flags so you compile only
what you use.

> **Disclaimer:** This library is **not** related to the MISRA C standard or
> guidelines. The name comes from the author's surname, Siddharth Mishra,
> nicknamed "Misra" among friends.

---

## Table of Contents

- [The Perspective](#the-perspective)
- [A Quick Taste](#a-quick-taste)
- [What You Get](#what-you-get)
- [Build and Install](#build-and-install)
- [Feature Flags](#feature-flags)
- [Six Core Ideas](#six-core-ideas)
  - [1. Allocators are user-owned values, never global state](#1-allocators-are-user-owned-values-never-global-state)
  - [2. `Scope` is lexical RAII, in plain C](#2-scope-is-lexical-raii-in-plain-c)
  - [3. Every object carries a magic; type-confusion dies at the dispatch](#3-every-object-carries-a-magic-type-confusion-dies-at-the-dispatch)
  - [4. Macros + `_Generic` give you generics without a template compiler](#4-macros--_generic-give-you-generics-without-a-template-compiler)
  - [5. Fallible by default, `Must`-variants for the unrecoverable](#5-fallible-by-default-must-variants-for-the-unrecoverable)
  - [6. One header, configured at build time](#6-one-header-configured-at-build-time)
- [Container Tour](#container-tour)
  - [Vec](#vec)
  - [Str](#str)
  - [List, Graph](#list-graph)
  - [BitVec, Int, Float](#bitvec-int-float)
- [Formatted I/O](#formatted-io)
- [Parsers](#parsers)
- [System Utilities](#system-utilities)
- [Contributing](#contributing)
- [License](#license)

---

## The Perspective

C is the language closest to the hardware, but its standard library shows its
age. The library APIs have hidden globals, mute allocator failures, throw away
length information, and expect the programmer to keep track of every lifetime
by hand. The ergonomic answers other languages settled on — borrow checkers,
scope-bound destructors, generics, explicit allocators, fallible-by-default
APIs — got there for a reason.

MisraStdC takes those ideas and asks: how much of each works in C11 without
adding a runtime? It turns out a lot.

- **From Rust:** explicit allocators threaded through every constructor, lexical
  scope-bound lifetimes, fallible-by-default APIs, structured format strings.
- **From Zig:** allocators as plain values, comptime-style type dispatch
  (`_Generic`), feature flags that flip whole subsystems on and off.
- **From C++:** RAII patterns — adapted to C's preprocessor, with one honest
  C-level caveat about `return` / `goto`.
- **From Python:** a single import (`#include <Misra.h>`) is enough.

Nothing in this library hides at runtime. Every macro expands to inlined struct
literals or named runtime helpers you can step through. Every allocator is a
plain stack-allocated struct, no `void *state` indirection, no hidden globals.
If the compiler can't see the allocator it's about to use, that's a compile
error. If a type lookup goes wrong, that's a `LOG_FATAL` from a magic check at
the dispatch boundary, not a corrupted heap three function calls later.

---

## A Quick Taste

```c
#include <Misra.h>

int main(void) {
    Scope(alloc, DefaultAllocator) {
        typedef Vec(int) IntVec;

        IntVec primes = VecInit();           // bound to MisraScope
        int    values[] = {2, 3, 5, 7, 11, 13};
        VecMustInsertRangeR(&primes, values, 0, 6);

        VecForeachIdx(&primes, p, idx) {
            WriteFmtLn("primes[{}] = {}", idx, p);
        }

        VecDeinit(&primes);
    }   // allocator destroyed automatically
}
```

That snippet uses one allocator (`DefaultAllocator` = a binned per-descriptor
heap built on top of `PageAllocator`), one generic container, type-safe
formatted I/O, and an iteration macro. No `malloc`, no `printf`, no global
state. Every object is on the stack except `primes.data`, which is reclaimed
when the `Scope` ends.

---

## What You Get

- **Five concrete allocators**, all user-owned, all per-descriptor:
  `HeapAllocator` (binned), `PageAllocator` (raw OS pages), `ArenaAllocator`
  (bump), `SlabAllocator` (growable fixed-slot pool), and `BudgetAllocator`
  (caller-buffer, fixed-budget, no-growth).
- **Generic containers** built on a shared runtime: `Vec(T)`, `List(T)`,
  `Map(K,V)`, `Graph(T)`, `BitVec`, `Str` (= `Vec(char)`), `Strs` (= `Vec(Str)`).
- **Arbitrary-precision arithmetic:** `Int` on top of `BitVec`, `Float` on top
  of `Int`, with radix-string conversion, modular arithmetic, primality,
  decimal-exact add/sub/mul/div.
- **Type-aware formatted I/O** with Rust-style `{}` placeholders; one
  `WriteFmt(...)` works for `int`, `f64`, `Str`, `Int`, `Float`, `BitVec`,
  C strings — all dispatched at compile time via `_Generic`.
- **JSON and key-value config parsers** as opt-in features.
- **Cross-platform system utilities:** subprocess control, directory listings,
  mutexes, environment access, current-process info, current-executable path.
- **Feature flags** that drop whole subsystems from the static library and
  from the installed header set. Disabling `bitvec`, `list`, `map`, `graph`,
  `int`, `float`, `parser_json`, etc. removes their `.c` files from
  `libmisra_std.a` and their `.h` files from the install prefix.

---

## Build and Install

```bash
git clone --recursive https://github.com/brightprogrammer/MisraStdC.git
cd MisraStdC
meson setup builddir
ninja -C builddir
ninja -C builddir test         # run the test suite
ninja -C builddir install      # install to the configured prefix
```

For development with sanitizers (the default test build):

```bash
meson setup builddir -Db_sanitize=address,undefined -Db_lundef=false
```

### A minimal build

If you only need `Vec`, `Str`, `Io`, and the default heap allocator, turn the
rest off:

```bash
meson setup builddir-min \
    -Dalloc_arena=false -Dalloc_slab=false -Dalloc_budget=false \
    -Dbitvec=false -Dlist=false -Dmap=false -Dgraph=false \
    -Dint=false -Dfloat=false \
    -Dfile=false -Diter=false \
    -Dsys_dir=false -Dsys_proc=false \
    -Dparser_json=false -Dparser_kvconfig=false
ninja -C builddir-min
ninja -C builddir-min install
```

The resulting `libmisra_std.a` contains only the ten foundation translation
units, and the install prefix contains only forty headers — about a third of
the default build. Adding `int` automatically pulls in `bitvec`; adding
`graph` pulls in `vec` (already foundation); adding `parser_kvconfig` pulls in
`map`. Dependencies between features resolve transitively at configure time.

---

## Feature Flags

| Flag                  | Adds                                                     | Auto-pulls       |
|-----------------------|----------------------------------------------------------|------------------|
| `alloc_arena`         | `ArenaAllocator`                                         | —                |
| `alloc_slab`          | `SlabAllocator` (growable fixed-slot pool)               | —                |
| `alloc_budget`        | `BudgetAllocator` (caller-buffer, fixed-budget)          | —                |
| `bitvec`              | `BitVec` packed bit container                            | —                |
| `list`                | `List(T)` doubly-linked list                             | —                |
| `map`                 | `Map(K,V)` hash map                                      | —                |
| `graph`               | `Graph(T)` directed graph (uses `Vec` runtime helpers)   | —                |
| `int`                 | Arbitrary-precision integer `Int`                        | `bitvec`         |
| `float`               | Arbitrary-precision decimal `Float`                      | `int → bitvec`   |
| `file`                | `ReadCompleteFile(...)` and file helpers                 | —                |
| `iter`                | Generic `Iter(T)` iteration helpers                      | —                |
| `sys_dir`             | `SysGetDirContents(...)` and friends                     | —                |
| `sys_proc`            | `SysProcCreate(...)` / spawn / wait                      | —                |
| `parser_json`         | JSON read/write                                          | —                |
| `parser_kvconfig`     | Key-value config parser                                  | `map`            |

Every enabled feature also defines `MISRA_HAVE_<NAME>` (= 1) in the
generated `Misra/Config.h`. User code can `#if MISRA_HAVE_BITVEC` to compile
against a partial install.

The foundation — always built, can't be opted out — is:
`Sys`, `Sys/Mutex`, `Std/Log`, `Std/Memory`, `Std/Allocator` (core + Page +
Heap = `DefaultAllocator`), `Std/Container/Vec`, `Std/Container/Str`,
`Std/Io`. `LOG_FATAL` formats its message through `Str` + `Io`, so those
two are foundation by transitive necessity.

### What it actually costs

The feature-flag system is not theatre — disabled features really do leave
the static library. Measured with `meson setup --buildtype=minsize
-Db_sanitize=none` (gcc 15.2, x86_64, Linux):

| Configuration                                                       | `libmisra_std.a` |
|---------------------------------------------------------------------|------------------|
| Foundation only                                                     | **491 KB**       |
| `+ bitvec`, `+ int`, `+ float`                                      | 967 KB           |
| `+ list`, `+ map`, `+ graph`, `+ iter`                              | 1 264 KB         |
| `+ file`, `+ sys_dir`, `+ sys_proc`, `+ alloc_arena/slab/budget`    | 1 415 KB         |
| `+ parser_json`                                                     | 1 481 KB         |
| Default (everything; `+ parser_kvconfig`)                           | **1 529 KB**     |

The smallest viable build is **roughly a third** of the full build. The
foundation alone covers a meaningful amount of work — `Vec`, `Str`, formatted
I/O, the default heap allocator, `Scope`, the magic-validated dispatch
machinery, and the OS abstractions for files, mutexes, and process
introspection. Everything past that is opt-in: bring in `bitvec/int/float`
when you need arbitrary precision, `list/map/graph` when you need richer
containers, parsers when you need to read external input, the optional
allocator backends when you have an unusual lifetime story to model.

These are static-archive sizes, larger than what actually lands in your final
binary — the linker pulls in only the `.o` files your code references. The
archive itself is what gets compiled and installed; the cost to your
executable is bounded above by the archive size and usually a lot smaller.

---

## Six Core Ideas

### 1. Allocators are user-owned values, never global state

Every dynamically-sized object in MisraStdC takes an `Allocator *` at init
time and uses it for every subsequent allocation. The library owns no
process-wide heap, no thread-local fallback, no shared "default" instance.

A typed allocator is a struct with state inline:

```c
DefaultAllocator alloc = DefaultAllocatorInit();   // entirely on the stack
Vec(int) v = VecInit(&alloc);
...
VecDeinit(&v);
DefaultAllocatorDeinit(&alloc);
```

The library ships five backends:

- **`PageAllocator`** — raw `mmap` / `VirtualAlloc`. The foundation under every
  other allocator, no libc heap.
- **`HeapAllocator`** — power-of-two size-class bins (16–2048 B) plus a
  page-passthrough for large allocations. Each `HeapAllocator` value carries
  its own bins; two of them on the stack never share memory. `DefaultAllocator`
  is a typedef for this.
- **`ArenaAllocator`** — bump cursor over page-backed chunks. `AllocatorFree`
  is a no-op; everything is released together on `ArenaAllocatorDeinit`. Best
  fit for parsers and per-request scratch.
- **`SlabAllocator`** — fixed-size slots with an intrusive free list, grows by
  pulling more page-backed slabs on demand.
- **`BudgetAllocator`** — caller hands in a fixed memory region at init; slots
  are carved out of it and never replenished. Suitable for embedded and
  freestanding contexts.

The library defines a small `_Generic` whitelist (`ALLOCATOR_OF`) so any of
these can pass anywhere an `Allocator *` is expected.

### 2. `Scope` is lexical RAII, in plain C

Manually pairing `*AllocatorInit()` and `*AllocatorDeinit(&...)` at every
exit point is the kind of bookkeeping nobody enjoys. `Scope` is a macro
that turns a block of code into an allocator lifetime:

```c
Scope(alloc, DefaultAllocator) {
    Vec(int) v = VecInit();        // bound to MisraScope (the internal pool)
    Vec(int) w = VecInit(alloc);   // bound to the named user pool
    Str line = StrInitFromZstr("hello");
    ...
    VecDeinit(&v);
    VecDeinit(&w);
    StrDeinit(&line);
}   // both allocators destroyed automatically
```

`Scope(name, AllocType)` introduces two stack-resident typed allocators:

- `name` — the user-visible pool. Pass it to helpers explicitly when you want
  allocations to land in your named slot.
- `MisraScope` — an internal pool that every zero-argument `*Init()` macro
  picks up implicitly. Library scratch and your named allocations stay
  separate by default.

When control leaves the block, both pools are destroyed.

`ScopeWith(alloc)` is the helper-side counterpart: borrow a caller-owned
`Allocator *` and expose it as `MisraScope` inside the block, without taking
ownership. `ExitScope` is an alias for `break` and runs the cleanup.

**The one C-level caveat:** `return` and `goto` that leave a `Scope` skip the
cleanup. There's no portable workaround in C (GCC/Clang's
`__attribute__((cleanup))` works but MSVC has nothing equivalent). Use
`ExitScope` to break out cleanly before returning.

### 3. Every object carries a magic; type-confusion dies at the dispatch

Every container (`Vec`, `Str`, `BitVec`, `List`, `Map`, `Graph`, `Int`,
`Float`) and every typed allocator carries an 8-byte magic value stamped at
init time. Each runtime helper validates the magic on entry. The cost is a
single 64-bit comparison; the upside is that:

- Passing an uninitialized object (`Vec v = {0}; VecPush(&v, 1);`) aborts
  with a clear `LOG_FATAL` rather than walking through garbage pointers.
- Reinterpreting one typed allocator as another (`HeapAllocator *` → `ArenaAllocator *`)
  aborts at the first dispatch instead of corrupting bins.
- Heap-spray and use-after-free patterns trip the validator long before they
  reach `mmap`-mapped pages.

Each allocator type has its own magic constant
(`MISRA_HEAP_ALLOCATOR_MAGIC`, `MISRA_PAGE_ALLOCATOR_MAGIC`, ...). Adding a
new typed allocator means defining its magic and adding it to the
`ALLOCATOR_OF` `_Generic` whitelist.

### 4. Macros + `_Generic` give you generics without a template compiler

There is no separate code-generation step. The generic shape comes from C11
macros:

- `Vec(T)`, `List(T)`, `Graph(T)`, `Map(K, V)`, `Pair(xT, yT)`, `Iter(T)`
  expand to anonymous structs. Distinct expansions are distinct types — wrap
  with a `typedef` if you want to reuse the type.
- Operations like `VecInsertR`, `VecAt`, `MapGet`, `GraphAddNodeR`, `IntAdd`,
  `FloatFrom` dispatch on the source value's type at the macro layer
  (`_Generic`) and forward to shared runtime helpers in `Source/`. Type
  information that can't be inferred is carried through `sizeof(T)` and
  `__typeof__`.
- The macros are designed to be expression-shaped where they return values
  (so you can branch on `VecInsertL(...)`) and statement-shaped where they
  encode flow control (`VecForeach`).

This means `Vec(int)` and `Vec(struct Point)` share the same runtime code
but get distinct compile-time types. No header explosion, no separately
compiled template instantiations.

### 5. Fallible by default, `Must`-variants for the unrecoverable

The library splits its public API into two parallel forms so each caller
decides where to draw the abort boundary:

- **Plain form** — propagating fallible API. Returns `bool` (or a sentinel
  like `GraphNodeId == 0`). The container is left unchanged on failure so
  the caller can retry or bubble the error up.
  ```c
  if (!VecInsertL(&v, item, 0)) {
      // recover, retry, or bubble up
  }
  ```
- **`Must` variant** — statement-style `do { ... } while (0)` wrapper that
  calls `LOG_FATAL` on failure. Use these at API boundaries where allocation
  failure isn't recoverable.
  ```c
  VecMustInsertL(&v, item, 0);   // aborts via LOG_FATAL on failure
  ```

Programmer errors — NULL where a pointer is required, out-of-range indices,
use of an uninitialized container — always abort, regardless of which form
you call. That's the magic check from idea #3 doing its job.

### 6. One header, configured at build time

```c
#include <Misra.h>
```

is enough. `Misra.h` is an umbrella that recursively pulls in every module
the current build enabled, via `#if MISRA_HAVE_<NAME>` checks against the
generated `Misra/Config.h`. If you disabled `parser_json` at configure time,
`<Misra/Parsers/JSON.h>` is neither installed nor pulled in by `Misra.h` —
but everything else is reachable through that one include.

You can still include sub-umbrellas (`<Misra/Std/Container.h>`,
`<Misra/Sys.h>`) when you want a narrower preprocessor cost, but you never
have to.

---

## Container Tour

All examples below assume you have already included `<Misra.h>`.

### Vec

```c
typedef Vec(int) IntVec;

int compare_ints(const void *a, const void *b) {
    return *(const int *)a - *(const int *)b;
}

Scope(alloc, DefaultAllocator) {
    IntVec numbers = VecInit();
    VecMustReserve(&numbers, 10);

    // Insert by R-value (copy) and L-value (ownership transfer).
    int val = 42;
    VecMustInsertL(&numbers, val, 0);    // `val` is now owned by `numbers`
    VecMustInsertR(&numbers, 10, 0);     // copy semantics, insert at front
    VecMustInsertR(&numbers, 30, 1);

    int items[] = {15, 25, 35};
    VecMustInsertRangeR(&numbers, items, VecLen(&numbers), 3);
    VecSort(&numbers, compare_ints);

    VecForeachIdx(&numbers, current, idx) {
        WriteFmtLn("[{}] = {}", idx, current);
    }
    VecForeachPtr(&numbers, p) {
        *p *= 2;
    }

    VecTryReduceSpace(&numbers);
    VecDeleteRange(&numbers, 1, 2);
    VecDeinit(&numbers);
}
```

Two insertion styles, intentional:

- **`...L` (l-value):** transfers ownership. If the container doesn't have a
  deep-copy callback, the source l-value is zeroed after insertion. Use for
  values built in a temporary that the container should now own.
- **`...R` (r-value):** plain by-value insertion. No ownership claim, no
  zeroing.

The split makes ownership transfers visible at call sites instead of buried
in convention.

### Str

`Str` is a typedef for `Vec(char)` with a null terminator maintained at
`data[length]`. Same runtime, but a richer set of macros for text:

```c
Scope(alloc, DefaultAllocator) {
    Str text  = StrInit();
    Str hello = StrInitFromZstr("Hello");
    Str world = StrInitFromCstr(", World!", 8);

    StrWriteFmt(&text, "{}{}\n", hello, world);

    bool starts = StrStartsWithZstr(&text, "Hello");
    bool ends   = StrEndsWithZstr(&text, "!\n");

    Str  csv   = StrInitFromZstr("one,two,three");
    Strs parts = StrSplit(&csv, ",");
    VecForeach(&parts, part) {
        WriteFmtLn("part: {}", part);
    }

    StrDeinit(&text); StrDeinit(&hello); StrDeinit(&world); StrDeinit(&csv);
    VecForeachPtr(&parts, part) { StrDeinit(part); }
    VecDeinit(&parts);
}
```

### List, Graph

```c
typedef List(int)  IntList;
typedef Graph(Str) NameGraph;

Scope(alloc, DefaultAllocator) {
    IntList ll = ListInit();
    ListMustPushBack(&ll, 10);
    ListMustPushBack(&ll, 20);
    ListMustPushFront(&ll, 5);
    ListForeach(&ll, n, i) {
        WriteFmtLn("ll[{}] = {}", i, n);
    }
    ListDeinit(&ll);

    // Graph with owned string node names.
    NameGraph graph = GraphInitWithDeepCopy(NULL, StrDeinit);
    GraphNodeId alpha = GraphAddNodeR(&graph, StrZ("Alpha"));
    GraphNodeId beta  = GraphAddNodeR(&graph, StrZ("Beta"));
    GraphAddEdge(&graph, alpha, beta);

    GraphForeachNode(&graph, node) {
        WriteFmtLn("{}: out={}, in={}",
                   GraphNodeData(&graph, node),
                   GraphOutDegree(&graph, GraphNodeGetId(node)),
                   GraphInDegree(&graph, GraphNodeGetId(node)));
    }
    GraphDeinit(&graph);
}
```

`Graph(T)` is built for analysis workloads: reachability, control flow,
dependency traversal. For graph-owned strings, prefer `Graph(Str)` plus
`GraphInitWithDeepCopy(NULL, StrDeinit)` and insert with
`GraphAddNodeR(..., StrZ("..."))` so the graph deep-copies and reclaims on
deinit. `Graph(const char *)` works too, but only when every stored pointer
outlives the graph (string literals, interned names).

`Map(K, V)` follows the same pattern; the user supplies a key hash
function and a key compare function at init time, see
`Misra/Std/Container/Map/Init.h` for the available constructors.

### BitVec, Int, Float

```c
Scope(alloc, DefaultAllocator) {
    // BitVec
    BitVec flags = BitVecFromStr("10110", alloc);
    BitVecPush(&flags, true);
    Str bin = BitVecToStr(&flags);
    WriteFmtLn("flags = 0b{}", bin);
    StrDeinit(&bin);
    BitVecDeinit(&flags);

    // Int — arbitrary precision.
    Int big     = IntFromHexStr("deadbeefcafe", alloc);
    Int squared = IntInit();
    IntMul(&squared, &big, &big);
    WriteFmtLn("big = 0x{x}, big^2 = 0x{x}", big, squared);
    IntDeinit(&big);
    IntDeinit(&squared);

    // Float — arbitrary precision, exact decimal.
    Float pi = FloatFromStr("3.14159265358979323846", alloc);
    WriteFmtLn("pi = {}", pi);
    WriteFmtLn("pi (10dp) = {.10}", pi);
    FloatDeinit(&pi);
}
```

Construction APIs that pull data in from outside the library (parse a
string, copy a byte buffer, build from a primitive) take an explicit
`Allocator *` parameter. The container has no existing allocator to inherit
from at that moment, so passing one is the only sensible contract.

`Int` operations include `IntAdd`, `IntSub`, `IntMul`, `IntDivMod`, `IntPow`,
`IntGcd`, `IntLcm`, `IntIsPrime`, `IntModPow`, base-2/8/10/16 string
conversion, byte import/export (LE and BE), and bit-level access through the
underlying `BitVec`. `Float` adds sign, decimal exponent, and a precision
parameter for division.

---

## Formatted I/O

The placeholder syntax is `{}` or `{[alignment][width][.precision][flags]}`.
Type dispatch is compile-time via `_Generic`, so one call works for every
supported argument type.

```c
WriteFmtLn("Hello, {}! count={}, pi={.4}", name, 42, 3.14159);
```

### What types work

The macros dispatch through `IOFMT(...)` which has `_Generic` cases for
`Str`, `Int`, `Float`, `BitVec`, `const char *`, `char *`, primitive integer
and floating-point types, and `char`. Anything else falls through to an
unsupported-type handler.

The catch: array types (`char[6]`, `const char[10]`) are distinct from
pointer types under `_Generic`. Bind string literals to a pointer variable
first.

```c
const char *title = "Mr.";        // good
char        name[] = "Alice";     // bad — array type
StrWriteFmt(&buf, "{}", title);
```

### Format Specifier Options

**Alignment** (in a field width):

| Specifier | Description                            |
|-----------|----------------------------------------|
| `<`       | Left-aligned (pad on right)            |
| `>`       | Right-aligned (pad on left, default)   |
| `^`       | Center-aligned (pad on both sides)     |

**Endianness** (when paired with raw I/O flag `r`):

| Specifier | Description                            |
|-----------|----------------------------------------|
| `<`       | Little Endian                          |
| `>`       | Big Endian (default)                   |
| `^`       | Native Endian                          |

**Type flags:**

| Flag | Description                                  | Example output      |
|------|----------------------------------------------|---------------------|
| `x`  | Hexadecimal lowercase                        | `0xdeadbeef`        |
| `X`  | Hexadecimal uppercase                        | `0xDEADBEEF`        |
| `b`  | Binary                                       | `0b10100101`        |
| `o`  | Octal                                        | `0o777`             |
| `c`  | Character formatting, preserve case          | raw character bytes |
| `a`  | Character formatting, force lowercase        | lowercased          |
| `A`  | Character formatting, force uppercase        | uppercased          |
| `r`  | Raw byte read/write                          | raw bytes           |
| `e`  | Scientific notation lowercase                | `1.235e+02`         |
| `E`  | Scientific notation uppercase                | `1.235E+02`         |
| `s`  | Read a quoted string or single word          | `"hello world"`     |

**Precision** (floating-point):

```c
{.2}    // two decimal places
{.0}    // no decimal places
{.10}   // ten decimal places
```

### Reading values

`StrReadFmt` / `FReadFmt` / `ReadFmt` parse the input cursor and advance it
on success. Pass the cursor as an assignable variable, not a literal:

```c
const char *cursor = "Count: 42, Name: Alice";
i32 count = 0;
Scope(alloc, DefaultAllocator) {
    Str user = StrInit();
    StrReadFmt(cursor, "Count: {}, Name: {}", count, user);
    WriteFmtLn("count = {}, user = {}", count, user);
    StrDeinit(&user);
}
```

### Available entry points

- `StrWriteFmt(&str, fmt, ...)` / `StrReadFmt(cursor, fmt, ...)`
- `FWriteFmt(file, fmt, ...)` / `FWriteFmtLn(...)` / `FReadFmt(file, fmt, ...)`
- `WriteFmt(fmt, ...)` / `WriteFmtLn(fmt, ...)` / `ReadFmt(fmt, ...)`
  (stdout / stdin)

---

## Parsers

JSON read/write is available when `parser_json` is enabled:

```c
typedef struct { float x, y; } Point;

Scope(alloc, DefaultAllocator) {
    Str json = StrInitFromZstr("{\"x\": 10.5, \"y\": 20.0}");
    Point p = {0};

    StrIter si = StrIterFromStr(&json);
    JR_OBJ(si, {
        JR_FLT_KV(si, "x", p.x);
        JR_FLT_KV(si, "y", p.y);
    });

    WriteFmtLn("point = ({}, {})", p.x, p.y);
    StrDeinit(&json);
}
```

KvConfig is a simple `key = value` / `key: value` parser with `#` and `;`
comment support, quoted values, and last-write-wins semantics, available
when `parser_kvconfig` is enabled:

```c
Scope(alloc, DefaultAllocator) {
    Str text = StrInitFromZstr(
        "host = localhost\n"
        "port = 8080\n"
        "debug = true\n"
    );
    KvConfig cfg = KvConfigInit();
    KvConfigParse(StrIterFromStr(&text), &cfg);

    Str host  = KvConfigGet(&cfg, "host");
    i64 port  = 0;
    bool dbg  = false;
    KvConfigGetI64(&cfg, "port", &port);
    KvConfigGetBool(&cfg, "debug", &dbg);
    WriteFmtLn("host={}, port={}, debug={}", host, port, dbg);

    StrDeinit(&host);
    KvConfigDeinit(&cfg);
    StrDeinit(&text);
}
```

---

## System Utilities

Cross-platform wrappers for subprocesses, directories, mutexes, environment
access. The subprocess example demonstrates the explicit-allocator-handoff
pattern: the caller passes the same allocator to `SysProcCreate` and
`SysProcDestroy`.

```c
// Verified with /bin/head: writes a value to the child, expects the same
// echoed back, prints the round-trip result.
int main(int argc, char **argv, char **envp) {
    (void)argc;
    Scope(alloc, DefaultAllocator) {
        SysProc *proc = SysProcCreate(argv[1], argv + 1, envp, alloc);
        SysProcWriteToStdinFmtLn(proc, "value = {}", 42);

        i32 val = 0;
        SysProcReadFromStdoutFmt(proc, "value = {}", val);
        WriteFmtLn("got value = {}", val);

        SysProcWaitFor(proc, 1000);
        SysProcDestroy(proc, alloc);
    }
}
```

---

## Contributing

Contributions are welcome.

Match the existing style in the files you touch. Run the test suite
(`ninja -C builddir test`) and `python Scripts/clang-format.py` before
sending a change. The default build (all features on) is what CI runs;
verify your change works there before opening a PR.

1. Fork the repository.
2. Create a feature branch: `git checkout -b feature/<name>`.
3. Commit with a short imperative subject and a body explaining the *why*.
4. Push: `git push origin feature/<name>`.
5. Open a Pull Request.

---

## License

This project is dedicated to the public domain under the
[Unlicense](LICENSE.md). You may use it, modify it, redistribute it, and
sell it without attribution. See [LICENSE.md](LICENSE.md) for the full
text.
