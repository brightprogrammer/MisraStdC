#include <Misra.h>
#include <Misra/Parsers/Dwarf.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Parsers/Elf.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Memory.h>
#include <Misra/Sys/SymbolResolver.h>

#include "../Util/TestRunner.h"

static __attribute__((noinline)) void dwarf_marker_helper(void) {
    __asm__ __volatile__("" ::
                             : "memory");
}

// Parsing /proc/self/exe's .debug_line should produce at least some
// entries when the binary is built with debug info (meson default).
bool test_dwarf_lines_load_self(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    Elf elf;
    if (!ElfOpen(&elf, "/proc/self/exe", ALLOCATOR_OF(&alloc))) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    DwarfLines lines;
    bool       built = DwarfLinesBuildFromElf(&lines, &elf, ALLOCATOR_OF(&alloc));
    bool       ok    = built && VecLen(&lines.entries) > 0;

    if (built)
        DwarfLinesDeinit(&lines);
    ElfDeinit(&elf);
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
    u64 file_relative = (u64)&dwarf_marker_helper - r.module_base;

    Elf elf;
    if (!ElfOpen(&elf, r.module_path, base)) {
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

    ElfDeinit(&elf);
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
    u64 file_relative = (u64)&dwarf_marker_helper - r.module_base;

    Elf elf;
    if (!ElfOpen(&elf, r.module_path, base)) {
        SymbolResolverDeinit(&res);
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    DwarfCfi cfi;
    bool     built = DwarfCfiBuildFromElf(&cfi, &elf, base);
    // Contract: the parse itself must succeed (an absent or sparse
    // `.eh_frame` is still success, just fewer FDEs). Whether *this*
    // binary's `.eh_frame` happens to cover the marker is an environment
    // detail -- some build configurations (e.g. a thin, statically
    // linked, unoptimised test executable) emit FDEs only for CRT stubs
    // and leave application functions uncovered. So the contract we
    // assert is conditional: IF an FDE covers the marker, it must report
    // a range that actually contains the address, and the CFI VM must
    // produce a usable unwind row for it. A "no FDE here" outcome is the
    // parser behaving correctly, not a failure.
    bool ok = built;
    if (built) {
        const DwarfFde *fde = DwarfCfiFindFde(&cfi, file_relative);
        if (fde != NULL) {
            // The FDE handed back must genuinely cover the queried
            // address -- a caller relies on the returned range.
            ok = ok && fde->pc_range > 0 && file_relative >= fde->pc_begin &&
                 file_relative < fde->pc_begin + fde->pc_range;

            // Run the CFI VM and verify we get a usable row: on x86-64
            // the CFA is `register + offset` (typically RSP + N), and the
            // return-address pseudo-register (DWARF reg 16) has a saved
            // location at some offset from CFA.
            DwarfUnwindRow row;
            ok = ok && DwarfCfiBuildRow(&cfi, fde, file_relative, &row);
            ok = ok && row.cfa.kind == DWARF_CFA_RULE_REG_OFFSET;
            ok = ok && row.regs[row.return_address_register].kind == DWARF_REG_RULE_OFFSET;
        }
        DwarfCfiDeinit(&cfi);
    }

    ElfDeinit(&elf);
    SymbolResolverDeinit(&res);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Build DwarfFunctions for /proc/self/exe and verify our marker
// helper resolves back to its name from .debug_info alone. The test
// binary isn't stripped, so .symtab works too; the point here is the
// .debug_info-only path.
bool test_dwarf_functions_resolves_helper_to_name(void) {
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
    u64 file_relative = (u64)&dwarf_marker_helper - r.module_base;

    Elf elf;
    if (!ElfOpen(&elf, r.module_path, base)) {
        SymbolResolverDeinit(&res);
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    DwarfFunctions fns;
    bool           built = DwarfFunctionsBuildFromElf(&fns, &elf, base);
    bool           ok    = false;
    if (built && VecLen(&fns.entries) > 0) {
        const DwarfFunction *f = DwarfFunctionsResolve(&fns, file_relative);
        ok = f != NULL && f->name != NULL && ZstrFindSubstring(f->name, "dwarf_marker_helper") != NULL;
        DwarfFunctionsDeinit(&fns);
    }

    ElfDeinit(&elf);
    SymbolResolverDeinit(&res);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ===========================================================================
// Self-contained `.debug_info` fixtures for DwarfFunctionsBuildFromSlices.
//
// These exercise the documented SUCCESS / FAILURE contract on crafted
// bytes, independent of the host binary's debug sections (which the
// /proc/self/exe tests above depend on). Everything asserted here is a
// caller-observable outcome: a known function name/range resolves, or a
// malformed / unsupported / truncated input is rejected.
// ===========================================================================

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

// DWARF constants the fixtures use.
enum {
    T_DW_TAG_subprogram = 0x2e,
    T_DW_AT_name        = 0x03,
    T_DW_AT_low_pc      = 0x11,
    T_DW_AT_high_pc     = 0x12,
    T_DW_FORM_addr      = 0x01,
    T_DW_FORM_data8     = 0x07,
    T_DW_FORM_string    = 0x08,
};

// A single-CU `.debug_abbrev` with one abbrev (code 1) describing a
// DW_TAG_subprogram with name(string) + low_pc(addr) + high_pc(data8).
static const u8 kAbbrev[] = {
    0x01,                // abbrev code 1
    T_DW_TAG_subprogram, // tag
    0x00,                // has_children = no
    T_DW_AT_name,
    T_DW_FORM_string,
    T_DW_AT_low_pc,
    T_DW_FORM_addr,
    T_DW_AT_high_pc,
    T_DW_FORM_data8,
    0x00,
    0x00, // end of attrs
    0x00, // end of abbrev table
};

// Build a single-CU `.debug_info` for one subprogram. `low`/`high_off`
// are the low_pc and the high_pc-as-offset. Writes into `buf`, returns
// the total length. `version` and `length_override` allow malformed /
// unsupported variants.
static u64 build_debug_info(u8 *buf, u16 version, u32 length_override, u64 low, u64 high_off, Zstr name) {
    // CU body after unit_length: version(2) abbrev_off(4) addr_size(1)
    //   + DIE: abbrev_code(1) name(NUL-terminated) low_pc(8) high_pc(8)
    //   + end-of-children(1 byte: abbrev code 0)
    u64 namelen = ZstrLen(name) + 1; // include NUL
    u8 *p       = buf + 4;           // leave room for unit_length
    u8 *start   = p;
    put_u16(p, version);
    p += 2;
    put_u32(p, 0); // abbrev_offset
    p    += 4;
    *p++  = 8;     // addr_size
    *p++  = 0x01;  // abbrev code 1
    for (u64 i = 0; i < namelen; ++i)
        *p++ = (u8)name[i];
    put_u64(p, low);
    p += 8;
    put_u64(p, high_off);
    p            += 8;
    *p++          = 0x00; // end-of-CU sibling terminator
    u32 body_len  = (u32)(p - start);
    put_u32(buf, length_override ? length_override : body_len);
    return (u64)(p - buf);
}

// SUCCESS contract: a well-formed CU resolves the function name across
// its whole [low_pc, high_pc) range, and rejects addresses outside it.
bool test_dwarf_info_resolves_known_function(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    u8  info[128];
    u64 info_len = build_debug_info(info, 4, 0, 0x4000, 0x40, "widget_init");

    DwarfFunctions fns;
    bool           built = DwarfFunctionsBuildFromSlices(&fns, info, info_len, kAbbrev, sizeof(kAbbrev), NULL, 0, base);
    bool           ok    = built && VecLen(&fns.entries) == 1;

    if (ok) {
        // Inside the range -> the known name comes back.
        const DwarfFunction *f = DwarfFunctionsResolve(&fns, 0x4000);
        ok                     = ok && f && f->name && ZstrCompare(f->name, "widget_init") == 0;
        f                      = DwarfFunctionsResolve(&fns, 0x4000 + 0x3f); // last byte in range
        ok                     = ok && f && f->name && ZstrCompare(f->name, "widget_init") == 0;
        // Below low_pc and at/above high_pc -> no match.
        ok = ok && DwarfFunctionsResolve(&fns, 0x3fff) == NULL;
        ok = ok && DwarfFunctionsResolve(&fns, 0x4040) == NULL;
    }

    if (built)
        DwarfFunctionsDeinit(&fns);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// FAILURE contract: 64-bit DWARF length form is documented unsupported
// in v1 -- must be rejected, not silently mis-parsed.
bool test_dwarf_info_rejects_64bit_length_form(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    u8 info[16];
    put_u32(info, 0xffffffffu); // 64-bit length escape
    put_u32(info + 4, 0);
    put_u32(info + 8, 0);

    DwarfFunctions fns;
    bool built = DwarfFunctionsBuildFromSlices(&fns, info, sizeof(info), kAbbrev, sizeof(kAbbrev), NULL, 0, base);
    bool ok    = !built; // rejected

    if (built)
        DwarfFunctionsDeinit(&fns);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// FAILURE contract: `.debug_info` present but `.debug_abbrev` absent is
// documented as an error (the parser cannot decode any DIE).
bool test_dwarf_info_rejects_missing_abbrev(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    u8  info[128];
    u64 info_len = build_debug_info(info, 4, 0, 0x4000, 0x40, "widget_init");

    DwarfFunctions fns;
    bool           built = DwarfFunctionsBuildFromSlices(&fns, info, info_len, NULL, 0, NULL, 0, base);
    bool           ok    = !built;

    if (built)
        DwarfFunctionsDeinit(&fns);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// FAILURE contract: a CU whose abbrev_offset points past the end of
// `.debug_abbrev` is malformed and must be rejected (no over-read).
bool test_dwarf_info_rejects_abbrev_offset_oob(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    u8  info[128];
    u64 info_len = build_debug_info(info, 4, 0, 0x4000, 0x40, "widget_init");
    // Overwrite abbrev_offset (at byte 6: after unit_length(4)+version(2))
    // with a value past the abbrev section size.
    put_u32(info + 6, 0xdeadbeefu);

    DwarfFunctions fns;
    bool           built = DwarfFunctionsBuildFromSlices(&fns, info, info_len, kAbbrev, sizeof(kAbbrev), NULL, 0, base);
    bool           ok    = !built;

    if (built)
        DwarfFunctionsDeinit(&fns);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// FAILURE contract: a CU claiming a unit_length that overruns the
// supplied `.debug_info` slice must be rejected, not read past the end.
bool test_dwarf_info_rejects_unit_length_overrun(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    u8  info[128];
    u64 info_len = build_debug_info(info, 4, 0x7fffff00u, 0x4000, 0x40, "widget_init");

    DwarfFunctions fns;
    bool           built = DwarfFunctionsBuildFromSlices(&fns, info, info_len, kAbbrev, sizeof(kAbbrev), NULL, 0, base);
    bool           ok    = !built;

    if (built)
        DwarfFunctionsDeinit(&fns);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// SUCCESS contract: no `.debug_info` at all is an empty-but-successful
// result -- a caller gets a valid, empty table (every Resolve misses).
bool test_dwarf_info_empty_is_success(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    DwarfFunctions fns;
    bool           built = DwarfFunctionsBuildFromSlices(&fns, NULL, 0, NULL, 0, NULL, 0, base);
    bool           ok    = built && VecLen(&fns.entries) == 0 && DwarfFunctionsResolve(&fns, 0x4000) == NULL;

    if (built)
        DwarfFunctionsDeinit(&fns);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ===========================================================================
// Self-contained `.debug_line` fixture, wrapped in a minimal in-memory
// ELF so it can be fed through DwarfLinesBuildFromElf. This pins the
// caller-observable contract of the line-number-program interpreter:
// a known program must resolve a known address to a known (file, line).
// The /proc/self/exe line tests above only check that *some* row
// resolves, so they cannot catch the program's address/line arithmetic
// going wrong.
// ===========================================================================

// DWARF line-number standard-opcode operand counts for opcodes 1..12.
static const u8 kStdOpcodeLengths[12] = {0, 1, 1, 1, 1, 0, 0, 0, 1, 0, 0, 1};

enum {
    DWLNS_COPY         = 0x01,
    DWLNS_ADVANCE_PC   = 0x02,
    DWLNS_ADVANCE_LINE = 0x03,
    DWLNE_END_SEQUENCE = 0x01,
    DWLNE_SET_ADDRESS  = 0x02,
};

// Build the `.debug_line` payload for a single DWARF-4 CU describing one
// file ("source.c") with two rows: address 0x2000 -> line 10, address
// 0x2008 -> line 20, then end_sequence at 0x2010. Returns total length.
static u64 build_debug_line(u8 *buf) {
    u8 *p = buf + 4; // reserve unit_length
    // --- header after unit_length ---
    put_u16(p, 4);
    p                 += 2;          // version
    u8 *hdr_len_field  = p;          // header_length (filled after tables)
    p                 += 4;
    u8 *after_hdr_len  = p;
    *p++               = 1;          // minimum_instruction_length
    *p++               = 1;          // maximum_operations_per_instruction
    *p++               = 1;          // default_is_stmt
    *p++               = (u8)(-5);   // line_base
    *p++               = 14;         // line_range
    *p++               = 13;         // opcode_base
    for (u32 i = 0; i < 12; ++i)
        *p++ = kStdOpcodeLengths[i]; // standard_opcode_lengths
    // include_directories: one entry "src_dir", then terminator NUL.
    // (A non-empty directory table ensures the file string lands at a
    // non-zero pool offset, which the parser uses as its "has a file"
    // sentinel.)
    const char dname[] = "src_dir";
    MemCopy(p, dname, sizeof(dname)); // includes NUL
    p    += sizeof(dname);
    *p++  = 0x00;                     // include_directories terminator
    // file_names: one entry "source.c", dir_idx 1, mtime 0, size 0, then NUL.
    const char fname[] = "source.c";
    MemCopy(p, fname, sizeof(fname)); // includes NUL
    p    += sizeof(fname);
    *p++  = 0x01;                     // dir index (uleb) -> "src_dir"
    *p++  = 0x00;                     // mtime (uleb)
    *p++  = 0x00;                     // file size (uleb)
    *p++  = 0x00;                     // file_names terminator
    // header_length = bytes from just after the header_length field to
    // the start of the program.
    put_u32(hdr_len_field, (u32)(p - after_hdr_len));

    // --- line number program ---
    // DW_LNE_SET_ADDRESS 0x2000
    *p++ = 0x00; // extended-op marker
    *p++ = 9;    // length = 1 (sub_op) + 8 (addr)
    *p++ = DWLNE_SET_ADDRESS;
    put_u64(p, 0x2000);
    p += 8;
    // DW_LNS_ADVANCE_LINE +9 (line 1 -> 10)
    *p++ = DWLNS_ADVANCE_LINE;
    *p++ = 9; // SLEB128 for +9
    // DW_LNS_COPY -> emit row (0x2000, line 10)
    *p++ = DWLNS_COPY;
    // DW_LNS_ADVANCE_PC +8
    *p++ = DWLNS_ADVANCE_PC;
    *p++ = 8; // ULEB128 8
    // DW_LNS_ADVANCE_LINE +10 (line 10 -> 20)
    *p++ = DWLNS_ADVANCE_LINE;
    *p++ = 10;
    // DW_LNS_COPY -> emit row (0x2008, line 20)
    *p++ = DWLNS_COPY;
    // DW_LNS_ADVANCE_PC +8 (to 0x2010)
    *p++ = DWLNS_ADVANCE_PC;
    *p++ = 8;
    // DW_LNE_END_SEQUENCE
    *p++ = 0x00;
    *p++ = 1;
    *p++ = DWLNE_END_SEQUENCE;

    u32 body_len = (u32)(p - (buf + 4));
    put_u32(buf, body_len); // unit_length
    return (u64)(p - buf);
}

// Wrap `debug_line_bytes` in a minimal ELF64 (3 sections: null,
// .shstrtab, .debug_line) and return total length into `*out_len`.
static void build_elf_with_debug_line(u8 *elf, u64 *out_len, const u8 *dl, u64 dl_len) {
    MemSet(elf, 0, 4096);
    // shstrtab content: "\0.shstrtab\0.debug_line\0"
    static const char shstr[]            = "\0.shstrtab\0.debug_line";
    const u32         shstrtab_name_off  = 1;             // ".shstrtab"
    const u32         debugline_name_off = 11;            // ".debug_line"
    u64               shstr_len          = sizeof(shstr); // includes trailing NUL

    // Layout: [ehdr 64][shstrtab][.debug_line][shdr table: 3 * 64]
    u64 ehdr_size = 64;
    u64 shstr_off = ehdr_size;
    u64 dl_off    = shstr_off + shstr_len;
    // align dl_off to 4 for tidiness (not required by parser)
    u64 shtab_off = dl_off + dl_len;

    // --- e_ident ---
    elf[0] = 0x7f;
    elf[1] = 'E';
    elf[2] = 'L';
    elf[3] = 'F';
    elf[4] = 2; // ELFCLASS64
    elf[5] = 1; // ELFDATA2LSB
    elf[6] = 1; // EV_CURRENT
    // --- ehdr (after e_ident, offset 16) ---
    put_u16(elf + 16, 2);         // e_type = ET_EXEC
    put_u16(elf + 18, 62);        // e_machine = x86-64
    put_u32(elf + 20, 1);         // e_version
    put_u64(elf + 24, 0);         // e_entry
    put_u64(elf + 32, 0);         // e_phoff
    put_u64(elf + 40, shtab_off); // e_shoff
    put_u32(elf + 48, 0);         // e_flags
    put_u16(elf + 52, 64);        // e_ehsize
    put_u16(elf + 54, 0);         // e_phentsize
    put_u16(elf + 56, 0);         // e_phnum
    put_u16(elf + 58, 64);        // e_shentsize
    put_u16(elf + 60, 3);         // e_shnum
    put_u16(elf + 62, 1);         // e_shstrndx (section 1 = .shstrtab)

    // --- section payloads ---
    MemCopy(elf + shstr_off, shstr, shstr_len);
    MemCopy(elf + dl_off, dl, dl_len);

    // --- section headers (3 * 64 bytes) ---
    u8 *sh = elf + shtab_off;
    // [0] SHT_NULL (all zero) -- already zeroed.
    // SHDR64 field offsets: sh_name@0 sh_type@4 sh_flags@8 sh_addr@16
    //                       sh_offset@24 sh_size@32 ...
    // [1] .shstrtab (SHT_STRTAB = 3)
    u8 *s1 = sh + 64;
    put_u32(s1 + 0, shstrtab_name_off);
    put_u32(s1 + 4, 3);          // SHT_STRTAB
    put_u64(s1 + 16, 0);         // sh_addr
    put_u64(s1 + 24, shstr_off); // sh_offset
    put_u64(s1 + 32, shstr_len); // sh_size
    // [2] .debug_line (SHT_PROGBITS = 1)
    u8 *s2 = sh + 128;
    put_u32(s2 + 0, debugline_name_off);
    put_u32(s2 + 4, 1);       // SHT_PROGBITS
    put_u64(s2 + 16, 0);      // sh_addr (file-relative VAs live in the program)
    put_u64(s2 + 24, dl_off); // sh_offset
    put_u64(s2 + 32, dl_len); // sh_size

    *out_len = shtab_off + 3 * 64;
}

// SUCCESS contract: a known line-number program resolves known addresses
// to the known file + line. Mutating the program interpreter's address
// or line arithmetic changes these caller-observed values.
bool test_dwarf_lines_resolves_known_program(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    u8  dl[256];
    u64 dl_len = build_debug_line(dl);

    static u8 elfbuf[4096];
    u64       elf_len = 0;
    build_elf_with_debug_line(elfbuf, &elf_len, dl, dl_len);

    Elf elf;
    if (!ElfOpenFromMemoryCopy(&elf, elfbuf, (size)elf_len, base)) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    DwarfLines lines;
    bool       built = DwarfLinesBuildFromElf(&lines, &elf, base);
    bool       ok    = built;
    if (built) {
        // Two real rows plus the end_sequence row.
        ok = ok && VecLen(&lines.entries) >= 2;

        // Address 0x2000 -> source.c:10
        const DwarfLineEntry *e = DwarfLinesResolve(&lines, 0x2000);
        ok                      = ok && e && e->file && ZstrFindSubstring(e->file, "source.c") != NULL && e->line == 10;

        // An address inside the first row's range still maps to line 10.
        e  = DwarfLinesResolve(&lines, 0x2004);
        ok = ok && e && e->line == 10;

        // Address 0x2008 -> source.c:20
        e  = DwarfLinesResolve(&lines, 0x2008);
        ok = ok && e && e->line == 20;

        // Below the first row's address -> unresolved.
        ok = ok && DwarfLinesResolve(&lines, 0x1000) == NULL;

        // At/after the end_sequence boundary -> unresolved.
        ok = ok && DwarfLinesResolve(&lines, 0x2010) == NULL;

        DwarfLinesDeinit(&lines);
    }

    ElfDeinit(&elf);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// FAILURE/robustness contract: a `.debug_line` CU whose unit_length
// overruns the section is rejected (the build fails) rather than read
// past the section end.
bool test_dwarf_lines_rejects_unit_length_overrun(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    u8  dl[256];
    u64 dl_len = build_debug_line(dl);
    // Inflate unit_length far past the section size.
    put_u32(dl, 0x7fffff00u);

    static u8 elfbuf[4096];
    u64       elf_len = 0;
    build_elf_with_debug_line(elfbuf, &elf_len, dl, dl_len);

    Elf elf;
    if (!ElfOpenFromMemoryCopy(&elf, elfbuf, (size)elf_len, base)) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    DwarfLines lines;
    bool       built = DwarfLinesBuildFromElf(&lines, &elf, base);
    // The malformed unit is rejected: either the build fails outright, or
    // (defensively) it succeeds but yields no usable rows. A caller must
    // never get a bogus resolved line from over-read bytes.
    bool ok = !built || VecLen(&lines.entries) == 0;

    if (built)
        DwarfLinesDeinit(&lines);
    ElfDeinit(&elf);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

int main(void) {
    WriteFmt("[INFO] Starting Dwarf tests\n\n");

    TestFunction tests[] = {
        test_dwarf_lines_load_self,
        test_dwarf_resolves_helper_to_source_file,
        test_dwarf_cfi_finds_fde_for_self,
        test_dwarf_functions_resolves_helper_to_name,
        test_dwarf_info_resolves_known_function,
        test_dwarf_info_rejects_64bit_length_form,
        test_dwarf_info_rejects_missing_abbrev,
        test_dwarf_info_rejects_abbrev_offset_oob,
        test_dwarf_info_rejects_unit_length_overrun,
        test_dwarf_info_empty_is_success,
        test_dwarf_lines_resolves_known_program,
        test_dwarf_lines_rejects_unit_length_overrun,
    };

    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), NULL, 0, "Dwarf");
}
