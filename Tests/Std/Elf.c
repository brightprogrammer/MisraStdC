#include <Misra.h>
#include <Misra/Parsers/Elf.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/File.h>
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
    ELF_FIXTURE_SIZE = 0x290,
    EHDR_SIZE        = 64,
    PHDR_SIZE        = 56,
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
    EM_AARCH64  = 183,

    // Real program-header table (the resolver walks it for load-bias
    // math). Two PT_LOAD segments; the second deliberately has
    // p_vaddr != p_offset (as AArch64's 64 KiB max-page-size produces),
    // so a test can pin that the decode keeps them distinct.
    PHOFF_VAL   = 0x220,
    PHNUM_VAL   = 2,
    PT_LOAD_VAL = 1,
    // segment 0: R-X, offset == vaddr.
    SEG0_OFF    = 0x0,
    SEG0_VADDR  = 0x400000,
    SEG0_FILESZ = 0x1000,
    SEG0_MEMSZ  = 0x1000,
    SEG0_FLAGS  = 5, // PF_R | PF_X
    // segment 1: RW, offset != vaddr (vaddr - offset = 0x410000).
    SEG1_OFF    = 0x1000,
    SEG1_VADDR  = 0x411000,
    SEG1_FILESZ = 0x500,
    SEG1_MEMSZ  = 0x600,
    SEG1_FLAGS  = 6,       // PF_R | PF_W
    SEG_ALIGN   = 0x10000, // 64 KiB (AArch64 max-page-size)

    // Distinctive non-zero sh_info on .symtab -- pins s.info = info
    // against the cxx_assign_const mutant that forces info to a constant.
    SYMTAB_INFO = 0x11,
};

// Header-field byte offsets within the ELF header (post e_ident@16).
enum {
    OFF_E_TYPE      = 16,
    OFF_E_MACHINE   = 18,
    OFF_E_ENTRY     = 24,
    OFF_E_PHOFF     = 32,
    OFF_E_SHOFF     = 40,
    OFF_E_SHENTSIZE = 58,
    OFF_E_SHNUM     = 60,
    OFF_E_SHSTRNDX  = 62,
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

// Write a 64-byte ELF64 LE section header at p, with an explicit sh_info.
static void
    wr_shdr2(u8 *p, u32 name, u32 type, u64 flags, u64 addr, u64 offset, u64 size, u32 link, u32 info, u64 entsize) {
    wr_u32(&p[0], name);
    wr_u32(&p[4], type);
    wr_u64(&p[8], flags);
    wr_u64(&p[16], addr);
    wr_u64(&p[24], offset);
    wr_u64(&p[32], size);
    wr_u32(&p[40], link);
    wr_u32(&p[44], info);
    wr_u64(&p[48], 1); // addralign
    wr_u64(&p[56], entsize);
}

// Write a 56-byte ELF64 LE program header at p.
static void wr_phdr(u8 *p, u32 type, u32 flags, u64 offset, u64 vaddr, u64 filesz, u64 memsz, u64 align) {
    wr_u32(&p[0], type);
    wr_u32(&p[4], flags);
    wr_u64(&p[8], offset);
    wr_u64(&p[16], vaddr);
    wr_u64(&p[24], vaddr); // p_paddr (unused by the parser)
    wr_u64(&p[32], filesz);
    wr_u64(&p[40], memsz);
    wr_u64(&p[48], align);
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
    wr_shdr2(
        &sht[SEC_SYMTAB * SHDR_SIZE],
        NAME_SYMTAB,
        ELF_SECTION_TYPE_SYMTAB,
        0,
        0,
        SYMTAB_OFF,
        SYMTAB_SZ,
        SEC_STRTAB,  // sh_link -> .strtab
        SYMTAB_INFO, // sh_info -- distinctive, pins the info decode
        SYM_SIZE
    );
    wr_shdr(&sht[SEC_STRTAB * SHDR_SIZE], NAME_STRTAB, ELF_SECTION_TYPE_STRTAB, 0, 0, STRTAB_OFF, STRTAB_SZ, 0, 0);

    // --- program header table (at PHOFF_VAL = 0x220) ----------------------
    wr_phdr(
        &elf_blob[PHOFF_VAL + 0 * PHDR_SIZE],
        PT_LOAD_VAL,
        SEG0_FLAGS,
        SEG0_OFF,
        SEG0_VADDR,
        SEG0_FILESZ,
        SEG0_MEMSZ,
        SEG_ALIGN
    );
    wr_phdr(
        &elf_blob[PHOFF_VAL + 1 * PHDR_SIZE],
        PT_LOAD_VAL,
        SEG1_FLAGS,
        SEG1_OFF,
        SEG1_VADDR,
        SEG1_FILESZ,
        SEG1_MEMSZ,
        SEG_ALIGN
    );
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

// Program headers decode in order with the exact PT_LOAD fields the
// fixture wrote -- including the second segment's p_vaddr != p_offset
// (the AArch64 64 KiB-page case the resolver's load-bias math depends
// on). Pins the per-field decode, the loop bounds, and that every
// program header is walked.
bool test_elf_decodes_program_headers(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    build_elf_blob();

    Elf  elf;
    bool ok = ElfOpenFromMemoryCopy(&elf, elf_blob, sizeof(elf_blob), ALLOCATOR_OF(&alloc));
    if (ok) {
        ok = VecLen(&elf.segments) == 2;
        if (ok) {
            const ElfSegment *s0 = VecPtrAt(&elf.segments, 0);
            const ElfSegment *s1 = VecPtrAt(&elf.segments, 1);
            ok                   = s0->type == ELF_PT_LOAD && s0->flags == SEG0_FLAGS && s0->offset == SEG0_OFF &&
                 s0->vaddr == SEG0_VADDR && s0->filesz == SEG0_FILESZ && s0->memsz == SEG0_MEMSZ &&
                 s0->align == SEG_ALIGN && s1->type == ELF_PT_LOAD && s1->flags == SEG1_FLAGS &&
                 s1->offset == SEG1_OFF && s1->vaddr == SEG1_VADDR && s1->filesz == SEG1_FILESZ &&
                 s1->memsz == SEG1_MEMSZ && s1->align == SEG_ALIGN &&
                 s1->vaddr != s1->offset; // distinct vaddr/offset survives decode
        }
        ElfDeinit(&elf);
    }
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

// A program-header table whose count runs past EOF is rejected: e_phnum
// drives the (count * PHDR_SIZE) range check, and without it the decode
// loop would read off the end of the buffer.
bool test_elf_rejects_phdr_table_out_of_range(void) {
    build_elf_blob();
    u8 bad[ELF_FIXTURE_SIZE];
    MemCopy(bad, elf_blob, sizeof(bad));
    wr_u16(&bad[16 + 40], 3); // e_phnum = 3, but only 2 PT_LOADs fit before EOF
    return elf_rejects(bad, sizeof(bad));
}

// e_shstrndx == e_shnum must be rejected. `shstrndx >= n` cannot be
// relaxed to `>`: the section header at index shnum is one past the table
// and FILE-CONTROLLED, so `>` would read it as the .shstrtab header. We
// plant a VALID one there (pointing at the real shstrtab) so a relaxed
// boundary would parse an attacker-chosen string table; correct `>=`
// rejects on the index itself, not on a downstream range failure.
bool test_elf_rejects_shstrndx_equals_shnum(void) {
    build_elf_blob();
    u8 bad[ELF_FIXTURE_SIZE];
    MemCopy(bad, elf_blob, sizeof(bad));
    wr_u16(&bad[16 + 46], N_SECTIONS); // e_shstrndx = shnum (out of range)
    wr_shdr(
        &bad[SHT_OFF + N_SECTIONS * SHDR_SIZE],
        NAME_SHSTRTAB,
        ELF_SECTION_TYPE_STRTAB,
        0,
        0,
        SHSTR_OFF,
        SHSTR_SIZE,
        0,
        0
    );
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
    DBG_N_SEC        = 4,     // alias used by the Mutants4 build_dbg builder
    DBG_BLOB_SIZE    = 0x198, // == DBG_FIXTURE_SIZE; named for build_dbg

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

// ===========================================================================
// Mutation-hardening tests (moved from Elf.Mutants1..4 staging files).
// Shared fixture helpers/builders are reused where their name+body match
// the baseline above; distinct builders are defined once here.
// ===========================================================================

// ---------------------------------------------------------------------------
// Elf.Mutants1: section-table / string-table / bounds path. Reuses the
// baseline wr_u16/32/64, wr_shdr2, elf_blob, build_elf_blob, and elf_rejects.
// ---------------------------------------------------------------------------

// Baseline: the full section table decodes with the exact count, names,
// types, sizes, and addresses. Pins the shnum loop (line 224) and the
// per-section field assignments against init / arithmetic mutants -- a
// dropped or extra iteration changes the count or the last/first section.
bool test_el1_section_table_exact(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    build_elf_blob();

    Elf elf;
    if (!ElfOpenFromMemoryCopy(&elf, elf_blob, sizeof(elf_blob), ALLOCATOR_OF(&alloc))) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    bool ok = VecLen(&elf.sections) == N_SECTIONS;

    // First section is the NULL section (name ""), last is .strtab.
    const ElfSection *first = VecPtrAt(&elf.sections, 0);
    const ElfSection *last  = VecPtrAt(&elf.sections, N_SECTIONS - 1);
    ok                      = ok && first->name && first->name[0] == '\0' && first->type == ELF_SECTION_TYPE_NULL;
    ok = ok && last->name && ZstrCompare(last->name, ".strtab") == 0 && last->type == ELF_SECTION_TYPE_STRTAB &&
         last->offset == STRTAB_OFF && last->size == STRTAB_SZ;

    ElfDeinit(&elf);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// .text decodes with the exact addr/offset/size/type/flags. Pins the
// sh_* field-offset reads and the per-field assignments.
bool test_el1_text_section_fields(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    build_elf_blob();

    Elf elf;
    if (!ElfOpenFromMemoryCopy(&elf, elf_blob, sizeof(elf_blob), ALLOCATOR_OF(&alloc))) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    const ElfSection *t = ElfFindSection(&elf, ".text");
    bool ok = t != NULL && t->type == ELF_SECTION_TYPE_PROGBITS && t->addr == TEXT_VADDR && t->offset == TEXT_OFF &&
              t->size == TEXT_SIZE && (t->flags & 0x4) != 0;

    ElfDeinit(&elf);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// The .symtab section's sh_info decodes to the distinctive non-zero value
// placed in the fixture. Kills the cxx_assign_const mutant on
// `s.info = info` (which would force info to a constant such as 42).
bool test_el1_section_info_field(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    build_elf_blob();

    Elf elf;
    if (!ElfOpenFromMemoryCopy(&elf, elf_blob, sizeof(elf_blob), ALLOCATOR_OF(&alloc))) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    const ElfSection *sym = ElfFindSection(&elf, ".symtab");
    // .symtab carries the distinctive info; every other section's info is 0.
    const ElfSection *txt = ElfFindSection(&elf, ".text");
    bool              ok  = sym != NULL && sym->info == SYMTAB_INFO && txt != NULL && txt->info == 0;

    ElfDeinit(&elf);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A section-header table whose count*entsize overruns the file (but whose
// offset alone is in range) is rejected. Kills both arithmetic mutants on
// `needed = (u64)n * SHDR64_SIZE` (line 194): forcing `needed` to a small
// constant, or dividing instead of multiplying, would shrink `needed`
// enough to slip the truncated table past the range check.
bool test_el1_sht_count_overruns_rejected(void) {
    build_elf_blob();
    // Trim the buffer so that shoff + n*64 lands past EOF, while shoff
    // itself stays inside. SHT_OFF = 0xE0, n = 5 -> needs 0xE0 + 320 =
    // 0x220 bytes. Cut to 0x100: shoff (0xE0) is in range, but the full
    // table (needs 0x220) is not. A mutated `needed` of 0, ~0, or 42
    // would wrongly accept.
    return elf_rejects(elf_blob, 0x100);
}

// shstrndx exactly equal to the section count is out of range (valid
// indices are 0..n-1) and must be rejected. Kills cxx_ge_to_gt on
// `shstrndx >= n` (line 200): under `>` the boundary index n slips
// through and the parser reads a section header one past the table.
bool test_el1_shstrndx_equals_count_rejected(void) {
    build_elf_blob();
    u8 bad[ELF_FIXTURE_SIZE];
    MemCopy(bad, elf_blob, sizeof(bad));
    // e_shstrndx sits at e_ident(16) + 46 = byte 62 in the header.
    wr_u16(&bad[16 + 46], (u16)N_SECTIONS); // e_shstrndx = n (one past the last valid index)
    return elf_rejects(bad, sizeof(bad));
}

// A shstrtab section header whose data range overruns the file is
// rejected. Kills cxx_replace_scalar_call on the `elf_range_ok(...)`
// guard at line 217: replacing the call with a truthy constant would
// make `!range_ok` always false and accept the out-of-range shstrtab.
bool test_el1_shstrtab_overruns_rejected(void) {
    build_elf_blob();
    u8 bad[ELF_FIXTURE_SIZE];
    MemCopy(bad, elf_blob, sizeof(bad));
    // shstrtab is section index 1; its sh_size is at SHT_OFF + 1*64 + 32.
    // Blow it up past EOF (offset stays valid, size overruns).
    wr_u64(&bad[SHT_OFF + SEC_SHSTRTAB * SHDR_SIZE + 32], (u64)sizeof(bad) + 0x1000);
    return elf_rejects(bad, sizeof(bad));
}

// A valid shstrtab range is accepted (the other half of the line-217
// guard): replacing `elf_range_ok(...)` with a falsy constant would make
// `!range_ok` always true and reject the perfectly valid baseline image.
// The baseline open succeeding is what kills that variant.
bool test_el1_valid_shstrtab_accepted(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    build_elf_blob();

    Elf  elf;
    bool ok = ElfOpenFromMemoryCopy(&elf, elf_blob, sizeof(elf_blob), ALLOCATOR_OF(&alloc));
    if (ok) {
        // Names resolved through the in-range shstrtab.
        ok = ElfFindSection(&elf, ".shstrtab") != NULL && ElfFindSection(&elf, ".text") != NULL;
        ElfDeinit(&elf);
    }
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Exact-fit at the top edge: shstrtab offset == file_size, size == 0.
// Real elf_range_ok accepts (offset is not strictly past EOF, zero size
// fits). Kills cxx_gt_to_ge on `offset > BufLength` (line 110): under
// `>=` the offset==len case is rejected and the open fails.
bool test_el1_range_offset_at_eof_zero_size(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    build_elf_blob();
    u8 bad[ELF_FIXTURE_SIZE];
    MemCopy(bad, elf_blob, sizeof(bad));
    // shstrtab header: offset = file_size, size = 0.
    wr_u64(&bad[SHT_OFF + SEC_SHSTRTAB * SHDR_SIZE + 24], (u64)sizeof(bad)); // sh_offset
    wr_u64(&bad[SHT_OFF + SEC_SHSTRTAB * SHDR_SIZE + 32], 0);                // sh_size

    Elf  elf;
    bool ok = ElfOpenFromMemoryCopy(&elf, bad, sizeof(bad), ALLOCATOR_OF(&alloc));
    if (ok) {
        // Empty shstrtab => every section name decodes to "".
        ok = VecLen(&elf.sections) == N_SECTIONS;
        ElfDeinit(&elf);
    }
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Exact-fit spanning the whole file: shstrtab offset == 0, size ==
// file_size. Real elf_range_ok accepts (size is not strictly past EOF,
// offset 0 leaves room for the full size). Kills cxx_gt_to_ge on
// `size > BufLength` (line 112): under `>=` size==len is rejected.
bool test_el1_range_size_equals_file_accepted(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    build_elf_blob();
    u8 bad[ELF_FIXTURE_SIZE];
    MemCopy(bad, elf_blob, sizeof(bad));
    // shstrtab header: offset = 0, size = file_size (exact fit).
    wr_u64(&bad[SHT_OFF + SEC_SHSTRTAB * SHDR_SIZE + 24], 0);                // sh_offset
    wr_u64(&bad[SHT_OFF + SEC_SHSTRTAB * SHDR_SIZE + 32], (u64)sizeof(bad)); // sh_size

    Elf  elf;
    bool ok = ElfOpenFromMemoryCopy(&elf, bad, sizeof(bad), ALLOCATOR_OF(&alloc));
    if (ok) {
        ok = VecLen(&elf.sections) == N_SECTIONS;
        ElfDeinit(&elf);
    }
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A symtab whose offset and size are each individually in range but whose
// offset+size overruns the file is rejected. Kills cxx_sub_to_add on
// `size > BufLength - offset` (line 116): the addition `BufLength + offset`
// is far larger than any section size, so the overrunning range is wrongly
// accepted under the mutant.
bool test_el1_range_offset_plus_size_overruns_rejected(void) {
    build_elf_blob();
    u8 bad[ELF_FIXTURE_SIZE];
    MemCopy(bad, elf_blob, sizeof(bad));
    // .symtab is section index 3. Set its offset/size so each fits alone
    // but the sum runs past EOF: offset = file_size - 0x20, size = 0x40.
    u64 base = SHT_OFF + SEC_SYMTAB * SHDR_SIZE;
    wr_u64(&bad[base + 24], (u64)sizeof(bad) - 0x20); // sh_offset (in range)
    wr_u64(&bad[base + 32], 0x40);                    // sh_size   (in range), sum overruns
    return elf_rejects(bad, sizeof(bad));
}

// shstrtab whose declared size omits the terminating NUL of the last
// name: that name must decode to "" (no NUL inside [idx, size)). Kills:
//   - cxx_eq_to_ne on `base[p] == '\0'` (line 102): under `!=` the scan
//     stops at the first non-NUL char and returns a non-empty name.
//   - cxx_lt_to_le on `p < strtab_size` (line 101) when the clipped byte
//     past `size` is itself a NUL (see the companion test below).
// Here we make the region past `size` non-NUL too, isolating the eq/ne
// kill: the name is unterminated within AND just past the declared size.
bool test_el1_strtab_unterminated_name_is_empty(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    MemSet(elf_blob, 0, sizeof(elf_blob));

    // Minimal valid header.
    elf_blob[0] = 0x7f;
    elf_blob[1] = 'E';
    elf_blob[2] = 'L';
    elf_blob[3] = 'F';
    elf_blob[4] = (u8)ELF_CLASS_64;
    elf_blob[5] = (u8)ELF_DATA_LSB;
    elf_blob[6] = 1;
    u8 *h       = &elf_blob[16];
    wr_u16(&h[0], (u16)ELF_TYPE_EXEC);
    wr_u16(&h[2], EM_X86_64);
    wr_u32(&h[4], 1);
    wr_u64(&h[24], SHT_OFF);
    wr_u16(&h[36], EHDR_SIZE);
    wr_u16(&h[42], SHDR_SIZE);
    wr_u16(&h[44], 2);            // 2 sections: NULL + shstrtab
    wr_u16(&h[46], SEC_SHSTRTAB); // shstrndx = 1

    // shstrtab content: "\0AB" then non-NUL filler. Declared size = 3
    // (covers "\0AB"). The NULL section has name 0 -> "". A second name
    // at idx 1 ("AB") has NO NUL inside [1,3): bytes are 'A','B'. The
    // byte at index 3 (== declared size) is also non-NUL ('C'), so under
    // the lt->le mutant the scan still finds no NUL and returns "" -- this
    // test targets only the eq->ne mutant.
    enum {
        U_SHSTR_OFF  = 0x40,
        U_SHSTR_SIZE = 3,
        U_NAME_AB    = 1,
    };
    elf_blob[U_SHSTR_OFF + 0] = '\0';
    elf_blob[U_SHSTR_OFF + 1] = 'A';
    elf_blob[U_SHSTR_OFF + 2] = 'B';
    elf_blob[U_SHSTR_OFF + 3] = 'C'; // just past declared size, non-NUL
    elf_blob[U_SHSTR_OFF + 4] = 'D';

    u8 *sht = &elf_blob[SHT_OFF];
    wr_shdr2(&sht[0], 0, ELF_SECTION_TYPE_NULL, 0, 0, 0, 0, 0, 0, 0);
    // Section 1 = the shstrtab itself; give it name idx U_NAME_AB ("AB").
    wr_shdr2(&sht[1 * SHDR_SIZE], U_NAME_AB, ELF_SECTION_TYPE_STRTAB, 0, 0, U_SHSTR_OFF, U_SHSTR_SIZE, 0, 0, 0);

    Elf elf;
    if (!ElfOpenFromMemoryCopy(&elf, elf_blob, sizeof(elf_blob), ALLOCATOR_OF(&alloc))) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    // Section 1's name idx points at "AB" with no terminator inside the
    // declared strtab size => real decodes "".
    const ElfSection *s1 = VecPtrAt(&elf.sections, 1);
    bool              ok = VecLen(&elf.sections) == 2 && s1->name != NULL && s1->name[0] == '\0';

    ElfDeinit(&elf);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// shstrtab whose name has no NUL inside [idx, size) but a NUL sitting
// exactly at index == size (one past the declared end). Real decodes ""
// (the terminator is outside the declared range). Kills cxx_lt_to_le on
// `p < strtab_size` (line 101): under `<=` the scan reaches p == size,
// sees that NUL, and wrongly returns a non-empty name.
bool test_el1_strtab_nul_at_size_boundary_is_empty(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    MemSet(elf_blob, 0, sizeof(elf_blob));

    elf_blob[0] = 0x7f;
    elf_blob[1] = 'E';
    elf_blob[2] = 'L';
    elf_blob[3] = 'F';
    elf_blob[4] = (u8)ELF_CLASS_64;
    elf_blob[5] = (u8)ELF_DATA_LSB;
    elf_blob[6] = 1;
    u8 *h       = &elf_blob[16];
    wr_u16(&h[0], (u16)ELF_TYPE_EXEC);
    wr_u16(&h[2], EM_X86_64);
    wr_u32(&h[4], 1);
    wr_u64(&h[24], SHT_OFF);
    wr_u16(&h[36], EHDR_SIZE);
    wr_u16(&h[42], SHDR_SIZE);
    wr_u16(&h[44], 2);
    wr_u16(&h[46], SEC_SHSTRTAB);

    // shstrtab: bytes "\0AB\0..." declared size = 3 (covers "\0AB").
    // Name idx 1 -> 'A','B' inside [1,3) -- no NUL. The NUL sits at index
    // 3 == declared size. Real: no NUL inside range -> "". le-mutant:
    // checks index 3, finds NUL -> returns "AB".
    enum {
        B_SHSTR_OFF  = 0x40,
        B_SHSTR_SIZE = 3,
        B_NAME_AB    = 1,
    };
    elf_blob[B_SHSTR_OFF + 0] = '\0';
    elf_blob[B_SHSTR_OFF + 1] = 'A';
    elf_blob[B_SHSTR_OFF + 2] = 'B';
    elf_blob[B_SHSTR_OFF + 3] = '\0'; // NUL exactly at index == size

    u8 *sht = &elf_blob[SHT_OFF];
    wr_shdr2(&sht[0], 0, ELF_SECTION_TYPE_NULL, 0, 0, 0, 0, 0, 0, 0);
    wr_shdr2(&sht[1 * SHDR_SIZE], B_NAME_AB, ELF_SECTION_TYPE_STRTAB, 0, 0, B_SHSTR_OFF, B_SHSTR_SIZE, 0, 0, 0);

    Elf elf;
    if (!ElfOpenFromMemoryCopy(&elf, elf_blob, sizeof(elf_blob), ALLOCATOR_OF(&alloc))) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    const ElfSection *s1 = VecPtrAt(&elf.sections, 1);
    bool              ok = VecLen(&elf.sections) == 2 && s1->name != NULL && s1->name[0] == '\0';

    ElfDeinit(&elf);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// shstrtab whose first byte is NOT a NUL, with a name that terminates
// forward but has no NUL anywhere before its index. Kills
// cxx_pre_inc_to_pre_dec on `++p` (line 101): a forward scan finds the
// terminator and returns the real name; a backward scan from idx walks
// down through non-NUL bytes to index 0 (also non-NUL) and underflows,
// finding no terminator and returning "".
bool test_el1_strtab_forward_scan_name(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    MemSet(elf_blob, 0, sizeof(elf_blob));

    elf_blob[0] = 0x7f;
    elf_blob[1] = 'E';
    elf_blob[2] = 'L';
    elf_blob[3] = 'F';
    elf_blob[4] = (u8)ELF_CLASS_64;
    elf_blob[5] = (u8)ELF_DATA_LSB;
    elf_blob[6] = 1;
    u8 *h       = &elf_blob[16];
    wr_u16(&h[0], (u16)ELF_TYPE_EXEC);
    wr_u16(&h[2], EM_X86_64);
    wr_u32(&h[4], 1);
    wr_u64(&h[24], SHT_OFF);
    wr_u16(&h[36], EHDR_SIZE);
    wr_u16(&h[42], SHDR_SIZE);
    wr_u16(&h[44], 2);
    wr_u16(&h[46], SEC_SHSTRTAB);

    // shstrtab: bytes 'X','Y','Z','\0', ... declared size = 4. Byte 0 is
    // 'X' (non-NUL), so a backward scan from idx 0 finds no NUL before
    // underflowing. Name idx 0 -> forward scan reads "XYZ" then NUL.
    enum {
        F_SHSTR_OFF  = 0x40,
        F_SHSTR_SIZE = 4,
        F_NAME_XYZ   = 0,
    };
    elf_blob[F_SHSTR_OFF + 0] = 'X';
    elf_blob[F_SHSTR_OFF + 1] = 'Y';
    elf_blob[F_SHSTR_OFF + 2] = 'Z';
    elf_blob[F_SHSTR_OFF + 3] = '\0';

    u8 *sht = &elf_blob[SHT_OFF];
    wr_shdr2(&sht[0], 0, ELF_SECTION_TYPE_NULL, 0, 0, 0, 0, 0, 0, 0);
    // Section 1 = shstrtab, name idx 0 -> "XYZ".
    wr_shdr2(&sht[1 * SHDR_SIZE], F_NAME_XYZ, ELF_SECTION_TYPE_STRTAB, 0, 0, F_SHSTR_OFF, F_SHSTR_SIZE, 0, 0, 0);

    Elf elf;
    if (!ElfOpenFromMemoryCopy(&elf, elf_blob, sizeof(elf_blob), ALLOCATOR_OF(&alloc))) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    const ElfSection *s1 = VecPtrAt(&elf.sections, 1);
    bool              ok = VecLen(&elf.sections) == 2 && s1->name != NULL && ZstrCompare(s1->name, "XYZ") == 0;

    ElfDeinit(&elf);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A name at a non-zero offset decodes exactly (not the first string).
// Pins that elf_str_at indexes from `base + idx`, not from base.
bool test_el1_strtab_nonzero_offset_name(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    build_elf_blob();

    Elf elf;
    if (!ElfOpenFromMemoryCopy(&elf, elf_blob, sizeof(elf_blob), ALLOCATOR_OF(&alloc))) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    // ".strtab" lives at NAME_STRTAB (25) in the shstrtab, well past the
    // first string. It must decode exactly, not collapse to "" or ".shstrtab".
    const ElfSection *s   = ElfFindSection(&elf, ".strtab");
    const ElfSection *sym = ElfFindSection(&elf, ".symtab");
    bool              ok =
        s != NULL && ZstrCompare(s->name, ".strtab") == 0 && sym != NULL && ZstrCompare(sym->name, ".symtab") == 0;

    ElfDeinit(&elf);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Finding the LAST section by name returns the correct pointer; an absent
// name returns NULL. Kills cxx_lt_to_le on `i < VecLen` (line 539): under
// `<=` the loop visits index == len (one past the vector). For the
// present-last case the correct match must still be the in-range element;
// for the absent case the real loop returns NULL without touching the
// out-of-bounds slot.
bool test_el1_find_section_last_and_absent(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    build_elf_blob();

    Elf elf;
    if (!ElfOpenFromMemoryCopy(&elf, elf_blob, sizeof(elf_blob), ALLOCATOR_OF(&alloc))) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    // .strtab is the final section in the table.
    const ElfSection *last     = ElfFindSection(&elf, ".strtab");
    const ElfSection *expected = VecPtrAt(&elf.sections, N_SECTIONS - 1);
    bool              ok       = last != NULL && last == expected;

    // Absent name -> NULL.
    ok = ok && ElfFindSection(&elf, ".no_such_section_xyz") == NULL;

    ElfDeinit(&elf);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// Elf.Mutants2: header-decode / open path. Reuses the baseline fixture,
// wr_shdr, build_elf_blob, elf_rejects, and the OFF_* enum above. Adds the
// minimal-64-byte image builder.
// ---------------------------------------------------------------------------

#if defined(__aarch64__)
#    define EM_SELF EM_AARCH64
#else
#    define EM_SELF EM_X86_64
#endif

bool test_el2_self_exe_header_exact(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Elf              elf;
    if (!ElfOpen(&elf, "/proc/self/exe", ALLOCATOR_OF(&alloc))) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    bool ok = elf.header.elf_class == ELF_CLASS_64;         // EI_CLASS decoded
    ok      = ok && elf.header.data == ELF_DATA_LSB;        // EI_DATA decoded
    ok      = ok && elf.header.machine == EM_SELF;          // e_machine matches host
    ok      = ok && (elf.header.type == ELF_TYPE_DYN || elf.header.type == ELF_TYPE_EXEC);
    ok      = ok && elf.header.shoff != 0;                  // section table present
    ok      = ok && elf.header.shnum > 0;                   // has sections
    ok      = ok && elf.header.shstrndx < elf.header.shnum; // shstrndx in range
    ok      = ok && VecLen(&elf.sections) == elf.header.shnum;

    ElfDeinit(&elf);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Fixture decodes to the EXACT header field values placed in the blob.
// Pins e_type/e_machine/e_entry/e_phoff/e_phnum/e_shoff/e_shnum/e_shstrndx
// against multi-byte-assembly (shift/or) and offset mutations.
bool test_el2_fixture_header_exact(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    build_elf_blob();

    Elf elf;
    if (!ElfOpenFromMemoryCopy(&elf, elf_blob, sizeof(elf_blob), ALLOCATOR_OF(&alloc))) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    bool ok = elf.header.elf_class == ELF_CLASS_64;
    ok      = ok && elf.header.data == ELF_DATA_LSB;
    ok      = ok && elf.header.type == ELF_TYPE_EXEC;
    ok      = ok && elf.header.machine == EM_X86_64;
    ok      = ok && elf.header.entry == ENTRY_VADDR;
    ok      = ok && elf.header.phoff == PHOFF_VAL;
    ok      = ok && elf.header.phnum == PHNUM_VAL;
    ok      = ok && elf.header.shoff == (u64)SHT_OFF;
    ok      = ok && elf.header.shnum == N_SECTIONS;
    ok      = ok && elf.header.shstrndx == SEC_SHSTRTAB;
    ok      = ok && VecLen(&elf.sections) == N_SECTIONS;

    ElfDeinit(&elf);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// L126: `BufLength < EI_NIDENT + 48` (== 64). The `<`->`<=` mutant
// rejects a buffer of length EXACTLY 64. A minimal valid 64-byte ELF
// (its single section header overlaps the header bytes at shoff=0)
// opens OK under the real `<`, so a successful open of an exactly-64-byte
// image flips the mutant.
static void build_min64(u8 *b) {
    MemSet(b, 0, 64);
    b[0] = 0x7f;
    b[1] = 'E';
    b[2] = 'L';
    b[3] = 'F';
    b[4] = (u8)ELF_CLASS_64;
    b[5] = (u8)ELF_DATA_LSB;
    b[6] = 1;

    u8 *h = &b[16];
    wr_u16(&h[0], (u16)ELF_TYPE_EXEC); // e_type
    wr_u16(&h[2], EM_X86_64);          // e_machine
    // e_entry (bytes 24..31) doubles as section[0].sh_offset (shoff=0):
    // keep it 0 so shstrtab offset is 0.
    wr_u64(&h[8], 0); // e_entry -> sh_offset = 0
    // e_phoff (bytes 32..39) doubles as section[0].sh_size: pick 64 so
    // the (overlapping) shstrtab covers the whole 64-byte file.
    wr_u64(&h[16], 64);        // e_phoff -> sh_size = 64
    wr_u64(&h[24], 0);         // e_shoff = 0 (SHT overlaps the header)
    wr_u16(&h[42], SHDR_SIZE); // e_shentsize = 64
    wr_u16(&h[44], 1);         // e_shnum = 1
    wr_u16(&h[46], 0);         // e_shstrndx = 0
}

bool test_el2_min64_opens(void) {
    u8 b[64];
    build_min64(b);

    DefaultAllocator alloc = DefaultAllocatorInit();
    Elf              elf;
    bool             opened = ElfOpenFromMemoryCopy(&elf, b, sizeof(b), ALLOCATOR_OF(&alloc));
    bool             ok     = opened && elf.header.shnum == 1 && VecLen(&elf.sections) == 1;
    if (opened)
        ElfDeinit(&elf);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// 63 bytes (one short of the minimum) must be rejected regardless of the
// `<` vs `<=` boundary -- pins the lower side of the length guard.
bool test_el2_min63_rejected(void) {
    u8 b[64];
    build_min64(b);
    return elf_rejects(b, 63);
}

bool test_el2_valid_fixture_opens(void) {
    build_elf_blob();
    DefaultAllocator alloc = DefaultAllocatorInit();
    Elf              elf;
    bool             opened = ElfOpenFromMemoryCopy(&elf, elf_blob, sizeof(elf_blob), ALLOCATOR_OF(&alloc));
    if (opened)
        ElfDeinit(&elf);
    DefaultAllocatorDeinit(&alloc);
    return opened;
}

bool test_el2_rejects_mag0(void) {
    build_elf_blob();
    u8 bad[ELF_FIXTURE_SIZE];
    MemCopy(bad, elf_blob, sizeof(bad));
    bad[0] = 0x00; // corrupt 0x7f
    return elf_rejects(bad, sizeof(bad));
}

bool test_el2_rejects_mag1(void) {
    build_elf_blob();
    u8 bad[ELF_FIXTURE_SIZE];
    MemCopy(bad, elf_blob, sizeof(bad));
    bad[1] = 'X'; // corrupt 'E'
    return elf_rejects(bad, sizeof(bad));
}

bool test_el2_rejects_mag2(void) {
    build_elf_blob();
    u8 bad[ELF_FIXTURE_SIZE];
    MemCopy(bad, elf_blob, sizeof(bad));
    bad[2] = 'X'; // corrupt 'L'
    return elf_rejects(bad, sizeof(bad));
}

bool test_el2_rejects_mag3(void) {
    build_elf_blob();
    u8 bad[ELF_FIXTURE_SIZE];
    MemCopy(bad, elf_blob, sizeof(bad));
    bad[3] = 'X'; // corrupt 'F'
    return elf_rejects(bad, sizeof(bad));
}

bool test_el2_rejects_class32(void) {
    build_elf_blob();
    u8 bad[ELF_FIXTURE_SIZE];
    MemCopy(bad, elf_blob, sizeof(bad));
    bad[4] = (u8)ELF_CLASS_32; // 32-bit not supported
    return elf_rejects(bad, sizeof(bad));
}

bool test_el2_rejects_class_invalid(void) {
    build_elf_blob();
    u8 bad[ELF_FIXTURE_SIZE];
    MemCopy(bad, elf_blob, sizeof(bad));
    bad[4] = (u8)ELF_CLASS_INVALID;
    return elf_rejects(bad, sizeof(bad));
}

bool test_el2_rejects_big_endian(void) {
    build_elf_blob();
    u8 bad[ELF_FIXTURE_SIZE];
    MemCopy(bad, elf_blob, sizeof(bad));
    bad[5] = (u8)ELF_DATA_MSB; // big-endian not supported
    return elf_rejects(bad, sizeof(bad));
}

bool test_el2_rejects_data_invalid(void) {
    build_elf_blob();
    u8 bad[ELF_FIXTURE_SIZE];
    MemCopy(bad, elf_blob, sizeof(bad));
    bad[5] = (u8)ELF_DATA_INVALID;
    return elf_rejects(bad, sizeof(bad));
}

// Class 64 + data LSB are the accepted values; a one-step bump on either
// must reject. (Pins the exact constant in each `!=` compare.)
bool test_el2_rejects_class_bumped(void) {
    build_elf_blob();
    u8 bad[ELF_FIXTURE_SIZE];
    MemCopy(bad, elf_blob, sizeof(bad));
    bad[4] = (u8)ELF_CLASS_64 + 1; // 3
    return elf_rejects(bad, sizeof(bad));
}

bool test_el2_rejects_truncated_40(void) {
    build_elf_blob();
    return elf_rejects(elf_blob, 40);
}

bool test_el2_rejects_truncated_zeroish(void) {
    build_elf_blob();
    // A handful of valid-magic bytes but far under 64 -> rejected.
    return elf_rejects(elf_blob, 16);
}

// L183: `if (shentsize != SHDR64_SIZE && shnum > 0)`. With shnum > 0 and
// e_shentsize != 64, the real code rejects in elf_decode_header. The
// `> 0`->`<= 0` mutant skips the shentsize check (shnum>0 makes shnum<=0
// false), so the mutant would ACCEPT -> open succeeds. Pinning "rejected"
// flips it. Also pins the `!=` compare.
bool test_el2_rejects_bad_shentsize(void) {
    build_elf_blob();
    u8 bad[ELF_FIXTURE_SIZE];
    MemCopy(bad, elf_blob, sizeof(bad));
    wr_u16(&bad[OFF_E_SHENTSIZE], 0); // e_shentsize = 0, with shnum > 0
    return elf_rejects(bad, sizeof(bad));
}

bool test_el2_rejects_bad_shentsize_65(void) {
    build_elf_blob();
    u8 bad[ELF_FIXTURE_SIZE];
    MemCopy(bad, elf_blob, sizeof(bad));
    wr_u16(&bad[OFF_E_SHENTSIZE], 65); // e_shentsize = 65, with shnum > 0
    return elf_rejects(bad, sizeof(bad));
}

// L456: `fail:` path runs `ElfDeinit(out)`, which zeroes `out` per the
// documented FAILURE contract ("out is left zeroed"). Remove-void-call
// would leave `out` carrying the partially decoded header + sections.
// Force a failure AFTER sections are populated (symtab range past EOF),
// then assert `out` is zeroed.
bool test_el2_failed_open_zeroes_out(void) {
    build_elf_blob();
    u8 bad[ELF_FIXTURE_SIZE];
    MemCopy(bad, elf_blob, sizeof(bad));
    // .symtab is section index 3; sh_size lives at SHT_OFF + 3*64 + 32.
    // Blow it past EOF so decode_symbols fails after decode_sections has
    // already filled out->sections and out->header.
    wr_u64(&bad[SHT_OFF + SEC_SYMTAB * SHDR_SIZE + 32], (u64)sizeof(bad) + 0x1000);

    DefaultAllocator alloc = DefaultAllocatorInit();
    Elf              elf;
    bool             opened = ElfOpenFromMemoryCopy(&elf, bad, sizeof(bad), ALLOCATOR_OF(&alloc));
    if (opened) {
        // Should not happen on real code; clean up and fail the test.
        ElfDeinit(&elf);
        DefaultAllocatorDeinit(&alloc);
        return false;
    }
    // Failure contract: `out` is left zeroed. ElfDeinit MemSets the whole
    // struct, so the header and section vector must read back empty.
    bool ok = elf.header.machine == 0 && elf.header.shnum == 0 && elf.header.shoff == 0 && elf.header.entry == 0 &&
              VecLen(&elf.sections) == 0 && VecLen(&elf.symbols) == 0;
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// elf_open path: a real on-disk valid ELF (this test binary itself) opens
// successfully through FileReadAndClose -> ElfOpenFromMemory. Pins the
// `FileReadAndClose(...) < 0` success path: a positive byte count must
// proceed to a successful parse.
bool test_el2_open_real_file_succeeds(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Elf              elf;
    bool             opened = ElfOpen(&elf, "/proc/self/exe", ALLOCATOR_OF(&alloc));
    bool             ok     = opened && elf.header.machine == EM_SELF && VecLen(&elf.sections) > 0;
    if (opened)
        ElfDeinit(&elf);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// Elf.Mutants3: symbol-table / address-search / debug-link / build-id
// paths. Defines its own symbol-table, build-id, and debuglink fixtures
// plus the wr_sym / wr_ehdr (5-arg, etype-parameterised) / wr_note_hdr
// helpers. wr_shdr is reused from the baseline.
// ---------------------------------------------------------------------------

// Write a 24-byte ELF64 LE symbol-table entry at p.
static void wr_sym(u8 *p, u32 name, u8 bind, u8 type, u16 shndx, u64 value, u64 size) {
    wr_u32(&p[0], name);
    p[4] = (u8)((bind << 4) | (type & 0xf)); // st_info
    p[5] = 0;                                // st_other
    wr_u16(&p[6], shndx);
    wr_u64(&p[8], value);
    wr_u64(&p[16], size);
}

// Write the 64-byte ELF64 LE file header (little-endian executable) at the
// start of blob, pointing the section header table at shoff with `shnum`
// sections and `shstrndx` as the section-name string table index.
static void wr_ehdr(u8 *blob, u16 etype, u64 shoff, u16 shnum, u16 shstrndx) {
    blob[0] = 0x7f;
    blob[1] = 'E';
    blob[2] = 'L';
    blob[3] = 'F';
    blob[4] = (u8)ELF_CLASS_64;
    blob[5] = (u8)ELF_DATA_LSB;
    blob[6] = 1;

    u8 *h = &blob[16];
    wr_u16(&h[0], etype);
    wr_u16(&h[2], EM_X86_64);
    wr_u32(&h[4], 1);
    wr_u64(&h[8], 0);          // e_entry
    wr_u64(&h[16], 0);         // e_phoff
    wr_u64(&h[24], shoff);     // e_shoff
    wr_u32(&h[32], 0);         // e_flags
    wr_u16(&h[36], EHDR_SIZE); // e_ehsize
    wr_u16(&h[38], 0);
    wr_u16(&h[40], 0);
    wr_u16(&h[42], SHDR_SIZE); // e_shentsize
    wr_u16(&h[44], shnum);     // e_shnum
    wr_u16(&h[46], shstrndx);  // e_shstrndx
}

// Symbol-table fixture: header + .shstrtab + .strtab + .symtab + .dynsym +
// .dynstr. Symbols placed so the address-search boundaries are exercisable.
enum {
    SYM_FIX_SIZE = 0x340,

    SF_SHSTR_OFF  = 0x40,
    SF_STRTAB_OFF = 0x80,
    SF_DYNSTR_OFF = 0xA0,
    SF_SYMTAB_OFF = 0xC0,
    SF_DYNSYM_OFF = 0x140,
    SF_SHT_OFF    = 0x180,

    SF_N_SECTIONS = 7,

    // section name offsets within .shstrtab
    SF_NM_SHSTRTAB = 1,
    SF_NM_SYMTAB   = 11,
    SF_NM_STRTAB   = 19,
    SF_NM_DYNSYM   = 27,
    SF_NM_DYNSTR   = 35,

    // section indices
    SF_SEC_NULL     = 0,
    SF_SEC_SHSTRTAB = 1,
    SF_SEC_STRTAB   = 2,
    SF_SEC_DYNSTR   = 3,
    SF_SEC_SYMTAB   = 4,
    SF_SEC_DYNSYM   = 5,
    SF_SEC_TEXT     = 6, // a PROGBITS placeholder so st_shndx points somewhere real

    // .strtab name offsets: "\0alpha\0beta\0local\0beta_lo\0"
    SF_STR_ALPHA  = 1,
    SF_STR_BETA   = 7,
    SF_STR_LOCAL  = 12,
    SF_STR_BETALO = 18,

    // .dynstr name offsets: "\0dyn_sym\0"
    SF_DYN_NAME = 1,

    // Symbol values / sizes chosen to exercise resolve boundaries.
    SF_ALPHA_VADDR = 0x1000,
    SF_ALPHA_SIZE  = 0x40,
    SF_BETA_VADDR  = 0x2000,
    SF_BETA_SIZE   = 0,    // zero-size symbol (exact-match path)
    SF_LOCAL_VADDR = 0x1000,
    SF_LOCAL_SIZE  = 0x40, // overlaps alpha exactly -> tie-break by bind

    SF_DYN_VADDR = 0x3000,
    SF_DYN_SIZE  = 0x10,
};

static u8 sym_blob[SYM_FIX_SIZE];

static void build_sym_blob(void) {
    MemSet(sym_blob, 0, sizeof(sym_blob));

    wr_ehdr(sym_blob, (u16)ELF_TYPE_EXEC, SF_SHT_OFF, SF_N_SECTIONS, SF_SEC_SHSTRTAB);

    const char shstr[] = "\0.shstrtab\0.symtab\0.strtab\0.dynsym\0.dynstr\0";
    MemCopy(&sym_blob[SF_SHSTR_OFF], shstr, sizeof(shstr));

    const char strtab[] = "\0alpha\0beta\0local\0beta_lo\0";
    MemCopy(&sym_blob[SF_STRTAB_OFF], strtab, sizeof(strtab));

    const char dynstr[] = "\0dyn_sym\0";
    MemCopy(&sym_blob[SF_DYNSTR_OFF], dynstr, sizeof(dynstr));

    // .symtab: [0]=null, [1]=alpha(GLOBAL FUNC, sized), [2]=beta_lo(LOCAL,
    //          size0 @0x2000, seen first), [3]=beta(GLOBAL, size0 @0x2000,
    //          must win the zero-size tie-break), [4]=local(LOCAL FUNC,
    //          overlaps alpha exactly -> sized tie-break).
    u8 *st = &sym_blob[SF_SYMTAB_OFF];
    wr_sym(&st[0 * SYM_SIZE], 0, 0, 0, 0, 0, 0);
    wr_sym(
        &st[1 * SYM_SIZE],
        SF_STR_ALPHA,
        (u8)ELF_SYMBOL_BIND_GLOBAL,
        (u8)ELF_SYMBOL_TYPE_FUNC,
        SF_SEC_TEXT,
        SF_ALPHA_VADDR,
        SF_ALPHA_SIZE
    );
    wr_sym(
        &st[2 * SYM_SIZE],
        SF_STR_BETALO,
        (u8)ELF_SYMBOL_BIND_LOCAL,
        (u8)ELF_SYMBOL_TYPE_FUNC,
        SF_SEC_TEXT,
        SF_BETA_VADDR,
        SF_BETA_SIZE
    );
    wr_sym(
        &st[3 * SYM_SIZE],
        SF_STR_BETA,
        (u8)ELF_SYMBOL_BIND_GLOBAL,
        (u8)ELF_SYMBOL_TYPE_FUNC,
        SF_SEC_TEXT,
        SF_BETA_VADDR,
        SF_BETA_SIZE
    );
    wr_sym(
        &st[4 * SYM_SIZE],
        SF_STR_LOCAL,
        (u8)ELF_SYMBOL_BIND_LOCAL,
        (u8)ELF_SYMBOL_TYPE_FUNC,
        SF_SEC_TEXT,
        SF_LOCAL_VADDR,
        SF_LOCAL_SIZE
    );

    // .dynsym: [0]=null, [1]=dyn_sym(GLOBAL FUNC)
    u8 *dt = &sym_blob[SF_DYNSYM_OFF];
    wr_sym(&dt[0 * SYM_SIZE], 0, 0, 0, 0, 0, 0);
    wr_sym(
        &dt[1 * SYM_SIZE],
        SF_DYN_NAME,
        (u8)ELF_SYMBOL_BIND_GLOBAL,
        (u8)ELF_SYMBOL_TYPE_FUNC,
        SF_SEC_TEXT,
        SF_DYN_VADDR,
        SF_DYN_SIZE
    );

    u8 *sht = &sym_blob[SF_SHT_OFF];
    wr_shdr(&sht[SF_SEC_NULL * SHDR_SIZE], 0, ELF_SECTION_TYPE_NULL, 0, 0, 0, 0, 0, 0);
    wr_shdr(
        &sht[SF_SEC_SHSTRTAB * SHDR_SIZE],
        SF_NM_SHSTRTAB,
        ELF_SECTION_TYPE_STRTAB,
        0,
        0,
        SF_SHSTR_OFF,
        sizeof(shstr),
        0,
        0
    );
    wr_shdr(
        &sht[SF_SEC_STRTAB * SHDR_SIZE],
        SF_NM_STRTAB,
        ELF_SECTION_TYPE_STRTAB,
        0,
        0,
        SF_STRTAB_OFF,
        sizeof(strtab),
        0,
        0
    );
    wr_shdr(
        &sht[SF_SEC_DYNSTR * SHDR_SIZE],
        SF_NM_DYNSTR,
        ELF_SECTION_TYPE_STRTAB,
        0,
        0,
        SF_DYNSTR_OFF,
        sizeof(dynstr),
        0,
        0
    );
    wr_shdr(
        &sht[SF_SEC_SYMTAB * SHDR_SIZE],
        SF_NM_SYMTAB,
        ELF_SECTION_TYPE_SYMTAB,
        0,
        0,
        SF_SYMTAB_OFF,
        5 * SYM_SIZE,
        SF_SEC_STRTAB, // sh_link -> .strtab
        SYM_SIZE
    );
    wr_shdr(
        &sht[SF_SEC_DYNSYM * SHDR_SIZE],
        SF_NM_DYNSYM,
        ELF_SECTION_TYPE_DYNSYM,
        0,
        0,
        SF_DYNSYM_OFF,
        2 * SYM_SIZE,
        SF_SEC_DYNSTR, // sh_link -> .dynstr
        SYM_SIZE
    );
    wr_shdr(&sht[SF_SEC_TEXT * SHDR_SIZE], 0, ELF_SECTION_TYPE_PROGBITS, 0x6, 0x1000, 0x40, 0x40, 0, 0);
}

static bool open_sym(Elf *elf, DefaultAllocator *alloc) {
    build_sym_blob();
    return ElfOpenFromMemoryCopy(elf, sym_blob, sizeof(sym_blob), ALLOCATOR_OF(alloc));
}

// --- elf_decode_symbol_table: exact symbol count from size / entsize -------
// .symtab declares size = 5*24 with entsize 24 -> exactly 5 symbols. An
// off-by-one in the count loop or a wrong divisor changes this count.
bool test_el3_symtab_count_exact(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Elf              elf;
    if (!open_sym(&elf, &alloc)) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }
    bool ok = VecLen(&elf.symbols) == 5;
    ElfDeinit(&elf);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// --- elf_decode_symbol_table: st_name -> .strtab name decodes exactly ------
// Pins st_name offset arithmetic for both alpha and local.
bool test_el3_symtab_names_decode(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Elf              elf;
    if (!open_sym(&elf, &alloc)) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }
    const ElfSymbol *a = VecPtrAt(&elf.symbols, 1);
    const ElfSymbol *b = VecPtrAt(&elf.symbols, 3);
    const ElfSymbol *l = VecPtrAt(&elf.symbols, 4);
    bool ok = a->name && ZstrCompare(a->name, "alpha") == 0 && b->name && ZstrCompare(b->name, "beta") == 0 &&
              l->name && ZstrCompare(l->name, "local") == 0;
    ElfDeinit(&elf);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// --- elf_decode_symbol_table: full value/size/bind/type/shndx of a sym -----
bool test_el3_symtab_fields_intact(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Elf              elf;
    if (!open_sym(&elf, &alloc)) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }
    const ElfSymbol *a  = VecPtrAt(&elf.symbols, 1);
    bool             ok = a->value == SF_ALPHA_VADDR && a->size == SF_ALPHA_SIZE && a->type == ELF_SYMBOL_TYPE_FUNC &&
              a->bind == ELF_SYMBOL_BIND_GLOBAL && a->section_index == SF_SEC_TEXT;
    ElfDeinit(&elf);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// --- elf_decode_symbols: the .dynsym table is decoded too (the second
// elf_decode_symbol_table call must run; a replaced/short-circuited call
// leaves dynamic_symbols empty). ---------------------------------------
bool test_el3_dynsym_decoded(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Elf              elf;
    if (!open_sym(&elf, &alloc)) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }
    bool ok = VecLen(&elf.dynamic_symbols) == 2;
    if (ok) {
        const ElfSymbol *d = VecPtrAt(&elf.dynamic_symbols, 1);
        ok                 = d->name && ZstrCompare(d->name, "dyn_sym") == 0 && d->value == SF_DYN_VADDR;
    }
    ElfDeinit(&elf);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// --- elf_decode_symbol_table: a strtab (symtab->link) pointing at an
// out-of-range section is rejected. Pins the elf_range_ok(strtab,...)
// guard: skipping it would read the strtab out of bounds. ------------------
bool test_el3_symtab_strtab_out_of_range_rejected(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    build_sym_blob();
    // Corrupt the .strtab section header (sh_offset) to point past EOF.
    // .symtab's sh_link is SF_SEC_STRTAB, so this is the table the symbol
    // decoder must range-check before reading names from it.
    wr_u64(&sym_blob[SF_SHT_OFF + SF_SEC_STRTAB * SHDR_SIZE + 24], (u64)sizeof(sym_blob) + 0x1000);

    Elf  elf;
    bool opened = ElfOpenFromMemoryCopy(&elf, sym_blob, sizeof(sym_blob), ALLOCATOR_OF(&alloc));
    if (opened)
        ElfDeinit(&elf);
    DefaultAllocatorDeinit(&alloc);
    return !opened; // must be rejected
}

// Sized-symbol boundaries: addr at value, value+size-1 match; value+size
// and value-1 miss. (Pins the >= / < comparisons.)
bool test_el3_resolve_sized_boundaries(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Elf              elf;
    if (!open_sym(&elf, &alloc)) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }
    const ElfSymbol *s;
    bool             ok = true;

    s  = ElfResolveAddress(&elf, SF_ALPHA_VADDR);                     // exact start
    ok = ok && s && s->name && ZstrCompare(s->name, "alpha") == 0;
    s  = ElfResolveAddress(&elf, SF_ALPHA_VADDR + SF_ALPHA_SIZE - 1); // last byte
    ok = ok && s && s->name && ZstrCompare(s->name, "alpha") == 0;
    s  = ElfResolveAddress(&elf, SF_ALPHA_VADDR + SF_ALPHA_SIZE);     // just past
    ok = ok && (s == NULL || ZstrCompare(s->name, "alpha") != 0);

    ElfDeinit(&elf);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Zero-size symbol (beta): matches only at the exact value. Pins the
// `s->value == vaddr` equality in the zero-size branch.
bool test_el3_resolve_zero_size_exact(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Elf              elf;
    if (!open_sym(&elf, &alloc)) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }
    const ElfSymbol *hit  = ElfResolveAddress(&elf, SF_BETA_VADDR);
    const ElfSymbol *miss = ElfResolveAddress(&elf, SF_BETA_VADDR + 1);
    bool             ok   = hit && hit->name && ZstrCompare(hit->name, "beta") == 0 && miss == NULL;
    ElfDeinit(&elf);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Tie-break in the zero-size path: `beta_lo` (LOCAL) and `beta` (GLOBAL)
// both have size 0 and value 0x2000. `beta_lo` is seen first; `beta` must
// displace it because it is GLOBAL. Pins `s->bind == ELF_SYMBOL_BIND_GLOBAL`
// in the zero-size branch -- flipping to != keeps the LOCAL match.
bool test_el3_resolve_prefers_global_zero_size(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Elf              elf;
    if (!open_sym(&elf, &alloc)) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }
    const ElfSymbol *s  = ElfResolveAddress(&elf, SF_BETA_VADDR);
    bool             ok = s && s->name && ZstrCompare(s->name, "beta") == 0 && s->bind == ELF_SYMBOL_BIND_GLOBAL;
    ElfDeinit(&elf);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Tie-break in the sized path: `alpha` (GLOBAL) and `local` (LOCAL) cover
// the identical range. The resolver must prefer the GLOBAL symbol. Pins
// `s->bind == ELF_SYMBOL_BIND_GLOBAL` in the sized branch -- flipping it
// to != would keep the first (LOCAL) match.
bool test_el3_resolve_prefers_global_sized(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Elf              elf;
    if (!open_sym(&elf, &alloc)) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }
    // alpha is symbol[1] (GLOBAL), local is symbol[3] (LOCAL); both cover
    // [0x1000, 0x1040). best starts at alpha; local is seen later and must
    // NOT displace it because local is not GLOBAL.
    const ElfSymbol *s  = ElfResolveAddress(&elf, SF_ALPHA_VADDR + 4);
    bool             ok = s && s->name && ZstrCompare(s->name, "alpha") == 0 && s->bind == ELF_SYMBOL_BIND_GLOBAL;
    ElfDeinit(&elf);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// build-id fixtures. Generic builder: an ELF with .shstrtab + a NOTE
// section whose payload we control byte-for-byte, so each note-header /
// alignment mutation changes the decoded build_id.
enum {
    BID_SIZE         = 0x200,
    BID_SHSTR_OFF    = 0x40,
    BID_NOTE_OFF     = 0x80,
    BID_SHT_OFF      = 0xC0,
    BID_N_SECTIONS   = 3,
    BID_NM_SHSTRTAB  = 1,
    BID_NM_NOTE      = 11,
    BID_SEC_SHSTRTAB = 1,
    BID_SEC_NOTE     = 2,
    NT_GNU_BUILD_ID  = 3,
};

// Build an ELF whose `.note.gnu.build-id` section is `note_size` bytes of
// raw `note_payload` at BID_NOTE_OFF. note_off_override, when non-zero,
// replaces the section's sh_offset (to drive out-of-range tests).
static u8 bid_blob[BID_SIZE];

static void build_bid_blob(const u8 *note_payload, u32 note_size, u64 note_off_override) {
    MemSet(bid_blob, 0, sizeof(bid_blob));
    wr_ehdr(bid_blob, (u16)ELF_TYPE_DYN, BID_SHT_OFF, BID_N_SECTIONS, BID_SEC_SHSTRTAB);

    const char shstr[] = "\0.shstrtab\0.note.gnu.build-id\0";
    MemCopy(&bid_blob[BID_SHSTR_OFF], shstr, sizeof(shstr));

    if (note_payload && note_size)
        MemCopy(&bid_blob[BID_NOTE_OFF], note_payload, note_size);

    u64 note_off = note_off_override ? note_off_override : (u64)BID_NOTE_OFF;

    u8 *sht = &bid_blob[BID_SHT_OFF];
    wr_shdr(&sht[0], 0, ELF_SECTION_TYPE_NULL, 0, 0, 0, 0, 0, 0);
    wr_shdr(
        &sht[BID_SEC_SHSTRTAB * SHDR_SIZE],
        BID_NM_SHSTRTAB,
        ELF_SECTION_TYPE_STRTAB,
        0,
        0,
        BID_SHSTR_OFF,
        sizeof(shstr),
        0,
        0
    );
    wr_shdr(&sht[BID_SEC_NOTE * SHDR_SIZE], BID_NM_NOTE, ELF_SECTION_TYPE_NOTE, 0, 0, note_off, note_size, 0, 0);
}

// Write a build-id note header (namesz, descsz, type) + "GNU\0" name into
// dst and return the byte count written (= 16). Caller appends the desc.
static void wr_note_hdr(u8 *dst, u32 namesz, u32 descsz, u32 type) {
    wr_u32(&dst[0], namesz);
    wr_u32(&dst[4], descsz);
    wr_u32(&dst[8], type);
    dst[12] = 'G';
    dst[13] = 'N';
    dst[14] = 'U';
    dst[15] = '\0';
}

// Well-formed build-id of `desc_len` bytes decodes to those exact bytes.
bool test_el3_build_id_decodes(void) {
    DefaultAllocator alloc  = DefaultAllocatorInit();
    static const u8  desc[] = {0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x11, 0x22};
    u8               note[16 + sizeof(desc)];
    wr_note_hdr(note, 4, (u32)sizeof(desc), NT_GNU_BUILD_ID);
    MemCopy(&note[16], desc, sizeof(desc));
    build_bid_blob(note, (u32)sizeof(note), 0);

    Elf elf;
    if (!ElfOpenFromMemoryCopy(&elf, bid_blob, sizeof(bid_blob), ALLOCATOR_OF(&alloc))) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }
    bool ok =
        elf.build_id != NULL && elf.build_id_size == sizeof(desc) && MemCompare(elf.build_id, desc, sizeof(desc)) == 0;
    ElfDeinit(&elf);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Boundary: a note section of EXACTLY 16 bytes (header only, descsz 0) is
// the minimum the decoder accepts (`note->size < 16` is a strict `<`). The
// decoder must accept it and set build_id (with size 0). `<=` would reject.
bool test_el3_build_id_size16_boundary(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    u8               note[16];
    wr_note_hdr(note, 4, 0, NT_GNU_BUILD_ID); // descsz = 0
    build_bid_blob(note, 16, 0);

    Elf elf;
    if (!ElfOpenFromMemoryCopy(&elf, bid_blob, sizeof(bid_blob), ALLOCATOR_OF(&alloc))) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }
    bool ok = elf.build_id != NULL && elf.build_id_size == 0;
    ElfDeinit(&elf);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A note whose descsz overruns the section is rejected (build_id stays
// NULL). Pins `(end - p) < name_padded + descsz`: replacing the
// subtraction with addition would let the overrun through.
bool test_el3_build_id_descsz_overrun_rejected(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    // 16-byte note: header says namesz=4, descsz=0xFFFFFFF0 (huge). Only
    // 0 desc bytes follow -> must be rejected.
    u8 note[16];
    wr_note_hdr(note, 4, 0xFFFFFFF0u, NT_GNU_BUILD_ID);
    build_bid_blob(note, 16, 0);

    Elf elf;
    if (!ElfOpenFromMemoryCopy(&elf, bid_blob, sizeof(bid_blob), ALLOCATOR_OF(&alloc))) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }
    bool ok = elf.build_id == NULL && elf.build_id_size == 0;
    ElfDeinit(&elf);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A note whose namesz is huge (so name_padded + descsz overruns) is
// rejected. Pins `(end - p) < name_padded + descsz` against the
// name-padding term specifically: sub->add lets the huge name_padded
// slip through.
bool test_el3_build_id_namesz_overrun_rejected(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    u8               note[16];
    wr_note_hdr(note, 0xFFFFFFF0u, 0, NT_GNU_BUILD_ID); // namesz huge, descsz 0
    build_bid_blob(note, 16, 0);

    Elf elf;
    if (!ElfOpenFromMemoryCopy(&elf, bid_blob, sizeof(bid_blob), ALLOCATOR_OF(&alloc))) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }
    bool ok = elf.build_id == NULL && elf.build_id_size == 0;
    ElfDeinit(&elf);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A note section whose sh_offset points past EOF must be range-rejected:
// build_id stays NULL and no out-of-bounds read happens. Pins the
// `elf_range_ok(self, note->offset, note->size)` guard (replacing the
// call with a constant truthy would read past the buffer).
bool test_el3_build_id_out_of_range_section(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    u8               note[16];
    wr_note_hdr(note, 4, 0, NT_GNU_BUILD_ID);
    // Point the note section's sh_offset past EOF.
    build_bid_blob(note, 16, (u64)sizeof(bid_blob) + 0x1000);

    Elf elf;
    if (!ElfOpenFromMemoryCopy(&elf, bid_blob, sizeof(bid_blob), ALLOCATOR_OF(&alloc))) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }
    bool ok = elf.build_id == NULL && elf.build_id_size == 0;
    ElfDeinit(&elf);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// .gnu_debuglink fixtures. Generic builder mirrors the build-id one.
enum {
    DL_SIZE         = 0x200,
    DL_SHSTR_OFF    = 0x40,
    DL_DATA_OFF     = 0x80,
    DL_SHT_OFF      = 0xC0,
    DL_N_SECTIONS   = 3,
    DL_NM_SHSTRTAB  = 1,
    DL_NM_DL        = 11,
    DL_SEC_SHSTRTAB = 1,
    DL_SEC_DL       = 2,
};

static u8 dl_blob[DL_SIZE];

static void build_dl_blob(const u8 *payload, u32 dl_size, u64 off_override) {
    MemSet(dl_blob, 0, sizeof(dl_blob));
    wr_ehdr(dl_blob, (u16)ELF_TYPE_DYN, DL_SHT_OFF, DL_N_SECTIONS, DL_SEC_SHSTRTAB);

    const char shstr[] = "\0.shstrtab\0.gnu_debuglink\0";
    MemCopy(&dl_blob[DL_SHSTR_OFF], shstr, sizeof(shstr));

    if (payload && dl_size)
        MemCopy(&dl_blob[DL_DATA_OFF], payload, dl_size);

    u64 off = off_override ? off_override : (u64)DL_DATA_OFF;

    u8 *sht = &dl_blob[DL_SHT_OFF];
    wr_shdr(&sht[0], 0, ELF_SECTION_TYPE_NULL, 0, 0, 0, 0, 0, 0);
    wr_shdr(
        &sht[DL_SEC_SHSTRTAB * SHDR_SIZE],
        DL_NM_SHSTRTAB,
        ELF_SECTION_TYPE_STRTAB,
        0,
        0,
        DL_SHSTR_OFF,
        sizeof(shstr),
        0,
        0
    );
    wr_shdr(&sht[DL_SEC_DL * SHDR_SIZE], DL_NM_DL, ELF_SECTION_TYPE_PROGBITS, 0, 0, off, dl_size, 0, 0);
}

// Well-formed .gnu_debuglink: "abc.debug\0" padded + crc decodes to the
// exact filename and CRC. Pins the name scan and the trailing-4-byte CRC.
bool test_el3_debuglink_decodes(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    // payload: "abc.debug\0" (10) padded to 12, then 4-byte crc -> 16 bytes
    u8 payload[16];
    MemSet(payload, 0, sizeof(payload));
    const char fname[] = "abc.debug";
    MemCopy(payload, fname, sizeof(fname)); // includes NUL
    wr_u32(&payload[12], 0x0a0b0c0du);
    build_dl_blob(payload, 16, 0);

    Elf elf;
    if (!ElfOpenFromMemoryCopy(&elf, dl_blob, sizeof(dl_blob), ALLOCATOR_OF(&alloc))) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }
    bool ok = elf.debuglink_name != NULL && ZstrCompare(elf.debuglink_name, "abc.debug") == 0 &&
              elf.debuglink_crc == 0x0a0b0c0du;
    ElfDeinit(&elf);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A .gnu_debuglink whose filename runs the full `max_name` window with no
// NUL terminator is rejected (debuglink_name stays NULL). Pins both
// `dl->size - 4` (the max_name bound; `+4` would scan into the CRC) and
// `name_len >= max_name` (`>` would accept the unterminated name).
bool test_el3_debuglink_unterminated_rejected(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    // size 16 -> max_name = 12. Fill [0,12) with non-NUL bytes (no
    // terminator inside the name window); the last 4 bytes (the CRC) are
    // ALL non-zero too, so a `+4` mis-bound would also fail to terminate
    // until past the section. Real code: name_len reaches max_name -> reject.
    u8 payload[16];
    for (int i = 0; i < 12; ++i)
        payload[i] = (u8)('A' + i);
    wr_u32(&payload[12], 0x01010101u); // crc bytes all non-zero
    build_dl_blob(payload, 16, 0);

    Elf elf;
    if (!ElfOpenFromMemoryCopy(&elf, dl_blob, sizeof(dl_blob), ALLOCATOR_OF(&alloc))) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }
    bool ok = elf.debuglink_name == NULL;
    ElfDeinit(&elf);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A .gnu_debuglink whose name region terminates immediately (empty name)
// is rejected. Pins the `name_len == 0` / name_len init guard.
bool test_el3_debuglink_empty_name_rejected(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    u8               payload[16];
    MemSet(payload, 0, sizeof(payload)); // base[0] == '\0' -> empty name
    wr_u32(&payload[12], 0x01020304u);
    build_dl_blob(payload, 16, 0);

    Elf elf;
    if (!ElfOpenFromMemoryCopy(&elf, dl_blob, sizeof(dl_blob), ALLOCATOR_OF(&alloc))) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }
    bool ok = elf.debuglink_name == NULL;
    ElfDeinit(&elf);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A .gnu_debuglink section whose sh_offset points past EOF is range-
// rejected (debuglink_name stays NULL; no OOB read). Pins the
// `elf_range_ok(self, dl->offset, dl->size)` guard.
bool test_el3_debuglink_out_of_range_section(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    u8               payload[16];
    MemSet(payload, 0, sizeof(payload));
    const char fname[] = "abc.debug";
    MemCopy(payload, fname, sizeof(fname));
    wr_u32(&payload[12], 0x0a0b0c0du);
    build_dl_blob(payload, 16, (u64)sizeof(dl_blob) + 0x1000);

    Elf elf;
    if (!ElfOpenFromMemoryCopy(&elf, dl_blob, sizeof(dl_blob), ALLOCATOR_OF(&alloc))) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }
    bool ok = elf.debuglink_name == NULL;
    ElfDeinit(&elf);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// Elf.Mutants4: section-table-overrun and build-id / debuglink range guards.
// Uses its own 4-arg executable-header writer (wr_ehdr_exec, renamed from the
// staging file's wr_ehdr to avoid colliding with the 5-arg one above) and a
// debug-metadata builder. wr_shdr, kBuildId, and the DBG_* enum are reused.
// ---------------------------------------------------------------------------

// Write a valid ELF64 LE executable header at the start of blob.
static void wr_ehdr_exec(u8 *blob, u64 shoff, u16 shnum, u16 shstrndx) {
    blob[0] = 0x7f;
    blob[1] = 'E';
    blob[2] = 'L';
    blob[3] = 'F';
    blob[4] = (u8)ELF_CLASS_64;
    blob[5] = (u8)ELF_DATA_LSB;
    blob[6] = 1;

    u8 *h = &blob[16];
    wr_u16(&h[0], (u16)ELF_TYPE_EXEC);
    wr_u16(&h[2], EM_X86_64);
    wr_u32(&h[4], 1);
    wr_u64(&h[8], 0);      // e_entry
    wr_u64(&h[16], 0);     // e_phoff
    wr_u64(&h[24], shoff); // e_shoff
    wr_u32(&h[32], 0);     // e_flags
    wr_u16(&h[36], EHDR_SIZE);
    wr_u16(&h[38], 0);
    wr_u16(&h[40], 0);
    wr_u16(&h[42], SHDR_SIZE);
    wr_u16(&h[44], shnum);
    wr_u16(&h[46], shstrndx);
}

// Site 1 (Elf.c:194) cxx_init_const: `needed = (u64)n * SHDR64_SIZE` -> `42`.
//
// `needed` is the byte size of the section-header table fed to
// elf_range_ok(self, shoff, needed). Craft a file where the section table
// truly overruns (shnum*64 > bytes left after shoff) BUT 42 <= bytes left.
// Real code rejects (range check fails). Mutant (42 <= remaining) accepts and
// would over-read. We assert the real parser REJECTS the file.
//
// Layout: header at 0, section table at e_shoff such that only ~50 bytes
// remain after it. shnum=2 -> needs 128 (overruns), but 42 <= 50 (mutant
// accepts). We make the file just big enough that 42 fits but 128 doesn't.
bool test_ef_section_table_overrun_rejected(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    // File is 64 (header) + 50 bytes of slack = 114. e_shoff = 64, so only
    // 50 bytes remain after shoff. shnum=2 needs 128 (overrun); 42 <= 50.
    enum {
        SHOFF    = 64,
        SLACK    = 50,
        BLOB_LEN = SHOFF + SLACK,
        SHNUM    = 2,
    };
    u8 blob[BLOB_LEN];
    MemSet(blob, 0, sizeof(blob));
    wr_ehdr_exec(blob, SHOFF, SHNUM, /*shstrndx=*/0);

    Elf  elf;
    bool opened = ElfOpenFromMemoryCopy(&elf, blob, sizeof(blob), ALLOCATOR_OF(&alloc));
    if (opened)
        ElfDeinit(&elf);
    DefaultAllocatorDeinit(&alloc);
    // Real code: range check on 128 fails -> rejected. Mutant: 42 passes,
    // proceeds (and would over-read the truncated table).
    return !opened;
}

// Shared builder for the debug-metadata fixtures (sites 2 and 3). A tiny
// ELF64 with .shstrtab, a NOTE section (.note.gnu.build-id), and a PROGBITS
// section (.gnu_debuglink). Callers can request that the note's or the
// debuglink's sh_offset/sh_size overrun the file to exercise the range guard.
//
// note_off/note_size and dl_off/dl_size let callers craft overrunning
// sections; pass the "good" defaults for a valid image.
static void build_dbg(u8 *blob, u64 note_off, u64 note_size, u64 dl_off, u64 dl_size) {
    MemSet(blob, 0, DBG_BLOB_SIZE);
    wr_ehdr_exec(blob, DBG_SHT_OFF, DBG_N_SEC, DBG_SEC_SHSTRTAB);
    blob[16] = (u8)ELF_TYPE_DYN; // e_type low byte (DYN); harmless cosmetic

    const char shstr[DBG_SHSTR_SIZE] = "\0.shstrtab\0.note.gnu.build-id\0.gnu_debuglink\0";
    MemCopy(&blob[DBG_SHSTR_OFF], shstr, DBG_SHSTR_SIZE);

    // build-id note: namesz=4, descsz=8, type=3, name "GNU\0", desc=kBuildId.
    u8 *note = &blob[DBG_NOTE_OFF];
    wr_u32(&note[0], 4);
    wr_u32(&note[4], BUILD_ID_LEN);
    wr_u32(&note[8], 3);
    note[12] = 'G';
    note[13] = 'N';
    note[14] = 'U';
    note[15] = '\0';
    MemCopy(&note[16], kBuildId, BUILD_ID_LEN);

    // .gnu_debuglink: "foo.debug\0" + pad + crc32.
    u8        *dl      = &blob[DBG_DL_OFF];
    const char fname[] = "foo.debug";
    MemCopy(dl, fname, sizeof(fname));
    wr_u32(&dl[DBG_DL_SIZE - 4], DEBUGLINK_CRC);

    u8 *sht = &blob[DBG_SHT_OFF];
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
    wr_shdr(&sht[DBG_SEC_NOTE * SHDR_SIZE], DBG_NAME_NOTE, ELF_SECTION_TYPE_NOTE, 0, 0, note_off, note_size, 0, 0);
    wr_shdr(&sht[DBG_SEC_DL * SHDR_SIZE], DBG_NAME_DEBUGLINK, ELF_SECTION_TYPE_PROGBITS, 0, 0, dl_off, dl_size, 0, 0);
}

// Baseline: a valid debug-metadata fixture decodes build-id and debuglink.
// (Anchors sites 2/3 -- they must still produce the right answer on valid
// input, so a mutant that always-skips the guard can't trivially pass by
// breaking the valid case too.)
bool test_ef_debug_metadata_valid(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    u8               blob[DBG_BLOB_SIZE];
    build_dbg(blob, DBG_NOTE_OFF, DBG_NOTE_SIZE, DBG_DL_OFF, DBG_DL_SIZE);

    Elf elf;
    if (!ElfOpenFromMemoryCopy(&elf, blob, sizeof(blob), ALLOCATOR_OF(&alloc))) {
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

// Site 2 (Elf.c:356) cxx_replace_scalar_call: `elf_range_ok(...)` -> 42, so
// `!42` is false and the range guard in elf_decode_build_id is SKIPPED.
//
// Craft the .note.gnu.build-id section so its sh_offset/sh_size overruns the
// file. Real code's range_ok returns false -> build_id stays NULL/0. Mutant
// skips the guard and over-reads. Assert the build-id is ABSENT.
//
// Note size is kept >= 16 (so the `note->size < 16` sibling guard does not
// short-circuit) and offset+size pushed past EOF.
bool test_ef_build_id_overrun_rejected(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    u8               blob[DBG_BLOB_SIZE];
    // Keep the note's offset at the valid, in-bounds header (so a guard-
    // skipping mutant finds a well-formed note and DOES set build_id), but
    // declare a sh_size that runs past EOF. Real code's range_ok rejects the
    // section outright -> build_id stays NULL. Mutant skips the guard, reads
    // the in-bounds header, and publishes build_id -> observable divergence.
    u64 bad_note_size = (u64)DBG_BLOB_SIZE; // offset 0x70 + this >> EOF
    build_dbg(blob, DBG_NOTE_OFF, bad_note_size, DBG_DL_OFF, DBG_DL_SIZE);

    Elf elf;
    if (!ElfOpenFromMemoryCopy(&elf, blob, sizeof(blob), ALLOCATOR_OF(&alloc))) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }
    // Real code: range guard rejects -> no build-id. (debuglink still valid,
    // but we only assert build-id here.)
    bool ok = elf.build_id == NULL && elf.build_id_size == 0;
    ElfDeinit(&elf);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Site 3 (Elf.c:387) cxx_replace_scalar_call: `elf_range_ok(...)` -> 42 in
// elf_decode_debug_link, skipping the range guard.
//
// Craft .gnu_debuglink so its sh_offset/sh_size overruns the file. Real code
// rejects -> debuglink_name stays NULL. Mutant over-reads. Assert debuglink
// is ABSENT. Size kept >= 5 so the sibling `dl->size < 5` guard doesn't fire.
bool test_ef_debug_link_overrun_rejected(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    u8               blob[DBG_BLOB_SIZE];
    // Keep the debuglink payload at its valid, in-bounds offset (so a guard-
    // skipping mutant finds a well-formed "foo.debug\0...crc" and DOES set
    // debuglink_name), but declare a sh_size running past EOF. Real code's
    // range_ok rejects -> debuglink_name stays NULL. Mutant publishes it.
    u64 bad_dl_size = (u64)DBG_BLOB_SIZE; // offset 0x88 + this >> EOF
    build_dbg(blob, DBG_NOTE_OFF, DBG_NOTE_SIZE, DBG_DL_OFF, bad_dl_size);

    Elf elf;
    if (!ElfOpenFromMemoryCopy(&elf, blob, sizeof(blob), ALLOCATOR_OF(&alloc))) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }
    bool ok = elf.debuglink_name == NULL && elf.debuglink_crc == 0;
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
        test_elf_decodes_program_headers,
        test_elf_rejects_phdr_table_out_of_range,
        test_elf_rejects_shstrndx_equals_shnum,
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

        // --- moved from Elf.Mutants1 ---
        test_el1_section_table_exact,
        test_el1_text_section_fields,
        test_el1_section_info_field,
        test_el1_sht_count_overruns_rejected,
        test_el1_shstrndx_equals_count_rejected,
        test_el1_shstrtab_overruns_rejected,
        test_el1_valid_shstrtab_accepted,
        test_el1_range_offset_at_eof_zero_size,
        test_el1_range_size_equals_file_accepted,
        test_el1_range_offset_plus_size_overruns_rejected,
        test_el1_strtab_unterminated_name_is_empty,
        test_el1_strtab_nul_at_size_boundary_is_empty,
        test_el1_strtab_forward_scan_name,
        test_el1_strtab_nonzero_offset_name,
        test_el1_find_section_last_and_absent,

        // --- moved from Elf.Mutants2 ---
        test_el2_self_exe_header_exact,
        test_el2_fixture_header_exact,
        test_el2_min64_opens,
        test_el2_min63_rejected,
        test_el2_valid_fixture_opens,
        test_el2_rejects_mag0,
        test_el2_rejects_mag1,
        test_el2_rejects_mag2,
        test_el2_rejects_mag3,
        test_el2_rejects_class32,
        test_el2_rejects_class_invalid,
        test_el2_rejects_big_endian,
        test_el2_rejects_data_invalid,
        test_el2_rejects_class_bumped,
        test_el2_rejects_truncated_40,
        test_el2_rejects_truncated_zeroish,
        test_el2_rejects_bad_shentsize,
        test_el2_rejects_bad_shentsize_65,
        test_el2_failed_open_zeroes_out,
        test_el2_open_real_file_succeeds,

        // --- moved from Elf.Mutants3 ---
        test_el3_symtab_count_exact,
        test_el3_symtab_names_decode,
        test_el3_symtab_fields_intact,
        test_el3_dynsym_decoded,
        test_el3_symtab_strtab_out_of_range_rejected,
        test_el3_resolve_sized_boundaries,
        test_el3_resolve_zero_size_exact,
        test_el3_resolve_prefers_global_zero_size,
        test_el3_resolve_prefers_global_sized,
        test_el3_build_id_decodes,
        test_el3_build_id_size16_boundary,
        test_el3_build_id_descsz_overrun_rejected,
        test_el3_build_id_namesz_overrun_rejected,
        test_el3_build_id_out_of_range_section,
        test_el3_debuglink_decodes,
        test_el3_debuglink_unterminated_rejected,
        test_el3_debuglink_empty_name_rejected,
        test_el3_debuglink_out_of_range_section,

        // --- moved from Elf.Mutants4 ---
        test_ef_section_table_overrun_rejected,
        test_ef_debug_metadata_valid,
        test_ef_build_id_overrun_rejected,
        test_ef_debug_link_overrun_rejected,
    };

    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), NULL, 0, "Elf");
}
