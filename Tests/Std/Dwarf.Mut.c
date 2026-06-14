/// file : tests/std/dwarf.mut.c
/// Targeted mutation-kill tests for Dwarf (.debug_line parser): each test drives
/// an input that makes a specific surviving mutant produce an observably-wrong
/// result. Self-contained DWARF-4 `.debug_line` fixtures wrapped in a minimal
/// in-memory ELF. Distinct from the existing Dwarf.* tests.
///
/// Two kill mechanisms are used:
///   - value kills: a crafted program whose resolved (address/line/file/dir)
///     differs between the real code and the mutant;
///   - leak kills: route every allocation through a DebugAllocator and assert
///     DebugAllocatorLiveCount(&dbg)==0 after teardown, so a dropped internal
///     *Deinit on a reachable error branch (cxx_remove_void_call) becomes a
///     detectable leak.
#include <Misra.h>
#include <Misra/Parsers/Dwarf.h>
#include <Misra/Parsers/Elf.h>
#include <Misra/Std/Allocator/Debug.h>
#include <Misra/Std/Container/Vec.h>
#include <Misra/Std/Memory.h>
#include <Misra/Std/Zstr.h>

#include "../Util/TestRunner.h"

// ---------------------------------------------------------------------------
// Little-endian writers.
// ---------------------------------------------------------------------------
static void put_u16(u8 *p, u16 v) {
    p[0] = (u8)(v & 0xff);
    p[1] = (u8)((v >> 8) & 0xff);
}
static void put_u32(u8 *p, u32 v) {
    for (u32 i = 0; i < 4; ++i)
        p[i] = (u8)((v >> (8 * i)) & 0xff);
}
static void put_u64(u8 *p, u64 v) {
    for (u32 i = 0; i < 8; ++i)
        p[i] = (u8)((v >> (8 * i)) & 0xff);
}

// Standard-opcode operand counts for opcodes 1..12 (the GCC/clang defaults).
static const u8 kStdOpcodeLengths[12] = {0, 1, 1, 1, 1, 0, 0, 0, 1, 0, 0, 1};

enum {
    DWLNS_COPY             = 0x01,
    DWLNS_ADVANCE_PC       = 0x02,
    DWLNS_ADVANCE_LINE     = 0x03,
    DWLNS_SET_FILE         = 0x04,
    DWLNS_CONST_ADD_PC     = 0x08,
    DWLNS_FIXED_ADVANCE_PC = 0x09,
    DWLNE_END_SEQUENCE     = 0x01,
    DWLNE_SET_ADDRESS      = 0x02,
};

// Header constants shared by every fixture (DWARF-4):
//   min_instr_len = 1, line_base = -5, line_range = 14, opcode_base = 13.
// Special-opcode math: adjusted = op - 13; op_adv = adjusted / 14;
//                      line_adv = -5 + (adjusted % 14).
// const_add_pc advance: adjusted = 255 - 13 = 242; op_adv = 242/14 = 17.
enum {
    HDR_MIN_INSTR_LEN = 1,
    HDR_LINE_BASE     = (u8)(-5),
    HDR_LINE_RANGE    = 14,
    HDR_OPCODE_BASE   = 13,
};

// Two include_directories ("dir_one","dir_two") and two file_names
// ("first.c" dir 1, "second.c" dir 2). Returns the program-write pointer.
typedef struct LineFixture {
    u8 *unit_len_field;
} LineFixture;

static u8 *write_line_header(u8 *buf, LineFixture *fx) {
    u8 *p               = buf;
    fx->unit_len_field  = p;
    p                  += 4; // unit_length (filled later)
    put_u16(p, 4);
    p                 += 2;  // version
    u8 *hdr_len_field  = p;
    p                 += 4;  // header_length (filled after tables)
    u8 *after_hdr_len  = p;
    *p++               = HDR_MIN_INSTR_LEN;
    *p++               = 1;  // maximum_operations_per_instruction
    *p++               = 1;  // default_is_stmt
    *p++               = HDR_LINE_BASE;
    *p++               = HDR_LINE_RANGE;
    *p++               = HDR_OPCODE_BASE;
    for (u32 i = 0; i < 12; ++i)
        *p++ = kStdOpcodeLengths[i]; // standard_opcode_lengths

    const char d1[] = "dir_one";
    MemCopy(p, d1, sizeof(d1));
    p               += sizeof(d1);
    const char d2[]  = "dir_two";
    MemCopy(p, d2, sizeof(d2));
    p    += sizeof(d2);
    *p++  = 0x00; // include_directories terminator

    const char f1[] = "first.c";
    MemCopy(p, f1, sizeof(f1));
    p               += sizeof(f1);
    *p++             = 0x01; // dir index 1 -> "dir_one"
    *p++             = 0x00; // mtime
    *p++             = 0x00; // size
    const char f2[]  = "second.c";
    MemCopy(p, f2, sizeof(f2));
    p    += sizeof(f2);
    *p++  = 0x02; // dir index 2 -> "dir_two"
    *p++  = 0x00; // mtime
    *p++  = 0x00; // size
    *p++  = 0x00; // file_names terminator

    put_u32(hdr_len_field, (u32)(p - after_hdr_len));
    return p;
}

static u64 finalize_unit(const LineFixture *fx, u8 *buf, u8 *p) {
    put_u32(fx->unit_len_field, (u32)(p - (fx->unit_len_field + 4)));
    return (u64)(p - buf);
}

static u8 *emit_set_address8(u8 *p, u64 addr) {
    *p++ = 0x00; // extended marker
    *p++ = 9;    // length = sub_op(1) + addr(8)
    *p++ = DWLNE_SET_ADDRESS;
    put_u64(p, addr);
    return p + 8;
}

static u8 *emit_end_sequence(u8 *p) {
    *p++ = 0x00;
    *p++ = 1;
    *p++ = DWLNE_END_SEQUENCE;
    return p;
}

// Wrap `dl` in a minimal ELF64 (null/.shstrtab/.debug_line) into `elf`.
static void build_elf_with_debug_line(u8 *elf, u64 elf_cap, u64 *out_len, const u8 *dl, u64 dl_len) {
    MemSet(elf, 0, (size)elf_cap);
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
    elf[4] = 2; // ELFCLASS64
    elf[5] = 1; // ELFDATA2LSB
    elf[6] = 1; // EV_CURRENT
    put_u16(elf + 16, 2);
    put_u16(elf + 18, 62);
    put_u32(elf + 20, 1);
    put_u64(elf + 40, shtab_off); // e_shoff
    put_u16(elf + 52, 64);        // e_ehsize
    put_u16(elf + 58, 64);        // e_shentsize
    put_u16(elf + 60, 3);         // e_shnum
    put_u16(elf + 62, 1);         // e_shstrndx

    MemCopy(elf + shstr_off, shstr, shstr_len);
    MemCopy(elf + dl_off, dl, dl_len);

    u8 *sh = elf + shtab_off;
    u8 *s1 = sh + 64;
    put_u32(s1 + 0, shstrtab_name_off);
    put_u32(s1 + 4, 3); // SHT_STRTAB
    put_u64(s1 + 24, shstr_off);
    put_u64(s1 + 32, shstr_len);
    u8 *s2 = sh + 128;
    put_u32(s2 + 0, debugline_name_off);
    put_u32(s2 + 4, 1); // SHT_PROGBITS
    put_u64(s2 + 24, dl_off);
    put_u64(s2 + 32, dl_len);

    *out_len = shtab_off + 3 * 64;
}

// Build a DwarfLines from a raw `.debug_line` payload through `base`.
// Returns the build result; on true the caller owns `*lines` + `*elf`.
static bool build_from_dl(DwarfLines *lines, Elf *elf, Allocator *base, const u8 *dl, u64 dl_len) {
    static u8 elfbuf[8192];
    u64       elf_len = 0;
    build_elf_with_debug_line(elfbuf, sizeof(elfbuf), &elf_len, dl, dl_len);
    if (!ElfOpenFromMemoryCopy(elf, elfbuf, (size)elf_len, base))
        return false;
    bool built = DwarfLinesBuildFromElf(lines, elf, base);
    return built;
}

// Lean leak-checking DebugAllocator config (no traces / overflow / history).
static DebugAllocator leak_alloc(void) {
    DebugAllocatorConfig cfg = DEBUG_ALLOCATOR_DEFAULTS;
    cfg.capture_traces       = false;
    cfg.detect_overflow      = false;
    cfg.track_freed_history  = false;
    return DebugAllocatorInitWith(cfg);
}

// ===========================================================================
// VALUE KILLS
// ===========================================================================

// 312:9  `u64 file_off = 0` -> `= 42`.
// When `st->file` is out of range, file_off must stay 0 so e->file is NULL.
// `= 42` makes e->file point at (pool + 42) -- a bogus string. Set file to a
// huge index (way past the 2-entry file table) so the in-range branch is
// skipped and file_off keeps its initial value.
bool test_dwm_file_off_init_zero_when_file_oob(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    u8          dl[512];
    LineFixture fx;
    u8         *p = write_line_header(dl, &fx);
    *p++          = DWLNS_SET_FILE;
    *p++          = 0x40; // file = 64 (out of range: only 2 files)
    p             = emit_set_address8(p, 0x3000);
    *p++          = DWLNS_COPY;
    *p++          = DWLNS_ADVANCE_PC;
    *p++          = 8;
    p             = emit_end_sequence(p);
    u64 dl_len    = finalize_unit(&fx, dl, p);

    Elf        elf;
    DwarfLines lines;
    bool       built = build_from_dl(&lines, &elf, base, dl, dl_len);
    bool       ok    = built;
    if (built) {
        const DwarfLineEntry *e = DwarfLinesResolve(&lines, 0x3000);
        // Out-of-range file -> no file string (NULL), not a bogus pointer.
        ok = ok && e && e->file == NULL;
        DwarfLinesDeinit(&lines);
        ElfDeinit(&elf);
    }
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// 313:9  `u64 dir_off = 0` -> `= 42`.
// A file whose dir index is out of range must yield a NULL dir. With `= 42`,
// dir_off is non-zero and e->dir points at (pool + 42). Set the current file's
// dir index out of range by selecting a file then... the table dir indices are
// fixed at 1 and 2, both in range. Instead use the default-file path with a
// file whose dir index is 0: file table here is fixed, so craft a third file
// with dir 0 is not possible with the shared header. Use SET_FILE to a file
// whose dir is in-range is wrong. Simpler: file index that is in-range for the
// file table but whose recorded dir index is 0 -> dir stays NULL. "dir_two"
// /"first.c" do not give dir 0; so build a one-off header with a file dir 0.
bool test_dwm_dir_off_init_zero_when_dir_zero(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    // One-off header: file "only.c" with dir index 0 (no directory).
    u8  dl[256];
    u8 *q               = dl;
    u8 *unit_len_field  = q;
    q                  += 4;
    put_u16(q, 4);
    q                 += 2;
    u8 *hdr_len_field  = q;
    q                 += 4;
    u8 *after_hdr_len  = q;
    *q++               = HDR_MIN_INSTR_LEN;
    *q++               = 1;
    *q++               = 1;
    *q++               = HDR_LINE_BASE;
    *q++               = HDR_LINE_RANGE;
    *q++               = HDR_OPCODE_BASE;
    for (u32 i = 0; i < 12; ++i)
        *q++ = kStdOpcodeLengths[i];
    // include_directories: one dir so the pool is non-empty (file lands at a
    // non-zero offset, so the file-present sentinel still works).
    const char d1[] = "somedir";
    MemCopy(q, d1, sizeof(d1));
    q    += sizeof(d1);
    *q++  = 0x00; // include_directories terminator
    // file_names: "only.c" with dir index 0 (root / no directory).
    const char f1[] = "only.c";
    MemCopy(q, f1, sizeof(f1));
    q    += sizeof(f1);
    *q++  = 0x00; // dir index 0
    *q++  = 0x00; // mtime
    *q++  = 0x00; // size
    *q++  = 0x00; // file_names terminator
    put_u32(hdr_len_field, (u32)(q - after_hdr_len));

    q    = emit_set_address8(q, 0x3300);
    *q++ = DWLNS_COPY;
    *q++ = DWLNS_ADVANCE_PC;
    *q++ = 8;
    q    = emit_end_sequence(q);
    put_u32(unit_len_field, (u32)(q - (unit_len_field + 4)));
    u64 dl_len = (u64)(q - dl);

    Elf        elf;
    DwarfLines lines;
    bool       built = build_from_dl(&lines, &elf, base, dl, dl_len);
    bool       ok    = built;
    if (built) {
        const DwarfLineEntry *e = DwarfLinesResolve(&lines, 0x3300);
        // file present, but dir index 0 -> dir must be NULL, not pool+42.
        ok = ok && e && e->file && ZstrFindSubstring(e->file, "only.c") != NULL;
        ok = ok && e && e->dir == NULL;
        DwarfLinesDeinit(&lines);
        ElfDeinit(&elf);
    }
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Locate the (only) real row and check its address, used by the saturation
// tests. Returns u64max-or-mismatch-safe: scans for the first non-end_sequence
// row and reports its address via *out, returns whether one was found.
static bool first_real_row_address(const DwarfLines *lines, u64 *out) {
    for (u64 i = 0; i < VecLen(&lines->entries); ++i) {
        const DwarfLineEntry *e = &VecAt(&lines->entries, i);
        if (!e->end_sequence) {
            *out = e->address;
            return true;
        }
    }
    return false;
}

// 420:36  DW_LNS_ADVANCE_PC overflow saturation `st.address = (u64)-1` -> `= 42`.
// Set address near u64 max, then ADVANCE_PC by an amount that overflows; the
// emitted row's address must saturate to u64max, not 42.
bool test_dwm_advance_pc_saturates_on_overflow(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    u8          dl[512];
    LineFixture fx;
    u8         *p = write_line_header(dl, &fx);
    p             = emit_set_address8(p, 0xfffffffffffffff0ull);
    *p++          = DWLNS_ADVANCE_PC;
    *p++          = 0x40;       // +64 -> 0xff..f0 + 64 overflows u64
    *p++          = DWLNS_COPY; // emit row at the saturated address
    p             = emit_end_sequence(p);
    u64 dl_len    = finalize_unit(&fx, dl, p);

    Elf        elf;
    DwarfLines lines;
    bool       built = build_from_dl(&lines, &elf, base, dl, dl_len);
    bool       ok    = built;
    if (built) {
        u64 addr = 0;
        ok       = ok && first_real_row_address(&lines, &addr) && addr == (u64)-1;
        DwarfLinesDeinit(&lines);
        ElfDeinit(&elf);
    }
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// 475:36  DW_LNS_FIXED_ADVANCE_PC overflow saturation `st.address = (u64)-1` -> `= 42`.
bool test_dwm_fixed_advance_pc_saturates_on_overflow(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    u8          dl[512];
    LineFixture fx;
    u8         *p = write_line_header(dl, &fx);
    p             = emit_set_address8(p, 0xffffffffffffffffull);
    *p++          = DWLNS_FIXED_ADVANCE_PC;
    put_u16(p, 0x0010);       // +16 -> overflow
    p          += 2;
    *p++        = DWLNS_COPY; // emit row at saturated address
    p           = emit_end_sequence(p);
    u64 dl_len  = finalize_unit(&fx, dl, p);

    Elf        elf;
    DwarfLines lines;
    bool       built = build_from_dl(&lines, &elf, base, dl, dl_len);
    bool       ok    = built;
    if (built) {
        u64 addr = 0;
        ok       = ok && first_real_row_address(&lines, &addr) && addr == (u64)-1;
        DwarfLinesDeinit(&lines);
        ElfDeinit(&elf);
    }
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// 463:36  DW_LNS_CONST_ADD_PC overflow saturation `st.address = (u64)-1` -> `= 42`.
// const_add_pc advances by op_adv = (255-13)/14 = 17 (with min_instr_len 1).
// Set address near u64 max so the +17 overflows.
bool test_dwm_const_add_pc_saturates_on_overflow(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    u8          dl[512];
    LineFixture fx;
    u8         *p = write_line_header(dl, &fx);
    p             = emit_set_address8(p, 0xfffffffffffffff8ull); // +17 overflows
    *p++          = DWLNS_CONST_ADD_PC;
    *p++          = DWLNS_COPY;                                  // emit row at saturated address
    p             = emit_end_sequence(p);
    u64 dl_len    = finalize_unit(&fx, dl, p);

    Elf        elf;
    DwarfLines lines;
    bool       built = build_from_dl(&lines, &elf, base, dl, dl_len);
    bool       ok    = built;
    if (built) {
        u64 addr = 0;
        ok       = ok && first_real_row_address(&lines, &addr) && addr == (u64)-1;
        DwarfLinesDeinit(&lines);
        ElfDeinit(&elf);
    }
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// 520:28  special-opcode overflow saturation `st.address = (u64)-1` -> `= 42`.
// Special opcode 0xff: adjusted=242, op_adv=17 (min_instr_len 1). Set address
// near u64 max so the +17 overflows; the special opcode emits its row.
bool test_dwm_special_opcode_saturates_on_overflow(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    u8          dl[512];
    LineFixture fx;
    u8         *p = write_line_header(dl, &fx);
    p             = emit_set_address8(p, 0xfffffffffffffff8ull); // +17 overflows
    *p++          = 0xff;                                        // special opcode (>= opcode_base) emits a row
    p             = emit_end_sequence(p);
    u64 dl_len    = finalize_unit(&fx, dl, p);

    Elf        elf;
    DwarfLines lines;
    bool       built = build_from_dl(&lines, &elf, base, dl, dl_len);
    bool       ok    = built;
    if (built) {
        u64 addr = 0;
        ok       = ok && first_real_row_address(&lines, &addr) && addr == (u64)-1;
        DwarfLinesDeinit(&lines);
        ElfDeinit(&elf);
    }
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// 568:16  `ok = false` in the BufReadU32LE-failure (short trailing bytes) branch
//         -> `ok = 42`.
// A valid CU followed by 2 trailing bytes: the section still has >0 bytes after
// the CU, but a 4-byte unit_length read fails. Real code sets ok=false and the
// whole build returns false. `ok = 42` (truthy) makes it return true instead.
bool test_dwm_trailing_short_bytes_fail_build(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    u8          dl[512];
    LineFixture fx;
    u8         *p = write_line_header(dl, &fx);
    p             = emit_set_address8(p, 0x5000);
    *p++          = DWLNS_COPY;
    *p++          = DWLNS_ADVANCE_PC;
    *p++          = 8;
    p             = emit_end_sequence(p);
    u64 dl_len    = finalize_unit(&fx, dl, p);
    // Append 2 trailing garbage bytes after the complete CU.
    dl[dl_len++] = 0xaa;
    dl[dl_len++] = 0xbb;

    Elf        elf;
    DwarfLines lines;
    bool       built = build_from_dl(&lines, &elf, base, dl, dl_len);
    // Real code rejects the truncated trailing unit (returns false).
    bool ok = !built;
    if (built) {
        DwarfLinesDeinit(&lines);
        ElfDeinit(&elf);
    } else {
        ElfDeinit(&elf);
    }
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// 610:16  `ok = false` on the collect_cu_strings-failure branch -> `ok = 42`.
// An include_directories table whose first string runs to the end of the CU
// without a NUL makes BufReadZstr fail inside collect_cu_strings. Real code sets
// ok=false and the build returns false. `ok = 42` makes it return true.
bool test_dwm_unterminated_dir_string_fails_build(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    // Hand-rolled header with an unterminated include_directories entry.
    u8  dl[256];
    u8 *q               = dl;
    u8 *unit_len_field  = q;
    q                  += 4;
    put_u16(q, 4);
    q                 += 2;
    u8 *hdr_len_field  = q;
    q                 += 4;
    u8 *after_hdr_len  = q;
    *q++               = HDR_MIN_INSTR_LEN;
    *q++               = 1;
    *q++               = 1;
    *q++               = HDR_LINE_BASE;
    *q++               = HDR_LINE_RANGE;
    *q++               = HDR_OPCODE_BASE;
    for (u32 i = 0; i < 12; ++i)
        *q++ = kStdOpcodeLengths[i];
    put_u32(hdr_len_field, (u32)(q - after_hdr_len));
    // include_directories: a string with NO terminating NUL, and the CU ends
    // immediately after it -> BufReadZstr can never find a NUL.
    const char d1[] = "unterminated_directory_entry_runs_off_the_end";
    MemCopy(q, d1, sizeof(d1) - 1); // copy WITHOUT the trailing NUL
    q += sizeof(d1) - 1;
    put_u32(unit_len_field, (u32)(q - (unit_len_field + 4)));
    u64 dl_len = (u64)(q - dl);

    Elf        elf;
    DwarfLines lines;
    bool       built = build_from_dl(&lines, &elf, base, dl, dl_len);
    bool       ok    = !built; // malformed table -> build must fail
    if (built) {
        DwarfLinesDeinit(&lines);
        ElfDeinit(&elf);
    } else {
        ElfDeinit(&elf);
    }
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ===========================================================================
// LEAK KILLS (cxx_remove_void_call on reachable error-path cleanup).
// ===========================================================================

// Build a CU whose program is truncated so run_line_program fails: a
// DW_LNS_ADVANCE_PC whose multi-byte ULEB operand runs off the end of the unit.
static u64 build_truncated_program_cu(u8 *dl) {
    LineFixture fx;
    u8         *p = write_line_header(dl, &fx);
    p             = emit_set_address8(p, 0x6000);
    *p++          = DWLNS_ADVANCE_PC;
    // ULEB128 with continuation bits set, but the unit ends here so the
    // operand is truncated -> BufReadULeb128 fails -> run_line_program fails.
    *p++ = 0x80;
    *p++ = 0x80;
    *p++ = 0x80;
    return finalize_unit(&fx, dl, p);
}

// 628:13  `cu_strings_deinit(&cs)` on the run_line_program-failure path is
//          dropped. The three CuStrings vectors leak. A truncated program
//          reaches this branch; the leak shows up as LiveCount > 0.
bool test_dwm_run_line_failure_frees_cu_strings(void) {
    DebugAllocator dbg  = leak_alloc();
    Allocator     *base = ALLOCATOR_OF(&dbg);

    u8  dl[512];
    u64 dl_len = build_truncated_program_cu(dl);

    Elf        elf;
    DwarfLines lines;
    bool       built = build_from_dl(&lines, &elf, base, dl, dl_len);
    if (built)
        DwarfLinesDeinit(&lines); // defensive: should not happen
    ElfDeinit(&elf);

    // Real code freed the CuStrings on the failure branch -> allocator drained.
    bool ok = !built && (DebugAllocatorLiveCount(&dbg) == 0);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// 652:9  `DwarfLinesDeinit(out)` on the `if (!ok)` build-failure teardown is
//         dropped. out->entries + string_pool leak. The same truncated-program
//         CU drives the build to failure; the dropped teardown leaks.
bool test_dwm_build_failure_frees_lines(void) {
    DebugAllocator dbg  = leak_alloc();
    Allocator     *base = ALLOCATOR_OF(&dbg);

    // Use a program that emits at least one row (so out->entries holds heap
    // storage) and THEN truncates, so the failure teardown has something to
    // free.
    u8          dl[512];
    LineFixture fx;
    u8         *p = write_line_header(dl, &fx);
    p             = emit_set_address8(p, 0x7000);
    *p++          = DWLNS_COPY; // emits a row -> entries grows
    *p++          = DWLNS_ADVANCE_PC;
    *p++          = 0x80;       // truncated ULEB -> run_line_program fails after a row exists
    *p++          = 0x80;
    *p++          = 0x80;
    u64 dl_len    = finalize_unit(&fx, dl, p);

    Elf        elf;
    DwarfLines lines;
    bool       built = build_from_dl(&lines, &elf, base, dl, dl_len);
    if (built)
        DwarfLinesDeinit(&lines);
    ElfDeinit(&elf);

    bool ok = !built && (DebugAllocatorLiveCount(&dbg) == 0);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// 609:13  `cu_strings_deinit(&cs)` on the collect_cu_strings-failure path is
//          dropped. Reach it with an unterminated include_directories string;
//          the CuStrings vectors leak.
bool test_dwm_collect_failure_frees_cu_strings(void) {
    DebugAllocator dbg  = leak_alloc();
    Allocator     *base = ALLOCATOR_OF(&dbg);

    u8  dl[256];
    u8 *q               = dl;
    u8 *unit_len_field  = q;
    q                  += 4;
    put_u16(q, 4);
    q                 += 2;
    u8 *hdr_len_field  = q;
    q                 += 4;
    u8 *after_hdr_len  = q;
    *q++               = HDR_MIN_INSTR_LEN;
    *q++               = 1;
    *q++               = 1;
    *q++               = HDR_LINE_BASE;
    *q++               = HDR_LINE_RANGE;
    *q++               = HDR_OPCODE_BASE;
    for (u32 i = 0; i < 12; ++i)
        *q++ = kStdOpcodeLengths[i];
    put_u32(hdr_len_field, (u32)(q - after_hdr_len));
    // One valid directory (so a pool offset + a dir_offsets entry are pushed
    // through the allocator), then a SECOND directory string that runs off the
    // end with no NUL -> BufReadZstr fails on the second iteration, after the
    // first iteration has already allocated CuStrings storage.
    const char d1[] = "good_dir";
    MemCopy(q, d1, sizeof(d1));
    q               += sizeof(d1);
    const char d2[]  = "runaway_directory_with_no_terminator_byte_present";
    MemCopy(q, d2, sizeof(d2) - 1); // no trailing NUL
    q += sizeof(d2) - 1;
    put_u32(unit_len_field, (u32)(q - (unit_len_field + 4)));
    u64 dl_len = (u64)(q - dl);

    Elf        elf;
    DwarfLines lines;
    bool       built = build_from_dl(&lines, &elf, base, dl, dl_len);
    if (built)
        DwarfLinesDeinit(&lines);
    ElfDeinit(&elf);

    bool ok = !built && (DebugAllocatorLiveCount(&dbg) == 0);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

int main(void) {
    TestFunction tests[] = {
        test_dwm_file_off_init_zero_when_file_oob,
        test_dwm_dir_off_init_zero_when_dir_zero,
        test_dwm_advance_pc_saturates_on_overflow,
        test_dwm_fixed_advance_pc_saturates_on_overflow,
        test_dwm_const_add_pc_saturates_on_overflow,
        test_dwm_special_opcode_saturates_on_overflow,
        test_dwm_trailing_short_bytes_fail_build,
        test_dwm_unterminated_dir_string_fails_build,
        test_dwm_run_line_failure_frees_cu_strings,
        test_dwm_build_failure_frees_lines,
        test_dwm_collect_failure_frees_cu_strings,
    };
    TestFunction deadend_tests[] = {0};
    (void)deadend_tests;
    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), deadend_tests, 0, "Dwarf.Mut");
}
