---
title: "Extending IO with User-Defined Types"
date: 2026-06-03
description: "Plug your own types into WriteFmt / StrReadFmt / FReadFmt through the IOFMT_USER_CASE_ extension hook, with no call-site wrapper and full compile-time type safety."
authors:
  - siddharth-mishra
tags:
  - io
  - formatting
  - generics
  - extension
---

The formatted-IO family in MisraStdC (`WriteFmt`, `StrAppendFmt`, `FReadFmt`, `StrReadFmt`, and friends) dispatches each argument through a `_Generic` switch keyed on the C type. That switch lives inside the `IOFMT(x)` macro in `<Misra/Std/Io.h>` and has one arm per supported type.

For built-ins (`i32`, `f64`, `Str`, `Zstr`, `BitVec`, `Int`, `Float`, …) those arms ship with the header. For your own types you would otherwise have to wrap every call site:

```c
WriteFmt("got {}", IO_WRAP(my_widget));  // not what we want
```

`IOFMT_USER_CASE_` removes that wrapper. After a one-time hook in your project header, every call site reads exactly the same as if the type were built in:

```c
WriteFmt("got {}", my_widget);           // dispatches through your writer
```

Dispatch stays compile-time, type-safe (unknown types fail to match), and zero-overhead beyond a normal indirect call.

## A Minimal Example

The TU layout has four blocks, in this order:

```c
// 1. Headers that introduce your type's dependencies and the iterator
//    helpers your reader will use. NOTHING that transitively pulls in
//    <Misra/Std/Io.h> belongs here -- if Io.h's empty-fallback for
//    IOFMT_USER_CASE_ runs before our override, the override silently
//    loses (the redefinition warning is the friendly outcome).
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Utility/StrIter.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Types.h>

// 2. The type itself.
typedef struct {
    i32 x;
    i32 y;
} Point2D;

// 3. The extension hook -- one arm per user type, comma-terminated.
//    MUST be defined before <Misra/Std/Io.h> is processed.
#define IOFMT_USER_CASE_(x, addr) \
    Point2D : TO_TYPE_SPECIFIC_IO(Point2D, addr),

// 4. The IO header (and any Io.h-pulling headers such as Log.h), then
//    your writer / reader definitions.
#include <Misra/Std/Io.h>
#include <Misra/Std/Log.h>

bool _write_Point2D(Str *o, FmtInfo *info, Point2D *p) {
    (void)info;
    return StrAppendFmt(o, "({}, {})", p->x, p->y);
}

// Readers walk the input through a StrIter rather than bare pointer
// arithmetic -- matches the in-tree convention (`_read_u8`, `_read_Str`)
// and means whitespace / delimiter handling can lean on the peek-then-
// Must idiom. When delegating to a built-in reader like `_read_i32`,
// hand it `StrIterDataAt(&si, StrIterIndex(&si))` and rebuild the
// iterator from the Zstr it returns.
Zstr _read_Point2D(Zstr i, FmtInfo *info, Point2D *p) {
    (void)info;
    StrIter si = StrIterFromZstr(i);
    char    c  = 0;

    while (StrIterPeek(&si, &c) && IS_SPACE(c)) StrIterMustNext(&si);
    if (!StrIterPeek(&si, &c) || c != '(') {
        return StrIterDataAt(&si, StrIterIndex(&si));
    }
    StrIterMustNext(&si);

    FmtInfo inner = {0};
    Zstr    rest  = _read_i32(StrIterDataAt(&si, StrIterIndex(&si)),
                              &inner, &p->x);

    si = StrIterFromZstr(rest);
    while (StrIterPeek(&si, &c) && (IS_SPACE(c) || c == ',')) {
        StrIterMustNext(&si);
    }
    rest = _read_i32(StrIterDataAt(&si, StrIterIndex(&si)),
                     &inner, &p->y);

    si = StrIterFromZstr(rest);
    while (StrIterPeek(&si, &c) && IS_SPACE(c)) StrIterMustNext(&si);
    if (StrIterPeek(&si, &c) && c == ')') StrIterMustNext(&si);
    return StrIterDataAt(&si, StrIterIndex(&si));
}
```

That is the entire surface. You can now write:

```c
Point2D p = {.x = 3, .y = 4};
WriteFmt("position = {}\n", p);          // "position = (3, 4)"

Zstr in = "(42, -9)";
Point2D q = {0};
StrReadFmt(in, "{}", q);                 // q == {42, -9}
```

…and mix user types with built-ins in the same call:

```c
i32 count = 7;
WriteFmt("got {} hits at {}\n", count, p);
```

A working version of this example lives at `Tests/Std/Io.UserTypes.c`.

## The Contract

Two function-symbol names per user type, both at file scope:

```c
bool _write_T(Str *o, FmtInfo *info, T *self);
Zstr _read_T (Zstr i, FmtInfo *info, T *self);
```

| Field | Meaning |
| --- | --- |
| `Str *o` | Output sink for the writer. Append with `StrAppendFmt(o, …)`, `StrAppend(o, …)`, etc. |
| `Zstr i` | Input cursor for the reader. Wrap with `StrIterFromZstr(i)` and walk it through `StrIterPeek` / `StrIterMustNext`; return the post-parse position via `StrIterDataAt(&si, StrIterIndex(&si))`. |
| `FmtInfo *info` | Parsed `{...}` spec (width, precision, flags). Ignore with `(void)info;` if you do not care. |
| `T *self` | Pointer to the user value. The IOFMT machinery passes `&val` of the original argument. |
| **Writer return** | `true` on success, `false` on allocation failure (the IO pipeline propagates). |
| **Reader return** | New cursor position into the original input (typically `i` advanced past the consumed bytes). |

Both functions are bound by token-pasting -- the macro `TO_TYPE_SPECIFIC_IO(T, addr)` expands to `_write_T` / `_read_T` symbol references. The names are not negotiable; the contract is the underscore-prefixed token-paste.

If you only need printing, define `_read_T` as a stub that returns `i` unchanged. Skipping the symbol entirely will trip a linker error at the first call site.

## Include-Order Rule

`IOFMT_USER_CASE_` works because of this fragment near the top of `<Misra/Std/Io.h>`:

```c
#ifndef IOFMT_USER_CASE_
#    define IOFMT_USER_CASE_(x, addr) /* empty -- override before include */
#endif
```

If you `#include <Misra/Std/Io.h>` *before* defining the hook, the empty fallback wins and your types never reach the `_Generic` switch. Symptoms:

- `WriteFmt("{}", my_widget);` fails to compile with *"controlling expression type not compatible with any generic association"* -- this is the safe failure mode.
- Or, if a built-in arm coincidentally matches (e.g. you wrap a single `i32`), the wrong writer runs silently.

Practical rule: put the `#define IOFMT_USER_CASE_(...)` line in **a single project-wide header**, include nothing else in that header except what your types' declarations need, and include it before any other Misra IO surface in every TU that formats your types.

## Forward Declarations Inside Writer Bodies

If your writer calls `StrAppendFmt`, `WriteFmt`, etc. with arguments of its own user type *or any other type covered by `IOFMT_USER_CASE_`*, the `_Generic` arms inside that nested expansion need every user `_write_T` / `_read_T` symbol to be visible already. Two patterns work:

```c
// Pattern A -- forward-declare both functions before either body.
bool _write_Point2D(Str *o, FmtInfo *info, Point2D *p);
Zstr _read_Point2D (Zstr i, FmtInfo *info, Point2D *p);

bool _write_Point2D(Str *o, FmtInfo *info, Point2D *p) { … }
Zstr _read_Point2D (Zstr i, FmtInfo *info, Point2D *p) { … }
```

```c
// Pattern B -- define readers first (they rarely re-enter IOFMT), then writers.
Zstr _read_Point2D(Zstr i, FmtInfo *info, Point2D *p) { … }
bool _write_Point2D(Str *o, FmtInfo *info, Point2D *p) { … }
```

Pattern A is the safe default once you have more than one user type.

## Recommended Layout: One `Io.h` / `Io.c` Pair per Project

The mechanism above is per-TU, but the natural unit of organisation is the
project: every TU that formats your types needs the same `IOFMT_USER_CASE_`
list visible, every `_write_T` / `_read_T` symbol needs to be linkable from
those TUs, and every user type in the list needs its struct declaration in
scope so the `_Generic` arm can mention it.

The cleanest way to satisfy all three constraints is to keep your IO
extension in a single project-internal pair:

```
MyApp/Io.h    -- forward-declares every user type's _write_T / _read_T,
                 defines IOFMT_USER_CASE_, then #include <Misra/Std/Io.h>
MyApp/Io.c    -- defines the bodies for every _write_T / _read_T
```

Every TU in your project then has exactly one extra include:

```c
#include "MyApp/Io.h"   // pulls in everything: types, hook, Misra/Std/Io.h
```

You can absolutely split the writers/readers across multiple `.c` files, or
keep them next to each type's own module, and it will compile and link.
But maintenance gets harder fast:

- **Macro drift.** Each TU that calls `WriteFmt(..., my_t)` needs
  `IOFMT_USER_CASE_` defined with `my_t`'s arm. If one TU's hook lags
  behind, that TU silently fails to match (compile error if you're lucky,
  wrong-arm dispatch if a built-in coincidentally matches the type).
- **Forward-declaration sprawl.** Pattern A (forward-declare every
  user-type writer/reader before any body) becomes "every TU has to
  forward-declare every user type's writer/reader" once nested writers
  cross TU boundaries. Centralising the forward decls in one header
  removes the bookkeeping.
- **Include-order auditing.** The "define hook before `<Misra/Std/Io.h>`"
  rule is easy to violate when the IO header is pulled transitively. A
  single project IO header that wraps the rule once eliminates the audit
  surface entirely.

If you do split, treat `MyApp/Io.h` as authoritative for the hook + forward
decls and treat the per-module `.c` files purely as homes for the bodies.
That keeps the macro discipline in one place and lets the bodies live next
to their data.

## Composing Across Libraries

`IOFMT_USER_CASE_` is a single preprocessor symbol. If two libraries both want to publish IO-able types, the consumer's chain has to thread them together. The usual idiom is *rename-then-extend*:

```c
// LibB/Io.h
#include "LibA/Io.h"          // already defined IOFMT_USER_CASE_

#define IOFMT_USER_CASE_PREV_ IOFMT_USER_CASE_
#undef  IOFMT_USER_CASE_
#define IOFMT_USER_CASE_(x, addr)            \
    IOFMT_USER_CASE_PREV_(x, addr)           \
    LibBType : TO_TYPE_SPECIFIC_IO(LibBType, addr),

#include <Misra/Std/Io.h>
```

Each library header in the chain accepts whatever the previous one defined and extends it. The final `<Misra/Std/Io.h>` include sees the union of all arms. Order of inclusion matters only insofar as the consumer chooses which library's types are visible at each point.

## What You Lose vs Built-Ins

| Aspect | Built-in (`i32`, `Str`, …) | User type via `IOFMT_USER_CASE_` |
| --- | --- | --- |
| Call-site syntax | `WriteFmt("{}", x)` | `WriteFmt("{}", x)` (identical) |
| Type checking | Compile-time, no `default` arm | Same |
| Dispatch cost | One indirect call | One indirect call |
| Cross-TU dispatch | N/A (single switch) | Works as long as the hook is visible in every TU |
| Spec parsing (`{:.2}` etc.) | Honored by the runtime, applied by the writer | Same `FmtInfo` reaches your writer; honoring it is your responsibility |
| Type erasure | None | `void *` cast inside the function entry; the macro ensures the cast is correct by construction |

The single cost is that the writer / reader bodies receive `void *` data (the function-pointer cast in `TO_TYPE_SPECIFIC_IO_IMPL` widens to `void *` for storage and the call site re-narrows). The narrowing back to `T *` is type-safe because the `_Generic` arm and the symbol name are bound at the same point: the only way for the wrong pointer to reach `_write_T` is if you mis-spell the type tag in `IOFMT_USER_CASE_`, which the compiler would reject anyway.

## Known Limitations

- **One hook per TU.** Multiple `#define IOFMT_USER_CASE_` in the same TU is a redefinition warning unless you use the rename-extend pattern above.
- **No spec-string dispatch.** `WriteFmt("{Widget:.2}", x)` does not look up "Widget" anywhere; the type tag lives at the C-type level. Your writer interprets the spec freely via `FmtInfo`.
- **Header-only.** The hook controls macro expansion; you cannot register a type from a `.c` file alone -- consumers' TUs need the macro visible.
- **No partial implementations.** Even a write-only user type must have a `_read_T` symbol (a trivial stub that returns `i` is fine). The constraint is symbol resolution, not call coverage.

## See Also

- [`Generic Containers and Ownership`](/guides/generic-containers-and-ownership/) -- for background on the `_Generic`-based dispatch patterns used elsewhere in the library.
- `Tests/Std/Io.UserTypes.c` -- working end-to-end example covering write, read, mixed-arg, and round-trip.
- `Include/Misra/Std/Io.h` -- the `IOFMT(x)` macro and the `TO_TYPE_SPECIFIC_IO` / `TypeSpecificIO` definitions.
