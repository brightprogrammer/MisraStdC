/// file      : Benchmark/Source/Bench.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Benchmark driver: runs cases, computes integer summary statistics,
/// and prints machine-readable result lines. See Bench.h for the model.

#include <Misra/Config.h>
#include <Misra/Std/Io.h>

#include "Bench.h"

// Defaults, applied when a BenchCase leaves the field zero.
#define BENCH_WARMUP_DEFAULT  2
#define BENCH_SAMPLES_DEFAULT 15
#define BENCH_SAMPLES_MAX     256

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

// Integer square root (floor). Used for the std-deviation without
// pulling in libm / float math.
static u64 bench_isqrt(u64 v) {
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

// In-place ascending insertion sort. `n` is small (<= BENCH_SAMPLES_MAX).
static void bench_sort(u64 *a, u32 n) {
    for (u32 i = 1; i < n; i++) {
        u64 k = a[i];
        i32 j = (i32)i - 1;
        while (j >= 0 && a[j] > k) {
            a[j + 1] = a[j];
            j--;
        }
        a[j + 1] = k;
    }
}

int bench_main(const char *module, const BenchCase *cases, u32 ncases, int argc, char **argv) {
    (void)argc;
    (void)argv;

    static const u64 default_sizes[] = {64, 1024, 16384, 262144, 0};

    DefaultAllocator heap = DefaultAllocatorInit();

    WriteFmtLn("MISRABENCH\tv1\tmodule={}\tarch={}", module, bench_arch_str());

    for (u32 c = 0; c < ncases; c++) {
        const BenchCase *bc      = &cases[c];
        const u64       *sizes   = bc->sizes ? bc->sizes : default_sizes;
        u32              warmup  = bc->warmup ? bc->warmup : BENCH_WARMUP_DEFAULT;
        u32              samples = bc->samples ? bc->samples : BENCH_SAMPLES_DEFAULT;
        if (samples > BENCH_SAMPLES_MAX)
            samples = BENCH_SAMPLES_MAX;

        for (u32 si = 0; sizes[si] != 0; si++) {
            u64 n = sizes[si];

            for (u32 w = 0; w < warmup; w++) {
                BenchCtx ctx = {.n = n, .alloc = &heap, .rng = {BENCH_RNG_SEED}};
                (void)bc->fn(&ctx);
            }

            u64 smp[BENCH_SAMPLES_MAX];
            for (u32 i = 0; i < samples; i++) {
                BenchCtx ctx = {.n = n, .alloc = &heap, .rng = {BENCH_RNG_SEED}};
                smp[i]       = bc->fn(&ctx);
            }

            // sum / mean before sorting; min / max / median after.
            u64 sum = 0;
            for (u32 i = 0; i < samples; i++)
                sum += smp[i];
            u64 mean = sum / samples;

            u64 var_acc = 0;
            for (u32 i = 0; i < samples; i++) {
                u64 d    = smp[i] > mean ? smp[i] - mean : mean - smp[i];
                var_acc += d * d;
            }
            u64 stddev = bench_isqrt(var_acc / samples);

            bench_sort(smp, samples);
            u64 mn  = smp[0];
            u64 mx  = smp[samples - 1];
            u64 med = smp[samples / 2];

            u64         ops  = bc->ops ? bc->ops(n) : n;
            const char *flag = med < BENCH_MIN_RELIABLE_NS ? "low" : "ok";

            WriteFmtLn(
                "BENCH\tmodule={}\tcase={}\tn={}\tops={}\tsamples={}\tmin_ns={}\tmed_ns={}\tmean_ns={}\tmax_ns={}"
                "\tstddev_ns={}\tflag={}",
                module,
                bc->name,
                n,
                ops,
                samples,
                mn,
                med,
                mean,
                mx,
                stddev,
                flag
            );
        }
    }

    DefaultAllocatorDeinit(&heap);
    return 0;
}
