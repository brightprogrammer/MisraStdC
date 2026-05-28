// Mirror of misra_bench.c for libc malloc/free. Same N, SZ, REPS so
// perf cycle counts are directly comparable.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define N    16384
#define SZ   64
#define REPS 200

static void *ptrs[N];

__attribute__((noinline))
static void libc_alloc_phase(void) {
    for (size_t i = 0; i < N; i++) {
        ptrs[i] = malloc(SZ);
    }
}

__attribute__((noinline))
static void libc_free_phase(void) {
    for (size_t i = N; i-- > 0;) {
        free(ptrs[i]);
    }
}

int main(int argc, char **argv) {
    // Warm up
    libc_alloc_phase();
    libc_free_phase();

    for (int r = 0; r < REPS; r++) {
        libc_alloc_phase();
        libc_free_phase();
    }

    (void)argc;
    (void)argv;
    return 0;
}
