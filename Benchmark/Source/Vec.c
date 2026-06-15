/// file      : Benchmark/Source/Vec.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Benchmarks for Vec. See Bench.h for the measurement model.

#include <Misra/Std/Container/Vec.h>

#include "Bench.h"

// Sequential read: sum of VecAt over the whole vector. The timed block
// holds only the VecAt loads; BenchUse keeps each load live without
// emitting an instruction (no accumulator in the hot path).
static u64 bench_vec_at_sequential(BenchCtx *ctx) {
    Vec(u64) v = VecInit(ctx->alloc);
    for (u64 i = 0; i < ctx->n; i++)
        VecPushBackR(&v, i);

    u64 t0 = ClockMonoNs();
    {
        for (u64 i = 0; i < ctx->n; i++)
            BenchUse(VecAt(&v, i));
    }
    u64 ns = ClockMonoNs() - t0;

    VecDeinit(&v);
    return ns;
}

static const BenchCase vec_cases[] = {
    {.name = "at_sequential", .fn = bench_vec_at_sequential},
};

BENCH_MODULE("Vec", vec_cases)
