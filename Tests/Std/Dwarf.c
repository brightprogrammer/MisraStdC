#include <Misra.h>
#include <Misra/Parsers/Dwarf.h>
#include <Misra/Parsers/Elf.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Sys/SymbolResolver.h>

#include <stdint.h>
#include <string.h>

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
        if (e && e->file && strstr(e->file, "Dwarf.c") != NULL && e->line > 0) {
            ok = true;
        }
        DwarfLinesDeinit(&lines);
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
    };

    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), NULL, 0, "Dwarf");
}
