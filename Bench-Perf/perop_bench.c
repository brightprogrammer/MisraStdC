// Per-operation latency measurement using clock_gettime(CLOCK_MONOTONIC).
//
// Records nanoseconds for EVERY individual alloc and EVERY individual
// free. Dumps medians + percentiles so you can see the distribution,
// not just averages.
//
// Build:
//   clang -O3 -g -march=x86-64-v3 -IInclude -Ibuild-perf/Include \
//     Bench-Perf/perop_bench.c build-perf/libmisra_std.a -lm -o /tmp/perop_misra
//   clang -O3 -g -march=x86-64-v3 -DUSE_LIBC \
//     Bench-Perf/perop_bench.c -lm -o /tmp/perop_libc

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef USE_LIBC
#  include <Misra/Std/Allocator.h>
#  include <Misra/Std/Allocator/Heap.h>
#endif

#define N    16384  // ops per phase
#define SZ   64
#define REPS 50    // outer reps for warmup + noise reduction

static void *ptrs[N];

// Pull the timer into one place so the inlining cost is identical
// across libc/misra paths. Marked `inline` because the per-op
// bookkeeping calls it twice per iteration and we want the
// clock_gettime call sequence visible at each measurement site.
static inline uint64_t now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

// Two-buffer storage for per-op timings, lazily allocated big enough
// for the whole run.
#define TOTAL_OPS (N * REPS)
static uint64_t alloc_ns[TOTAL_OPS];
static uint64_t free_ns[TOTAL_OPS];

static int cmp_u64(const void *a, const void *b) {
    uint64_t x = *(const uint64_t *)a, y = *(const uint64_t *)b;
    return (x > y) - (x < y);
}

static void report(const char *label, uint64_t *arr, size_t n) {
    qsort(arr, n, sizeof(uint64_t), cmp_u64);
    uint64_t sum = 0;
    for (size_t i = 0; i < n; ++i) sum += arr[i];
    double mean = (double)sum / (double)n;
    printf("%-15s  n=%zu  min=%lu  p50=%lu  p90=%lu  p99=%lu  p999=%lu  max=%lu  mean=%.2f\n",
        label, n,
        arr[0],
        arr[n/2],
        arr[(size_t)(n*0.90)],
        arr[(size_t)(n*0.99)],
        arr[(size_t)(n*0.999)],
        arr[n-1],
        mean);
}

int main(void) {
#ifndef USE_LIBC
    HeapAllocator h = HeapAllocatorInit();
    // Typed dispatch: the _Generic in AllocatorAlloc picks
    // heap_allocator_allocate directly. Passing `Allocator *` would
    // route through AllocatorAlloc_dyn (ValidateAllocator + vtable
    // indirect) and measure a different path than misra_bench does.
#define DO_ALLOC()  AllocatorAlloc(&h, SZ, 0)
#define DO_FREE(p)  AllocatorFree(&h, p)
#else
#define DO_ALLOC()  malloc(SZ)
#define DO_FREE(p)  free(p)
#endif

    // Warmup: fill & drain once so descriptor arrays are sized.
    for (size_t i = 0; i < N; i++) ptrs[i] = DO_ALLOC();
    for (size_t i = N; i-- > 0;) DO_FREE(ptrs[i]);

    size_t a_idx = 0, f_idx = 0;

    for (int r = 0; r < REPS; r++) {
        // Alloc phase, per-op timing.
        for (size_t i = 0; i < N; i++) {
            uint64_t t0 = now_ns();
            ptrs[i]     = DO_ALLOC();
            uint64_t t1 = now_ns();
            alloc_ns[a_idx++] = t1 - t0;
        }
        // Free phase, per-op timing.
        for (size_t i = N; i-- > 0;) {
            uint64_t t0 = now_ns();
            DO_FREE(ptrs[i]);
            uint64_t t1 = now_ns();
            free_ns[f_idx++] = t1 - t0;
        }
    }

    report("alloc (ns)", alloc_ns, a_idx);
    report("free  (ns)", free_ns, f_idx);

#ifndef USE_LIBC
    HeapAllocatorDeinit(&h);
#endif
    return 0;
}
