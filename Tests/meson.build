# Tests meson.build file
#
# Note: If you see a warning about sanitizers on Clang with b_lundef,
# you can fix it by setting b_lundef=false in your meson command:
#   meson setup builddir -Db_sanitize=address,undefined -Db_lundef=false
#
# Or add this to your meson_options.txt file:
#   option('b_lundef', type: 'boolean', value: false)

# Include test utilities
subdir('Util')

# Test configuration
test_args = common_c_args

# Group tests by module
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
    ['Str.Foreach.Simple', 'Std/Str.Foreach.Simple.c'],
    ['Str.Ops', 'Std/Str.Ops.c'],
]

str_deadend_tests = [
    ['Str.Foreach.Deadend', 'Std/Str.Foreach.Deadend.c'],
]

vec_tests = [
    ['Vec.Type', 'Std/Vec.Type.c'],
    ['Vec.Init', 'Std/Vec.Init.c'],
    ['Vec.Access', 'Std/Vec.Access.c'],
    ['Vec.Memory', 'Std/Vec.Memory.c'],
    ['Vec.Insert', 'Std/Vec.Insert.c'],
    ['Vec.Remove', 'Std/Vec.Remove.c'],
    ['Vec.Foreach.Simple', 'Std/Vec.Foreach.Simple.c'],
    ['Vec.Ops', 'Std/Vec.Ops.c'],
    ['Vec.Complex', 'Std/Vec.Complex.c'],
]

vec_deadend_tests = [
    ['Vec.Foreach.Deadend', 'Std/Vec.Foreach.Deadend.c'],
]

json_tests = [
    ['Json.Read.Simple', 'Json/Read.Simple.c'],
    ['Json.Read.Nested', 'Json/Read.Nested.c'],
    ['Json.Read.EdgeCases', 'Json/Read.EdgeCases.c'],
    ['Json.Write.Simple', 'Json/Write.Simple.c'],
    ['Json.Write.Nested', 'Json/Write.Nested.c'],
    ['Json.Write.EdgeCases', 'Json/Write.EdgeCases.c'],
    ['Json.RoundTrip', 'Json/RoundTrip.c'],
    # ['Json.RW', 'Json/RW.c'],
]

# Build and register all normal tests with full sanitizer support
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

# Build and register deadend tests WITHOUT sanitizers to avoid memory leak reports
foreach test_info : str_deadend_tests
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

foreach test_info : vec_deadend_tests
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
