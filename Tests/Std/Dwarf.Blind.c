#include <Misra.h>
#include <Misra/Parsers/Dwarf.h>
#include <Misra/Parsers/Elf.h>
#include <Misra/Std/Allocator/Debug.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Memory.h>
#include <Misra/Std/Zstr.h>

#include "../Util/TestRunner.h"

// ---------------------------------------------------------------------------
// Self-contained byte helpers (cannot reference Dwarf.c internals).
// ---------------------------------------------------------------------------
static void put_u16(u8 *p, u16 v) {
    p[0] = (u8)(v & 0xff);
    p[1] = (u8)(v >> 8);
}
static void put_u32(u8 *p, u32 v) {
    p[0] = (u8)(v & 0xff);
    p[1] = (u8)(v >> 8);
    p[2] = (u8)(v >> 16);
    p[3] = (u8)(v >> 24);
}
static void put_u64(u8 *p, u64 v) {
    for (u32 i = 0; i < 8; ++i)
        p[i] = (u8)(v >> (8 * i));
}

static const u8 kStdOpcodeLengths[12] = {0, 1, 1, 1, 1, 0, 0, 0, 1, 0, 0, 1};

enum {
    DWLNS_COPY         = 0x01,
    DWLNS_ADVANCE_PC   = 0x02,
    DWLNS_ADVANCE_LINE = 0x03,
    DWLNS_SET_FILE     = 0x04,
    DWLNE_END_SEQUENCE = 0x01,
    DWLNE_SET_ADDRESS  = 0x02,
    DWLNE_SET_DISC     = 0x04,
};

static u8 *emit_set_address8(u8 *p, u64 addr) {
    *p++ = 0x00;
    *p++ = 9;
    *p++ = DWLNE_SET_ADDRESS;
    put_u64(p, addr);
    return p + 8;
}

// Build a DWARF-4 .debug_line CU with a configurable directory/file table.
// `ndirs` directory names "dirA","dirB",... and `nfiles` files "fileA.c",...
// with file i assigned dir index `file_dir_idx[i]`. The line-number program
// `prog`/`prog_len` follows.
static u64 build_line_cu(u8 *buf, u32 ndirs, u32 nfiles, const u8 *file_dir_idx, const u8 *prog, u64 prog_len) {
    u8 *p = buf + 4;        // reserve unit_length
    put_u16(p, 4);
    p                 += 2; // version
    u8 *hdr_len_field  = p;
    p                 += 4;
    u8 *after_hdr_len  = p;
    *p++               = 1;        // min_instr_len
    *p++               = 1;        // max_ops_per_instr
    *p++               = 1;        // default_is_stmt
    *p++               = (u8)(-5); // line_base
    *p++               = 14;       // line_range
    *p++               = 13;       // opcode_base
    for (u32 i = 0; i < 12; ++i)
        *p++ = kStdOpcodeLengths[i];

    // include_directories
    for (u32 d = 0; d < ndirs; ++d) {
        const char base[] = "dir";
        MemCopy(p, base, 3);
        p    += 3;
        *p++  = (u8)('A' + d);
        *p++  = 0x00;
    }
    *p++ = 0x00; // include_directories terminator

    // file_names
    for (u32 f = 0; f < nfiles; ++f) {
        const char base[] = "file";
        MemCopy(p, base, 4);
        p    += 4;
        *p++  = (u8)('A' + f);
        *p++  = '.';
        *p++  = 'c';
        *p++  = 0x00;
        *p++  = file_dir_idx[f]; // dir index (uleb)
        *p++  = 0x00;            // mtime
        *p++  = 0x00;            // size
    }
    *p++ = 0x00;                 // file_names terminator

    put_u32(hdr_len_field, (u32)(p - after_hdr_len));

    MemCopy(p, prog, prog_len);
    p += prog_len;

    u32 body_len = (u32)(p - (buf + 4));
    put_u32(buf, body_len);
    return (u64)(p - buf);
}

// Wrap a .debug_line payload in a minimal ELF64 (null, .shstrtab, .debug_line).
static void build_elf(u8 *elf, u64 *out_len, const u8 *dl, u64 dl_len) {
    MemSet(elf, 0, 8192);
    static const char shstr[]            = "\0.shstrtab\0.debug_line";
    const u32         shstrtab_name_off  = 1;
    const u32         debugline_name_off = 11;
    u64               shstr_len          = sizeof(shstr);

    u64 ehdr_size = 64;
    u64 shstr_off = ehdr_size;
    u64 dl_off    = shstr_off + shstr_len;
    u64 shtab_off = dl_off + dl_len;

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
    MemCopy(elf + dl_off, dl, dl_len);

    u8 *sh = elf + shtab_off;
    u8 *s1 = sh + 64;
    put_u32(s1 + 0, shstrtab_name_off);
    put_u32(s1 + 4, 3);
    put_u64(s1 + 24, shstr_off);
    put_u64(s1 + 32, shstr_len);
    u8 *s2 = sh + 128;
    put_u32(s2 + 0, debugline_name_off);
    put_u32(s2 + 4, 1);
    put_u64(s2 + 24, dl_off);
    put_u64(s2 + 32, dl_len);

    *out_len = shtab_off + 3 * 64;
}

static bool build_lines(DwarfLines *out, Elf *elf, const u8 *dl, u64 dl_len, Allocator *base) {
    static u8 elfbuf[8192];
    u64       elf_len = 0;
    build_elf(elfbuf, &elf_len, dl, dl_len);
    if (!ElfOpenFromMemoryCopy(elf, elfbuf, (size)elf_len, base))
        return false;
    return DwarfLinesBuildFromElf(out, elf, base);
}

// ===========================================================================
// 314:39  lnp_emit  `st->file - 1 < VecLen(&cs->file_offsets)`  (lt_to_le)
//
// file is 1-based; file_offsets has N entries. Selecting file == N+1 makes
// `st->file - 1 == N == VecLen`. Real `<`: out of range -> file_off stays 0
// -> e->file == NULL. Mutant `<=`: passes the guard and indexes one past the
// end. With exactly 1 file (N=1), SET_FILE 2 gives st->file-1 == 1 == VecLen.
// ===========================================================================
bool test_blind_set_file_past_end_yields_null_file(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    u8  fdi[1] = {1};
    u8  prog[64];
    u8 *p = prog;
    *p++  = DWLNS_SET_FILE;
    *p++  = 2;          // one past the single file (N=1)
    p     = emit_set_address8(p, 0x2000);
    *p++  = DWLNS_ADVANCE_LINE;
    *p++  = 9;          // line 1 -> 10
    *p++  = DWLNS_COPY; // row at 0x2000, file 2 (out of range)
    *p++  = DWLNS_ADVANCE_PC;
    *p++  = 8;
    *p++  = 0x00;
    *p++  = 1;
    *p++  = DWLNE_END_SEQUENCE;

    u8         dl[512];
    u64        dl_len = build_line_cu(dl, 1, 1, fdi, prog, (u64)(p - prog));
    Elf        elf;
    DwarfLines lines;
    if (!build_lines(&lines, &elf, dl, dl_len, base)) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    bool                  ok = true;
    const DwarfLineEntry *e  = DwarfLinesResolve(&lines, 0x2000);
    ok                       = ok && e && e->line == 10;
    // Real code: file index 2 is out of range -> NULL file.
    ok = ok && e && e->file == NULL;

    DwarfLinesDeinit(&lines);
    ElfDeinit(&elf);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ===========================================================================
// 317:41  lnp_emit  `dir_idx - 1 < VecLen(&cs->dir_offsets)`  (lt_to_le)
//
// dir_idx is 1-based; dir_offsets has M entries. A file whose dir index is
// M+1 makes `dir_idx - 1 == M == VecLen`. Real `<`: out of range -> dir_off
// stays 0 -> e->dir == NULL. Mutant `<=`: passes the guard, indexes one past
// the end. Use M=1 dir, file dir index 2.
// ===========================================================================
bool test_blind_dir_idx_past_end_yields_null_dir(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    // 1 dir, 1 file, file's dir index = 2 (one past the single dir).
    u8  fdi[1] = {2};
    u8  prog[64];
    u8 *p = prog;
    p     = emit_set_address8(p, 0x3000);
    *p++  = DWLNS_ADVANCE_LINE;
    *p++  = 6;          // line 1 -> 7
    *p++  = DWLNS_COPY; // row at 0x3000, default file 1, dir idx 2 (OOR)
    *p++  = DWLNS_ADVANCE_PC;
    *p++  = 8;
    *p++  = 0x00;
    *p++  = 1;
    *p++  = DWLNE_END_SEQUENCE;

    u8         dl[512];
    u64        dl_len = build_line_cu(dl, 1, 1, fdi, prog, (u64)(p - prog));
    Elf        elf;
    DwarfLines lines;
    if (!build_lines(&lines, &elf, dl, dl_len, base)) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    bool                  ok = true;
    const DwarfLineEntry *e  = DwarfLinesResolve(&lines, 0x3000);
    ok                       = ok && e && e->line == 7;
    ok                       = ok && e && e->file != NULL; // file itself resolves
    // Real code: dir index 2 is out of range (only 1 dir) -> NULL dir.
    ok = ok && e && e->dir == NULL;

    DwarfLinesDeinit(&lines);
    ElfDeinit(&elf);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ===========================================================================
// 387:26  run_line_program  SET_DISCRIMINATOR `BufReadULeb128(&cur, &disc)`
// (cxx_replace_scalar_call -> the read's return value becomes 42, truthy).
//
// On a TRUNCATED discriminator ULEB the real read returns false and the build
// fails. The mutant ignores the failure (42 is truthy), keeps going, and the
// build succeeds. We craft a valid first row, then a SET_DISCRIMINATOR whose
// ULEB consists only of continuation bytes running to the end of the unit, so
// BufReadULeb128 hits the buffer end and returns false.
//   Real:  build fails (returns false).
//   Mutant: build succeeds (returns true) with a resolvable row at 0x9000.
// ===========================================================================
bool test_blind_set_discriminator_truncated_uleb_rejected(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    u8  fdi[1] = {1};
    u8  prog[64];
    u8 *p = prog;
    p     = emit_set_address8(p, 0x9000);
    *p++  = DWLNS_ADVANCE_LINE;
    *p++  = 9;          // line 1 -> 10
    *p++  = DWLNS_COPY; // valid row at 0x9000
    // SET_DISCRIMINATOR extended op with an unterminated ULEB: marker, length,
    // sub_op, then continuation-only bytes that run to the end of the unit.
    *p++ = 0x00;
    *p++ = 4;    // length = sub_op(1) + 3 continuation bytes
    *p++ = DWLNE_SET_DISC;
    *p++ = 0x80; // continuation bit set ...
    *p++ = 0x80;
    *p++ = 0x80; // ... and no terminating byte before unit end

    u8         dl[512];
    u64        dl_len = build_line_cu(dl, 1, 1, fdi, prog, (u64)(p - prog));
    Elf        elf;
    DwarfLines lines;
    bool       built = build_lines(&lines, &elf, dl, dl_len, base);

    bool ok;
    if (!built) {
        // Real code rejects the truncated discriminator ULEB.
        ok = true;
    } else {
        // A survivor that ignored the failed read built a (bogus) table and
        // resolves the leading row -> kill condition.
        ok = (DwarfLinesResolve(&lines, 0x9000) == NULL);
        DwarfLinesDeinit(&lines);
    }

    ElfDeinit(&elf);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

int main(void) {
    WriteFmt("[INFO] Starting Dwarf.Blind tests\n\n");

    TestFunction tests[] = {
        test_blind_set_file_past_end_yields_null_file,
        test_blind_dir_idx_past_end_yields_null_dir,
        test_blind_set_discriminator_truncated_uleb_rejected,
    };

    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), NULL, 0, "Dwarf.Blind");
}
