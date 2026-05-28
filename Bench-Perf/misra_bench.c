// Standalone misra HeapAllocator bench for `perf record`.
//
// Runs many iterations of "alloc N items, free N items" so perf can
// attribute cycles to specific functions. No Google Benchmark, no C++,
// no libstdc++ noise.
//
// Compile:
//   nix-shell -p clang --command 'clang -O3 -g -march=x86-64-v3 \
//       -IInclude -Ibuild-bench/Include \
//       Bench-Perf/misra_bench.c build-bench/libmisra_std.a -lm -o /tmp/misra_bench'
//
// Run under perf:
//   perf record -F 9999 --call-graph=lbr -o /tmp/misra.data -- /tmp/misra_bench alloc
//   perf report -i /tmp/misra.data

#include <Misra/Std/Allocator.h>
#include <Misra/Std/Allocator/Heap.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define N    16384
#define SZ   64
#define REPS 200

static void *ptrs[N];

// Two top-level entry points so `perf report` cleanly separates them.
// `__attribute__((noinline))` keeps perf from folding them into main.

// Pass the TYPED HeapAllocator* so _Generic picks the
// heap_allocator_allocate/heap_allocator_deallocate symbols
// directly. Passing as Allocator* would route through
// AllocatorAlloc_dyn + ValidateAllocator (the runtime-vtable path)
// which is only needed when the concrete type isn't known at the
// call site. The official bench (Allocator_misra.c) does the same.
__attribute__((noinline))
static void misra_alloc_phase(HeapAllocator *h) {
    for (size_t i = 0; i < N; i++) {
        ptrs[i] = AllocatorAlloc(h, SZ, 0);
    }
}

__attribute__((noinline))
static void misra_free_phase(HeapAllocator *h) {
    for (size_t i = N; i-- > 0;) {
        AllocatorFree(h, ptrs[i]);
    }
}

int main(int argc, char **argv) {
    int run_alloc = 1, run_free = 1;
    if (argc >= 2) {
        if (strcmp(argv[1], "alloc") == 0) { run_free = 0; }
        else if (strcmp(argv[1], "free") == 0) { run_alloc = 0; }
        else if (strcmp(argv[1], "both") == 0) {}
        else { fprintf(stderr, "usage: %s [alloc|free|both]\n", argv[0]); return 1; }
    }

    HeapAllocator h = HeapAllocatorInit();

    // Warm up: one full cycle so descriptor array reaches steady-state
    // size. This isolates measurement from one-time growth costs.
    misra_alloc_phase(&h);
    misra_free_phase(&h);

    if (run_alloc && run_free) {
        // both: full alloc-then-free cycle per rep.
        for (int r = 0; r < REPS; r++) {
            misra_alloc_phase(&h);
            misra_free_phase(&h);
        }
    } else if (run_alloc) {
        // alloc-only: allocate once per rep, free at the END after all
        // reps so perf samples land entirely in the alloc path. Costs
        // REPS*N allocations of working memory; HeapAllocator handles
        // this comfortably for N=16384.
        for (int r = 0; r < REPS; r++) {
            misra_alloc_phase(&h);
        }
        misra_free_phase(&h);
    } else if (run_free) {
        // free-only: pre-allocate once outside the timed loop, then
        // free + re-alloc per rep. The re-alloc is a known confound
        // (it shows up in perf samples too); the alternative would be
        // to alloc REPS*N up front and free them in chunks, which
        // changes per-iteration cache behaviour. Document and leave
        // the trade-off for the operator to interpret.
        misra_alloc_phase(&h);
        for (int r = 0; r < REPS; r++) {
            misra_free_phase(&h);
            misra_alloc_phase(&h);
        }
        misra_free_phase(&h);
    }

    HeapAllocatorDeinit(&h);
    return 0;
}
