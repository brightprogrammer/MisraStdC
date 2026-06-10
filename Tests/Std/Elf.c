#include <Misra.h>
#include <Misra/Parsers/Elf.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Memory.h>

#include "../Util/TestRunner.h"

// Little-endian writers into a flat buffer.
static void wr_u16(u8 *p, u16 v) {
    p[0] = (u8)(v & 0xff);
    p[1] = (u8)(v >> 8);
}
static void wr_u32(u8 *p, u32 v) {
    p[0] = (u8)(v & 0xff);
    p[1] = (u8)(v >> 8);
    p[2] = (u8)(v >> 16);
    p[3] = (u8)(v >> 24);
}
static void wr_u64(u8 *p, u64 v) {
    for (int i = 0; i < 8; ++i)
        p[i] = (u8)((v >> (i * 8)) & 0xff);
}

// ---------------------------------------------------------------------------
// Synthetic ELF64 LSB executable fixture. Just enough to exercise the
// documented decode contract: header fields, a named section, and a
// function symbol with a known value/size. Layout (offsets absolute):
//   0x00  ELF header (64 bytes)
//   0x40  .shstrtab content (33 bytes)
//   0x61  .strtab content   (9 bytes)
//   0x70  symbol table      (2 * 24 bytes)
//   0xA0  .text data        (0x40 bytes)
//   0xE0  section header table (5 * 64 bytes)
// ---------------------------------------------------------------------------

enum {
    ELF_FIXTURE_SIZE = 0x220,
    EHDR_SIZE        = 64,
    SHDR_SIZE        = 64,
    SYM_SIZE         = 24,

    SHSTR_OFF  = 0x40,
    SHSTR_SIZE = 33,
    STRTAB_OFF = 0x61,
    STRTAB_SZ  = 9,
    SYMTAB_OFF = 0x70,
    SYMTAB_SZ  = 2 * SYM_SIZE,
    TEXT_OFF   = 0xA0,
    TEXT_SIZE  = 0x40,
    SHT_OFF    = 0xE0,
    N_SECTIONS = 5,

    // name offsets within .shstrtab
    NAME_SHSTRTAB = 1,
    NAME_TEXT     = 11,
    NAME_SYMTAB   = 17,
    NAME_STRTAB   = 25,

    // section header table indices
    SEC_NULL     = 0,
    SEC_SHSTRTAB = 1,
    SEC_TEXT     = 2,
    SEC_SYMTAB   = 3,
    SEC_STRTAB   = 4,

    TEXT_VADDR  = 0x401000,
    FUNC_VADDR  = 0x401010,
    FUNC_SIZE   = 0x20,
    ENTRY_VADDR = 0x401000,
    EM_X86_64   = 62,

    // Program-header fields. The parser only records them (it does not
    // walk program headers), so any in-file value is fine; we pick
    // distinctive non-zero values so the decode is observable.
    PHOFF_VAL = 0x40,
    PHNUM_VAL = 3,
};

// Write a 64-byte ELF64 LE section header at p.
static void wr_shdr(u8 *p, u32 name, u32 type, u64 flags, u64 addr, u64 offset, u64 size, u32 link, u64 entsize) {
    wr_u32(&p[0], name);
    wr_u32(&p[4], type);
    wr_u64(&p[8], flags);
    wr_u64(&p[16], addr);
    wr_u64(&p[24], offset);
    wr_u64(&p[32], size);
    wr_u32(&p[40], link);
    wr_u32(&p[44], 0); // info
    wr_u64(&p[48], 1); // addralign
    wr_u64(&p[56], entsize);
}

static u8 elf_blob[ELF_FIXTURE_SIZE];

static void build_elf_blob(void) {
    MemSet(elf_blob, 0, sizeof(elf_blob));

    // --- ELF header -------------------------------------------------------
    elf_blob[0] = 0x7f;
    elf_blob[1] = 'E';
    elf_blob[2] = 'L';
    elf_blob[3] = 'F';
    elf_blob[4] = (u8)ELF_CLASS_64;    // EI_CLASS
    elf_blob[5] = (u8)ELF_DATA_LSB;    // EI_DATA
    elf_blob[6] = 1;                   // EI_VERSION

    u8 *h = &elf_blob[16];             // after e_ident
    wr_u16(&h[0], (u16)ELF_TYPE_EXEC); // e_type
    wr_u16(&h[2], EM_X86_64);          // e_machine
    wr_u32(&h[4], 1);                  // e_version
    wr_u64(&h[8], ENTRY_VADDR);        // e_entry
    wr_u64(&h[16], PHOFF_VAL);         // e_phoff
    wr_u64(&h[24], SHT_OFF);           // e_shoff
    wr_u32(&h[32], 0);                 // e_flags
    wr_u16(&h[36], EHDR_SIZE);         // e_ehsize
    wr_u16(&h[38], 0);                 // e_phentsize
    wr_u16(&h[40], PHNUM_VAL);         // e_phnum
    wr_u16(&h[42], SHDR_SIZE);         // e_shentsize
    wr_u16(&h[44], N_SECTIONS);        // e_shnum
    wr_u16(&h[46], SEC_SHSTRTAB);      // e_shstrndx

    // --- .shstrtab content ------------------------------------------------
    const char shstr[SHSTR_SIZE] = "\0.shstrtab\0.text\0.symtab\0.strtab\0";
    MemCopy(&elf_blob[SHSTR_OFF], shstr, SHSTR_SIZE);

    // --- .strtab content --------------------------------------------------
    const char strtab[STRTAB_SZ] = "\0my_func\0";
    MemCopy(&elf_blob[STRTAB_OFF], strtab, STRTAB_SZ);

    // --- symbol table: [0]=null, [1]=my_func (FUNC, GLOBAL) ---------------
    u8 *s1 = &elf_blob[SYMTAB_OFF + SYM_SIZE];
    wr_u32(&s1[0], 1);                                                  // st_name -> "my_func"
    s1[4] = (u8)((ELF_SYMBOL_BIND_GLOBAL << 4) | ELF_SYMBOL_TYPE_FUNC); // st_info
    s1[5] = 0;                                                          // st_other
    wr_u16(&s1[6], SEC_TEXT);                                           // st_shndx
    wr_u64(&s1[8], FUNC_VADDR);                                         // st_value
    wr_u64(&s1[16], FUNC_SIZE);                                         // st_size

    // --- section header table ---------------------------------------------
    u8 *sht = &elf_blob[SHT_OFF];
    wr_shdr(&sht[SEC_NULL * SHDR_SIZE], 0, ELF_SECTION_TYPE_NULL, 0, 0, 0, 0, 0, 0);
    wr_shdr(&sht[SEC_SHSTRTAB * SHDR_SIZE], NAME_SHSTRTAB, ELF_SECTION_TYPE_STRTAB, 0, 0, SHSTR_OFF, SHSTR_SIZE, 0, 0);
    wr_shdr(
        &sht[SEC_TEXT * SHDR_SIZE],
        NAME_TEXT,
        ELF_SECTION_TYPE_PROGBITS,
        0x6, // SHF_ALLOC | SHF_EXECINSTR
        TEXT_VADDR,
        TEXT_OFF,
        TEXT_SIZE,
        0,
        0
    );
    wr_shdr(
        &sht[SEC_SYMTAB * SHDR_SIZE],
        NAME_SYMTAB,
        ELF_SECTION_TYPE_SYMTAB,
        0,
        0,
        SYMTAB_OFF,
        SYMTAB_SZ,
        SEC_STRTAB, // sh_link -> .strtab
        SYM_SIZE
    );
    wr_shdr(&sht[SEC_STRTAB * SHDR_SIZE], NAME_STRTAB, ELF_SECTION_TYPE_STRTAB, 0, 0, STRTAB_OFF, STRTAB_SZ, 0, 0);
}

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

// ---------------------------------------------------------------------------
// Synthetic-fixture contract tests. These pin caller-observable decode
// results against a known-good little-endian ELF64 image, and assert the
// parser rejects malformed images (the documented FAILURE contract:
// "logs the failing step ... `out` is left zeroed").
// ---------------------------------------------------------------------------

// Known-good fixture decodes to the exact header / section / symbol the
// caller relies on.
bool test_elf_fixture_decodes_known_fields(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    build_elf_blob();

    Elf  elf;
    bool ok = ElfOpenFromMemoryCopy(&elf, elf_blob, sizeof(elf_blob), ALLOCATOR_OF(&alloc));
    if (!ok) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    ok = elf.header.elf_class == ELF_CLASS_64 && elf.header.data == ELF_DATA_LSB && elf.header.type == ELF_TYPE_EXEC &&
         elf.header.machine == EM_X86_64 && elf.header.entry == ENTRY_VADDR && elf.header.phoff == PHOFF_VAL &&
         elf.header.phnum == PHNUM_VAL && elf.header.shnum == N_SECTIONS && elf.header.shstrndx == SEC_SHSTRTAB;

    // Sections decode in order with correct names.
    ok                     = ok && VecLen(&elf.sections) == N_SECTIONS;
    const ElfSection *text = ElfFindSection(&elf, ".text");
    ok = ok && text != NULL && text->addr == TEXT_VADDR && text->offset == TEXT_OFF && text->size == TEXT_SIZE &&
         text->type == ELF_SECTION_TYPE_PROGBITS;

    // The one named function symbol decodes with its value, size, name,
    // bind, type, and section index intact.
    ok              = ok && VecLen(&elf.symbols) == 2; // null sym + my_func
    bool found_func = false;
    for (u64 i = 0; i < VecLen(&elf.symbols); ++i) {
        const ElfSymbol *s = VecPtrAt(&elf.symbols, i);
        if (s->name && ZstrCompare(s->name, "my_func") == 0) {
            found_func = s->value == FUNC_VADDR && s->size == FUNC_SIZE && s->type == ELF_SYMBOL_TYPE_FUNC &&
                         s->bind == ELF_SYMBOL_BIND_GLOBAL && s->section_index == SEC_TEXT;
        }
    }
    ok = ok && found_func;

    ElfDeinit(&elf);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ElfResolveAddress maps an address inside [value, value+size) to the
// enclosing symbol, and returns NULL outside it.
bool test_elf_resolve_address(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    build_elf_blob();

    Elf elf;
    if (!ElfOpenFromMemoryCopy(&elf, elf_blob, sizeof(elf_blob), ALLOCATOR_OF(&alloc))) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    // Exactly at the symbol start.
    const ElfSymbol *s  = ElfResolveAddress(&elf, FUNC_VADDR);
    bool             ok = s && s->name && ZstrCompare(s->name, "my_func") == 0;

    // Inside the body.
    s  = ElfResolveAddress(&elf, FUNC_VADDR + FUNC_SIZE / 2);
    ok = ok && s && ZstrCompare(s->name, "my_func") == 0;

    // Last covered byte.
    s  = ElfResolveAddress(&elf, FUNC_VADDR + FUNC_SIZE - 1);
    ok = ok && s && ZstrCompare(s->name, "my_func") == 0;

    // Just past the end -> no match.
    s  = ElfResolveAddress(&elf, FUNC_VADDR + FUNC_SIZE);
    ok = ok && s == NULL;

    // Below the symbol -> no match.
    s  = ElfResolveAddress(&elf, FUNC_VADDR - 1);
    ok = ok && s == NULL;

    ElfDeinit(&elf);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ElfFindSection returns NULL for a name that isn't present.
bool test_elf_find_section_absent(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    build_elf_blob();

    Elf elf;
    if (!ElfOpenFromMemoryCopy(&elf, elf_blob, sizeof(elf_blob), ALLOCATOR_OF(&alloc))) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }
    bool ok = ElfFindSection(&elf, ".nosuchsection") == NULL && ElfFindSection(&elf, ".text") != NULL;
    ElfDeinit(&elf);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Helper: a malformed image must be rejected (returns false). On
// rejection the parser must not leave the caller's allocator leaking,
// so we still Deinit the (zeroed) Elf.
static bool elf_rejects(const u8 *bytes, u64 len) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Elf              elf;
    bool             opened = ElfOpenFromMemoryCopy(&elf, bytes, len, ALLOCATOR_OF(&alloc));
    if (opened)
        ElfDeinit(&elf);
    DefaultAllocatorDeinit(&alloc);
    return !opened;
}

// Wrong magic, wrong class, wrong endianness, and a truncated header are
// all rejected (documented FAILURE: "magic / class / decoding").
bool test_elf_rejects_bad_magic(void) {
    build_elf_blob();
    u8 bad[ELF_FIXTURE_SIZE];
    MemCopy(bad, elf_blob, sizeof(bad));
    bad[1] = 'X'; // corrupt 'E' of the ELF magic
    return elf_rejects(bad, sizeof(bad));
}

bool test_elf_rejects_wrong_class(void) {
    build_elf_blob();
    u8 bad[ELF_FIXTURE_SIZE];
    MemCopy(bad, elf_blob, sizeof(bad));
    bad[4] = (u8)ELF_CLASS_32; // v1 supports ELF64 only
    return elf_rejects(bad, sizeof(bad));
}

bool test_elf_rejects_big_endian(void) {
    build_elf_blob();
    u8 bad[ELF_FIXTURE_SIZE];
    MemCopy(bad, elf_blob, sizeof(bad));
    bad[5] = (u8)ELF_DATA_MSB; // v1 supports little-endian only
    return elf_rejects(bad, sizeof(bad));
}

bool test_elf_rejects_truncated_header(void) {
    build_elf_blob();
    // Fewer than EI_NIDENT + 48 = 64 bytes for the ELF64 header.
    return elf_rejects(elf_blob, 40);
}

// A section header table whose offset/count runs past EOF is rejected
// rather than read out of bounds.
bool test_elf_rejects_sht_past_eof(void) {
    build_elf_blob();
    u8 bad[ELF_FIXTURE_SIZE];
    MemCopy(bad, elf_blob, sizeof(bad));
    // Point e_shoff (at e_ident + 24 = offset 40) well past EOF.
    wr_u64(&bad[16 + 24], (u64)sizeof(bad) + 0x1000);
    return elf_rejects(bad, sizeof(bad));
}

// e_shstrndx outside the section count is rejected.
bool test_elf_rejects_bad_shstrndx(void) {
    build_elf_blob();
    u8 bad[ELF_FIXTURE_SIZE];
    MemCopy(bad, elf_blob, sizeof(bad));
    wr_u16(&bad[16 + 30], (u16)(N_SECTIONS + 5)); // e_shstrndx past end
    return elf_rejects(bad, sizeof(bad));
}

// A .symtab whose data range overruns the file is rejected (bounds
// contract) rather than walked out of bounds.
bool test_elf_rejects_symtab_past_eof(void) {
    build_elf_blob();
    u8 bad[ELF_FIXTURE_SIZE];
    MemCopy(bad, elf_blob, sizeof(bad));
    // .symtab is section index 3; its header's sh_size is at
    // SHT_OFF + 3*64 + 32. Blow it up past EOF.
    wr_u64(&bad[SHT_OFF + SEC_SYMTAB * SHDR_SIZE + 32], (u64)sizeof(bad) + 0x1000);
    return elf_rejects(bad, sizeof(bad));
}

// ---------------------------------------------------------------------------
// Stripped-binary metadata fixture: a tiny ELF64 carrying a
// `.note.gnu.build-id` note and a `.gnu_debuglink`. Pins the
// caller-observable decode of build_id / build_id_size / debuglink_name
// / debuglink_crc, none of which the self-exe tests exercise (the
// self-exe test only checks build-id *presence*, and never the
// debuglink at all).
// ---------------------------------------------------------------------------

enum {
    DBG_FIXTURE_SIZE = 0x198,
    DBG_SHSTR_OFF    = 0x40,
    DBG_SHSTR_SIZE   = 45,
    DBG_NOTE_OFF     = 0x70,
    DBG_NOTE_SIZE    = 24,
    DBG_DL_OFF       = 0x88,
    DBG_DL_SIZE      = 16,
    DBG_SHT_OFF      = 0x98,
    DBG_N_SECTIONS   = 4,

    DBG_NAME_SHSTRTAB  = 1,
    DBG_NAME_NOTE      = 11,
    DBG_NAME_DEBUGLINK = 30,

    DBG_SEC_SHSTRTAB = 1,
    DBG_SEC_NOTE     = 2,
    DBG_SEC_DL       = 3,

    BUILD_ID_LEN  = 8,
    DEBUGLINK_CRC = 0x01020304,
};

static const u8 kBuildId[BUILD_ID_LEN] = {0xde, 0xad, 0xbe, 0xef, 0x01, 0x23, 0x45, 0x67};

static u8 dbg_blob[DBG_FIXTURE_SIZE];

static void build_dbg_blob(void) {
    MemSet(dbg_blob, 0, sizeof(dbg_blob));

    // --- ELF header -------------------------------------------------------
    dbg_blob[0] = 0x7f;
    dbg_blob[1] = 'E';
    dbg_blob[2] = 'L';
    dbg_blob[3] = 'F';
    dbg_blob[4] = (u8)ELF_CLASS_64;
    dbg_blob[5] = (u8)ELF_DATA_LSB;
    dbg_blob[6] = 1;

    u8 *h = &dbg_blob[16];
    wr_u16(&h[0], (u16)ELF_TYPE_DYN);
    wr_u16(&h[2], EM_X86_64);
    wr_u32(&h[4], 1);
    wr_u64(&h[8], 0);
    wr_u64(&h[16], 0);
    wr_u64(&h[24], DBG_SHT_OFF);
    wr_u32(&h[32], 0);
    wr_u16(&h[36], EHDR_SIZE);
    wr_u16(&h[38], 0);
    wr_u16(&h[40], 0);
    wr_u16(&h[42], SHDR_SIZE);
    wr_u16(&h[44], DBG_N_SECTIONS);
    wr_u16(&h[46], DBG_SEC_SHSTRTAB);

    // --- .shstrtab --------------------------------------------------------
    const char shstr[DBG_SHSTR_SIZE] = "\0.shstrtab\0.note.gnu.build-id\0.gnu_debuglink\0";
    MemCopy(&dbg_blob[DBG_SHSTR_OFF], shstr, DBG_SHSTR_SIZE);

    // --- build-id note: namesz=4, descsz=8, type=3 (NT_GNU_BUILD_ID), -----
    //     name "GNU\0", desc = kBuildId.
    u8 *note = &dbg_blob[DBG_NOTE_OFF];
    wr_u32(&note[0], 4);            // namesz
    wr_u32(&note[4], BUILD_ID_LEN); // descsz
    wr_u32(&note[8], 3);            // type = NT_GNU_BUILD_ID
    note[12] = 'G';
    note[13] = 'N';
    note[14] = 'U';
    note[15] = '\0';
    MemCopy(&note[16], kBuildId, BUILD_ID_LEN);

    // --- .gnu_debuglink: "foo.debug\0" + pad + crc32 ----------------------
    u8        *dl      = &dbg_blob[DBG_DL_OFF];
    const char fname[] = "foo.debug";
    MemCopy(dl, fname, sizeof(fname)); // includes NUL
    wr_u32(&dl[DBG_DL_SIZE - 4], DEBUGLINK_CRC);

    // --- section header table ---------------------------------------------
    u8 *sht = &dbg_blob[DBG_SHT_OFF];
    wr_shdr(&sht[0], 0, ELF_SECTION_TYPE_NULL, 0, 0, 0, 0, 0, 0);
    wr_shdr(
        &sht[DBG_SEC_SHSTRTAB * SHDR_SIZE],
        DBG_NAME_SHSTRTAB,
        ELF_SECTION_TYPE_STRTAB,
        0,
        0,
        DBG_SHSTR_OFF,
        DBG_SHSTR_SIZE,
        0,
        0
    );
    wr_shdr(
        &sht[DBG_SEC_NOTE * SHDR_SIZE],
        DBG_NAME_NOTE,
        ELF_SECTION_TYPE_NOTE,
        0,
        0,
        DBG_NOTE_OFF,
        DBG_NOTE_SIZE,
        0,
        0
    );
    wr_shdr(
        &sht[DBG_SEC_DL * SHDR_SIZE],
        DBG_NAME_DEBUGLINK,
        ELF_SECTION_TYPE_PROGBITS,
        0,
        0,
        DBG_DL_OFF,
        DBG_DL_SIZE,
        0,
        0
    );
}

// build_id bytes + size and the .gnu_debuglink name + crc decode to the
// exact values placed in the fixture.
bool test_elf_decodes_debug_metadata(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    build_dbg_blob();

    Elf elf;
    if (!ElfOpenFromMemoryCopy(&elf, dbg_blob, sizeof(dbg_blob), ALLOCATOR_OF(&alloc))) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    bool ok = elf.build_id != NULL && elf.build_id_size == BUILD_ID_LEN &&
              MemCompare(elf.build_id, kBuildId, BUILD_ID_LEN) == 0;

    ok = ok && elf.debuglink_name != NULL && ZstrCompare(elf.debuglink_name, "foo.debug") == 0 &&
         elf.debuglink_crc == DEBUGLINK_CRC;

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
        test_elf_fixture_decodes_known_fields,
        test_elf_resolve_address,
        test_elf_find_section_absent,
        test_elf_rejects_bad_magic,
        test_elf_rejects_wrong_class,
        test_elf_rejects_big_endian,
        test_elf_rejects_truncated_header,
        test_elf_rejects_sht_past_eof,
        test_elf_rejects_bad_shstrndx,
        test_elf_rejects_symtab_past_eof,
        test_elf_decodes_debug_metadata,
    };

    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), NULL, 0, "Elf");
}
