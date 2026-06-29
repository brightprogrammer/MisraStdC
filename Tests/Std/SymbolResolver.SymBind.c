/// file      : Tests/Std/SymbolResolver.SymBind.c
/// author    : adversarial-review
/// This is free and unencumbered software released into the public domain.
///
/// Adversarial disproof harness for the proposed SYMRES-SYMBIND-GLOBAL
/// ledger entry (lines 324:56 / 328:82, the `s->bind == ELF_SYMBOL_BIND_GLOBAL`
/// tie-break that makes a GLOBAL symbol win over a LOCAL one aliasing the SAME
/// address). The agent bucketed it B (strategy). This test pins the
/// caller-observable outcome: when a GLOBAL and a LOCAL symbol share an
/// address, Resolve must report the GLOBAL (exported) name. Built unstripped so
/// both symbols survive in .symtab. PUBLIC API only.

#include <Misra.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Sys/SymbolResolver.h>

#include "../Util/TestRunner.h"

// Two SIZED (FUNC) symbols at the exact same address: one GLOBAL, one LOCAL.
// Exercises the sized-range tie-break at line 328.
__asm__(
    ".pushsection .text.sr_bind_sized, \"ax\", @progbits\n"
    ".globl sr_bind_global_sized\n"
    ".type sr_bind_global_sized, @function\n"
    ".type sr_bind_local_sized, @function\n"
    "sr_bind_global_sized:\n"
    "sr_bind_local_sized:\n"
    "    nop; nop; nop; nop; nop; nop; nop; nop\n"
    "    ret\n"
    ".size sr_bind_global_sized, 9\n"
    ".size sr_bind_local_sized, 9\n"
    ".popsection\n"
);
extern void sr_bind_global_sized(void);

// Two size-0 NOTYPE symbols at the exact same address: one GLOBAL, one LOCAL.
// Exercises the size-0 exact-match tie-break at line 324.
__asm__(
    ".pushsection .text.sr_bind_zero, \"ax\", @progbits\n"
    ".globl sr_bind_global_zero\n"
    "sr_bind_global_zero:\n"
    "sr_bind_local_zero:\n"
    "    ret\n"
    ".popsection\n"
);
extern void sr_bind_global_zero(void);

// Sized range tie-break: the GLOBAL name must come back, not the LOCAL alias.
bool test_symbind_sized_prefers_global(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    SymbolResolver   res;
    if (!SymbolResolverInit(&res, ALLOCATOR_OF(&alloc))) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    ResolvedSymbol r;
    bool           ok = SymbolResolverResolve(&res, (void *)&sr_bind_global_sized, &r);
    ok                = ok && r.symbol_name != NULL;
    ok                = ok && ZstrFindSubstring(r.symbol_name, "sr_bind_global_sized") != NULL;
    ok                = ok && ZstrFindSubstring(r.symbol_name, "sr_bind_local_sized") == NULL;

    SymbolResolverDeinit(&res);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Size-0 exact tie-break: the GLOBAL name must come back, not the LOCAL alias.
bool test_symbind_zero_prefers_global(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    SymbolResolver   res;
    if (!SymbolResolverInit(&res, ALLOCATOR_OF(&alloc))) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    ResolvedSymbol r;
    bool           ok = SymbolResolverResolve(&res, (void *)&sr_bind_global_zero, &r);
    ok                = ok && r.symbol_name != NULL;
    ok                = ok && ZstrFindSubstring(r.symbol_name, "sr_bind_global_zero") != NULL;
    ok                = ok && ZstrFindSubstring(r.symbol_name, "sr_bind_local_zero") == NULL;

    SymbolResolverDeinit(&res);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

int main(void) {
    WriteFmt("[INFO] Starting SymbolResolver.SymBind tests\n\n");

    TestFunction tests[] = {
        test_symbind_sized_prefers_global,
        test_symbind_zero_prefers_global,
    };

    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), NULL, 0, "SymbolResolver.SymBind");
}
