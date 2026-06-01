/// file      : parsers/elf.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// ELF (Executable and Linkable Format) parser. Reads the ELF header,
/// section headers, and symbol tables of a 64-bit ELF object directly
/// out of bytes. The intended consumers are:
///
///   - `Sys/SymbolResolver`, which needs static-symbol coverage
///     (i.e. `.symtab` entries, not just the dynamic-exports table).
///   - The DWARF parser, which needs `.debug_*` section locations.
///
/// v1 supports ELF64 little-endian only. ELF32 / big-endian come later
/// — flagged in FUTURE-PLANS.md.

#ifndef MISRA_PARSERS_ELF_H
#define MISRA_PARSERS_ELF_H

#include <Misra/Parsers/Elf/Private.h>
#include <Misra/Std/Allocator.h>
#include <Misra/Std/Container/Buf.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Container/Vec.h>
#include <Misra/Std/Zstr.h>
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
// they are valid as long as the `Elf` is alive.
// ---------------------------------------------------------------------------

///
/// Decoded ELF file header (just the fields users care about).
///
typedef struct ElfHeader {
    ElfClass elf_class;
    ElfData  data;
    ElfType  type;
    u16      machine;
    u64      entry;
    u64      phoff;
    u64      shoff;
    u16      phnum;
    u16      shnum;
    u16      shstrndx;
} ElfHeader;

///
/// Decoded section header. `name` is borrowed from the file's
/// `.shstrtab` and stays valid until `ElfDeinit`.
///
typedef struct ElfSection {
    Zstr name;
    u32  type;       // ElfSectionType plus arch-specific extensions
    u64  flags;
    u64  addr;       // runtime virtual address, if SHF_ALLOC
    u64  offset;     // file offset of section data
    u64  size;       // bytes
    u32  link;       // section-specific cross-reference
    u32  info;       // section-specific
    u64  entry_size; // for table-shaped sections
} ElfSection;

///
/// Decoded symbol-table entry. `name` is borrowed from the file's
/// `.strtab` (for `symbols`) or `.dynstr` (for `dynamic_symbols`).
///
typedef struct ElfSymbol {
    Zstr          name;
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
/// Three construction paths, all of which leave the `Elf` in the
/// same lifecycle state: parser owns the bytes, parser frees them on
/// `ElfDeinit`. There is no "borrowed buffer" mode -- the L / R
/// split below mirrors `VecInsertL` / `VecInsertR`:
///
///   `ElfOpen`               — reads a file from disk; parser owns
///                                 the resulting buffer end-to-end.
///   `ElfOpenFromMemory`     — **L**: takes ownership of the
///                                 caller's `(data, data_size)`. Caller
///                                 must not free or touch `data`
///                                 afterwards. `alloc` MUST be the
///                                 allocator that produced `data`.
///   `ElfOpenFromMemoryCopy` — **R**: allocates internally through
///                                 `alloc`, copies the caller's bytes
///                                 in, and never retains the caller's
///                                 pointer. Caller's buffer is
///                                 untouched and remains theirs.
///
/// FIELDS:
/// - data            : Raw ELF bytes as a `Buf` (owned). Carries its
///                     own length and allocator -- read via
///                     `BufLength` / `BufData` / `BufAllocator`.
/// - header          : Decoded ELF header.
/// - sections        : All section headers, in original order.
/// - symbols         : Entries from `.symtab` (may be empty if stripped).
/// - dynamic_symbols : Entries from `.dynsym` (always present for
///                     dynamic objects).
/// - build_id        : Bytes of the GNU build-ID (from
///                     `.note.gnu.build-id`); used to find a
///                     stripped binary's sidecar `.debug` file under
///                     `/usr/lib/debug/.build-id/...`. NULL when the
///                     binary has no build-ID note.
/// - build_id_size   : Length of `build_id` in bytes (typically 20).
/// - debuglink_name  : Filename portion stored in `.gnu_debuglink`,
///                     identifying a sidecar debug file. NULL when
///                     the binary lacks the section.
/// - debuglink_crc   : CRC32 of the expected sidecar contents
///                     (validated by the resolver before use).
///
typedef struct Elf {
    Buf         data;
    ElfHeader   header;
    ElfSections sections;
    ElfSymbols  symbols;
    ElfSymbols  dynamic_symbols;
    const u8   *build_id;
    u32         build_id_size;
    Zstr        debuglink_name;
    u32         debuglink_crc;
} Elf;

///
/// Borrowed handle to the parser's owned byte buffer. Cross-namespace
/// readers (`Dwarf*BuildFromElf`, ...) need section bytes off the loaded
/// file; this is the public seam they go through instead of reaching at
/// `self->data` directly.
///
/// TAGS: Parser, ELF, Accessor
///
#define ElfBuf(self) ((void)0, &(self)->data)

///
/// Open and parse an ELF file from disk.
///
/// out[out]   : Populated on success.
/// path[in]   : Filesystem path. Prefer `Str *`; `Zstr` (NUL-terminated) accepted.
/// alloc[in]  : Allocator for the read-in byte buffer and the section /
///              symbol vectors. Must outlive the `Elf`.
///
/// SUCCESS : Returns true; `out` owns the read-in buffer and will free
///           it on `ElfDeinit`.
/// FAILURE : Returns false; logs the failing step (open / read / magic /
///           class / decoding). `out` is left zeroed.
///
/// TAGS: Parser, ELF, File
///
#define ElfOpen(...) OVERLOAD(ElfOpen, __VA_ARGS__)
#define ElfOpen_2(out, path)                                                                                           \
    _Generic(                                                                                                          \
        (path),                                                                                                        \
        Str *: elf_open((out), (Zstr)StrBegin((Str *)(path)), MisraScope),                                             \
        Zstr: elf_open((out), (Zstr)(path), MisraScope),                                                               \
        char *: elf_open((out), (Zstr)(path), MisraScope)                                                              \
    )
#define ElfOpen_3(out, path, alloc)                                                                                    \
    _Generic(                                                                                                          \
        (path),                                                                                                        \
        Str *: elf_open((out), (Zstr)StrBegin((Str *)(path)), ALLOCATOR_OF(alloc)),                                    \
        Zstr: elf_open((out), (Zstr)(path), ALLOCATOR_OF(alloc)),                                                      \
        char *: elf_open((out), (Zstr)(path), ALLOCATOR_OF(alloc))                                                     \
    )

///
/// Parse an ELF object from an in-memory byte range -- **L-value /
/// ownership-transfer form** (mirrors `VecInsertL`).
///
/// Takes the caller's `Buf` by pointer. The parser snapshots the Buf
/// internally and zeroes the caller's `*in` so any post-call use sees
/// an empty Buf instead of a stale alias. The parser then owns the
/// bytes and the buffer's allocator; both are released by
/// `ElfDeinit`. The zero-on-take invariant holds on both success
/// and failure -- on parse failure the parser still consumed the Buf,
/// just frees it through the carried allocator before returning.
///
/// USAGE:
///   Buf buf = BufInit(&alloc);
///   FileRead(&f, &buf);
///   ElfOpenFromMemory(&elf, &buf);
///   // buf is now {NULL, 0, 0, NULL} -- safe to drop on stack.
///
/// out[out]    : Populated on success.
/// in[in,out]  : Pointer to the caller's `Buf`. `BufData(in)` must be
///               non-NULL and `BufAllocator(in)` must be set. After the
///               call, `*in` is zeroed (success or failure).
///
/// SUCCESS : Returns true; `out` owns the bytes; `*in` is zeroed.
/// FAILURE : Returns false; the bytes have been freed; `*in` is
///           zeroed; `out` is left zeroed.
///
/// TAGS: Parser, ELF, Memory, Ownership
///
bool ElfOpenFromMemory(Elf *out, Buf *in);

///
/// Parse an ELF object from an in-memory byte range -- **R-value /
/// copy form** (mirrors `VecInsertR`).
///
/// Parser allocates its own buffer through `alloc` and `MemCopy`s the
/// caller's bytes in. The caller's pointer is never retained: after
/// this call returns, the caller's buffer is theirs to do anything
/// with (including free, mutate, or hand to another parser).
///
/// out[out]      : Populated on success.
/// data[in]      : Raw ELF bytes. Read-only here; caller keeps them.
/// data_size[in] : Length of `data` in bytes.
/// alloc[in]     : Allocator for the internal copy and the section /
///                 symbol vectors. Must outlive the `Elf`.
///
/// SUCCESS : Returns true; `out` owns an independent copy of `data`.
/// FAILURE : Returns false; logs the failing step. `out` is left
///           zeroed and the caller's `data` is untouched.
///
/// TAGS: Parser, ELF, Memory, Copy
///
#define ElfOpenFromMemoryCopy(...)                    OVERLOAD(ElfOpenFromMemoryCopy, __VA_ARGS__)
#define ElfOpenFromMemoryCopy_3(out, data, data_size) elf_open_from_memory_copy((out), (data), (data_size), MisraScope)
#define ElfOpenFromMemoryCopy_4(out, data, data_size, alloc)                                                           \
    elf_open_from_memory_copy((out), (data), (data_size), ALLOCATOR_OF(alloc))

///
/// Release storage owned by an `Elf`. Frees the byte buffer through
/// the `data` Buf's carried allocator and tears down the section /
/// symbol vectors.
/// All three `ElfOpen*` constructors leave the parser as the
/// sole owner of `data`, so this is unconditional. Safe to call on
/// a zeroed struct.
///
/// SUCCESS : Returns to the caller. `*self` is zeroed.
/// FAILURE : Function cannot fail. NULL `self` is a no-op.
///
/// TAGS: Parser, ELF, Deinit, Lifecycle
///
void ElfDeinit(Elf *self);

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
///           is valid until `ElfDeinit`.
/// FAILURE : Returns NULL if no symbol covers `vaddr`.
///
/// TAGS: Parser, ELF, Symbol
///
const ElfSymbol *ElfResolveAddress(const Elf *self, u64 vaddr);

///
/// Find a section by name (first match) within the parsed ELF.
///
/// self[in] : Parsed Elf object.
/// name[in] : Section name to look up (case-sensitive).
///
/// SUCCESS : Returns a pointer to the first matching `ElfSection` in
///           `self->sections`. The pointer is borrowed and valid until
///           `ElfDeinit(self)`.
/// FAILURE : Returns NULL when no section name matches. `self` is
///           untouched.
///
/// TAGS: Parser, ELF, Section, Query
///
#define ElfFindSection(self, name)                                                                                     \
    _Generic((name), Str *: elf_find_section_str, Zstr: elf_find_section_zstr, char *: elf_find_section_zstr)(         \
        (self),                                                                                                        \
        (name)                                                                                                         \
    )

#endif // MISRA_PARSERS_ELF_H
