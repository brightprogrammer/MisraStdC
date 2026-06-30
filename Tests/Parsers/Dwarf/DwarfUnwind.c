/// file      : tests/std/dwarf_unwind.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Mutation-hardening for the DWARF CFI unwinder (Parsers/DwarfUnwind.c): the
/// `.eh_frame`/`.debug_frame` record parser, the `DW_EH_PE_*` encoded-pointer
/// reader, and the `DW_CFA_*` bytecode VM (`DwarfCfiBuildRow`). Driven ENTIRELY
/// through the public API (`DwarfCfiBuildFromElf` / `DwarfCfiFindFde` /
/// `DwarfCfiBuildRow`) on hand-crafted ELF + CFI byte fixtures -- no source
/// include. Real compilers emit only a handful of CFA opcodes / pointer
/// encodings; crafted streams reach the rest deterministically.

#include <Misra.h>
#include <Misra/Parsers/Dwarf.h>
#include <Misra/Parsers/Elf.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Memory.h>

#include "../../Util/TestRunner.h"

// --- little-endian scalar writers ------------------------------------------
static void put_u16(u8 *p, u16 v) {
    p[0] = (u8)v;
    p[1] = (u8)(v >> 8);
}
static void put_u32(u8 *p, u32 v) {
    for (int i = 0; i < 4; i++)
        p[i] = (u8)(v >> (8 * i));
}
static void put_u64(u8 *p, u64 v) {
    for (int i = 0; i < 8; i++)
        p[i] = (u8)(v >> (8 * i));
}

// --- minimal ELF wrapper: one .eh_frame PROGBITS section at `eh_addr` -------
// Mirrors Dwarf.Mut.c's build_elf_with_debug_line. Section layout:
//   [ehdr][shstrtab][.eh_frame data][shdr0 null][shdr1 shstrtab][shdr2 eh].
static void build_elf_with_eh_frame(u8 *elf, u64 *out_len, const u8 *eh, u64 eh_len, u64 eh_addr) {
    static const char shstr[]           = "\0.shstrtab\0.eh_frame";
    const u32         shstrtab_name_off = 1;
    const u32         eh_name_off       = 11;
    u64               shstr_len         = sizeof(shstr);

    u64 ehdr_size = 64;
    u64 shstr_off = ehdr_size;
    u64 eh_off    = shstr_off + shstr_len;
    u64 shtab_off = eh_off + eh_len;

    elf[0] = 0x7f;
    elf[1] = 'E';
    elf[2] = 'L';
    elf[3] = 'F';
    elf[4] = 2;                   // ELFCLASS64
    elf[5] = 1;                   // ELFDATA2LSB
    elf[6] = 1;                   // EV_CURRENT
    put_u16(elf + 16, 2);         // e_type = ET_EXEC
    put_u16(elf + 18, 62);        // e_machine = x86-64
    put_u32(elf + 20, 1);         // e_version
    put_u64(elf + 40, shtab_off); // e_shoff
    put_u16(elf + 52, 64);        // e_ehsize
    put_u16(elf + 58, 64);        // e_shentsize
    put_u16(elf + 60, 3);         // e_shnum
    put_u16(elf + 62, 1);         // e_shstrndx

    MemCopy(elf + shstr_off, shstr, shstr_len);
    MemCopy(elf + eh_off, eh, eh_len);

    u8 *sh = elf + shtab_off;
    u8 *s1 = sh + 64;
    put_u32(s1 + 0, shstrtab_name_off);
    put_u32(s1 + 4, 3); // SHT_STRTAB
    put_u64(s1 + 24, shstr_off);
    put_u64(s1 + 32, shstr_len);
    u8 *s2 = sh + 128;
    put_u32(s2 + 0, eh_name_off);
    put_u32(s2 + 4, 1);        // SHT_PROGBITS
    put_u64(s2 + 16, eh_addr); // sh_addr (pcrel anchor base)
    put_u64(s2 + 24, eh_off);
    put_u64(s2 + 32, eh_len);

    *out_len = shtab_off + 3 * 64;
}

// --- .eh_frame assembler: one CIE (v1, empty augmentation -> ABSPTR FDE
// encoding, code_align 1, data_align -8, RA reg 16) + one FDE + terminator.
// pc_begin / pc_range are written as absolute 8-byte values, so the FDE covers
// [pc_begin, pc_begin + pc_range). Returns the .eh_frame length.
static u64
    build_eh_frame(u8 *eh, const u8 *cie_insns, u64 cie_n, u64 pc_begin, u64 pc_range, const u8 *fde_insns, u64 fde_n) {
    // CIE body (after length field): id(4) ver(1) aug(1) calign(1) dalign(1)
    // ra(1) + insns.
    u64 cie_body = 9 + cie_n;
    put_u32(eh + 0, (u32)cie_body); // CIE length
    put_u32(eh + 4, 0);             // CIE id == 0
    eh[8]  = 1;                     // version
    eh[9]  = 0;                     // augmentation "" (empty)
    eh[10] = 0x01;                  // code_alignment_factor = uleb(1)
    eh[11] = 0x78;                  // data_alignment_factor = sleb(-8)
    eh[12] = 16;                    // return_address_register (u8, v1)
    MemCopy(eh + 13, cie_insns, cie_n);
    u64 fde_off = 13 + cie_n;       // == 4 + cie_body

    // FDE body: cie_ptr(4) pc_begin(8) pc_range(8) + insns.
    u64 fde_body = 20 + fde_n;
    put_u32(eh + fde_off + 0, (u32)fde_body);
    // CIE_pointer = back-offset = (id-field offset) - (CIE offset 0).
    put_u32(eh + fde_off + 4, (u32)(fde_off + 4));
    put_u64(eh + fde_off + 8, pc_begin);
    put_u64(eh + fde_off + 16, pc_range);
    MemCopy(eh + fde_off + 24, fde_insns, fde_n);
    u64 term_off = fde_off + 24 + fde_n;

    put_u32(eh + term_off, 0); // terminator
    return term_off + 4;
}

// Parse a crafted CIE/FDE and compute the unwind row at `target_pc`. Returns
// true iff the parse + FDE lookup + row build all succeed.
static bool craft_row(
    const u8       *cie_insns,
    u64             cie_n,
    u64             pc_begin,
    u64             pc_range,
    const u8       *fde_insns,
    u64             fde_n,
    u64             target_pc,
    DwarfUnwindRow *out_row
) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    static u8 eh[1024];
    u64       eh_len = build_eh_frame(eh, cie_insns, cie_n, pc_begin, pc_range, fde_insns, fde_n);

    static u8 elfbuf[4096];
    u64       elf_len = 0;
    build_elf_with_eh_frame(elfbuf, &elf_len, eh, eh_len, /*eh_addr*/ 0x4000);

    Elf elf;
    if (!ElfOpenFromMemoryCopy(&elf, elfbuf, (size)elf_len, base)) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    DwarfCfi cfi;
    bool     ok = DwarfCfiBuildFromElf(&cfi, &elf, base);
    if (ok) {
        const DwarfFde *fde = DwarfCfiFindFde(&cfi, target_pc);
        ok                  = fde != NULL && DwarfCfiBuildRow(&cfi, fde, target_pc, out_row);
        DwarfCfiDeinit(&cfi);
    }
    ElfDeinit(&elf);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// --- LEB128 + DW_EH_PE value encoders (for the augmented-CIE builder) -------
static u64 enc_uleb(u8 *p, u64 v) {
    u64 n = 0;
    do {
        u8 b   = v & 0x7f;
        v    >>= 7;
        if (v)
            b |= 0x80;
        p[n++] = b;
    } while (v);
    return n;
}
static u64 enc_sleb(u8 *p, i64 v) {
    u64  n    = 0;
    bool more = true;
    while (more) {
        u8 b   = (u8)(v & 0x7f);
        v    >>= 7;
        if ((v == 0 && !(b & 0x40)) || (v == -1 && (b & 0x40)))
            more = false;
        else
            b |= 0x80;
        p[n++] = b;
    }
    return n;
}
// Encode `v` in DW_EH_PE value-kind `vk` (low nibble of an encoding byte).
static u64 enc_value(u8 *p, u8 vk, i64 v) {
    switch (vk) {
        case 0x00 : // ABSPTR
        case 0x04 : // UDATA8
        case 0x0c : // SDATA8
            put_u64(p, (u64)v);
            return 8;
        case 0x03 : // UDATA4
        case 0x0b : // SDATA4
            put_u32(p, (u32)v);
            return 4;
        case 0x02 : // UDATA2
        case 0x0a : // SDATA2
            put_u16(p, (u16)v);
            return 2;
        case 0x01 : // ULEB128
            return enc_uleb(p, (u64)v);
        case 0x09 : // SLEB128
            return enc_sleb(p, v);
    }
    return 0;
}

// CIE shape knobs for the flexible builder. `aug` is the literal augmentation
// string ("", "zR", "zLR", "zPR", "zRS", "zQR" ...); for each char after 'z'
// the matching encoding byte (and, for 'P', an ABSPTR personality pointer) is
// emitted into the aug-data block in string order.
typedef struct {
    u8          version;        // 1, 3, or 4
    const char *aug;            // augmentation string
    u8          addr_size;      // emitted only for version >= 4
    u8          code_align;     // code_alignment_factor uleb byte (omitted -> 0)
    u64         code_align_big; // if nonzero, emit this as a multi-byte ULEB instead
    u8          data_align;     // single sleb byte (0x78 == -8)
    u8          ra;             // return_address_register
    u8          fde_enc;        // 'R' byte
    u8          lsda_enc;       // 'L' byte
    u8          pers_enc;       // 'P' byte (personality pointer encoding)
    u8          fde_aug_n;      // FDE-level augmentation-data length (0xFF filler)
    const u8   *insns;
    u64         insns_n;
} CieSpec;

#define EH_SECTION_VADDR 0x4000u

// Assemble a single CIE (per `c`) + one FDE (pc_begin/pc_range encoded with the
// CIE's 'R' encoding, ABSPTR if none) + terminator. Returns the .eh_frame len.
static u64 build_cfi_ex(u8 *eh, const CieSpec *c, i64 pc_begin, u64 pc_range, const u8 *fde_insns, u64 fde_n) {
    const char *a = c->aug ? c->aug : "";
    u64         p = 0;

    u64 cie_len_pos     = p;
    p                  += 4;
    u64 cie_body_start  = p;
    put_u32(eh + p, 0); // CIE id == 0 (.eh_frame)
    p       += 4;
    eh[p++]  = c->version;
    for (const char *s = a; *s; s++)
        eh[p++] = (u8)*s;
    eh[p++] = 0;                                  // augmentation NUL
    if (c->version >= 4) {
        eh[p++] = c->addr_size;                   // address_size
        eh[p++] = 0;                              // segment_size
    }
    if (c->code_align_big)
        p += enc_uleb(eh + p, c->code_align_big); // multi-byte code_alignment_factor
    else
        eh[p++] = c->code_align;                  // single-byte code_alignment_factor (0 if omitted)
    eh[p++] = c->data_align;                      // data_alignment_factor = sleb byte
    if (c->version >= 3)
        p += enc_uleb(eh + p, c->ra);
    else
        eh[p++] = c->ra;

    if (a[0] == 'z') {
        u8  aug[64];
        u64 an = 0;
        for (const char *s = a + 1; *s; s++) {
            if (*s == 'L')
                aug[an++] = c->lsda_enc;
            else if (*s == 'P') {
                aug[an++]  = c->pers_enc;
                an        += enc_value(aug + an, c->pers_enc & 0x0f, 0); // personality ptr = 0
            } else if (*s == 'R')
                aug[an++] = c->fde_enc;
            // 'S' and unknown chars contribute no aug-data bytes
        }
        p += enc_uleb(eh + p, an);
        MemCopy(eh + p, aug, an);
        p += an;
    }
    if (c->insns_n) {
        MemCopy(eh + p, c->insns, c->insns_n);
        p += c->insns_n;
    }
    put_u32(eh + cie_len_pos, (u32)(p - cie_body_start));

    // FDE
    u64 fde_len_pos     = p;
    p                  += 4;
    u64 fde_body_start  = p;
    put_u32(eh + p, (u32)fde_body_start); // CIE_pointer = back-offset to CIE @0
    p += 4;

    bool has_r = false;
    for (const char *s = a; *s; s++)
        if (*s == 'R')
            has_r = true;
    u8  fde_enc = has_r ? c->fde_enc : 0x00;
    u8  vk      = fde_enc & 0x0f;
    u8  base    = fde_enc & 0x70;
    i64 stored  = pc_begin;
    if (base == 0x10) { // PCREL: store target relative to this byte's vaddr
        u64 here = EH_SECTION_VADDR + p;
        stored   = (i64)((u64)pc_begin - here);
    }
    p += enc_value(eh + p, vk, stored);
    p += enc_value(eh + p, vk, (i64)pc_range); // pc_range: value-kind only
    if (a[0] == 'z') {
        p += enc_uleb(eh + p, c->fde_aug_n);   // FDE aug-data length
        for (u8 k = 0; k < c->fde_aug_n; k++)
            eh[p++] = 0xff;                    // filler: an invalid opcode if mis-run as instructions
    }
    if (fde_n) {
        MemCopy(eh + p, fde_insns, fde_n);
        p += fde_n;
    }
    put_u32(eh + fde_len_pos, (u32)(p - fde_body_start));

    put_u32(eh + p, 0); // terminator
    p += 4;
    return p;
}

// craft_row, but with a fully-specified CIE (augmentation / version / FDE
// pointer encoding). Same public-API path as craft_row.
static bool craft_row_ex(
    const CieSpec  *cie,
    i64             pc_begin,
    u64             pc_range,
    const u8       *fde_insns,
    u64             fde_n,
    u64             target_pc,
    DwarfUnwindRow *out_row
) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    static u8 eh[1024];
    u64       eh_len = build_cfi_ex(eh, cie, pc_begin, pc_range, fde_insns, fde_n);

    static u8 elfbuf[4096];
    u64       elf_len = 0;
    build_elf_with_eh_frame(elfbuf, &elf_len, eh, eh_len, EH_SECTION_VADDR);

    Elf elf;
    if (!ElfOpenFromMemoryCopy(&elf, elfbuf, (size)elf_len, base)) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }
    DwarfCfi cfi;
    bool     ok = DwarfCfiBuildFromElf(&cfi, &elf, base);
    if (ok) {
        const DwarfFde *fde = DwarfCfiFindFde(&cfi, target_pc);
        ok                  = fde != NULL && DwarfCfiBuildRow(&cfi, fde, target_pc, out_row);
        DwarfCfiDeinit(&cfi);
    }
    ElfDeinit(&elf);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// --- .debug_frame path: an ELF carrying ONLY a .debug_frame section (no
// .eh_frame), exercising DwarfCfiBuildFromElf's fallback and cfi_parse_section's
// is_debug_frame arms (CIE id sentinel 0xffffffff, absolute CIE pointer,
// absolute FDE addresses). clang under -g emits CFI only here.
static void build_elf_with_debug_frame(u8 *elf, u64 *out_len, const u8 *df, u64 df_len, u64 df_addr) {
    static const char shstr[]     = "\0.shstrtab\0.debug_frame";
    const u32         shstrtab_no = 1;
    const u32         df_name_no  = 11;
    u64               shstr_len   = sizeof(shstr);

    u64 shstr_off = 64;
    u64 df_off    = shstr_off + shstr_len;
    u64 shtab_off = df_off + df_len;

    elf[0] = 0x7f;
    elf[1] = 'E';
    elf[2] = 'L';
    elf[3] = 'F';
    elf[4] = 2;
    elf[5] = 1;
    elf[6] = 1;
    put_u16(elf + 16, 2);
    put_u16(elf + 18, 62);
    put_u32(elf + 20, 1);
    put_u64(elf + 40, shtab_off);
    put_u16(elf + 52, 64);
    put_u16(elf + 58, 64);
    put_u16(elf + 60, 3);
    put_u16(elf + 62, 1);

    MemCopy(elf + shstr_off, shstr, shstr_len);
    MemCopy(elf + df_off, df, df_len);

    u8 *sh = elf + shtab_off;
    u8 *s1 = sh + 64;
    put_u32(s1 + 0, shstrtab_no);
    put_u32(s1 + 4, 3);
    put_u64(s1 + 24, shstr_off);
    put_u64(s1 + 32, shstr_len);
    u8 *s2 = sh + 128;
    put_u32(s2 + 0, df_name_no);
    put_u32(s2 + 4, 1);
    put_u64(s2 + 16, df_addr);
    put_u64(s2 + 24, df_off);
    put_u64(s2 + 32, df_len);

    *out_len = shtab_off + 3 * 64;
}

// Assemble a .debug_frame: CIE (id == 0xffffffff sentinel, empty augmentation
// -> ABSPTR addresses) + one FDE (CIE_pointer = absolute section offset 0,
// 8-byte absolute pc_begin/pc_range) + terminator.
static u64 build_debug_frame(
    u8       *df,
    const u8 *cie_insns,
    u64       cie_n,
    u64       pc_begin,
    u64       pc_range,
    const u8 *fde_insns,
    u64       fde_n
) {
    u64 cie_body = 9 + cie_n;
    put_u32(df + 0, (u32)cie_body);
    put_u32(df + 4, 0xffffffff); // .debug_frame CIE id sentinel
    df[8]  = 1;                  // version
    df[9]  = 0;                  // augmentation ""
    df[10] = 0x01;               // code_alignment_factor
    df[11] = 0x78;               // data_alignment_factor = -8
    df[12] = 16;                 // return_address_register
    MemCopy(df + 13, cie_insns, cie_n);
    u64 fde_off = 13 + cie_n;

    u64 fde_body = 20 + fde_n;
    put_u32(df + fde_off + 0, (u32)fde_body);
    put_u32(df + fde_off + 4, 0); // CIE_pointer = absolute offset of CIE (== 0)
    put_u64(df + fde_off + 8, pc_begin);
    put_u64(df + fde_off + 16, pc_range);
    MemCopy(df + fde_off + 24, fde_insns, fde_n);
    u64 term_off = fde_off + 24 + fde_n;

    put_u32(df + term_off, 0); // terminator
    return term_off + 4;
}

static bool craft_row_debug(
    const u8       *cie_insns,
    u64             cie_n,
    u64             pc_begin,
    u64             pc_range,
    const u8       *fde_insns,
    u64             fde_n,
    u64             target_pc,
    DwarfUnwindRow *out_row
) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    static u8 df[1024];
    u64       df_len = build_debug_frame(df, cie_insns, cie_n, pc_begin, pc_range, fde_insns, fde_n);

    static u8 elfbuf[4096];
    u64       elf_len = 0;
    build_elf_with_debug_frame(elfbuf, &elf_len, df, df_len, EH_SECTION_VADDR);

    Elf elf;
    if (!ElfOpenFromMemoryCopy(&elf, elfbuf, (size)elf_len, base)) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }
    DwarfCfi cfi;
    bool     ok = DwarfCfiBuildFromElf(&cfi, &elf, base);
    if (ok) {
        const DwarfFde *fde = DwarfCfiFindFde(&cfi, target_pc);
        ok                  = fde != NULL && DwarfCfiBuildRow(&cfi, fde, target_pc, out_row);
        DwarfCfiDeinit(&cfi);
    }
    ElfDeinit(&elf);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// --- multi-record .eh_frame writer: append CIEs/FDEs at known offsets so a
// fixture can carry more than one of each (needed to exercise the CIE/FDE
// linear-scan loops -- a single element hides the iterator direction).
typedef struct {
    u8 *buf;
    u64 pos;
} EhWriter;

// Append a v1 empty-augmentation CIE (ABSPTR addresses); return its offset.
static u64 eh_put_cie(EhWriter *w, const u8 *insns, u64 n) {
    u64 off  = w->pos;
    u64 body = 9 + n;
    put_u32(w->buf + w->pos, (u32)body);
    w->pos += 4;
    put_u32(w->buf + w->pos, 0); // CIE id == 0
    w->pos           += 4;
    w->buf[w->pos++]  = 1;       // version
    w->buf[w->pos++]  = 0;       // augmentation ""
    w->buf[w->pos++]  = 1;       // code_alignment_factor
    w->buf[w->pos++]  = 0x78;    // data_alignment_factor = -8
    w->buf[w->pos++]  = 16;      // return_address_register
    MemCopy(w->buf + w->pos, insns, n);
    w->pos += n;
    return off;
}

// Append an FDE referencing the CIE at `cie_off` (8-byte absolute addresses);
// return the FDE record's offset within the section.
static u64 eh_put_fde(EhWriter *w, u64 cie_off, u64 pc_begin, u64 pc_range, const u8 *insns, u64 n) {
    u64 off  = w->pos;
    u64 body = 20 + n;
    put_u32(w->buf + w->pos, (u32)body);
    w->pos      += 4;
    u64 idfield  = w->pos;
    put_u32(w->buf + w->pos, (u32)(idfield - cie_off)); // back-offset to CIE
    w->pos += 4;
    put_u64(w->buf + w->pos, pc_begin);
    w->pos += 8;
    put_u64(w->buf + w->pos, pc_range);
    w->pos += 8;
    MemCopy(w->buf + w->pos, insns, n);
    w->pos += n;
    return off;
}
static void eh_put_terminator(EhWriter *w) {
    put_u32(w->buf + w->pos, 0);
    w->pos += 4;
}

// Run a pre-assembled .eh_frame buffer through the public API.
static bool craft_lookup(const u8 *eh, u64 eh_len, u64 target_pc, DwarfUnwindRow *out_row) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    static u8 elfbuf[4096];
    u64       elf_len = 0;
    build_elf_with_eh_frame(elfbuf, &elf_len, eh, eh_len, EH_SECTION_VADDR);

    Elf elf;
    if (!ElfOpenFromMemoryCopy(&elf, elfbuf, (size)elf_len, base)) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }
    DwarfCfi cfi;
    bool     ok = DwarfCfiBuildFromElf(&cfi, &elf, base);
    if (ok) {
        const DwarfFde *fde = DwarfCfiFindFde(&cfi, target_pc);
        ok                  = fde != NULL && DwarfCfiBuildRow(&cfi, fde, target_pc, out_row);
        DwarfCfiDeinit(&cfi);
    }
    ElfDeinit(&elf);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// --- two-PROGBITS ELF: an empty .eh_frame plus a populated .debug_frame, to
// exercise build_from_elf's `eh->size > 0` guard (a >= mutant would pick the
// empty .eh_frame instead of falling back).
static void build_elf_eh_and_debug(u8 *elf, u64 *out_len, const u8 *df, u64 df_len, u64 df_addr) {
    static const char shstr[]     = "\0.shstrtab\0.eh_frame\0.debug_frame";
    const u32         shstrtab_no = 1;
    const u32         eh_no       = 11; // ".eh_frame"
    const u32         df_no       = 21; // ".debug_frame"
    u64               shstr_len   = sizeof(shstr);

    u64 shstr_off = 64;
    u64 df_off    = shstr_off + shstr_len;
    u64 shtab_off = df_off + df_len;

    elf[0] = 0x7f;
    elf[1] = 'E';
    elf[2] = 'L';
    elf[3] = 'F';
    elf[4] = 2;
    elf[5] = 1;
    elf[6] = 1;
    put_u16(elf + 16, 2);
    put_u16(elf + 18, 62);
    put_u32(elf + 20, 1);
    put_u64(elf + 40, shtab_off);
    put_u16(elf + 52, 64);
    put_u16(elf + 58, 64);
    put_u16(elf + 60, 4); // null, shstrtab, .eh_frame, .debug_frame
    put_u16(elf + 62, 1);

    MemCopy(elf + shstr_off, shstr, shstr_len);
    MemCopy(elf + df_off, df, df_len);

    u8 *sh = elf + shtab_off;
    u8 *s1 = sh + 64; // .shstrtab
    put_u32(s1 + 0, shstrtab_no);
    put_u32(s1 + 4, 3);
    put_u64(s1 + 24, shstr_off);
    put_u64(s1 + 32, shstr_len);
    u8 *s2 = sh + 128; // .eh_frame (size 0)
    put_u32(s2 + 0, eh_no);
    put_u32(s2 + 4, 1);
    put_u64(s2 + 16, 0);
    put_u64(s2 + 24, df_off);
    put_u64(s2 + 32, 0); // sh_size == 0
    u8 *s3 = sh + 192;   // .debug_frame
    put_u32(s3 + 0, df_no);
    put_u32(s3 + 4, 1);
    put_u64(s3 + 16, df_addr);
    put_u64(s3 + 24, df_off);
    put_u64(s3 + 32, df_len);

    *out_len = shtab_off + 4 * 64;
}

// ===========================================================================
// CFA rule opcodes
// ===========================================================================

// DW_CFA_def_cfa reg, off -> CFA = reg + off. Pins the opcode, the register
// ULEB, and the offset ULEB.
bool test_du_def_cfa(void) {
    u8             cie[] = {0x0c, 0x07, 0x10}; // def_cfa r7, 16
    DwarfUnwindRow row;
    bool           ok = craft_row(cie, sizeof(cie), 0x1000, 0x100, NULL, 0, 0x1000, &row);
    ok                = ok && row.cfa.kind == DWARF_CFA_RULE_REG_OFFSET;
    ok                = ok && row.cfa.reg == 7;
    ok                = ok && row.cfa.offset == 16;
    return ok;
}

// DW_CFA_def_cfa_offset changes only the offset; DW_CFA_def_cfa_register only
// the register. Start from def_cfa r7,8 then override each.
bool test_du_def_cfa_offset_and_register(void) {
    u8 cie[] = {
        0x0c,
        0x07,
        0x08, // def_cfa r7, 8
        0x0e,
        0x20, // def_cfa_offset 32
        0x0d,
        0x06, // def_cfa_register r6
    };
    DwarfUnwindRow row;
    bool           ok = craft_row(cie, sizeof(cie), 0x2000, 0x80, NULL, 0, 0x2000, &row);
    ok                = ok && row.cfa.kind == DWARF_CFA_RULE_REG_OFFSET;
    ok                = ok && row.cfa.reg == 6;     // overridden by def_cfa_register
    ok                = ok && row.cfa.offset == 32; // overridden by def_cfa_offset
    return ok;
}

// DW_CFA_offset (high-2-bit form 0b10): reg saved at CFA + N*data_align.
// data_align is -8, so `offset r16, 1` -> regs[16] = OFFSET at -8.
bool test_du_offset_rule(void) {
    u8 cie[] = {
        0x0c,
        0x07,
        0x10, // def_cfa r7, 16
        0x90,
        0x01, // offset r16, 1  (0x80|16, uleb 1)  -> -8
    };
    DwarfUnwindRow row;
    bool           ok = craft_row(cie, sizeof(cie), 0x1000, 0x40, NULL, 0, 0x1000, &row);
    ok                = ok && row.regs[16].kind == DWARF_REG_RULE_OFFSET;
    ok                = ok && row.regs[16].offset == -8;
    return ok;
}

// advance_loc (high-2-bit form 0b01): the FDE row at a PC BEFORE the advance
// differs from the row at/after it. Set CFA offset 8, advance 4, set offset 24.
// At pc_begin -> offset 8; at pc_begin+4 -> offset 24.
bool test_du_advance_loc_changes_row(void) {
    u8 cie[] = {0x0c, 0x07, 0x08}; // def_cfa r7, 8
    u8 fde[] = {
        0x44,                      // advance_loc 4   (0x40|4)
        0x0e,
        0x18,                      // def_cfa_offset 24
    };
    DwarfUnwindRow r_before, r_after;
    bool           ok = craft_row(cie, sizeof(cie), 0x1000, 0x100, fde, sizeof(fde), 0x1000, &r_before);
    ok                = ok && craft_row(cie, sizeof(cie), 0x1000, 0x100, fde, sizeof(fde), 0x1004, &r_after);
    ok                = ok && r_before.cfa.offset == 8; // before the advance
    ok                = ok && r_after.cfa.offset == 24; // after
    return ok;
}

// remember_state / restore_state: save the row, mutate, restore. After restore
// the CFA offset must be the saved value, not the mutated one.
bool test_du_remember_restore_state(void) {
    u8 cie[] = {0x0c, 0x07, 0x08}; // def_cfa r7, 8
    u8 fde[] = {
        0x0a,                      // remember_state
        0x0e,
        0x40,                      // def_cfa_offset 64
        0x0b,                      // restore_state  -> back to offset 8
    };
    DwarfUnwindRow row;
    bool           ok = craft_row(cie, sizeof(cie), 0x1000, 0x100, fde, sizeof(fde), 0x10ff, &row);
    ok                = ok && row.cfa.kind == DWARF_CFA_RULE_REG_OFFSET;
    ok                = ok && row.cfa.offset == 8;               // restored, not 64
    ok                = ok && row.return_address_register == 16; // RA preserved across restore
    return ok;
}

// nop is skipped; the following def_cfa still applies.
bool test_du_nop(void) {
    u8             cie[] = {0x00, 0x00, 0x0c, 0x07, 0x08}; // nop, nop, def_cfa r7, 8
    DwarfUnwindRow row;
    bool           ok = craft_row(cie, sizeof(cie), 0x1000, 0x40, NULL, 0, 0x1000, &row);
    return ok && row.cfa.reg == 7 && row.cfa.offset == 8;
}

// offset_extended reg, off -> reg rule OFFSET at off * data_align (-8).
bool test_du_offset_extended(void) {
    u8             cie[] = {0x0c, 0x07, 0x10, 0x05, 0x05, 0x02}; // def_cfa r7,16; offset_extended r5, 2
    DwarfUnwindRow row;
    bool           ok = craft_row(cie, sizeof(cie), 0x1000, 0x40, NULL, 0, 0x1000, &row);
    return ok && row.regs[5].kind == DWARF_REG_RULE_OFFSET && row.regs[5].offset == -16;
}

// undefined resets a previously-set rule to UNDEFINED (distinguishable from
// never-set because we set OFFSET first).
bool test_du_undefined(void) {
    u8             cie[] = {0x0c, 0x07, 0x10, 0x85, 0x02, 0x07, 0x05}; // def_cfa; offset r5,2; undefined r5
    DwarfUnwindRow row;
    bool           ok = craft_row(cie, sizeof(cie), 0x1000, 0x40, NULL, 0, 0x1000, &row);
    return ok && row.regs[5].kind == DWARF_REG_RULE_UNDEFINED;
}

// same_value rule.
bool test_du_same_value(void) {
    u8             cie[] = {0x0c, 0x07, 0x10, 0x85, 0x02, 0x08, 0x05}; // ...; offset r5,2; same_value r5
    DwarfUnwindRow row;
    bool           ok = craft_row(cie, sizeof(cie), 0x1000, 0x40, NULL, 0, 0x1000, &row);
    return ok && row.regs[5].kind == DWARF_REG_RULE_SAME_VALUE;
}

// register rule: r5's value is in r6.
bool test_du_register(void) {
    u8             cie[] = {0x0c, 0x07, 0x10, 0x09, 0x05, 0x06}; // def_cfa; register r5 = r6
    DwarfUnwindRow row;
    bool           ok = craft_row(cie, sizeof(cie), 0x1000, 0x40, NULL, 0, 0x1000, &row);
    return ok && row.regs[5].kind == DWARF_REG_RULE_REGISTER && row.regs[5].reg == 6;
}

// restore / restore_extended return a reg to its CIE-initial rule. CIE sets
// offset r5,2; FDE undefines then restores it -> back to OFFSET.
bool test_du_restore_extended(void) {
    u8             cie[] = {0x0c, 0x07, 0x10, 0x85, 0x02}; // def_cfa; offset r5, 2 (CIE rule)
    u8             fde[] = {0x07, 0x05, 0x06, 0x05};       // undefined r5; restore_extended r5
    DwarfUnwindRow row;
    bool           ok = craft_row(cie, sizeof(cie), 0x1000, 0x40, fde, sizeof(fde), 0x1000, &row);
    return ok && row.regs[5].kind == DWARF_REG_RULE_OFFSET && row.regs[5].offset == -16;
}

// restore (high-2-bit 0b11 form): same, via 0xC0|reg.
bool test_du_restore_highbit(void) {
    u8             cie[] = {0x0c, 0x07, 0x10, 0x85, 0x02}; // def_cfa; offset r5, 2
    u8             fde[] = {0x07, 0x05, 0xc5};             // undefined r5; restore r5 (0xC0|5)
    DwarfUnwindRow row;
    bool           ok = craft_row(cie, sizeof(cie), 0x1000, 0x40, fde, sizeof(fde), 0x1000, &row);
    return ok && row.regs[5].kind == DWARF_REG_RULE_OFFSET && row.regs[5].offset == -16;
}

// advance_loc1/2/4: explicit-width PC advances change which row applies. The
// targets straddle the advance_loc4 region boundary [0x2030, 0x2070) so the
// `location <= stop_at && stop_at < next` row-selection comparison is pinned at
// both ends (0x2030 == location, 0x2070 == next).
bool test_du_advance_loc124(void) {
    u8 cie[] = {0x0c, 0x07, 0x08}; // def_cfa r7, 8
    u8 fde[] = {
        0x02,
        0x10,                      // advance_loc1 16  -> 0x2010
        0x0e,
        0x20,                      // def_cfa_offset 32
        0x03,
        0x20,
        0x00,                      // advance_loc2 32  -> 0x2030
        0x0e,
        0x30,                      // def_cfa_offset 48
        0x04,
        0x40,
        0x00,
        0x00,
        0x00, // advance_loc4 64 -> 0x2070
        0x0e,
        0x38, // def_cfa_offset 56
    };
    DwarfUnwindRow r0, r1, r2, r3, r4;
    bool           ok = craft_row(cie, sizeof(cie), 0x2000, 0x200, fde, sizeof(fde), 0x2000, &r0);
    ok                = ok && craft_row(cie, sizeof(cie), 0x2000, 0x200, fde, sizeof(fde), 0x2010, &r1);
    ok                = ok && craft_row(cie, sizeof(cie), 0x2000, 0x200, fde, sizeof(fde), 0x2030, &r2);
    ok                = ok && craft_row(cie, sizeof(cie), 0x2000, 0x200, fde, sizeof(fde), 0x2050, &r3);
    ok                = ok && craft_row(cie, sizeof(cie), 0x2000, 0x200, fde, sizeof(fde), 0x2070, &r4);
    return ok && r0.cfa.offset == 8 && r1.cfa.offset == 32 && r2.cfa.offset == 48 && r3.cfa.offset == 48 &&
           r4.cfa.offset == 56;
}

// def_cfa_sf / def_cfa_offset_sf: signed FACTORED offset (* data_align -8).
bool test_du_def_cfa_sf(void) {
    u8             cie[] = {0x12, 0x07, 0x7f}; // def_cfa_sf r7, sleb(-1) -> -1 * -8 = 8
    DwarfUnwindRow row;
    bool           ok = craft_row(cie, sizeof(cie), 0x1000, 0x40, NULL, 0, 0x1000, &row);
    return ok && row.cfa.kind == DWARF_CFA_RULE_REG_OFFSET && row.cfa.reg == 7 && row.cfa.offset == 8;
}

// offset_extended_sf: signed factored reg offset.
bool test_du_offset_extended_sf(void) {
    u8             cie[] = {0x0c, 0x07, 0x10, 0x11, 0x05, 0x02}; // def_cfa; offset_extended_sf r5, sleb(2) -> -16
    DwarfUnwindRow row;
    bool           ok = craft_row(cie, sizeof(cie), 0x1000, 0x40, NULL, 0, 0x1000, &row);
    return ok && row.regs[5].kind == DWARF_REG_RULE_OFFSET && row.regs[5].offset == -16;
}

// val_offset: reg VALUE is CFA + off*data_align (not stored *at* CFA+off).
bool test_du_val_offset(void) {
    u8             cie[] = {0x0c, 0x07, 0x10, 0x14, 0x05, 0x02}; // def_cfa; val_offset r5, 2 -> -16
    DwarfUnwindRow row;
    bool           ok = craft_row(cie, sizeof(cie), 0x1000, 0x40, NULL, 0, 0x1000, &row);
    return ok && row.regs[5].kind == DWARF_REG_RULE_VAL_OFFSET && row.regs[5].offset == -16;
}

// def_cfa_expression / expression / val_expression set EXPRESSION rules (the
// expression bytecode is not evaluated in v1 -- we pin the rule KIND only).
bool test_du_expression_kinds(void) {
    // def_cfa_expression: block len 1, one nop-ish byte.
    u8             cie1[] = {0x0f, 0x01, 0x00};
    DwarfUnwindRow row;
    bool           ok = craft_row(cie1, sizeof(cie1), 0x1000, 0x40, NULL, 0, 0x1000, &row);
    ok                = ok && row.cfa.kind == DWARF_CFA_RULE_EXPRESSION;
    // expression r5: reg, block len 1.
    u8 cie2[] = {0x0c, 0x07, 0x10, 0x10, 0x05, 0x01, 0x00};
    ok        = ok && craft_row(cie2, sizeof(cie2), 0x1000, 0x40, NULL, 0, 0x1000, &row);
    ok        = ok && row.regs[5].kind == DWARF_REG_RULE_EXPRESSION;
    return ok;
}

// def_cfa_offset as the FIRST cfa op (initial cfa.kind == UNDEFINED) forces the
// rule kind to REG_OFFSET. Distinguishes the `kind != REG_OFFSET` guard from its
// `==` mutant, which would leave the kind UNDEFINED.
bool test_du_def_cfa_offset_standalone(void) {
    u8             cie[] = {0x0e, 0x18}; // def_cfa_offset 24 (no prior def_cfa)
    DwarfUnwindRow row;
    bool           ok = craft_row(cie, sizeof(cie), 0x1000, 0x40, NULL, 0, 0x1000, &row);
    return ok && row.cfa.kind == DWARF_CFA_RULE_REG_OFFSET && row.cfa.offset == 24;
}

// def_cfa_register as the first cfa op: same guard, on the register path.
bool test_du_def_cfa_register_standalone(void) {
    u8             cie[] = {0x0d, 0x06}; // def_cfa_register r6 (no prior def_cfa)
    DwarfUnwindRow row;
    bool           ok = craft_row(cie, sizeof(cie), 0x1000, 0x40, NULL, 0, 0x1000, &row);
    return ok && row.cfa.kind == DWARF_CFA_RULE_REG_OFFSET && row.cfa.reg == 6;
}

// def_cfa_offset_sf as first cfa op: signed factored offset (* data_align -8),
// and the same kind-promotion guard. sleb(-3) * -8 = 24.
bool test_du_def_cfa_offset_sf(void) {
    u8             cie[] = {0x13, 0x7d}; // def_cfa_offset_sf sleb(-3)
    DwarfUnwindRow row;
    bool           ok = craft_row(cie, sizeof(cie), 0x1000, 0x40, NULL, 0, 0x1000, &row);
    return ok && row.cfa.kind == DWARF_CFA_RULE_REG_OFFSET && row.cfa.offset == 24;
}

// val_offset_sf: signed factored reg offset, VAL_OFFSET rule. sleb(3) * -8 = -24.
bool test_du_val_offset_sf(void) {
    u8             cie[] = {0x0c, 0x07, 0x10, 0x15, 0x05, 0x03}; // def_cfa; val_offset_sf r5, sleb(3)
    DwarfUnwindRow row;
    bool           ok = craft_row(cie, sizeof(cie), 0x1000, 0x40, NULL, 0, 0x1000, &row);
    return ok && row.regs[5].kind == DWARF_REG_RULE_VAL_OFFSET && row.regs[5].offset == -24;
}

// set_loc sets the location counter to an absolute PC. The FDE advances by
// set_loc to 0x3010 (offset 32), then advance_loc 4 to 0x3014 (offset 48).
// Targets pin the set_loc stop boundary (0x3010 == abs_pc) and prove the
// location actually became abs_pc, not a constant (0x3013 needs the later
// advance to land in the 0x3014 region established off the real location).
bool test_du_set_loc(void) {
    u8 cie[] = {0x0c, 0x07, 0x08}; // def_cfa r7, 8
    u8 fde[] = {
        0x01,
        0x10,
        0x30,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00, // set_loc 0x3010
        0x0e,
        0x20, // def_cfa_offset 32
        0x44, // advance_loc 4 -> 0x3014
        0x0e,
        0x30, // def_cfa_offset 48
    };
    DwarfUnwindRow r0, r1, r2;
    bool           ok = craft_row(cie, sizeof(cie), 0x3000, 0x100, fde, sizeof(fde), 0x3000, &r0);
    ok                = ok && craft_row(cie, sizeof(cie), 0x3000, 0x100, fde, sizeof(fde), 0x3010, &r1);
    ok                = ok && craft_row(cie, sizeof(cie), 0x3000, 0x100, fde, sizeof(fde), 0x3013, &r2);
    return ok && r0.cfa.offset == 8 // before set_loc region
        && r1.cfa.offset == 32      // at 0x3010 (set_loc boundary), before advance
        && r2.cfa.offset == 32;     // 0x3013 still in [0x3010, 0x3014)
}

// ===========================================================================
// FDE pointer encodings (decode_eh_ptr) -- driven via a 'zR' CIE that sets the
// FDE encoding. Each asserts the FDE was found at exactly pc_begin (proving the
// value decoded correctly) and NOT found just below it.
// ===========================================================================
static bool fde_enc_roundtrips(u8 fde_enc, i64 pc_begin, u64 pc_range) {
    u8      cfa[] = {0x0c, 0x07, 0x10}; // def_cfa r7, 16
    CieSpec cie   = {
          .version    = 1,
          .aug        = "zR",
          .data_align = 0x78,
          .ra         = 16,
          .fde_enc    = fde_enc,
          .insns      = cfa,
          .insns_n    = sizeof(cfa)
    };
    DwarfUnwindRow row;
    bool           hit     = craft_row_ex(&cie, pc_begin, pc_range, NULL, 0, (u64)pc_begin, &row);
    bool           miss_lo = craft_row_ex(&cie, pc_begin, pc_range, NULL, 0, (u64)pc_begin - 1, &row);
    // The range upper bound is exclusive: pc_begin + pc_range must NOT match.
    bool miss_hi = craft_row_ex(&cie, pc_begin, pc_range, NULL, 0, (u64)pc_begin + pc_range, &row);
    return hit && !miss_lo && !miss_hi && row.cfa.reg == 7 && row.cfa.offset == 16;
}
bool test_du_enc_udata2(void) {
    return fde_enc_roundtrips(0x02, 0x0800, 0x40);
}
bool test_du_enc_udata4(void) {
    return fde_enc_roundtrips(0x03, 0x8000, 0x100);
}
bool test_du_enc_udata8(void) {
    return fde_enc_roundtrips(0x04, 0x8000, 0x100);
}
bool test_du_enc_uleb128(void) {
    return fde_enc_roundtrips(0x01, 0x8000, 0x100);
}
bool test_du_enc_sdata2(void) {
    return fde_enc_roundtrips(0x0a, 0x0400, 0x40);
}
bool test_du_enc_sdata4(void) {
    return fde_enc_roundtrips(0x0b, 0x8000, 0x100);
}
bool test_du_enc_sdata8(void) {
    return fde_enc_roundtrips(0x0c, 0x8000, 0x100);
}
bool test_du_enc_sleb128(void) {
    return fde_enc_roundtrips(0x09, 0x8000, 0x100);
}
// PCREL adds the encoded byte's own vaddr; roundtrip must still land on pc_begin.
bool test_du_enc_pcrel_sdata4(void) {
    return fde_enc_roundtrips(0x10 | 0x0b, 0x8000, 0x100);
}
bool test_du_enc_pcrel_udata4(void) {
    return fde_enc_roundtrips(0x10 | 0x03, 0x8000, 0x100);
}

// INDIRECT and OMIT must fail closed -> no FDE parsed -> lookup fails.
bool test_du_enc_indirect_fails(void) {
    u8      cfa[] = {0x0c, 0x07, 0x10};
    CieSpec cie   = {
          .version    = 1,
          .aug        = "zR",
          .data_align = 0x78,
          .ra         = 16,
          .fde_enc    = 0x80 | 0x03, // INDIRECT|UDATA4
          .insns      = cfa,
          .insns_n    = sizeof(cfa)
    };
    DwarfUnwindRow row;
    return !craft_row_ex(&cie, 0x8000, 0x100, NULL, 0, 0x8000, &row);
}
bool test_du_enc_omit_fails(void) {
    u8      cfa[] = {0x0c, 0x07, 0x10};
    CieSpec cie   = {
          .version    = 1,
          .aug        = "zR",
          .data_align = 0x78,
          .ra         = 16,
          .fde_enc    = 0xff, // DW_EH_PE_OMIT
          .insns      = cfa,
          .insns_n    = sizeof(cfa)
    };
    DwarfUnwindRow row;
    return !craft_row_ex(&cie, 0x8000, 0x100, NULL, 0, 0x8000, &row);
}

// ===========================================================================
// CIE parsing (parse_cie): versions, augmentation arms.
// ===========================================================================

// version 3: return_address_register is a ULEB, not a single byte. RA=200 is
// multi-byte in ULEB (0xc8 0x01) but one byte as u8 -- a v1-style read would
// mis-consume and shift the initial instructions, corrupting the CFA.
bool test_du_cie_v3(void) {
    u8             cfa[] = {0x0c, 0x07, 0x10};
    CieSpec        cie = {.version = 3, .aug = "", .data_align = 0x78, .ra = 200, .insns = cfa, .insns_n = sizeof(cfa)};
    DwarfUnwindRow row;
    bool           ok = craft_row_ex(&cie, 0x1000, 0x40, NULL, 0, 0x1000, &row);
    return ok && row.cfa.reg == 7 && row.cfa.offset == 16 && row.return_address_register == 200;
}

// version 4: carries address_size + segment_size; we accept only 8 / 0.
bool test_du_cie_v4(void) {
    u8      cfa[] = {0x0c, 0x07, 0x10};
    CieSpec cie =
        {.version = 4, .aug = "", .addr_size = 8, .data_align = 0x78, .ra = 16, .insns = cfa, .insns_n = sizeof(cfa)};
    DwarfUnwindRow row;
    bool           ok = craft_row_ex(&cie, 0x1000, 0x40, NULL, 0, 0x1000, &row);
    return ok && row.cfa.reg == 7 && row.cfa.offset == 16;
}

// version 4 with a non-8 address_size must be rejected (no CIE -> no FDE).
bool test_du_cie_v4_bad_addr_size(void) {
    u8      cfa[] = {0x0c, 0x07, 0x10};
    CieSpec cie =
        {.version = 4, .aug = "", .addr_size = 4, .data_align = 0x78, .ra = 16, .insns = cfa, .insns_n = sizeof(cfa)};
    DwarfUnwindRow row;
    return !craft_row_ex(&cie, 0x1000, 0x40, NULL, 0, 0x1000, &row);
}

// Unsupported CIE version (2) is rejected.
bool test_du_cie_bad_version(void) {
    u8             cfa[] = {0x0c, 0x07, 0x10};
    CieSpec        cie = {.version = 2, .aug = "", .data_align = 0x78, .ra = 16, .insns = cfa, .insns_n = sizeof(cfa)};
    DwarfUnwindRow row;
    return !craft_row_ex(&cie, 0x1000, 0x40, NULL, 0, 0x1000, &row);
}

// 'zLR': the 'L' arm consumes one LSDA-encoding byte before 'R'. If it were
// mis-consumed, 'R' would read the L byte (0x0b SDATA4) as the FDE encoding
// instead of 0x03 UDATA4 -- and pc_begin=0x80000000 (bit 31 set) decodes
// differently between the two (UDATA4 -> 0x80000000, SDATA4 -> sign-extended),
// so the FDE lookup would miss.
bool test_du_cie_aug_LR(void) {
    u8      cfa[] = {0x0c, 0x07, 0x10};
    CieSpec cie   = {
          .version    = 1,
          .aug        = "zLR",
          .data_align = 0x78,
          .ra         = 16,
          .fde_enc    = 0x03, // UDATA4 FDE encoding
          .lsda_enc   = 0x0b, // SDATA4 LSDA encoding (distinct from R)
          .insns      = cfa,
          .insns_n    = sizeof(cfa)
    };
    DwarfUnwindRow row;
    return craft_row_ex(&cie, 0x80000000, 0x100, NULL, 0, 0x80000000, &row) && row.cfa.reg == 7;
}

// 'zPR': the 'P' arm consumes a personality encoding byte + pointer before 'R'.
bool test_du_cie_aug_PR(void) {
    u8      cfa[] = {0x0c, 0x07, 0x10};
    CieSpec cie   = {
          .version    = 1,
          .aug        = "zPR",
          .data_align = 0x78,
          .ra         = 16,
          .fde_enc    = 0x03,
          .pers_enc   = 0x00, // ABSPTR personality ptr
          .insns      = cfa,
          .insns_n    = sizeof(cfa)
    };
    DwarfUnwindRow row;
    return craft_row_ex(&cie, 0x8000, 0x100, NULL, 0, 0x8000, &row) && row.cfa.reg == 7;
}

// 'zRS' (S = signal frame flag, no data) and an unknown aug char ('Q') must
// both still leave the cursor at aug-end so the FDE encoding resolves.
bool test_du_cie_aug_RS_and_unknown(void) {
    u8      cfa[] = {0x0c, 0x07, 0x10};
    CieSpec rs    = {
           .version    = 1,
           .aug        = "zRS",
           .data_align = 0x78,
           .ra         = 16,
           .fde_enc    = 0x03,
           .insns      = cfa,
           .insns_n    = sizeof(cfa)
    };
    CieSpec q = {
        .version    = 1,
        .aug        = "zQR",
        .data_align = 0x78,
        .ra         = 16,
        .fde_enc    = 0x03,
        .insns      = cfa,
        .insns_n    = sizeof(cfa)
    };
    DwarfUnwindRow row;
    bool           ok = craft_row_ex(&rs, 0x8000, 0x100, NULL, 0, 0x8000, &row) && row.cfa.reg == 7;
    ok                = ok && craft_row_ex(&q, 0x8000, 0x100, NULL, 0, 0x8000, &row) && row.cfa.reg == 7;
    return ok;
}

// remember_state stack overflow: CFI_STATE_STACK (4) pushes are fine; the 5th
// must make the row build fail rather than overflow the saved-state stack.
bool test_du_remember_state_overflow(void) {
    u8             cfa[]  = {0x0c, 0x07, 0x10};
    u8             five[] = {0x0a, 0x0a, 0x0a, 0x0a, 0x0a}; // 5x remember_state
    u8             four[] = {0x0a, 0x0a, 0x0a, 0x0a};       // 4x remember_state
    DwarfUnwindRow row;
    CieSpec        cie = {.version = 1, .aug = "", .data_align = 0x78, .ra = 16, .insns = cfa, .insns_n = sizeof(cfa)};
    bool           four_ok = craft_row_ex(&cie, 0x1000, 0x40, four, sizeof(four), 0x1000, &row);
    bool           five_ok = craft_row_ex(&cie, 0x1000, 0x40, five, sizeof(five), 0x1000, &row);
    return four_ok && !five_ok; // 4 remembers OK, 5th overflows -> build fails
}

// A non-PCREL, non-zero base (datarel) with a signed value exercises
// decode_eh_ptr's "fall back to absolute" arm (result = signed_val).
bool test_du_enc_datarel_sdata4(void) {
    return fde_enc_roundtrips(0x30 | 0x0b, 0x8000, 0x100); // DATAREL | SDATA4
}

// pc_range 0x10000 needs the full 4-byte UDATA4 read; a mis-derived (2-byte)
// range encoding would read 0 and collapse the FDE to an empty span.
bool test_du_fde_large_range(void) {
    u8      cfa[] = {0x0c, 0x07, 0x10};
    CieSpec cie   = {
          .version    = 1,
          .aug        = "zR",
          .data_align = 0x78,
          .ra         = 16,
          .fde_enc    = 0x03 /*UDATA4*/,
          .insns      = cfa,
          .insns_n    = sizeof(cfa)
    };
    DwarfUnwindRow row;
    bool           hit = craft_row_ex(&cie, 0x8000, 0x10000, NULL, 0, 0x12000, &row);
    return hit && row.cfa.reg == 7 && row.cfa.offset == 16;
}

// .debug_frame fallback: build an ELF with ONLY .debug_frame and confirm the
// CFA resolves -- exercising the fallback branch + cfi_parse_section's
// is_debug_frame arms (0xffffffff CIE id, absolute CIE pointer/addresses).
bool test_du_debug_frame_fallback(void) {
    u8             cfa[] = {0x0c, 0x07, 0x10}; // def_cfa r7, 16
    DwarfUnwindRow row;
    bool           hit  = craft_row_debug(cfa, sizeof(cfa), 0x5000, 0x80, NULL, 0, 0x5000, &row);
    bool           miss = craft_row_debug(cfa, sizeof(cfa), 0x5000, 0x80, NULL, 0, 0x4fff, &row);
    return hit && !miss && row.cfa.kind == DWARF_CFA_RULE_REG_OFFSET && row.cfa.reg == 7 && row.cfa.offset == 16;
}

// Run a CIE (RA=16) + the register-writing opcode `ops` (which targets register
// 32 == DWARF_UNWIND_MAX_REGS) and report whether RA survived. regs[32] aliases
// the row's public return_address_register, so a too-loose `reg <= MAX` bound
// clobbers it; a correct `reg < MAX` skips the write. Output goes through a
// slack-padded buffer so the mutant-only regs[32].offset write (past the struct)
// is harmless.
static bool reg32_ra_preserved(const u8 *ops, u64 n) {
    u8              buf[sizeof(DwarfUnwindRow) + 64];
    DwarfUnwindRow *row = (DwarfUnwindRow *)buf;
    CieSpec cie = {.version = 1, .aug = "", .code_align = 1, .data_align = 0x78, .ra = 16, .insns = ops, .insns_n = n};
    return craft_row_ex(&cie, 0x1000, 0x40, NULL, 0, 0x1000, row) && row->return_address_register == 16;
}

// Single contract: every register-writing CFA opcode bounds-checks its register
// index, rejecting reg == DWARF_UNWIND_MAX_REGS (32). Kills the `reg < MAX` ->
// `reg <= MAX` mutants across offset / offset_extended(_sf) / undefined /
// same_value / register / val_offset(_sf) / expression.
bool test_du_reg_index_bounds(void) {
    u8 off[]     = {0x0c, 0x07, 0x10, 0xa0, 0x01};             // DW_CFA_offset reg32
    u8 offx[]    = {0x0c, 0x07, 0x10, 0x05, 0x20, 0x01};       // offset_extended reg32
    u8 offxsf[]  = {0x0c, 0x07, 0x10, 0x11, 0x20, 0x01};       // offset_extended_sf reg32
    u8 undef[]   = {0x0c, 0x07, 0x10, 0x07, 0x20};             // undefined reg32
    u8 same[]    = {0x0c, 0x07, 0x10, 0x08, 0x20};             // same_value reg32
    u8 regop[]   = {0x0c, 0x07, 0x10, 0x09, 0x20, 0x06};       // register reg32 = r6
    u8 valoff[]  = {0x0c, 0x07, 0x10, 0x14, 0x20, 0x01};       // val_offset reg32
    u8 valofsf[] = {0x0c, 0x07, 0x10, 0x15, 0x20, 0x01};       // val_offset_sf reg32
    u8 expr[]    = {0x0c, 0x07, 0x10, 0x10, 0x20, 0x01, 0x00}; // expression reg32
    return reg32_ra_preserved(off, sizeof(off)) && reg32_ra_preserved(offx, sizeof(offx)) &&
           reg32_ra_preserved(offxsf, sizeof(offxsf)) && reg32_ra_preserved(undef, sizeof(undef)) &&
           reg32_ra_preserved(same, sizeof(same)) && reg32_ra_preserved(regop, sizeof(regop)) &&
           reg32_ra_preserved(valoff, sizeof(valoff)) && reg32_ra_preserved(valofsf, sizeof(valofsf)) &&
           reg32_ra_preserved(expr, sizeof(expr));
}

// Single contract: vm_init derives the effective code-alignment factor as
// (factor > 0 ? factor : 1), and advance_loc multiplies its operand by it.
// factor 0 must clamp to 1 (so advance_loc 1 advances by 1, freezing the row at
// pc_begin to the CIE's offset 8); factor 5 must scale (advance_loc 1 advances
// by 5, so pc_begin+4 is still pre-advance, offset 8). Kills the `> 0` -> `>= 0`
// and `> 0` -> `<= 0` mutants.
bool test_du_code_align_factor(void) {
    u8             cfa[] = {0x0c, 0x07, 0x08}; // def_cfa r7, 8
    u8             fde[] = {0x41, 0x0e, 0x18}; // advance_loc 1; def_cfa_offset 24
    DwarfUnwindRow r0, r5;
    CieSpec        c0 =
        {.version = 1, .aug = "", .code_align = 0, .data_align = 0x78, .ra = 16, .insns = cfa, .insns_n = sizeof(cfa)};
    CieSpec c5 =
        {.version = 1, .aug = "", .code_align = 5, .data_align = 0x78, .ra = 16, .insns = cfa, .insns_n = sizeof(cfa)};
    bool ok0 = craft_row_ex(&c0, 0x1000, 0x40, fde, sizeof(fde), 0x1000, &r0); // factor 0 -> clamp 1
    bool ok5 = craft_row_ex(&c5, 0x1000, 0x40, fde, sizeof(fde), 0x1004, &r5); // factor 5, +4 pre-advance
    return ok0 && ok5 && r0.cfa.offset == 8 && r5.cfa.offset == 8;
}

// Two FDEs sharing a CIE: the lookup address lives only in the SECOND FDE, so
// DwarfCfiFindFde's scan must advance past the first (kills the `i++` -> `i--`
// iterator mutant, which a single-FDE fixture can't reach).
bool test_du_two_fdes(void) {
    static u8 eh[256];
    EhWriter  w     = {.buf = eh, .pos = 0};
    u8        cfa[] = {0x0c, 0x07, 0x10};
    u64       cie   = eh_put_cie(&w, cfa, sizeof(cfa));
    eh_put_fde(&w, cie, 0x1000, 0x100, NULL, 0);
    eh_put_fde(&w, cie, 0x2000, 0x100, NULL, 0);
    eh_put_terminator(&w);
    DwarfUnwindRow row;
    return craft_lookup(eh, w.pos, 0x2000, &row) && row.cfa.reg == 7 && row.cfa.offset == 16;
}

// Two CIEs: the FDE references the SECOND one, so DwarfCfiFindCie must scan past
// the first (kills its `i++` -> `i--` iterator mutant). The resolved CIE's
// def_cfa (r6, 16) proves the right CIE was selected.
bool test_du_two_cies(void) {
    static u8 eh[256];
    EhWriter  w      = {.buf = eh, .pos = 0};
    u8        cfa0[] = {0x0c, 0x07, 0x08}; // CIE[0]: def_cfa r7, 8
    u8        cfa1[] = {0x0c, 0x06, 0x10}; // CIE[1]: def_cfa r6, 16
    eh_put_cie(&w, cfa0, sizeof(cfa0));
    u64 cie1 = eh_put_cie(&w, cfa1, sizeof(cfa1));
    eh_put_fde(&w, cie1, 0x1000, 0x100, NULL, 0);
    eh_put_terminator(&w);
    DwarfUnwindRow row;
    return craft_lookup(eh, w.pos, 0x1000, &row) && row.cfa.reg == 6 && row.cfa.offset == 16;
}

// A record (the FDE) whose length exactly equals the section's remaining bytes
// -- no terminator follows. Pins cfi_parse_section's `length32 > remaining`
// bound (a `>=` mutant would reject this valid last record).
bool test_du_record_fills_section(void) {
    static u8 eh[256];
    EhWriter  w     = {.buf = eh, .pos = 0};
    u8        cfa[] = {0x0c, 0x07, 0x10};
    u64       cie   = eh_put_cie(&w, cfa, sizeof(cfa));
    eh_put_fde(&w, cie, 0x1000, 0x100, NULL, 0); // no terminator
    DwarfUnwindRow row;
    return craft_lookup(eh, w.pos, 0x1000, &row) && row.cfa.reg == 7;
}

// An ELF with an EMPTY .eh_frame plus a populated .debug_frame: build_from_elf
// must fall back to .debug_frame (its `eh->size > 0` guard). A `>= 0` mutant
// would pick the empty .eh_frame and find no FDE.
bool test_du_eh_empty_debug_fallback(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);
    u8               cfa[] = {0x0c, 0x07, 0x10};
    static u8        df[256];
    u64              df_len = build_debug_frame(df, cfa, sizeof(cfa), 0x5000, 0x80, NULL, 0);
    static u8        elfbuf[4096];
    u64              elf_len = 0;
    build_elf_eh_and_debug(elfbuf, &elf_len, df, df_len, EH_SECTION_VADDR);

    bool ok = false;
    Elf  elf;
    if (ElfOpenFromMemoryCopy(&elf, elfbuf, (size)elf_len, base)) {
        DwarfCfi cfi;
        if (DwarfCfiBuildFromElf(&cfi, &elf, base)) {
            const DwarfFde *fde = DwarfCfiFindFde(&cfi, 0x5000);
            DwarfUnwindRow  row;
            // eh_frame_addr must record the .debug_frame section's address (the
            // fallback branch), not a constant.
            ok = fde != NULL && DwarfCfiBuildRow(&cfi, fde, 0x5000, &row) && row.cfa.reg == 7 &&
                 cfi.eh_frame_addr == EH_SECTION_VADDR;
            DwarfCfiDeinit(&cfi);
        }
        ElfDeinit(&elf);
    }
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// FDE augmentation data (3 bytes) must be skipped before the instructions. If
// parse_fde drops the aug-len read, the 0xFF filler runs as an invalid opcode
// and the row build fails -- so a clean build proves the skip happened.
bool test_du_fde_aug_skipped(void) {
    CieSpec cie = {
        .version    = 1,
        .aug        = "zR",
        .code_align = 1,
        .data_align = 0x78,
        .ra         = 16,
        .fde_enc    = 0x03 /*UDATA4*/,
        .fde_aug_n  = 3
    };
    u8             fde[] = {0x0c, 0x07, 0x10}; // def_cfa r7,16 in the FDE
    DwarfUnwindRow row;
    return craft_row_ex(&cie, 0x8000, 0x100, fde, sizeof(fde), 0x8000, &row) && row.cfa.reg == 7;
}

// A 'zR' CIE with no initial instructions: the augmentation data fills the CIE
// body exactly, so aug_len == remaining. Pins parse_cie's `aug_len > remaining`
// bound (a `>=` mutant would reject this valid CIE -> the FDE loses its CIE).
bool test_du_cie_aug_fills_body(void) {
    CieSpec        cie   = {.version = 1, .aug = "zR", .code_align = 1, .data_align = 0x78, .ra = 16, .fde_enc = 0x03};
    u8             fde[] = {0x0c, 0x07, 0x10}; // def_cfa in the FDE
    DwarfUnwindRow row;
    return craft_row_ex(&cie, 0x8000, 0x100, fde, sizeof(fde), 0x8000, &row) && row.cfa.reg == 7;
}

// DwarfCfiFindFde's upper bound is exclusive: querying pc_begin+pc_range
// directly (bypassing BuildRow's own range check, which would otherwise mask
// it) must miss. Kills FindFde's `vaddr < end` -> `<=` mutant.
bool test_du_findfde_upper_bound_exclusive(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);
    u8               cfa[] = {0x0c, 0x07, 0x10};
    static u8        eh[256];
    u64              eh_len = build_eh_frame(eh, cfa, sizeof(cfa), 0x1000, 0x100, NULL, 0);
    static u8        elfbuf[4096];
    u64              elf_len = 0;
    build_elf_with_eh_frame(elfbuf, &elf_len, eh, eh_len, 0x4000);

    bool ok = false;
    Elf  elf;
    if (ElfOpenFromMemoryCopy(&elf, elfbuf, (size)elf_len, base)) {
        DwarfCfi cfi;
        if (DwarfCfiBuildFromElf(&cfi, &elf, base)) {
            ok = DwarfCfiFindFde(&cfi, 0x10ff) != NULL  // last in-range address
              && DwarfCfiFindFde(&cfi, 0x1100) == NULL; // == end: exclusive -> miss
            DwarfCfiDeinit(&cfi);
        }
        ElfDeinit(&elf);
    }
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// DwarfCfiBuildRow rejects a target at/after the FDE's end. Calling it directly
// (with an FDE obtained for an in-range address) exercises its own bound,
// independent of FindFde. Kills BuildRow's `target >= end` -> `>` mutant.
bool test_du_buildrow_rejects_out_of_range(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);
    u8               cfa[] = {0x0c, 0x07, 0x10};
    static u8        eh[256];
    u64              eh_len = build_eh_frame(eh, cfa, sizeof(cfa), 0x1000, 0x100, NULL, 0);
    static u8        elfbuf[4096];
    u64              elf_len = 0;
    build_elf_with_eh_frame(elfbuf, &elf_len, eh, eh_len, 0x4000);

    bool ok = false;
    Elf  elf;
    if (ElfOpenFromMemoryCopy(&elf, elfbuf, (size)elf_len, base)) {
        DwarfCfi cfi;
        if (DwarfCfiBuildFromElf(&cfi, &elf, base)) {
            const DwarfFde *fde = DwarfCfiFindFde(&cfi, 0x1000);
            DwarfUnwindRow  row;
            ok = fde != NULL && DwarfCfiBuildRow(&cfi, fde, 0x1080, &row) // in range -> ok
              && !DwarfCfiBuildRow(&cfi, fde, 0x1100, &row);              // == end -> rejected
            DwarfCfiDeinit(&cfi);
        }
        ElfDeinit(&elf);
    }
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// The CIE's decoded fields are observable via DwarfCfiFindCie + DwarfCfi's
// public members. Pins the stored `version`, `has_augmentation`, and the
// build's `eh_frame_addr` (all are `= …` assigns whose mutants would survive
// without a reader).
bool test_du_cie_fields_observable(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);
    u8               cfa[] = {0x0c, 0x07, 0x10};
    CieSpec          c     = {
                     .version    = 1,
                     .aug        = "zR",
                     .code_align = 1,
                     .data_align = 0x78,
                     .ra         = 16,
                     .fde_enc    = 0x03,
                     .insns      = cfa,
                     .insns_n    = sizeof(cfa)
    };
    static u8 eh[256];
    u64       eh_len = build_cfi_ex(eh, &c, 0x8000, 0x100, NULL, 0);
    static u8 elfbuf[4096];
    u64       elf_len = 0;
    build_elf_with_eh_frame(elfbuf, &elf_len, eh, eh_len, EH_SECTION_VADDR);

    bool ok = false;
    Elf  elf;
    if (ElfOpenFromMemoryCopy(&elf, elfbuf, (size)elf_len, base)) {
        DwarfCfi cfi;
        if (DwarfCfiBuildFromElf(&cfi, &elf, base)) {
            const DwarfCie *cie = DwarfCfiFindCie(&cfi, 0);
            ok                  = cie != NULL && cie->version == 1 && cie->has_augmentation == 1 && cie->offset == 0 &&
                 cie->fde_pointer_encoding == 0x03 && cfi.eh_frame_addr == EH_SECTION_VADDR;
            DwarfCfiDeinit(&cfi);
        }
        ElfDeinit(&elf);
    }
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// The FDE's `offset` (its position within the section) is observable via
// DwarfCfiFindFde. Pins the `out->offset = body_start - section_data` assign
// (a `= 42` or `- -> +` mutant would not match the writer-known offset).
bool test_du_fde_offset_observable(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);
    static u8        eh[256];
    EhWriter         w       = {.buf = eh, .pos = 0};
    u8               cfa[]   = {0x0c, 0x07, 0x10};
    u64              cie     = eh_put_cie(&w, cfa, sizeof(cfa));
    u64              fde_off = eh_put_fde(&w, cie, 0x1000, 0x100, NULL, 0);
    eh_put_terminator(&w);
    static u8 elfbuf[4096];
    u64       elf_len = 0;
    build_elf_with_eh_frame(elfbuf, &elf_len, eh, w.pos, EH_SECTION_VADDR);

    bool ok = false;
    Elf  elf;
    if (ElfOpenFromMemoryCopy(&elf, elfbuf, (size)elf_len, base)) {
        DwarfCfi cfi;
        if (DwarfCfiBuildFromElf(&cfi, &elf, base)) {
            const DwarfFde *fde = DwarfCfiFindFde(&cfi, 0x1000);
            ok                  = fde != NULL && fde->offset == fde_off;
            DwarfCfiDeinit(&cfi);
        }
        ElfDeinit(&elf);
    }
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// An advance whose (delta * code_align) overflows u64 must saturate the next-PC
// to UINT64_MAX, so a target just past pc_begin stays frozen at the pre-advance
// row. code_align is attacker-controlled (CIE ULEB), so this path is reachable.
// A `next = 42` mutant would instead let the target advance. Covers the
// high-bit advance and advance_loc1/2/4.
static bool advance_saturates(const u8 *adv, u64 adv_n) {
    u8  cfa[] = {0x0c, 0x07, 0x08}; // def_cfa r7, 8
    u8  fde[16];
    u64 n = 0;
    for (u64 i = 0; i < adv_n; i++)
        fde[n++] = adv[i];
    fde[n++]  = 0x0e; // def_cfa_offset 24
    fde[n++]  = 0x18;
    CieSpec c = {
        .version        = 1,
        .aug            = "",
        .code_align_big = (u64)1 << 61, // delta(8) * 2^61 = 2^64 -> overflow
        .data_align     = 0x78,
        .ra             = 16,
        .insns          = cfa,
        .insns_n        = sizeof(cfa)
    };
    DwarfUnwindRow row;
    bool           ok = craft_row_ex(&c, 0x1000, 0x200, fde, n, 0x1001, &row);
    return ok && row.cfa.offset == 8;         // frozen pre-advance: saturation covered [pc_begin, MAX)
}
bool test_du_advance_overflow_saturates(void) {
    u8 hi[] = {0x48};                         // advance_loc 8 (high-bit)
    u8 l1[] = {0x02, 0x08};                   // advance_loc1 8
    u8 l2[] = {0x03, 0x08, 0x00};             // advance_loc2 8
    u8 l4[] = {0x04, 0x08, 0x00, 0x00, 0x00}; // advance_loc4 8
    return advance_saturates(hi, sizeof(hi)) && advance_saturates(l1, sizeof(l1)) &&
           advance_saturates(l2, sizeof(l2)) && advance_saturates(l4, sizeof(l4));
}

// An advance's location update must propagate to a following advance: the
// second advance computes next from the first's new location. A `location = 42`
// mutant would misplace the second advance's region. Covers the high-bit
// advance (first fixture) and advance_loc4 (second).
bool test_du_advance_location_propagates(void) {
    u8      cfa[]    = {0x0c, 0x07, 0x08};
    u8      fde_hi[] = {0x44, 0x0e, 0x18, 0x44, 0x0e, 0x30};                // adv4; off24; adv4; off48
    u8      fde_l4[] = {0x04, 0x04, 0, 0, 0, 0x0e, 0x18, 0x44, 0x0e, 0x30}; // loc4 adv4; off24; adv4; off48
    CieSpec c =
        {.version = 1, .aug = "", .code_align = 1, .data_align = 0x78, .ra = 16, .insns = cfa, .insns_n = sizeof(cfa)};
    DwarfUnwindRow r1, r2;
    bool           ok = craft_row_ex(&c, 0x1000, 0x200, fde_hi, sizeof(fde_hi), 0x1006, &r1) &&
              craft_row_ex(&c, 0x1000, 0x200, fde_l4, sizeof(fde_l4), 0x1006, &r2);
    return ok && r1.cfa.offset == 24 && r2.cfa.offset == 24;
}

int main(void) {
    WriteFmt("[INFO] Starting DwarfUnwind tests\n\n");

    TestFunction tests[] = {
        test_du_def_cfa,
        test_du_def_cfa_offset_and_register,
        test_du_offset_rule,
        test_du_advance_loc_changes_row,
        test_du_remember_restore_state,
        test_du_nop,
        test_du_offset_extended,
        test_du_undefined,
        test_du_same_value,
        test_du_register,
        test_du_restore_extended,
        test_du_restore_highbit,
        test_du_advance_loc124,
        test_du_def_cfa_sf,
        test_du_offset_extended_sf,
        test_du_val_offset,
        test_du_expression_kinds,
        test_du_def_cfa_offset_standalone,
        test_du_def_cfa_register_standalone,
        test_du_def_cfa_offset_sf,
        test_du_val_offset_sf,
        test_du_set_loc,
        test_du_enc_udata2,
        test_du_enc_udata4,
        test_du_enc_udata8,
        test_du_enc_uleb128,
        test_du_enc_sdata2,
        test_du_enc_sdata4,
        test_du_enc_sdata8,
        test_du_enc_sleb128,
        test_du_enc_pcrel_sdata4,
        test_du_enc_pcrel_udata4,
        test_du_enc_indirect_fails,
        test_du_enc_omit_fails,
        test_du_cie_v3,
        test_du_cie_v4,
        test_du_cie_v4_bad_addr_size,
        test_du_cie_bad_version,
        test_du_cie_aug_LR,
        test_du_cie_aug_PR,
        test_du_cie_aug_RS_and_unknown,
        test_du_remember_state_overflow,
        test_du_enc_datarel_sdata4,
        test_du_fde_large_range,
        test_du_debug_frame_fallback,
        test_du_reg_index_bounds,
        test_du_code_align_factor,
        test_du_two_fdes,
        test_du_two_cies,
        test_du_record_fills_section,
        test_du_eh_empty_debug_fallback,
        test_du_fde_aug_skipped,
        test_du_cie_aug_fills_body,
        test_du_findfde_upper_bound_exclusive,
        test_du_buildrow_rejects_out_of_range,
        test_du_cie_fields_observable,
        test_du_fde_offset_observable,
        test_du_advance_overflow_saturates,
        test_du_advance_location_propagates,
    };

    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), NULL, 0, "DwarfUnwind");
}
