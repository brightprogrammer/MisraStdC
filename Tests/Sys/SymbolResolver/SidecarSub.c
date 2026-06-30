/// file      : Tests/Std/SymbolResolver.SidecarSub.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// `.gnu_debuglink` discovery via the `{binary_dir}/.debug/{name}` search
/// location (path #3), exercised through the PUBLIC SymbolResolver API only.
/// meson splits this image's debug info into a sidecar that it places ONLY
/// under a `.debug/` subdirectory next to the stripped binary -- never
/// adjacent -- so the resolver must fall past path #2 ({dir}/{name}) and find
/// it under path #3. Resolving the stripped marker names it only when that
/// search arm fires, opens the sidecar, and the resolver consults its symtab.

#include <Misra.h>
#include <Misra/Std/Allocator/Debug.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Sys/SymbolResolver.h>

#include "../../Util/TestRunner.h"

// Static marker: stripped from the running image, preserved only in the
// `.debug/` sidecar's symtab. Padding gives it a sizeable body.
static __attribute__((noinline)) void subdir_marker(void) {
    __asm__ __volatile__("nop; nop; nop; nop; nop; nop; nop; nop" ::
                             : "memory");
    __asm__ __volatile__("nop; nop; nop; nop; nop; nop; nop; nop" ::
                             : "memory");
}

// The marker survives only in the sidecar under `.debug/`, so naming it proves
// the path-#3 search arm: {dir} is prefixed and `/.debug/{name}` appended,
// the sidecar opens, and its symtab is consulted. Dropping the dirname prefix,
// the existence probe, or the match collapses the name back to NULL.
bool test_sidecarsub_path3_names_marker(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    SymbolResolver   res;
    if (!SymbolResolverInit(&res, ALLOCATOR_OF(&alloc))) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    u64            addr = (u64)(void *)&subdir_marker;
    ResolvedSymbol r;
    bool           ok = SymbolResolverResolve(&res, (void *)addr, &r);
    ok                = ok && r.symbol_name && ZstrFindSubstring(r.symbol_name, "subdir_marker") != NULL;
    ok                = ok && r.offset == 0;
    ok                = ok && (r.module_base + r.symbol_value + r.offset) == addr;

    SymbolResolverDeinit(&res);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Teardown frees the opened sidecar and the transient search path used to
// find it. The path-#3 success arm's StrDeinit is what returns the live count
// to baseline; dropping it leaks and the count stays high.
bool test_sidecarsub_teardown_frees(void) {
    DebugAllocator alloc    = DebugAllocatorInit();
    size           baseline = DebugAllocatorLiveCount(&alloc);

    SymbolResolver res;
    if (!SymbolResolverInit(&res, ALLOCATOR_OF(&alloc))) {
        DebugAllocatorDeinit(&alloc);
        return false;
    }

    ResolvedSymbol r;
    bool           ok = SymbolResolverResolve(&res, (void *)&subdir_marker, &r);
    ok                = ok && r.symbol_name && ZstrFindSubstring(r.symbol_name, "subdir_marker") != NULL;
    ok                = ok && DebugAllocatorLiveCount(&alloc) > baseline;

    SymbolResolverDeinit(&res);
    ok = ok && DebugAllocatorLiveCount(&alloc) == baseline;

    DebugAllocatorDeinit(&alloc);
    return ok;
}

int main(void) {
    WriteFmt("[INFO] Starting SymbolResolver.SidecarSub tests\n\n");

    TestFunction tests[] = {
        test_sidecarsub_path3_names_marker,
        test_sidecarsub_teardown_frees,
    };

    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), NULL, 0, "SymbolResolver.SidecarSub");
}
