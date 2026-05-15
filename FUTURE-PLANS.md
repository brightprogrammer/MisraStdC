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
- Validate `.gnu_debuglink` CRC32 against the sidecar's contents before using it (currently only Build-ID lookup is verified end-to-end).
- Add an integration test that strips a binary and reattaches a separate-debug-file via `objcopy --add-gnu-debuglink`, then asserts that SymbolResolver still resolves symbols + source lines through the sidecar.
- `Parsers/Dwarf`: walk `DW_AT_ranges` for discontiguous subprograms (currently they're skipped — only contiguous `low_pc`/`high_pc` functions land in `DwarfFunctions`).
- `Parsers/Dwarf`: follow `DW_AT_specification` / `DW_AT_abstract_origin` to attach a name to defining subprogram DIEs that themselves carry no `DW_AT_name` (e.g. some C++ out-of-line method definitions).
- `Parsers/Pe`: in-tree PE/COFF parser. Read DOS + NT headers, section table, and the debug directory's CodeView entry so we can extract `(pdb_path, guid, age)`. Lets us locate the PDB without relying on dbghelp's `SymGetModuleInfo`.
- `Parsers/Pdb`: in-tree PDB reader. Needs an MSF (Multi-Stream File) container layer first — superblock + free page map + stream directory + per-stream page chain. On top of that, parse the PDB Info stream (#1, validates GUID/age), the DBI stream (#3, gives module list + section contributions), and the Globals/Publics streams (S_PUB32 records map function names to RVA). Goal is `SymbolResolverResolve` parity with dbghelp's `SymFromAddr` for stripped binaries that ship a `.pdb`. Defer line-number resolution (`.symtab+0x...:line`) to a v2 of the parser since it needs the modi stream walker.
- `Sys/Backtrace`: replace dbghelp `SymFromAddr` / `SymGetLineFromAddr64` on Windows with the in-tree PE+PDB chain once both parsers exist. `CaptureStackBackTrace` (kernel32) stays — it's not a dbghelp dependency, and the alternative is reimplementing SEH-table-driven unwinding from RtlLookupFunctionEntry, which is its own multi-week effort.

## Naming / Platform
- `FileGetSize` / `ProcGetCurrentId` carry namespace prefixes only to avoid WINAPI macro collisions (`GetFileSize`, `GetCurrentProcessId`). Consider `#undef`'ing the WINAPI macros inside the `Sys/*` translation units and reverting to the cleaner bare names.
