# Allocator benchmark. Off by default; enable with -Dbenchmark=true.
#
# One binary per allocator (libc-shape and MisraStdC alike). Each
# binary shares the same Google Benchmark suite; backends differ only
# in which allocator implementation backs the bench_alloc / bench_free
# / bench_realloc wrappers and which workloads they can honestly serve.
#
#   bench-glibc              -- default malloc resolution, no extra link
#   bench-jemalloc           -- -ljemalloc; libjemalloc interposes malloc
#   bench-mimalloc           -- -lmimalloc; libmimalloc interposes malloc
#   bench-tcmalloc           -- -ltcmalloc; libtcmalloc interposes malloc
#   bench-misra-heap         -- HeapAllocator: general-purpose binned heap.
#   bench-misra-slab         -- SlabAllocator: fixed slot in [16, 4096].
#                               n/a on mixed-size / super-page / realloc rows.
#   bench-misra-arena        -- ArenaAllocator: bump-pointer with bulk
#                               O(1) reset. n/a on many-live-alloc rows.
#   bench-misra-page         -- PageAllocator: one mmap per alloc, page-
#                               rounded. n/a on sub-page rows.
#   bench-misra-budget       -- BudgetAllocator: caller-buffer fixed-budget
#                               pool, no growth. n/a on mixed-size / realloc.
#
# DebugAllocator is deliberately NOT benchmarked. It's a diagnostic
# wrapper (leak/canary/trace tracking) meant for tests and fuzz, not
# a production allocator; benchmarking it against general-purpose
# allocators measures the cost of those diagnostics, not allocator
# design. Tests cover its correctness; perf isn't its goal.
#
# Each backend is one C source file plus the shared C++ bench harness.
# The libc-shape backends share Allocator_libc.c; each misra backend
# has its own Allocator_misra_<name>.c. The decision about which
# allocator is "right" for a given workload is intentionally NOT made
# here -- columns show what each allocator does on each row, n/a where
# the contract doesn't fit, and the reader decides.
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
# One binary per allocator type the library exposes. Backends report
# n/a (via bench_alloc returning NULL or the bench skipping) for rows
# their allocator's contract can't serve honestly.
bench_misra_backends = [
    {'name': 'misra-heap',   'src': 'Source/Allocator_misra_heap.c'},
    {'name': 'misra-slab',   'src': 'Source/Allocator_misra_slab.c'},
    {'name': 'misra-arena',  'src': 'Source/Allocator_misra_arena.c'},
    {'name': 'misra-page',   'src': 'Source/Allocator_misra_page.c'},
    {'name': 'misra-budget', 'src': 'Source/Allocator_misra_budget.c'},
]

foreach b : bench_misra_backends
    executable(
        'bench-' + b['name'],
        files(b['src']) + bench_cpp_sources,
        c_args: ['-DBENCH_BACKEND_NAME="' + b['name'] + '"'],
        include_directories: bench_inc,
        dependencies: [bench_gbench_dep, misra_std_dep],
        install: false,
    )
endforeach
