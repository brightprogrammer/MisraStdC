# AGENTS.md

Operating guide for AI agents (and humans) working in this repo. Read it before
making changes. The conventions below are enforced — drift makes the codebase
harder to review and harder to maintain.

## Repo at a glance

MisraStdC is a C11 generic-container and formatted-I/O library. Macros are the
typed front door (`Vec(T)`, `VecInsertL`, `IntAdd`, ...); the macros expand to
runtime helper functions in `Source/`. Internal helpers stay private; the public
surface is the macros and the functions documented in the non-`Private.h`
headers under `Include/Misra/...`.

There is an in-progress refactor on `allocator-fallible-container-refactor`
that converts container operations to fallible APIs with `Must*` aborting
variants. The design rationale lives in
`Docs/content/english/guides/planned-fallible-apis-and-allocators.md` — read it
before changing container error handling.

## Pre-commit expectations

These are non-negotiable. Run them before every commit:

1. **All tests must pass.** From `builddir/`, run `meson test --no-rebuild`. The
   acceptable result is `Fail: 0`. Pre-existing failures inherited from a
   parent commit can be flagged to the user, but never introduce a new failure
   or worsen an existing one without explicit acknowledgement.
2. **Run clang-format.** From repo root: `python Scripts/clang-format.py`, or
   `bash Scripts/check-format.sh` to verify. Format the files you touched at
   minimum.
3. **Build cleanly.** `ninja -C builddir` should have no `error:` lines. The
   project intentionally keeps `_FORTIFY_SOURCE` warnings; ignore those.

If any of the three is red, the commit isn't ready.

## Git workflow

- **Small commits are preferred.** Make one logical change per commit, with a
  message that explains the why.
- **Commit messages**: short subject line in imperative mood, optionally
  followed by a body paragraph(s) describing motivation, scope, and notable
  consequences. Match the style already on `git log`.
- **Merges require explicit user approval.** Do not merge feature branches into
  `master` without confirmation.
- **All destructive git ops require user approval before action.** That
  includes `git push --force`, `git reset --hard`, `git checkout HEAD -- .`,
  branch deletion, and especially `git stash -u` (which swallows untracked
  files and is a common foot-gun).
- **Don't commit untracked working files.** `TODO`, scratch `.md` notes,
  `.claude/` settings, etc. belong outside source control unless the user asks.
- **No `--no-verify` or hook-skipping flags** unless the user explicitly asks.

## Identifier conventions

Public vs private is signalled by **name shape** and **file placement**.

### Public API → PascalCase

Anything users call. Lives in non-`Private.h` headers under `Include/Misra/`.
Examples: `VecInsertL`, `VecMustReserve`, `MapInsertL`, `BitVecPush`,
`IntAdd` (the macro), `FloatFrom`, `StrInitFromZstr`, `LOG_FATAL`,
`SysAbort`.

Macros that share an identifier with a runtime function (e.g. `IntAdd` is both
a macro and a function declared in `Private.h`) keep the macro name in
PascalCase. The function becomes snake_case (see below).

### Private / internal helpers → snake_case

Anything not intended for users. Includes:

- runtime functions in `Source/Misra/.../*.c`
- declarations in `Include/Misra/.../<Container>/Private.h`
- file-local `static inline` helpers in any header
- `_Generic` dispatch targets even if they share a name with a public macro

Examples: `insert_range_into_vec`, `vec_aligned_size`, `map_slot_occupied`,
`int_add`, `float_compare_with_error`, `int_log2_no_error`.

If you discover a function in `Private.h` that user code actually calls
directly (not through a dispatch macro), move it to the matching public header
(`Math.h`, `Convert.h`, etc.) and keep it PascalCase. Otherwise, leave it in
`Private.h` and make sure it is snake_case.

### No `MISRA_` or `MISRA_PRIV_` prefixes on identifiers

The only places `MISRA_` is allowed:

- Header include guards: `MISRA_STD_CONTAINER_VEC_INSERT_H` etc.
- Magic-number sentinels stored on struct instances: `MISRA_VEC_MAGIC`,
  `MISRA_INT_MAGIC`, ...

Everywhere else, drop the prefix. File placement plus naming convention
already signal "internal."

## Error handling

This is the core design choice — read it carefully.

### Programmer error → abort

Caller bugs die loudly. Use `LOG_FATAL("message")` for:

- violated preconditions
- NULL where a valid pointer is required
- out-of-bounds access from caller misuse
- uninitialized / corrupted objects
- broken internal invariants

`LOG_FATAL` is itself a do-while macro that logs + calls `SysAbort()`. Use it
directly. **Do not** add a per-family `fprintf + SysAbort` helper or a wrapper
macro just to print a fatal message. Inline `LOG_FATAL` at the call site.

### Runtime failure → propagate

Allocation failure, parse failure, range errors that aren't caller bugs — these
are normal failures, not aborts. Public fallible APIs return `bool` (or a
sentinel like `GraphNodeId == 0`) so callers can recover.

### Two-axis split: plain vs Must

- Plain operation name = propagating fallible form, expression-shaped, returns
  `bool` (or pointer/value). Used as `if (!VecInsertL(&v, val, idx)) return false;`
- `Must` variant = statement-style `do { ... } while (0)` macro that calls
  `LOG_FATAL` on failure. Used as `VecMustInsertL(&v, val, idx);`

Both call the same underlying runtime helper. Don't introduce a third layer
between them.

### Where `LOG_FATAL` can and can't be used

`LOG_FATAL` lives in `Misra/Std/Log.h`. `Log.h` transitively includes `Io.h`
and `Container.h`, so the include chain `Log -> Io -> Container -> Vec` makes
it impossible to include `Log.h` from any container header directly.

Consequences:

- **Inside a `#define ...` macro body** — late preprocessor binding means you
  *can* reference `LOG_FATAL` even from container headers, because the macro
  is expanded at the user's call site (after `Std.h` has pulled in `Log.h`).
- **Inside a `static inline` function body in a container header** — you
  *cannot* use `LOG_FATAL`, because the preprocessor binds at header preprocess
  time. The right answer is to move that helper to the matching `.c` file
  (e.g. `Vec.c`, `List.c`) and declare it in `<Container>/Private.h`. From the
  `.c` side `LOG_FATAL` is reachable and the helper participates in the same
  runtime layer as `insert_range_into_vec`, `merge_list`, etc.

This refactor direction is consistent with the planned-fallible-API guide:
"runtime helpers perform the actual reserve, insert, move, copy, and cleanup
work" — those helpers belong in `.c` files.

### Two-axis policy (from the design guide)

The allocator and the API layer have different jobs:

- **Allocator effort axis** — how hard the allocator tries before giving up
  (single try, retry, fallback, etc.).
- **API termination axis** — whether the public API propagates failure
  (plain name) or treats failure as must-succeed (`Must*` name).

`VecInsert...` propagates. `VecMustInsert...` terminates at the API boundary.
Allocator behaviour stays independent of name choice.

## Macros

- Public macros are PascalCase and live in `Include/Misra/.../`.
- Generic dispatch helpers (e.g. `INT_ADD_DISPATCH`, `BITVEC_EDIT_DISTANCE_SELECT`)
  use SCREAMING_SNAKE_CASE and do **not** carry the `MISRA_` prefix.
- Propagating forms must be **expression-shaped** so they evaluate to a value
  that callers can branch on. They use comma-operator chains:
  ```c
  #define VecInsertL(v, lval, idx)            \
      (ValidateVec(v),                        \
       VEC_TYPECHECK_L((v), (lval)),          \
       vec_insert_one_l(GENERIC_VEC(v), ...))
  ```
- `Must*` forms are **statement-style** `do { ... } while (0)` macros wrapping
  the propagating form plus an inline `LOG_FATAL`:
  ```c
  #define VecMustInsertL(v, lval, idx)                    \
      do {                                                \
          if (!VecInsertL((v), (lval), (idx))) {          \
              LOG_FATAL("VecMustInsertL failed");         \
          }                                               \
      } while (0)
  ```
- The same identifier may name both a public macro and a same-name dispatch
  target (e.g. `IntAdd`). After our snake_case refactor the function becomes
  `int_add` and only the macro keeps the PascalCase name; the C preprocessor's
  no-recursive-expansion rule keeps both side-by-side.

## Documentation

Documentation above every public API is mandatory. Use the project's `///`
style:

```c
///
/// Short one-line summary.
/// Optional longer description on the next lines.
///
/// param1[in]      : What it is and any constraints.
/// param2[in,out]  : What it is and how it changes.
/// out_param[out]  : Where the result lands.
///
/// SUCCESS : Full behaviour on the success path — control flow returns, return
///           value or sentinel, and any state effects the caller should know
///           about (out param populated, length bumped, source emptied on
///           ownership transfer, etc.).
/// FAILURE : Full behaviour on the failure path — whether control flow returns
///           at all, return value or sentinel, and the state of the object on
///           failure (unchanged, partially populated, source left intact,
///           etc.).
///
/// USAGE:
///   ExampleCall(&value, 10);
///
/// TAGS: Container, Operation, Category
///
ReturnType ApiName(ArgType arg);
```

Notes on `SUCCESS:` / `FAILURE:`:

- These describe **the contract**, not just the literal return value.
  "Returning" is the return of **control flow** to the caller. State that
  explicitly when the function aborts or otherwise does not return.
- Cover what the reader needs to know without opening the source:
  - return value or sentinel (`true`, `false`, `NULL`, `GraphNodeId`, ...)
  - whether control returns at all
  - state changes the caller cares about (out param populated, source
    emptied, vector unchanged on failure, length bumped, allocator bound,
    internal rollback applied)
- For a `Must*` variant, the `FAILURE:` line should say something like
  "Does not return — aborts via `LOG_FATAL` / `SysAbort`."
- For a fallible propagating form, give both the bool return AND the state
  guarantee, e.g.:
  ```
  /// SUCCESS : Returns `true`. The vector contains the inserted element at `idx`.
  /// FAILURE : Returns `false` on allocation failure. The vector and `lval` are unchanged.
  ```
- For a void-returning operation that cannot fail, write
  `/// FAILURE : Function cannot fail.` rather than omitting the section.
- Older doc blocks sometimes use `RETURNS:` instead of `SUCCESS:` /
  `FAILURE:`. New code should use the SUCCESS/FAILURE pair.

Other notes:

- The `USAGE:` example should be a realistic snippet, not a placeholder.
- `TAGS:` are free-form categorical labels used by the docs generator under
  `Docs/`.
- Doc comments reference the **user-facing macro name** (PascalCase) even
  when the underlying function is snake_case — examples live at the public
  surface.
- File-level header: every header file starts with `/// file : path`,
  `/// author : ...`, the license notice, and a one-line summary.

Internal helpers (snake_case, declared in `Private.h` or static in a `.c`
file) don't need doc blocks. Add a one-line comment only if the why is
non-obvious.

## Library hygiene

- **Prefer the library's own utilities over libc** when an equivalent exists.
  Use `MemCopy` / `MemMove` / `MemSet` instead of `memcpy` / `memmove` /
  `memset` everywhere except inside the memory implementation itself. Use
  `ZstrCompare` / `ZstrLen` instead of `strcmp` / `strlen`.
- **Validation belongs in the macro layer** when feasible, so runtime helpers
  stay focused on the actual mutation. `ValidateVec(v)` and the
  `VEC_TYPECHECK_*` helpers run at the macro layer before the runtime helper
  is called.
- **Allocators are passed by value at init time**, copied into the owning
  object, and **immutable for the lifetime of that object**. Public init APIs
  accept an optional allocator (`VecInit()` vs `VecInit(alloc)`); when omitted,
  `DefaultAllocator()` is used.

## No library-owned global state

The library never owns mutable global state. This includes:

- No file-scope `static` variables holding mutable state that outlives a
  single call.
- No process-wide allocator pools, caches, or shared bookkeeping.
- No thread-local fallbacks that behave like globals.

Every piece of mutable state must live inside a user-owned object (a
container, an allocator instance, etc.) and be allocated/released through
the same allocator that owns the rest of that object's storage. The user
constructs allocators; the library never creates them implicitly behind the
user's back.

Why: globals race under concurrent use, hide ownership, and make tear-down
order undefined. The library's design forces ownership to flow through
explicit `Allocator` handoffs at object construction time. That makes
ownership transfers visible at every call site, which is the whole point.

Two concrete consequences:

- **Allocator bootstrap goes through `PageAllocator`**, which is the only
  stateless allocator in the library (its `allocate` calls `mmap` /
  `VirtualAlloc` directly with no setup). Stateful allocators
  (`HeapAllocator`, `ArenaAllocator`, `PoolAllocator`) allocate their state
  via `PageAllocator` lazily on first use through `state_init` and free it
  through `state_deinit`.
- **For raw buffer allocations (not through a container)**, the user must
  construct an allocator explicitly and pass it. The library does not
  provide a hidden "global heap" to fall back on. Users who want libc-managed
  allocations construct a libc-backed allocator and pass it.

## File placement and naming-shape pairing

Internal helpers are signalled by **both** location and name shape. Either
one alone is insufficient.

- A function declared in a non-`Private.h` header but in `snake_case` is a
  violation - either it's actually internal (move declaration to the matching
  `Private.h`, leave the name in snake_case) or it's actually public (rename
  to PascalCase and keep it in the public header).
- A function declared in `Private.h` but in `PascalCase` is a violation -
  rename it to snake_case.
- A function declared in a public header with `Internal` suffix, `Impl`
  suffix, or leading underscore is a violation - those naming dodges don't
  substitute for proper file placement. Move the declaration to the matching
  `Private.h` and rename to snake_case.

Identifiers starting with an underscore are **reserved** by the C standard
for the implementation (`_<lowercase>` at file scope, `_<uppercase>` or
`__<anything>` always). Library identifiers must not start with an underscore
even for "internal" helpers.

When a public macro needs to call a snake_case runtime helper that lives in
`Private.h`, the public umbrella header (`Misra/Std/Container/Vec.h`,
`Misra/Std/Allocator.h`, ...) `#include`s the `Private.h` so the declaration
is visible at the user's call site after macro expansion. Users don't include
`Private.h` directly; they include the umbrella.

## Comments

- Default to **no comments**. Names should be self-explanatory.
- Add a comment only when the **why** is non-obvious: a hidden constraint, a
  subtle invariant, a workaround for a specific bug, behaviour that would
  surprise a future reader.
- Do not narrate what the code does. Doc comments at the public boundary are
  the only place to describe behaviour at length.
- Avoid comments that reference the current task, PR, or caller. Those rot
  fast and belong in commit messages, not source.

## When in doubt

- **Read the guide first.** `Docs/content/english/guides/` has prose
  walkthroughs of containers, ownership, and the in-progress refactor.
- **Match local convention** within the file you're editing. The library has
  decades of accreted style; pick up the local idiom rather than imposing a
  global rewrite mid-PR.
- **Ask the user** before doing anything broad or destructive. Sweeping
  renames, merges, force-pushes, and schema-level refactors should be
  confirmed first.
