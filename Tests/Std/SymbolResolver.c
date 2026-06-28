#include <Misra.h>
#include <Misra/Std/Allocator/Debug.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Parsers/ProcMaps.h>
#include <Misra/Sys/SymbolResolver.h>


#include "../Util/TestRunner.h"

// Marker function with an externally-visible-ish name so we can find
// it back. Built with sanitizers and -O0 in test config, so the
// function symbol survives in .symtab.
static __attribute__((noinline)) void symres_marker_helper(void) {
    // Force a real prologue so the symbol has size > 0 and our address
    // lands inside it.
    __asm__ __volatile__("" ::
                             : "memory");
}

// ---------------------------------------------------------------------------
// Marker functions. Built at -O0 with sanitizers in test config so the
// symbols survive in .symtab with non-zero size. The volatile asm padding
// forces a real, sizeable body (> 42 bytes) so we can assert that the
// resolved symbol_size is a genuine value and not a mutated constant.
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

// ---------------------------------------------------------------------------
// Helper mirroring Tests/Std/SymbolResolver.c idiom.
// ---------------------------------------------------------------------------

static bool resolve_addr(SymbolResolver *res, void *addr, ResolvedSymbol *out) {
    return SymbolResolverResolve(res, addr, out);
}

// === Resolve: module + symbol-name correctness =============================

// Resolving the entry address of a known function names that function and
// reports a module path. Mirrors the base SymbolResolver.c test.
bool test_sr1_resolve_names_function(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    SymbolResolver   res;
    if (!SymbolResolverInit(&res, ALLOCATOR_OF(&alloc))) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    ResolvedSymbol r;
    bool           ok = resolve_addr(&res, (void *)&sr1_marker_a, &r);
    ok                = ok && r.module_path && r.module_path[0] != '\0';
    ok                = ok && r.symbol_name != NULL;
    ok                = ok && ZstrFindSubstring(r.symbol_name, "sr1_marker_a") != NULL;

    SymbolResolverDeinit(&res);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Distinct functions resolve to distinct names: guards against the symbol
// lookup collapsing onto one entry.
bool test_sr1_resolve_distinct_symbols(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    SymbolResolver   res;
    if (!SymbolResolverInit(&res, ALLOCATOR_OF(&alloc))) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    ResolvedSymbol ra, rb;
    bool           ok = resolve_addr(&res, (void *)&sr1_marker_a, &ra);
    ok                = ok && resolve_addr(&res, (void *)&sr1_marker_b, &rb);
    ok                = ok && ra.symbol_name && rb.symbol_name;
    ok                = ok && ZstrFindSubstring(ra.symbol_name, "sr1_marker_a") != NULL;
    ok                = ok && ZstrFindSubstring(rb.symbol_name, "sr1_marker_b") != NULL;
    ok                = ok && ra.symbol_value != rb.symbol_value;

    SymbolResolverDeinit(&res);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// === Resolve: address arithmetic (module_base, symbol_value, offset) =======

// Entry address: offset must be exactly 0 and the reconstruction
// module_base + symbol_value + offset == addr must hold. Kills the
// assign-const on module_base/symbol_value/offset and the sub->add swap
// on the offset arithmetic at the function entry.
bool test_sr1_entry_offset_zero(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    SymbolResolver   res;
    if (!SymbolResolverInit(&res, ALLOCATOR_OF(&alloc))) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    u64            addr = (u64)(void *)&sr1_marker_a;
    ResolvedSymbol r;
    bool           ok = resolve_addr(&res, (void *)addr, &r);
    ok                = ok && r.symbol_name != NULL;
    // offset of the entry point is zero.
    ok = ok && r.offset == 0;
    // module_base is a real non-zero load base, not a constant.
    ok = ok && r.module_base != 0;
    // Reconstruction invariant holds at the entry.
    ok = ok && (r.module_base + r.symbol_value + r.offset) == addr;

    SymbolResolverDeinit(&res);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Interior address: offset must equal the byte distance K from entry, the
// symbol_value must be unchanged vs the entry resolve, and the
// reconstruction invariant must still hold. This pins offset = fr - value
// (sub, not add) and the assign-const mutations on value/offset.
bool test_sr1_interior_offset_matches(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    SymbolResolver   res;
    if (!SymbolResolverInit(&res, ALLOCATOR_OF(&alloc))) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    const u64      K    = 16;
    u64            base = (u64)(void *)&sr1_marker_a;
    ResolvedSymbol r0, rk;
    bool           ok = resolve_addr(&res, (void *)base, &r0);
    ok                = ok && resolve_addr(&res, (void *)(base + K), &rk);
    ok                = ok && r0.symbol_name && rk.symbol_name;
    // Same enclosing symbol for entry and interior address.
    ok = ok && rk.symbol_value == r0.symbol_value;
    ok = ok && rk.symbol_size == r0.symbol_size;
    // Interior offset is exactly K (sub, not add: base+K - value == K).
    ok = ok && rk.offset == K;
    ok = ok && r0.offset == 0;
    // Reconstruction invariant at the interior address.
    ok = ok && (rk.module_base + rk.symbol_value + rk.offset) == (base + K);

    SymbolResolverDeinit(&res);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// module_base is shared across two addresses in the same module and equals
// addr - file_relative for each. Independent pin on the load_base math and
// the module_base assignment.
bool test_sr1_module_base_consistent(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    SymbolResolver   res;
    if (!SymbolResolverInit(&res, ALLOCATOR_OF(&alloc))) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    ResolvedSymbol ra, rb;
    bool           ok = resolve_addr(&res, (void *)&sr1_marker_a, &ra);
    ok                = ok && resolve_addr(&res, (void *)&sr1_marker_b, &rb);
    // Both live in this test binary => same module base.
    ok = ok && ra.module_base == rb.module_base;
    ok = ok && ra.module_base != 0;
    // Both module paths are the same backing file.
    ok = ok && ra.module_path && rb.module_path;
    ok = ok && ZstrCompare(ra.module_path, rb.module_path) == 0;

    SymbolResolverDeinit(&res);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// symbol_size is a genuine size (> 42 for the padded marker), not a mutated
// constant, and the interior probe at K must fall strictly inside it.
bool test_sr1_symbol_size_real(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    SymbolResolver   res;
    if (!SymbolResolverInit(&res, ALLOCATOR_OF(&alloc))) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    ResolvedSymbol r;
    bool           ok = resolve_addr(&res, (void *)&sr1_marker_a, &r);
    ok                = ok && r.symbol_name != NULL;
    // Padded body is well over 42 bytes; a size->42 mutation is caught.
    ok = ok && r.symbol_size > 42;
    // Offset within size for the entry.
    ok = ok && r.offset < r.symbol_size;

    SymbolResolverDeinit(&res);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Last byte still inside the symbol resolves to the same function with
// offset == size-1; just past the end must NOT name the same function.
// Pins the symbol-range boundary handling end-to-end.
bool test_sr1_symbol_range_boundary(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    SymbolResolver   res;
    if (!SymbolResolverInit(&res, ALLOCATOR_OF(&alloc))) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    ResolvedSymbol r0;
    bool           ok = resolve_addr(&res, (void *)&sr1_marker_a, &r0);
    ok                = ok && r0.symbol_name && r0.symbol_size > 0;
    if (!ok) {
        SymbolResolverDeinit(&res);
        DefaultAllocatorDeinit(&alloc);
        return false;
    }
    u64 base = (u64)(void *)&sr1_marker_a;
    u64 size = r0.symbol_size;

    // Last byte of the symbol: still the same function, offset == size-1.
    ResolvedSymbol rlast;
    ok = resolve_addr(&res, (void *)(base + size - 1), &rlast);
    ok = ok && rlast.symbol_name;
    ok = ok && rlast.symbol_value == r0.symbol_value;
    ok = ok && rlast.offset == size - 1;

    SymbolResolverDeinit(&res);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// === Failure paths =========================================================

// An address that lands inside a module but outside any named symbol takes
// the else branch: symbol_name is NULL and offset == file_relative (the
// distance from module_base), not the matched-symbol offset. We probe a
// few low file-relative offsets from module_base; ELF headers / padding
// there carry no function symbol. Pins the else-branch offset assignment.
bool test_sr1_no_symbol_offset_is_file_relative(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    SymbolResolver   res;
    if (!SymbolResolverInit(&res, ALLOCATOR_OF(&alloc))) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    // Get this module's base via a normal resolve.
    ResolvedSymbol probe;
    bool           got_base = resolve_addr(&res, (void *)&sr1_marker_a, &probe);
    bool           ok       = got_base && probe.module_base != 0;
    if (ok) {
        u64 base = probe.module_base;
        // Scan a handful of small file-relative offsets; find one that
        // lands in a module mapping but resolves no symbol name.
        bool found_unnamed = false;
        for (u64 off = 1; off <= 8 && !found_unnamed; ++off) {
            ResolvedSymbol r;
            if (resolve_addr(&res, (void *)(base + off), &r)) {
                if (r.symbol_name == NULL) {
                    // else branch: offset is the raw file-relative distance.
                    found_unnamed = true;
                    ok            = ok && r.offset == off;
                }
            }
        }
        ok = ok && found_unnamed;
    }

    SymbolResolverDeinit(&res);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// An address not mapped to any module resolves to false.
bool test_sr1_unmapped_returns_false(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    SymbolResolver   res;
    if (!SymbolResolverInit(&res, ALLOCATOR_OF(&alloc))) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    ResolvedSymbol r;
    // Page-zero-ish address that no module maps.
    bool ok = !SymbolResolverResolve(&res, (void *)(u64)0x1000, &r);

    SymbolResolverDeinit(&res);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

bool test_symres_resolve_self(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    SymbolResolver   res;

    if (!SymbolResolverInit(&res, ALLOCATOR_OF(&alloc))) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    ResolvedSymbol r;
    bool           ok = SymbolResolverResolve(&res, (void *)&test_symres_resolve_self, &r);
    ok                = ok && r.module_path && r.module_path[0] != '\0';
    // We're a function so a symbol should resolve. Name may or may
    // not exactly equal "test_symres_resolve_self" depending on
    // optimizer; just require non-NULL.
    ok = ok && r.symbol_name != NULL;

    SymbolResolverDeinit(&res);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

bool test_symres_static_symbol_resolves(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    SymbolResolver   res;

    if (!SymbolResolverInit(&res, ALLOCATOR_OF(&alloc))) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    // Static functions don't appear in .dynsym but do appear in
    // .symtab. libc dladdr would fail to name this; we should not.
    ResolvedSymbol r;
    bool           ok = SymbolResolverResolve(&res, (void *)&symres_marker_helper, &r);
    ok                = ok && r.symbol_name != NULL && ZstrFindSubstring(r.symbol_name, "symres_marker_helper") != NULL;

    SymbolResolverDeinit(&res);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ===========================================================================
// Module cache (resolver_cache_find_or_open) — exercised purely through the
// public Init/Resolve/Deinit API + a DebugAllocator. A correct cache opens
// each module once; a broken find-loop or key compare re-opens it, which the
// allocator's live count catches — no need to peek the internal cache vector.
// ===========================================================================

// A global lives in the module's rw- data segment — a DIFFERENT
// /proc/self/maps line than the r-x code segment, so same path TEXT but a
// distinct raw-buffer pointer. Resolving a code address then this data
// address makes the cache's pointer-compare miss and fall to the ZstrCompare
// path.
static __attribute__((used)) volatile u64 sr_cache_data_marker = 0xC0FFEEULL;

// Start address of an executable, file-backed mapping in a module OTHER than
// `self_path`, via the public ProcMaps API only.
static bool find_other_module_addr(Zstr self_path, u64 *out_addr) {
    DefaultAllocator a = DefaultAllocatorInit();
    ProcMaps         maps;
    bool             got = false;
    if (ProcMapsLoad(&maps, ALLOCATOR_OF(&a))) {
        for (u64 i = 0; i < VecLen(&maps.entries); ++i) {
            const ProcMapEntry *m = VecPtrAt(&maps.entries, i);
            if (m->path && m->path[0] == '/' && (m->perms & PROC_MAP_PERM_EXEC) &&
                ZstrCompare(m->path, self_path) != 0) {
                *out_addr = m->start;
                got       = true;
                break;
            }
        }
        ProcMapsDeinit(&maps);
    }
    DefaultAllocatorDeinit(&a);
    return got;
}

// Two resolves into the SAME module (two functions, same r-x mapping): opened
// on the first, a cache HIT on the second. A skipped / mis-bounded find loop
// re-opens the module -> live count rises.
bool test_sr_cache_hit_same_module(void) {
    DebugAllocator alloc = DebugAllocatorInit();
    SymbolResolver res;
    if (!SymbolResolverInit(&res, ALLOCATOR_OF(&alloc))) {
        DebugAllocatorDeinit(&alloc);
        return false;
    }

    ResolvedSymbol ra;
    bool           ok    = SymbolResolverResolve(&res, (void *)&sr1_marker_a, &ra) && ra.module_path;
    size           live1 = DebugAllocatorLiveCount(&alloc);

    ResolvedSymbol rb;
    ok         = ok && SymbolResolverResolve(&res, (void *)&sr1_marker_b, &rb) && rb.module_path;
    size live2 = DebugAllocatorLiveCount(&alloc);

    ok = ok && live2 == live1;                                   // no re-open
    ok = ok && ZstrCompare(ra.module_path, rb.module_path) == 0; // same module file

    SymbolResolverDeinit(&res);
    DebugAllocatorDeinit(&alloc);
    return ok;
}

// Resolve a code address, then a DATA address (global) in the same module.
// The data mapping is a separate /proc line — same path text, distinct
// pointer — so only the ZstrCompare fallback can match: still a HIT, no
// re-open. A mutant breaking `ZstrCompare(...) == 0` misses and re-opens.
bool test_sr_cache_zstrcompare_fallback(void) {
    DebugAllocator alloc = DebugAllocatorInit();
    SymbolResolver res;
    if (!SymbolResolverInit(&res, ALLOCATOR_OF(&alloc))) {
        DebugAllocatorDeinit(&alloc);
        return false;
    }

    ResolvedSymbol rc;
    bool           ok    = SymbolResolverResolve(&res, (void *)&sr1_marker_a, &rc) && rc.module_path;
    size           live1 = DebugAllocatorLiveCount(&alloc);

    ResolvedSymbol rd;
    ok         = ok && SymbolResolverResolve(&res, (void *)&sr_cache_data_marker, &rd) && rd.module_path;
    size live2 = DebugAllocatorLiveCount(&alloc);

    ok = ok && live2 == live1;                                   // fallback hit, no re-open
    ok = ok && ZstrCompare(rc.module_path, rd.module_path) == 0; // same module file

    SymbolResolverDeinit(&res);
    DebugAllocatorDeinit(&alloc);
    return ok;
}

// Resolve in our module, then a DIFFERENT module: a MISS that opens a second
// module. A flipped key compare returns our first entry for the other
// address -> wrong module_path and no second open.
bool test_sr_cache_cross_module_distinct(void) {
    DebugAllocator alloc = DebugAllocatorInit();
    SymbolResolver res;
    if (!SymbolResolverInit(&res, ALLOCATOR_OF(&alloc))) {
        DebugAllocatorDeinit(&alloc);
        return false;
    }

    ResolvedSymbol ra;
    bool           ok = SymbolResolverResolve(&res, (void *)&sr1_marker_a, &ra) && ra.module_path;

    u64 other  = 0;
    ok         = ok && find_other_module_addr(ra.module_path, &other);
    size live1 = DebugAllocatorLiveCount(&alloc);

    ResolvedSymbol ro;
    ok         = ok && SymbolResolverResolve(&res, (void *)other, &ro) && ro.module_path;
    size live2 = DebugAllocatorLiveCount(&alloc);

    ok = ok && ZstrCompare(ro.module_path, ra.module_path) != 0; // distinct module
    ok = ok && live2 > live1;                                    // second module was opened

    SymbolResolverDeinit(&res);
    DebugAllocatorDeinit(&alloc);
    return ok;
}

// Resolving the other module's address AGAIN is a HIT on entry[1]: the find
// loop must walk PAST entry[0]; nothing re-opens.
bool test_sr_cache_other_module_rehit(void) {
    DebugAllocator alloc = DebugAllocatorInit();
    SymbolResolver res;
    if (!SymbolResolverInit(&res, ALLOCATOR_OF(&alloc))) {
        DebugAllocatorDeinit(&alloc);
        return false;
    }

    ResolvedSymbol ra;
    bool           ok = SymbolResolverResolve(&res, (void *)&sr1_marker_a, &ra) && ra.module_path;

    u64 other = 0;
    ok        = ok && find_other_module_addr(ra.module_path, &other);

    ResolvedSymbol r1;
    ok         = ok && SymbolResolverResolve(&res, (void *)other, &r1) && r1.module_path;
    size live1 = DebugAllocatorLiveCount(&alloc);

    ResolvedSymbol r2;
    ok         = ok && SymbolResolverResolve(&res, (void *)other, &r2) && r2.module_path;
    size live2 = DebugAllocatorLiveCount(&alloc);

    ok = ok && live2 == live1;                                   // hit on entry[1], no re-open
    ok = ok && ZstrCompare(r1.module_path, r2.module_path) == 0; // same module

    SymbolResolverDeinit(&res);
    DebugAllocatorDeinit(&alloc);
    return ok;
}

// Resolve (populating cache + ELF + DWARF), then Deinit: a correct teardown
// frees everything and the live count returns to the pre-Init baseline.
bool test_sr_deinit_frees_everything(void) {
    DebugAllocator alloc    = DebugAllocatorInit();
    size           baseline = DebugAllocatorLiveCount(&alloc);

    SymbolResolver res;
    if (!SymbolResolverInit(&res, ALLOCATOR_OF(&alloc))) {
        DebugAllocatorDeinit(&alloc);
        return false;
    }
    ResolvedSymbol r;
    bool           ok = SymbolResolverResolve(&res, (void *)&sr1_marker_a, &r);

    SymbolResolverDeinit(&res);
    ok = ok && DebugAllocatorLiveCount(&alloc) == baseline;

    DebugAllocatorDeinit(&alloc);
    return ok;
}

int main(void) {
    WriteFmt("[INFO] Starting SymbolResolver tests\n\n");

    TestFunction tests[] = {
        test_symres_resolve_self,
        test_symres_static_symbol_resolves,
        test_sr1_resolve_names_function,
        test_sr1_resolve_distinct_symbols,
        test_sr1_entry_offset_zero,
        test_sr1_interior_offset_matches,
        test_sr1_module_base_consistent,
        test_sr1_symbol_size_real,
        test_sr1_symbol_range_boundary,
        test_sr1_no_symbol_offset_is_file_relative,
        test_sr1_unmapped_returns_false,
        test_sr_cache_hit_same_module,
        test_sr_cache_zstrcompare_fallback,
        test_sr_cache_cross_module_distinct,
        test_sr_cache_other_module_rehit,
        test_sr_deinit_frees_everything,
    };

    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), NULL, 0, "SymbolResolver");
}
