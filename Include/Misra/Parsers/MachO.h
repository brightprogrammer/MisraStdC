/// file      : MachO.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Mach-O (Apple's executable / shared-library format) parser. The
/// macOS counterpart of `Parsers/Elf` and `Parsers/Pe`. Reads the
/// 64-bit Mach-O header plus the load commands we need for runtime
/// symbol resolution:
///
///   - `LC_SEGMENT_64` -- segment + section layout (for vmaddr/vmsize
///     and the section table, which DWARF parsers consume).
///   - `LC_SYMTAB`     -- classic symbol table (nlist_64 entries +
///     a string table). The actual function names live here.
///   - `LC_UUID`       -- 16-byte UUID used to locate the matching
///     `.dSYM` debug bundle on disk.
///
/// v1 supports 64-bit thin Mach-O (mach_header_64) only. Fat / universal
/// binaries (`CAFEBABE`) and 32-bit slices are detected and rejected;
/// the caller can pick the right slice and pass it to
/// `MachoOpenFromMemory`.

#ifndef MISRA_PARSERS_MACHO_H
#define MISRA_PARSERS_MACHO_H

#include <Misra/Std/Allocator.h>
#include <Misra/Std/Container/Buf.h>
#include <Misra/Std/Container/Vec.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Types.h>

typedef enum MachoType {
    MACHO_FILE_TYPE_NONE    = 0,
    MACHO_FILE_TYPE_OBJECT  = 0x1,
    MACHO_FILE_TYPE_EXECUTE = 0x2,
    MACHO_FILE_TYPE_DYLIB   = 0x6,
    MACHO_FILE_TYPE_BUNDLE  = 0x8,
    MACHO_FILE_TYPE_DSYM    = 0xA, // companion dSYM
} MachoType;

///
/// Decoded segment_command_64. Segments are spans of the file that
/// map to contiguous virtual-address ranges at runtime.
///
typedef struct MachoSegment {
    char name[17]; // 16 + NUL
    u64  vmaddr;
    u64  vmsize;
    u64  fileoff;
    u64  filesize;
    u32  nsects;
    u32  flags;
} MachoSegment;

///
/// Decoded section_64. Each segment contains zero or more sections;
/// DWARF lives in the `__DWARF` segment, code lives in `__TEXT`.
///
typedef struct MachoSection {
    char segment[17];
    char section[17];
    u64  addr;   // virtual address
    u64  size;
    u32  offset; // file offset
    u32  flags;
} MachoSection;

///
/// Decoded nlist_64 entry. `name` is borrowed from the file's symbol
/// string table (LC_SYMTAB.stroff).
///
typedef struct MachoSymbol {
    Zstr name;
    u64  value;         // virtual address
    u8   type;          // n_type bitfield
    u8   section_index; // n_sect, 1-based (0 = NO_SECT)
} MachoSymbol;

typedef Vec(MachoSegment) MachoSegments;
typedef Vec(MachoSection) MachoSections;
typedef Vec(MachoSymbol) MachoSymbols;

///
/// Parsed Mach-O file. Holds the raw bytes plus decoded indices. All
/// three `MachoOpen*` constructors leave the parser as the sole
/// owner of `data` -- see the `OpenFromMemory` / `OpenFromMemoryCopy`
/// docs below for the L / R semantics (mirrors `VecInsertL` /
/// `VecInsertR`).
///
/// FIELDS:
/// - allocator     : Allocator backing `data` and the
///                   segments/sections/symbols vectors.
/// - data          : Raw Mach-O bytes (owned).
/// - data_size     : Length of `data` in bytes.
/// - cputype       : Mach-O `cputype` value (e.g. 0x01000007 = x86_64,
///                   0x0100000C = arm64).
/// - filetype      : `MachoType` value.
/// - uuid          : 16-byte UUID from LC_UUID if present.
/// - has_uuid      : True iff LC_UUID was found.
/// - segments      : All LC_SEGMENT_64 entries.
/// - sections      : Flat list of all sections across segments.
/// - symbols       : Entries from LC_SYMTAB; may be empty if stripped.
///
typedef struct Macho {
    Buf           data;
    u32           cputype;
    MachoType     filetype;
    u8            uuid[16];
    bool          has_uuid;
    MachoSegments segments;
    MachoSections sections;
    MachoSymbols  symbols;
} Macho;

///
/// Open and parse a Mach-O file from disk.
///
/// SUCCESS : Returns true; parser owns the read-in buffer.
/// FAILURE : Returns false on read / magic / load-command parse error.
///           Fat/universal headers (`CAFEBABE`) are rejected as
///           unsupported in v1; the caller must pick a slice.
///
/// TAGS: Parser, MachO, File
///
bool macho_open(Macho *out, Zstr path, Allocator *alloc);
#define MachoOpen(...) MISRA_OVERLOAD(MachoOpen, __VA_ARGS__)
#define MachoOpen_2(out, path)                                                                                         \
    _Generic(                                                                                                          \
        (path),                                                                                                        \
        Str *: macho_open((out), ((Str *)(path))->data, MisraScope),                                                   \
        char *: macho_open((out), (const char *)(path), MisraScope),                                                   \
        const char *: macho_open((out), (const char *)(path), MisraScope)                                              \
    )
#define MachoOpen_3(out, path, alloc)                                                                                  \
    _Generic(                                                                                                          \
        (path),                                                                                                        \
        Str *: macho_open((out), ((Str *)(path))->data, ALLOCATOR_OF(alloc)),                                          \
        char *: macho_open((out), (const char *)(path), ALLOCATOR_OF(alloc)),                                          \
        const char *: macho_open((out), (const char *)(path), ALLOCATOR_OF(alloc))                                     \
    )

///
/// Parse a Mach-O image from an in-memory byte range -- **L-value /
/// ownership-transfer form** (mirrors `VecInsertL`).
///
/// Takes the caller's `Buf` by pointer. The parser snapshots the Buf
/// and zeroes the caller's `*in` so any post-call use is an empty Buf
/// rather than a stale alias. Allocator comes from the Buf. The
/// zero-on-take invariant holds on success and failure.
///
/// USAGE:
///   Buf buf = BufInit(&alloc);
///   FileRead(&f, &buf);
///   MachoOpenFromMemory(&m, &buf);
///   // buf is now zeroed.
///
/// SUCCESS : Returns true; `out` owns the bytes; `*in` is zeroed.
/// FAILURE : Returns false; the bytes have been freed; `*in` is zeroed;
///           `out` is left zeroed.
///
/// TAGS: Parser, MachO, Memory, Ownership
///
bool MachoOpenFromMemory(Macho *out, Buf *in);

///
/// Parse a Mach-O image from an in-memory byte range -- **R-value /
/// copy form** (mirrors `VecInsertR`).
///
/// Parser allocates its own buffer through `alloc` and `MemCopy`s the
/// caller's bytes in. Caller's pointer is never retained; their
/// buffer remains theirs.
///
/// SUCCESS : Returns true; `out` owns an independent copy of `data`.
/// FAILURE : Returns false; `out` zeroed; caller's `data` untouched.
///
/// TAGS: Parser, MachO, Memory, Copy
///
bool macho_open_from_memory_copy(Macho *out, const u8 *data, size data_size, Allocator *alloc);
#define MachoOpenFromMemoryCopy(...) MISRA_OVERLOAD(MachoOpenFromMemoryCopy, __VA_ARGS__)
#define MachoOpenFromMemoryCopy_3(out, data, data_size)                                                                \
    macho_open_from_memory_copy((out), (data), (data_size), MisraScope)
#define MachoOpenFromMemoryCopy_4(out, data, data_size, alloc)                                                         \
    macho_open_from_memory_copy((out), (data), (data_size), ALLOCATOR_OF(alloc))

///
/// Release storage owned by a `Macho`. Frees `data` through
/// `allocator` (unconditional -- the parser always owns its bytes)
/// and tears down the vectors. Safe on a zeroed struct.
///
/// SUCCESS : Returns to the caller. `*self` is zeroed.
/// FAILURE : Function cannot fail. NULL `self` is a no-op.
///
void MachoDeinit(Macho *self);

///
/// Find a section by (segment, section) name pair.
///
/// SUCCESS : Returns a pointer to the matching `MachoSection`,
///           borrowed from `self` (valid until `MachoDeinit`).
/// FAILURE : Returns NULL when no section matches.
///
const MachoSection *MachoFindSection(const Macho *self, Zstr segment, Zstr section);

///
/// Look up the symbol whose `value` is closest-not-greater than
/// `vaddr` and within a reasonable function span (next symbol's value
/// or the segment end).
///
/// `vaddr` is the file-relative virtual address: for a runtime IP in a
/// PIE binary, subtract the slide first.
///
/// SUCCESS : Returns a pointer to the matching `MachoSymbol`.
/// FAILURE : Returns NULL.
///
const MachoSymbol *MachoResolveAddress(const Macho *self, u64 vaddr);

#endif // MISRA_PARSERS_MACHO_H
