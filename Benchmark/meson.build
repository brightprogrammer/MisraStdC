# Allocator benchmark. Off by default; enable with -Dbenchmark=true.
#
# Builds eight binaries that share the same Google Benchmark suite and
# differ only in which allocator they exercise:
#
#   bench-glibc              -- default malloc resolution, no extra link
#   bench-jemalloc           -- -ljemalloc; libjemalloc interposes malloc
#   bench-mimalloc           -- -lmimalloc; libmimalloc interposes malloc
#   bench-tcmalloc           -- -ltcmalloc; libtcmalloc interposes malloc
#   bench-misra              -- HeapAllocator for every benchmark (the
#                               "wrong tool for the job" baseline -- it's
#                               general-purpose, applied to fixed-size
#                               workloads too)
#   bench-misra-correct      -- SlabAllocator for fixed-size benches,
#                               HeapAllocator for mixed-size benches.
#                               Honours the bench_use_fixed_size hint
#                               that the bench harness emits.
#   bench-misra-arena        -- ArenaAllocator: bump-pointer with bulk
#                               O(1) reset. Targets BM_ArenaBumpReset.
#   bench-misra-page         -- PageAllocator: one mmap per alloc, page-
#                               rounded. Targets page-aligned sizes.
#
# Each backend is one C/C++ source file plus the shared bench harness.
# The libc-shape backends share Allocator_libc.c; the four misra backends
# each have their own (Allocator_misra.c, Allocator_misra_correct.c,
# Allocator_misra_arena.c, Allocator_misra_page.c).
#
# All binaries link the same misra_std the parent project just built --
# no separate library build, no flake, no nix dependency. Run them via
# Scripts/run.py to refresh README.md.

# The benchmark harness is C++ (Google Benchmark headers are C++).
# Parent project is C-only; pull the C++ compiler in only when the
# benchmark is built so C-only consumers don't trip on C++ discovery.
add_languages('cpp', required: true, native: false)

bench_gbench_dep = dependency('benchmark', required: true,
    not_found_message: 'Google Benchmark not found; install pkg-config "benchmark" or disable -Dbenchmark.')

bench_jemalloc_dep = cc.find_library('jemalloc', required: true,
    has_headers: ['jemalloc/jemalloc.h'])
bench_mimalloc_dep = cc.find_library('mimalloc', required: true,
    has_headers: ['mimalloc.h'])
bench_tcmalloc_dep = cc.find_library('tcmalloc', required: true,
    has_headers: ['gperftools/malloc_extension_c.h'])

bench_cpp_sources = files(
    'Source/Bench.cpp',
    'Source/Fragmentation.cpp',
    'Source/Main.cpp',
)

bench_inc = [include_directories('Source'), inc_misra]

# Libc-shape backends. Same C wrapper, four link variants.
bench_libc_backends = [
    {'name': 'glibc',    'deps': []},
    {'name': 'jemalloc', 'deps': [bench_jemalloc_dep]},
    {'name': 'mimalloc', 'deps': [bench_mimalloc_dep]},
    {'name': 'tcmalloc', 'deps': [bench_tcmalloc_dep]},
]

foreach b : bench_libc_backends
    name = b['name']
    executable(
        'bench-' + name,
        files('Source/Allocator_libc.c') + bench_cpp_sources,
        c_args: [
            '-DBENCH_BACKEND_NAME="' + name + '"',
            '-DBENCH_BACKEND_' + name.to_upper() + '=1',
        ],
        include_directories: bench_inc,
        dependencies: [bench_gbench_dep] + b['deps'],
        install: false,
    )
endforeach

# Misra backends. Each picks up the in-tree misra_std built above.
# Cross-TU inlining of the trivial `bench_alloc` / `bench_free` /
# `bench_realloc` wrappers into `Bench.cpp`'s hot loop relies on the
# project being configured with `-Db_lto=true`; without that the
# wrapper costs a real function call per iteration.
#
# Variants:
#   - misra         : HeapAllocator (the default in-tree path).
#   - misra-correct : same but with extra invariant assertions enabled.
#   - misra-arena   : ArenaAllocator. Bump-pointer with no per-alloc
#                     free; targets BM_ArenaBumpReset (bulk-O(1) reset).
#                     Pair-style benches leak until the next teardown,
#                     which the benchmarks tolerate.
#   - misra-page    : PageAllocator. One mmap per alloc rounded to a
#                     page; useful at page-aligned sizes
#                     (AllocFreePair/4096, /16384, /65536). Sub-page
#                     requests round up and waste the rest.
bench_misra_backends = [
    {'name': 'misra',         'src': 'Source/Allocator_misra.c'},
    {'name': 'misra-correct', 'src': 'Source/Allocator_misra_correct.c'},
    {'name': 'misra-arena',   'src': 'Source/Allocator_misra_arena.c'},
    {'name': 'misra-page',    'src': 'Source/Allocator_misra_page.c'},
]

foreach b : bench_misra_backends
    executable(
        'bench-' + b['name'],
        files(b['src']) + bench_cpp_sources,
        c_args: ['-DBENCH_MISRA_VARIANT_NAME="' + b['name'] + '"'],
        include_directories: bench_inc,
        dependencies: [bench_gbench_dep, misra_std_dep],
        install: false,
    )
endforeach
