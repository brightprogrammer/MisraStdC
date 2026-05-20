// Microbenchmarks: pure timing, no fragmentation accounting.
//
// Categories:
//   1. alloc/free pair at fixed sizes  (16 .. 64K)
//   2. batch alloc-N then free-N       (warm bin / free list effects)
//   3. realloc growth                   (8 -> 1M, geometric)
//   4. mixed pareto sizes               (real-world-ish distribution)
//   5. alloc + write every byte         (alloc cost vs zero-touch cost)
//
// Each runs once per binary; the binary itself picks the backend at
// link time. The runner script (Scripts/run.sh) executes one binary per
// allocator and concatenates the JSON output.

#include "Allocator.h"

#include <benchmark/benchmark.h>

#include <cstdint>
#include <cstring>
#include <random>
#include <vector>

// ---------------------------------------------------------------------------
// 1. alloc/free pair at fixed sizes
// ---------------------------------------------------------------------------
static void BM_AllocFreePair(benchmark::State &state) {
    const size_t sz = static_cast<size_t>(state.range(0));
    // Fixed-size workload -- backends with a slab/fixed-size allocator
    // swap to it here. libc backends + the "wrong tool" misra backend
    // ignore the hint. See Allocator.h: bench_use_fixed_size.
    bench_use_fixed_size(sz);
    for (auto _ : state) {
        void *p = bench_alloc(sz);
        benchmark::DoNotOptimize(p);
        bench_free(p);
    }
    bench_use_general();
    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(sz));
    state.SetItemsProcessed(int64_t(state.iterations()));
}
BENCHMARK(BM_AllocFreePair)
    ->Arg(16)->Arg(64)->Arg(256)->Arg(1024)->Arg(4096)->Arg(16384)->Arg(65536);

// ---------------------------------------------------------------------------
// 2. batch alloc-N then free-N
// ---------------------------------------------------------------------------
// Holding pointers in a vector lets the allocator's free list grow,
// then drains it all at once. Exposes free-list / bin reuse cost
// distinct from the single-pair churn above.
static void BM_BatchAllocFree(benchmark::State &state) {
    const size_t sz = 64;
    const size_t n  = static_cast<size_t>(state.range(0));
    std::vector<void *> ptrs(n);
    bench_use_fixed_size(sz);
    for (auto _ : state) {
        for (size_t i = 0; i < n; i++) ptrs[i] = bench_alloc(sz);
        // Free in reverse order: last-allocated, first-freed. Lets
        // bump-style allocators (ArenaAllocator) rewind their cursor
        // on each free instead of leaking. Other allocators are
        // indifferent to free order at this granularity.
        for (size_t i = n; i-- > 0; ) bench_free(ptrs[i]);
    }
    bench_use_general();
    state.SetItemsProcessed(int64_t(state.iterations()) * int64_t(n));
}
BENCHMARK(BM_BatchAllocFree)->Arg(128)->Arg(1024)->Arg(8192);

// ---------------------------------------------------------------------------
// 3. realloc growth (geometric)
// ---------------------------------------------------------------------------
// Mimics Vec's amortised growth pattern: realloc with 2x each step
// from 8 B up to 1 MiB, then free. Some allocators (MisraStdC Heap,
// jemalloc) cross size-class boundaries and have to copy; others
// (mimalloc) handle this with in-place extension more often.
static void BM_ReallocGrow(benchmark::State &state) {
    for (auto _ : state) {
        void  *p   = nullptr;
        size_t cur = 0;
        for (size_t sz = 8; sz <= 1u << 20; sz <<= 1) {
            p = bench_realloc(p, sz);
            benchmark::DoNotOptimize(p);
            cur = sz;
        }
        bench_free(p);
        benchmark::DoNotOptimize(cur);
    }
}
BENCHMARK(BM_ReallocGrow);

// ---------------------------------------------------------------------------
// 4. mixed pareto-distributed sizes
// ---------------------------------------------------------------------------
// Realistic-ish workload: most allocations are small, a long tail of
// large ones. Stresses how the allocator handles size-class mixing.
static void BM_MixedPareto(benchmark::State &state) {
    std::mt19937                            rng(0x5eed);
    std::uniform_real_distribution<double>  u(1e-9, 1.0);
    // Pareto inverse CDF: x = xm / U^(1/alpha). alpha=1.16 is the
    // canonical "80/20" shape (Vilfredo's number). xm=24 B keeps the
    // bulk of draws in the binned-small range for all three modern
    // allocators (jemalloc / mimalloc / MisraStdC Heap).
    const double xm    = 24.0;
    const double alpha = 1.16;
    const size_t batch = 512;
    std::vector<void *>  ptrs(batch);
    std::vector<size_t>  szs(batch);
    for (auto _ : state) {
        for (size_t i = 0; i < batch; i++) {
            double s = xm / std::pow(u(rng), 1.0 / alpha);
            if (s > 256 * 1024) s = 256 * 1024;
            szs[i]  = static_cast<size_t>(s);
            ptrs[i] = bench_alloc(szs[i]);
        }
        for (size_t i = 0; i < batch; i++) bench_free(ptrs[i]);
    }
    state.SetItemsProcessed(int64_t(state.iterations()) * int64_t(batch));
}
BENCHMARK(BM_MixedPareto);

// ---------------------------------------------------------------------------
// 5. arena bump + bulk reset
// ---------------------------------------------------------------------------
// Allocate N small objects, then either reset the backing arena (bulk
// O(1) free) or fall back to freeing each pointer individually. Same
// workload on every backend; arena-shaped backends short-circuit to
// reset. Shows the cost differential of "1 reset" vs "N individual
// frees" for the same bump-N workload.
static void BM_ArenaBumpReset(benchmark::State &state) {
    const size_t n  = static_cast<size_t>(state.range(0));
    const size_t sz = 32;
    std::vector<void *> ptrs(n);
    for (auto _ : state) {
        for (size_t i = 0; i < n; i++) ptrs[i] = bench_alloc(sz);
        if (bench_can_reset()) {
            bench_reset();
        } else {
            for (size_t i = 0; i < n; i++) bench_free(ptrs[i]);
        }
    }
    state.SetItemsProcessed(int64_t(state.iterations()) * int64_t(n));
}
BENCHMARK(BM_ArenaBumpReset)->Arg(128)->Arg(1024)->Arg(8192);

// ---------------------------------------------------------------------------
// 6. alloc + write every byte
// ---------------------------------------------------------------------------
// Surfaces the first-page-fault cost an allocator pays on lazy-mmap
// backends (every backend here, including MisraStdC's PageAllocator).
// Comparing against BM_AllocFreePair at the same size separates "alloc
// dispatch" from "alloc + first touch".
static void BM_AllocTouchFree(benchmark::State &state) {
    const size_t sz = static_cast<size_t>(state.range(0));
    bench_use_fixed_size(sz);
    for (auto _ : state) {
        void *p = bench_alloc(sz);
        std::memset(p, 0xA5, sz);
        benchmark::DoNotOptimize(p);
        bench_free(p);
    }
    bench_use_general();
    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(sz));
}
BENCHMARK(BM_AllocTouchFree)->Arg(64)->Arg(4096)->Arg(65536);
