/// file      : Tests/Std/SymbolResolver.BestEffort.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// BEST-EFFORT SymbolResolver tests: these assert build-layout-sensitive
/// behaviour (`.eh_frame` FDE coverage) that does NOT survive an -O0 +
/// mull-IR-plugin build, so they FAIL their baseline under that build and
/// cannot be measured by mutation testing. They are split out of
/// `SymbolResolver.c` / `SymbolResolver.Internal.c` so those two files keep
/// VALID mull baselines (all-pass), while this file is registered separately
/// as baseline-failing-but-expected.
///
/// Moved here:
///   * from SymbolResolver.c (public API, FindFde pc-range):
///       test_sr1_findfde_covers_entry
///       test_sr1_findfde_interior
///       test_sr1_findfde_cached
///   * from SymbolResolver.Internal.c (source-include, CFI teardown):
///       test_sr3_deinit_frees_cfi
///
/// The source unit is included ONCE below so the static internals used by
/// the sr3 test are visible AND the public symbols (SymbolResolverInit /
/// Resolve / Deinit / FindFde) are supplied to the sr1 tests — no duplicate
/// definitions, the library object is never pulled.

#include <Misra.h>
#include <Misra/Std/Allocator/Debug.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Sys/SymbolResolver.h>

#include "../Util/TestRunner.h"

// Pull in the unit under test ONCE: provides the public symbols for the sr1
// tests and exposes the static cache internals for the sr3 CFI test.
#include "../../Source/Misra/Sys/SymbolResolver.c"

#if FEATURE_PARSER_DWARF
#    include <Misra/Parsers/Dwarf.h>

// ---------------------------------------------------------------------------
// Markers for the public-API FindFde tests. Built at -O0 with sanitizers in
// test config so the symbols survive in .symtab with non-zero size; the
// volatile asm padding forces a real, sizeable body.
// ---------------------------------------------------------------------------

static __attribute__((noinline)) void sr1_marker_a(void) {
    __asm__ __volatile__("" ::
                             : "memory");
    __asm__ __volatile__("nop; nop; nop; nop; nop; nop; nop; nop" ::
                             : "memory");
    __asm__ __volatile__("nop; nop; nop; nop; nop; nop; nop; nop" ::
                             : "memory");
    __asm__ __volatile__("nop; nop; nop; nop; nop; nop; nop; nop" ::
                             : "memory");
    __asm__ __volatile__("nop; nop; nop; nop; nop; nop; nop; nop" ::
                             : "memory");
    __asm__ __volatile__("nop; nop; nop; nop; nop; nop; nop; nop" ::
                             : "memory");
    __asm__ __volatile__("nop; nop; nop; nop; nop; nop; nop; nop" ::
                             : "memory");
    __asm__ __volatile__("" ::
                             : "memory");
}

static __attribute__((noinline)) void sr1_marker_b(void) {
    __asm__ __volatile__("" ::
                             : "memory");
    __asm__ __volatile__("nop; nop; nop; nop; nop; nop; nop; nop" ::
                             : "memory");
    __asm__ __volatile__("nop; nop; nop; nop; nop; nop; nop; nop" ::
                             : "memory");
    __asm__ __volatile__("" ::
                             : "memory");
}

// === FindFde: pc-range boundaries ==========================================

// An FDE covers the marker function and its range contains the entry
// address (file-relative). module_base reported matches the resolve path.
bool test_sr1_findfde_covers_entry(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    SymbolResolver   res;
    if (!SymbolResolverInit(&res, ALLOCATOR_OF(&alloc))) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    const DwarfCfi *cfi   = NULL;
    const DwarfFde *fde   = NULL;
    u64             mbase = 0;
    u64             addr  = (u64)(void *)&sr1_marker_a;
    bool            ok    = SymbolResolverFindFde(&res, (void *)addr, &cfi, &fde, &mbase);
    ok                    = ok && cfi != NULL && fde != NULL;
    if (ok) {
        u64 fr = addr - mbase;
        // Entry is inside [pc_begin, pc_begin + pc_range).
        ok = ok && fr >= fde->pc_begin;
        ok = ok && fr < fde->pc_begin + fde->pc_range;
        ok = ok && fde->pc_range > 0;
        ok = ok && mbase != 0;
    }

    SymbolResolverDeinit(&res);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// FDE found for an interior address; pin module_base consistency: addr -
// module_base == file_relative used for the lookup, exercising the
// addr - load_base subtraction in FindFde.
bool test_sr1_findfde_interior(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    SymbolResolver   res;
    if (!SymbolResolverInit(&res, ALLOCATOR_OF(&alloc))) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    const DwarfCfi *cfi   = NULL;
    const DwarfFde *fde   = NULL;
    u64             mbase = 0;
    u64             base  = (u64)(void *)&sr1_marker_a;
    bool            ok    = SymbolResolverFindFde(&res, (void *)(base + 8), &cfi, &fde, &mbase);
    ok                    = ok && fde != NULL;
    if (ok) {
        u64 fr = (base + 8) - mbase;
        ok     = ok && fr >= fde->pc_begin;
        ok     = ok && fr < fde->pc_begin + fde->pc_range;
        // The FDE for the interior address is the same one covering the
        // entry (range starts at or before the entry's file-relative addr).
        u64 entry_fr = base - mbase;
        ok           = ok && entry_fr >= fde->pc_begin;
    }

    SymbolResolverDeinit(&res);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// FindFde for the same address resolved twice returns a stable module_base
// and FDE pc range, exercising the cached-cfi branch on the second call.
bool test_sr1_findfde_cached(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    SymbolResolver   res;
    if (!SymbolResolverInit(&res, ALLOCATOR_OF(&alloc))) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    const DwarfCfi *c1 = NULL, *c2 = NULL;
    const DwarfFde *f1 = NULL, *f2 = NULL;
    u64             b1 = 0, b2 = 0;
    u64             addr = (u64)(void *)&sr1_marker_b;
    bool            ok   = SymbolResolverFindFde(&res, (void *)addr, &c1, &f1, &b1);
    ok                   = ok && SymbolResolverFindFde(&res, (void *)addr, &c2, &f2, &b2);
    ok                   = ok && f1 && f2 && b1 == b2 && b1 != 0;
    ok                   = ok && f1->pc_begin == f2->pc_begin;
    ok                   = ok && f1->pc_range == f2->pc_range;
    if (ok) {
        u64 fr = addr - b1;
        ok     = ok && fr >= f1->pc_begin && fr < f1->pc_begin + f1->pc_range;
    }

    SymbolResolverDeinit(&res);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// CFI teardown marker (from SymbolResolver.Internal.c). No-inline marker in
// OUR module; -O0 test builds keep its .symtab entry.
// ---------------------------------------------------------------------------

static __attribute__((noinline)) void sr3_marker_a(void) {
    __asm__ __volatile__("" ::
                             : "memory");
}

// Same teardown contract as test_sr3_deinit_frees_everything, but first
// build the cache entry's `.eh_frame` CFI via FindFde so the
// `DwarfCfiDeinit(&cfi)` arm of the cleanup loop is exercised: removing it
// leaves the CFI tables leaked and the post-Deinit live count above
// baseline.
bool test_sr3_deinit_frees_cfi(void) {
    DebugAllocator alloc    = DebugAllocatorInit();
    size           baseline = DebugAllocatorLiveCount(&alloc);

    SymbolResolver res;
    if (!SymbolResolverInit(&res, ALLOCATOR_OF(&alloc)))
        return false;

    ResolvedSymbol ra;
    bool           ok = SymbolResolverResolve(&res, (void *)&sr3_marker_a, &ra);

    const DwarfCfi *cfi = NULL;
    const DwarfFde *fde = NULL;
    u64             mb  = 0;
    ok                  = ok && SymbolResolverFindFde(&res, (void *)&sr3_marker_a, &cfi, &fde, &mb);
    ok                  = ok && cfi != NULL && fde != NULL;
    ok                  = ok && DebugAllocatorLiveCount(&alloc) > baseline;

    SymbolResolverDeinit(&res);
    ok = ok && DebugAllocatorLiveCount(&alloc) == baseline;

    DebugAllocatorDeinit(&alloc);
    return ok;
}
#endif

int main(void) {
    WriteFmt("[INFO] Starting SymbolResolver.BestEffort tests\n\n");

    TestFunction tests[] = {
#if FEATURE_PARSER_DWARF
        test_sr1_findfde_covers_entry,
        test_sr1_findfde_interior,
        test_sr1_findfde_cached,
        test_sr3_deinit_frees_cfi,
#endif
    };

    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), NULL, 0, "SymbolResolver.BestEffort");
}
