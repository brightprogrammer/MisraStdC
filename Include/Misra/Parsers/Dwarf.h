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

#endif // MISRA_PARSERS_DWARF_H
