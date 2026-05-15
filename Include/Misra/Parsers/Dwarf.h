/// file      : Dwarf.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// DWARF debugging-format parser, v1. Reads the `.debug_line` section
/// of an ELF file and reconstructs the IP → `{file, line, column}`
/// mapping that compilers emit there.
///
/// DWARF is a big spec. v1 of this module is deliberately narrow:
///
///   - DWARF version 4. (Version 5 changes directory / file table
///     encoding; tracked in FUTURE-PLANS.md.)
///   - `.debug_line` only. (`.debug_info` for function names + inlined
///     frame attribution is a separate v2 effort.)
///   - 32-bit DWARF length form. (64-bit form is the
///     `0xffffffff`-prefixed variant; rare on Linux today, tracked.)
///
/// Useful for: turning the `module!symbol+offset` lines our
/// `Sys/Backtrace` already emits into `module!symbol+offset (file:line)`.

#ifndef MISRA_PARSERS_DWARF_H
#define MISRA_PARSERS_DWARF_H

#include <Misra/Parsers/Elf.h>
#include <Misra/Std/Allocator.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Container/Vec.h>
#include <Misra/Types.h>

///
/// One row of the line-number matrix. `address` is the file-relative
/// virtual address (the same address-space as `ElfSymbol.value`), so a
/// caller who already did `runtime_addr - module_base` (e.g. via
/// `SymbolResolver`) can feed the result directly to
/// `DwarfLinesResolve`.
///
/// FIELDS:
/// - address     : Code address this row applies to.
/// - file        : Source filename (borrowed from internal string pool).
/// - dir         : Compilation directory hint, may be NULL.
/// - line        : Source line number (1-based, 0 means "no line").
/// - column      : Source column (1-based, 0 means "no column").
/// - is_stmt     : True at statement boundaries — preferred breakpoints.
/// - end_sequence: True for the synthetic row that closes a contiguous
///                 range. Lookups never return such a row.
///
typedef struct DwarfLineEntry {
    u64         address;
    const char *file;
    const char *dir;
    u32         line;
    u32         column;
    bool        is_stmt;
    bool        end_sequence;
} DwarfLineEntry;

typedef Vec(DwarfLineEntry) DwarfLineEntries;

///
/// Parsed `.debug_line` content. Entries are flat and sorted-by-
/// address per the line-number program's natural emission order
/// inside each compilation unit. Lookups do a linear scan across all
/// CUs.
///
/// FIELDS:
/// - allocator   : Allocator backing entries + string_pool.
/// - entries     : All (address, file, line, ...) rows, in CU /
///                 program order.
/// - string_pool : Owned buffer holding the file / directory strings
///                 that `entries` borrows from. Required because
///                 `.debug_line` file/dir entries are interleaved
///                 byte-strings inside the section — we copy them
///                 into a contiguous pool so the `ElfFile`'s lifetime
///                 doesn't have to match ours.
///
typedef struct DwarfLines {
    Allocator       *allocator;
    DwarfLineEntries entries;
    Str              string_pool;
} DwarfLines;

///
/// Parse the `.debug_line` section of a previously-opened `ElfFile`.
///
/// out[out]   : Populated on success.
/// elf[in]    : ELF file to read from. Borrowed; not retained.
/// alloc[in]  : Allocator for the resulting tables + string pool.
///
/// SUCCESS : Returns true. `out->entries.length` may be 0 if the
///           binary has no `.debug_line` section (stripped or built
///           without -g).
/// FAILURE : Returns false; logs the failing step. `out` is left zeroed.
///
/// TAGS: Parser, DWARF, Lines
///
bool DwarfLinesBuildFromElf(DwarfLines *out, const ElfFile *elf, Allocator *alloc);

///
/// Find the entry covering `vaddr` (file-relative virtual address).
/// The match is the row with the greatest `address <= vaddr` whose
/// sequence has not ended; if `vaddr` falls past the last row of
/// every sequence, returns NULL.
///
/// SUCCESS : Returns a pointer to the matching entry inside
///           `self->entries`. Valid until `DwarfLinesDeinit`.
/// FAILURE : Returns NULL when no row covers `vaddr`.
///
/// TAGS: Parser, DWARF, Lookup
///
const DwarfLineEntry *DwarfLinesResolve(const DwarfLines *self, u64 vaddr);

///
/// Release storage owned by a `DwarfLines`. Safe on a zeroed struct.
///
void DwarfLinesDeinit(DwarfLines *self);

// ===========================================================================
// .eh_frame CFI (Call Frame Information) — used by the frame-pointer-less
// stack unwinder.
// ===========================================================================
//
// CIE = Common Information Entry. One per "augmentation profile". Holds
//       per-FDE-set settings: code/data alignment factors, return-address
//       register number, pointer-encoding scheme, default initial
//       instructions (which we replay before any FDE-specific ones).
// FDE = Frame Description Entry. One per function range. Holds the
//       address range it covers plus a stream of CFI instructions
//       describing how saved registers can be recovered at each IP.

///
/// Decoded CIE record (the parts we care about). Strings point into
/// the section bytes and are valid for the parsed `DwarfCfi`'s lifetime.
///
typedef struct DwarfCie {
    u64       offset;                  // offset of this CIE inside .eh_frame
    u8        version;
    u8        return_address_register; // DWARF register number for the return PC
    u8        fde_pointer_encoding;    // DW_EH_PE_* used for FDE pc_begin / pc_range
    u8        has_augmentation;        // bool: augmentation string starts with 'z'
    i64       code_alignment_factor;   // ULEB128
    i64       data_alignment_factor;   // SLEB128
    const u8 *initial_instructions;
    u64       initial_instructions_size;
} DwarfCie;

///
/// Decoded FDE record. `pc_begin` and `pc_range` are file-relative
/// virtual addresses (the same address-space `ElfSymbol.value` uses).
/// `instructions` is the per-FDE CFI bytecode that runs after the
/// CIE's initial instructions.
///
typedef struct DwarfFde {
    u64       offset;     // offset of this FDE inside .eh_frame
    u64       cie_offset; // offset of the CIE this FDE references
    u64       pc_begin;
    u64       pc_range;
    const u8 *instructions;
    u64       instructions_size;
} DwarfFde;

typedef Vec(DwarfCie) DwarfCies;
typedef Vec(DwarfFde) DwarfFdes;

///
/// Parsed `.eh_frame` index. CIEs are de-duplicated; FDEs each point at
/// their CIE by offset. `eh_frame_base` is the runtime address that
/// pc-relative encodings in `.eh_frame` are relative to (= the section's
/// load address); we record the file-relative form, so DwarfCfi-consuming
/// callers should pass already-file-relative PCs.
///
typedef struct DwarfCfi {
    Allocator *allocator;
    DwarfCies  cies;
    DwarfFdes  fdes;
    u64        eh_frame_addr;
} DwarfCfi;

///
/// Parse the `.eh_frame` section of an already-opened ElfFile.
///
/// out[out]   : Populated on success.
/// elf[in]    : ELF file to read from. Borrowed.
/// alloc[in]  : Allocator backing the CIE / FDE vectors.
///
/// SUCCESS : Returns true. `out->fdes.length` may be 0 if the binary
///           lacks a `.eh_frame` section (very unusual on modern Linux).
/// FAILURE : Returns false; logs the failing step. `out` is left zeroed.
///
/// TAGS: Parser, DWARF, CFI
///
bool DwarfCfiBuildFromElf(DwarfCfi *out, const ElfFile *elf, Allocator *alloc);

///
/// Find the FDE whose `[pc_begin, pc_begin + pc_range)` range contains
/// `vaddr` (file-relative). Returns NULL if no FDE covers the address.
///
const DwarfFde *DwarfCfiFindFde(const DwarfCfi *self, u64 vaddr);

///
/// Find a CIE by its `.eh_frame` offset (used to link FDE -> CIE).
///
const DwarfCie *DwarfCfiFindCie(const DwarfCfi *self, u64 cie_offset);

void DwarfCfiDeinit(DwarfCfi *self);

#endif // MISRA_PARSERS_DWARF_H
