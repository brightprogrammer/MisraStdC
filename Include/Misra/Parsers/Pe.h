/// file      : parsers/pe.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// PE/COFF (Portable Executable / Common Object File Format) parser.
/// The Windows counterpart of `Parsers/Elf`. Reads the DOS stub, the
/// NT headers, the section table, and the data directories of a 64-bit
/// PE32+ image directly out of bytes, so a parsed `Pe` is just a view
/// over caller-owned memory and can be built without a loader hosting
/// the image first.
///
/// The narrow goal of v1 is to support the backtrace-resolution
/// pipeline: locate the CodeView debug record so `Parsers/Pdb` can find
/// the matching `.pdb` for symbol-name resolution. RVA -> file-offset
/// translation is exposed for callers that want to walk other sections
/// (debug directory aside).

#ifndef MISRA_PARSERS_PE_H
#define MISRA_PARSERS_PE_H

#include <Misra/Parsers/Pe/Private.h>
#include <Misra/Std/Allocator.h>
#include <Misra/Std/Container/Buf.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Container/Vec.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Types.h>

typedef enum PeMachine {
    PE_MACHINE_UNKNOWN = 0,
    PE_MACHINE_I386    = 0x14C,
    PE_MACHINE_X86_64  = 0x8664,
    PE_MACHINE_ARM     = 0x1C0,
    PE_MACHINE_ARM64   = 0xAA64,
} PeMachine;

///
/// Decoded section header. PE section names are at most 8 bytes
/// (longer names exist for object files via `/N` offsets into the
/// string table, but executables don't use them); we store them as a
/// fixed 9-byte buffer (8 + NUL).
///
typedef struct PeSection {
    char name[9];
    u32  virtual_size;
    u32  virtual_address; // RVA (offset from ImageBase)
    u32  raw_size;
    u32  raw_offset;      // file offset
    u32  characteristics;
} PeSection;

typedef Vec(PeSection) PeSections;

///
/// CodeView debug record extracted from the PE Debug Directory. The
/// matching PDB file is identified by its `(guid, age)` pair; the
/// `pdb_path` is a hint from the linker (usually an absolute path on
/// the build machine -- callers should treat it as a basename plus
/// fallback search rather than an authoritative location).
///
/// FIELDS:
/// - present  : True if the PE contained a CodeView entry. False for
///              binaries built without `-debug` / `/DEBUG` or stripped
///              of debug info entirely.
/// - guid     : 16-byte unique identifier matching the corresponding
///              PDB's signature.
/// - age      : Generation counter incremented on every PDB write;
///              the PDB must have the same age to be considered a
///              match.
/// - pdb_path : NUL-terminated string borrowed from the PE bytes.
///              Valid until `PeDeinit`.
///
typedef struct PeCodeViewInfo {
    bool present;
    u8   guid[16];
    u32  age;
    Zstr pdb_path;
} PeCodeViewInfo;

///
/// Parsed PE file. Holds the raw bytes plus decoded indices. All
/// three `PeOpen*` constructors leave the parser as the sole
/// owner of `data` -- see the L / R semantics on the `FromMemory` /
/// `FromMemoryCopy` constructors (mirrors `VecInsertL` / `VecInsertR`).
///
/// FIELDS:
/// - data          : Raw PE bytes as a `Buf` (owned). Carries its
///                   own length and allocator -- read via
///                   `BufLength` / `BufData` / `BufAllocator`.
/// - machine       : Decoded `IMAGE_FILE_HEADER.Machine`.
/// - is_pe32_plus  : True for PE32+ (64-bit). v1 supports both PE32
///                   and PE32+ headers, but the address-bearing fields
///                   are widened to 64 bits even on PE32.
/// - image_base    : `OptionalHeader.ImageBase` -- the runtime virtual
///                   address the loader places the image at when no
///                   relocation is required.
/// - size_of_image : `OptionalHeader.SizeOfImage` -- total in-memory
///                   size (helps bounds-check RVAs).
/// - sections      : All section headers, in original order.
/// - codeview      : CodeView debug record, if present.
///
typedef struct Pe {
    Buf            data;
    PeMachine      machine;
    bool           is_pe32_plus;
    u64            image_base;
    u32            size_of_image;
    PeSections     sections;
    PeCodeViewInfo codeview;
} Pe;

///
/// Borrowed handle to the PE's decoded CodeView debug record. Cross-
/// namespace readers (`PdbCache`, ...) inspect `(guid, age, pdb_path)`
/// to locate the matching PDB; this is the public seam they go through
/// instead of reaching at `self->codeview` directly.
///
/// TAGS: Parser, PE, Accessor
///
#define PeCodeView(self) ((void)0, &(self)->codeview)

///
/// Open and parse a PE file from disk.
///
/// Call shapes via `OVERLOAD` + `_Generic` on `path`:
///   `PeOpen(out, path)`                 -- `path` is `Str *` or `Zstr`.
///   `PeOpen(out, path, alloc)`          -- same, explicit allocator.
///   `PeOpen(out, path, path_len, alloc)`-- `path` is a fixed-length view
///                                          (`Zstr`, `size`); it is copied
///                                          into a stack buffer and
///                                          NUL-terminated for the open.
///
/// out[out]     : Populated on success.
/// path[in]     : Filesystem path. `Str *` preferred; `Zstr ` accepted.
/// path_len[in] : Length of `path` for the fixed-length form.
/// alloc[in]    : Allocator for the read-in buffer and the sections
///                vector. Must outlive the `Pe`.
///
/// SUCCESS : Returns true; `out` owns the read-in buffer.
/// FAILURE : Returns false; logs the failing step. `out` is left zeroed.
///
/// TAGS: Parser, PE, File
///
#define PeOpen(...) OVERLOAD(PeOpen, __VA_ARGS__)
#define PeOpen_2(out, path)                                                                                            \
    _Generic(                                                                                                          \
        (path),                                                                                                        \
        Str *: pe_open((out), (Zstr)StrBegin((Str *)(path)), MisraScope),                                              \
        Zstr: pe_open((out), (Zstr)(path), MisraScope),                                                                \
        char *: pe_open((out), (Zstr)(path), MisraScope)                                                               \
    )
#define PeOpen_3(out, path, alloc)                                                                                     \
    _Generic(                                                                                                          \
        (path),                                                                                                        \
        Str *: pe_open((out), (Zstr)StrBegin((Str *)(path)), ALLOCATOR_OF(alloc)),                                     \
        Zstr: pe_open((out), (Zstr)(path), ALLOCATOR_OF(alloc)),                                                       \
        char *: pe_open((out), (Zstr)(path), ALLOCATOR_OF(alloc))                                                      \
    )
#define PeOpen_4(out, path, len, alloc) pe_open_n((out), (Zstr)(path), (len), ALLOCATOR_OF(alloc))

///
/// Parse a PE image from an in-memory byte range -- **L-value /
/// ownership-transfer form** (mirrors `VecInsertL`).
///
/// Takes the caller's `Buf` by pointer. The parser snapshots the Buf
/// and zeroes the caller's `*in` so any post-call use sees an empty
/// Buf instead of a stale alias. Allocator comes from the Buf. The
/// zero-on-take invariant holds on success and failure.
///
/// USAGE:
///   Buf buf = BufInit(&alloc);
///   FileRead(&f, &buf);
///   PeOpenFromMemory(&pe, &buf);
///   // buf is now zeroed.
///
/// SUCCESS : Returns true; `out` owns the bytes; `*in` is zeroed.
/// FAILURE : Returns false; the bytes have been freed through the Buf's
///           allocator; `*in` is zeroed; `out` is left zeroed.
///
/// TAGS: Parser, PE, Memory, Ownership
///
bool PeOpenFromMemory(Pe *out, Buf *in);

///
/// Parse a PE image from an in-memory byte range -- **R-value /
/// copy form** (mirrors `VecInsertR`).
///
/// Parser allocates its own buffer through `alloc` and `MemCopy`s the
/// caller's bytes in. Caller's pointer is never retained.
///
/// SUCCESS : Returns true; `out` owns an independent copy of `data`.
/// FAILURE : Returns false; `out` zeroed; caller's `data` untouched.
///
/// TAGS: Parser, PE, Memory, Copy
///
#define PeOpenFromMemoryCopy(...)                    OVERLOAD(PeOpenFromMemoryCopy, __VA_ARGS__)
#define PeOpenFromMemoryCopy_3(out, data, data_size) pe_open_from_memory_copy((out), (data), (data_size), MisraScope)
#define PeOpenFromMemoryCopy_4(out, data, data_size, alloc)                                                            \
    pe_open_from_memory_copy((out), (data), (data_size), ALLOCATOR_OF(alloc))

///
/// Release storage owned by a `Pe`. Frees the `data` Buf through
/// its carried allocator (unconditional -- parser always owns its
/// bytes) and tears down the sections vector. Safe on a zeroed struct.
///
/// SUCCESS : Returns to the caller. `*self` is zeroed.
/// FAILURE : Function cannot fail. NULL `self` is a no-op.
///
/// TAGS: Parser, PE, Deinit, Lifecycle
///
void PeDeinit(Pe *self);

///
/// Find a section by name (first match; PE allows duplicates but
/// they're vanishingly rare).
///
/// Call shapes via `OVERLOAD` + `_Generic` on `name`:
///   `PeFindSection(self, name)`           -- `name` is `Str *` or `Zstr`.
///   `PeFindSection(self, name, name_len)` -- `name` is a fixed-length
///                                            view (`Zstr`, `size`);
///                                            matched over exactly
///                                            `name_len` bytes, no copy.
///
/// SUCCESS : Returns a pointer to the matching `PeSection`, borrowed
///           from `self` (valid until `PeDeinit`).
/// FAILURE : Returns NULL when no section matches.
///
/// TAGS: Parser, PE, Section, Query
///
#define PeFindSection(...) OVERLOAD(PeFindSection, __VA_ARGS__)
#define PeFindSection_2(self, name)                                                                                    \
    _Generic((name), Str *: pe_find_section_str, Zstr: pe_find_section_zstr, char *: pe_find_section_zstr)(            \
        (self),                                                                                                        \
        (name)                                                                                                         \
    )
#define PeFindSection_3(self, name, name_len) pe_find_section_cstr((self), (Zstr)(name), (name_len))

///
/// Convert an RVA (offset from `ImageBase`) to a file offset by
/// finding the section whose `[VirtualAddress, VirtualAddress +
/// VirtualSize)` contains `rva`, then adding the in-section delta to
/// the section's `PointerToRawData`.
///
/// SUCCESS : Returns true; `*out_offset` is set.
/// FAILURE : Returns false when no section covers `rva` or the result
///           would point past `BufLength(&self->data)`.
///
/// TAGS: Parser, PE, Address
///
bool PeRvaToOffset(const Pe *self, u32 rva, u64 *out_offset);

#endif // MISRA_PARSERS_PE_H
