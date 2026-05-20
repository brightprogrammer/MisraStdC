// MisraStdC backend.
//
// HeapAllocator is per-lifetime by design in MisraStdC -- see the
// README's "one allocator per logical lifetime" section. Treating it as
// a process-wide heap, the way malloc/jemalloc/mimalloc/tcmalloc get
// used, is therefore the *least flattering* shape for it. We do it
// anyway because the user asked to compare it against general-purpose
// allocators. The arena/scope-style benches that would actually favour
// MisraStdC are a different exercise.
//
// The HeapAllocator is stack-resident (~160 B) and stamped with a magic
// value at init; AllocatorAlloc / AllocatorFree validate it on every
// call. We hold one instance for the process and use it across every
// benchmark iteration.
//
// This file is compiled as C, not C++, because HeapAllocatorInit() is a
// compound literal with designated initializers (C99) and isn't valid in
// C++ without a wrapper.

#include "Allocator.h"

#include <Misra/Std/Allocator.h>
#include <Misra/Std/Allocator/Heap.h>
#include <Misra/Std/Allocator/Page.h>

static HeapAllocator g_heap;
static Allocator    *g_alloc = NULL;
// `g_heap_typed` exists so the bench passes a typed `HeapAllocator *`
// to AllocatorAlloc / AllocatorFree. With the perf-branch _Generic
// macros that picks the typed-direct path (heap_allocator_allocate /
// heap_allocator_free), skipping AllocatorAlloc_dyn + ValidateAllocator
// + the retry loop + stats accounting. Without the macros (stock
// build) it's just an unused alias.
static HeapAllocator *g_heap_typed = NULL;

const char *bench_backend_name(void) {
    // Variant name is supplied at compile time by meson so the JSON
    // output identifies which libmisra_std was linked (fast vs full
    // heap_validate_self). Defaults to plain "misra" if undefined,
    // for callers that build a single variant.
#ifdef BENCH_MISRA_VARIANT_NAME
    return BENCH_MISRA_VARIANT_NAME;
#else
    return "misra";
#endif
}

void bench_init(void) {
    g_heap       = HeapAllocatorInit();
    g_alloc      = ALLOCATOR_OF(&g_heap);
    g_heap_typed = &g_heap;
}

// This is the "wrong tool" comparison backend: HeapAllocator is the
// general-purpose allocator and we use it across every benchmark,
// including fixed-size workloads where SlabAllocator would be the
// honest pick. The hints exist so the bench code is uniform; this
// backend deliberately ignores them, which is the whole point of
// the comparison against Allocator_misra_correct.c.
void bench_use_fixed_size(size_t slot) { (void)slot; }
void bench_use_general(void)           {}
int  bench_can_reset(void)             { return 0; }
void bench_reset(void)                 {}

void bench_teardown(void) {
    if (g_alloc) {
        HeapAllocatorDeinit(&g_heap);
        g_alloc      = NULL;
        g_heap_typed = NULL;
    }
}

void *bench_alloc(size_t n) {
    // Pass the typed HeapAllocator* so the perf-branch _Generic macro
    // picks heap_allocator_allocate directly. zeroed=0: harness zeros
    // itself when it needs to, same as malloc().
    return AllocatorAlloc(g_heap_typed, (size)n, 0);
}

void *bench_realloc(void *p, size_t n) {
    // AllocatorRealloc has no typed-direct path (composite of resize
    // + remap + stats), so this stays on the dyn wrapper either way.
    return AllocatorRealloc(g_heap_typed, p, (size)n);
}

void bench_free(void *p) {
    // typed-direct AllocatorFree resolves to heap_allocator_free
    // static-inline trampoline (one tail call into heap_allocator_deallocate).
    AllocatorFree(g_heap_typed, p);
}

uint64_t bench_live_bytes(void) {
#if FEATURE_ALLOC_STATS
    AllocatorStats s = AllocatorGetStats(g_alloc);
    return (uint64_t)s.bytes_in_use;
#else
    return 0;
#endif
}

uint64_t bench_footprint_bytes(void) {
    // The HeapAllocator's embedded PageAllocator tracks every live
    // mmap region in `entries[0..len-1]`. Each entry carries the
    // rounded mmap length (PageEntry.bytes -- see Page.h:32-40).
    // Summing gives the exact kernel-committed footprint of misra's
    // heap, with zero contamination from the bench process's other
    // memory (gbench, std::vectors, libc).
    //
    // entries[] holds: every user page that the heap's S/M/L bins
    // are carving slots out of; every XL passthrough region (one
    // mmap per >2048 B allocation); and the heap's own
    // descriptor-array storage. So it's the complete "what did
    // misra ask the kernel for" number.
    if (!g_alloc) return 0;
    const PageAllocator *p = &g_heap.page;
    uint64_t total = 0;
    for (u32 i = 0; i < p->len; i++) {
        total += (uint64_t)p->entries[i].bytes;
    }
    return total;
}
