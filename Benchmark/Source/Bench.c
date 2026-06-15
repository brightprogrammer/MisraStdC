/// file      : Benchmark/Source/Bench.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Benchmark driver: runs cases, computes integer summary statistics,
/// and prints machine-readable result lines. See Bench.h for the model.
///
/// The driver leans on MisraStdC primitives: samples live in a
/// `Vec(u64)` ordered with `VecSort`. The lone exception is the standard
/// deviation's integer square root -- the library's only sqrt is the
/// arbitrary-precision `IntSqrt`, too heavy for a `u64`, so the narrow
/// `bench_u64_sqrt` is written here (the "no fitting primitive -> write
/// one" branch of the dogfooding rule).

#include <Misra/Config.h>
#include <Misra/Std/Container/Vec.h>
#include <Misra/Std/Io.h>

#include "Bench.h"

// Defaults, applied when a BenchCase leaves the field zero.
#define BENCH_WARMUP_DEFAULT  2
#define BENCH_SAMPLES_DEFAULT 15

// Below this median, a single sample is dominated by clock overhead and
// resolution; the row is flagged so the reader treats it as indicative
// rather than precise. (Roughly two orders of magnitude over the ~20 ns
// ClockMonoNs call cost.)
#define BENCH_MIN_RELIABLE_NS 2000

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

    WriteFmtLn("MISRABENCH\tv1\tmodule={}\tarch={}", module, bench_arch_str());

    for (u32 c = 0; c < ncases; c++) {
        const BenchCase *bc       = &cases[c];
        const u64       *sizes    = bc->sizes ? bc->sizes : default_sizes;
        u32              warmup   = bc->warmup ? bc->warmup : BENCH_WARMUP_DEFAULT;
        u32              n_sample = bc->samples ? bc->samples : BENCH_SAMPLES_DEFAULT;

        VecReserve(&samples, n_sample); // fixed capacity, reused across sizes

        for (u32 si = 0; sizes[si] != 0; si++) {
            u64 n = sizes[si];

            for (u32 w = 0; w < warmup; w++) {
                BenchCtx ctx = {.n = n, .alloc = &heap, .rng = {BENCH_RNG_SEED}};
                (void)bc->fn(&ctx);
            }

            VecClear(&samples);
            for (u32 i = 0; i < n_sample; i++) {
                BenchCtx ctx = {.n = n, .alloc = &heap, .rng = {BENCH_RNG_SEED}};
                VecPushBackR(&samples, bc->fn(&ctx));
            }

            // mean and variance over the unsorted samples.
            u64 sum                      = 0;
            VecForeach(&samples, s) sum += s;
            u64 mean                     = sum / n_sample;

            u64 var_acc = 0;
            VecForeach(&samples, s) {
                u64 d    = s > mean ? s - mean : mean - s;
                var_acc += d * d;
            }
            u64 stddev = bench_u64_sqrt(var_acc / n_sample);

            // min / median / max off the sorted samples.
            VecSort(&samples, bench_u64_cmp);
            u64 mn  = VecAt(&samples, 0);
            u64 med = VecAt(&samples, n_sample / 2);
            u64 mx  = VecAt(&samples, n_sample - 1);

            u64         ops  = bc->ops ? bc->ops(n) : n;
            const char *flag = med < BENCH_MIN_RELIABLE_NS ? "low" : "ok";

            WriteFmtLn(
                "BENCH\tmodule={}\tcase={}\tn={}\tops={}\tsamples={}\tmin_ns={}\tmed_ns={}\tmean_ns={}\tmax_ns={}"
                "\tstddev_ns={}\tflag={}",
                module,
                bc->name,
                n,
                ops,
                n_sample,
                mn,
                med,
                mean,
                mx,
                stddev,
                flag
            );
        }
    }

    VecDeinit(&samples);
    DefaultAllocatorDeinit(&heap);
    return 0;
}
