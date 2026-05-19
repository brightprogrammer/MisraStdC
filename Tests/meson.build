#Tests meson.build file
#
#Note : If you see a warning about sanitizers on Clang with b_lundef,
#you can fix it by setting b_lundef = false in your meson command:
#meson setup builddir - Db_sanitize = address, undefined - Db_lundef = false
#
#Or add this to your meson_options.txt file:
#option('b_lundef', type : 'boolean', value : false)

#Include test utilities
subdir('Util')

# Default test timeout headroom.
#
# Meson's default per-test timeout is 30 s. Test binaries link
# `misra_std_no_backtrace` (LogWrite's FATAL backtrace formatter
# gated out), so deadend suites no longer pay the per-LOG_FATAL macOS
# dSYM + DWARF symbolisation cost. A small 2x cushion covers the
# Backtrace / MachO stress tests, which legitimately spend ~28 s on
# macOS CI even on the full library.
add_test_setup('default', is_default: true, timeout_multiplier: 2)

#Test configuration
test_args = common_c_args

# Project-wide policy check: no libc includes outside the documented
# OS boundary (POSIX / Win32 are not libc, see the script's
# docblock). Runs as a meson test so any libc creep is reported the
# next time the test suite is run.
no_libc_check = find_program('python3', required : true)
test(
    'CheckNoLibc',
    no_libc_check,
    args : [meson.project_source_root() / 'Tests' / 'Util' / 'check_no_libc.py'],
    workdir : meson.project_source_root(),
)

#Group tests by module
io_tests = [
    ['Io.Write', 'Std/Io.Write.c'],
    ['Io.Read', 'Std/Io.Read.c'],
]

file_tests = [
    ['File', 'Std/File.c'],
]

argparse_tests = [
    ['ArgParse', 'Std/ArgParse.c'],
]

socket_tests = [
    ['Socket', 'Std/Socket.c'],
]

http_tests = [
    ['Http', 'Std/Http.c'],
]

elf_tests = [
    ['Elf', 'Std/Elf.c'],
]

dwarf_tests = [
    ['Dwarf', 'Std/Dwarf.c'],
]

pe_tests = [
    ['Pe', 'Std/Pe.c'],
]

pdb_tests = [
    ['Pdb', 'Std/Pdb.c'],
]

pdb_cache_tests = [
    ['PdbCache', 'Std/PdbCache.c'],
]

macho_tests = [
    ['MachO', 'Std/MachO.c'],
]

dns_tests = [
    ['Dns', 'Std/Dns.c'],
]

sys_dns_tests = [
    ['SysDns', 'Std/SysDns.c'],
]

macho_cache_tests = [
    ['MachoCache', 'Std/MachoCache.c'],
]

procmaps_tests = [
    ['ProcMaps', 'Std/ProcMaps.c'],
]

symresolve_tests = [
    ['SymbolResolver', 'Std/SymbolResolver.c'],
]

backtrace_tests = [
    ['Backtrace', 'Std/Backtrace.c'],
]

page_protect_tests = [
    ['PageProtect', 'Std/PageProtect.c'],
]

alloc_debug_tests = [
    ['AllocDebug', 'Std/AllocDebug.c'],
]

iter_tests = [
    ['Iter', 'Std/Iter.c'],
]

iter_deadend_tests = [
    ['Iter.Deadend', 'Std/Iter.Deadend.c'],
]

buf_tests = [
    ['Buf', 'Std/Buf.c'],
]

str_tests = [
    ['Str.Type', 'Std/Str.Type.c'],
    ['Str.Init', 'Std/Str.Init.c'],
    ['Str.Insert', 'Std/Str.Insert.c'],
    ['Str.Remove', 'Std/Str.Remove.c'],
    ['Str.Access', 'Std/Str.Access.c'],
    ['Str.Memory', 'Std/Str.Memory.c'],
    ['Str.Convert', 'Std/Str.Convert.c'],
    ['Str.Foreach', 'Std/Str.Foreach.c'],
    ['Str.Ops', 'Std/Str.Ops.c'],
]

vec_tests = [
    ['Vec.Type', 'Std/Vec.Type.c'],
    ['Vec.Init', 'Std/Vec.Init.c'],
    ['Vec.Access', 'Std/Vec.Access.c'],
    ['Vec.Memory', 'Std/Vec.Memory.c'],
    ['Vec.Insert', 'Std/Vec.Insert.c'],
    ['Vec.Remove', 'Std/Vec.Remove.c'],
    ['Vec.Foreach', 'Std/Vec.Foreach.c'],
    ['Vec.Ops', 'Std/Vec.Ops.c'],
    ['Vec.Complex', 'Std/Vec.Complex.c'],
]

graph_tests = [
    ['Graph.Type', 'Std/Graph.Type.c'],
    ['Graph.Init', 'Std/Graph.Init.c'],
    ['Graph.Access', 'Std/Graph.Access.c'],
    ['Graph.Insert', 'Std/Graph.Insert.c'],
    ['Graph.Foreach', 'Std/Graph.Foreach.c'],
    ['Graph.Ops', 'Std/Graph.Ops.c'],
]

list_tests = [
    ['List.Type', 'Std/List.Type.c'],
    ['List.Init', 'Std/List.Init.c'],
    ['List.Access', 'Std/List.Access.c'],
    ['List.Insert', 'Std/List.Insert.c'],
    ['List.Remove', 'Std/List.Remove.c'],
    ['List.Foreach', 'Std/List.Foreach.c'],
    ['List.Ops', 'Std/List.Ops.c'],
    ['List.Deadend', 'Std/List.Deadend.c'],
]

map_tests = [
    ['Map.Type', 'Std/Map.Type.c'],
    ['Map.Init', 'Std/Map.Init.c'],
    ['Map.Access', 'Std/Map.Access.c'],
    ['Map.Insert', 'Std/Map.Insert.c'],
    ['Map.Remove', 'Std/Map.Remove.c'],
    ['Map.Foreach', 'Std/Map.Foreach.c'],
    ['Map.Ops', 'Std/Map.Ops.c'],
    ['Map.Deadend', 'Std/Map.Deadend.c'],
]

bitvec_tests = [
    ['BitVec.Type', 'Std/BitVec.Type.c'],
    ['BitVec.Init', 'Std/BitVec.Init.c'],
    ['BitVec.Access.Simple', 'Std/BitVec.Access.Simple.c'],
    ['BitVec.Insert', 'Std/BitVec.Insert.c'],
    ['BitVec.Remove', 'Std/BitVec.Remove.c'],
    ['BitVec.Memory', 'Std/BitVec.Memory.c'],
    ['BitVec.BitWise.Simple', 'Std/BitVec.BitWise.Simple.c'],
    ['BitVec.Compare', 'Std/BitVec.Compare.c'],
    ['BitVec.Convert', 'Std/BitVec.Convert.c'],
    ['BitVec.Pattern.Simple', 'Std/BitVec.Pattern.Simple.c'],
    ['BitVec.Math', 'Std/BitVec.Math.c'],
    ['BitVec.Foreach.Simple', 'Std/BitVec.Foreach.Simple.c'],
]

int_tests = [
    ['Int.Type', 'Std/Int.Type.c'],
    ['Int.Access', 'Std/Int.Access.c'],
    ['Int.Compare', 'Std/Int.Compare.c'],
    ['Int.Convert', 'Std/Int.Convert.c'],
    ['Int.Math', 'Std/Int.Math.c'],
]

float_tests = [
    ['Float.Type', 'Std/Float.Type.c'],
    ['Float.Access', 'Std/Float.Access.c'],
    ['Float.Compare', 'Std/Float.Compare.c'],
    ['Float.Convert', 'Std/Float.Convert.c'],
    ['Float.Math', 'Std/Float.Math.c'],
]

bitvec_deadend_tests = [
    ['BitVec.Access.Deadend', 'Std/BitVec.Access.Deadend.c'],
    ['BitVec.BitWise.Deadend', 'Std/BitVec.BitWise.Deadend.c'],
    ['BitVec.Pattern.Deadend', 'Std/BitVec.Pattern.Deadend.c'],
]
if opt_list
    # Foreach.Deadend touches List runtime helpers to assert iterator
    # type mismatches, so it needs the optional `list` feature.
    bitvec_deadend_tests += [['BitVec.Foreach.Deadend', 'Std/BitVec.Foreach.Deadend.c']]
endif

json_tests = [
    ['Json.Read.Simple', 'Json/Read.Simple.c'],
    ['Json.Read.Nested', 'Json/Read.Nested.c'],
    ['Json.Read.EdgeCases', 'Json/Read.EdgeCases.c'],
    ['Json.Write.Simple', 'Json/Write.Simple.c'],
    ['Json.Write.Nested', 'Json/Write.Nested.c'],
    ['Json.Write.EdgeCases', 'Json/Write.EdgeCases.c'],
    ['Json.RoundTrip', 'Json/RoundTrip.c'],
#['Json.RW', 'Json/RW.c'],
]

kvconfig_tests = [
    ['KvConfig.Parse', 'KvConfig/Parse.c'],
]

allocator_tests = [
    # Page and Heap are foundation.
    ['Allocator.Page', 'Std/Allocator.Page.c'],
    ['Allocator.Heap', 'Std/Allocator.Heap.c'],
]
if opt_alloc_arena
    allocator_tests += [['Allocator.Arena', 'Std/Allocator.Arena.c']]
endif
if opt_alloc_slab
    allocator_tests += [['Allocator.Slab', 'Std/Allocator.Slab.c']]
endif
if opt_alloc_budget
    allocator_tests += [['Allocator.Budget', 'Std/Allocator.Budget.c']]
endif

#Io tests exercise format specifiers for every supported type
#(Int, Float, BitVec included). Skip when any of those features
#are disabled.
if opt_bitvec and opt_int and opt_float
    foreach test_info : io_tests
        test_name = test_info[0]
        test_src = test_info[1]

        test_exe = executable(
            test_name,
            files(test_src),
            link_with: [misra_std_no_backtrace],
            dependencies: [test_util_dep_no_backtrace],
            c_args: test_args,
            include_directories: inc_misra,
            install: false
        )

        test(test_name, test_exe)
    endforeach
endif

if opt_file
    foreach test_info : file_tests
        test_name = test_info[0]
        test_src = test_info[1]

        test_exe = executable(
            test_name,
            files(test_src),
            link_with: [misra_std_no_backtrace],
            dependencies: [test_util_dep_no_backtrace],
            c_args: test_args,
            include_directories: inc_misra,
            install: false
        )

        test(test_name, test_exe)
    endforeach
endif

# ArgParse is foundation -- always compile, always test.
foreach test_info : argparse_tests
    test_name = test_info[0]
    test_src = test_info[1]

    test_exe = executable(
        test_name,
        files(test_src),
        link_with: [misra_std_no_backtrace],
        dependencies: [test_util_dep_no_backtrace],
        c_args: test_args,
        include_directories: inc_misra,
        install: false
    )

    test(test_name, test_exe)
endforeach

if opt_sys_socket
    foreach test_info : socket_tests
        test_name = test_info[0]
        test_src = test_info[1]

        test_exe = executable(
            test_name,
            files(test_src),
            link_with: [misra_std_no_backtrace],
            dependencies: [test_util_dep_no_backtrace],
            c_args: test_args,
            include_directories: inc_misra,
            install: false
        )

        test(test_name, test_exe)
    endforeach
endif

if opt_parser_http
    foreach test_info : http_tests
        test_name = test_info[0]
        test_src = test_info[1]

        test_exe = executable(
            test_name,
            files(test_src),
            link_with: [misra_std_no_backtrace],
            dependencies: [test_util_dep_no_backtrace],
            c_args: test_args,
            include_directories: inc_misra,
            install: false
        )

        test(test_name, test_exe)
    endforeach
endif

# The PE parser test builds a synthetic PE blob in memory, so it
# doesn't need a real Windows host -- it runs on every platform.
if opt_parser_pe
    foreach test_info : pe_tests
        test_name = test_info[0]
        test_src = test_info[1]

        test_exe = executable(
            test_name,
            files(test_src),
            link_with: [misra_std_no_backtrace],
            dependencies: [test_util_dep_no_backtrace],
            c_args: test_args,
            include_directories: inc_misra,
            install: false
        )

        test(test_name, test_exe)
    endforeach
endif

# Same story for the PDB parser: synthetic MSF blob, no real Windows
# toolchain needed.
if opt_parser_pdb
    foreach test_info : pdb_tests
        test_name = test_info[0]
        test_src = test_info[1]

        test_exe = executable(
            test_name,
            files(test_src),
            link_with: [misra_std_no_backtrace],
            dependencies: [test_util_dep_no_backtrace],
            c_args: test_args,
            include_directories: inc_misra,
            install: false
        )

        test(test_name, test_exe)
    endforeach

    foreach test_info : pdb_cache_tests
        test_name = test_info[0]
        test_src = test_info[1]

        test_exe = executable(
            test_name,
            files(test_src),
            link_with: [misra_std_no_backtrace],
            dependencies: [test_util_dep_no_backtrace],
            c_args: test_args,
            include_directories: inc_misra,
            install: false
        )

        test(test_name, test_exe)
    endforeach
endif

# Same model: synthetic Mach-O blob, no actual macOS host needed.
if opt_parser_macho
    foreach test_info : macho_tests
        test_name = test_info[0]
        test_src = test_info[1]

        test_exe = executable(
            test_name,
            files(test_src),
            link_with: [misra_std_no_backtrace],
            dependencies: [test_util_dep_no_backtrace],
            c_args: test_args,
            include_directories: inc_misra,
            install: false
        )

        test(test_name, test_exe)
    endforeach

    # MachoCache test builds a dSYM bundle directory tree via
    # `system("mkdir -p ...")` / `rm -rf ...`. Those are POSIX shell
    # idioms; cmd.exe on Windows doesn't speak them. Gate the test
    # to non-Windows -- the MachoCache code itself is portable C99,
    # but the dSYM concept is macOS-only, so Windows coverage adds
    # little.
    if host_machine.system() != 'windows'
        foreach test_info : macho_cache_tests
            test_name = test_info[0]
            test_src = test_info[1]

            test_exe = executable(
                test_name,
                files(test_src),
                link_with: [misra_std_no_backtrace],
                dependencies: [test_util_dep_no_backtrace],
                c_args: test_args,
                include_directories: inc_misra,
                install: false
            )

            test(test_name, test_exe)
        endforeach
    endif
endif

# Parser/Dns: pure wire-format codec test. No network, no host
# dependency -- builds on every platform that has the parser enabled.
if opt_parser_dns
    foreach test_info : dns_tests
        test_name = test_info[0]
        test_src = test_info[1]

        test_exe = executable(
            test_name,
            files(test_src),
            link_with: [misra_std_no_backtrace],
            dependencies: [test_util_dep_no_backtrace],
            c_args: test_args,
            include_directories: inc_misra,
            install: false
        )

        test(test_name, test_exe)
    endforeach
endif

# Sys/Dns: integration test using /etc/hosts only (no network calls).
# Requires Sys/Socket so we can build the SocketAddr results. Gated to
# POSIX hosts because Windows's default hosts file is comment-only
# (Windows uses kernel DNS via iphlpapi for `localhost`), so the
# `localhost`-from-hosts assertions would fail there. Re-enable on
# Windows once the iphlpapi-backed nameserver discovery is wired in
# (tracked in FUTURE-PLANS).
if opt_sys_dns and host_machine.system() != 'windows'
    foreach test_info : sys_dns_tests
        test_name = test_info[0]
        test_src = test_info[1]

        test_exe = executable(
            test_name,
            files(test_src),
            link_with: [misra_std_no_backtrace],
            dependencies: [test_util_dep_no_backtrace],
            c_args: test_args,
            include_directories: inc_misra,
            install: false
        )

        test(test_name, test_exe)
    endforeach
endif

# Parser/Elf, Parser/Dwarf, and the Sys/* resolve / backtrace tests
# all open /proc/self/exe at runtime and use GCC-specific attributes
# (__attribute__((noinline)), __asm__ __volatile__). The library code
# itself is portable; only the tests are Linux + GCC/Clang-only.
tests_need_posix_exe = host_machine.system() == 'linux'

if opt_parser_elf and tests_need_posix_exe
    foreach test_info : elf_tests
        test_name = test_info[0]
        test_src = test_info[1]

        # Force a build-id note so the build_id-parsing test path
        # exercises the parser. Most distros' linkers default to
        # emitting one but not all; the explicit flag makes the test
        # portable across configurations.
        test_exe = executable(
            test_name,
            files(test_src),
            link_with: [misra_std_no_backtrace],
            dependencies: [test_util_dep_no_backtrace],
            c_args: test_args,
            include_directories: inc_misra,
            link_args: ['-Wl,--build-id=sha1'],
            install: false
        )

        # Elf test parses /proc/self/exe (the sanitised test binary
        # itself -- large, full debug info) four times back-to-back.
        # Every byte read goes through ASan's shadow-byte check; on a
        # shared GitHub Ubuntu runner this clocks in around 50s, leaving
        # no margin above the 60s timeout. Bump it to be safe.
        test(test_name, test_exe, timeout: 120)
    endforeach
endif

if opt_parser_dwarf and tests_need_posix_exe
    foreach test_info : dwarf_tests
        test_name = test_info[0]
        test_src = test_info[1]

        # The dwarf parser is DWARF 4 only in v1; modern GCC defaults
        # to DWARF 5, so force the test binary to emit v4 debug info.
        # DWARF 5 support is in FUTURE-PLANS.md.
        test_exe = executable(
            test_name,
            files(test_src),
            link_with: [misra_std_no_backtrace],
            dependencies: [test_util_dep_no_backtrace],
            c_args: test_args + ['-gdwarf-4'],
            include_directories: inc_misra,
            link_args: ['-rdynamic'],
            install: false
        )

        # Dwarf parser test directly exercises SymbolResolver, which
        # probes many external-debug paths (/usr/lib/debug/.build-id/*
        # etc.) that don't exist on CI runners -- each failed open is
        # cheap but the chain repeats per symbol. Give explicit room
        # over the default 30 s * multiplier so CI doesn't flake.
        test(test_name, test_exe, timeout: 120)
    endforeach

    # Stripped-binary integration test. The same source produces both
    # the test driver (unstripped, so it can reflect on its own symbols)
    # and a stripped copy (with .symtab + .strtab removed but
    # .debug_info kept). The driver opens both and verifies the
    # stripped copy still names the marker via .debug_info alone.
    objcopy = find_program('objcopy', required: false)
    if objcopy.found()
        stripped_driver = executable(
            'Dwarf.Stripped',
            files('Std/StrippedResolve.c'),
            link_with: [misra_std_no_backtrace],
            dependencies: [test_util_dep_no_backtrace],
            c_args: test_args + ['-gdwarf-4'],
            include_directories: inc_misra,
            link_args: ['-rdynamic'],
            install: false
        )

        stripped_target = custom_target(
            'Dwarf.Stripped.stripped',
            input: stripped_driver,
            output: 'Dwarf.Stripped.stripped',
            # `-N name` removes that single symbol from .symtab while
            # leaving the rest of the file (including .debug_info)
            # intact. That's exactly the failure mode we want to
            # simulate: a symbol present in DWARF but missing from the
            # ELF symbol tables. (`-R .symtab` would be cleaner but
            # current binutils silently refuses to drop the section.)
            command: [
                objcopy,
                '-N', 'marker_alpha',
                '-N', 'marker_beta',
                '@INPUT@',
                '@OUTPUT@',
            ],
            build_by_default: true
        )

        # Same SymbolResolver-driven slowness as the Dwarf parser test;
        # see comment above.
        test('Dwarf.Stripped', stripped_driver, args: [stripped_target.full_path()], timeout: 120)
    endif
endif

if opt_sys_procmaps and tests_need_posix_exe
    foreach test_info : procmaps_tests
        test_name = test_info[0]
        test_src = test_info[1]

        test_exe = executable(
            test_name,
            files(test_src),
            link_with: [misra_std_no_backtrace],
            dependencies: [test_util_dep_no_backtrace],
            c_args: test_args,
            include_directories: inc_misra,
            install: false
        )

        test(test_name, test_exe)
    endforeach
endif

if opt_sys_symresolve and tests_need_posix_exe
    foreach test_info : symresolve_tests
        test_name = test_info[0]
        test_src = test_info[1]

        test_exe = executable(
            test_name,
            files(test_src),
            link_with: [misra_std_no_backtrace],
            dependencies: [test_util_dep_no_backtrace],
            c_args: test_args,
            include_directories: inc_misra,
            link_args: ['-rdynamic'],
            install: false
        )

        # SymbolResolver test probes external debug-symbol paths
        # (/usr/lib/debug/.build-id/*) that don't exist on CI runners.
        # Each ENOENT is cheap on its own, but ASan + UBSan instrumentation
        # over the full resolve chain on a large sanitised binary pushes
        # this near the 60s default. Bump.
        test(test_name, test_exe, timeout: 120)
    endforeach
endif

# Backtrace tests run wherever Sys/Backtrace itself is supported:
# Linux (FP walk + ELF/DWARF) or Darwin (FP walk + dyld + Mach-O).
# Windows is skipped here -- we have no test host and the
# CaptureStackBackTrace + dbghelp path is its own beast.
backtrace_test_platforms = (host_machine.system() == 'linux' or host_machine.system() == 'darwin')
if opt_sys_backtrace and backtrace_test_platforms
    backtrace_test_c_args   = test_args + ['-fno-omit-frame-pointer', '-gdwarf-4']
    backtrace_test_link_args = []
    if host_machine.system() == 'linux'
        # `-rdynamic` keeps helper symbols visible to .dynsym so
        # SymbolResolver (ELF-based) can find them via dlopen-style
        # lookups. On Darwin there's no `.dynsym`; LC_SYMTAB already
        # carries static-function names by default, and `ld64` doesn't
        # accept `-rdynamic`.
        backtrace_test_link_args += '-rdynamic'
    endif

    foreach test_info : backtrace_tests
        test_name = test_info[0]
        test_src = test_info[1]

        test_exe = executable(
            test_name,
            files(test_src),
            link_with: [misra_std_no_backtrace],
            dependencies: [test_util_dep_no_backtrace],
            c_args: backtrace_test_c_args,
            include_directories: inc_misra,
            link_args: backtrace_test_link_args,
            install: false
        )

        # The Backtrace test exercises the full symbolisation chain
        # (CaptureStackBackTrace + SymbolResolver + DWARF lookups) on
        # purpose. CI runners without debug-symbol packages spend
        # noticeable time hitting ENOENT on the build-id paths the
        # resolver probes. Headroom over the global multiplier.
        test(test_name, test_exe, timeout: 120)
    endforeach
endif

foreach test_info : page_protect_tests
    test_name = test_info[0]
    test_src = test_info[1]

    test_exe = executable(
        test_name,
        files(test_src),
        link_with: [misra_std_no_backtrace],
        dependencies: [test_util_dep_no_backtrace],
        c_args: test_args,
        include_directories: inc_misra,
        install: false
    )

    test(test_name, test_exe)
endforeach

if opt_alloc_debug
    foreach test_info : alloc_debug_tests
        test_name = test_info[0]
        test_src = test_info[1]

        test_exe = executable(
            test_name,
            files(test_src),
            link_with: [misra_std_no_backtrace],
            dependencies: [test_util_dep_no_backtrace],
            c_args: test_args,
            include_directories: inc_misra,
            install: false
        )

        test(test_name, test_exe)
    endforeach
endif

foreach test_info : str_tests
    test_name = test_info[0]
    test_src = test_info[1]

    test_exe = executable(
        test_name,
        files(test_src),
        link_with: [misra_std_no_backtrace],
        dependencies: [test_util_dep_no_backtrace],
        c_args: test_args,
        include_directories: inc_misra,
        install: false
    )

    test(test_name, test_exe)
endforeach

foreach test_info : iter_tests + iter_deadend_tests + buf_tests
    test_name = test_info[0]
    test_src = test_info[1]

    test_exe = executable(
        test_name,
        files(test_src),
        link_with: [misra_std_no_backtrace],
        dependencies: [test_util_dep_no_backtrace],
        c_args: test_args,
        include_directories: inc_misra,
        install: false
    )

    test(test_name, test_exe)
endforeach

foreach test_info : vec_tests
    test_name = test_info[0]
    test_src = test_info[1]
    
    test_exe = executable(
        test_name,
        files(test_src),
        link_with: [misra_std_no_backtrace],
        dependencies: [test_util_dep_no_backtrace],
        c_args: test_args,
        include_directories: inc_misra,
        install: false
    )
    
    test(test_name, test_exe)
endforeach 

if opt_graph
    foreach test_info : graph_tests
        test_name = test_info[0]
        test_src = test_info[1]

        test_exe = executable(
            test_name,
            files(test_src),
            link_with: [misra_std_no_backtrace],
            dependencies: [test_util_dep_no_backtrace],
            c_args: test_args,
            include_directories: inc_misra,
            install: false
        )

        test(test_name, test_exe)
    endforeach
endif

if opt_list
    foreach test_info : list_tests
        test_name = test_info[0]
        test_src = test_info[1]

        test_exe = executable(
            test_name,
            files(test_src),
            link_with: [misra_std_no_backtrace],
            dependencies: [test_util_dep_no_backtrace],
            c_args: test_args,
            include_directories: inc_misra,
            install: false
        )

        test(test_name, test_exe)
    endforeach
endif

if opt_map
    foreach test_info : map_tests
        test_name = test_info[0]
        test_src = test_info[1]

        test_exe = executable(
            test_name,
            files(test_src),
            link_with: [misra_std_no_backtrace],
            dependencies: [test_util_dep_no_backtrace],
            c_args: test_args,
            include_directories: inc_misra,
            install: false
        )

        test(test_name, test_exe)
    endforeach
endif

if opt_parser_json
    foreach test_info : json_tests
        test_name = test_info[0]
        test_src = test_info[1]

        test_exe = executable(
            test_name,
            files(test_src),
            link_with: [misra_std_no_backtrace],
            dependencies: [test_util_dep_no_backtrace],
            c_args: test_args,
            include_directories: inc_misra,
            install: false
        )

        test(test_name, test_exe)
    endforeach
endif

if opt_parser_kvconf
    foreach test_info : kvconfig_tests
        test_name = test_info[0]
        test_src = test_info[1]

        test_exe = executable(
            test_name,
            files(test_src),
            link_with: [misra_std_no_backtrace],
            dependencies: [test_util_dep_no_backtrace],
            c_args: test_args,
            include_directories: inc_misra,
            install: false
        )

        test(test_name, test_exe)
    endforeach
endif

foreach test_info : allocator_tests
    test_name = test_info[0]
    test_src = test_info[1]

    test_exe = executable(
        test_name,
        files(test_src),
        link_with: [misra_std_no_backtrace],
        dependencies: [test_util_dep_no_backtrace],
        c_args: test_args,
        include_directories: inc_misra,
        install: false
    )

    test(test_name, test_exe)
endforeach

if opt_bitvec
    foreach test_info : bitvec_tests
        test_name = test_info[0]
        test_src = test_info[1]

        test_exe = executable(
            test_name,
            files(test_src),
            link_with: [misra_std_no_backtrace],
            dependencies: [test_util_dep_no_backtrace],
            c_args: test_args,
            include_directories: inc_misra,
            install: false
        )

        test(test_name, test_exe)
    endforeach
endif

if opt_int
    foreach test_info : int_tests
        test_name = test_info[0]
        test_src = test_info[1]

        test_exe = executable(
            test_name,
            files(test_src),
            link_with: [misra_std_no_backtrace],
            dependencies: [test_util_dep_no_backtrace],
            c_args: test_args,
            include_directories: inc_misra,
            install: false
        )

        test(test_name, test_exe)
    endforeach
endif

if opt_float
    foreach test_info : float_tests
        test_name = test_info[0]
        test_src = test_info[1]

        test_exe = executable(
            test_name,
            files(test_src),
            link_with: [misra_std_no_backtrace],
            dependencies: [test_util_dep_no_backtrace],
            c_args: test_args,
            include_directories: inc_misra,
            install: false
        )

        # Float tests exercise BitVec / Int / Float chains heavily; under
        # sanitizer + per-call allocator magic validation they run for
        # well over the meson default 30s. Give them generous headroom so
        # CI doesn't flake on slow runners.
        test(test_name, test_exe, timeout: 120)
    endforeach
endif

if opt_bitvec
    foreach test_info : bitvec_deadend_tests
        test_name = test_info[0]
        test_src = test_info[1]

        test_exe = executable(
            test_name,
            files(test_src),
            link_with: [misra_std_no_sanitizers],
            dependencies: [test_util_dep_no_sanitizers],
            c_args: test_args,
            include_directories: inc_misra,
            install: false,
            override_options: ['b_sanitize=none']
        )

        test(test_name, test_exe)
    endforeach
endif
