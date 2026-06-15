# Benchmark suite. One freestanding executable per module file in
# Source/ (Bench.c is the shared harness, not a module). Built only
# under -Dbenchmark=true. After building, run Scripts/run.py to refresh
# README.md.
#
# Each executable links the real (release) library and the freestanding
# entry point / link args, exactly like the Bin/ tools -- benchmarks are
# libc-free.

bench_modules = ['Vec']

bench_harness = files('Source/Bench.c')

foreach module : bench_modules
    executable(
        'bench_' + module.to_lower(),
        files('Source' / module + '.c') + bench_harness + freestanding_extra_sources,
        link_with: [misra_std],
        c_args: common_c_args,
        include_directories: inc_misra,
        install: false,
        link_args: freestanding_link_args,
    )
endforeach
