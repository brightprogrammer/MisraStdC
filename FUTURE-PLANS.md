# FUTURE-PLANS

Parking lot for items we've identified during real implementation work but deferred to avoid going down rabbit holes. One line per item — if it grows past a line, it deserves its own doc or a real ticket. Append, don't refactor.

## Allocators
- Allocator vtable: per-call alignment parameter on `AllocatorAlloc` / `AllocatorRealloc` (currently fixed at allocator init).
- Allocator vtable: split `resize` (in-place, returns bool) vs `remap` (may move, returns ptr-or-NULL); keep `reallocate` as the convenience that cascades.
- `StackFallbackAllocator`: composition wrapper that tries a fixed stack buffer first then falls back to a parent allocator.
- `LoggingAllocator`: composition wrapper that logs every alloc/realloc/free on a parent.
- `ThreadSafeAllocator`: composition wrapper that adds a mutex around any parent.
- `GuardPageAllocator`: per-allocation `mmap` + `mprotect(PROT_NONE)` flanking guard pages for overflow-traps-instantly testing.

## Sys / Networking
- `Sys/Socket`: in-tree DNS resolver — currently `SocketAddrParse` rejects hostnames with a clear error and accepts numeric IPv4 / IPv6 only. A real resolver needs UDP/TCP query to `/etc/resolv.conf` nameservers, `A` + `AAAA` walk, `CNAME` chain follow, `/etc/hosts` override, optional caching.
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
- `Parsers/MachO`: `N_STAB` filter fix -- per the Mach-O spec any high bit of `n_type` (mask `0xE0`) flags an entry as a debug stab; the original check required all three bits set so common stab types (`N_SO`, `N_FUN`, `N_OSO`, `N_BNSYM`) were polluting symbol lookups. Caught when explicit Backtrace tests ran on the macOS host.
- `Tests/Std/MachO`: Darwin-only round-trip against the running test binary via `_NSGetExecutablePath` + `MachoFileOpen` + `MachoFileResolveAddress`; structural parallel of the Linux `Tests/Std/Elf` `/proc/self/exe` smoke tests.

### libc-diet (May 2026)
- Project-wide: dropped 65 of 67 imported libc symbols from `libmisra_std.a`. The remaining 2 are compiler-emitted (`__errno_location`, `__stack_chk_fail`) and would need different build flags to drop.
- `Sys/Socket`: `SocketAddrParse` now uses in-tree `parse_ipv4` / `parse_ipv6` / `parse_port` instead of libc `getaddrinfo`. RFC 5952 "::" compression on the v6 side. Hostname resolution returns a clear error pointing at the future-plans DNS resolver entry. `freeaddrinfo` also gone; `<netdb.h>` no longer included.
- `Source/Misra/_Syscall.h`: private internal header with `misra_sys0..misra_sys6` inline-asm wrappers and a `MISRA_SYS_*` syscall-number table for Linux x86_64 and aarch64. Single shared plumbing for every direct-syscall site; macOS / Windows fall through to libSystem / kernel32.
- `Sys.c`: `abort()` -> per-arch inline-asm trap (`ud2`/`brk #0`/`udf #0`/`__debugbreak`); `getpid` -> direct syscall; `getenv` -> walk `extern char **environ`; `strerror_r` -> in-tree errno description table covering ~45 POSIX values.
- `Sys/Mutex.c`: pthread -> futex (Linux) / `os_unfair_lock` (macOS) / `SRWLOCK` (Windows). Sizeof(Mutex) shrinks from ~40 bytes to 4-pointer-sized. Drepper-style 3-state futex; lock fast-path is one CAS.
- `Sys/Proc.c`: 10 process / I/O wrappers (`close`, `read`, `write`, `pipe`/`pipe2`, `dup2`/`dup3`, `fork`/`clone`, `execve`, `kill`, `readlink`/`readlinkat`, `waitpid`/`wait4`) re-routed through `misra_sys*` via local `#define` shims so the existing call sites stay textually unchanged.
- `Sys/Socket.c`: full BSD-sockets call set (`socket`, `bind`, `connect`, `listen`, `accept`, `recv`/`recvfrom`, `send`/`sendto`, `setsockopt`, `close`, `fcntl`, `poll`/`ppoll`) re-routed through `misra_sys*`. `gai_strerror` -> in-tree EAI table. `inet_ntop` -> in-tree IPv4 dotted-quad + RFC 5952 IPv6 compression formatter. `ntohs` -> `FROM_BIG_ENDIAN2` from Types.h.
- `Sys/Dir.c`: `opendir`/`readdir`/`closedir` -> direct `getdents64` syscall + manual `struct linux_dirent64` walk. `stat` for `FileGetSize` -> `open` + `lseek(END)` + `close` (avoids the kernel/libc `struct stat` ABI mismatch).
- `Std/Allocator/Page.c`: `mmap` / `munmap` / `mprotect` via direct syscalls; `sysconf(_SC_PAGESIZE)` -> hardcoded per-arch constant (4 KiB everywhere except Apple Silicon at 16 KiB).
- `Std/File.h` / `Std/File.c`: new cross-platform `File` value type wrapping an fd on POSIX / `HANDLE` on Windows. `FileOpen` / `FileClose` / `FileRead` / `FileWrite` / `FileSeek` / `FileTell` / `FileFlush` / `FileIsEof` / `FileFromFd` / `FileStdin` / `FileStdout` / `FileStderr`. Replaces all `FILE *` use across `File.c`, `Log.c`, `Io.c`, `ProcMaps.c`, `MachoCache.c`, `PdbCache.c`, `SymbolResolver.c`.
- `Std/Io.c`: `f_write_fmt` / `f_read_fmt` now take `File *` instead of `FILE *`. `FWriteFmt` / `FWriteFmtLn` / `FReadFmt` keep their macro shape; `WriteFmt` / `WriteFmtLn` / `ReadFmt` expand to a stack-local `File f = FileStdout()`-style binding so the OS's well-known channels are wrapped per-call without process state.
- `Std/Log.c`: completely stateless rewrite. No globals, no init, no deinit, no mutex. Per-call stack `HeapAllocator` formats the line via `StrWriteFmt`, single `FileWrite` to fd 1 (INFO) or fd 2 (ERROR/FATAL) — POSIX atomic-small-write carries the thread-safety load. `LOG_FATAL` auto-appends a captured stack trace before `Abort()`. `LogInit` / `LogDeinit` deleted from the public API; 6 `Bin/` callers updated.
- `Std/Container/Str.c` + JSON.h: dropped the printf-style `StrPrintf` / `StrAppendf` (`vsnprintf`-backed). JSON.h's `JW_*` writer macros now route through `StrWriteFmt` so the typed-dispatch path handles `{}` placeholders for `Str` / `Int` / `Float` / etc. natively.
- `Std/Container/Float.c`: `snprintf("%g"/"%.17g")` round-trip replaced by direct IEEE-754 bit-fishing into `(sig * 5^|binexp|, dexp = binexp)` form. Round-trip is now bit-exact (the libc `%.17g` path was shortest-form and could lose information).
- `Std/Memory.h` / `Std/Memory.c`: added `MemSort` (quicksort + insertion-sort fallback + tail-iter on larger partition, replaces libc `qsort`), `ZstrToI64` / `ZstrToF64` (decimal parsers replacing `strtoll` / `strtod`). All Vec / List / JSON / KvConfig / Float consumers migrated.
- `Std/Allocator/Page.c`: ctype + `strtol(base=16)` in `Io.c` replaced with a 20-line `hex_nibble` / `hex_byte` helper for `\xNN` escape parsing.
- `Std/Container/Map.c`: probe-budget exhaustion now forces capacity growth (passes `n = capacity + 1` to `next_capacity`) instead of looping at the same size when a hash cluster exceeds `max_probe_count`. Drove the DebugAllocator runaway-rehash bug earlier; regression test added.

### debug-allocator (May 2026)
- `Std/Allocator/Debug`: init-by-value refactor -- struct literal you assign to a stack variable, no Create/Destroy, no globals. Each instance owns inline `heap` / `meta` / `page` allocators; the live-map's allocator pointer is lazily rebound on first use against the now-stable `self`. Thread affinity enforced via TLS-marker-address creator-TID check (no mutex, no globals).
- `Std/Allocator/Debug`: `force_page_backing` config (UAF detection mode) -- routes every alloc through the internal `PageAllocator`, `PageProtect(PROT_NONE)`'s the region on free so any dangling-pointer dereference traps with SIGSEGV at the bug site. Reserved for tests / fuzz harnesses (freed pages are never reclaimed).
- `Std/Allocator/Default`: `default_alloc_debug` meson option aliases `DefaultAllocator` to `DebugAllocator` (per-instance, no globals); drop-in ASan/MSan-style leak + double-free + canary-overflow tracking with no call-site changes. Pairs with `default_alloc_debug_page_backed` to also flip on the UAF mode.
- `Sys/Backtrace`: forward-declares `SymbolResolver` so transitively pulling `Parsers/Elf.h` into every TU stops happening; needed once `Default.h` started pulling `Debug.h` through `default_alloc_debug=true`.
- `Std/Container/Map`: probe-budget exhaustion now forces capacity growth instead of rehashing at the same size. Linear / quadratic probing limited to `max_probe_count=128` could otherwise loop forever when a hash cluster exceeded the budget (rehash at same size -> same first_index -> same cluster). Surfaced under DebugAllocator churn; regression test added.
