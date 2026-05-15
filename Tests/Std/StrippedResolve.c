// Integration test for the .debug_info function-name fallback.
//
// This source builds twice via meson: once as the unstripped test
// driver (Dwarf.Stripped) and once as a stripped copy of that same
// binary (Dwarf.Stripped.stripped). The driver receives the stripped
// copy's path as argv[1].
//
// `objcopy -R .symtab -R .strtab` removes both ELF symbol tables but
// leaves `.dynsym`, `.text`, and the `.debug_*` sections. Static
// functions don't appear in `.dynsym`, so an ELF symbol lookup for our
// `marker_*` helpers necessarily misses on the stripped copy. The
// DwarfFunctions cascade must therefore be doing the resolution.

#include <Misra.h>
#include <Misra/Parsers/Dwarf.h>
#include <Misra/Parsers/Elf.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/File.h>
#include <Misra/Std/Log.h>
#include <Misra/Sys/SymbolResolver.h>

#include <stdint.h>

#include "../Util/TestRunner.h"

// Named, static, noinline so the compiler can't fold or hide them.
// The `volatile` write defeats DCE without needing an external linkage.
static volatile u32                   sink;
static __attribute__((noinline)) void marker_alpha(void) {
    sink += 1;
}
static __attribute__((noinline)) void marker_beta(void) {
    sink += 2;
}

static const char *stripped_path_arg = NULL;

// Open the stripped sibling and try to resolve `func_addr` (a runtime
// pointer into THIS process) to a name. The function's file-relative
// address is the same in both copies because objcopy -R only removes
// non-allocated sections.
static bool resolve_through_stripped(void (*func)(void), const char *expect_name) {
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
    u64 file_relative = (u64)(uintptr_t)func - r.module_base;
    SymbolResolverDeinit(&res);

    // (2) Open the stripped sibling.
    ElfFile stripped;
    if (!ElfFileOpen(&stripped, stripped_path_arg, base)) {
        LOG_ERROR("stripped binary not openable: {}", stripped_path_arg);
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    // (3) Symbol lookup against .symtab/.dynsym should miss for our
    //     static helpers (they're not exported, and .symtab is gone).
    const ElfSymbol *sym                    = ElfFileResolveAddress(&stripped, file_relative);
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
    if (built && fns.entries.length > 0) {
        const DwarfFunction *f = DwarfFunctionsResolve(&fns, file_relative);
        ok                     = f && f->name && ZstrFindSubstring(f->name, expect_name) != NULL;
        DwarfFunctionsDeinit(&fns);
    }

    ElfFileDeinit(&stripped);
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
