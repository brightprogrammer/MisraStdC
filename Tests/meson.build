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
    ['List.Access', 'Std/List.Access.c'],
]

map_tests = [
    ['Map.Type', 'Std/Map.Type.c'],
    ['Map.Init', 'Std/Map.Init.c'],
    ['Map.Access', 'Std/Map.Access.c'],
    ['Map.Insert', 'Std/Map.Insert.c'],
    ['Map.Remove', 'Std/Map.Remove.c'],
    ['Map.Foreach', 'Std/Map.Foreach.c'],
    ['Map.Ops', 'Std/Map.Ops.c'],
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
    ['BitVec.Foreach.Deadend', 'Std/BitVec.Foreach.Deadend.c'],
]

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

#Build and register all normal tests with full sanitizer support
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

    test(test_name, test_exe)
endforeach

foreach test_info : bitvec_deadend_tests
    test_name = test_info[0]
    test_src = test_info[1]
    
    test_exe = executable(
        test_name,
        files(test_src),
        link_with: [misra_std_no_sanitizers],  # Use the non-sanitized library
        dependencies: [test_util_dep_no_sanitizers],  # Use non-sanitized test utils
        c_args: test_args,
        include_directories: inc_misra,
        install: false,
        override_options: ['b_sanitize=none']  # Disable all sanitizers for deadend tests
    )
    
    test(test_name, test_exe)
endforeach
