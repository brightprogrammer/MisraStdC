/// file      : Pe.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// PE/COFF (Portable Executable / Common Object File Format) parser.
/// The Windows counterpart of `Parsers/Elf`. Reads the DOS stub, the
/// NT headers, the section table, and the data directories of a 64-bit
/// PE32+ image directly out of bytes -- no Windows API, no dbghelp.
///
/// The narrow goal of v1 is to support our backtrace-resolution
/// pipeline: locate the CodeView debug record so the in-tree
/// `Parsers/Pdb` reader can find the matching `.pdb` for symbol-name
/// resolution. RVA -> file-offset translation is exposed for callers
/// that want to walk other sections (debug directory aside).

#ifndef MISRA_PARSERS_PE_H
#define MISRA_PARSERS_PE_H

#include <Misra/Std/Allocator.h>
#include <Misra/Std/Container/Vec.h>
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
///              Valid until `PeFileDeinit`.
///
typedef struct PeCodeViewInfo {
    bool        present;
    u8          guid[16];
    u32         age;
    const char *pdb_path;
} PeCodeViewInfo;

///
/// Parsed PE file. Holds the raw bytes plus decoded indices. Like
/// `ElfFile`, supports both an owned-buffer and a borrowed-buffer
/// construction path.
///
/// FIELDS:
/// - allocator     : Allocator for the owned buffer (if any) and the
///                   sections vector.
/// - data          : Pointer to the raw PE bytes.
/// - data_size     : Length of `data` in bytes.
/// - owns_data     : True if `data` was allocated via `allocator`.
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
typedef struct PeFile {
    Allocator     *allocator;
    u8            *data;
    size           data_size;
    bool           owns_data;
    PeMachine      machine;
    bool           is_pe32_plus;
    u64            image_base;
    u32            size_of_image;
    PeSections     sections;
    PeCodeViewInfo codeview;
} PeFile;

///
/// Open and parse a PE file from disk.
///
/// out[out]   : Populated on success.
/// path[in]   : Filesystem path.
/// alloc[in]  : Allocator for the read-in buffer and the sections
///              vector. Must outlive the `PeFile`.
///
/// SUCCESS : Returns true; `out` is fully populated and
///           `out->owns_data` is true.
/// FAILURE : Returns false; logs the failing step (open / magic /
///           NT-signature / etc). `out` is left zeroed.
///
/// TAGS: Parser, PE, File
///
bool pe_file_open(PeFile *out, const char *path, Allocator *alloc);
#define PeFileOpen(...)                MISRA_OVERLOAD(PeFileOpen, __VA_ARGS__)
#define PeFileOpen_2(out, path)        pe_file_open((out), (path), MisraScope)
#define PeFileOpen_3(out, path, alloc) pe_file_open((out), (path), ALLOCATOR_OF(alloc))

///
/// Parse a PE image from an in-memory byte range. The `data` buffer
/// is borrowed -- the caller must keep it alive for the lifetime of
/// the returned `PeFile`.
///
/// SUCCESS : Returns true; `out->owns_data` is false.
/// FAILURE : Returns false; logs the failing step. `out` is left
///           zeroed.
///
/// TAGS: Parser, PE, Memory
///
bool pe_file_open_from_memory(PeFile *out, u8 *data, size data_size, Allocator *alloc);
#define PeFileOpenFromMemory(...)                          MISRA_OVERLOAD(PeFileOpenFromMemory, __VA_ARGS__)
#define PeFileOpenFromMemory_3(out, data, data_size)        pe_file_open_from_memory((out), (data), (data_size), MisraScope)
#define PeFileOpenFromMemory_4(out, data, data_size, alloc) pe_file_open_from_memory((out), (data), (data_size), ALLOCATOR_OF(alloc))

///
/// Release storage owned by a `PeFile`. Frees the byte buffer if
/// `owns_data` was true. Safe on a zeroed struct.
///
void PeFileDeinit(PeFile *self);

///
/// Find a section by name (first match; PE allows duplicates but
/// they're vanishingly rare).
///
const PeSection *PeFileFindSection(const PeFile *self, const char *name);

///
/// Convert an RVA (offset from `ImageBase`) to a file offset by
/// finding the section whose `[VirtualAddress, VirtualAddress +
/// VirtualSize)` contains `rva`, then adding the in-section delta to
/// the section's `PointerToRawData`.
///
/// SUCCESS : Returns true; `*out_offset` is set.
/// FAILURE : Returns false when no section covers `rva` or the result
///           would point past `data_size`.
///
/// TAGS: Parser, PE, Address
///
bool PeFileRvaToOffset(const PeFile *self, u32 rva, u64 *out_offset);

#endif // MISRA_PARSERS_PE_H
