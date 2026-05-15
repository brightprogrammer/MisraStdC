# FUTURE-PLANS

Parking lot for items we've identified during real implementation work but deferred to avoid going down rabbit holes. One line per item — if it grows past a line, it deserves its own doc or a real ticket. Append, don't refactor.

## Allocators
- DebugAllocator: wire `PageProtect(PROT_NONE)` UAF mode behind a `force_page_backing` config; routes through `PageAllocator` instead of the user-supplied parent.
- DebugAllocator: optional mutex field for thread safety (single-threaded only today).
- DebugAllocator: replace libc `backtrace()` + `dladdr` with a DWARF unwinder so static functions resolve in leak reports.
- Allocator vtable: per-call alignment parameter on `AllocatorAlloc` / `AllocatorRealloc` (currently fixed at allocator init).
- Allocator vtable: split `resize` (in-place, returns bool) vs `remap` (may move, returns ptr-or-NULL); keep `reallocate` as the convenience that cascades.
- `StackFallbackAllocator`: composition wrapper that tries a fixed stack buffer first then falls back to a parent allocator.
- `LoggingAllocator`: composition wrapper that logs every alloc/realloc/free on a parent.
- `ThreadSafeAllocator`: composition wrapper that adds a mutex around any parent.
- `GuardPageAllocator`: per-allocation `mmap` + `mprotect(PROT_NONE)` flanking guard pages for overflow-traps-instantly testing.

## Sys / Networking
- `Sys/Socket`: replace libc `getaddrinfo` with an in-tree DNS resolver (eliminate libc dependency).
- `Sys/Socket`: Windows port — currently `#error` on `_WIN32` (needs `SOCKET` vs `int` reconciliation, `WSAStartup`, `closesocket`, `WSAPoll`).
- `Sys/Socket`: move `SocketPoll` from `poll()` to `epoll` / `kqueue` for >1000-fd scale.
- `Sys/Backtrace`: consider symbolize-once cache so repeated `FormatStackTrace` calls don't redo `dladdr` work.

## Beam (reverse proxy)
- Multi-connection concurrency — currently one connection serviced at a time.
- TLS termination — needs an external library (mbedTLS / LibreSSL) or a serious in-tree commitment.
- HTTP/2 and WebSockets — parser today is HTTP/1.1-only.
- Host-header rewrite and routing rules — currently passes everything through to one upstream.
- Config file (KvConfig-based) instead of `--listen` / `--upstream` flags.

## Containers
- `Vec`: add `count * aligned_size` overflow guard at entry of `insert_range_into_vec` / `insert_range_fast_into_vec` / `remove_range_vec` / `fast_remove_range_vec` — defensive, not exploitable on current call paths.

## Language / Macros
- `Scope`: `ScopeReturn` / `ScopeGoto` via `setjmp` / `longjmp` for clean early-return from inside a `Scope` block.

## Parsers
- Port `Bin/ElfInfo.c` onto `Parsers/Elf` so its local enum definitions are removed and `Parsers/Elf.h` can rejoin the `Misra.h` umbrella.
- Extend `Parsers/Elf` to ELF32 + big-endian (v1 is ELF64-LSB only).
- Add DWARF 5 support to `Parsers/Dwarf` (`.debug_line` header changed to entry-format records; current parser silently skips v5 CUs).
- Add DWARF 64-bit length form to `Parsers/Dwarf` (`0xffffffff`-prefixed initial length; rare on Linux but used on macOS / large binaries).
- `Parsers/Dwarf` v2: `.debug_info` + `.debug_abbrev` for function-name attribution beyond ELF symbols, including inlined-frame expansion.
- Wire `DwarfLines` into `Sys/Backtrace::FormatStackTraceWith` so rendered traces append `(file:line)`.

## Naming / Platform
- `FileGetSize` / `ProcGetCurrentId` carry namespace prefixes only to avoid WINAPI macro collisions (`GetFileSize`, `GetCurrentProcessId`). Consider `#undef`'ing the WINAPI macros inside the `Sys/*` translation units and reverting to the cleaner bare names.
