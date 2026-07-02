project('misra', 'c',
    default_options: ['buildtype=debug', 'optimization=0', 'b_lto=false', 'c_std=c11']
)

cc = meson.get_compiler('c')

# Custom compile args. The Io/IOFMT macro layer casts `(void *)&x` for
# every formatted argument; for `const`-typed inputs this drops the
# const qualifier. clang / gcc / msvc all flag it under different names.
# It's a structural property of the macro design, not a per-callsite
# bug, so we silence the specific warning everywhere it's recognized
# rather than chasing hundreds of individual call sites.
common_c_args = []
foreach arg : [
    '-Wno-incompatible-pointer-types-discards-qualifiers',  # clang
    '-Wno-discarded-qualifiers',                            # gcc
    # Uninitialized-variable checks. The project's convention is
    # "initialise at declaration"; `-Wuninitialized` (clang+gcc) plus
    # `-Wmaybe-uninitialized` (gcc only) catches the slips. -Werror
    # on these so it fails the build and we don't accumulate drift.
    '-Wuninitialized',
    '-Wmaybe-uninitialized',
    '-Werror=uninitialized',
    '-Werror=maybe-uninitialized',
    # Force string literals to type `const char *` (a.k.a. Zstr), not
    # bare `char *`. Catches `char *p = "literal"` and similar smells
    # at the call site. There is no MSVC C equivalent -- the C
    # standard types literals as `char[N]` decaying to `char *`, and
    # `/Zc:strictStrings` is a C++-only flag. To stay portable, every
    # `_Generic` arm that matches `Zstr` also inlines a `char *`
    # synonym arm with the same body; that's the project convention.
    '-Wwrite-strings',
]
    if cc.has_argument(arg)
        common_c_args += arg
    endif
endforeach
if cc.get_id() == 'msvc' or cc.get_id() == 'clang-cl'
    common_c_args += '/wd4090'
    # NOTE: `/Zc:strictStrings` is intentionally not set. Microsoft's
    # docs are explicit that the flag enforces C++ const qualifications
    # on string literals and has NO effect in C mode. MSVC's C
    # compiler follows the C standard, which types literals as
    # `char[N]` decaying to `char *`. The codebase handles this by
    # giving every `_Generic` Zstr-arm a `char *` synonym -- see
    # Conventions/CODING-CONVENTIONS.md and Zstr.h for the rule.
endif

inc_misra = include_directories('Source', 'Include')

# ---------------------------------------------------------------------------
# Feature flags. The mandatory foundation (Sys, Sys/Mutex, Log, Memory,
# Allocator core + Page + Heap) is always compiled in. Everything else is
# opt-out. Dependencies are resolved transitively: enabling a feature that
# pulls in another feature transparently enables that other feature too.
# See meson_options.txt for the per-feature description.
# ---------------------------------------------------------------------------

# Read raw user choices for optional features.
opt_alloc_arena    = get_option('alloc_arena')
opt_alloc_slab     = get_option('alloc_slab')
opt_alloc_budget   = get_option('alloc_budget')
opt_alloc_stats    = get_option('alloc_stats')
opt_alloc_debug    = get_option('alloc_debug')
opt_default_alloc_debug = get_option('default_alloc_debug')
opt_default_alloc_debug_page_backed = get_option('default_alloc_debug_page_backed')

opt_bitvec_user    = get_option('bitvec')
opt_list           = get_option('list')
opt_map_user       = get_option('map')
opt_graph          = get_option('graph')
opt_int_user       = get_option('int')
opt_float          = get_option('float')

opt_file_user      = get_option('file')

opt_sys_dir        = get_option('sys_dir')
opt_sys_proc       = get_option('sys_proc')
opt_sys_socket     = get_option('sys_socket')
opt_sys_procmaps   = get_option('sys_procmaps')
opt_sys_symresolve = get_option('sys_symresolve')
opt_sys_backtrace  = get_option('sys_backtrace')
opt_sys_dns        = get_option('sys_dns')

# Platform availability matrix:
#   sys_procmaps:    Linux only (reads /proc/self/maps)
#   sys_symresolve:  Linux only (builds on procmaps + ELF parsing)
#   sys_backtrace:   Linux (in-tree via SymbolResolver), Windows (dbghelp +
#                    in-tree PE/PDB), or Darwin (in-tree Mach-O + dSYM)
#   alloc_debug:     wherever sys_backtrace is
#   sys_socket:      Linux + macOS via BSD sockets; Windows via Winsock2
#   sys_dns:         rides on sys_socket; reads /etc/hosts and
#                    /etc/resolv.conf on POSIX, the Win32 equivalents
#                    on Windows.
sys_kernel = host_machine.system()
if sys_kernel != 'linux'
    opt_sys_procmaps   = false
    opt_sys_symresolve = false
endif
if sys_kernel != 'linux' and sys_kernel != 'windows' and sys_kernel != 'darwin'
    opt_sys_backtrace = false
endif
# sys_dns rides on top of sys_socket + parser_dns.
if not opt_sys_socket
    opt_sys_dns = false
endif

# Winsock dependency for sys_socket on Windows. cc.find_library finds
# Ws2_32 via the platform SDK; the link arg is appended to the static
# library's dependencies so every executable that uses libmisra_std
# gets it transitively.
ws2_32_dep = []
if sys_kernel == 'windows' and opt_sys_socket
    ws2_32_dep = [cc.find_library('Ws2_32', required: true)]
endif

opt_parser_json    = get_option('parser_json')
opt_parser_kvconf  = get_option('parser_kvconfig')
opt_parser_http    = get_option('parser_http')
opt_parser_elf     = get_option('parser_elf')
opt_parser_dwarf   = get_option('parser_dwarf')
opt_parser_pe      = get_option('parser_pe')
opt_parser_pdb     = get_option('parser_pdb')
opt_parser_macho   = get_option('parser_macho')
opt_parser_dns     = get_option('parser_dns')

# Vec, Str, Io are part of the mandatory foundation: `LOG_FATAL` formats
# its message through `Str` and emits via the `Io` formatted path, so
# anything that wants logging-on-failure - which is all of the library -
# pulls them in. The `vec`, `str`, `io` flags are therefore not exposed as
# options. Removing that dependency would require splitting `LOG_FATAL`
# into a bare-syscall variant for the foundation and a formatted variant
# for callers that already have `Str` / `Io`. Out of scope for the
# feature-flag refactor.

# DebugAllocator hard-requires sys_backtrace. Rather than letting
# alloc_debug force backtrace on (which can re-enable an explicitly-
# disabled platform-unavailable feature), we go the other direction:
# alloc_debug can only be on if backtrace is. macOS / BSD / etc. with
# backtrace disabled above also lose alloc_debug. Clip BEFORE the
# alloc_debug -> map dependency resolves below; otherwise `map` would
# get force-enabled to satisfy a dependency from an alloc_debug that
# we're about to drop, leaving the consumer with the dependee compiled
# in but no dependent that pulls it.
opt_alloc_debug = opt_alloc_debug and opt_sys_backtrace
# default_alloc_debug hard-requires alloc_debug (it's a thin proxy in
# front of a DebugAllocator). And the page-backed UAF variant
# hard-requires default_alloc_debug itself.
opt_default_alloc_debug = opt_default_alloc_debug and opt_alloc_debug
opt_default_alloc_debug_page_backed = opt_default_alloc_debug_page_backed and opt_default_alloc_debug

# Transitively resolve dependencies among the optional features.
opt_int    = opt_int_user    or opt_float                       # float -> int
opt_bitvec = opt_bitvec_user or opt_int                         # int -> bitvec
opt_map    = opt_map_user    or opt_parser_kvconf or opt_alloc_debug  # kvconfig|alloc_debug -> map

# Linux-only dependency: in-tree Sys/Backtrace pulls in SymbolResolver,
# which pulls in ProcMaps + Parsers/Elf. The Windows backend uses
# dbghelp directly and skips this chain.
if sys_kernel == 'linux' and opt_sys_backtrace
    opt_sys_symresolve = true
endif
if opt_sys_symresolve
    opt_sys_procmaps = true
    opt_parser_elf   = true
endif

# parser_dwarf depends on parser_elf for section discovery.
if opt_parser_dwarf
    opt_parser_elf = true
endif

# parser_pdb hard-requires parser_pe to locate the PDB reference.
if opt_parser_pdb
    opt_parser_pe = true
endif

# sys_dns hard-requires the wire parser (DnsParseResponse /
# DnsResponseDeinit / dns_build_query_zstr all live in Parsers/Dns.c)
# AND ArenaAllocator (Sys/Dns.c uses ArenaAllocator unconditionally for
# its in-arena resolution scratch). Resolve here, BEFORE the source-list
# build below, so an explicit `-Dparser_dns=false -Dsys_dns=true` or
# `-Dalloc_arena=false -Dsys_dns=true` still pulls in the dependees
# instead of leaving Sys/Dns.c with undefined references. sys_dns ->
# sys_socket is already enforced upstream by the platform-clip block
# (`if not opt_sys_socket: opt_sys_dns = false`).
if opt_sys_dns
    opt_parser_dns  = true
    opt_alloc_arena = true
endif

# parser_elf / parser_pe / parser_macho all use FileReadAndClose to slurp
# the on-disk image before parsing.
opt_file = opt_file_user or opt_parser_elf or opt_parser_pe or opt_parser_macho

# `iter` is part of the foundation in practice: `Std/Io.c`'s
# `buf_read_fmt` / `buf_append_fmt` / `buf_write_fmt` use
# `IterRemainingLength` / `IterMustMove` / `IterPos` on `BufIter`, and
# `Io.c` is unconditionally compiled (LOG_FATAL formats through it).

# sys_procmaps is Linux-only; verified above.

# ---------------------------------------------------------------------------
# Build source list. Foundation first, optional features appended.
# ---------------------------------------------------------------------------
misra_std_sources = files(
    # Foundation: OS abstraction + log + memory + allocator core + the
    # Vec/Str/Io plumbing that LOG_FATAL formats through.
    'Source/Misra/Sys.c',
    'Source/Misra/Sys/Mutex.c',
    'Source/Misra/Sys/Clock.c',
    'Source/Misra/Std/Log.c',
    'Source/Misra/Std/Memory.c',
    'Source/Misra/Std/Zstr.c',
    'Source/Misra/Std/DateTime.c',
    'Source/Misra/Std/Allocator.c',
    'Source/Misra/Std/Allocator/_Os.c',
    'Source/Misra/Std/Allocator/Page.c',
    'Source/Misra/Std/Allocator/Heap.c',
    'Source/Misra/Std/Container/Vec.c',
    'Source/Misra/Std/Container/Str.c',
    'Source/Misra/Std/Io.c',
    'Source/Misra/Std/Prng.c',
    'Source/Misra/Std/ArgParse.c',
)

# Linux + x86_64/aarch64: bring in our own `_start` entry point so we
# can drop glibc's crt1.o + __libc_start_main from every binary. The
# matching `-nostartfiles` link flag is added a few lines below.
# Other platforms (macOS, Windows, ARM32, etc.) keep the toolchain's
# CRT entry untouched.
sys_cpu = host_machine.cpu_family()
have_misra_start = sys_kernel == 'linux' and (sys_cpu == 'x86_64' or sys_cpu == 'aarch64')

# Windows freestanding currently requires clang-cl: it gives us
# clang_rt.builtins for the compiler-emitted helpers (memcpy/memset/
# __chkstk etc.) without dragging UCRT in. MSVC bundles its
# equivalents inside libcmt, which can't be cleanly separated from
# the rest of the C runtime.
have_misra_start_win = sys_kernel == 'windows' \
    and cc.get_id() == 'clang-cl' \
    and sys_cpu == 'x86_64'

# FEATURE_DIRECT_SYSCALL covers both the Linux and Darwin (XNU) direct-
# syscall paths -- the asm wrappers and per-OS syscall number tables
# live in Source/Misra/_Syscall.h. Apple "deprecates" raw syscalls
# but they still work for the BSD-class numbers we use; libSystem is
# the only Apple-supported alternative. Cosmopolitan libc takes the
# same approach. macOS support intentionally pins to a tested major
# version -- Apple has renumbered syscalls historically.
have_direct_syscall = have_misra_start or (sys_kernel == 'darwin' and (sys_cpu == 'x86_64' or sys_cpu == 'aarch64'))

# `freestanding`: the library's default identity. _Freestanding.c's
# mem* overrides, _StartLinux.c's custom `_start`, the in-tree
# stack-protector helpers, and the `-nostdlib`-shaped link args on
# Bin/ tools are all keyed off this. Two escape hatches turn it off:
#
#   1. `b_sanitize` != 'none' -- sanitizer instrumentation
#      structurally requires libc and so cannot coexist with
#      freestanding:
#        - libasan / libubsan / liblsan live in libsanitizer
#          (libc-side). '-nostdlib' on Bin/ tools drops them,
#          producing __asan_* / __ubsan_* undefined references at
#          link time.
#        - ASan's own mem* interceptors clash with our in-tree mem*
#          overrides in _Freestanding.c.
#        - Stack-protector + FORTIFY_SOURCE machinery is needed by
#          some sanitizer configs.
#        - Our _start replacement skips ASan's TLS setup / ctor
#          running.
#
#   2. `-Ddisable_libc_diet=true` -- explicit hosted build for CI
#      matrix entries (and downstream packagers) that want to keep
#      libc / UCRT on the link line without enabling a sanitizer.
#      The Windows-LLVM `standard` matrix entry uses this to verify
#      the hosted clang-cl + UCRT path stays green alongside the
#      sibling `freestanding` entry that exercises /NODEFAULTLIB.
#
# So: freestanding is the shipping default. Sanitizers or
# disable_libc_diet flip the build into hosted mode. Everything
# below that uses `freestanding` is gating freestanding-specific
# behaviour, not making a user-visible choice.
freestanding = (get_option('b_sanitize') == 'none') and not get_option('disable_libc_diet')

# `-ffreestanding` (C only). Tells the compiler the program is
# freestanding; implies `-fno-builtin` and stops the optimizer from
# rewriting the Mem* byte loops into `call memset`/`memcpy`. Without
# this, our `_Freestanding.c` shim shadows libc's symbol and the
# rewritten call recurses infinitely at runtime on -O2+ builds. C
# only because `-ffreestanding` is a C-language flag; scoping it to C
# keeps it off any C++ TU, whose standard library refuses to compile
# under `-ffreestanding`.
if freestanding and cc.has_argument('-ffreestanding')
    add_project_arguments('-ffreestanding', language: 'c')
endif

# x86_64: pick the highest microarchitecture level the *runtime* CPU
# actually supports. cc.has_argument only checks whether the compiler
# accepts the flag — it'll happily accept `-march=x86-64-v3` on any
# host, and the resulting AVX2/BMI2/FMA instructions will then SIGILL
# on a CPU that lacks them (e.g. Ivy Bridge or older). Detect via
# /proc/cpuinfo on Linux so the same source tree compiles for a Haswell
# CI runner the same as for a 2012 dev box.
#
# Levels (per System V psABI):
#   x86-64-v4 : AVX-512F + AVX-512BW + AVX-512CD + AVX-512DQ + AVX-512VL
#   x86-64-v3 : AVX, AVX2, BMI1, BMI2, F16C, FMA, LZCNT, MOVBE, OSXSAVE
#   x86-64-v2 : SSE3, SSSE3, SSE4.1, SSE4.2, POPCNT, CX16, LAHF-SAHF
#   x86-64    : baseline (always assumed; no flag needed)
march_level = ''
if host_machine.cpu_family() == 'x86_64' and host_machine.system() == 'linux'
    march_probe = run_command(
        'sh', '-c',
        'F=$(grep "^flags" /proc/cpuinfo | head -1); ' +
        'check() { echo "$F" | grep -qE "\\b$1\\b"; }; ' +
        'if   check avx512f; then echo x86-64-v4; ' +
        'elif check avx2 && check bmi2 && check fma; then echo x86-64-v3; ' +
        'elif check sse4_2 && check popcnt; then echo x86-64-v2; ' +
        'fi',
        check: false,
    )
    march_level = march_probe.stdout().strip()
endif
# Non-Linux x86_64 (macOS, Windows): keep the historical default — trust
# cc.has_argument. CI on those platforms doesn't currently target older
# CPUs, and a proper sysctl/cpuid probe isn't worth the meson plumbing.
if host_machine.cpu_family() == 'x86_64' and march_level == '' \
   and host_machine.system() != 'linux' and cc.has_argument('-march=x86-64-v3')
    march_level = 'x86-64-v3'
endif
if march_level != '' and cc.has_argument('-march=' + march_level)
    add_project_arguments('-march=' + march_level, language: 'c')
    add_project_arguments('-march=' + march_level, language: 'cpp')
    message('x86_64: -march=' + march_level)
elif host_machine.cpu_family() == 'x86_64'
    message('x86_64: using baseline -march=x86-64 (no v2+ features detected)')
endif

# _Freestanding.c: compiler-/harness-emitted symbols (memcpy/memset/
# memmove/memcmp/bzero, plus setjmp/longjmp on Linux). Cross-platform.
#
# Gating: on Linux+Mac we always pull this in (under freestanding)
# because clang on those targets emits direct mem* calls regardless of
# linker mode. On Windows we only pull it in when have_misra_start_win
# is on (clang-cl freestanding flavour): for the standard 'clang +
# UCRT' flavour, vcruntime_string.h has already declared memcpy / etc.
# with __cdecl and full prototypes -- our overrides would 'conflict'
# at compile time. The standard flavour doesn't need our overrides
# anyway; UCRT supplies them.
if freestanding and \
   ((sys_kernel == 'linux' or sys_kernel == 'darwin') and \
    (sys_cpu == 'x86_64' or sys_cpu == 'aarch64'))
    misra_std_sources += files('Source/Misra/_Freestanding.c')
elif freestanding and have_misra_start_win
    misra_std_sources += files('Source/Misra/_Freestanding.c')
endif

if have_misra_start and freestanding
    misra_std_sources += files('Source/Misra/_StartLinux.c')

    # `-nostartfiles` drops crt1.o (our _start replaces it) from every
    # link line in the project. Keeps libc.so.6 around for test
    # scaffolding (mkstemp, fopen, remove) and for the few libc-bound
    # paths that don't go through direct syscall.
    add_project_link_arguments('-nostartfiles', language: 'c')

    # `freestanding_link_args` is the stronger flag set we apply only
    # to the shipping Bin/ tools. `-nostdlib` removes the entire libc
    # dependency; we add `-lgcc` back for compiler-runtime helpers
    # (128-bit divide, soft-float, etc.) the compiler emits implicit
    # calls to. mem*/setjmp come from _Freestanding.c.
    #
    # Result: shipped binaries (beam, resolve, etc.) report `ldd ...
    # => statically linked`. Test binaries keep their libc-bound
    # scaffolding (tests use mkstemp / fopen / remove for temp-file
    # setup; rewriting all of them is out of scope for the libc-diet
    # effort).
    freestanding_link_args = ['-nostdlib', '-lgcc']
    # Linux freestanding doesn't need per-target sources; _StartLinux.c
    # goes into libmisra_std (Linux libc on tests still gets the
    # COMDAT-resolution treatment so no duplicate-symbol conflict).
    freestanding_extra_sources = []
elif have_misra_start_win and freestanding
    # Windows freestanding (clang-cl only): drop UCRT + vcruntime via
    # /NODEFAULTLIB. Compiler-runtime helpers (memcpy etc.) come from
    # _Freestanding.c in libmisra_std. The custom entry point
    # (misra_start) and the stack-canary / __chkstk / _fltused /
    # UCRT-inline-fallout stubs live in separate TUs that are NOT in
    # libmisra_std -- they get added as per-target sources to
    # freestanding Bin/ executables (see freestanding_extra_sources
    # below). Keeping them out of libmisra_std avoids 'duplicate
    # symbol' conflicts with msvcrtd.lib(gs_cookie.obj) when test +
    # FuzzHarness binaries (which keep UCRT) pull libmisra_std.

    # Project-wide compile-side disablement of the libc-coupled
    # codegen clang-cl would otherwise insert into our objects.
    add_project_arguments(
        # /GS-: disable stack-canary insertion (no __security_cookie
        # / __security_check_cookie emission in prologues).
        '/GS-',
        # NOTE: we deliberately do NOT pass -mno-stack-arg-probe. clang-cl
        # emits a __chkstk call in any prologue whose frame exceeds one
        # page (4 KiB); that probe walks the stack guard pages so a
        # multi-page frame faults them in instead of skipping one and
        # crashing (0xC0000005). The probe is resolved from _WinStubs.c
        # (__chkstk) for the freestanding Bin/ tools and from UCRT for the
        # test/fuzz binaries, so large frames are safe everywhere. Keeping
        # the probe means a function with a big local array just works.
        language: 'c',
    )

    freestanding_link_args = [
        # Drop every flavour of CRT (static/dynamic, release/debug).
        '/NODEFAULTLIB:libucrt.lib',
        '/NODEFAULTLIB:libucrtd.lib',
        '/NODEFAULTLIB:libvcruntime.lib',
        '/NODEFAULTLIB:libvcruntimed.lib',
        '/NODEFAULTLIB:libcmt.lib',
        '/NODEFAULTLIB:libcmtd.lib',
        '/NODEFAULTLIB:msvcrt.lib',
        '/NODEFAULTLIB:msvcrtd.lib',
        '/NODEFAULTLIB:oldnames.lib',
        # Keep the platform DLLs we actually use.
        '/DEFAULTLIB:kernel32.lib',
        # Custom entry replaces mainCRTStartup.
        '/ENTRY:windows_start',
        '/SUBSYSTEM:CONSOLE',
    ]
    # Per-target sources for freestanding Bin/ tools: the custom
    # entry point (misra_start) + UCRT/compiler-runtime stubs.
    # Compiled per-executable rather than via libmisra_std so test
    # binaries (which keep UCRT) don't pull our __security_cookie
    # def and clash with UCRT's.
    freestanding_extra_sources = files(
        'Source/Misra/_StartWin.c',
        'Source/Misra/_WinStubs.c',
    )
else
    freestanding_link_args = []
    freestanding_extra_sources = []
endif

# FORTIFY_SOURCE pulls in libc-provided size-checker helpers
# (__memcpy_chk / __strcpy_chk / ...) that we can't satisfy in a
# libc-diet build, so turn it off. Stack-protector is KEPT ON:
# the instrumentation references __stack_chk_guard / __stack_chk_fail,
# which we provide ourselves in _Freestanding.c (seeded from
# AT_RANDOM via _StartLinux.c on Linux). Libc-diet builds get
# real canary protection -- a buffer overflow that smashes the
# return address now aborts via LogWrite + Abort instead of
# returning to attacker-controlled memory.
#
# MSVC / clang-cl have neither flag: MSVC's canaries are inlined
# without external libc symbols (controlled via /GS), and there's
# no FORTIFY_SOURCE concept on the MS side. Skip the gcc/clang
# flag set on those compilers to avoid the unknown-argument error.
if freestanding and \
   cc.get_id() != 'msvc' and cc.get_id() != 'clang-cl'
    add_project_arguments(
        '-U_FORTIFY_SOURCE',
        language: 'c',
    )
endif

if opt_alloc_arena
    misra_std_sources += files('Source/Misra/Std/Allocator/Arena.c')
endif
if opt_alloc_slab
    misra_std_sources += files('Source/Misra/Std/Allocator/Slab.c')
endif
if opt_alloc_budget
    misra_std_sources += files('Source/Misra/Std/Allocator/Budget.c')
endif
if opt_alloc_debug
    misra_std_sources += files('Source/Misra/Std/Allocator/Debug.c')
endif

if opt_bitvec
    misra_std_sources += files('Source/Misra/Std/Container/BitVec.c')
endif
if opt_list
    misra_std_sources += files('Source/Misra/Std/Container/List.c')
endif
if opt_map
    misra_std_sources += files('Source/Misra/Std/Container/Map.c')
endif
if opt_graph
    misra_std_sources += files('Source/Misra/Std/Container/Graph.c')
endif
if opt_int
    misra_std_sources += files('Source/Misra/Std/Container/Int.c')
endif
if opt_float
    misra_std_sources += files('Source/Misra/Std/Container/Float.c')
endif

if opt_file
    misra_std_sources += files('Source/Misra/Std/File.c')
    # ClockLocal's /etc/localtime (TZif) offset resolver. Needs File, so
    # it rides the same gate; Clock.c falls back to UTC without it.
    misra_std_sources += files('Source/Misra/Parsers/Tzif.c')
endif
misra_std_sources += files('Source/Misra/Std/Utility/Iter.c')

if opt_sys_dir
    misra_std_sources += files('Source/Misra/Sys/Dir.c')
endif
if opt_sys_proc
    misra_std_sources += files('Source/Misra/Sys/Proc.c')
endif
if opt_sys_socket
    misra_std_sources += files('Source/Misra/Sys/Socket.c')
endif
if opt_sys_procmaps
    misra_std_sources += files('Source/Misra/Parsers/ProcMaps.c')
endif
if opt_sys_symresolve
    misra_std_sources += files('Source/Misra/Sys/SymbolResolver.c')
endif
if opt_sys_backtrace
    misra_std_sources += files('Source/Misra/Sys/Backtrace.c')
endif

if opt_parser_json
    misra_std_sources += files('Source/Misra/Parsers/JSON.c')
endif
if opt_parser_kvconf
    misra_std_sources += files('Source/Misra/Parsers/KvConfig.c')
endif
if opt_parser_http
    misra_std_sources += files('Source/Misra/Parsers/Http.c')
endif
if opt_parser_elf
    misra_std_sources += files('Source/Misra/Parsers/Elf.c')
endif
if opt_parser_dwarf
    misra_std_sources += files(
        'Source/Misra/Parsers/Dwarf.c',
        'Source/Misra/Parsers/DwarfUnwind.c',
        'Source/Misra/Parsers/DwarfInfo.c',
    )
endif
if opt_parser_pe
    misra_std_sources += files('Source/Misra/Parsers/Pe.c')
endif
if opt_parser_pdb
    misra_std_sources += files(
        'Source/Misra/Parsers/Pdb.c',
        'Source/Misra/Sys/PdbCache.c',
    )
endif
if opt_parser_macho
    misra_std_sources += files(
        'Source/Misra/Parsers/MachO.c',
        'Source/Misra/Sys/MachoCache.c',
    )
endif
if opt_parser_dns
    misra_std_sources += files('Source/Misra/Parsers/Dns.c')
endif
if opt_sys_dns
    misra_std_sources += files('Source/Misra/Sys/Dns.c')
endif

# Compile-time discovery: each enabled feature exposes
# `FEATURE_<NAME> == 1` via a generated `Misra/Config.h`. Every
# library and consumer translation unit picks this up transitively
# through `Misra/Types.h`, so `#if FEATURE_BITVEC` works whether
# you build against the in-tree library or against the installed
# headers.
config_data = configuration_data()
config_data.set10('FEATURE_ALLOC_ARENA',     opt_alloc_arena)
config_data.set10('FEATURE_ALLOC_SLAB',      opt_alloc_slab)
config_data.set10('FEATURE_ALLOC_BUDGET',    opt_alloc_budget)
config_data.set10('FEATURE_ALLOC_STATS',     opt_alloc_stats)
config_data.set10('FEATURE_ALLOC_DEBUG',     opt_alloc_debug)
config_data.set10('FEATURE_DEFAULT_ALLOC_DEBUG',  opt_default_alloc_debug)
config_data.set10('FEATURE_DEFAULT_ALLOC_DEBUG_PAGE_BACKED', opt_default_alloc_debug_page_backed)
config_data.set10('FEATURE_BITVEC',          opt_bitvec)
config_data.set10('FEATURE_LIST',            opt_list)
config_data.set10('FEATURE_MAP',             opt_map)
config_data.set10('FEATURE_GRAPH',           opt_graph)
config_data.set10('FEATURE_INT',             opt_int)
config_data.set10('FEATURE_FLOAT',           opt_float)
config_data.set10('FEATURE_FILE',            opt_file)
config_data.set10('FEATURE_SYS_DIR',         opt_sys_dir)
config_data.set10('FEATURE_SYS_PROC',        opt_sys_proc)
config_data.set10('FEATURE_SYS_SOCKET',      opt_sys_socket)
config_data.set10('FEATURE_SYS_PROCMAPS',    opt_sys_procmaps)
config_data.set10('FEATURE_SYS_SYMRESOLVE',  opt_sys_symresolve)
config_data.set10('FEATURE_SYS_BACKTRACE',   opt_sys_backtrace)
config_data.set10('FEATURE_PARSER_JSON',     opt_parser_json)
config_data.set10('FEATURE_PARSER_KVCONFIG', opt_parser_kvconf)
config_data.set10('FEATURE_PARSER_HTTP',     opt_parser_http)
config_data.set10('FEATURE_PARSER_DWARF',    opt_parser_dwarf)
config_data.set10('FEATURE_PARSER_PDB',      opt_parser_pdb)
config_data.set10('FEATURE_PARSER_MACHO',    opt_parser_macho)
# DIRECT_SYSCALL is not user-configurable; it's purely a function of
# (kernel, cpu_family) and tracks whether _Syscall.h's inline-asm
# wrappers are available. Setting it here means SYS_ERRNO + friends in
# the public Sys.h can be specialised correctly the moment any Misra
# header is included, without having to depend on _Syscall.h being
# pre-included.
config_data.set10('FEATURE_DIRECT_SYSCALL',  have_direct_syscall)
config_data.set10('FEATURE_PARSER_DNS',      opt_parser_dns)
config_data.set10('FEATURE_SYS_DNS',         opt_sys_dns)

# Platform / architecture macros. These mirror the compiler builtins
# (`__linux__`, `__APPLE__`, `_WIN32`, `__x86_64__`, ...) but are
# project-controlled: exactly one PLATFORM_* and one ARCHITECTURE_* is
# `1`, the rest are `0`. Consumer code uses `#if PLATFORM_LINUX` rather
# than `#if defined(__linux__)`. PLATFORM_UNIX is the POSIX-like
# convenience flag (Linux OR Darwin, extensible to BSD).
config_data.set10('PLATFORM_LINUX',          sys_kernel == 'linux')
config_data.set10('PLATFORM_DARWIN',         sys_kernel == 'darwin')
config_data.set10('PLATFORM_WINDOWS',        sys_kernel == 'windows')
config_data.set10('PLATFORM_UNIX',           sys_kernel == 'linux' or sys_kernel == 'darwin')
config_data.set10('ARCHITECTURE_X86_64',     sys_cpu == 'x86_64')
config_data.set10('ARCHITECTURE_AARCH64',    sys_cpu == 'aarch64')
config_data.set10('ARCHITECTURE_X86_32',     sys_cpu == 'x86')
config_data.set10('ARCHITECTURE_ARM32',      sys_cpu == 'arm')

# Numeric tunables. `set` (vs `set10`) emits the literal value -- the
# `@FOO@` substitution in Config.h.in would normally be used, but we
# don't have one; configure_file with `configuration:` mode emits
# `#define FOO value` for each key, which is exactly what we want.
config_data.set('PRNG_RESEED_INTERVAL', get_option('prng_reseed_interval'))

# `subdir(...)` runs the inner meson.build, where `configure_file`
# emits `build/Include/Misra/Config.h`. The top-level
# `include_directories('Include')` already covers both the source
# `Include/` and its build-tree mirror, so `<Misra/Config.h>` resolves
# correctly in-tree, and the installed copy at
# `<prefix>/include/Misra/Config.h` covers downstream consumers.
subdir('Include/Misra')

# Print which optional features are enabled so the configure log is honest.
message('MisraStdC features enabled (host system: @0@):'.format(sys_kernel))
message('  alloc_arena=@0@  alloc_slab=@1@  alloc_budget=@2@  alloc_stats=@3@  alloc_debug=@4@'.format(
    opt_alloc_arena, opt_alloc_slab, opt_alloc_budget, opt_alloc_stats, opt_alloc_debug))
message('  default_alloc_debug=@0@  default_alloc_debug_page_backed=@1@'.format(
    opt_default_alloc_debug, opt_default_alloc_debug_page_backed))
message('  bitvec=@0@  list=@1@  map=@2@  graph=@3@  int=@4@  float=@5@'.format(
    opt_bitvec, opt_list, opt_map, opt_graph, opt_int, opt_float))
message('  file=@0@  sys_dir=@1@  sys_proc=@2@  sys_socket=@3@'.format(
    opt_file, opt_sys_dir, opt_sys_proc, opt_sys_socket))
message('  sys_dns=@0@'.format(opt_sys_dns))
message('  sys_procmaps=@0@  sys_symresolve=@1@  sys_backtrace=@2@'.format(
    opt_sys_procmaps, opt_sys_symresolve, opt_sys_backtrace))
message('  parser_json=@0@  parser_kvconfig=@1@  parser_http=@2@  parser_elf=@3@  parser_dwarf=@4@'.format(
    opt_parser_json, opt_parser_kvconf, opt_parser_http, opt_parser_elf, opt_parser_dwarf))
message('  parser_pe=@0@  parser_pdb=@1@  parser_macho=@2@  parser_dns=@3@'.format(opt_parser_pe, opt_parser_pdb, opt_parser_macho, opt_parser_dns))

# Build-time platform deps. The Windows Sys/Backtrace backend links
# against dbghelp for SymFromAddr / SymGetLineFromAddr64. Sys/Socket
# on Windows pulls in Ws2_32 for Winsock2. Std/Prng's Windows path
# calls BCryptGenRandom, which lives in bcrypt.dll.
misra_std_platform_deps = []
if opt_sys_backtrace and host_machine.system() == 'windows'
    misra_std_platform_deps += cc.find_library('dbghelp', required: true)
endif
if opt_sys_socket and host_machine.system() == 'windows'
    misra_std_platform_deps += ws2_32_dep
endif
if host_machine.system() == 'windows'
    misra_std_platform_deps += cc.find_library('bcrypt', required: true)
endif

# Regular library with sanitizers for normal tests and executables
misra_std = static_library(
    'misra_std',
    misra_std_sources,
    c_args: common_c_args,
    dependencies: misra_std_platform_deps,
    include_directories: inc_misra,
    install: true
)

# Dependency for working with other projects
misra_std_dep = declare_dependency(
    include_directories : inc_misra,
    link_with: misra_std,
    dependencies: misra_std_platform_deps,
)

# Test-only library variant for deadend suites that can't carry
# sanitisers (the LOG_FATAL longjmp path leaks the per-FATAL
# HeapAllocator, which ASan loudly reports). Same sources as
# `misra_std`; sanitisers off via override, plus the same
# LOG_NO_BACKTRACE gate as `misra_std_no_backtrace` because the
# only consumers (bitvec_deadend_tests + their test_util variant) fire
# many LOG_FATALs per binary and don't need the trace either.
misra_std_no_sanitizers = static_library(
    'misra_std_no_sanitizers',
    misra_std_sources,
    c_args: common_c_args + ['-DLOG_NO_BACKTRACE=1'],
    dependencies: misra_std_platform_deps,
    include_directories: inc_misra,
    install: false,
    override_options: ['b_sanitize=none']
)

# Test-only library variant with `LogWrite`'s FATAL backtrace block
# compiled out (LOG_NO_BACKTRACE). Deadend test binaries link
# this so each `LOG_FATAL` skips the per-frame macOS dSYM/DWARF
# symbolisation (which Mach-O re-parses every call -- ~99% of wall
# clock under ASan, see Log.c gate for rationale). Sanitisers stay
# enabled; only the trace formatter is gated. Production code paths
# (Bin/ tools, misra_std proper) keep full symbolisation.
misra_std_no_backtrace = static_library(
    'misra_std_no_backtrace',
    misra_std_sources,
    c_args: common_c_args + ['-DLOG_NO_BACKTRACE=1'],
    dependencies: misra_std_platform_deps,
    include_directories: inc_misra,
    install: false,
)

# Bin/* executables. The shipping set is two tools: beam (reverse proxy)
# and resolve (DNS CLI). Both are gated on the features they exercise.
if opt_sys_socket and opt_parser_http
    executable(
        'beam',
        files('Bin/Beam.c') + freestanding_extra_sources,
        link_with: [misra_std],
        c_args: common_c_args,
        include_directories: inc_misra,
        install: true,
        link_args: freestanding_link_args
    )
endif

if opt_sys_dns
    executable(
        'resolve',
        files('Bin/Resolve.c') + freestanding_extra_sources,
        link_with: [misra_std],
        c_args: common_c_args,
        include_directories: inc_misra,
        install: true,
        link_args: freestanding_link_args
    )
endif

# calc: an immediate-mode parser-combinator calculator (Bin/CalC.c) -- the
# worked example / playground for Include/Misra/ParserCombinator.h. Built and
# kept green by the tree, but not installed: it demonstrates the DSL, it is not
# a shipping utility. Needs Map (variables) and File (the stdin REPL).
if opt_map and opt_file
    executable(
        'calc',
        files('Bin/CalC.c') + freestanding_extra_sources,
        link_with: [misra_std],
        c_args: common_c_args,
        include_directories: inc_misra,
        install: false,
        link_args: freestanding_link_args
    )
endif

# Fuzzing harness pulls in List explicitly through Fuzz/Harness/ListInt.c.
if opt_list
    executable(
        'FuzzHarness',
        files(
            'Fuzz/Harness.c',
            'Fuzz/Harness/VecInt.c',
            'Fuzz/Harness/VecCharPtr.c',
            'Fuzz/Harness/VecStr.c',
            'Fuzz/Harness/ListInt.c'
        ),
        link_with: [misra_std],
        c_args: common_c_args,
        include_directories: inc_misra,
        install: false
        # FuzzHarness is libFuzzer-integrated; keep libc-bound
        # malloc/fopen/etc., so don't apply freestanding_link_args.
    )
endif

# Include tests
subdir('Tests')

# Install library headers
# Header install gating. Only headers for enabled features land in the
# install prefix. Disabled features have their `Container/<Name>/`,
# `Allocator/<Name>.h`, `Sys/<Name>.h`, or `Parsers/<Name>.h` excluded.
# The umbrella headers themselves (`Misra.h`, `Misra/Std.h`, ...)
# already use `#if FEATURE_*` so a downstream consumer picking up
# only the installed subset still gets a clean include graph.

install_subdir_excludes_dirs = []
install_subdir_excludes_files = []

if not opt_alloc_arena
    install_subdir_excludes_files += 'Std/Allocator/Arena.h'
endif
if not opt_alloc_slab
    install_subdir_excludes_files += 'Std/Allocator/Slab.h'
endif
if not opt_alloc_budget
    install_subdir_excludes_files += 'Std/Allocator/Budget.h'
endif
if not opt_alloc_debug
    install_subdir_excludes_files += 'Std/Allocator/Debug.h'
endif

if not opt_bitvec
    install_subdir_excludes_dirs  += 'Std/Container/BitVec'
    install_subdir_excludes_files += 'Std/Container/BitVec.h'
endif
if not opt_list
    install_subdir_excludes_dirs  += 'Std/Container/List'
    install_subdir_excludes_files += 'Std/Container/List.h'
endif
if not opt_map
    install_subdir_excludes_dirs  += 'Std/Container/Map'
    install_subdir_excludes_files += 'Std/Container/Map.h'
endif
if not opt_graph
    install_subdir_excludes_dirs  += 'Std/Container/Graph'
    install_subdir_excludes_files += 'Std/Container/Graph.h'
endif
if not opt_int
    install_subdir_excludes_dirs  += 'Std/Container/Int'
    install_subdir_excludes_files += 'Std/Container/Int.h'
endif
if not opt_float
    install_subdir_excludes_dirs  += 'Std/Container/Float'
    install_subdir_excludes_files += 'Std/Container/Float.h'
endif

if not opt_file
    install_subdir_excludes_files += 'Std/File.h'
endif

if not opt_sys_dir
    install_subdir_excludes_files += 'Sys/Dir.h'
endif
if not opt_sys_proc
    install_subdir_excludes_files += 'Sys/Proc.h'
endif
if not opt_sys_socket
    install_subdir_excludes_files += 'Sys/Socket.h'
endif
if not opt_sys_procmaps
    install_subdir_excludes_files += 'Parsers/ProcMaps.h'
endif
if not opt_sys_symresolve
    install_subdir_excludes_files += 'Sys/SymbolResolver.h'
endif
if not opt_sys_backtrace
    install_subdir_excludes_files += 'Sys/Backtrace.h'
endif

if not opt_parser_json
    install_subdir_excludes_files += 'Parsers/JSON.h'
endif
if not opt_parser_kvconf
    install_subdir_excludes_files += 'Parsers/KvConfig.h'
endif
if not opt_parser_http
    install_subdir_excludes_files += 'Parsers/Http.h'
endif
if not opt_parser_elf
    install_subdir_excludes_files += 'Parsers/Elf.h'
endif
if not opt_parser_dwarf
    install_subdir_excludes_files += 'Parsers/Dwarf.h'
endif
if not opt_parser_pe
    install_subdir_excludes_files += 'Parsers/Pe.h'
endif
if not opt_parser_pdb
    install_subdir_excludes_files += 'Parsers/Pdb.h'
    install_subdir_excludes_files += 'Sys/PdbCache.h'
endif
if not opt_parser_macho
    install_subdir_excludes_files += 'Parsers/MachO.h'
    install_subdir_excludes_files += 'Sys/MachoCache.h'
endif
if not opt_parser_dns
    install_subdir_excludes_files += 'Parsers/Dns.h'
endif
if not opt_sys_dns
    install_subdir_excludes_files += 'Sys/Dns.h'
endif

install_subdir(
    'Include/Misra',
    install_dir: 'include',
    exclude_directories: install_subdir_excludes_dirs,
    exclude_files: install_subdir_excludes_files,
)
install_headers('Include/Misra.h')

# Generate pkg-config for user linkage
pkg = import('pkgconfig')
pkg.generate(
    misra_std,
    name: 'MisraStdC',
    description: 'Generic and almost type-safe programming in C',
    filebase: 'libmisra_std',
    url: 'https://docs.brightprogrammer.in'
)
