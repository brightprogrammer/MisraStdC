# MisraStdC

[![Linux Build](https://github.com/brightprogrammer/MisraStdC/actions/workflows/test-linux.yml/badge.svg?branch=master)](https://github.com/brightprogrammer/MisraStdC/actions/workflows/test-linux.yml)
[![macOS Build](https://github.com/brightprogrammer/MisraStdC/actions/workflows/test-macos.yml/badge.svg?branch=master)](https://github.com/brightprogrammer/MisraStdC/actions/workflows/test-macos.yml)
[![Windows MSVC](https://github.com/brightprogrammer/MisraStdC/actions/workflows/test-windows-msvc.yml/badge.svg?branch=master)](https://github.com/brightprogrammer/MisraStdC/actions/workflows/test-windows-msvc.yml)
[![Windows LLVM](https://github.com/brightprogrammer/MisraStdC/actions/workflows/test-windows-llvm.yml/badge.svg?branch=master)](https://github.com/brightprogrammer/MisraStdC/actions/workflows/test-windows-llvm.yml)
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
- [Freestanding (Libc-Free Binaries)](#freestanding-libc-free-binaries)
- [Choosing an Allocator](#choosing-an-allocator)
- [Six Core Ideas](#six-core-ideas)
  - [1. Allocators define shared lifetimes, not per-object state](#1-allocators-define-shared-lifetimes-not-per-object-state)
  - [2. `Scope` is lexical RAII, in plain C](#2-scope-is-lexical-raii-in-plain-c)
  - [3. Every object carries a magic; type-confusion dies at the dispatch](#3-every-object-carries-a-magic-type-confusion-dies-at-the-dispatch)
  - [4. Macros + `_Generic` give you generics without a template compiler](#4-macros--_generic-give-you-generics-without-a-template-compiler)
  - [5. Fallible by default, `Must`-variants for the unrecoverable](#5-fallible-by-default-must-variants-for-the-unrecoverable)
  - [6. One header, configured at build time](#6-one-header-configured-at-build-time)
- [Container Tour](#container-tour)
  - [Vec](#vec)
  - [Str](#str)
  - [Buf](#buf)
  - [List, Graph](#list-graph)
  - [BitVec, Int, Float](#bitvec-int-float)
- [Formatted I/O](#formatted-io)
- [Parsers](#parsers)
  - [JSON](#json)
  - [KvConfig](#kvconfig)
  - [Binary parsers: Elf, Macho, Pe, Pdb](#binary-parsers-elf-macho-pe-pdb)
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

- **Six concrete allocators**, all user-owned, all per-descriptor:
  `HeapAllocator` (binned), `PageAllocator` (raw OS pages), `ArenaAllocator`
  (bump), `SlabAllocator` (growable fixed-slot pool), `BudgetAllocator`
  (caller-buffer, fixed-budget, no-growth), and `DebugAllocator` (ASan-style
  leak / double-free / canary-overflow / use-after-free detection — opt-in
  process-wide via the `default_alloc_debug` build flag).
- **Generic containers** built on a shared runtime: `Vec(T)`, `List(T)`,
  `Map(K,V)`, `Graph(T)`, `BitVec`, `Str` (= `Vec(char)`), `Strs` (= `Vec(Str)`),
  `Buf` (= `Vec(u8)`, with cursor-style binary reads via `BufIter`).
- **Arbitrary-precision arithmetic:** `Int` on top of `BitVec`, `Float` on top
  of `Int`, with radix-string conversion, modular arithmetic, primality,
  decimal-exact add/sub/mul/div.
- **Type-aware formatted I/O** with Rust-style `{}` placeholders; one
  `WriteFmt(...)` works for `int`, `f64`, `Str`, `Int`, `Float`, `BitVec`,
  C strings — all dispatched at compile time via `_Generic`.
- **JSON and key-value config parsers** as opt-in features.
- **Binary-format parsers** for `Elf`, `Macho`, `Pe`, and `Pdb`, all walking
  bytes through `BufIter` + `BufReadFmt` — no `libelf`, no `dbghelp`. Used
  by the in-tree backtrace symbolizer.
- **Cross-platform system utilities:** subprocess control, directory listings,
  mutexes, environment access, current-process info, current-executable path.
- **Feature flags** that drop whole subsystems from the static library and
  from the installed header set. Disabling `bitvec`, `list`, `map`, `graph`,
  `int`, `float`, `parser_json`, etc. removes their `.c` files from
  `libmisra_std.a` and their `.h` files from the install prefix.
- **Libc-free shipping binaries** on Linux, macOS, and Windows. The Bin/
  tools (`beam`, `resolve`) link against zero libc by default: direct
  syscalls on
  Linux + macOS (XNU BSD subset on Mac, custom `_start`, in-tree mem* and
  setjmp), `/NODEFAULTLIB` + custom `misra_start` entry on Windows. CI
  asserts the import table per OS — see
  [Freestanding (Libc-Free Binaries)](#freestanding-libc-free-binaries).

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

If you only need `Vec`, `Str`, `Buf`, `Io`, and the default heap allocator,
turn the rest off. Every optional feature has to be disabled by name --
several of them are pulled in by other defaults (e.g. `alloc_debug`
auto-enables `map` + `sys_backtrace`, which in turn pulls in `parser_elf`
+ `parser_dwarf`), so a partial disable list still ships most of the
library. Note also that `file`, `iter`, and `alloc_arena` cannot be
disabled today -- `Io.c` uses `StrIter` internally, the binary parsers
(`elf`/`pe`/`macho`) slurp via `FileRead`, and `Sys/Dns` uses
`ArenaAllocator` for its in-arena resolution. The dependency edges
aren't optional yet.

```bash
meson setup builddir-min \
    -Dalloc_slab=false -Dalloc_budget=false -Dalloc_debug=false \
    -Dbitvec=false -Dlist=false -Dmap=false -Dgraph=false \
    -Dint=false -Dfloat=false \
    -Dsys_dir=false -Dsys_proc=false -Dsys_socket=false \
    -Dsys_procmaps=false -Dsys_symresolve=false -Dsys_backtrace=false \
    -Dsys_dns=false \
    -Dparser_json=false -Dparser_kvconfig=false -Dparser_http=false \
    -Dparser_elf=false -Dparser_dwarf=false \
    -Dparser_pe=false -Dparser_pdb=false -Dparser_macho=false \
    -Dparser_dns=false
ninja -C builddir-min libmisra_std.a
ninja -C builddir-min install
```

The resulting `libmisra_std.a` contains only the foundation translation
units plus the three load-bearing optional ones noted above, and the
install prefix shrinks to about a third of the default build. Adding
`int` automatically pulls in `bitvec`; adding `graph` pulls in `vec`
(already foundation); adding `parser_kvconfig` pulls in `map`; enabling
`alloc_debug` pulls in `map` + `sys_backtrace` (and, on Linux, the full
ELF/DWARF symbolizer chain behind it). Dependencies between features
resolve transitively at configure time.

Note: the in-tree test suite assumes the default (everything-on) build.
Running `ninja test` against a minimal config will fail to link tests
for the disabled features; build `libmisra_std.a` directly (as above)
or stick with the default configuration when running the test suite.

---

## Feature Flags

| Flag                              | Adds                                                                                                                | Auto-pulls                       |
|-----------------------------------|---------------------------------------------------------------------------------------------------------------------|----------------------------------|
| `alloc_arena`                     | `ArenaAllocator`                                                                                                    | —                                |
| `alloc_slab`                      | `SlabAllocator` (growable fixed-slot pool)                                                                          | —                                |
| `alloc_budget`                    | `BudgetAllocator` (caller-buffer, fixed-budget)                                                                     | —                                |
| `alloc_stats`                     | Per-`Allocator` byte / call / peak counters                                                                         | —                                |
| `alloc_debug`                     | `DebugAllocator` (leak / double-free / canary-overflow / stack-trace tracking)                                      | `map`, `sys_backtrace`           |
| `default_alloc_debug`             | Alias `DefaultAllocator` to `DebugAllocator` everywhere — drop-in ASan/MSan-style with no call-site changes         | `alloc_debug`                    |
| `default_alloc_debug_page_backed` | Layer page-backed UAF detection on top: every alloc consumes whole pages, free `PROT_NONE`s the region              | `default_alloc_debug`            |
| `bitvec`                          | `BitVec` packed bit container                                                                                       | —                                |
| `list`                            | `List(T)` doubly-linked list                                                                                        | —                                |
| `map`                             | `Map(K,V)` hash map                                                                                                 | —                                |
| `graph`                           | `Graph(T)` directed graph (uses `Vec` runtime helpers)                                                              | —                                |
| `int`                             | Arbitrary-precision integer `Int`                                                                                   | `bitvec`                         |
| `float`                           | Arbitrary-precision decimal `Float`                                                                                 | `int → bitvec`                   |
| `file`                            | `File` cross-platform handle + `FileRead{,AndClose}` / `FileWrite{,AndClose}` slurp helpers                          | —                                |
| `iter`                            | Generic `Iter(T)` iteration helpers                                                                                 | —                                |
| `sys_dir`                         | `DirGetContents(...)` and friends                                                                                   | —                                |
| `sys_proc`                        | `ProcCreate(...)` / spawn / wait                                                                                    | —                                |
| `sys_socket`                      | BSD-sockets API (`Listener`, `Socket`, `SocketPoll`)                                                                | —                                |
| `sys_dns`                         | `DnsResolver` (in-tree A/AAAA + getaddrinfo-free name resolution)                                                   | `sys_socket`, `parser_dns`       |
| `sys_procmaps`                    | `ProcMaps` (process module-map snapshot for the symbolizer chain)                                                   | —                                |
| `sys_symresolve`                  | `SymbolResolver` (cross-platform module + address-to-name chain)                                                    | `sys_procmaps`                   |
| `sys_backtrace`                   | `CaptureStackTrace` / `FormatStackTrace`, plus the in-tree symbolizer chain on Linux                                | `parser_elf`, `parser_dwarf` (Linux); `parser_macho` (macOS); `parser_pdb`, `parser_pe` (Windows) |
| `parser_json`                     | JSON read/write                                                                                                     | —                                |
| `parser_kvconfig`                 | Key-value config parser                                                                                             | `map`                            |
| `parser_http`                     | HTTP/1.1 request + response parsing / serialization (transport-agnostic)                                            | —                                |
| `parser_dns`                      | DNS wire-format encode/decode per RFC 1035                                                                          | —                                |

Every enabled feature also defines `FEATURE_<NAME>` (= 1) in the
generated `Misra/Config.h`. User code can `#if FEATURE_BITVEC` to compile
against a partial install.

The foundation — always built, can't be opted out — is:
`Sys`, `Sys/Mutex`, `Std/Log`, `Std/Memory`, `Std/Zstr`, `Std/Allocator`
(core + `_Os` + Page + Heap = `DefaultAllocator`), `Std/Container/Vec`,
`Std/Container/Str`, `Std/Io`, `Std/Prng`, `Std/ArgParse`. `LOG_FATAL`
formats its message through `Str` + `Io`, so those two are foundation by
transitive necessity; `Prng` and `ArgParse` are kept in the foundation
because the shipping Bin/ tools (`beam`, `resolve`) depend on them and
they have no platform/feature gating to make optional.

### What it actually costs

The feature-flag system is not theatre — disabled features really do leave
the static library. Three measurements per configuration, all gcc 15.2,
x86_64, Linux, `meson setup --buildtype=minsize -Db_sanitize=none`:

1. **Archive raw** — `libmisra_std.a` straight out of the build, including
   debug section metadata, relocation tables, and unresolved-symbol entries.
2. **Archive stripped** — same archive after `strip --strip-unneeded`.
3. **Consumer stripped** — a tiny program that uses `Vec`, `Scope`, and
   `WriteFmtLn` linked against the archive and stripped. This is the
   realistic number for "how much of the library actually ends up in my
   binary."

| Configuration                                                       | Archive raw | Archive stripped | Consumer stripped |
|---------------------------------------------------------------------|------------:|-----------------:|------------------:|
| Foundation only                                                     |     501 KB  |       **222 KB** |        **131 KB** |
| Default (everything, `alloc_stats` on)                              |   1 549 KB  |       **662 KB** |        **247 KB** |

A `Vec`-using consumer ends up at **127 KB** against the foundation-only
build, **243 KB** against the full build — the linker pulls in only the `.o`
files your code references, regardless of how big the archive gets. Disabling
a feature you don't need really does keep its code out of your binary.

---

## Freestanding (Libc-Free Binaries)

The library's identity is freestanding. Shipping `Bin/` tools link
against zero libc on every supported OS. Not "minimal libc", not
"static libc" — **no libc**. Every syscall goes direct to the kernel;
compiler-emitted helpers (`memcpy`, `memset`, `setjmp`, `__chkstk`,
stack canaries) come from in-tree sources. The "platform" DLLs that
remain are the OS's stable kernel ABI, not the C library: direct
Linux syscalls, the BSD subset of XNU on macOS via `syscall` /
`svc #0x80`, and `kernel32`/`ws2_32`/`dbghelp` on Windows.

The one exception is documented below: ASan / UBSan builds
automatically fall back to a hosted configuration because libsanitizer
is libc-resident. There's no user-facing toggle — the build derives
the freestanding-vs-hosted shape from `-Db_sanitize=...` alone.

### What survives the diet, per OS

| Symbol category                          | Linux                                | macOS                                          | Windows (clang-cl)                  |
|------------------------------------------|--------------------------------------|------------------------------------------------|-------------------------------------|
| `open`/`close`/`read`/`write`/`mmap`/…    | direct syscalls                      | direct XNU syscalls                            | `kernel32!CreateFileA` / `ReadFile` |
| `socket`/`bind`/`recvfrom`/`poll`/…       | direct syscalls                      | direct XNU syscalls                            | `Ws2_32!WSARecv` / `WSAPoll`        |
| `fork`/`execve`/`waitpid`/`pipe`/`kill`   | direct syscalls                      | direct XNU syscalls                            | `kernel32!CreateProcessA`           |
| `sigaction` / signal handling             | direct `rt_sigaction` + custom restorer | direct XNU `sigaction` + custom `sa_tramp`     | `kernel32!SetConsoleCtrlHandler`    |
| `getdents` / directory enumeration        | direct `getdents64`                  | direct `getdirentries64` (Darwin struct layout)| `kernel32!FindFirstFile`            |
| `errno`                                   | `-rc` from syscall return            | `-rc` from carry-flag handling                 | `GetLastError()`                    |
| `memcpy`/`memset`/`memmove`/`memcmp`/`bzero` | in-tree byte-loop forwarders (`_Freestanding.c`) | same                                           | same                                |
| `setjmp`/`longjmp`                        | hand-written naked asm per ABI       | (test harness keeps libSystem)                 | (test harness keeps UCRT)           |
| Process entry point                       | in-tree `_start` (drops `crt1.o`)    | (libSystem; no Mac freestanding `_start` yet)  | custom `misra_start` + `mainCRTStartup` drop |
| Stack-protector helpers (`__stack_chk_*`) | `-fno-stack-protector` drops emission | same; plus `___chkstk_darwin` stub             | `/GS-` drops emission; stub for residual `__security_cookie` |
| `__chkstk` / stack-probe                  | not emitted                          | stubbed                                        | `-mno-stack-arg-probe` + stub       |
| Allowed "platform" surface                | nothing — `ldd` says `statically linked` | `_dyld_*` (Mach-O image enumeration for `Sys/Backtrace`) | `KERNEL32.dll`, `WS2_32.dll`, `DBGHELP.DLL`, `ADVAPI32.dll` (+ the meson-auto-injected user32/gdi32/etc. when actually referenced) |

CI enforces this per OS. Linux freestanding asserts `nm -u` is empty and
`ldd` reports `statically linked`. macOS freestanding asserts `nm -u`
contains nothing outside the four allowed `__dyld_*` entries. Windows
freestanding asserts `llvm-readobj --coff-imports` lists only DLLs from
the platform allowlist — `ucrtbase`, `vcruntime`, `msvcp*`, and the
`api-ms-win-crt-*` UCRT facade DLLs are explicitly forbidden.

### Why this matters

Static linking against glibc is impossible on Linux without exotic
patches. Bundling musl works but trades one libc for another and brings
its own malloc / setjmp / mem* implementations into your binary.
libSystem on macOS isn't even technically replaceable — Apple lists it
as the only sanctioned syscall ABI. UCRT on Windows ships with the OS
but still couples your binary to its FILE / locale / exit-handler /
exception machinery. Each of those is a maintenance dependency you
didn't ask for.

Going libc-free means: smaller binaries, deterministic init (no CRT
constructors running before `main`), no implicit globals (no `errno`
TLS slot, no `__progname`), no surprise allocations from `printf`
formatting, and a build that works the same way whether you're cross-
compiling for an embedded target or running on the host. The OS
kernel ABI (Linux syscall numbers, XNU's BSD class, the Win32 API
surface) is more stable than any libc API — that's what we depend on
instead.

### How it works

- **`Source/Misra/_Syscall.h`** — per-OS / per-arch asm wrappers
  (`misra_sys0..6` for Linux, BSD-class-prefixed equivalents for Darwin)
  plus the syscall-number tables. One file gates whether
  `FEATURE_DIRECT_SYSCALL` is on (Linux x86_64/aarch64 + Darwin
  x86_64/aarch64).
- **`Source/Misra/_StartLinux.c`** — hand-written `_start` assembly that
  reads `argc`/`argv`/`envp` off the stack the way the Linux ELF kernel
  hands it to you, calls `main`, then `SYS_exit_group`. Replaces
  `crt1.o` via `-nostartfiles`.
- **`Source/Misra/_StartWin.c`** — Windows analogue: `misra_start` calls
  `kernel32!GetCommandLineA`, tokenises argv, calls `main`, then
  `ExitProcess`. Wired in via `/ENTRY:misra_start`.
- **`Source/Misra/_Freestanding.c`** — compiler-emitted intrinsics:
  `memcpy`/`memmove`/`memset`/`memcmp` byte-loop forwarders (cross-OS);
  `setjmp`/`longjmp` naked-asm with project-internal `jmp_buf` layout
  (Linux only); `bzero` (Linux + Mac); `__chkstk_darwin` stub (Mac).
- **`Source/Misra/_WinStubs.c`** — Windows compiler-runtime stubs:
  `__security_cookie`, `__security_check_cookie`, `__chkstk`, `_fltused`,
  `__imp___stdio_common_vsprintf`. Linked per Bin/-target (kept out of
  `libmisra_std.a`) to avoid clashes with `msvcrtd.lib` in test
  binaries that keep UCRT.
- **`Bin/Beam.c`** sigaction — Linux uses direct `rt_sigaction` with a
  custom restorer trampoline on x86_64. Darwin uses direct XNU
  `sigaction` (#46) with a hand-rolled signal trampoline that calls
  `sigreturn` (#184) after invoking the handler. Windows uses
  `SetConsoleCtrlHandler` (kernel32, not UCRT's `signal()`).

### Sanitizer builds (the one auto-hosted case)

Sanitizer builds (`-Db_sanitize=address,undefined` or `=address`) keep
the full libc — the sanitizer runtimes (`libasan`, `libubsan`,
`clang_rt.asan-x86_64`) live inside libsanitizer, which is a libc-side
library. `-nostdlib` would drop them, breaking the link. Sanitizers
also have their own `mem*` interceptors that would clash with our
in-tree `_Freestanding.c` overrides, and they expect a normal stack
layout including the canary slots the freestanding path replaces.

So when any sanitizer is active, the build automatically drops out of
freestanding: `_Freestanding.c` / `_StartLinux.c` / the in-tree
stack-protector helpers are skipped, and `Bin/` tools link the full
libc + libsanitizer like a normal hosted program. This is keyed
internally off `get_option('b_sanitize') != 'none'`; there is no
user-facing toggle for it. The shipping build is always freestanding.

ASan / UBSan aren't redundant with the project's own `DebugAllocator`
— they cover bug classes the allocator structurally can't see (stack
overflows, global overflows, uninitialised reads, integer overflow,
misaligned loads, use-after-scope). They're the reason CI runs each
OS in two flavours:

- `sanitized` — full libc + ASan + UBSan, exercises parser / fuzz
  paths under undefined-behaviour instrumentation.
- `freestanding` — the shipping configuration, asserts on `nm -u` /
  `dumpbin`-style allowlist (Linux: empty `nm -u` + `ldd` reports
  `statically linked`; Mac: only `__dyld_*` entries; Windows: only
  platform-DLL imports, no `ucrtbase*` / `vcruntime*` / `msvcp*`).

A regression in either fails CI independently.

### Limitations and unfinished pieces

- **macOS aarch64 sigtramp** is verified end-to-end (SIGINT/SIGTERM
  deliver cleanly, process exits 0). The x86_64 trampoline is written
  but untested — GitHub's `macos-latest` is Apple Silicon.
- **Linux aarch64** paths are gated correctly but only Linux x86_64 has
  full CI coverage.
- **Windows freestanding is clang-cl only.** MSVC bundles its
  compiler-runtime helpers inside `libcmt` and can't be cleanly
  separated. The MSVC CI job runs the standard (with-UCRT) path.
- **Test harness `setjmp`/`longjmp`** still link libc on Mac and
  Windows. Splitting the harness out would be the next freestanding
  push if test binaries need to be libc-free too.

---

## Choosing an Allocator

MisraStdC ships six allocators. Each has a sweet spot; numbers cited below come from [Benchmark/README.md](Benchmark/README.md) (rerun `Benchmark/Scripts/run.py` on your host to refresh).

| Your workload | Use | Why |
|---|---|---|
| Mixed sizes, varied lifetimes | `HeapAllocator` | The general-purpose default. Multi-size bins + sorted descriptor arrays. |
| One fixed size, high churn | `SlabAllocator(size)` | Bitmap-backed pool, slot lookup by `ptr & ~PAGE_MASK`. O(log N) free. |
| Per-scope (request, frame, parser run) | `ArenaAllocator` + `Reset` | Bump-pointer alloc; `Reset` releases the whole batch in O(1). |
| Page-aligned regions (shm, JIT, mmap'd files) | `PageAllocator` | One mmap per alloc. Foundation everything else builds on. |
| Hard memory quota | `BudgetAllocator(wrap, cap)` | Wraps any of the above; refuses allocations once the cap is hit. |
| Hunting leaks / double-free in dev/CI | `DebugAllocator(wrap)` | Wraps any of the above; tracks every live alloc with capture-site backtrace. |

### When to pick each one

**`HeapAllocator`** — default for unknown/mixed workloads. Beats glibc on >= 4 KiB allocations (8 ns/pair vs glibc's 24 ns where its tcache runs out). Loses to glibc on small sizes (~10 ns vs 4.5 ns) because we keep per-call safety checks they skip. Don't use if your workload is dominated by one size — `SlabAllocator` will beat it.

**`SlabAllocator(size)`** — when allocations are all the same size (linked-list nodes, fixed-shape structs, message buffers). Flat ~9 ns/pair across every slot size, including 4 KiB+ where `HeapAllocator` and glibc go to mmap. Initialize one per size. Refuses non-power-of-2 slot sizes by rounding up (16-byte minimum, max one OS page per slab).

**`ArenaAllocator`** — when allocations share a scope: a request handler, a parser invocation, a frame in a game loop. Bump-allocate is the cheapest individual alloc the library does. `Reset` releases everything in O(1). Also supports **single-deep LIFO rewind** on `free` (the most-recently-bumped pointer rewinds the cursor), so strict alloc-then-free-immediately patterns work without leaking, and arena is actually the fastest MisraStdC allocator on that workload (faster than `SlabAllocator`). Don't use for batch-then-bulk-free workloads — only the last-allocated pointer rewinds; older frees are no-ops, memory grows until `Reset`.

**`PageAllocator`** — when you need page-aligned memory (shared-memory regions, JIT code pages, `mmap`'d files), or as the page source for another allocator. Every alloc is an `mmap` (rounded up to a page); sub-page requests waste the rest of the page. Not a general-purpose allocator.

**`BudgetAllocator`** — wraps another allocator and enforces a hard byte quota. Use when a subsystem must not exceed a memory budget (sandboxed component, resource-limited worker, bounded cache). Adds a quota check and a stats update around every wrapped call.

**`DebugAllocator`** — wraps another allocator and tracks every live allocation with its capture-site backtrace. Use in dev/CI builds to catch leaks, double-frees, and use-after-frees at the point of failure. Substantial per-call overhead (backtrace capture + tracking map insert); don't ship it in production hot paths.

### Bench

Full numbers, methodology, and reproduction steps in **[Benchmark/README.md](Benchmark/README.md)**. To refresh:

```sh
meson setup build -Dbenchmark=true -Dbuildtype=release -Doptimization=3
ninja  -C build
python3 Benchmark/Scripts/run.py build
```

---

## Six Core Ideas

### 1. Allocators define shared lifetimes, not per-object state

Every dynamically-sized object in MisraStdC stores a single `Allocator *`
— a pointer to a **shared** allocator, not a private one. The library
owns no process-wide heap, no thread-local fallback, no implicit "default"
instance; the caller picks the allocator and decides who shares it.

A typed allocator is a struct with state inline on the stack:

```c
DefaultAllocator alloc = DefaultAllocatorInit();   // ~160 B on the stack
Vec(int) a = VecInit(&alloc);
Vec(int) b = VecInit(&alloc);                       // shares the same pool
Vec(int) c = VecInit(&alloc);
...
VecDeinit(&a); VecDeinit(&b); VecDeinit(&c);
DefaultAllocatorDeinit(&alloc);
```

What that pointer says is two things, and **both are about the allocator,
not the object**:

1. **Where memory comes from.** Every allocation / realloc / free routes
   through this allocator. Objects sharing one allocator share its backing
   pool — page reuse, slot reuse, free-list locality for free.
2. **When the memory becomes invalid.** When the allocator dies (or its
   `Scope` ends), every object still pointing at it has dangling `data`.
   The allocator must outlive every object that uses it.

So the natural mental model: **one allocator per logical lifetime.** Per
request, per file parse, per game tick, per session. Everything created in
that work-unit shares an allocator and dies with it. This is not a small
optimisation — it is the deliberate substitute for tracking per-object
lifetimes by hand.

The library ships six backends:

- **`PageAllocator`** — raw `mmap` / `VirtualAlloc`. The foundation under every
  growing allocator, no libc heap.
- **`HeapAllocator`** — power-of-two size-class bins (16–2048 B) plus a
  page-passthrough for large allocations. `DefaultAllocator` is a typedef
  for this. Best fit when you need per-object `free` (long-lived caches,
  arbitrary delete patterns).
- **`ArenaAllocator`** — bump cursor over page-backed chunks. `AllocatorFree`
  is a no-op; everything is released together on `ArenaAllocatorDeinit`.
  Best fit when "everything dies together" — parsers, per-request work,
  per-frame scratch.
- **`SlabAllocator`** — fixed-size slots with an intrusive free list, grows
  by pulling more page-backed slabs on demand. Best fit for homogeneous
  workloads (e.g. a list of fixed-size nodes).
- **`BudgetAllocator`** — caller hands in a fixed memory region at init;
  slots are carved out of it and never replenished. Best fit for
  freestanding contexts or hard caps.
- **`DebugAllocator`** — wraps an internally-owned `HeapAllocator` and a
  per-thread tracking map so every live allocation is bookkept. Catches
  leaks (reported with the captured allocation stack trace at
  `DebugAllocatorDeinit` time), double-frees, and canary-pattern overflow
  past the user region. Optional `force_page_backing` config routes every
  allocation through `mmap` and `PageProtect(PROT_NONE)`s the region on
  free so use-after-free traps with SIGSEGV at the moment of the bug. Set
  the `default_alloc_debug` meson option to make `DefaultAllocator`
  silently become a `DebugAllocator` everywhere — drop-in ASan/MSan-style
  detection with no call-site changes, no globals, init-by-value like the
  other backends.

The library defines a small `_Generic` whitelist (`ALLOCATOR_OF`) so any of
these can pass anywhere an `Allocator *` is expected.

**Pointer-escape pitfall.** Because containers only hold an `Allocator *`,
they trivially survive being copied or returned, but their backing pages
do not. The rule is:

> **Never return or store a container whose backing allocator lives on
> your stack frame.** The container header is fine, but its `data` will
> dangle the moment the allocator's stack frame is reclaimed. If a value
> needs to outlive the caller, allocate it through an allocator that
> outlives the caller too — usually one passed in by the caller, or a
> longer-lived per-subsystem allocator.

A common variant: putting a `Vec` into a `Map` by ownership transfer
across a `Scope` boundary. The `Map` lives longer than the `Scope`, so
the inserted `Vec`'s `data` points at pages that are about to be unmapped.
Deep-copy-on-insert (`VecInitWithDeepCopy(...)`, `MapInitFull(...)` with
copy callbacks) avoids this — the destination container rebuilds the
storage through its own allocator.

### Memory pressure: every allocator carries its own stats

Each `Allocator` base carries an `AllocatorStats` struct that the dispatch
layer updates on every `allocate` / `reallocate` / `deallocate`:

```c
DefaultAllocator alloc = DefaultAllocatorInit();
Allocator       *a     = ALLOCATOR_OF(&alloc);

Vec(int) v = VecInit(a);
for (int i = 0; i < 10000; i++) VecMustPushBackR(&v, i);

AllocatorStats s = AllocatorGetStats(a);
WriteFmtLn("allocs={}, frees={}, in_use={} B, peak={} B",
           s.allocations, s.deallocations, s.bytes_in_use, s.peak_bytes_in_use);
```

The seven fields are `bytes_requested`, `bytes_in_use`, `peak_bytes_in_use`,
`allocations`, `reallocations`, `deallocations`, and `failed_allocations`.
Counters live on the `Allocator` base, so every typed backend gets
accounting for free — no per-allocator implementation cost. The whole
machinery is gated by the `alloc_stats` feature flag (default on); when
disabled the struct shrinks and the dispatch path drops the counter
updates entirely.

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
(`HEAP_ALLOCATOR_MAGIC`, `PAGE_ALLOCATOR_MAGIC`, ...). Adding a
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
the current build enabled, via `#if FEATURE_<NAME>` checks against the
generated `Misra/Config.h`. If you disabled `parser_json` at configure time,
`<Misra/Parsers/JSON.h>` is neither installed nor pulled in by `Misra.h` —
but everything else is reachable through that one include.

A handful of optional headers stay outside the umbrella because they
expose ELF / DWARF / Mach-O / PE / PDB constant names that downstream
code may already carry: `<Misra/Parsers/Elf.h>`,
`<Misra/Parsers/Dwarf.h>`, `<Misra/Parsers/MachO.h>`,
`<Misra/Parsers/Pe.h>`, `<Misra/Parsers/Pdb.h>`,
`<Misra/Sys/SymbolResolver.h>`, `<Misra/Sys/Backtrace.h>`,
`<Misra/Sys/PdbCache.h>`, and `<Misra/Sys/MachoCache.h>`. Include
those directly when you want the parser, the resolver, the backtrace
formatter, or the symbol caches.

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

    bool starts = StrStartsWith(&text, "Hello");
    bool ends   = StrEndsWith(&text, "!\n");

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

### Buf

`Buf` is `Str`'s binary cousin: a `Vec(u8)` typedef for code that produces or
consumes raw wire bytes — headers, on-disk file formats, network frames.
Same `VecInit` / `VecDeinit` underneath, with a thin layer of binary
read/write primitives on top:

```c
Scope(alloc, DefaultAllocator) {
    Buf bytes = BufInit();
    BufWriteU32LE(&bytes, 0xDEADBEEF);
    BufWriteZstr (&bytes, "hello");

    BufIter it = BufIterFromBuf(&bytes);
    u32     tag = 0;
    BufReadU32LE(&it, &tag);
    const char *msg = BufReadZstr(&it);
    WriteFmtLn("tag=0x{x}, msg={}", tag, msg);

    BufDeinit(&bytes);
}
```

`BufIter` is a cursor over `(data, length)` — separate from the `Buf` itself,
so the same iterator type works for borrowed memory you don't own
(`BufIterFromMemory(ptr, n)`). The read primitives advance the cursor and
return `false` on truncation; no pointer arithmetic in caller code.

When you need a sub-range — say, a length-prefixed record inside a larger
buffer — `IterCarve(&parent, n)` returns a child iterator over the parent's
next `n` bytes without modifying the parent. `IterTruncate(&it, n)` is the
in-place variant: caps the current iterator so only the next `n` bytes are
reachable. Both keep the buffer bounds where the format puts them, instead
of in scattered `if (pos + size > len)` checks.

The format layer in `<Misra/Std/Io.h>` adds four operations on top:
`BufReadFmt(&it, "{<4}{<2}", tag, flags)` consumes typed binary fields from
a cursor; `BufAppendFmt` / `BufWriteFmt` / `BufPatchFmt` emit them. The
format directive `{<Nr}` means little-endian width `N` (1, 2, 4, or 8);
`{>Nr}` is big-endian. Width must match the destination's natural size, so
mismatches are caught at the call site.

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
  (normal output / standard input channel — `FileFromFd(1)` / `FileFromFd(0)`
  on POSIX, the corresponding `GetStdHandle` on Windows)
- `BufReadFmt(&iter, fmt, ...)` / `BufAppendFmt(&buf, fmt, ...)` /
  `BufWriteFmt(&buf, fmt, ...)` / `BufPatchFmt(&buf, offset, fmt, ...)`
  (binary cursor / Buf — `{<Nr}` LE and `{>Nr}` BE only, `N` in `{1,2,4,8}`)

### Files in one call

For the common case of "slurp this path" or "write this Buf to that path",
`<Misra/Std/File.h>` exposes one-shot helpers:

```c
Scope(alloc, DefaultAllocator) {
    Buf payload = BufInit();
    FileReadAndClose("input.bin", &payload);          // open + read + close
    BufAppendFmt(&payload, "{<4}", (u32)0x4D495352);  // patch a trailer
    FileWriteAndClose("output.bin", &payload);
}
```

Both helpers dispatch the path on `Str *` / `char *` and the container on
`Buf *` / `Str *`. `FileRead(&file, &buf)` (and `FileRead(&file, &str)`)
is the lower-level read-to-EOF overload when you already hold a `File`.
`FileGetSize(path)` in `<Misra/Sys/Dir.h>` answers the path-based size
question without an open.

---

## Parsers

### JSON

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

### KvConfig

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

### Binary parsers: Elf, Macho, Pe, Pdb

Four binary-format readers in `<Misra/Parsers/{Elf,MachO,Pe,Pdb}.h>`, all
shaped the same way. Internally each one stores a `Buf` of the raw file
bytes and walks it through `BufIter` + `BufReadFmt`; the parser owns the
bytes for its whole lifetime. The in-tree backtrace symbolizer is the
primary consumer (ELF + DWARF on Linux, Mach-O + dSYM on macOS, PE + PDB
on Windows), but the APIs are usable on their own.

Three construction paths per parser, mirroring `VecInsertL` / `VecInsertR`:

```c
Scope(alloc, DefaultAllocator) {
    // Path-based: parser opens, reads, owns.
    Elf elf = {0};
    if (ElfOpen(&elf, "/usr/bin/ls")) {
        const ElfSection *text = ElfFindSection(&elf, ".text");
        if (text) WriteFmtLn(".text at 0x{x}, {} bytes", text->addr, text->size);
        ElfDeinit(&elf);
    }

    // L-form: hand the parser an existing Buf. The Buf is snapshot'd and
    // zeroed in place, so the caller's local is safe to drop on stack.
    Buf bytes = BufInit();
    FileReadAndClose("vmlinux", &bytes);
    Elf elf2 = {0};
    ElfOpenFromMemory(&elf2, &bytes);   // *bytes is now zeroed
    ElfDeinit(&elf2);

    // R-form: parser copies; caller's data is untouched.
    Elf elf3 = {0};
    ElfOpenFromMemoryCopy(&elf3, raw_data, raw_size);
    ElfDeinit(&elf3);
}
```

The L-form's ownership-transfer semantics are the reason this isn't just a
single "from memory" constructor: passing `&bytes` makes it visible at the
call site that the parser is taking the Buf, and the zero-on-take leaves
no dangling alias for the caller to misuse on the next line.

`Macho`, `Pe`, and `Pdb` carry the same `Open` / `OpenFromMemory` /
`OpenFromMemoryCopy` / `Deinit` shape, plus the lookups each format needs
— `MachoFindSection` / `MachoResolveAddress`, `PeFindSection` plus the
`codeview` field for matching the binary to its PDB, and `PdbResolveRva`
for function lookup by image-relative address.

---

## System Utilities

Cross-platform wrappers for the OS surface: subprocesses (`Sys/Proc`),
directory walks (`Sys/Dir`), mutexes (`Sys/Mutex`), errno translation
(`Sys/Errno`), BSD sockets (`Sys/Socket`), in-tree DNS resolution
(`Sys/Dns`), process module maps (`Sys/ProcMaps`), the
symbolizer-resolver chain (`Sys/SymbolResolver`), and stack-trace
capture / formatting (`Sys/Backtrace`).

```c
// Verified with /bin/head: writes a value to the child, expects the same
// echoed back, prints the round-trip result.
int main(int argc, char **argv, char **envp) {
    (void)argc;
    Scope(alloc, DefaultAllocator) {
        Proc proc;
        ProcInit(&proc, argv[1], argv + 1, envp);  // alloc defaulted via Scope
        ProcWriteToStdinFmtLn(&proc, "value = {}", 42);

        i32 val = 0;
        ProcReadFromStdoutFmt(&proc, "value = {}", val);
        WriteFmtLn("got value = {}", val);

        ProcWaitFor(&proc, 1000);
        ProcDeinit(&proc);
    }
}
```

Backtrace capture works the same way and routes through the in-tree
ELF / Mach-O / PE / PDB / DWARF parsers — no `libunwind`, no `addr2line`:

```c
Scope(alloc, DefaultAllocator) {
    StackFrames frames = VecInitT(frames, alloc);
    CaptureStackTrace(&frames, /*skip=*/0);
    Str rendered = StrInit(alloc);
    FormatStackTrace(&rendered, &frames, alloc);
    WriteFmtLn("{}", &rendered);
    StrDeinit(&rendered);
    VecDeinit(&frames);
}
```

---

## Contributing

Contributions are welcome.

Read [`CODING-CONVENTIONS.md`](CODING-CONVENTIONS.md) first — it captures
the project's rules on naming, allocator handling, the libc-free mindset,
`_Generic` dispatch, macro hygiene, Zstr-vs-Str preference, testing,
documentation, and the smaller carve-outs that come up in code review.
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
