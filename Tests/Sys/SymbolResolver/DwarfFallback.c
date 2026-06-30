/// file      : Tests/Std/SymbolResolver.DwarfFallback.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// `.debug_info` function-name + `.debug_line` fallback, exercised through the
/// PUBLIC SymbolResolver API only. meson builds this source with DWARF-4 debug
/// info, then `objcopy -N df_marker_*` deletes the marker symbols from
/// `.symtab` while leaving `.debug_info`/`.debug_line` intact. The STRIPPED
/// copy is what runs: resolving a marker's own address misses both `.symtab`
/// and `.dynsym` (the markers are `static`), so a name only appears when the
/// resolver falls through to the DWARF subprogram table, and a source line
/// only appears when it consults the DWARF line program.

#include <Misra.h>
#include <Misra/Std/Allocator/Debug.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Sys/SymbolResolver.h>

#include "../../Util/TestRunner.h"

static volatile u32 df_sink;

// Two static, noinline markers on distinct source lines. The volatile write
// defeats DCE without external linkage, keeping them out of `.dynsym`; the
// padding gives each a real, sizeable body.
static __attribute__((noinline)) void df_marker_a(void) {
    df_sink += 1;
    __asm__ __volatile__("nop; nop; nop; nop; nop; nop; nop; nop" ::
                             : "memory");
}

static __attribute__((noinline)) void df_marker_b(void) {
    df_sink += 2;
    __asm__ __volatile__("nop; nop; nop; nop; nop; nop; nop; nop" ::
                             : "memory");
}

// Resolving a marker whose symtab/dynsym entries were stripped still names it
// (via the DWARF subprogram DIE), reports offset 0 at the entry, a genuine
// size, and a load-base reconstruction that holds. Pins the DWARF-function
// fallback's name/value/size/offset assignments.
bool test_dwarffallback_names_and_arith(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    SymbolResolver   res;
    if (!SymbolResolverInit(&res, ALLOCATOR_OF(&alloc))) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    u64            addr = (u64)(void *)&df_marker_a;
    ResolvedSymbol r;
    bool           ok = SymbolResolverResolve(&res, (void *)addr, &r);
    ok                = ok && r.symbol_name && ZstrFindSubstring(r.symbol_name, "df_marker_a") != NULL;
    // Entry address: offset 0, reconstruction holds against the DWARF-sourced
    // low_pc, and the body has a real (small, non-zero) size.
    ok = ok && r.offset == 0;
    ok = ok && (r.module_base + r.symbol_value + r.offset) == addr;
    ok = ok && r.symbol_size > 0;
    // Pin the reported size to the real body extent: its last byte
    // (base + size - 1) must still name the marker at offset size-1. A
    // wrong size -- a constant, or low_pc+high_pc instead of the difference
    // -- pushes that probe outside the body and the marker name disappears.
    if (ok) {
        ResolvedSymbol rlast;
        ok = SymbolResolverResolve(&res, (void *)(addr + r.symbol_size - 1), &rlast);
        ok = ok && rlast.symbol_name && ZstrFindSubstring(rlast.symbol_name, "df_marker_a") != NULL;
        ok = ok && rlast.symbol_value == r.symbol_value;
        ok = ok && rlast.offset == r.symbol_size - 1;
    }

    SymbolResolverDeinit(&res);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// An interior address reports a non-zero offset that stays inside the body and
// preserves the same symbol value: pins offset = file_relative - low_pc (sub,
// not add) for the DWARF-function arm.
bool test_dwarffallback_interior_offset(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    SymbolResolver   res;
    if (!SymbolResolverInit(&res, ALLOCATOR_OF(&alloc))) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    u64            base = (u64)(void *)&df_marker_a;
    ResolvedSymbol r0, rk;
    bool           ok = SymbolResolverResolve(&res, (void *)base, &r0);
    ok                = ok && r0.symbol_name && r0.symbol_size > 4;
    if (ok) {
        ok = SymbolResolverResolve(&res, (void *)(base + 4), &rk);
        ok = ok && rk.symbol_name && rk.symbol_value == r0.symbol_value;
        ok = ok && rk.offset == 4;
    }

    SymbolResolverDeinit(&res);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// `.debug_line` fallback: each marker reports a source file in this TU and a
// 1-based line, and the two markers' lines differ (they are on distinct source
// lines). Pins the source_line passthrough against a constant. Column info is
// disabled for this fixture, so the reported column is 0 (faithful passthrough
// of "unknown").
bool test_dwarffallback_source_lines(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    SymbolResolver   res;
    if (!SymbolResolverInit(&res, ALLOCATOR_OF(&alloc))) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    ResolvedSymbol ra, rb;
    bool           ok = SymbolResolverResolve(&res, (void *)&df_marker_a, &ra);
    ok                = ok && SymbolResolverResolve(&res, (void *)&df_marker_b, &rb);
    ok                = ok && ra.source_file && ZstrFindSubstring(ra.source_file, "DwarfFallback.c") != NULL;
    ok                = ok && rb.source_file && ZstrFindSubstring(rb.source_file, "DwarfFallback.c") != NULL;
    ok                = ok && ra.source_line > 0 && rb.source_line > 0;
    ok                = ok && ra.source_line != rb.source_line;
    // Built with -gno-column-info: the line program carries no column, so the
    // resolver passes through 0.
    ok = ok && ra.source_column == 0;

    SymbolResolverDeinit(&res);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Teardown after a resolve that built the DWARF function + line tables frees
// them. Dropping either Deinit arm leaks; the live count fails to return.
bool test_dwarffallback_teardown_frees(void) {
    DebugAllocator alloc    = DebugAllocatorInit();
    size           baseline = DebugAllocatorLiveCount(&alloc);

    SymbolResolver res;
    if (!SymbolResolverInit(&res, ALLOCATOR_OF(&alloc))) {
        DebugAllocatorDeinit(&alloc);
        return false;
    }

    ResolvedSymbol r;
    bool           ok = SymbolResolverResolve(&res, (void *)&df_marker_a, &r);
    // The name came from DWARF, proving the function table was built (and the
    // line table alongside it).
    ok = ok && r.symbol_name && ZstrFindSubstring(r.symbol_name, "df_marker_a") != NULL;
    ok = ok && DebugAllocatorLiveCount(&alloc) > baseline;

    SymbolResolverDeinit(&res);
    ok = ok && DebugAllocatorLiveCount(&alloc) == baseline;

    DebugAllocatorDeinit(&alloc);
    return ok;
}

int main(void) {
    WriteFmt("[INFO] Starting SymbolResolver.DwarfFallback tests\n\n");

    TestFunction tests[] = {
        test_dwarffallback_names_and_arith,
        test_dwarffallback_interior_offset,
        test_dwarffallback_source_lines,
        test_dwarffallback_teardown_frees,
    };

    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), NULL, 0, "SymbolResolver.DwarfFallback");
}
