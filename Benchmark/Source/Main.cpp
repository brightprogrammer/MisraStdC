// Benchmark entry point. We can't use BENCHMARK_MAIN() because we need
// bench_init()/bench_teardown() around the Run call -- the MisraStdC
// HeapAllocator has to be constructed before the first BM_* hits it and
// destructed after the last one to drop its mmap'd pages cleanly.

#include "Allocator.h"

#include <benchmark/benchmark.h>

int main(int argc, char **argv) {
    bench_init();

    // Tag the run with the backend name so the JSON output identifies
    // itself without the caller having to track which binary produced
    // which file.
    benchmark::AddCustomContext("backend", bench_backend_name());

    benchmark::Initialize(&argc, argv);
    if (benchmark::ReportUnrecognizedArguments(argc, argv)) {
        bench_teardown();
        return 1;
    }
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();

    bench_teardown();
    return 0;
}
