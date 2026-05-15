# FUTURE-PLANS

Parking lot for items we've identified during real implementation work but deferred to avoid going down rabbit holes. One line per item — if it grows past a line, it deserves its own doc or a real ticket. Append, don't refactor.

## Allocators
- DebugAllocator: wire `PageProtect(PROT_NONE)` UAF mode behind a `force_page_backing` config; routes through `PageAllocator` instead of the user-supplied parent.
- DebugAllocator: optional mutex field for thread safety (single-threaded only today).
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
- `Sys/Backtrace` (Linux/macOS x86-64): sigsetjmp-guarded `read_u64_at` so a wild RIP during CFI / FP walking aborts the walk instead of crashing the whole process.
- `Sys/Backtrace`: aarch64 CFI walker — same shape as the x86-64 path but reads x29 / x30 / sp registers; deferred until we have an arm64 host to test on.

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
- `Parsers/Pdb`: support PDBs whose directory block-map spills past a single MSF page (currently v1 caps at one block-map page, ~4 MB of directory bytes).
- `Parsers/Pdb`: line-number resolution via the per-module symbol stream (current v1 only resolves names; `file:line` for stack frames is a v2 task).
- `Parsers/Pdb`: also walk `S_LPROC32` / `S_GPROC32` records in module symbol streams so private (non-public) function names appear in stack traces.
- `Parsers/MachO`: fat / universal binary support — pick the slice matching the host CPU instead of rejecting outright.
- `Sys/MachoCache`: also check `~/Library/Developer/Xcode/DerivedData/.../dSYM` for the dSYM when no sibling `<binary>.dSYM` bundle exists alongside the loaded image.

## Naming / Platform
- `FileGetSize` / `ProcGetCurrentId` carry namespace prefixes only to avoid WINAPI macro collisions (`GetFileSize`, `GetCurrentProcessId`). Consider `#undef`'ing the WINAPI macros inside the `Sys/*` translation units and reverting to the cleaner bare names.

## Completed
Items below have landed; kept here as a history of what each branch closed out.

### backtrace-hardening (May 2026)
- `Parsers/Elf`: `.note.gnu.build-id` + `.gnu_debuglink` parsing so the resolver can find a stripped binary's sidecar `.debug` file.
- `Parsers/Dwarf`: `.eh_frame` CIE/FDE parser + CFA bytecode interpreter + walker driver (`CaptureStackTraceCfi`) so unwinding works on `-fomit-frame-pointer` builds.
- `Parsers/Dwarf`: `.debug_info` function-name index (`DwarfFunctions`) for stripped binaries whose debug info kept the names.
- `SymbolResolver` cascade extended: main `.symtab` → sidecar `.symtab` → main `.debug_info` → sidecar `.debug_info`, with build-ID and (best-effort) debuglink pairing.
- `Parsers/Pe`: PE/COFF parser with DOS / NT / optional-header walk and CodeView (RSDS) record extraction.
- `Parsers/Pdb`: MSF container reader + PDB Info stream + DBI / SymRecord / SectionHdr walker for public function names.
- `Sys/PdbCache`: portable PE → PDB resolver with GUID/age validation; wired as the primary symbolizer in the Windows `Sys/Backtrace` path (dbghelp kept as fallback).
- `Parsers/MachO`: 64-bit Mach-O parser (LC_SEGMENT_64 + LC_SYMTAB + LC_UUID).
- `Sys/MachoCache`: dSYM-aware resolver (main symtab → dSYM symtab → dSYM DWARF) with UUID-match enforcement.
- `Sys/Backtrace`: macOS / Darwin backend (FP walk + dyld image lookup + MachoCache); brings the in-tree symbolizer to all three desktop platforms.
- `Sys/Backtrace`: raw + Vec shapes for `CaptureStackTrace` / `CaptureStackTraceCfi` / `FormatStackTrace` / `FormatStackTraceWith` via `MISRA_OVERLOAD`; preserves the alloc-free path the DebugAllocator depends on.
- `Std/Container/BitVec`: raw + Vec shapes for `BitVecFindAllPattern` (Vec of indices) and `BitVecRunLengths` (Vec of `{length, value}` records); same `MISRA_OVERLOAD` dispatch.
- `Std/Allocator/Debug`: backtrace + symbol resolution now goes through the in-tree chain end-to-end; no libc `backtrace()` or `dladdr` dependency. Static functions resolve through `.symtab` or `.debug_info`.
