/// file      : Benchmark/Source/Vec.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Benchmarks for Vec. See Bench.h for the measurement model.

#include <Misra/Std/Container/Vec.h>

#include "Bench.h"

typedef Vec(u64) BenchU64Vec;

// ---------------------------------------------------------------------------
// Shared fixture for element-access benchmarks: a value vector holding
// [0, n) and a vector of n random indices into it. Built once per size and
// reused across every pass (reads never dirty it). `at_sequential` ignores
// `idx`; `at_random` walks through it. Capacity is reserved up front since
// the size is known.
// ---------------------------------------------------------------------------
typedef struct {
    BenchU64Vec vec;
    BenchU64Vec idx;
} u64_index_fixture;

static void u64_index_setup(BenchCtx *ctx) {
    u64_index_fixture *f = AllocatorAlloc(ctx->alloc, sizeof(*f), 1);
    f->vec               = (BenchU64Vec)VecInit(ctx->alloc);
    f->idx               = (BenchU64Vec)VecInit(ctx->alloc);
    VecReserve(&f->vec, ctx->n);
    VecReserve(&f->idx, ctx->n);
    for (u64 i = 0; i < ctx->n; i++)
        VecPushBackR(&f->vec, i);
    for (u64 i = 0; i < ctx->n; i++)
        VecPushBackR(&f->idx, bench_rng_below(&ctx->rng, ctx->n));
    ctx->fx = f;
}

static void u64_index_teardown(BenchCtx *ctx) {
    u64_index_fixture *f = ctx->fx;
    VecDeinit(&f->vec);
    VecDeinit(&f->idx);
    AllocatorFree(ctx->alloc, f);
    ctx->fx = NULL;
}

// at_sequential: read every element in index order. The timed block holds
// only the VecAt loads; BenchUse keeps each live without emitting an
// instruction.
static u64 at_sequential_run(BenchCtx *ctx) {
    u64_index_fixture *f  = ctx->fx;
    u64                t0 = ClockMonoNs();
    {
        for (u64 i = 0; i < ctx->n; i++)
            BenchUse(VecAt(&f->vec, i));
    }
    return ClockMonoNs() - t0;
}

// at_random: read elements in a random order, defeating the prefetcher --
// the value load is the cache-miss-inducing one under test. The index
// source is grabbed as a raw pointer BEFORE the timed region (the indices
// themselves were generated once in setup), so the timed loop holds exactly
// one VecAt -- the value access -- not a second VecAt for the index. `idx[i]`
// is a plain sequential load, the minimal, unavoidable index source.
static u64 at_random_run(BenchCtx *ctx) {
    u64_index_fixture *f   = ctx->fx;
    const u64         *idx = (const u64 *)VecBegin(&f->idx);
    u64                t0  = ClockMonoNs();
    {
        for (u64 i = 0; i < ctx->n; i++)
            BenchUse(VecAt(&f->vec, idx[i]));
    }
    return ClockMonoNs() - t0;
}

static const BenchCase vec_cases[] = {
    BENCH_CASE_FX(at_sequential, u64_index_setup, u64_index_teardown),
    BENCH_CASE_FX(at_random, u64_index_setup, u64_index_teardown),
};

BENCH_MODULE("Vec", vec_cases)
