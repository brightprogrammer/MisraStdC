project('misra', 'c',
    default_options: ['buildtype=debug', 'optimization=0', 'b_lto=false', 'c_std=c11']
)

# Custom compile args
common_c_args = []

cc = meson.get_compiler('c')
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

opt_bitvec_user    = get_option('bitvec')
opt_list           = get_option('list')
opt_map_user       = get_option('map')
opt_graph          = get_option('graph')
opt_int_user       = get_option('int')
opt_float          = get_option('float')

opt_file           = get_option('file')
opt_iter           = get_option('iter')

opt_sys_dir        = get_option('sys_dir')
opt_sys_proc       = get_option('sys_proc')

opt_parser_json    = get_option('parser_json')
opt_parser_kvconf  = get_option('parser_kvconfig')

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
opt_map    = opt_map_user    or opt_parser_kvconf               # kvconfig -> map

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
    'Source/Misra/Std/Allocator.c',
    'Source/Misra/Std/Allocator/Page.c',
    'Source/Misra/Std/Allocator/Heap.c',
    'Source/Misra/Std/Container/Vec.c',
    'Source/Misra/Std/Container/Str.c',
    'Source/Misra/Std/Io.c',
)

if opt_alloc_arena
    misra_std_sources += files('Source/Misra/Std/Allocator/Arena.c')
endif
if opt_alloc_slab
    misra_std_sources += files('Source/Misra/Std/Allocator/Slab.c')
endif
if opt_alloc_budget
    misra_std_sources += files('Source/Misra/Std/Allocator/Budget.c')
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

if opt_parser_json
    misra_std_sources += files('Source/Misra/Parsers/JSON.c')
endif
if opt_parser_kvconf
    misra_std_sources += files('Source/Misra/Parsers/KvConfig.c')
endif

# Compile-time discovery: each enabled feature exposes MISRA_HAVE_<NAME>=1
# so downstream code can `#ifdef` against a partial build.
feature_defines = []
if opt_alloc_arena
    feature_defines += '-DMISRA_HAVE_ALLOC_ARENA=1'
endif
if opt_alloc_slab
    feature_defines += '-DMISRA_HAVE_ALLOC_SLAB=1'
endif
if opt_alloc_budget
    feature_defines += '-DMISRA_HAVE_ALLOC_BUDGET=1'
endif
if opt_bitvec
    feature_defines += '-DMISRA_HAVE_BITVEC=1'
endif
if opt_list
    feature_defines += '-DMISRA_HAVE_LIST=1'
endif
if opt_map
    feature_defines += '-DMISRA_HAVE_MAP=1'
endif
if opt_graph
    feature_defines += '-DMISRA_HAVE_GRAPH=1'
endif
if opt_int
    feature_defines += '-DMISRA_HAVE_INT=1'
endif
if opt_float
    feature_defines += '-DMISRA_HAVE_FLOAT=1'
endif
if opt_file
    feature_defines += '-DMISRA_HAVE_FILE=1'
endif
if opt_iter
    feature_defines += '-DMISRA_HAVE_ITER=1'
endif
if opt_sys_dir
    feature_defines += '-DMISRA_HAVE_SYS_DIR=1'
endif
if opt_sys_proc
    feature_defines += '-DMISRA_HAVE_SYS_PROC=1'
endif
if opt_parser_json
    feature_defines += '-DMISRA_HAVE_PARSER_JSON=1'
endif
if opt_parser_kvconf
    feature_defines += '-DMISRA_HAVE_PARSER_KVCONFIG=1'
endif

common_c_args += feature_defines

# Print which optional features are enabled so the configure log is honest.
message('MisraStdC features enabled:')
message('  alloc_arena=@0@  alloc_slab=@1@  alloc_budget=@2@'.format(
    opt_alloc_arena, opt_alloc_slab, opt_alloc_budget))
message('  bitvec=@0@  list=@1@  map=@2@  graph=@3@  int=@4@  float=@5@'.format(
    opt_bitvec, opt_list, opt_map, opt_graph, opt_int, opt_float))
message('  file=@0@  iter=@1@  sys_dir=@2@  sys_proc=@3@'.format(
    opt_file, opt_iter, opt_sys_dir, opt_sys_proc))
message('  parser_json=@0@  parser_kvconfig=@1@'.format(
    opt_parser_json, opt_parser_kvconf))

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

# Bin/* executables are built only when the features they depend on are
# enabled. Each call site reads `ReadCompleteFile`, `SysProc*`,
# `SysDirContents`, etc., so we gate them on the corresponding flag.
if opt_file
    executable(
        'MisraEnum',
        files('Bin/MisraEnum.c'),
        link_with: [misra_std],
        c_args: common_c_args,
        include_directories: inc_misra,
        install: false
    )
endif

if opt_sys_proc
    executable(
        'SubProcComm',
        files('Bin/SubProcComm.c'),
        link_with: [misra_std],
        c_args: common_c_args,
        include_directories: inc_misra,
        install: false
    )
endif

if opt_file and opt_sys_dir
    executable(
        'MisraDoc',
        files('Bin/MisraDoc.c'),
        link_with: [misra_std],
        c_args: common_c_args,
        include_directories: inc_misra,
        install: false
    )
endif

if opt_file
    executable(
        'Demangler',
        files('Bin/Demangler/Main.c'),
        link_with: [misra_std],
        c_args: common_c_args,
        include_directories: inc_misra,
        install: false
    )
endif

if opt_file
    executable(
        'ElfInfo',
        files('Bin/ElfInfo.c'),
        link_with: [misra_std],
        c_args: common_c_args,
        include_directories: inc_misra,
        install: false
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
    )
endif

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
