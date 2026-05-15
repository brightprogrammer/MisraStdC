#include <Misra.h>
#include <Misra/Parsers/Dwarf.h>
#include <Misra/Parsers/Elf.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Sys/SymbolResolver.h>

#include <stdint.h>

#include "../Util/TestRunner.h"

static __attribute__((noinline)) void dwarf_marker_helper(void) {
    __asm__ __volatile__("" ::
                             : "memory");
}

// Parsing /proc/self/exe's .debug_line should produce at least some
// entries when the binary is built with debug info (meson default).
bool test_dwarf_lines_load_self(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    ElfFile elf;
    if (!ElfFileOpen(&elf, "/proc/self/exe", ALLOCATOR_OF(&alloc))) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    DwarfLines lines;
    bool       built = DwarfLinesBuildFromElf(&lines, &elf, ALLOCATOR_OF(&alloc));
    bool       ok    = built && lines.entries.length > 0;

    if (built)
        DwarfLinesDeinit(&lines);
    ElfFileDeinit(&elf);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Resolve a runtime address back through the chain:
//   runtime IP -> SymbolResolver -> {module, symbol, file_relative_va}
//   file_relative_va -> DwarfLinesResolve -> {file, line}
// For dwarf_marker_helper, "Dwarf.c" should be in the file path.
bool test_dwarf_resolves_helper_to_source_file(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    SymbolResolver res;
    if (!SymbolResolverInit(&res, base)) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    ResolvedSymbol r;
    if (!SymbolResolverResolve(&res, (void *)&dwarf_marker_helper, &r)) {
        SymbolResolverDeinit(&res);
        DefaultAllocatorDeinit(&alloc);
        return false;
    }
    u64 file_relative = (u64)(uintptr_t)&dwarf_marker_helper - r.module_base;

    ElfFile elf;
    if (!ElfFileOpen(&elf, r.module_path, base)) {
        SymbolResolverDeinit(&res);
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    DwarfLines lines;
    bool       built = DwarfLinesBuildFromElf(&lines, &elf, base);
    bool       ok    = false;
    if (built) {
        const DwarfLineEntry *e = DwarfLinesResolve(&lines, file_relative);
        if (e && e->file && ZstrFindSubstring(e->file, "Dwarf.c") != NULL && e->line > 0) {
            ok = true;
        }
        DwarfLinesDeinit(&lines);
    }

    ElfFileDeinit(&elf);
    SymbolResolverDeinit(&res);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Walk /proc/self/exe's .eh_frame, then look up an FDE for the
// address of a known function in this binary. GCC and clang emit
// .eh_frame for every function unless explicitly told not to, so
// we expect a hit.
bool test_dwarf_cfi_finds_fde_for_self(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    SymbolResolver res;
    if (!SymbolResolverInit(&res, base)) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    ResolvedSymbol r;
    if (!SymbolResolverResolve(&res, (void *)&dwarf_marker_helper, &r)) {
        SymbolResolverDeinit(&res);
        DefaultAllocatorDeinit(&alloc);
        return false;
    }
    u64 file_relative = (u64)(uintptr_t)&dwarf_marker_helper - r.module_base;

    ElfFile elf;
    if (!ElfFileOpen(&elf, r.module_path, base)) {
        SymbolResolverDeinit(&res);
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    DwarfCfi cfi;
    bool     built = DwarfCfiBuildFromElf(&cfi, &elf, base);
    bool     ok    = false;
    if (built) {
        const DwarfFde *fde = DwarfCfiFindFde(&cfi, file_relative);
        ok                  = fde != NULL && fde->pc_range > 0 && file_relative >= fde->pc_begin &&
             file_relative < fde->pc_begin + fde->pc_range;

        // Run the CFI VM and verify we get a usable row: on x86-64 the
        // CFA is always `register + offset` (typically RSP + N), and the
        // return-address pseudo-register (DWARF reg 16) has a saved
        // location at some offset from CFA.
        if (ok) {
            DwarfUnwindRow row;
            ok = DwarfCfiBuildRow(&cfi, fde, file_relative, &row);
            ok = ok && row.cfa.kind == DWARF_CFA_RULE_REG_OFFSET;
            ok = ok && row.regs[row.return_address_register].kind == DWARF_REG_RULE_OFFSET;
        }
        DwarfCfiDeinit(&cfi);
    }

    ElfFileDeinit(&elf);
    SymbolResolverDeinit(&res);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

int main(void) {
    WriteFmt("[INFO] Starting Dwarf tests\n\n");

    TestFunction tests[] = {
        test_dwarf_lines_load_self,
        test_dwarf_resolves_helper_to_source_file,
        test_dwarf_cfi_finds_fde_for_self,
    };

    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), NULL, 0, "Dwarf");
}
