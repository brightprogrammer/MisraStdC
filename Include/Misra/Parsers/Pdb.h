/// file      : Pdb.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// PDB (Program Database) reader. The Windows counterpart to DWARF:
/// a sidecar file written by `link.exe` / `lld-link` containing
/// symbols, types, and (optionally) line info for a PE binary.
///
/// PDB on disk is wrapped in an MSF (Multi-Stream File) container --
/// roughly "a tiny in-file filesystem of fixed-size pages". Stream
/// directory at a fixed location lists every stream's byte length
/// plus the page indices that store its content. The PDB itself is a
/// handful of well-known streams sitting inside that container:
///
///   - Stream #1: "PDB Info" -- GUID, age, named-streams hash.
///   - Stream #3: "DBI" -- per-module info + section contributions +
///                indices of the symbol streams.
///   - Globals / Publics streams (numbered from DBI): function-name
///                to RVA map (`S_PUB32` records, mainly).
///
/// v1 of this reader is narrow on purpose: open the MSF, validate the
/// PDB Info GUID/age, and surface a function-name lookup-by-RVA. Type
/// info, line numbers, and module-private symbols are deferred.

#ifndef MISRA_PARSERS_PDB_H
#define MISRA_PARSERS_PDB_H

#include <Misra/Std/Allocator.h>
#include <Misra/Std/Container/Vec.h>
#include <Misra/Types.h>

///
/// One public function name + range from the PDB.
///
/// FIELDS:
/// - rva  : Image-relative virtual address of the function start
///          (= file-relative VA in PE / `runtime_addr - ImageBase`).
/// - size : Best-effort byte size. PDB `S_PUB32` records don't carry
///          a size directly; we infer it as `next_rva - this_rva`
///          after sorting, so the last function's `size` is 0.
/// - name : Function name string. Borrowed from the PDB's name pool;
///          valid until `PdbFileDeinit`.
///
typedef struct PdbFunction {
    u32         rva;
    u32         size;
    const char *name;
} PdbFunction;

typedef Vec(PdbFunction) PdbFunctions;

///
/// PDB info-stream contents (stream #1). Used by callers that want
/// to validate the PDB matches a particular PE's CodeView record
/// before trusting any names out of it.
///
typedef struct PdbInfo {
    u32 version;
    u32 signature; // historical Unix timestamp; unused in v1
    u32 age;       // must match PE codeview.age
    u8  guid[16];  // must match PE codeview.guid
} PdbInfo;

///
/// Parsed PDB file. Holds the raw bytes (owned or borrowed) plus
/// decoded indices into them.
///
/// FIELDS:
/// - allocator   : Allocator backing the byte buffer + functions vec.
/// - data        : Raw PDB bytes.
/// - data_size   : Length of `data` in bytes.
/// - owns_data   : True if `data` was allocated via `allocator`.
/// - block_size  : MSF page size (read from the superblock; usually
///                 4096 but can be 512/1024/2048).
/// - num_streams : Stream count from the directory.
/// - info        : Decoded PDB Info stream (#1).
/// - functions   : Sorted-by-rva list of public function names from
///                 the Publics stream. Populated by
///                 `PdbFileOpen[FromMemory]`.
///
typedef struct PdbFile {
    Allocator   *allocator;
    u8          *data;
    size         data_size;
    bool         owns_data;
    u32          block_size;
    u32          num_streams;
    PdbInfo      info;
    PdbFunctions functions;
    // `dir_stream_blocks` borrows from `data` (it's an array of u32
    // block indices for the stream directory itself), and
    // `stream_dir` is a malloced contiguous reconstruction of the
    // directory bytes. Together they support stream reads.
    const u32 *dir_stream_blocks;
    u32        dir_stream_blocks_count;
    u8        *stream_dir;
    u32        stream_dir_size;
    // Per-stream metadata extracted from the directory.
    u32        *stream_sizes;  // length = num_streams
    const u32 **stream_blocks; // length = num_streams; each entry points into stream_dir bytes
    u32        *stream_block_counts;
    // Owned name pool for function-name strings. `functions[i].name`
    // is a borrowed pointer into here; pool and entries are freed
    // together in `PdbFileDeinit`.
    char *name_pool;
    size  name_pool_size;
    size  name_pool_used;
} PdbFile;

///
/// Open and parse a PDB from disk.
///
/// SUCCESS : Returns true; `out->owns_data` is true.
/// FAILURE : Returns false; logs the failing step (open / magic /
///           directory-parse / etc). `out` is left zeroed.
///
/// TAGS: Parser, PDB, File
///
bool PdbFileOpen(PdbFile *out, const char *path, Allocator *alloc);

///
/// Open and parse a PDB from an in-memory byte range. The `data`
/// buffer is borrowed.
///
/// SUCCESS : Returns true; `out->owns_data` is false.
/// FAILURE : Returns false. `out` is left zeroed.
///
/// TAGS: Parser, PDB, Memory
///
bool PdbFileOpenFromMemory(PdbFile *out, u8 *data, size data_size, Allocator *alloc);

///
/// Release storage owned by a `PdbFile`. Safe on a zeroed struct.
///
void PdbFileDeinit(PdbFile *self);

///
/// Locate the function whose `[rva, rva + size)` range contains
/// `rva`. (For the trailing function whose `size` is 0 we accept any
/// `rva >= function.rva`.)
///
/// SUCCESS : Returns a pointer to the matching entry. Valid until
///           `PdbFileDeinit`.
/// FAILURE : Returns NULL.
///
const PdbFunction *PdbFileResolveRva(const PdbFile *self, u32 rva);

#endif // MISRA_PARSERS_PDB_H
