# Coding Conventions

This document captures the conventions the project follows. It's aimed at
newcomers and contributors who want to understand why the code is shaped the
way it is, before opening a PR. The rules are short by design — read the rest
of the codebase to see them in action.

---

## Naming

- **Private / internal helpers**: `snake_case` (`file_remaining_size`,
  `elf_decode_header`). Static functions, `.c`-local helpers, and runtime
  bodies invoked only by public macros all live here.
- **Public API**: `PascalCase` (`FileOpen`, `BufReadFmt`, `ElfDeinit`).
- **No project prefix on identifiers.** The include path
  (`<Misra/Std/Container/Buf.h>`) already names the namespace; adding
  `Misra` / `MISRA_` / `misra_` to every symbol is noise. The `MISRA_`
  prefix is reserved for three things only: include guards, `FEATURE_*`
  build-config flags surfaced by `meson.build`, and `_MAGIC` struct
  sentinels (e.g. `HEAP_ALLOCATOR_MAGIC`, `VEC_MAGIC`).
- **Short names where the include path already disambiguates.** `Buf`,
  `Str`, `Vec`, `Elf` — not `MisraBuf`, not `ElfFile`.
- **Tool binaries** ship with a single short word as their name
  (see `Bin/Beam.c`, `Bin/Resolve.c`).
- **C-strings in public API surface use `Zstr`**, not `char *` /
  `const char *`. `Zstr` (`<Misra/Std/Zstr.h>`) is the project's name
  for a NUL-terminated C-string parameter. Internal helpers (`static`
  in a `.c` file) may use `char *` / `const char *` directly. In
  `_Generic` dispatch the underlying `char *` and `const char *`
  branches are still listed explicitly, because string literals and
  `const`-returning callers each need their own match.

## API shape

- **`FooInit` is a macro**, not a function. It expands to a designated-
  initializer struct literal (`VecInit`, `StrInit`, `HeapAllocatorInit`).
  It cannot fail. Use `MISRA_OVERLOAD` for arg-count dispatch.
- **`FooDeinit` is a function**, and the only way to release `Foo`.
- **`Init` + `Deinit`, never `Create` / `Destroy`.** Any `Create` /
  `Destroy` pair you see is a leftover; new code should not copy it.
- **Containers carry their `Allocator *` inline.** Everything else takes
  the allocator as an explicit parameter to both `Init` and `Deinit`, so
  lifetime is visible at every call site.
- **Ownership transfer (L-form / move APIs)** take a pointer to the
  caller's payload, snapshot it locally, then zero the caller's view so
  any stale alias dies fast. Two variants in the codebase:
  - L-form on a struct payload (e.g. `Buf` handed to `ElfOpenFromMemory`):
    signature is `T *`; the function ends with `MemSet(p, 0, sizeof *p)`.
  - L-form on a container element (e.g. `VecInsertL(&v, lval, idx)`): the
    macro takes the l-value, the implementation reads its bytes, then the
    macro zeroes the source on success.
  In both cases the zero-on-take invariant holds on success AND failure.
- **R-form (`*FromMemoryCopy` / `VecInsertR`)** copies; the caller's data
  is untouched and remains theirs.

## Allocators

- **Init by value.** No globals, no TLS-as-globals, no hidden default
  allocator behind a function. If you can't see the allocator that's
  about to be used, that's a design smell.
- **No `parent` / `upstream` / `wrapped` allocator fields.** Embed the
  backing allocator inline by value. Proxy allocators are tempting and
  always wrong here.
- **Allocators fail loud.** Bad free, foreign pointer, state-machine
  violation → `LOG_FATAL` with a backtrace. Soft no-op returns hide bugs;
  this project would rather you crash on the spot.
- **Stack-promote transient strings** with `StrInitStack(str, alloc, n,
  body)` and a rough capacity. Don't spin up a fresh `HeapAllocator` to
  hold a short-lived string.

## Libc-free mindset

- This project is a **libc replacement**, not a libc wrapper. Treat
  `<stdio.h>`, `<string.h>`, `<errno.h>`, `<stdlib.h>` as off-limits
  inside the library proper. POSIX (`<sys/mman.h>`, `<unistd.h>`, etc.)
  and Win32 are fine — those are kernel boundaries, not libc.
- The full mapping (which libc header maps to which in-tree replacement)
  lives in `Tests/Util/check_no_libc.py`. That script is the source of
  truth: it runs in test and will fail the build if a libc header sneaks
  back in.
- Before reaching for a libc function, **grep for the in-tree
  equivalent**. Most common ones live in `Misra/Std/{Memory,Zstr,Io}.h`.
  If it's truly missing, write a small replacement rather than including
  the libc header.
- Don't speak libc in design, naming, or comments. No `stderr`, no
  `FILE`, no `errno`, no `fopen` vocabulary leaking into the API surface.

## `_Generic` dispatch

- **Inline `_Generic` directly in each macro.** Never extract a shared
  "extract raw pointer" / "convert type X to type Y" helper. The point
  of the wrapped types is to hide the raw form; pulling it back out via
  a shared helper undoes the design.
- **Type-safe dispatch — list the accepted types explicitly.** Avoid a
  `default:` arm that silently casts a wrong type to the function's
  expected one. Any other input type should trigger a compile-time
  `_Generic` mismatch.
  - Path dispatch: `Str *`, `char *`, `const char *` (note: not
    `const Str *` — see `<Misra/Std/File.h>`, `<Misra/Sys/Dir.h>`,
    `<Misra/Parsers/Elf.h>` for the canonical shape).
  - Container-out dispatch: `Buf *`, `Str *`.

  Skeleton:
  ```c
  #define FileGetSize(path)                              \
      _Generic((path),                                   \
          Str *:        file_get_size(((Str *)(path))->data), \
          char *:       file_get_size((const char *)(path)), \
          const char *: file_get_size((const char *)(path)))
  ```
- **Don't reach into a wrapped type's fields from outside its `.c`
  file.** Go through the public accessor macros: `BufLength` / `BufData`
  / `BufAllocator` (`<Misra/Std/Container/Buf.h>`), `StrLen` / `StrBegin`
  (`<Misra/Std/Container/Str/Access.h>`), and the corresponding `Vec`
  ones. Direct field access is reserved for the container's own
  implementation.

## Macro hygiene

- **Any macro-local temporary that lives in the caller's scope must be
  named with `UNPL(base)`** (`<Misra/Types.h>`). This applies to every
  macro, not just `for`-chain foreach helpers: swap scratch slots,
  saved pointers, range bounds, single-shot guards, anything the macro
  introduces that the caller might also have named. `UNPL` pastes
  `__LINE__` onto the base so each expansion gets a fresh identifier;
  without it, a macro that declares `tmp` will collide with a caller's
  own `tmp` and silently shadow it, or break compilation in the lucky
  cases.
- Names the caller passes in (loop variable, index variable, output
  parameter) are part of the macro's contract — those stay as-is.
  `UNPL` is only for identifiers the macro mints for its own
  bookkeeping.

## Sub-range iteration

- Use `BufIter` (or the generic `Iter(T)`) for cursor-style reads.
  `IterMove` / `IterMustMove` for position, `IterRead` / `IterPeekAt`
  for values, `IterRemainingLength` for the bytes-left query.
- For sub-ranges: `IterCarve(parent, n)` (in
  `<Misra/Std/Utility/Iter/Init.h>`) returns a child iter starting at
  the parent's current pos with length `n`; the parent is unchanged.
  `IterTruncate(iter, n)` (in `<Misra/Std/Utility/Iter/Access.h>`) caps
  an iter in place so only `n` more reads remain.
- For fixed-layout binary records, reach for `BufReadFmt` / `BufAppendFmt`
  / `BufWriteFmt` (declared in `<Misra/Std/Io.h>`). Format directives are
  `{<Nr}` (little-endian) and `{>Nr}` (big-endian), with `N` in
  `{1, 2, 4, 8}`.

## Documentation

- Every public function and macro gets a doc comment with **`SUCCESS:`**
  and **`FAILURE:`** lines. Both lines describe the full behaviour —
  return value, control flow, and state effects — not just what the
  function returns.
- **Comments explain *why*, not *what*.** A well-named identifier
  already says what; comments should add the non-obvious reason. Don't
  reference the current PR, ticket number, or caller — those rot.
- Default to no in-code comment. Add one only when removing it would
  confuse a future reader.

## Commits and pre-commit

- **Tests must pass** before every commit.
- **`clang-format` runs on every modified C source file**
  (`.c` / `.h` / `.cpp` / `.hpp` / `.inc`). Do **not** run it on
  `meson.build`, markdown, or other non-C files — it'll mangle them.
- **Small, focused commits.** Don't bundle unrelated cleanups with a
  feature change; reviewers should be able to read one commit in one
  sitting.
- **Don't speculatively push fixes** to a remote that runs CI on every
  push. Gather all the errors from a CI round, fix locally, then push
  once. Round-trip "maybe this works" pushes burn signal.

## Destructive operations

- Confirm before any destructive git operation: `git stash -u`,
  `git checkout HEAD -- .`, `git reset --hard`, force-push,
  branch deletion. They can wipe uncommitted work that the local
  environment hasn't surfaced.
- If you encounter unexpected state (a stray branch, an unfamiliar lock
  file, a build artifact you didn't make), investigate before deleting.
  It may be someone else's in-progress work.

---

If a rule here disagrees with what the code actually does, the code is
likely behind on cleanup — but flag it on the relevant PR rather than
silently following the stale pattern.
