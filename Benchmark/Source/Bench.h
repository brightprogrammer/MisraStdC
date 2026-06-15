/// file      : Benchmark/Source/Bench.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Benchmark harness for MisraStdC. One module file (e.g. Vec.c) lists
/// its benchmark cases and ends with `BENCH_MODULE(...)`, which emits a
/// `main` that runs every case across a set of workload sizes and prints
/// machine-readable result lines. The harness is libc-free, like the
/// rest of the library; it links freestanding exactly as the Bin/ tools
/// do.
///
/// MEASUREMENT MODEL. A benchmark is a single function that returns the
/// elapsed nanoseconds of the operation under test. It owns its own
/// timing: it brackets ONLY the operation with two `ClockMonoNs()` reads
/// and a `{ }` block, and does all fixture construction / input
/// generation / result consumption OUTSIDE that block (untimed). The
/// driver re-initialises the context (RNG included) before every call,
/// so each sample starts from a byte-identical state -- destructive ops
/// are safe because each call rebuilds its own fixture.
///
/// DEFEATING DEAD-CODE ELIMINATION. The timed block must contain nothing
/// but the target API call(s) and the minimal loop to repeat them. Never
/// add an accumulator, a store, or any arithmetic to "keep the code
/// alive" -- use `BenchUse`, which forces a value to be materialised but
/// emits no machine instruction (verified on disassembly).

#ifndef MISRA_BENCH_H
#define MISRA_BENCH_H

#include <Misra/Std/Allocator/Default.h>
#include <Misra/Sys/Clock.h>
#include <Misra/Types.h>

///
/// Force `x` to be computed and held in a register, defeating dead-code
/// elimination, while emitting ZERO machine instructions. Use this to
/// consume the result of a value-returning op inside a timed loop.
///
/// The `"r"` constraint (not `"r,m"`) is deliberate: `"r,m"` lets the
/// compiler hand the memory operand to the empty asm without ever
/// loading it, so the load -- the very thing under test -- is elided.
/// Copying through a local and constraining to a register forces the
/// load to happen.
///
#define BenchUse(x)                                                                                                    \
    do {                                                                                                               \
        __typeof__(x) _bench_use_v = (x);                                                                              \
        __asm__ volatile(""                                                                                            \
                         :                                                                                             \
                         : "r"(_bench_use_v)                                                                           \
                         : "memory");                                                                                  \
    } while (0)

///
/// Compiler reordering / store-elision barrier. Emits no instruction;
/// prevents the compiler from moving memory operations across this
/// point. Use when an op's observable effect is a store the compiler
/// might otherwise drop.
///
#define BenchClobber()                                                                                                 \
    __asm__ volatile(""                                                                                                \
                     :                                                                                                 \
                     :                                                                                                 \
                     : "memory")

///
/// Deterministic, fixed-seed PRNG (splitmix64) for benchmark inputs.
/// Re-seeded identically by the driver before every call, so generated
/// inputs are reproducible run to run. Distinct from `Std/Prng`, which
/// seeds from kernel entropy and is therefore non-reproducible.
///
#define BENCH_RNG_SEED 0x9E3779B97F4A7C15ull

typedef struct {
    u64 s;
} BenchRng;

static inline u64 bench_rng_next(BenchRng *r) {
    u64 z = (r->s += 0x9E3779B97F4A7C15ull);
    z     = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z     = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
}

/// Unbiased random value in `[0, bound)`. `bound` must be non-zero.
static inline u64 bench_rng_below(BenchRng *r, u64 bound) {
    return bench_rng_next(r) % bound;
}

///
/// Result sink. Benchmarks flush consumed results here, OUTSIDE the
/// timed region, so the optimizer cannot delete the work that produced
/// them. Declared `volatile` so the writes are never elided.
///
extern volatile u64 g_bench_sink;

static inline void bench_keep_u64(u64 x) {
    g_bench_sink += x;
}
static inline void bench_keep_ptr(const void *p) {
    g_bench_sink += (u64)p; // every supported target is 64-bit; cast through u64
}

///
/// Context handed to every benchmark. The driver fills it fresh before
/// each call: `n` is the workload size for the current row, `alloc` is
/// the (single, reused) allocator to build fixtures with, and `rng` is
/// re-seeded to `BENCH_RNG_SEED` every call.
///
typedef struct {
    u64               n;
    DefaultAllocator *alloc;
    BenchRng          rng;
} BenchCtx;

/// A benchmark: returns the elapsed nanoseconds of its timed region.
typedef u64 (*BenchFn)(BenchCtx *ctx);

///
/// A registered benchmark case.
///   name    : short identifier, e.g. "at_sequential".
///   fn      : the benchmark function.
///   ops     : how many target-API ops one call represents, used as the
///             ns-per-op divisor by downstream tooling. NULL means `n`.
///   sizes   : 0-terminated array of workload sizes. NULL means the
///             driver's default set.
///   samples : recorded measurements per size. 0 means the default.
///   warmup  : discarded measurements per size. 0 means the default.
///
typedef struct {
    const char *name;
    BenchFn     fn;
    u64 (*ops)(u64 n);
    const u64 *sizes;
    u32        samples;
    u32        warmup;
} BenchCase;

///
/// Run every case in `cases` and print result lines to stdout. Returns
/// 0. `argc`/`argv` are accepted for forward-compatible case filtering
/// and currently unused.
///
int bench_main(const char *module, const BenchCase *cases, u32 ncases, int argc, char **argv);

///
/// Emit a `main` for a module file. `cases` must be a statically-sized
/// array so its length is computed here.
///
#define BENCH_MODULE(modname, cases)                                                                                   \
    int main(int argc, char **argv) {                                                                                  \
        return bench_main((modname), (cases), (u32)(sizeof(cases) / sizeof((cases)[0])), argc, argv);                  \
    }

#endif // MISRA_BENCH_H
