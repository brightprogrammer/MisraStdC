project('misra', 'c',
    default_options: ['buildtype=debug', 'optimization=0', 'b_lto=false', 'c_std=c11']
)

cc = meson.get_compiler('c')
warning_flags = []
if cc.get_id() in ['gcc', 'clang']
  warning_flags += [
    '-Wall',
    '-Wextra',
    '-Wpedantic',
    '-Werror',
  ]
elif cc.get_id() == 'msvc'
  warning_flags += [
    '/W4',     # Warning level 4
    '/WX',     # Treat warnings as errors
  ]
endif

add_project_arguments(warning_flags, language: 'c')

# Custom compile args
common_c_args = []

inc_misra = include_directories('Source', 'Include')

# Add the misra_std library
misra_std_sources = files(
    'Source/Misra/Sys.c',
    'Source/Misra/Std/Io.c',
    'Source/Misra/Std/Log.c',
    'Source/Misra/Std/File.c',
    'Source/Misra/Std/Container/Vec.c',
    'Source/Misra/Std/Container/Str.c',
    'Source/Misra/Std/Utility/Iter.c',
    'Source/Misra/Parsers/JSON.c'
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

executable(
    'Demangler',
    files('Bin/Demangler/Main.c'),
    link_with: [misra_std],
    c_args: common_c_args,
    include_directories: inc_misra,
    install: true
)
