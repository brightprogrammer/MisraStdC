// Fragmentation benchmark.
//
// Google Benchmark itself doesn't measure space -- only time. So this
// file uses GBench as a harness but reports three custom counters,
// every one of which comes from the backend's OWN introspection API.
// The harness never counts bytes on its own behalf -- if a backend
// can't report a number, the row reads n/a rather than getting a
// faked-up "we tracked it ourselves" value:
//
//   live_MB       : currently-outstanding allocation bytes, as the
//                   backend itself reports them (mallinfo2 / mallctl /
//                   MallocExtension on the libc-shape backends;
//                   `AllocatorBytesInUse(a)` -- gated on
//                   FEATURE_ALLOC_STATS -- on the misra backends). n/a
//                   when the backend has no working stats API (mimalloc
//                   3.3.0's `mi_stats_get` is broken; FEATURE_ALLOC_STATS
//                   compiled-out on the misra backends).
//   footprint_MB  : allocator-reported bytes pulled from the OS
//                   (`base.footprint_bytes` on misra -- ALWAYS available,
//                   not gated; mallinfo2 / mallctl on the libc-shape).
//   frag_ratio    : (footprint - live) / footprint, clamped to [0, 1].
//                   Higher = more memory the allocator is holding past
//                   what callers actually need. Emitted only when both
//                   numbers are available; n/a otherwise.
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

void report_counters(benchmark::State &state) {
    // For the misra backends bench_footprint_bytes() / bench_live_bytes()
    // are direct field reads (`base.footprint_bytes`,
    // `AllocatorBytesInUse`); for the libc-shape backends each is a
    // `mallinfo2` / `mallctl` / `MallocExtension` call. None of these
    // dominate iteration timing on their own, but pause GBench's clock
    // anyway so the reported time only reflects the alloc/free
    // workload, not the introspection calls.
    state.PauseTiming();
    const uint64_t footprint = bench_footprint_bytes();
    const uint64_t live      = bench_live_bytes();
    state.ResumeTiming();

    // Counters emitted only when the backend actually reported the
    // number. Missing counters surface as n/a in the rendered table
    // (run.py treats 0.0 / absent counters as n/a). No harness-side
    // bookkeeping covers for a missing API -- if jemalloc/glibc/etc.
    // chooses not to expose a number, that's what the column shows.
    if (live > 0) {
        const double live_MB      = double(live) / (1024.0 * 1024.0);
        state.counters["live_MB"] = benchmark::Counter(live_MB, benchmark::Counter::kAvgThreads);
    }
    if (footprint > 0) {
        const double footprint_MB      = double(footprint) / (1024.0 * 1024.0);
        state.counters["footprint_MB"] = benchmark::Counter(footprint_MB, benchmark::Counter::kAvgThreads);
    }
    // frag_ratio is derived; only meaningful when both inputs exist
    // and footprint >= live (footprint < live is a backend-stats bug,
    // not a fragmentation observation).
    if (live > 0 && footprint > live) {
        const double frag            = double(footprint - live) / double(footprint);
        state.counters["frag_ratio"] = benchmark::Counter(frag, benchmark::Counter::kAvgThreads);
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
        std::vector<void *> small_v(n_small);
        for (size_t i = 0; i < n_small; i++) {
            small_v[i] = bench_alloc(small_sz);
            touch(small_v[i], small_sz);
        }

        // Free even indices. Odd ones stay live, creating the checkerboard
        // of holes in the size-S bin.
        for (size_t i = 0; i < n_small; i += 2) {
            bench_free(small_v[i]);
            small_v[i] = nullptr;
        }

        // Now try to fill the gaps with allocations that DON'T match
        // the small-S slot size. A non-coalescing binned allocator
        // can't use the holes; a coalescing one can.
        std::vector<void *> big_v(n_small / 2);
        for (size_t i = 0; i < big_v.size(); i++) {
            big_v[i] = bench_alloc(big_sz);
            touch(big_v[i], big_sz);
        }

        report_counters(state);

        // Drain.
        for (void *p : small_v)
            if (p)
                bench_free(p);
        for (void *p : big_v)
            if (p)
                bench_free(p);
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
    // Sizes chosen to span six of MisraStdC Heap's eight power-of-two
    // bin classes (16/32/64/128/256/1024), skipping 512 and 2048 to
    // keep the working set compact. Hits jemalloc/mimalloc/tcmalloc
    // bins similarly.
    const size_t sizes[] = {16, 32, 64, 128, 256, 1024};

    for (auto _ : state) {
        std::vector<void *> live(n_blocks);
        for (size_t i = 0; i < n_blocks; i++) {
            size_t s = sizes[size_pick(rng)];
            live[i]  = bench_alloc(s);
            touch(live[i], s);
        }
        for (size_t r = 0; r < rounds; r++) {
            for (size_t i = 0; i < n_blocks; i++) {
                if (live[i] && coin(rng)) {
                    bench_free(live[i]);
                    size_t s = sizes[size_pick(rng)];
                    live[i]  = bench_alloc(s);
                    touch(live[i], s);
                }
            }
        }
        report_counters(state);
        for (void *p : live)
            if (p)
                bench_free(p);
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
        std::vector<void *> v(n);
        for (size_t i = 0; i < n; i++) {
            size_t s = overhangs[i % (sizeof(overhangs) / sizeof(overhangs[0]))];
            v[i]     = bench_alloc(s);
            touch(v[i], s);
        }
        report_counters(state);
        for (void *p : v)
            if (p)
                bench_free(p);
    }
}
BENCHMARK(BM_Frag_PageOverhang)->Unit(benchmark::kMillisecond);
