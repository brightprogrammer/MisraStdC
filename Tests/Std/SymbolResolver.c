#include <Misra.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Zstr.h>
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

int main(void) {
    WriteFmt("[INFO] Starting SymbolResolver tests\n\n");

    TestFunction tests[] = {
        test_symres_resolve_self,
        test_symres_static_symbol_resolves,
    };

    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), NULL, 0, "SymbolResolver");
}
