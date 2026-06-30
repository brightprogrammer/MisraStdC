/// file      : Tests/Std/SymbolResolver.Bias.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Load-bias correctness for the `p_vaddr != p_offset` case, exercised
/// through the PUBLIC SymbolResolver API only. The fixture shared object
/// `SymbolResolver.BiasLib.c` is linked with a 64 KiB max-page-size, so its
/// RW segment's p_vaddr runs a page ahead of its p_offset. Resolving a DATA
/// symbol in that gapped segment names it ONLY when `resolver_load_bias`
/// picks the covering PT_LOAD by the address's own file offset and applies
/// the p_vaddr-space formula. The historical `start - file_offset` fallback
/// biases the file-relative address by a page, the symbol search misses, and
/// the name vanishes -- which is exactly what the boundary / arithmetic
/// mutants in `resolver_load_bias` trigger.

#include <Misra.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Parsers/ProcMaps.h>
#include <Misra/Sys/SymbolResolver.h>

#include "../../Util/TestRunner.h"

extern void *symres_bias_data_addr(void);

// Find, through the public ProcMaps API, the writable file-backed mapping of
// `path` that contains `addr` -- the fixture .so's gapped RW segment. Hands
// back its runtime start and backing-file offset so a test can compute the
// historical `start - file_offset` fallback base independently.
static bool find_rw_mapping(Zstr path, u64 addr, u64 *map_start, u64 *map_file_offset) {
    DefaultAllocator a = DefaultAllocatorInit();
    ProcMaps         maps;
    bool             got = false;
    if (ProcMapsLoad(&maps, ALLOCATOR_OF(&a))) {
        for (u64 i = 0; i < VecLen(&maps.entries); ++i) {
            const ProcMapEntry *m = VecPtrAt(&maps.entries, i);
            if (m->path && ZstrCompare(m->path, path) == 0 && (m->perms & PROC_MAP_PERM_WRITE) && addr >= m->start &&
                addr < m->end) {
                *map_start       = m->start;
                *map_file_offset = m->file_offset;
                got              = true;
                break;
            }
        }
        ProcMapsDeinit(&maps);
    }
    DefaultAllocatorDeinit(&a);
    return got;
}

// The data symbol sits in a segment whose p_vaddr leads its p_offset by a
// page. A correct load bias resolves the right file-relative address and the
// search names the symbol; the start-file_offset shortcut (or any boundary
// flip that skips the covering PT_LOAD) lands a page off and the name is gone.
bool test_bias_names_data_via_pvaddr_formula(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    SymbolResolver   res;
    if (!SymbolResolverInit(&res, ALLOCATOR_OF(&alloc))) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    void          *addr = symres_bias_data_addr();
    ResolvedSymbol r;
    bool           ok = SymbolResolverResolve(&res, addr, &r);
    ok                = ok && r.symbol_name && ZstrFindSubstring(r.symbol_name, "symres_bias_data") != NULL;
    ok                = ok && r.offset == 0;
    ok                = ok && (r.module_base + r.symbol_value + r.offset) == (u64)addr;

    SymbolResolverDeinit(&res);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// An interior address in the gapped data symbol reports offset == K against
// the same symbol value: a wrong load bias shifts the file-relative address,
// so either the symbol is missed or the offset no longer matches K.
bool test_bias_interior_offset_via_pvaddr_formula(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    SymbolResolver   res;
    if (!SymbolResolverInit(&res, ALLOCATOR_OF(&alloc))) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    u64            base = (u64)symres_bias_data_addr();
    const u64      K    = 8;
    ResolvedSymbol r0, rk;
    bool           ok = SymbolResolverResolve(&res, (void *)base, &r0);
    ok                = ok && SymbolResolverResolve(&res, (void *)(base + K), &rk);
    ok                = ok && r0.symbol_name && rk.symbol_name;
    ok                = ok && ZstrFindSubstring(rk.symbol_name, "symres_bias_data") != NULL;
    ok                = ok && rk.symbol_value == r0.symbol_value;
    ok                = ok && rk.offset == K;
    ok                = ok && r0.offset == 0;

    SymbolResolverDeinit(&res);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// An address just past the data symbol -- inside the writable mapping's page
// but beyond the RW segment's file content (seg->offset + seg->filesz) -- is
// covered by no PT_LOAD, so load-bias must use the historical fallback
// `map_start - map_file_offset`. Pinning the resolved base to that exact
// difference kills the fallback's sub->add flip.
bool test_bias_fallback_base_for_uncovered_addr(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    SymbolResolver   res;
    if (!SymbolResolverInit(&res, ALLOCATOR_OF(&alloc))) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    void          *data = symres_bias_data_addr();
    ResolvedSymbol r0;
    bool           ok = SymbolResolverResolve(&res, data, &r0) && r0.module_path && r0.symbol_size > 0;

    u64 ms = 0, mfo = 0;
    ok = ok && find_rw_mapping(r0.module_path, (u64)data, &ms, &mfo);
    ok = ok && mfo > 0;
    if (ok) {
        u64            uncovered = (u64)data + r0.symbol_size + 8;
        ResolvedSymbol rf;
        ok = SymbolResolverResolve(&res, (void *)uncovered, &rf);
        ok = ok && rf.module_base == ms - mfo;
    }

    SymbolResolverDeinit(&res);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// The data symbol fills the gapped RW segment to its file end, so the address
// one past it sits exactly at seg->offset + seg->filesz. The strict `<` upper
// bound must EXCLUDE that one-past-end address and drop to the fallback base; a
// `<=` would wrongly keep it inside the segment and report the p_vaddr base.
bool test_bias_segment_end_excluded(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    SymbolResolver   res;
    if (!SymbolResolverInit(&res, ALLOCATOR_OF(&alloc))) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    void          *data = symres_bias_data_addr();
    ResolvedSymbol r0;
    bool           ok = SymbolResolverResolve(&res, data, &r0) && r0.module_path && r0.symbol_size > 0;

    u64 ms = 0, mfo = 0;
    ok = ok && find_rw_mapping(r0.module_path, (u64)data, &ms, &mfo);
    if (ok) {
        u64            past = (u64)data + r0.symbol_size;
        ResolvedSymbol re;
        ok = SymbolResolverResolve(&res, (void *)past, &re);
        ok = ok && re.module_base == ms - mfo;       // fallback base, segment excluded
        ok = ok && re.module_base != r0.module_base; // distinct from the in-segment base
    }

    SymbolResolverDeinit(&res);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

int main(void) {
    WriteFmt("[INFO] Starting SymbolResolver.Bias tests\n\n");

    TestFunction tests[] = {
        test_bias_names_data_via_pvaddr_formula,
        test_bias_interior_offset_via_pvaddr_formula,
        test_bias_fallback_base_for_uncovered_addr,
        test_bias_segment_end_excluded,
    };

    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), NULL, 0, "SymbolResolver.Bias");
}
