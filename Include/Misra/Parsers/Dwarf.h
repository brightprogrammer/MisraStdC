/// file      : parsers/dwarf.h
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
    u64  address;
    Zstr file;
    Zstr dir;
    u32  line;
    u32  column;
    bool is_stmt;
    bool end_sequence;
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
///                 into a contiguous pool so the `Elf`'s lifetime
///                 doesn't have to match ours.
///
typedef struct DwarfLines {
    Allocator       *allocator;
    DwarfLineEntries entries;
    Str              string_pool;
} DwarfLines;

///
/// Parse the `.debug_line` section of a previously-opened `Elf`.
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
bool dwarf_lines_build_from_elf(DwarfLines *out, const Elf *elf, Allocator *alloc);
#define DwarfLinesBuildFromElf(...)               OVERLOAD(DwarfLinesBuildFromElf, __VA_ARGS__)
#define DwarfLinesBuildFromElf_2(out, elf)        dwarf_lines_build_from_elf((out), (elf), MisraScope)
#define DwarfLinesBuildFromElf_3(out, elf, alloc) dwarf_lines_build_from_elf((out), (elf), ALLOCATOR_OF(alloc))

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
/// SUCCESS : Returns to the caller. `*self` is zeroed.
/// FAILURE : Function cannot fail. NULL `self` is a no-op.
///
/// TAGS: Parser, DWARF, Lines, Deinit, Lifecycle
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
/// Parse the `.eh_frame` section of an already-opened Elf.
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
bool dwarf_cfi_build_from_elf(DwarfCfi *out, const Elf *elf, Allocator *alloc);
#define DwarfCfiBuildFromElf(...)               OVERLOAD(DwarfCfiBuildFromElf, __VA_ARGS__)
#define DwarfCfiBuildFromElf_2(out, elf)        dwarf_cfi_build_from_elf((out), (elf), MisraScope)
#define DwarfCfiBuildFromElf_3(out, elf, alloc) dwarf_cfi_build_from_elf((out), (elf), ALLOCATOR_OF(alloc))

///
/// Find the FDE whose `[pc_begin, pc_begin + pc_range)` range contains
/// `vaddr` (file-relative).
///
/// SUCCESS : Returns a pointer to the matching FDE, borrowed from
///           `self` (valid until `DwarfCfiDeinit`).
/// FAILURE : Returns NULL when no FDE covers `vaddr`.
///
/// TAGS: Parser, DWARF, CFI, Lookup
///
const DwarfFde *DwarfCfiFindFde(const DwarfCfi *self, u64 vaddr);

///
/// Find a CIE by its `.eh_frame` offset (used to link FDE -> CIE).
///
/// SUCCESS : Returns a pointer to the CIE at `cie_offset`, borrowed
///           from `self` (valid until `DwarfCfiDeinit`).
/// FAILURE : Returns NULL when no CIE sits at that offset.
///
/// TAGS: Parser, DWARF, CFI, Lookup
///
const DwarfCie *DwarfCfiFindCie(const DwarfCfi *self, u64 cie_offset);

///
/// Release every CIE / FDE table the CFI parser allocated. Safe on a
/// zeroed struct.
///
/// SUCCESS : Returns to the caller. `*self` is zeroed.
/// FAILURE : Function cannot fail. NULL `self` is a no-op.
///
/// TAGS: Parser, DWARF, CFI, Deinit, Lifecycle
///
void DwarfCfiDeinit(DwarfCfi *self);

// ---------------------------------------------------------------------------
// CFI bytecode interpreter — turns an FDE + target PC into an unwind row.
// ---------------------------------------------------------------------------
//
// We model only the rules needed to recover the previous frame's
// return address (and, on most ABIs, its stack pointer). For x86-64
// SysV that's the CFA rule and the rule for DWARF register 16 (the
// return-address pseudo-register).

typedef enum DwarfCfaRuleKind {
    DWARF_CFA_RULE_UNDEFINED = 0,
    DWARF_CFA_RULE_REG_OFFSET, // CFA = registers[reg] + offset
    DWARF_CFA_RULE_EXPRESSION, // CFA = result of a DWARF expression (not run in v1)
} DwarfCfaRuleKind;

typedef struct DwarfCfaRule {
    DwarfCfaRuleKind kind;
    u8               reg;    // valid when kind == REG_OFFSET
    i64              offset; // valid when kind == REG_OFFSET
} DwarfCfaRule;

typedef enum DwarfRegRuleKind {
    DWARF_REG_RULE_UNDEFINED = 0,
    DWARF_REG_RULE_SAME_VALUE,
    DWARF_REG_RULE_OFFSET,     // value at *(CFA + offset)
    DWARF_REG_RULE_VAL_OFFSET, // value = CFA + offset
    DWARF_REG_RULE_REGISTER,   // value in register `reg`
    DWARF_REG_RULE_EXPRESSION, // skipped in v1
} DwarfRegRuleKind;

typedef struct DwarfRegRule {
    DwarfRegRuleKind kind;
    i64              offset; // for OFFSET / VAL_OFFSET
    u8               reg;    // for REGISTER
} DwarfRegRule;

// Enough register slots for x86-64 (17 mainline DWARF registers + the
// return-address pseudo-register at index 16). aarch64 uses 31 GPRs
// plus a few specials; sized to 32 to cover both.
#define DWARF_UNWIND_MAX_REGS 32

typedef struct DwarfUnwindRow {
    DwarfCfaRule cfa;
    DwarfRegRule regs[DWARF_UNWIND_MAX_REGS];
    u8           return_address_register;
} DwarfUnwindRow;

///
/// Run the CIE's initial instructions plus the FDE's instructions up
/// to (but not past) `target_pc`, producing the unwind row that
/// applies at that PC.
///
/// cfi[in]       : Parsed `.eh_frame` index.
/// fde[in]       : FDE covering `target_pc` (caller obtained via
///                 `DwarfCfiFindFde`).
/// target_pc[in] : File-relative virtual address inside the FDE's
///                 [pc_begin, pc_begin+pc_range) range.
/// out[out]      : Populated on success.
///
/// SUCCESS : Returns true. `out->cfa.kind` is set to a usable rule
///           (typically REG_OFFSET on x86-64). `out->regs[ra]` tells
///           the caller where to find the previous frame's return
///           address.
/// FAILURE : Returns false on malformed CFI bytecode, unsupported
///           DW_CFA_*_expression instructions, or `target_pc`
///           outside the FDE. `out` is left zeroed.
///
/// TAGS: Parser, DWARF, CFI, Unwind
///
bool DwarfCfiBuildRow(const DwarfCfi *cfi, const DwarfFde *fde, u64 target_pc, DwarfUnwindRow *out);

// ===========================================================================
// .debug_info — function-name fallback when `.symtab` is stripped.
// ===========================================================================
//
// `.debug_info` is the full DWARF DIE (Debugging Information Entry) tree
// — types, variables, scopes, the lot. v1 of this parser narrows it
// hard: we walk only DW_TAG_subprogram DIEs and pull out (name, low_pc,
// high_pc). That's exactly the information a stripped binary lacks
// when its sidecar debug file kept `.debug_info` but not `.symtab`,
// which happens with some build pipelines (objcopy --only-keep-debug
// preserves the lot, but some homemade strip flows are less generous).

typedef struct DwarfFunction {
    u64  low_pc;  // file-relative virtual address (same space as ElfSymbol.value)
    u64  high_pc; // exclusive end
    Zstr name;    // borrowed from `string_pool`
} DwarfFunction;

typedef Vec(DwarfFunction) DwarfFunctionEntries;

///
/// Function-name index built from `.debug_info` + `.debug_abbrev` (+
/// `.debug_str` for indirect names). Entries are sorted by `low_pc`
/// to allow a binary-search lookup.
///
/// FIELDS:
/// - allocator   : Allocator backing entries + string_pool.
/// - entries     : Sorted-by-low_pc list of `DwarfFunction` rows.
/// - string_pool : Owned buffer holding function-name strings that
///                 `entries.name` borrows from. We copy because DWARF
///                 strings can come from either inline byte-strings or
///                 the separately-located `.debug_str` section.
///
typedef struct DwarfFunctions {
    Allocator           *allocator;
    DwarfFunctionEntries entries;
    Str                  string_pool;
} DwarfFunctions;

///
/// Parse `.debug_info` and build the function-name index.
///
/// out[out]   : Populated on success.
/// elf[in]    : ELF file to read from. Borrowed; not retained.
/// alloc[in]  : Allocator for the table + string pool.
///
/// SUCCESS : Returns true. `out->entries.length` is 0 if the binary
///           has no `.debug_info` (stripped + no sidecar) — still success.
/// FAILURE : Returns false on malformed DWARF or unsupported version
///           / 64-bit length form. `out` is left zeroed.
///
/// TAGS: Parser, DWARF, Info
///
bool dwarf_functions_build_from_elf(DwarfFunctions *out, const Elf *elf, Allocator *alloc);
#define DwarfFunctionsBuildFromElf(...)               OVERLOAD(DwarfFunctionsBuildFromElf, __VA_ARGS__)
#define DwarfFunctionsBuildFromElf_2(out, elf)        dwarf_functions_build_from_elf((out), (elf), MisraScope)
#define DwarfFunctionsBuildFromElf_3(out, elf, alloc) dwarf_functions_build_from_elf((out), (elf), ALLOCATOR_OF(alloc))

///
/// Container-agnostic builder. Same parser, but takes the three
/// section payloads directly. The Mach-O backtrace path uses this to
/// feed bytes from `__DWARF,__debug_*` sections inside a dSYM bundle.
///
/// Any of `info_bytes` / `abbrev_bytes` / `str_bytes` may be NULL when
/// the corresponding section is absent; an empty `info_bytes` is
/// treated as "no debug info" (success with empty table).
///
/// SUCCESS : Returns `true`. `*out` is populated; empty when there is
///           no `.debug_info` to parse.
/// FAILURE : Returns `false` on allocator OOM or malformed DWARF.
///           `*out` is left in the partial state it had reached;
///           caller should `DwarfFunctionsDeinit` to release it.
///
/// TAGS: Parser, DWARF, Info, Bytes
///
bool DwarfFunctionsBuildFromSlices(
    DwarfFunctions *out,
    const u8       *info_bytes,
    u64             info_size,
    const u8       *abbrev_bytes,
    u64             abbrev_size,
    const u8       *str_bytes,
    u64             str_size,
    Allocator      *alloc
);

///
/// Find the function whose `[low_pc, high_pc)` range contains `vaddr`
/// (file-relative).
///
/// SUCCESS : Returns a pointer to the matching entry. Valid until
///           `DwarfFunctionsDeinit`.
/// FAILURE : Returns NULL when no entry covers `vaddr`.
///
/// TAGS: Parser, DWARF, Lookup
///
const DwarfFunction *DwarfFunctionsResolve(const DwarfFunctions *self, u64 vaddr);

///
/// Release the function table built by `DwarfFunctionsFromInfo`. Frees
/// the owned function-name pool and the sorted entries vector, then
/// zeroes the struct so any later use trips the NULL-self diagnostic.
///
/// self[in,out] : DwarfFunctions instance, or NULL.
///
/// SUCCESS : Function returns. Every `DwarfFunction` previously
///           handed out by `DwarfFunctionsResolve` is invalid.
/// FAILURE : No action when `self` is NULL.
///
/// TAGS: Parser, DWARF, Cleanup
///
void DwarfFunctionsDeinit(DwarfFunctions *self);

#endif // MISRA_PARSERS_DWARF_H
