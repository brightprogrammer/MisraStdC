/// file      : Elf.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// ELF (Executable and Linkable Format) parser. Reads the ELF header,
/// section headers, and symbol tables of a 64-bit ELF object directly
/// out of bytes — no `libelf`, no `<elf.h>`, no `libdl`. The intended
/// consumers are:
///
///   - Our own `dladdr` replacement that wants to resolve static
///     symbols (the libc `dladdr` only walks `.dynsym`).
///   - A future DWARF parser that needs `.debug_*` section locations.
///
/// v1 supports ELF64 little-endian only. ELF32 / big-endian come later
/// — flagged in FUTURE-PLANS.md.

#ifndef MISRA_PARSERS_ELF_H
#define MISRA_PARSERS_ELF_H

#include <Misra/Std/Allocator.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Container/Vec.h>
#include <Misra/Types.h>

// ---------------------------------------------------------------------------
// Spec constants we need at the surface. (Just the few we expose to
// callers — the long lists of machine types, section types, etc., live
// in the .c file.)
// ---------------------------------------------------------------------------

typedef enum ElfClass {
    ELF_CLASS_INVALID = 0,
    ELF_CLASS_32      = 1,
    ELF_CLASS_64      = 2,
} ElfClass;

typedef enum ElfData {
    ELF_DATA_INVALID = 0,
    ELF_DATA_LSB     = 1, // little-endian
    ELF_DATA_MSB     = 2, // big-endian
} ElfData;

typedef enum ElfType {
    ELF_TYPE_NONE = 0,
    ELF_TYPE_REL  = 1, // relocatable object
    ELF_TYPE_EXEC = 2, // executable
    ELF_TYPE_DYN  = 3, // shared object (also PIE)
    ELF_TYPE_CORE = 4, // core dump
} ElfType;

typedef enum ElfSectionType {
    ELF_SECTION_TYPE_NULL     = 0,
    ELF_SECTION_TYPE_PROGBITS = 1,
    ELF_SECTION_TYPE_SYMTAB   = 2,
    ELF_SECTION_TYPE_STRTAB   = 3,
    ELF_SECTION_TYPE_RELA     = 4,
    ELF_SECTION_TYPE_HASH     = 5,
    ELF_SECTION_TYPE_DYNAMIC  = 6,
    ELF_SECTION_TYPE_NOTE     = 7,
    ELF_SECTION_TYPE_NOBITS   = 8,
    ELF_SECTION_TYPE_REL      = 9,
    ELF_SECTION_TYPE_DYNSYM   = 11,
} ElfSectionType;

typedef enum ElfSymbolBind {
    ELF_SYMBOL_BIND_LOCAL  = 0,
    ELF_SYMBOL_BIND_GLOBAL = 1,
    ELF_SYMBOL_BIND_WEAK   = 2,
} ElfSymbolBind;

typedef enum ElfSymbolType {
    ELF_SYMBOL_TYPE_NOTYPE  = 0,
    ELF_SYMBOL_TYPE_OBJECT  = 1,
    ELF_SYMBOL_TYPE_FUNC    = 2,
    ELF_SYMBOL_TYPE_SECTION = 3,
    ELF_SYMBOL_TYPE_FILE    = 4,
    ELF_SYMBOL_TYPE_COMMON  = 5,
    ELF_SYMBOL_TYPE_TLS     = 6,
} ElfSymbolType;

// ---------------------------------------------------------------------------
// Parsed records. Strings point into the file's loaded byte buffer —
// they are valid as long as the `ElfFile` is alive.
// ---------------------------------------------------------------------------

///
/// Decoded ELF file header (just the fields users care about).
///
typedef struct ElfHeader {
    ElfClass class;
    ElfData data;
    ElfType type;
    u16     machine;
    u64     entry;
    u64     phoff;
    u64     shoff;
    u16     phnum;
    u16     shnum;
    u16     shstrndx;
} ElfHeader;

///
/// Decoded section header. `name` is borrowed from the file's
/// `.shstrtab` and stays valid until `ElfFileDeinit`.
///
typedef struct ElfSection {
    const char *name;
    u32         type;       // ElfSectionType plus arch-specific extensions
    u64         flags;
    u64         addr;       // runtime virtual address, if SHF_ALLOC
    u64         offset;     // file offset of section data
    u64         size;       // bytes
    u32         link;       // section-specific cross-reference
    u32         info;       // section-specific
    u64         entry_size; // for table-shaped sections
} ElfSection;

///
/// Decoded symbol-table entry. `name` is borrowed from the file's
/// `.strtab` (for `symbols`) or `.dynstr` (for `dynamic_symbols`).
///
typedef struct ElfSymbol {
    const char   *name;
    ElfSymbolBind bind;
    ElfSymbolType type;
    u16           section_index;
    u64           value; // virtual address for ET_EXEC / ET_DYN
    u64           size;
} ElfSymbol;

typedef Vec(ElfSection) ElfSections;
typedef Vec(ElfSymbol) ElfSymbols;

///
/// Parsed ELF file. Holds the raw bytes plus decoded indices into them.
/// Two construction paths:
///
///   `ElfFileOpen`           — reads a file from disk; the `ElfFile`
///                             owns the byte buffer and frees it on
///                             `ElfFileDeinit`.
///   `ElfFileOpenFromMemory` — borrows a caller-supplied buffer; the
///                             caller is responsible for keeping the
///                             buffer alive until `ElfFileDeinit`.
///
/// FIELDS:
/// - allocator       : Allocator used for the owned buffer (if any) and
///                     for the section / symbol vectors.
/// - data            : Pointer to the raw file bytes.
/// - data_size       : Length of `data` in bytes.
/// - owns_data       : True if `data` was allocated via `allocator`.
/// - header          : Decoded ELF header.
/// - sections        : All section headers, in original order.
/// - symbols         : Entries from `.symtab` (may be empty if stripped).
/// - dynamic_symbols : Entries from `.dynsym` (always present for
///                     dynamic objects).
///
typedef struct ElfFile {
    Allocator  *allocator;
    u8         *data;
    size        data_size;
    bool        owns_data;
    ElfHeader   header;
    ElfSections sections;
    ElfSymbols  symbols;
    ElfSymbols  dynamic_symbols;
} ElfFile;

///
/// Open and parse an ELF file from disk.
///
/// out[out]   : Populated on success.
/// path[in]   : Filesystem path.
/// alloc[in]  : Allocator for the read-in byte buffer and the section /
///              symbol vectors. Must outlive the `ElfFile`.
///
/// SUCCESS : Returns true; `out` is fully populated and `out->owns_data`
///           is true.
/// FAILURE : Returns false; logs the failing step (open / read / magic /
///           class / decoding). `out` is left zeroed.
///
/// TAGS: Parser, ELF, File
///
bool ElfFileOpen(ElfFile *out, const char *path, Allocator *alloc);

///
/// Parse an ELF object from an in-memory byte range. The `data` buffer
/// is borrowed — the caller must keep it alive for the lifetime of the
/// returned `ElfFile`.
///
/// out[out]      : Populated on success.
/// data[in]      : Raw ELF bytes. Borrowed; not copied.
/// data_size[in] : Length of `data` in bytes.
/// alloc[in]     : Allocator for the section / symbol vectors.
///
/// SUCCESS : Returns true; `out->owns_data` is false.
/// FAILURE : Returns false; logs the failing step. `out` is left zeroed.
///
/// TAGS: Parser, ELF, Memory
///
bool ElfFileOpenFromMemory(ElfFile *out, u8 *data, size data_size, Allocator *alloc);

///
/// Release storage owned by an `ElfFile`. Frees the byte buffer if
/// `owns_data` was true. Safe to call on a zeroed struct.
///
void ElfFileDeinit(ElfFile *self);

///
/// Look up the symbol whose `[value, value+size)` range contains
/// `vaddr`. Searches `symbols` first, then `dynamic_symbols` — so
/// static (non-exported) functions resolve when `.symtab` is present.
///
/// `vaddr` is the file-relative virtual address — i.e. for a runtime
/// pointer in a `ET_DYN` (PIE / shared object), the caller subtracts
/// the load base first. For `ET_EXEC` it's the address as-is.
///
/// self[in]   : Parsed ELF file.
/// vaddr[in]  : Virtual address to resolve.
///
/// SUCCESS : Returns a pointer to the matching `ElfSymbol`. The pointer
///           is valid until `ElfFileDeinit`.
/// FAILURE : Returns NULL if no symbol covers `vaddr`.
///
/// TAGS: Parser, ELF, Symbol
///
const ElfSymbol *ElfFileResolveAddress(const ElfFile *self, u64 vaddr);

///
/// Find a section by name (first match). Returns NULL if absent.
///
const ElfSection *ElfFileFindSection(const ElfFile *self, const char *name);

#endif // MISRA_PARSERS_ELF_H
