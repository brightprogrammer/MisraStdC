project('misra', 'c',
    default_options: ['buildtype=debug', 'optimization=0', 'b_lto=false', 'c_std=c11']
)

# Custom compile args
common_c_args = []

inc_misra = include_directories('Source', 'Include')

# Add the misra_std library
misra_std_sources = files(
    'Source/Misra/Sys.c',
    'Source/Misra/Std/Log.c',
    'Source/Misra/Std/File.c',
    'Source/Misra/Std/Container/Vec.c',
    'Source/Misra/Std/Container/Str.c',
    'Source/Misra/Parsers/JSON.c',
    'Source/Misra/Parsers/C.c'
)

misra_std = static_library(
    'misra_std',
    misra_std_sources,
    c_args: common_c_args,
    include_directories: inc_misra,
    install: true
)

# Add the misra executable
misra_sources = files(
    'Main.c'
)

executable(
    'misra',
    misra_sources,
    link_with: [misra_std],
    c_args: common_c_args,
    include_directories: inc_misra,
    install: true

)

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
