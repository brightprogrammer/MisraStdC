#include <Misra.h>
#include <Misra/Parsers/Elf.h>
#include <Misra/Std/Allocator/Debug.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/File.h>
#include <Misra/Std/Memory.h>

#include "../Util/TestRunner.h"

// ---------------------------------------------------------------------------
// Little-endian writers into a flat buffer.
// ---------------------------------------------------------------------------
static void bl_wr_u16(u8 *p, u16 v) {
    p[0] = (u8)(v & 0xff);
    p[1] = (u8)(v >> 8);
}
static void bl_wr_u32(u8 *p, u32 v) {
    p[0] = (u8)(v & 0xff);
    p[1] = (u8)(v >> 8);
    p[2] = (u8)(v >> 16);
    p[3] = (u8)(v >> 24);
}
static void bl_wr_u64(u8 *p, u64 v) {
    for (int i = 0; i < 8; ++i)
        p[i] = (u8)((v >> (i * 8)) & 0xff);
}

enum {
    BL_EHDR_SIZE = 64,
    BL_SHDR_SIZE = 64,
    BL_SYM_SIZE  = 24,
    BL_EM_X86_64 = 62,
};

// Write a 64-byte ELF64 LE section header.
static void
    bl_wr_shdr(u8 *p, u32 name, u32 type, u64 flags, u64 addr, u64 offset, u64 size, u32 link, u32 info, u64 entsize) {
    bl_wr_u32(&p[0], name);
    bl_wr_u32(&p[4], type);
    bl_wr_u64(&p[8], flags);
    bl_wr_u64(&p[16], addr);
    bl_wr_u64(&p[24], offset);
    bl_wr_u64(&p[32], size);
    bl_wr_u32(&p[40], link);
    bl_wr_u32(&p[44], info);
    bl_wr_u64(&p[48], 1);
    bl_wr_u64(&p[56], entsize);
}

// Fill a minimal valid ELF64 LE header into buf with the given shoff/shnum/shstrndx.
static void bl_wr_header(u8 *buf, u64 shoff, u16 shnum, u16 shstrndx) {
    buf[0] = 0x7f;
    buf[1] = 'E';
    buf[2] = 'L';
    buf[3] = 'F';
    buf[4] = (u8)ELF_CLASS_64;
    buf[5] = (u8)ELF_DATA_LSB;
    buf[6] = 1;
    u8 *h  = &buf[16];
    bl_wr_u16(&h[0], (u16)ELF_TYPE_EXEC);
    bl_wr_u16(&h[2], BL_EM_X86_64);
    bl_wr_u32(&h[4], 1);
    bl_wr_u64(&h[24], shoff);
    bl_wr_u16(&h[36], BL_EHDR_SIZE);
    bl_wr_u16(&h[42], BL_SHDR_SIZE);
    bl_wr_u16(&h[44], shnum);
    bl_wr_u16(&h[46], shstrndx);
}

// ===========================================================================
// 267:20 cxx_ge_to_gt -- elf_decode_symbol_table `strtab_idx >= VecLen` -> `>`.
//
//   u32 strtab_idx = symtab->link;
//   if (strtab_idx >= VecLen(&self->sections)) { ...reject... }
//   const ElfSection *strtab = VecPtrAt(&self->sections, strtab_idx);
//
// Set symtab->sh_link == number_of_sections (== VecLen). Real: `idx >= len`
// is true -> rejects -> open returns false. Mutant: `idx > len` is false ->
// accepts, dereferences VecPtrAt(sections, len) (one past) and reads a
// garbage strtab -> open does NOT reject the same way. We assert the open is
// rejected (the real, documented FAILURE contract); the mutant diverges
// (it proceeds past the link bound).  CONFIRMED KILL by mull (absent from
// the Elf.Blind survivor report).
// ===========================================================================
bool test_bl_symtab_link_equals_seccount_rejected(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    enum {
        FSIZE      = 0x200,
        SHSTR_OFF  = 0x40,
        SHSTR_SZ   = 20,
        SYMTAB_OFF = 0x80,
        SYMTAB_SZ  = BL_SYM_SIZE, // one (null) symbol
        SHT_OFF    = 0x100,
        NSEC       = 3,           // NULL, shstrtab, symtab
    };
    u8 buf[FSIZE];
    MemSet(buf, 0, sizeof(buf));
    bl_wr_header(buf, SHT_OFF, NSEC, 1);

    // shstrtab: "\0.shstrtab\0.symtab\0"
    MemCopy(&buf[SHSTR_OFF], "\0.shstrtab\0.symtab\0", 19);

    u8 *sht = &buf[SHT_OFF];
    bl_wr_shdr(&sht[0], 0, ELF_SECTION_TYPE_NULL, 0, 0, 0, 0, 0, 0, 0);
    bl_wr_shdr(&sht[1 * BL_SHDR_SIZE], 1, ELF_SECTION_TYPE_STRTAB, 0, 0, SHSTR_OFF, SHSTR_SZ, 0, 0, 0);
    // symtab with sh_link == NSEC (== VecLen, one past the last valid index).
    bl_wr_shdr(
        &sht[2 * BL_SHDR_SIZE],
        11,
        ELF_SECTION_TYPE_SYMTAB,
        0,
        0,
        SYMTAB_OFF,
        SYMTAB_SZ,
        NSEC, // sh_link out of range (== section count)
        0,
        BL_SYM_SIZE
    );

    Elf  elf;
    bool opened = ElfOpenFromMemoryCopy(&elf, buf, sizeof(buf), ALLOCATOR_OF(&alloc));
    if (opened)
        ElfDeinit(&elf);
    DefaultAllocatorDeinit(&alloc);
    return !opened; // real rejects an out-of-range symtab link
}

// Companion (positive pin): a valid sh_link (== NSEC-1, the last valid index)
// is accepted and parses. This anchors that the link bound is specifically
// about `== len`, not a general rejection of symtabs.
bool test_bl_symtab_link_in_range_accepted(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    enum {
        FSIZE      = 0x200,
        SHSTR_OFF  = 0x40,
        SHSTR_SZ   = 28,
        STRTAB_OFF = 0x60,
        STRTAB_SZ  = 1,
        SYMTAB_OFF = 0x80,
        SYMTAB_SZ  = BL_SYM_SIZE,
        SHT_OFF    = 0x100,
        NSEC       = 4, // NULL, shstrtab, symtab, strtab
    };
    u8 buf[FSIZE];
    MemSet(buf, 0, sizeof(buf));
    bl_wr_header(buf, SHT_OFF, NSEC, 1);
    MemCopy(&buf[SHSTR_OFF], "\0.shstrtab\0.symtab\0.strtab\0", 27);
    buf[STRTAB_OFF] = '\0';

    u8 *sht = &buf[SHT_OFF];
    bl_wr_shdr(&sht[0], 0, ELF_SECTION_TYPE_NULL, 0, 0, 0, 0, 0, 0, 0);
    bl_wr_shdr(&sht[1 * BL_SHDR_SIZE], 1, ELF_SECTION_TYPE_STRTAB, 0, 0, SHSTR_OFF, SHSTR_SZ, 0, 0, 0);
    bl_wr_shdr(
        &sht[2 * BL_SHDR_SIZE],
        11,
        ELF_SECTION_TYPE_SYMTAB,
        0,
        0,
        SYMTAB_OFF,
        SYMTAB_SZ,
        3, // sh_link -> .strtab (index 3 = NSEC-1, valid)
        0,
        BL_SYM_SIZE
    );
    bl_wr_shdr(&sht[3 * BL_SHDR_SIZE], 19, ELF_SECTION_TYPE_STRTAB, 0, 0, STRTAB_OFF, STRTAB_SZ, 0, 0, 0);

    Elf  elf;
    bool ok = ElfOpenFromMemoryCopy(&elf, buf, sizeof(buf), ALLOCATOR_OF(&alloc));
    if (ok) {
        ok = VecLen(&elf.sections) == NSEC;
        ElfDeinit(&elf);
    }
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

int main(void) {
    TestFunction tests[] = {
        test_bl_symtab_link_equals_seccount_rejected,
        test_bl_symtab_link_in_range_accepted,
    };
    TestFunction deadend_tests[] = {0};
    (void)deadend_tests;
    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), deadend_tests, 0, "Elf.Blind");
}
