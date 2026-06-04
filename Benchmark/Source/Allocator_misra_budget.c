// MisraStdC BudgetAllocator backend.
//
// BudgetAllocator is a caller-buffer, fixed-budget pool: one slot size
// for life, no growth. Backing storage is a 16 MiB static BSS buffer
// sized to cover BM_BatchAllocFree/8192 at 64 B (~0.5 MiB live) and
// BM_AllocFreePair up to 64 KiB. Workloads outside the fixed-slot
// contract (mixed sizes, realloc growth, bulk reset) leave bench_alloc
// returning NULL, which renders n/a in the column.

#include "Allocator.h"

#include <Misra/Std/Allocator.h>
#include <Misra/Std/Allocator/Budget.h>

#define BENCH_BUDGET_BUF_BYTES (16u * 1024u * 1024u)
static u8 g_budget_buf[BENCH_BUDGET_BUF_BYTES];

typedef enum {
    BUDGET_MODE_NONE = 0,
    BUDGET_MODE_LIVE = 1,
} bench_budget_mode;

static BudgetAllocator   g_budget;
static bench_budget_mode g_mode = BUDGET_MODE_NONE;

Zstr bench_backend_name(void) {
#ifdef BENCH_BACKEND_NAME
    return BENCH_BACKEND_NAME;
#else
    return "misra-budget";
#endif
}

void bench_init(void) {
    g_mode = BUDGET_MODE_NONE;
}

void bench_teardown(void) {
    g_mode = BUDGET_MODE_NONE;
}

void bench_use_fixed_size(size_t slot) {
    if (slot == 0u) {
        g_mode = BUDGET_MODE_NONE;
        return;
    }
    g_budget = BudgetAllocatorInit(g_budget_buf, (size)BENCH_BUDGET_BUF_BYTES, (size)slot);
    g_mode   = BUDGET_MODE_LIVE;
}

void bench_use_general(void) {
    // Mixed-size workloads can't be served by a single Budget instance;
    // the slot size is fixed for life. Drop to NONE so bench_alloc
    // reports n/a for those rows instead of mis-routing.
    g_mode = BUDGET_MODE_NONE;
}

int bench_can_reset(void) {
    return 0;
}
void bench_reset(void) {}

void *bench_alloc(size_t n) {
    if (g_mode != BUDGET_MODE_LIVE) {
        return NULL;
    }
    return AllocatorAlloc(&g_budget, (size)n, 0);
}

void *bench_realloc(void *p, size_t n) {
    // Budget's remap fails once new_size exceeds slot_size, so the
    // BM_ReallocGrow ladder (8 B .. 1 MiB) can't be served on any
    // single fixed-slot Budget. Return NULL so the row reads n/a.
    (void)p;
    (void)n;
    return NULL;
}

void bench_free(void *p) {
    if (g_mode != BUDGET_MODE_LIVE || p == NULL) {
        return;
    }
    AllocatorFree(&g_budget, p);
}

uint64_t bench_live_bytes(void) {
#if FEATURE_ALLOC_STATS
    if (g_mode != BUDGET_MODE_LIVE) {
        return 0;
    }
    return (uint64_t)AllocatorBytesInUse(&g_budget);
#else
    return 0;
#endif
}

uint64_t bench_footprint_bytes(void) {
    // Budget's footprint = the caller buffer it was handed. There is
    // no separate OS-page accounting because Budget never calls the
    // kernel. Report the whole buffer when live, zero otherwise.
    if (g_mode != BUDGET_MODE_LIVE) {
        return 0;
    }
    return (uint64_t)BENCH_BUDGET_BUF_BYTES;
}
