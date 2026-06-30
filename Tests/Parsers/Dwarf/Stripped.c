// Integration test for the .debug_info function-name fallback.
//
// This source builds twice via meson: once as the unstripped test
// driver (Dwarf.Stripped) and once as a copy with the marker symbols
// stripped via `objcopy -N marker_alpha -N marker_beta`. The driver
// receives the stripped copy's path as argv[1].
//
// `-N name` deletes a named symbol from .symtab while leaving the
// rest of the binary (including .debug_info) intact. Combined with
// `static` linkage (which keeps the markers out of .dynsym), it
// reproduces the failure mode the cascade exists to handle: a
// function present in DWARF but absent from both ELF symbol tables.

#include <Misra.h>
#include <Misra/Parsers/Dwarf.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Parsers/Elf.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/File.h>
#include <Misra/Std/Log.h>
#include <Misra/Sys/SymbolResolver.h>

#include "../../Util/TestRunner.h"

// Named, static, noinline so the compiler can't fold or hide them.
// The `volatile` write defeats DCE without needing an external linkage.
static volatile u32                   sink;
static __attribute__((noinline)) void marker_alpha(void) {
    sink += 1;
}
static __attribute__((noinline)) void marker_beta(void) {
    sink += 2;
}

static Zstr stripped_path_arg = NULL;

// Open the stripped sibling and try to resolve `func_addr` (a runtime
// pointer into THIS process) to a name. The function's file-relative
// address is the same in both copies because objcopy -R only removes
// non-allocated sections.
static bool resolve_through_stripped(void (*func)(void), Zstr expect_name) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    // (1) Find this process's load base for /proc/self/exe.
    SymbolResolver res;
    if (!SymbolResolverInit(&res, base)) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }
    ResolvedSymbol r;
    if (!SymbolResolverResolve(&res, (void *)func, &r)) {
        SymbolResolverDeinit(&res);
        DefaultAllocatorDeinit(&alloc);
        return false;
    }
    u64 file_relative = (u64)func - r.module_base;
    SymbolResolverDeinit(&res);

    // (2) Open the stripped sibling.
    Elf stripped;
    if (!ElfOpen(&stripped, stripped_path_arg, base)) {
        LOG_ERROR("stripped binary not openable: {}", stripped_path_arg);
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    // (3) Symbol lookup against .symtab/.dynsym should miss for our
    //     static helpers (they're not exported, and .symtab is gone).
    const ElfSymbol *sym                    = ElfResolveAddress(&stripped, file_relative);
    bool             sym_missing_or_unnamed = (!sym) || (sym && (!sym->name || !sym->name[0]));
    if (!sym_missing_or_unnamed) {
        // The static marker shouldn't be in .dynsym; if we got a name,
        // either the toolchain exported it or the strip didn't take.
        WriteFmt("[INFO] unexpected ELF symbol hit for static helper: {}\n", sym->name);
    }

    // (4) DwarfFunctions on the stripped copy MUST find the name.
    DwarfFunctions fns;
    bool           built = DwarfFunctionsBuildFromElf(&fns, &stripped, base);
    bool           ok    = false;
    if (built && VecLen(&fns.entries) > 0) {
        const DwarfFunction *f = DwarfFunctionsResolve(&fns, file_relative);
        ok                     = f && f->name && ZstrFindSubstring(f->name, expect_name) != NULL;
        DwarfFunctionsDeinit(&fns);
    }

    ElfDeinit(&stripped);
    DefaultAllocatorDeinit(&alloc);
    return ok && sym_missing_or_unnamed;
}

bool test_stripped_resolves_marker_alpha(void) {
    return resolve_through_stripped(marker_alpha, "marker_alpha");
}
bool test_stripped_resolves_marker_beta(void) {
    return resolve_through_stripped(marker_beta, "marker_beta");
}

int main(int argc, char **argv) {
    if (argc < 2) {
        WriteFmt("usage: {} <path-to-stripped-binary>\n", argv[0]);
        return 1;
    }
    stripped_path_arg = argv[1];

    WriteFmt("[INFO] Stripped-binary resolve test against: {}\n\n", stripped_path_arg);

    TestFunction tests[] = {
        test_stripped_resolves_marker_alpha,
        test_stripped_resolves_marker_beta,
    };

    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), NULL, 0, "Dwarf.Stripped");
}
