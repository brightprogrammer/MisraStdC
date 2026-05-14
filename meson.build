project('misra', 'c',
    default_options: ['buildtype=debug', 'optimization=0', 'b_lto=false', 'c_std=c11']
)

# Custom compile args
common_c_args = []

cc = meson.get_compiler('c')
math_dep = cc.find_library('m', required: false)

inc_misra = include_directories('Source', 'Include')

# Add the misra_std library
misra_std_sources = files(
    'Source/Misra/Sys.c',
    'Source/Misra/Sys/Mutex.c',
    'Source/Misra/Sys/Proc.c',
    'Source/Misra/Sys/Dir.c',
    'Source/Misra/Std/Allocator.c',
    'Source/Misra/Std/Allocator/Page.c',
    'Source/Misra/Std/Allocator/Arena.c',
    'Source/Misra/Std/Io.c',
    'Source/Misra/Std/Log.c',
    'Source/Misra/Std/File.c',
    'Source/Misra/Std/Memory.c',
    'Source/Misra/Std/Container/Vec.c',
    'Source/Misra/Std/Container/Str.c',
    'Source/Misra/Std/Container/Graph.c',
    'Source/Misra/Std/Container/List.c',
    'Source/Misra/Std/Container/Map.c',
    'Source/Misra/Std/Container/BitVec.c',
    'Source/Misra/Std/Container/Int.c',
    'Source/Misra/Std/Container/Float.c',
    'Source/Misra/Std/Utility/Iter.c',
    'Source/Misra/Parsers/JSON.c',
    'Source/Misra/Parsers/KvConfig.c'
)

# Regular library with sanitizers for normal tests and executables
misra_std = static_library(
    'misra_std',
    misra_std_sources,
    c_args: common_c_args,
    dependencies: math_dep,
    include_directories: inc_misra,
    install: true
)

# Dependency for working with other projects
misra_std_dep = declare_dependency(
    include_directories : inc_misra,
    link_with: misra_std,
    dependencies: [math_dep],
)

# Non-sanitized library specifically for deadend tests to avoid memory leak reports
misra_std_no_sanitizers = static_library(
    'misra_std_no_sanitizers',
    misra_std_sources,
    c_args: common_c_args,
    dependencies: math_dep,
    include_directories: inc_misra,
    install: false,
    override_options: ['b_sanitize=none']
)

# Main.c and misra executable have been removed

executable(
    'MisraEnum',
    files('Bin/MisraEnum.c'),
    link_with: [misra_std],
    c_args: common_c_args,
    include_directories: inc_misra,
    install: false
)

executable(
    'SubProcComm',
    files('Bin/SubProcComm.c'),
    link_with: [misra_std],
    c_args: common_c_args,
    include_directories: inc_misra,
    install: false
)

executable(
    'MisraDoc',
    files('Bin/MisraDoc.c'),
    link_with: [misra_std],
    c_args: common_c_args,
    include_directories: inc_misra,
    install: false
)

executable(
    'Demangler',
    files('Bin/Demangler/Main.c'),
    link_with: [misra_std],
    c_args: common_c_args,
    include_directories: inc_misra,
    install: false
)

executable(
    'ElfInfo',
    files('Bin/ElfInfo.c'),
    link_with: [misra_std],
    c_args: common_c_args,
    include_directories: inc_misra,
    install: false
)

# Fuzzing harness executable (modular)
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
)

# Include tests
subdir('Tests')

# Install library headers
install_subdir('Include/Misra', install_dir: 'include')
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
