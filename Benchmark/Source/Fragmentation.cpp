// Fragmentation benchmark.
//
// Google Benchmark itself doesn't measure space -- only time. So this
// file uses GBench as a harness but reports three custom counters:
//
//   live_MB       : sum of currently-outstanding allocation sizes,
//                   tracked by us (this file owns the {ptr, size} list)
//   footprint_MB  : allocator-reported bytes pulled from the OS
//                   (`base.footprint_bytes` on the misra backends,
//                   mallinfo2 / mallctl on the libc-shape backends --
//                   each backend's own introspection API, no
//                   process-noise from gbench or libstdc++).
//   frag_ratio    : (footprint - live) / footprint, clamped to [0, 1].
//                   Higher = more memory the allocator is holding past
//                   what callers actually need.
//
// The workload that exposes fragmentation:
//   1. Allocate N small + M medium + K large blocks
//   2. Free every OTHER small block (classic checkerboard pattern)
//   3. Try to allocate big blocks again
//   4. Measure live vs footprint
//
// This pattern is hostile to size-class allocators with no
// cross-class coalescing -- size class S has many half-empty pages
// that XL can't reuse. MisraStdC Heap, jemalloc, mimalloc, tcmalloc
// all have different strategies here.

#include "Allocator.h"

#include <benchmark/benchmark.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <random>
#include <vector>

namespace {

struct Alloc {
    void  *ptr;
    size_t size;
};

// Write a byte pattern across the whole allocation. Two reasons:
//   1. Faulting in every page exposes the "alloc + first touch"
//      cost the bench is supposed to measure -- otherwise a lazy
//      mmap allocator looks artificially fast vs an arena-served
//      one because the page-fault penalty hides on the next read.
//   2. A side effect: glibc/jemalloc/mimalloc/tcmalloc serve out
//      of pre-grown arenas, while MisraStdC's PageAllocator-backed
//      bins map fresh pages on demand; touching keeps the workload
//      symmetric across backends instead of accidentally rewarding
//      lazy mmap.
inline void touch(void *p, size_t n) {
    if (p)
        std::memset(p, 0xA5, n);
}

uint64_t sum_live(const std::vector<Alloc> &v) {
    uint64_t t = 0;
    for (const auto &a : v)
        if (a.ptr)
            t += a.size;
    return t;
}

void report_counters(benchmark::State &state, uint64_t live) {
    // For the misra backends bench_footprint_bytes() is a single
    // direct field read (`base.footprint_bytes`); for the libc-shape
    // backends it's a `mallinfo2` / `mallctl` call. None of these
    // dominate iteration timing on their own, but pause GBench's
    // clock anyway so the reported time only reflects the
    // alloc/free workload, not the introspection calls.
    state.PauseTiming();
    const uint64_t footprint    = bench_footprint_bytes();
    const uint64_t backend_live = bench_live_bytes();
    state.ResumeTiming();

    const double live_MB      = double(live) / (1024.0 * 1024.0);
    const double footprint_MB = double(footprint) / (1024.0 * 1024.0);
    double       frag         = 0.0;
    if (footprint > live)
        frag = double(footprint - live) / double(footprint);

    state.counters["live_MB"]      = benchmark::Counter(live_MB, benchmark::Counter::kAvgThreads);
    state.counters["footprint_MB"] = benchmark::Counter(footprint_MB, benchmark::Counter::kAvgThreads);
    state.counters["frag_ratio"]   = benchmark::Counter(frag, benchmark::Counter::kAvgThreads);
    if (backend_live) {
        // Allocator-reported live bytes. Should match our external
        // sum_live() modulo internal rounding; divergence flags a
        // bookkeeping mismatch worth investigating.
        const double bl_MB                = double(backend_live) / (1024.0 * 1024.0);
        state.counters["backend_live_MB"] = benchmark::Counter(bl_MB, benchmark::Counter::kAvgThreads);
    }
}

} // namespace

// ---------------------------------------------------------------------------
// Checkerboard pattern: alloc many same-size blocks, free every other,
// then try to allocate one large block per freed pair. The large block
// won't fit in any single freed slot -- it has to come from new pages
// unless the allocator coalesces.
// ---------------------------------------------------------------------------
static void BM_Frag_Checkerboard(benchmark::State &state) {
    const size_t small_sz = 64;
    const size_t big_sz   = 256; // size of would-be coalesced pair
    const size_t n_small  = static_cast<size_t>(state.range(0));

    for (auto _ : state) {
        std::vector<Alloc> small_v(n_small);
        for (size_t i = 0; i < n_small; i++) {
            small_v[i] = {bench_alloc(small_sz), small_sz};
            touch(small_v[i].ptr, small_sz);
        }

        // Free even indices. Odd ones stay live, creating the checkerboard
        // of holes in the size-S bin.
        for (size_t i = 0; i < n_small; i += 2) {
            bench_free(small_v[i].ptr);
            small_v[i].ptr = nullptr;
        }

        // Now try to fill the gaps with allocations that DON'T match
        // the small-S slot size. A non-coalescing binned allocator
        // can't use the holes; a coalescing one can.
        std::vector<Alloc> big_v(n_small / 2);
        for (size_t i = 0; i < big_v.size(); i++) {
            big_v[i] = {bench_alloc(big_sz), big_sz};
            touch(big_v[i].ptr, big_sz);
        }

        const uint64_t live = sum_live(small_v) + sum_live(big_v);
        report_counters(state, live);

        // Drain.
        for (auto &a : small_v)
            if (a.ptr)
                bench_free(a.ptr);
        for (auto &a : big_v)
            if (a.ptr)
                bench_free(a.ptr);
    }
}
BENCHMARK(BM_Frag_Checkerboard)->Arg(4096)->Arg(16384)->Arg(65536)->Unit(benchmark::kMillisecond);

// ---------------------------------------------------------------------------
// Lifetime mix: simulate a long-running process with allocations of
// varied lifetimes. Each round, a random subset of live allocations
// is freed and replaced. Repeat until heap state stabilises.
// ---------------------------------------------------------------------------
static void BM_Frag_LifetimeMix(benchmark::State &state) {
    const size_t                          n_blocks = 16384;
    const size_t                          rounds   = 16;
    std::mt19937                          rng(0xC0FFEE);
    std::uniform_int_distribution<int>    coin(0, 1);
    std::uniform_int_distribution<size_t> size_pick(0, 5);
    // Sizes chosen to span MisraStdC Heap's three small-class bins
    // (16/32/64), medium-class bins (128/256/512), and one L bin
    // (1024). Hits jemalloc/mimalloc/tcmalloc bins similarly.
    const size_t sizes[] = {16, 32, 64, 128, 256, 1024};

    for (auto _ : state) {
        std::vector<Alloc> live(n_blocks);
        for (size_t i = 0; i < n_blocks; i++) {
            size_t s = sizes[size_pick(rng)];
            live[i]  = {bench_alloc(s), s};
            touch(live[i].ptr, s);
        }
        for (size_t r = 0; r < rounds; r++) {
            for (size_t i = 0; i < n_blocks; i++) {
                if (live[i].ptr && coin(rng)) {
                    bench_free(live[i].ptr);
                    size_t s = sizes[size_pick(rng)];
                    live[i]  = {bench_alloc(s), s};
                    touch(live[i].ptr, s);
                }
            }
        }
        report_counters(state, sum_live(live));
        for (auto &a : live)
            if (a.ptr)
                bench_free(a.ptr);
    }
}
BENCHMARK(BM_Frag_LifetimeMix)->Unit(benchmark::kMillisecond);

// ---------------------------------------------------------------------------
// Page-overhang: allocations whose size just-exceeds a size-class
// boundary. Forces the allocator into the next bin up, wasting the
// remainder per allocation.
// ---------------------------------------------------------------------------
static void BM_Frag_PageOverhang(benchmark::State &state) {
    // 17 just-overhangs MisraStdC's 16 B bin into the 32 B bin (50%
    // overhead per alloc). 65 overhangs the 64 B bin into 128 B (49%
    // overhead). jemalloc's bins are similar (16/32/48/64/80/96/112/128
    // -- the overhang is smaller per step but still present).
    const size_t n           = 65536;
    const size_t overhangs[] = {17, 33, 65, 129, 257, 513, 1025};

    for (auto _ : state) {
        std::vector<Alloc> v(n);
        for (size_t i = 0; i < n; i++) {
            size_t s = overhangs[i % (sizeof(overhangs) / sizeof(overhangs[0]))];
            v[i]     = {bench_alloc(s), s};
            touch(v[i].ptr, s);
        }
        report_counters(state, sum_live(v));
        for (auto &a : v)
            if (a.ptr)
                bench_free(a.ptr);
    }
}
BENCHMARK(BM_Frag_PageOverhang)->Unit(benchmark::kMillisecond);
