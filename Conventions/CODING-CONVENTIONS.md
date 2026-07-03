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
  prefix itself is reserved for two narrow uses only:
  - **Include guards** on public headers (`MISRA_STD_CONTAINER_BUF_H`,
    `MISRA_SYS_PROC_H`, etc.). The path-derived guard name keeps them
    collision-free in downstream projects.
  - **`MISRA_SYS_*` direct-syscall kernel-ABI constants** in
    `Source/Misra/_Syscall.h` only (collision-avoidance with libc's
    `SYS_*` / `sys_*` syscall vocabulary).

  Two other UPPER_SNAKE name families are project-controlled but do NOT
  carry the `MISRA_` prefix and live elsewhere:
  - **`FEATURE_*`** build-config flags emitted by `meson.build` /
    `Config.h` (`FEATURE_PARSER_JSON`, `FEATURE_ALLOC_DEBUG`, ...).
  - **`*_MAGIC`** struct sentinels (`HEAP_ALLOCATOR_MAGIC`, `VEC_MAGIC`,
    ...) defined per-type next to the struct they tag.
- **Short names where the include path already disambiguates.** `Buf`,
  `Str`, `Vec`, `Elf` — not `MisraBuf`, not `ElfFile`.
- **Tool binaries** ship with a single short word as their name
  (see `Bin/Beam.c`, `Bin/Resolve.c`).
- **C-strings everywhere use `Zstr`**, not `char *` / `const char *`,
  in declarations / parameters / return types / fields. `Zstr`
  (`<Misra/Std/Zstr.h>`) is the project's name for a NUL-terminated
  C-string. The build sets `-Wwrite-strings` (gcc/clang/clang-cl) so
  string literals carry type `const char *` (= `Zstr`) and `char *`
  variables get rejected at the call site.
- **`_Generic` dispatch arms have a `char *` synonym next to every
  `Zstr` arm.** This is the one carve-out, and it exists because
  MSVC's C `_Generic` follows the C standard (string literals are
  `char[N]` decaying to `char *`); `/Zc:strictStrings` is a C++-only
  flag with no effect in C mode. Every site that dispatches on a
  string-shaped argument inlines both arms with identical bodies:

      _Generic((s),
          Zstr:   foo_zstr((Zstr)(s)),
          char *: foo_zstr((Zstr)(s)))

  Inline the pair at every site; do NOT introduce a wrapper macro
  that hides the `_Generic`. The library's read-only invariant for
  `Zstr` parameters is preserved -- the `char *` arm casts to `Zstr`
  before dispatching, and `Zstr` is `const char *`, so the input
  cannot be mutated through the parameter.
- **`Cstr` is a naming-suffix, not a type.** The `Cstr` form of an API
  takes `(Zstr, size)` — a non-NUL-terminated view of memory, or a
  NUL-terminated string truncated at an explicit length. See
  `StrStartsWith*` for the canonical Cstr / Zstr / unsuffixed-Str
  overload family.
- **Raw byte buffers use `u8 *`**, not `char *`. When a pointer holds
  bytes you'll do byte-grain arithmetic on (allocator chunks, owned
  memory regions, packed-record cursors), the type is `u8 *`. `char *`
  is reserved for the C-string dispatch arms above; `void *` only
  appears at API boundaries that genuinely accept opaque buffers.
  `Arena.last_ptr`, `Budget.buf`/`slots`, `ArenaChunk.base` follow this
  rule. The honest type matters: signed/unsigned, plain-char/typed-char
  divergence between platforms has bitten this codebase before.
- **Pointers returned by macros over a typed container return the
  contained type.** A macro that fishes an element pointer out of
  `Vec(T)` / `Str` / `List(T)` / `BitVec` / ... returns `T *`, not
  `char *` / `u8 *` / `void *`. `VecPtrAt`, `VecBegin`, `VecEnd`,
  `VecAt`, `ListNodePtrAt`, etc. all cast their final result to
  `VEC_DATATYPE(v) *` (or the equivalent type accessor) before
  handing it back; the internal byte-arithmetic is `char *` so that
  `Str` (a `Vec(char)`) proxies cleanly without `-Wpointer-sign`,
  but the macro's *return* type is always the contained `T *`.
  Callers receive a fully typed pointer they can deref / index
  without a cast.

## API shape

- **`FooInit` is a macro**, not a function. It expands to a designated-
  initializer struct literal (`VecInit`, `StrInit`, `HeapAllocatorInit`).
  It cannot fail. Use `OVERLOAD` for arg-count dispatch.
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
- **Unsuffixed default = L.** When a family has both `*L` and `*R`
  variants, the bare unsuffixed name aliases to L: `VecInsert` ≡
  `VecInsertL`, `ElfOpenFromMemory` is the L-form (`ElfOpenFromMemoryCopy`
  is the explicit R counterpart). Don't dispatch an unsuffixed macro to
  an R primitive — that inverts the convention and surprises callers.
- **L primitives are strict-typed; R primitives are permissive-typed.**
  L's internal `CHECK_TYPE_EQUIVALENCE` requires the source element
  type to match the container's element type *exactly* (no const
  promotion, no integer narrowing) — because L zeroes the source on
  success, and that's only safe on writeable storage of the right
  element type. R uses `CHECK_TYPE_CONVERTIBLE`, which accepts any
  initialization-compatible source (e.g. `int` literal → `char`,
  `char *` → `const char *`). Two consequences:
  - Read-only / const sources (`Zstr`, `Cstr`-shaped `(Zstr, size)`)
    *cannot* ride an L-form macro. The L primitive's strict-type
    check rejects `const T *` for a `Vec(T)`. So a family whose
    inputs are only ever const surfaces only R-form variants; there
    is no L-form and no unsuffixed default — callers spell out `*R`.
  - Integer literals like `'-'` (type `int` in C11) compile with R's
    `CHECK_TYPE_CONVERTIBLE` but fail L's `CHECK_TYPE_EQUIVALENCE`.
    Callers who want L semantics with literals must bind to a typed
    lvalue first (`char c = '-'; VecInsertL(&v, c, 0)`).
- **Prefer `Str` to `Zstr`.** `Str` carries its length and allocator
  inline; `Zstr` is a raw NUL-terminated pointer. For stored fields,
  parameters the function needs a length for, and anywhere the caller's
  life is easier with the length already attached, use `Str` (or
  `const Str *` when the function only reads). `Zstr` is reserved for
  cases where the caller is genuinely working with a bare C-string —
  string literals, argv, kernel-boundary parameters — and knows the
  no-length view is what they want.
- **Owned strings must be `Str`.** If a struct *owns* the storage of a
  NUL-terminated string — allocates it, frees it on `Deinit` — the
  field is a `Str` (which carries its allocator and self-cleans). A
  `Zstr` field is acceptable only as a borrowed view, where someone
  else owns the lifetime. The fix when you find owned `Zstr`: switch
  the field to `Str`, replace the manual `AllocatorAlloc + MemCopy`
  with `StrTryInitFromCstr` (or similar), and the Deinit loop becomes
  `StrDeinit(&field)`. `MachoCacheEntry.module_path` and
  `PdbCacheEntry.module_path` are the canonical references.
- **A `(Zstr, length)` API always ships with a `Str` overload.** If a
  function takes a Zstr together with an explicit length (i.e. it's
  basically a Str minus the wrapper), provide a `Str` overload
  alongside so callers holding a `Str` don't have to reach inside for
  `.length` and `.data`. The canonical shape is the `Cstr` / `Zstr` /
  unsuffixed-Str naming pattern as **private snake_case backends in
  a `Private.h`** (one file per namespace) plus a **single public
  `PascalCase` macro** on top, dispatching via `OVERLOAD`
  (arg count for the Cstr variant) and `_Generic` (`Str *` vs `Zstr`):

  ```c
  // Private — Std/Container/Str/Private.h (one backend per shape).
  bool str_starts_with_str  (const Str *s, const Str *prefix);
  bool str_starts_with_zstr (const Str *s, Zstr prefix);
  bool str_starts_with_cstr (const Str *s, Zstr prefix, size prefix_len);

  // Public — Std/Container/Str/Ops.h (single unified entry point).
  #define StrStartsWith(...) OVERLOAD(StrStartsWith, __VA_ARGS__)
  #define StrStartsWith_2(s, prefix)                                       \
      _Generic((prefix),                                                   \
          Str *:  str_starts_with_str ((s), (const Str *)(prefix)),        \
          Zstr:   str_starts_with_zstr((s), (Zstr)(prefix)),               \
          char *: str_starts_with_zstr((s), (Zstr)(prefix)))
  #define StrStartsWith_3(s, prefix, prefix_len)                           \
      str_starts_with_cstr((s), (Zstr)(prefix), (prefix_len))
  ```

  The `char *` arm is the MSVC-portability synonym for `Zstr` — see
  the "C-strings everywhere use `Zstr`" rule above.

  The user-facing surface is just `StrStartsWith` — callers never type
  `*Cstr` / `*Zstr` suffixes. Same applies whenever a `Zstr` parameter
  shows up: add the `Str` overload (or surface that the function
  should be `Str`-only — if no caller ever wants the `Zstr` form, you
  don't need it). The reference implementations are
  `Std/Container/Str/Ops.h`'s `StrStartsWith`, `StrEndsWith`,
  `StrIndexOf`, `StrContains`, `StrReplace` families.
- **Accessor macros are read-only.** `VecLen`, `VecCapacity`,
  `StrCapacity`, `MapPairCount`, `ListHead`, `BitVecData`, etc. expose
  state for inspection; they don't mutate. Mutation always goes through
  a dedicated mutator (`VecResize`, `VecReserve`, `VecTryReduceSpace`,
  `MapInsertR`, `ListPushBack`, ...). There is no `SetCapacity` /
  `SetLength` / `SetHead` shape anywhere in the codebase, and there
  shouldn't be — adjusting state has invariants the mutators enforce.
  Intentional-corruption tests that need to bypass an invariant (to
  verify that a validator catches it) write the field directly, with
  an inline comment explaining why no public accessor or mutator
  covers the case.
- **Container key types ship `*_hash` and `*_compare`.** Any type
  meant to be a `Map` key (or `Vec`/`List` element with comparison
  semantics) must expose two snake_case helpers in the
  `GenericHash` / `GenericCompare` shapes:

  ```c
  u64 foo_hash(const void *data, u32 size);     // size is ignored;
                                                // length lives in the type
  i32 foo_compare(const void *lhs, const void *rhs);
  ```

  These drop straight into `MapInitWithDeepCopy` / `VecSort` /
  `MapPolicyLinear`-style slots. `str_hash` / `str_compare` in
  `<Misra/Std/Container/Str/Ops.h>` are the canonical references.

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
- **Shared per-instance state lives on the base, never duplicated per
  typed subtype.** A typed allocator embeds `Allocator base` and reads
  / writes any uniform metric (`AllocatorStats`, `footprint_bytes`)
  through that one field. Subtypes don't carry parallel copies.
- **Each piece of state is written from exactly one layer.** Where a
  typed body and a dispatch wrapper both exist for the same operation,
  the typed body owns the writes; the wrapper just routes. Two writers
  for the same counter would double-count and force callers to know
  which entry point they used.
- **Read-only fields are exposed through `((void)0, ptr->field)`
  accessor macros, not function calls.** Same shape as `BufLength`,
  `VecCapacity`, `AllocatorBytesInUse`, `AllocatorFootprintBytes`,
  etc. No `*Get*`-style getter functions, no `_Generic`-dispatched
  readers — readers stay zero-overhead lvalue-rejecting macros.
- **Stack-promote transient containers** with `*InitStack` (`StrInitStack`,
  `VecInitStack`, ...) -- see the dedicated section below.

## Stack-init APIs

- **Stack means stack.** `*InitStack` macros take **no allocator**.
  The backing storage is a stack array sized by the `ne` argument;
  the container's inline allocator slot is `NULL`. If you would have
  written `char buf[N]` and tracked its length yourself, use
  `StrInitStack(name, N) { ... }` instead -- the lifetime, capacity,
  and zero-on-exit are all expressed by the macro.
- **Overflow is a contract violation, not a fallback.** Any
  operation that would grow the container past `ne` lands in the
  realloc path with a NULL allocator; the runtime aborts with
  `LOG_FATAL("vector not growable, no allocator assigned, probably
  stack inited")`. There is no spill / overflow / upstream allocator
  argument. If a caller legitimately needs spill behaviour, that
  caller wants an allocator-backed container, not `*InitStack`.
- **No deep-copy callbacks on stack-init.** `copy_init` / `copy_deinit`
  hooks take an `Allocator *` for the inner resources they own. A
  stack-init container has no allocator and so can't pair with deep
  copies. If you need deep-copy ownership for elements, switch to a
  heap-backed container -- there is no `*InitStackWithDeepCopy`.
- **For-chain scope idiom.** The body is regular code below the
  macro, not a brace-delimited macro argument. This keeps `return` /
  `break` / `continue` from being macro-mangled and matches `Scope(...)`
  in `<Misra/Std/Allocator.h>`:

      StrInitStack(buf, 1024) {
          ssize_t n = read(fd, StrBegin(&buf), 1023);
          StrResize(&buf, (size)n);
          StrMergeR(out, &buf);
      }

  `return` / `goto` leaving the body skip the zero-on-exit cleanup
  (a C-level limitation, not a bug). Use `break` to leave cleanly.
- **When to use vs heap container.** Reach for `*InitStack` when the
  size is bounded by something you control (a syscall buffer, a
  formatted line, a per-iteration scratch space). Reach for a heap
  container when the size is caller-controlled or unbounded
  (parsing arbitrary input, accumulating across iterations whose
  total is unknown). Don't paper over an unknown-size case with a
  guessed `ne`.
- **Don't call `Deinit` on a stack-backed handle inside the scope.**
  The scope macro zeroes both the backing buffer and the container
  handle on exit, which invalidates the magic value; after the
  scope, any operation on the handle (including a stray
  `StrDeinit` / `VecDeinit`) trips `validate_vec` with the
  `Either uninitialized or corrupted!` message. Inside the scope,
  there is no separate teardown to run -- the macro IS the teardown.

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
- **Kernel-boundary signatures keep their OS-native shape.** When a
  function is forwarding straight into a syscall or Win32 entry point,
  the parameter list matches whatever that boundary expects — `Zstr`
  rules don't override `execve`'s `char **argv` / `char **envp`, and
  the Zstr-everywhere rule doesn't override `int main(int argc, char
  **argv)`. The boundary is the carve-out; everything that wraps it
  is fair game for Misra-native types.
- **Fuzz harnesses are out of scope.** Code under `Fuzz/` is allowed to
  include libc / C++ standard headers — these are harness boundaries that
  intentionally exercise the in-tree implementation against external
  inputs. `check_no_libc.py` covers the library proper (`Source/`,
  `Include/`, `Tests/`).

## `_Generic` dispatch

- **Inline `_Generic` directly in each macro.** Never extract a shared
  "extract raw pointer" / "convert type X to type Y" helper. The point
  of the wrapped types is to hide the raw form; pulling it back out via
  a shared helper undoes the design.
- **Type-safe dispatch — list the accepted types explicitly.** Avoid a
  `default:` arm that silently casts a wrong type to the function's
  expected one. Any other input type should trigger a compile-time
  `_Generic` mismatch.
  - Path / string dispatch: `Str *` plus the `Zstr` + `char *` pair
    (the two share a body — see the "C-strings everywhere use `Zstr`"
    rule for why MSVC C requires the `char *` synonym). No
    `const char *` arms.
  - Container-out dispatch: `Buf *`, `Str *`.

  Skeleton:
  ```c
  #define FileGetSize(path)                                      \
      _Generic((path),                                           \
          Str *:  file_get_size(StrBegin((Str *)(path))),        \
          Zstr:   file_get_size((Zstr)(path)),                   \
          char *: file_get_size((Zstr)(path)))
  ```

  The `char *` arm is the MSVC-portability synonym for `Zstr` — see
  the "C-strings everywhere use `Zstr`" rule.

  For functions that already have an arg-count variant of the API
  (`*Cstr` form taking `(Zstr, size)`), combine the type dispatch
  above with `OVERLOAD` for arg count — see the
  *StrStartsWith family* example next to the `Cstr` / `Zstr` /
  unsuffixed-Str description in the *API shape* section.
- **Don't reach into a wrapped type's fields from outside its
  namespace.** Go through the public accessor macros: `BufLength` /
  `BufData` / `BufAllocator` (`<Misra/Std/Container/Buf.h>`), `StrLen`
  / `StrBegin` (`<Misra/Std/Container/Str/Access.h>`), and the
  corresponding `Vec` ones.
  - **Same-namespace public macros may touch their own type's fields
    directly.** `VecPushBack` reading `(v)->length`, `MapCompact`
    reading `(m)->policy`, `ListForeach` reading `(l)->tail` etc. are
    NOT violations -- they are part of the same container's public
    macro layer, just split across the header for inlining. The rule
    is "don't reach in from OUTSIDE the namespace," not "the macro
    body must call an accessor that wraps the same field read."
  - **Tests are allowed to read fields directly when no public
    accessor covers the invariant**, BUT prefer the public API
    wherever it exists. Tests double as usage examples; a test that
    open-codes field reads when an accessor would do the same job
    models bad usage. If a test repeatedly needs a state observation
    that no accessor exposes, add the accessor (it is presumably
    useful to non-test consumers too).
- **`_Generic` arm bodies must have uniform shape.** C11 type-checks
  *every* arm's body (not just the selected one). An arm that
  dereferences (`((T *)(val))->field`) and an arm that treats `val`
  as a value (`(char)(val)`) cannot coexist in the same `_Generic`:
  when `val` has the *other* arm's type, the non-selected arm fails
  to type-check (CHECK_TYPE_EQUIVALENCE / CHECK_TYPE_CONVERTIBLE
  fires inside the unreached arm's macro expansion). Concretely:
  do not mix single-value arms (`char:`, `int:`) with pointer-deref
  arms (`Str *:`, `Zstr:`) in one `_Generic`. Split into two macros
  — one for the single-value shape, one for the range / pointer
  shape — and give them distinct names (e.g. `*Insert` for single,
  `*InsertMany` for range). The unified-macro-with-mixed-shapes
  pattern only works when every arm body has the same pointer-deref
  or pointer-cast shape.

## Type lattice

Several library boundaries pair a concrete struct with an embedded base
struct (e.g. `HeapAllocator` contains `Allocator base` at offset 0; same
shape for `PageAllocator`, `ArenaAllocator`, `SlabAllocator`,
`BudgetAllocator`, `DebugAllocator`). These pairs form a lattice with the
concrete type at the bottom and the type-erased base at the top.

**Movement is one-way: down only.** Code is allowed to specialise a
generic pointer into a concrete one (`(HeapAllocator *)self` at a vtable
entry, or at the boundary where a `void *` arrives from an external
module). Code is **not allowed** to generalise a concrete pointer back to
its base mid-flow. Once the type is concrete, it stays concrete until the
caller passes it across a genuinely type-erased boundary.

- **Forbidden: `.base` shuffles in internal callers.**

      // wrong -- specialise (have HeapAllocator *) -> generalise to
      //         &heap->page.base (Allocator *) -> let _Generic re-erase
      //         to AllocatorAlloc_dyn -> vtable indirect -> typed func.
      AllocatorAlloc(&heap->page.base, n, false);

      // right -- stay specialised end-to-end.
      page_allocator_allocate(&heap->page, n, false);

- **Forbidden: typed entry points that take the generic.** If
  `xx_allocator_allocate` knows it always receives an `XxAllocator *`,
  its signature must say so:

      // wrong -- generic in, immediate cast back to specific.
      void *xx_allocator_allocate(Allocator *self, size n, i8 z) {
          XxAllocator *x = (XxAllocator *)self;
          ...
      }

      // right -- typed self, no cast inside, no .base in caller.
      void *xx_allocator_allocate(XxAllocator *self, size n, i8 z) {
          ...
      }

- **Vtable install does the cast, never a thunk symbol.** Function-
  pointer casts at compound-literal vtable initialisation are the one
  legitimate up-cast in the codebase — they live exactly at the
  type-erasure boundary and turn into zero machine instructions:

      // right -- cast at install.
      #define XxAllocatorInit() ((XxAllocator) {       \
          .base = {                                    \
              .allocate   = (AllocatorAllocateFn)   xx_allocator_allocate,   \
              .resize     = (AllocatorResizeFn)     xx_allocator_resize,     \
              .remap      = (AllocatorRemapFn)      xx_allocator_remap,      \
              .deallocate = (AllocatorDeallocateFn) xx_allocator_deallocate, \
              ...                                                            \
          }, ...                                                             \
      })

  Do not introduce an `xx_allocator_allocate_dyn(Allocator *)` thunk that
  wraps `xx_allocator_allocate(XxAllocator *)`. The vtable install is
  already an up-cast — adding a thunk is a redundant up-cast plus a real
  symbol the linker has to carry.

- **`_Generic` dispatch macros never re-erase.** The `AllocatorAlloc` /
  `AllocatorResize` / `AllocatorRemap` / `AllocatorFree` macros hand the
  caller's pointer straight to the chosen arm. They do **not** insert
  `(Allocator *)(self)` between `_Generic` and the call — the typed arm
  expects the typed pointer, the `Allocator *` arm receives an already-
  `Allocator *` argument from the caller.

- **Type erasure is a design problem, not a tool.** This convention has
  no "exception" clause. Every `Allocator *` (or other base-typed value)
  flowing through the codebase is a place where the compiler has *less*
  information than the human who wrote the call. There is no list of
  "places where erasure is fine" — there are only places where the
  current code happens to be erased, and the question is whether that
  should still be true.

  Before introducing any new type-erased field, parameter, container
  element, or callback signature: stop, and discuss it with whoever owns
  the affected design. Bring concrete answers to: which concrete types
  flow through this site, why a typed (or `_Generic`-dispatched, or
  parametrically-generated) shape doesn't fit, what the alternative
  shapes were and why they were rejected. Document the outcome of that
  discussion alongside the field/parameter. If the answer is "I wasn't
  sure," the answer is not erasure — the answer is to keep asking.

  The dyn-dispatch path through `AllocatorAlloc_dyn` exists for one
  specific reason: **out-of-tree extensibility.** A downstream consumer
  who defines their own `MyAllocator` cannot add an arm to the
  library's `_Generic` macros — so the catch-all `Allocator *:
  AllocatorAlloc_dyn` arm handles their typed pointer (after a one-step
  upcast at the call site) by going through the vtable they installed
  in their own `MyAllocatorInit`. The dyn path is the extension seam,
  not a sanctioned fallback for in-tree code. Every in-tree allocator
  has its own `_Generic` arm; an in-tree caller that ends up on the dyn
  arm is, by definition, doing something wrong.

**Why:** every up-cast discards information the compiler already had.
Best case the optimizer recovers it after inlining; common case it falls
through to the slower `_dyn` arm (`ValidateAllocator` + vtable indirect)
or pays for a tautological cast at function entry. But the bigger cost
is that the human reader, the static analyser, the IDE jump-to-definition
and the type checker all lose the same information the compiler does.
Erasure is cheap to type and expensive to live with; it is worth the
up-front design conversation every single time.

The lattice rule generalises beyond allocators. Any time the codebase
introduces a concrete-vs-base pair (containers with a base, parsers with
a base context, etc.), the same constraint applies: down-cast only at
the vtable install. There is no other place this convention sanctions an
up-cast.

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
- **Cross-macro protocol names are exempt from `UNPL`.** When an outer
  macro stamps a fixed identifier that inner macros are expected to
  read by name (e.g. `key` inside `JR_OBJ` / `JR_OBJ_KV`,
  `___is_first___` inside `JW_OBJ` / `JW_OBJ_KV`), `UNPL` would break
  the protocol — the outer's mint and the inner's lookup have to agree
  on the spelling. The protocol identifiers should be obvious-looking
  enough to flag at a reader's first glance, and a comment in the
  defining header should call them out.
- **Macros only earn their keep through transformation or code
  generation.** A `#define Foo(a, b) foo((a), (b))` that just renames
  a function and forwards its arguments unchanged is deadweight —
  delete the macro and let users call `foo` directly. Reach for a
  macro only when you're doing something a function call can't:
  stamping `__LINE__` (`UNPL`), arg-count dispatch (`OVERLOAD`),
  `_Generic` type dispatch, generating a `for`-chain body, etc.
  Think of macros as a code generator, not as the default ergonomics
  layer.
- **Namespace-inheritance aliases are not deadweight.** When a type is
  a typedef'd specialisation of another container (`Str` is a
  `Vec(char)`), the specialisation's own headers carry forwarding
  aliases for the parent container's ops — `StrForeach` →
  `VecForeach`, `StrResize` → `VecResize`, `StrAlignedOffsetAt` →
  `VecAlignedOffsetAt`, the full Insert / Remove / Memory / Access
  surface. These look like 1:1 renames but are the specialisation's
  own public API: callers should never have to know that `Str` is
  implemented as a `Vec(char)` to iterate it, and the alias is what
  enforces that. The alias's doc reframes the contract in the
  specialised vocabulary (string / character) rather than echoing the
  generic one (vector / element).
- **Local stamping macros are how repetitive type-instantiated bodies
  earn their keep.** When the same function body needs to exist for
  several concrete types -- and there is no way to share the body
  through a runtime helper because the type appears in the signature
  / cast / parser pick / bound check -- write the body once as a
  `_MAKE_FOO(NAME, T, ...)` macro at the top of the `.c`, stamp the
  instantiations underneath, and `#undef` the macro on the very next
  line so it cannot leak out of the file. The macro is parameterised
  over real type information (`T`, picked parser, bound-check block),
  not just a name -- that's the difference from an alias wrapper.
  Canonical reference: `_MAKE_READ_TXT_INT` + `_U_BOUND` / `_I_BOUND`
  in `Source/Misra/Std/Io.c`, which stamps `_read_u16` /
  `_read_u32` / `_read_u64` / `_read_i8..i64` with the right parser
  and the right post-parse bound check. The pattern is:

      #define _MAKE_FOO(NAME, T, ...)                                  \
          T fn_##NAME(...) { /* body uses T, parser, bound check */ }

      _MAKE_FOO(u16, u16, ...)
      _MAKE_FOO(u32, u32, ...)
      ...
      #undef _MAKE_FOO

  Keep the macro `.c`-local; never expose a stamping macro as a public
  API. If a caller would need to stamp their own, the abstraction
  belongs as a `_Generic` dispatch or a typed helper, not as a public
  stamp macro.
- **Don't pad structs by hand.** The compiler already inserts
  alignment padding correctly for the target ABI; explicit `_pad`
  fields are over-engineering. The C standard, the platform ABI, and
  the compiler are the authorities on layout. Hand-rolled padding
  pretends to pin the layout but actually just clutters the struct
  and risks getting the alignment wrong on a target the author didn't
  anticipate. Trust the compiler. The same rule covers other "I'll
  guess what the compiler will do" reach-arounds: don't second-guess
  the optimizer with `__attribute__((aligned))` or `__attribute__((packed))`
  unless you have a documented hardware or wire-format constraint
  forcing the choice, and write the reason inline when you do.
- **Over-engineering is not entertained.** The simplest design that
  satisfies the requirements is the right one. Indirection layers,
  caches that aren't proven necessary, classes / freelists / state
  machines added "for symmetry" or "in case we need it later" all
  earn deletion at review time. If you don't have a measured reason
  for the complexity, it doesn't go in.

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

## `*Must*` APIs and runtime input

- The `*Must*` family (`StrIterMustNext`, `StrIterMustMove`,
  `IterMustMove`, `IterMustRead`, `StrMustResize`, `VecMustInsert`,
  `BufMustRead*`, …) calls `LOG_FATAL` when its precondition fails.
  That is a process abort, not a recoverable error — it is the
  callsite's job to prove the precondition holds before invoking the
  `Must*`.
- **Safe usage:** the precondition is statically dominated by code
  immediately above the call. The two canonical shapes:
  - **peek-then-Must:** `StrIterPeek(&si, &c)` (or `StrIterRead`)
    succeeded in the surrounding control flow, proving ≥1 byte
    remains, and then `StrIterMustNext(&si)` consumes that byte.
  - **bounds-then-Must:** `if (StrIterRemainingLength(&si) >= N) { ...
    StrIterMustMove(&si, N); ... }` (or the same shape with
    `IterRemainingLength`). The `if` is the proof; the `Must` is the
    payoff.
- **Unsafe usage:** any `*Must*` whose precondition depends on
  runtime / attacker-controlled data (parsed file content, network
  bytes, `/proc` or `/etc` text, CLI argv, JSON / KvConfig values)
  without a preceding bounds check that statically dominates the
  call. Crafted truncated input becomes a denial-of-service: the
  process aborts on data, not bugs. Use the non-`Must` variant and
  bail (`LOG_ERROR` + return `saved_si` / sentinel) instead.
- **Format strings are an exception by policy, not by type.** The
  `Fmt` parsers in `Io.c` use `*Must*` over the format string and
  rely on every caller passing a C string literal. The day a `Fmt`
  API grows a caller that pipes runtime text into the format
  argument, those `*Must*` sites become DoS vectors — flag and
  convert them.
- **Audit hook:** when adding a `*Must*` callsite, name in a comment
  *what* proves the precondition (the peek above, the
  `RemainingLength` guard, the `elf_range_ok` / `cmdsize` window).
  If you cannot name it, you are introducing an UNSAFE site.

## Documentation

- Every public function and macro gets a doc comment with **`SUCCESS:`**
  and **`FAILURE:`** lines. Both lines describe the full behaviour —
  return value, control flow, and state effects — not just what the
  function returns.
- **Simple field-accessor macros are exempt.** A macro whose body is a
  single field read — `VecLen(v) -> (v)->length`, `MapCapacity(m) ->
  (m)->capacity`, `ListHead(l) -> (l)->head`, `BitVecData(bv) ->
  (bv)->data` — needs only the one-line description and a `TAGS` line.
  These cannot fail, don't mutate, and the `SUCCESS:` / `FAILURE:`
  block would be pure boilerplate. The exemption is narrow: anything
  that branches, validates, allocates, or otherwise has observable
  failure modes (`StrEmpty` does, despite being short) keeps the
  full `SUCCESS:` / `FAILURE:` block.
- **Comments explain *why*, not *what*.** A well-named identifier
  already says what; comments should add the non-obvious reason. Don't
  reference the current PR, ticket number, or caller — those rot.
- Default to no in-code comment. Add one only when removing it would
  confuse a future reader.
- **Public docs reference only the public surface.** When you document
  a `PascalCase` macro or function, don't name the underlying
  `snake_case` backend in the prose ("calls `foo_init_inner` to ...").
  The backend is an implementation detail and rots when internals
  change. The public API is the user's contract; the helper it
  happens to expand to is not.

## Testing

- **Tests live outside every container's namespace.** That means
  tests read `Str` / `Buf` / `Vec` / `BitVec` / `Map` / `List` state
  through the public accessor macros (`StrBegin` / `StrLen` /
  `VecBegin` / `VecLength` / `VecAt` / `VecPtrAt` / `BitVecLen` /
  `BitVecData` / `MapPairCount` / `MapCapacity` / `ListHead` /
  `ListTail` / ...), never by indexing into `.data` / `.length` /
  `.capacity` / `.head` / etc. directly. Adding a new accessor is the
  right answer when a test wants something the accessors don't cover
  yet.
- **`.Deadend` tests are the carve-out.** Files in
  `Tests/Std/*.Deadend.c` and other intentional-corruption tests
  exercise the validators by violating an invariant — bypassing
  capacity bookkeeping, scrambling a magic value, overrunning a buffer.
  These tests write fields directly because that's the whole point of
  the test.
- **Fixture-local owned storage** (e.g. `Vec.Complex.c`'s `char *name`
  / `int *values` inside a `ComplexItem` that exercises
  `VecInitWithDeepCopy` callbacks) is fine as raw pointers because the
  whole point of the fixture is to model an arbitrary T whose nested
  allocations are managed *outside* the container. A comment in the
  fixture explains the intent.
- **One aborting operation per deadend test function.** The harness
  `longjmp`s out of the test on the *first* `LOG_FATAL` / abort (see
  `Tests/Util/TestRunner.c::test_deadend`), so any aborting operation
  after the first is dead code that certifies nothing. A deadend
  function that performs two or more independently-aborting operations
  only ever exercises the first; split it into one function per
  contract. Setup (`Init` / `Push` / `Alloc` / `WriteFmt`) and the
  post-abort `Deinit` tail don't count — only operations that are
  themselves expected to abort. With exactly one such operation per
  function, "an abort happened" is the same statement as "the *right*
  abort happened".

## Compiler flags (mandatory)

These are baked into `meson.build`'s `common_c_args`; downstream consumers
that bypass the meson build must enable equivalents themselves or the
`_Generic` dispatch the convention relies on will silently mis-match.

- **`-Wwrite-strings`** (gcc, clang, clang-cl) — gives string literals
  the type `const char *` (= `Zstr`) on these toolchains so a `char *`
  variable or parameter cannot be silently initialised from a literal.
  This catches code-smell ("I have a mutable buffer where a `Zstr`
  belongs") at the call site rather than at runtime.
- **`/Zc:strictStrings` is NOT useful in C mode.** Microsoft's docs are
  explicit: the flag enforces C++ const qualifications on string
  literals and has no effect on C compilation. Pure-C MSVC therefore
  follows the C standard, which types literals as `char[N]` decaying
  to `char *`. To stay portable across gcc/clang/MSVC, every
  `_Generic` dispatch on a string-shaped argument inlines a `char *`
  synonym arm next to the `Zstr` arm — see the "C-strings everywhere
  use `Zstr`" rule.
- **`-Wuninitialized` / `-Wmaybe-uninitialized` / `-Werror=` on both** —
  enforces the "initialise at declaration" convention. Listed for
  completeness; not new in this section.

Verified compiler behaviour:

- GCC and Clang: empirically tested. With `-Wwrite-strings`,
  `_Generic((literal), Zstr: ...)` matches; without it, the literal
  is `char *` and only the `char *` arm matches.
- MSVC (cl) in C mode: empirically observed. String literal is
  `char *` regardless of `/Zc:strictStrings`. The `char *` arm is the
  only one that matches; the `Zstr` arm is unreachable for literals
  but still useful for `Zstr`-typed locals / parameters.
- clang-cl honours both `-W` and `/Zc` spellings; the codebase passes
  both for parity.

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
