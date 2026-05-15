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

#Test configuration
test_args = common_c_args

#Group tests by module
io_tests = [
    ['Io.Write', 'Std/Io.Write.c'],
    ['Io.Read', 'Std/Io.Read.c'],
]

file_tests = [
    ['File', 'Std/File.c'],
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
            link_with: [misra_std],
            dependencies: [test_util_dep],
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
            link_with: [misra_std],
            dependencies: [test_util_dep],
            c_args: test_args,
            include_directories: inc_misra,
            install: false
        )

        test(test_name, test_exe)
    endforeach
endif

if opt_sys_socket
    foreach test_info : socket_tests
        test_name = test_info[0]
        test_src = test_info[1]

        test_exe = executable(
            test_name,
            files(test_src),
            link_with: [misra_std],
            dependencies: [test_util_dep],
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
            link_with: [misra_std],
            dependencies: [test_util_dep],
            c_args: test_args,
            include_directories: inc_misra,
            install: false
        )

        test(test_name, test_exe)
    endforeach
endif

if opt_parser_elf
    foreach test_info : elf_tests
        test_name = test_info[0]
        test_src = test_info[1]

        test_exe = executable(
            test_name,
            files(test_src),
            link_with: [misra_std],
            dependencies: [test_util_dep],
            c_args: test_args,
            include_directories: inc_misra,
            install: false
        )

        test(test_name, test_exe)
    endforeach
endif

if opt_parser_dwarf
    foreach test_info : dwarf_tests
        test_name = test_info[0]
        test_src = test_info[1]

        # The dwarf parser is DWARF 4 only in v1; modern GCC defaults
        # to DWARF 5, so force the test binary to emit v4 debug info.
        # DWARF 5 support is in FUTURE-PLANS.md.
        test_exe = executable(
            test_name,
            files(test_src),
            link_with: [misra_std],
            dependencies: [test_util_dep],
            c_args: test_args + ['-gdwarf-4'],
            include_directories: inc_misra,
            link_args: ['-rdynamic'],
            install: false
        )

        test(test_name, test_exe)
    endforeach
endif

if opt_sys_procmaps
    foreach test_info : procmaps_tests
        test_name = test_info[0]
        test_src = test_info[1]

        test_exe = executable(
            test_name,
            files(test_src),
            link_with: [misra_std],
            dependencies: [test_util_dep],
            c_args: test_args,
            include_directories: inc_misra,
            install: false
        )

        test(test_name, test_exe)
    endforeach
endif

if opt_sys_symresolve
    foreach test_info : symresolve_tests
        test_name = test_info[0]
        test_src = test_info[1]

        test_exe = executable(
            test_name,
            files(test_src),
            link_with: [misra_std],
            dependencies: [test_util_dep],
            c_args: test_args,
            include_directories: inc_misra,
            link_args: ['-rdynamic'],
            install: false
        )

        test(test_name, test_exe)
    endforeach
endif

if opt_sys_backtrace
    foreach test_info : backtrace_tests
        test_name = test_info[0]
        test_src = test_info[1]

        # -fno-omit-frame-pointer guarantees the saved-FP chain our
        # unwinder walks. -rdynamic keeps helper symbols visible to
        # .symtab resolution. -gdwarf-4 produces .debug_line in a
        # form our DWARF parser understands (v5 support is in
        # FUTURE-PLANS).
        test_exe = executable(
            test_name,
            files(test_src),
            link_with: [misra_std],
            dependencies: [test_util_dep],
            c_args: test_args + ['-fno-omit-frame-pointer', '-gdwarf-4'],
            include_directories: inc_misra,
            link_args: ['-rdynamic'],
            install: false
        )

        test(test_name, test_exe)
    endforeach
endif

foreach test_info : page_protect_tests
    test_name = test_info[0]
    test_src = test_info[1]

    test_exe = executable(
        test_name,
        files(test_src),
        link_with: [misra_std],
        dependencies: [test_util_dep],
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
            link_with: [misra_std],
            dependencies: [test_util_dep],
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
        link_with: [misra_std],
        dependencies: [test_util_dep],
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
        link_with: [misra_std],
        dependencies: [test_util_dep],
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
            link_with: [misra_std],
            dependencies: [test_util_dep],
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
            link_with: [misra_std],
            dependencies: [test_util_dep],
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
            link_with: [misra_std],
            dependencies: [test_util_dep],
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
            link_with: [misra_std],
            dependencies: [test_util_dep],
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
            link_with: [misra_std],
            dependencies: [test_util_dep],
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
        link_with: [misra_std],
        dependencies: [test_util_dep],
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
            link_with: [misra_std],
            dependencies: [test_util_dep],
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
            link_with: [misra_std],
            dependencies: [test_util_dep],
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
            link_with: [misra_std],
            dependencies: [test_util_dep],
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
