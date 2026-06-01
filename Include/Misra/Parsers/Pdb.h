/// file      : parsers/pdb.h
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

#include <Misra/Parsers/Pdb/Private.h>
#include <Misra/Std/Allocator.h>
#include <Misra/Std/Container/Buf.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Container/Vec.h>
#include <Misra/Std/Zstr.h>
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
///          valid until `PdbDeinit`.
///
typedef struct PdbFunction {
    u32  rva;
    u32  size;
    Zstr name;
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
/// Parsed PDB file. Holds the raw bytes (always parser-owned) plus
/// decoded indices into them. All three `PdbOpen*` constructors
/// leave the parser as sole owner of `data` -- see the L / R
/// semantics on the `FromMemory` / `FromMemoryCopy` constructors
/// (mirrors `VecInsertL` / `VecInsertR`).
///
/// FIELDS:
/// - data        : Raw PDB bytes as a `Buf` (owned). Carries its own
///                 length and allocator -- read via `BufLength` /
///                 `BufData` / `BufAllocator`.
/// - block_size  : MSF page size (read from the superblock; usually
///                 4096 but can be 512/1024/2048).
/// - num_streams : Stream count from the directory.
/// - info        : Decoded PDB Info stream (#1).
/// - functions   : Sorted-by-rva list of public function names from
///                 the Publics stream. Populated by
///                 `PdbOpen[FromMemory]`.
///
typedef struct Pdb {
    Buf          data;
    u32          block_size;
    u32          num_streams;
    PdbInfo      info;
    PdbFunctions functions;
    // `dir_stream_blocks` borrows from `data` (it's an array of u32
    // block indices for the stream directory itself), and
    // `stream_dir` is an allocator-backed contiguous reconstruction of
    // the directory bytes. Together they support stream reads.
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
    // together in `PdbDeinit`.
    Str name_pool;
} Pdb;

///
/// Borrowed handle to the decoded PDB Info stream (stream #1). Cross-
/// namespace readers (`PdbCache`, ...) compare `(guid, age)` against
/// the PE's CodeView record to confirm the PDB matches; this is the
/// public seam they go through instead of reaching at `self->info`
/// directly.
///
/// TAGS: Parser, PDB, Accessor
///
#define PdbInfoStream(self) ((void)0, &(self)->info)

///
/// Open and parse a PDB from disk.
///
/// SUCCESS : Returns true; parser owns the read-in buffer.
/// FAILURE : Returns false; logs the failing step. `out` is left zeroed.
///
/// TAGS: Parser, PDB, File
///
#define PdbOpen(...) OVERLOAD(PdbOpen, __VA_ARGS__)
#define PdbOpen_2(out, path)                                                                                           \
    _Generic(                                                                                                          \
        (path),                                                                                                        \
        Str *: pdb_open((out), (Zstr)StrBegin((Str *)(path)), MisraScope),                                             \
        Zstr: pdb_open((out), (Zstr)(path), MisraScope),                                                               \
        char *: pdb_open((out), (Zstr)(path), MisraScope)                                                              \
    )
#define PdbOpen_3(out, path, alloc)                                                                                    \
    _Generic(                                                                                                          \
        (path),                                                                                                        \
        Str *: pdb_open((out), (Zstr)StrBegin((Str *)(path)), ALLOCATOR_OF(alloc)),                                    \
        Zstr: pdb_open((out), (Zstr)(path), ALLOCATOR_OF(alloc)),                                                      \
        char *: pdb_open((out), (Zstr)(path), ALLOCATOR_OF(alloc))                                                     \
    )

///
/// Open and parse a PDB from an in-memory byte range -- **L-value /
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
///   PdbOpenFromMemory(&pdb, &buf);
///   // buf is now zeroed.
///
/// SUCCESS : Returns true; `out` owns the bytes; `*in` is zeroed.
/// FAILURE : Returns false; the bytes have been freed through the Buf's
///           allocator; `*in` is zeroed; `out` is left zeroed.
///
/// TAGS: Parser, PDB, Memory, Ownership
///
bool PdbOpenFromMemory(Pdb *out, Buf *in);

///
/// Open and parse a PDB from an in-memory byte range -- **R-value /
/// copy form** (mirrors `VecInsertR`).
///
/// Parser allocates its own buffer through `alloc` and `MemCopy`s
/// the caller's bytes in. Caller's pointer is never retained.
///
/// SUCCESS : Returns true; `out` owns an independent copy of `data`.
/// FAILURE : Returns false; `out` zeroed; caller's `data` untouched.
///
/// TAGS: Parser, PDB, Memory, Copy
///
#define PdbOpenFromMemoryCopy(...)                    OVERLOAD(PdbOpenFromMemoryCopy, __VA_ARGS__)
#define PdbOpenFromMemoryCopy_3(out, data, data_size) pdb_open_from_memory_copy((out), (data), (data_size), MisraScope)
#define PdbOpenFromMemoryCopy_4(out, data, data_size, alloc)                                                           \
    pdb_open_from_memory_copy((out), (data), (data_size), ALLOCATOR_OF(alloc))

///
/// Release storage owned by a `Pdb`. Safe on a zeroed struct.
///
/// SUCCESS : Returns to the caller. `*self` is zeroed.
/// FAILURE : Function cannot fail. NULL `self` is a no-op.
///
/// TAGS: Parser, PDB, Deinit, Lifecycle
///
void PdbDeinit(Pdb *self);

///
/// Locate the function whose `[rva, rva + size)` range contains
/// `rva`. (For the trailing function whose `size` is 0 we accept any
/// `rva >= function.rva`.)
///
/// SUCCESS : Returns a pointer to the matching entry. Valid until
///           `PdbDeinit`.
/// FAILURE : Returns NULL.
///
/// TAGS: Parser, PDB, Function, Resolve
///
const PdbFunction *PdbResolveRva(const Pdb *self, u32 rva);

#endif // MISRA_PARSERS_PDB_H
