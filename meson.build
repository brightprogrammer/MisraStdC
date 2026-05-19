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
]
    if cc.has_argument(arg)
        common_c_args += arg
    endif
endforeach
if cc.get_id() == 'msvc' or cc.get_id() == 'clang-cl'
    common_c_args += '/wd4090'
endif

math_dep = cc.find_library('m', required: false)

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
opt_iter           = get_option('iter')

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

# Transitively resolve dependencies among the optional features.
opt_int    = opt_int_user    or opt_float                       # float -> int
opt_bitvec = opt_bitvec_user or opt_int                         # int -> bitvec
opt_map    = opt_map_user    or opt_parser_kvconf or opt_alloc_debug  # kvconfig|alloc_debug -> map

# DebugAllocator hard-requires sys_backtrace. Rather than letting
# alloc_debug force backtrace on (which can re-enable an explicitly-
# disabled platform-unavailable feature), we go the other direction:
# alloc_debug can only be on if backtrace is. macOS / BSD / etc. with
# backtrace disabled above also lose alloc_debug.
opt_alloc_debug = opt_alloc_debug and opt_sys_backtrace
# default_alloc_debug hard-requires alloc_debug (it's a thin proxy in
# front of a DebugAllocator). And the page-backed UAF variant
# hard-requires default_alloc_debug itself.
opt_default_alloc_debug = opt_default_alloc_debug and opt_alloc_debug
opt_default_alloc_debug_page_backed = opt_default_alloc_debug_page_backed and opt_default_alloc_debug

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

# parser_elf / parser_pe / parser_macho all use ReadCompleteFile.
opt_file = opt_file_user or opt_parser_elf or opt_parser_pe or opt_parser_macho

# sys_procmaps is Linux-only; verified above.

# ---------------------------------------------------------------------------
# Build source list. Foundation first, optional features appended.
# ---------------------------------------------------------------------------
misra_std_sources = files(
    # Foundation: OS abstraction + log + memory + allocator core + the
    # Vec/Str/Io plumbing that LOG_FATAL formats through.
    'Source/Misra/Sys.c',
    'Source/Misra/Sys/Mutex.c',
    'Source/Misra/Std/Log.c',
    'Source/Misra/Std/Memory.c',
    'Source/Misra/Std/Zstr.c',
    'Source/Misra/Std/Allocator.c',
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

# `enable_libc_diet`: master switch for the whole freestanding /
# direct-syscall machinery. We turn it OFF whenever any sanitizer is
# active, because:
#   - libasan / libubsan / liblsan / etc. live in libsanitizer (a
#     libc-side library). '-nostdlib' on Bin/ tools drops them too,
#     producing __asan_* / __ubsan_* undefined references at link
#     time (this is what failed Linux CI on commit 6ce17b7).
#   - ASan's own mem* interceptors would clash with our in-tree mem*
#     overrides in _Freestanding.c.
#   - Stack-protector + FORTIFY_SOURCE machinery is needed by the
#     sanitizer instrumentation in some scenarios.
#   - Our _start replacement skips ASan's TLS setup / ctor running.
#
# Sanitizer builds don't care about the libc-diet property -- they
# care about correctness under instrumentation. Take the full libc
# path on those.
enable_libc_diet = get_option('b_sanitize') == 'none'

# _Freestanding.c: compiler-/harness-emitted symbols (memcpy/memset/
# memmove/memcmp/bzero, plus setjmp/longjmp on Linux). Cross-platform.
#
# Gating: on Linux+Mac we always pull this in (under enable_libc_diet)
# because clang on those targets emits direct mem* calls regardless of
# linker mode. On Windows we only pull it in when have_misra_start_win
# is on (clang-cl freestanding flavour): for the standard 'clang +
# UCRT' flavour, vcruntime_string.h has already declared memcpy / etc.
# with __cdecl and full prototypes -- our overrides would 'conflict'
# at compile time. The standard flavour doesn't need our overrides
# anyway; UCRT supplies them.
if enable_libc_diet and \
   ((sys_kernel == 'linux' or sys_kernel == 'darwin') and \
    (sys_cpu == 'x86_64' or sys_cpu == 'aarch64'))
    misra_std_sources += files('Source/Misra/_Freestanding.c')
elif enable_libc_diet and have_misra_start_win
    misra_std_sources += files('Source/Misra/_Freestanding.c')
endif

if have_misra_start and enable_libc_diet
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
elif have_misra_start_win and enable_libc_diet
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
        # -mno-stack-arg-probe: stop clang emitting __chkstk calls
        # in function prologues with large stack frames. Our runtime
        # never allocates the multi-page stack buffers the probe
        # protects against.
        '-mno-stack-arg-probe',
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
        '/ENTRY:misra_start',
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

# Stack-protector / FORTIFY_SOURCE pull in libc-provided helpers
# (__stack_chk_fail, __stack_chk_guard on Linux; ___stack_chk_fail +
# ___stack_chk_guard + ___chkstk_darwin on Mac; __memcpy_chk etc.
# under fortify). We don't gain anything from them in a libc-diet
# project that goes through hand-audited byte loops everywhere --
# turn both off on the GCC/clang side. MSVC has its own /GS flag
# but doesn't pull libc helpers for stack canaries (the runtime
# check is inlined), and has no FORTIFY_SOURCE equivalent -- skip
# both flags on cl.exe / clang-cl to avoid the unknown-argument
# error. Also skip under sanitizers -- they expect a normal stack
# layout including canary slots in some configs.
if enable_libc_diet and \
   cc.get_id() != 'msvc' and cc.get_id() != 'clang-cl'
    add_project_arguments(
        '-fno-stack-protector',
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
endif
if opt_iter
    misra_std_sources += files('Source/Misra/Std/Utility/Iter.c')
endif

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
    misra_std_sources += files('Source/Misra/Sys/ProcMaps.c')
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
    # sys_dns hard-requires the wire parser + the socket layer.
    opt_parser_dns = true
    opt_sys_socket = true
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
config_data.set10('FEATURE_ITER',            opt_iter)
config_data.set10('FEATURE_SYS_DIR',         opt_sys_dir)
config_data.set10('FEATURE_SYS_PROC',        opt_sys_proc)
config_data.set10('FEATURE_SYS_SOCKET',      opt_sys_socket)
config_data.set10('FEATURE_SYS_PROCMAPS',    opt_sys_procmaps)
config_data.set10('FEATURE_SYS_SYMRESOLVE',  opt_sys_symresolve)
config_data.set10('FEATURE_SYS_BACKTRACE',   opt_sys_backtrace)
config_data.set10('FEATURE_PARSER_JSON',     opt_parser_json)
config_data.set10('FEATURE_PARSER_KVCONFIG', opt_parser_kvconf)
config_data.set10('FEATURE_PARSER_HTTP',     opt_parser_http)
config_data.set10('FEATURE_PARSER_ELF',      opt_parser_elf)
config_data.set10('FEATURE_PARSER_DWARF',    opt_parser_dwarf)
config_data.set10('FEATURE_PARSER_PE',       opt_parser_pe)
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
message('  file=@0@  iter=@1@  sys_dir=@2@  sys_proc=@3@  sys_socket=@4@'.format(
    opt_file, opt_iter, opt_sys_dir, opt_sys_proc, opt_sys_socket))
message('  sys_dns=@0@'.format(opt_sys_dns))
message('  sys_procmaps=@0@  sys_symresolve=@1@  sys_backtrace=@2@'.format(
    opt_sys_procmaps, opt_sys_symresolve, opt_sys_backtrace))
message('  parser_json=@0@  parser_kvconfig=@1@  parser_http=@2@  parser_elf=@3@  parser_dwarf=@4@'.format(
    opt_parser_json, opt_parser_kvconf, opt_parser_http, opt_parser_elf, opt_parser_dwarf))
message('  parser_pe=@0@  parser_pdb=@1@  parser_macho=@2@  parser_dns=@3@'.format(opt_parser_pe, opt_parser_pdb, opt_parser_macho, opt_parser_dns))

# Build-time platform deps. The Windows Sys/Backtrace backend links
# against dbghelp for SymFromAddr / SymGetLineFromAddr64. Sys/Socket
# on Windows pulls in Ws2_32 for Winsock2.
misra_std_platform_deps = [math_dep]
if opt_sys_backtrace and host_machine.system() == 'windows'
    misra_std_platform_deps += cc.find_library('dbghelp', required: true)
endif
if opt_sys_socket and host_machine.system() == 'windows'
    misra_std_platform_deps += ws2_32_dep
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

# Non-sanitized library specifically for deadend tests to avoid memory leak reports
misra_std_no_sanitizers = static_library(
    'misra_std_no_sanitizers',
    misra_std_sources,
    c_args: common_c_args,
    dependencies: misra_std_platform_deps,
    include_directories: inc_misra,
    install: false,
    override_options: ['b_sanitize=none']
)

# Main.c and misra executable have been removed

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
if not opt_iter
    install_subdir_excludes_dirs  += 'Std/Utility/Iter'
    install_subdir_excludes_files += 'Std/Utility/Iter.h'
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
    install_subdir_excludes_files += 'Sys/ProcMaps.h'
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
