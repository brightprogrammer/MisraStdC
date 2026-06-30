/// file      : Tests/Std/SymbolResolver.SidecarDwarf.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// `.debug_info` function-name fallback sourced from a SIDECAR, exercised
/// through the PUBLIC SymbolResolver API only. meson builds this image with
/// DWARF-4, splits the debug info into a `.debug/` sidecar, deletes the marker
/// from the SIDECAR's `.symtab` (so the sidecar cannot name it via symbols),
/// then `--strip-all`s the running image (no `.symtab`, no `.debug_*`). The
/// marker therefore survives ONLY in the sidecar's `.debug_info`: resolving it
/// names it solely through the sidecar DWARF-function table, which forces that
/// table to be built and, on teardown, freed.

#include <Misra.h>
#include <Misra/Std/Allocator/Debug.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Sys/SymbolResolver.h>

#include "../../Util/TestRunner.h"

static volatile u32 scd_sink;

static __attribute__((noinline)) void scd_marker(void) {
    scd_sink += 1;
    __asm__ __volatile__("nop; nop; nop; nop; nop; nop; nop; nop" ::
                             : "memory");
}

// The marker is gone from both images' symbol tables and from the running
// image's DWARF, surviving only in the sidecar's `.debug_info`. Naming it
// proves the full chain: debuglink discovery -> sidecar open -> sidecar
// DWARF-function table built -> resolved.
bool test_sidecardwarf_names_via_sidecar_functions(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    SymbolResolver   res;
    if (!SymbolResolverInit(&res, ALLOCATOR_OF(&alloc))) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    ResolvedSymbol r;
    bool           ok = SymbolResolverResolve(&res, (void *)&scd_marker, &r);
    ok                = ok && r.symbol_name && ZstrFindSubstring(r.symbol_name, "scd_marker") != NULL;

    SymbolResolverDeinit(&res);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Teardown after a resolve that built the SIDECAR DWARF-function table frees
// it. Dropping the sidecar-functions Deinit arm leaks; the live count fails to
// return to baseline.
bool test_sidecardwarf_teardown_frees_sidecar_functions(void) {
    DebugAllocator alloc    = DebugAllocatorInit();
    size           baseline = DebugAllocatorLiveCount(&alloc);

    SymbolResolver res;
    if (!SymbolResolverInit(&res, ALLOCATOR_OF(&alloc))) {
        DebugAllocatorDeinit(&alloc);
        return false;
    }

    ResolvedSymbol r;
    bool           ok = SymbolResolverResolve(&res, (void *)&scd_marker, &r);
    ok                = ok && r.symbol_name && ZstrFindSubstring(r.symbol_name, "scd_marker") != NULL;
    ok                = ok && DebugAllocatorLiveCount(&alloc) > baseline;

    SymbolResolverDeinit(&res);
    ok = ok && DebugAllocatorLiveCount(&alloc) == baseline;

    DebugAllocatorDeinit(&alloc);
    return ok;
}

int main(void) {
    WriteFmt("[INFO] Starting SymbolResolver.SidecarDwarf tests\n\n");

    TestFunction tests[] = {
        test_sidecardwarf_names_via_sidecar_functions,
        test_sidecardwarf_teardown_frees_sidecar_functions,
    };

    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), NULL, 0, "SymbolResolver.SidecarDwarf");
}
