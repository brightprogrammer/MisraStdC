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
    'Source/Misra/Std/Io.c',
    'Source/Misra/Std/Log.c',
    'Source/Misra/Std/File.c',
    'Source/Misra/Std/Memory.c',
    'Source/Misra/Std/Container/Vec.c',
    'Source/Misra/Std/Container/Str.c',
    'Source/Misra/Std/Utility/Iter.c',
    'Source/Misra/Parsers/JSON.c'
)

misra_std = static_library(
    'misra_std',
    misra_std_sources,
    c_args: common_c_args,
    dependencies: math_dep,
    include_directories: inc_misra,
    install: true
)

# Main.c and misra executable have been removed

executable(
    'MisraEnum',
    files('Bin/MisraEnum.c'),
    link_with: [misra_std],
    c_args: common_c_args,
    include_directories: inc_misra,
    install: true
)

executable(
    'MisraDoc',
    files('Bin/MisraDoc.c'),
    link_with: [misra_std],
    c_args: common_c_args,
    include_directories: inc_misra,
    install: true
)

executable(
    'Demangler',
    files('Bin/Demangler/Main.c'),
    link_with: [misra_std],
    c_args: common_c_args,
    include_directories: inc_misra,
    install: true
)

# Include tests
subdir('Tests')
