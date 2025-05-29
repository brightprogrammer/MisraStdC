# Tests meson.build file
#
# Note: If you see a warning about sanitizers on Clang with b_lundef,
# you can fix it by setting b_lundef=false in your meson command:
#   meson setup builddir -Db_sanitize=address,undefined -Db_lundef=false
#
# Or add this to your meson_options.txt file:
#   option('b_lundef', type: 'boolean', value: false)

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
    ['Str.Foreach', 'Std/Str.Foreach.c'],
    ['Str.Ops', 'Std/Str.Ops.c'],
]

# Build and register all tests
foreach test_info : io_tests
    test_name = test_info[0]
    test_src = test_info[1]
    
    test_exe = executable(
        test_name,
        files(test_src),
        link_with: [misra_std],
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
        c_args: test_args,
        include_directories: inc_misra,
        install: false
    )
    
    test(test_name, test_exe)
endforeach 
