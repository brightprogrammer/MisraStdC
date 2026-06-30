/// file      : Tests/Std/SymbolResolver.Sidecar.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Sidecar / `.gnu_debuglink` discovery, exercised through the PUBLIC
/// SymbolResolver API only. The test binary itself is the fixture: meson
/// strips it (`--strip-all`) and points a `.gnu_debuglink` at an adjacent
/// `SymbolResolver.Sidecar.debug` that keeps the full `.symtab`. The
/// running image therefore has NO symbol for `sidecar_marker`, so resolving
/// its address only yields a name when `try_open_sidecar` finds + matches
/// the debuglink sidecar and the resolver searches the sidecar's tables.

#include <Misra.h>
#include <Misra/Std/Allocator/Debug.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Sys/SymbolResolver.h>

#include "../../Util/TestRunner.h"

// A static, non-exported marker. Stripped out of the running image but
// preserved in the adjacent sidecar's `.symtab`. The padding gives it a
// real, sizeable body so the resolved address lands strictly inside it.
static __attribute__((noinline)) void sidecar_marker(void) {
    __asm__ __volatile__("nop; nop; nop; nop; nop; nop; nop; nop" ::
                             : "memory");
    __asm__ __volatile__("nop; nop; nop; nop; nop; nop; nop; nop" ::
                             : "memory");
}

// The debuglink sidecar adjacent to the stripped binary is the ONLY place
// `sidecar_marker` survives, so naming it proves the full public chain:
// debuglink search (path #2) -> ElfOpen -> sidecar_matches -> the sidecar's
// `.symtab` is consulted for the name. A broken path build, a rejected
// match, or a skipped sidecar lookup all collapse the name back to NULL.
bool test_sidecar_debuglink_names_marker(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    SymbolResolver   res;
    if (!SymbolResolverInit(&res, ALLOCATOR_OF(&alloc))) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    ResolvedSymbol r;
    bool           ok = SymbolResolverResolve(&res, (void *)&sidecar_marker, &r);
    ok                = ok && r.module_path && r.module_path[0] != '\0';
    ok                = ok && r.symbol_name && ZstrFindSubstring(r.symbol_name, "sidecar_marker") != NULL;
    // Entry address -> offset zero, reconstruction holds against the
    // sidecar-sourced symbol value, and the sidecar's `.symtab` carries a
    // real size for the padded body.
    ok = ok && r.offset == 0;
    ok = ok && r.symbol_size > 0;
    ok = ok && (r.module_base + r.symbol_value + r.offset) == (u64)(void *)&sidecar_marker;
    // `.debug_line` lives only in the sidecar too: a source file/line for
    // this TU proves the sidecar DWARF was consulted.
    ok = ok && r.source_file && ZstrFindSubstring(r.source_file, "Sidecar.c") != NULL;
    ok = ok && r.source_line > 0;

    SymbolResolverDeinit(&res);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Teardown frees the opened sidecar ELF (and the transient search path).
// A populated cache with a discovered sidecar sits above baseline; correct
// teardown (sidecar ElfDeinit + main ElfDeinit + cache free, plus the
// success-path StrDeinit inside try_open_sidecar) returns to it. Dropping
// the sidecar ElfDeinit arm or the path StrDeinit leaks and the live count
// stays high.
bool test_sidecar_teardown_frees_sidecar(void) {
    DebugAllocator alloc    = DebugAllocatorInit();
    size           baseline = DebugAllocatorLiveCount(&alloc);

    SymbolResolver res;
    if (!SymbolResolverInit(&res, ALLOCATOR_OF(&alloc))) {
        DebugAllocatorDeinit(&alloc);
        return false;
    }

    ResolvedSymbol r;
    bool           ok = SymbolResolverResolve(&res, (void *)&sidecar_marker, &r);
    // The marker is named only via the opened sidecar, proving the cache
    // entry actually carries one.
    ok = ok && r.symbol_name && ZstrFindSubstring(r.symbol_name, "sidecar_marker") != NULL;
    ok = ok && DebugAllocatorLiveCount(&alloc) > baseline;

    SymbolResolverDeinit(&res);
    ok = ok && DebugAllocatorLiveCount(&alloc) == baseline;

    DebugAllocatorDeinit(&alloc);
    return ok;
}

int main(void) {
    WriteFmt("[INFO] Starting SymbolResolver.Sidecar tests\n\n");

    TestFunction tests[] = {
        test_sidecar_debuglink_names_marker,
        test_sidecar_teardown_frees_sidecar,
    };

    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), NULL, 0, "SymbolResolver.Sidecar");
}
