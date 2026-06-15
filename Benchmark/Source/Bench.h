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
/// MEASUREMENT MODEL. A case is a `setup` / `run` / `teardown` triple. For
/// each workload size the driver builds the fixture once via `setup`, then
/// calls `run` repeatedly, accumulating each pass's elapsed `dt` until the
/// time budget is spent or the iteration cap is hit (whichever first), and
/// reports min / median / mean / stddev over the collected samples. `run`
/// owns its timing: it brackets ONLY the operation with two `ClockMonoNs()`
/// reads and a `{ }` block, keeping all input generation / result
/// consumption outside that block. A read-only `run` reuses the fixture
/// across every pass; a `run` that mutates or consumes the fixture raises
/// `ctx->dirty`, and the driver rebuilds (teardown + setup) before the next
/// pass.
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
/// There is deliberately NO `"memory"` clobber here. A memory clobber
/// would force the compiler to reload every memory operand each
/// iteration and forbid hoisting loop-invariant setup (e.g. a
/// container's element-stride computation) out of the timed loop --
/// measuring a pessimised op no real caller would see. `BenchUse` only
/// materialises the value; the loads under test still happen because
/// their addresses vary. When you genuinely need to stop the compiler
/// reordering or eliding memory effects, reach for `BenchClobber`.
///
#define BenchUse(x)                                                                                                    \
    do {                                                                                                               \
        __typeof__(x) _bench_use_v = (x);                                                                              \
        __asm__ volatile(""                                                                                            \
                         :                                                                                             \
                         : "r"(_bench_use_v));                                                                         \
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
/// Context shared across a case's setup / run / teardown for one
/// workload size. `n` is the workload size, `alloc` is the allocator to
/// build the fixture with, `rng` is the fixed-seed PRNG (re-seeded once
/// per size), `fx` is the case-owned fixture pointer (set by `setup`,
/// used by `run`, freed by `teardown`), and `dirty` is the signal a
/// `run` raises when it left the fixture unusable for the next pass.
///
typedef struct {
    u64               n;
    DefaultAllocator *alloc;
    BenchRng          rng;
    void             *fx;
    bool              dirty;
} BenchCtx;

/// Build the fixture for `ctx->n` into `ctx->fx`. Untimed.
typedef void (*BenchSetupFn)(BenchCtx *ctx);
/// Time exactly the operation under test over `ctx->fx`; return elapsed
/// nanoseconds. If the operation left the fixture unusable for a
/// subsequent pass (e.g. it mutated or consumed it), set `ctx->dirty` so
/// the driver rebuilds before the next pass.
typedef u64 (*BenchRunFn)(BenchCtx *ctx);
/// Release the fixture in `ctx->fx`. Untimed.
typedef void (*BenchTeardownFn)(BenchCtx *ctx);

///
/// A registered benchmark case.
///   name     : short identifier, e.g. "at_sequential".
///   setup    : build the fixture (once per size, and again after any
///              `dirty` pass).
///   run      : time one pass of the operation; return elapsed ns; raise
///              `ctx->dirty` if the fixture is now spent.
///   teardown : free the fixture.
///   sizes    : 0-terminated array of workload sizes. NULL means the
///              driver's default set.
///
typedef struct {
    const char     *name;
    BenchSetupFn    setup;
    BenchRunFn      run;
    BenchTeardownFn teardown;
    const u64      *sizes;
} BenchCase;

///
/// Run every case in `cases` and print result lines to stdout. Returns
/// 0. `argc`/`argv` are accepted for forward-compatible case filtering
/// and currently unused.
///
int bench_main(const char *module, const BenchCase *cases, u32 ncases, int argc, char **argv);

///
/// Build a `BenchCase` from a bare case name. Derives the display name
/// (`"<name>"`) and the three callbacks (`<name>_setup`, `<name>_run`,
/// `<name>_teardown`) by token-pasting, collapsing the descriptor to one
/// line: `BENCH_CASE(at_sequential)`.
///
#define BENCH_CASE(cname) {.name = #cname, .setup = cname##_setup, .run = cname##_run, .teardown = cname##_teardown}

///
/// Like `BENCH_CASE`, but with an explicit shared `setup`/`teardown` so
/// several cases over the same fixture reuse one pair; only `<name>_run`
/// is derived: `BENCH_CASE_FX(at_random, u64_index_setup, u64_index_teardown)`.
///
#define BENCH_CASE_FX(cname, setup_fn, teardown_fn)                                                                    \
    {.name = #cname, .setup = (setup_fn), .run = cname##_run, .teardown = (teardown_fn)}

///
/// Emit a `main` for a module file. `cases` must be a statically-sized
/// array so its length is computed here.
///
#define BENCH_MODULE(modname, cases)                                                                                   \
    int main(int argc, char **argv) {                                                                                  \
        return bench_main((modname), (cases), (u32)(sizeof(cases) / sizeof((cases)[0])), argc, argv);                  \
    }

#endif // MISRA_BENCH_H
