#include <Misra.h>
#include <Misra/Parsers/Elf.h>
#include <Misra/Std/Allocator/Default.h>

#include "../Util/TestRunner.h"

// Open this test binary itself via /proc/self/exe and verify we can
// parse it: ELF64 header, at least one section, at least a non-empty
// symbol table (debug builds aren't stripped).
bool test_elf_self_exe_parse(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Elf              elf;

    bool opened = ElfOpen(&elf, "/proc/self/exe", ALLOCATOR_OF(&alloc));
    if (!opened) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    bool ok = elf.header.elf_class == ELF_CLASS_64 && elf.header.data == ELF_DATA_LSB &&
              (elf.header.type == ELF_TYPE_EXEC || elf.header.type == ELF_TYPE_DYN) && VecLen(&elf.sections) > 0;

    // A test binary built with sanitizers should have both static and
    // dynamic symbol tables.
    ok = ok && VecLen(&elf.symbols) > 0;

    ElfDeinit(&elf);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Find a section by name and verify it's the right shape.
bool test_elf_find_text_section(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Elf              elf;

    if (!ElfOpen(&elf, "/proc/self/exe", ALLOCATOR_OF(&alloc))) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    const ElfSection *text = ElfFindSection(&elf, ".text");
    bool              ok   = text != NULL && text->size > 0 && (text->flags & 0x4); // SHF_EXECINSTR = 0x4

    ElfDeinit(&elf);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// We can find at least one named function symbol pointing at the .text
// section. Don't pin to a specific name — the test binary's main is
// the most likely candidate but a static helper would also do.
// GCC and clang both emit `.note.gnu.build-id` by default. Confirm
// we surface the bytes — this is what lets SymbolResolver look up
// /usr/lib/debug/.build-id/aa/bbbb...debug sidecars for stripped
// production binaries.
bool test_elf_build_id_present(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Elf              elf;
    if (!ElfOpen(&elf, "/proc/self/exe", ALLOCATOR_OF(&alloc))) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }
    bool ok = elf.build_id != NULL && elf.build_id_size > 0 && elf.build_id_size <= 64;
    ElfDeinit(&elf);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

bool test_elf_some_function_symbol(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Elf              elf;

    if (!ElfOpen(&elf, "/proc/self/exe", ALLOCATOR_OF(&alloc))) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    bool ok = false;
    for (u64 i = 0; i < VecLen(&elf.symbols); ++i) {
        const ElfSymbol *s = VecPtrAt(&elf.symbols, i);
        if (s->type == ELF_SYMBOL_TYPE_FUNC && s->size > 0 && s->name && s->name[0] != '\0') {
            ok = true;
            break;
        }
    }

    ElfDeinit(&elf);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

int main(void) {
    WriteFmt("[INFO] Starting Elf tests\n\n");

    TestFunction tests[] = {
        test_elf_self_exe_parse,
        test_elf_find_text_section,
        test_elf_build_id_present,
        test_elf_some_function_symbol,
    };

    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), NULL, 0, "Elf");
}
