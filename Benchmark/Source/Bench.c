/// file      : Benchmark/Source/Bench.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Benchmark driver. For each (case, size) it builds the fixture once,
/// then accumulates one-pass `dt` samples until either the time budget
/// is spent or the iteration cap is hit, rebuilding the fixture whenever
/// a pass reports it `dirty`. Reports integer summary statistics over the
/// collected samples. See Bench.h for the measurement model.
///
/// The driver uses MisraStdC primitives: samples live in a `Vec(u64)`
/// ordered with `VecSort`. The iteration cap bounds the sample count so
/// the buffer stays small even for fast ops.

#include <Misra/Config.h>
#include <Misra/Std/Container/Vec.h>
#include <Misra/Std/Io.h>

#include "Bench.h"

// Stop a measurement once EITHER limit is reached:
//   - accumulated timed `dt` reaches the budget, or
//   - the sample count reaches the cap (so a fast op doesn't spin for
//     the full budget, and the sample buffer stays bounded).
#define BENCH_BUDGET_NS 30000000000ull // 30 s of accumulated dt
#define BENCH_ITER_CAP  100000u

volatile u64 g_bench_sink;

static const char *bench_arch_str(void) {
#if ARCHITECTURE_X86_64
    return "x86_64";
#elif ARCHITECTURE_AARCH64
    return "aarch64";
#else
    return "unknown";
#endif
}

// Ascending GenericCompare over u64 samples, for VecSort.
static i32 bench_u64_cmp(const void *first, const void *second) {
    u64 a = *(const u64 *)first;
    u64 b = *(const u64 *)second;
    return a < b ? -1 : (a > b ? 1 : 0);
}

// floor(sqrt(v)) over u64, by Newton's method.
//
// TODO: remove this once Int auto-dispatches a fixed-width fast path for
// values that fit a machine word (falling back to the bignum only on
// overflow). Today the library's only sqrt is the arbitrary-precision
// IntSqrt -- Int is a BitVec-backed bignum, so rooting a 64-bit variance
// through it would allocate a bignum per result row. Until Int does that
// dispatch, we write the narrow sqrt the library lacks.
static u64 bench_u64_sqrt(u64 v) {
    if (v == 0)
        return 0;
    u64 x = v;
    u64 y = (x + 1) / 2;
    while (y < x) {
        x = y;
        y = (x + v / x) / 2;
    }
    return x;
}

int bench_main(const char *module, const BenchCase *cases, u32 ncases, int argc, char **argv) {
    (void)argc;
    (void)argv;

    static const u64 default_sizes[] = {64, 1024, 16384, 262144, 0};

    DefaultAllocator heap = DefaultAllocatorInit();
    Vec(u64) samples      = VecInit(&heap);
    VecReserve(&samples, BENCH_ITER_CAP); // bounded once; reused across rows

    WriteFmtLn("MISRABENCH\tv1\tmodule={}\tarch={}", module, bench_arch_str());

    for (u32 c = 0; c < ncases; c++) {
        const BenchCase *bc    = &cases[c];
        const u64       *sizes = bc->sizes ? bc->sizes : default_sizes;

        for (u32 si = 0; sizes[si] != 0; si++) {
            BenchCtx ctx = {.n = sizes[si], .alloc = &heap, .rng = {BENCH_RNG_SEED}, .fx = NULL, .dirty = false};

            bc->setup(&ctx);

            VecClear(&samples);
            u64 acc = 0;
            while (acc < BENCH_BUDGET_NS && VecLen(&samples) < BENCH_ITER_CAP) {
                ctx.dirty = false;
                u64 dt    = bc->run(&ctx);
                VecPushBackR(&samples, dt);
                acc += dt;
                if (ctx.dirty) {
                    bc->teardown(&ctx);
                    bc->setup(&ctx);
                }
            }

            bc->teardown(&ctx);

            u64 passes = VecLen(&samples);

            u64 sum                      = 0;
            VecForeach(&samples, s) sum += s;
            u64 mean                     = sum / passes;

            u64 var_acc = 0;
            VecForeach(&samples, s) {
                u64 d    = s > mean ? s - mean : mean - s;
                var_acc += d * d;
            }
            u64 stddev = bench_u64_sqrt(var_acc / passes);

            VecSort(&samples, bench_u64_cmp);
            u64 mn  = VecAt(&samples, 0);
            u64 med = VecAt(&samples, passes / 2);
            u64 mx  = VecAt(&samples, passes - 1);

            WriteFmtLn(
                "BENCH\tmodule={}\tcase={}\tn={}\tpasses={}\tmin_ns={}\tmed_ns={}\tmean_ns={}\tmax_ns={}\tstddev_ns={}",
                module,
                bc->name,
                ctx.n,
                passes,
                mn,
                med,
                mean,
                mx,
                stddev
            );
        }
    }

    VecDeinit(&samples);
    DefaultAllocatorDeinit(&heap);
    return 0;
}
