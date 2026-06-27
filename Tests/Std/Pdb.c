// PDB / MSF container reader unit test. Builds a minimal valid MSF
// blob in memory with two streams (#0 = empty, #1 = PDB Info) and
// verifies the reader correctly:
//   - validates the MSF 7.00 magic,
//   - parses the superblock (block_size, num_directory_bytes,
//     block_map_addr),
//   - reconstructs the stream directory by following the block-map,
//   - extracts per-stream size + block list,
//   - reads stream #1 contents (version / signature / age / GUID).

#include <Misra.h>
#include <Misra/Parsers/Pdb.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Allocator/Debug.h>
#include <Misra/Std/Container/Buf.h>
#include <Misra/Std/File.h>
#include <Misra/Std/Memory.h>

#include "../Util/TestRunner.h"

static void wr_u32(u8 *p, u32 v) {
    p[0] = (u8)(v & 0xff);
    p[1] = (u8)(v >> 8);
    p[2] = (u8)(v >> 16);
    p[3] = (u8)(v >> 24);
}

// Layout (block_size = 512):
//   page 0 : superblock
//   page 1 : free-block map (unused -- zero-filled)
//   page 2 : (reserved -- zero-filled)
//   page 3 : block_map page (one u32 = 4, pointing at the directory page)
//   page 4 : stream directory (16 bytes)
//   page 5 : stream #1 content (28 bytes of PDB Info)
enum {
    BLOCK_SIZE = 512,
    NUM_PAGES  = 6,
    BLOB_SIZE  = BLOCK_SIZE * NUM_PAGES,
    BLOCK_MAP  = 3,
    DIR_PAGE   = 4,
    INFO_PAGE  = 5,
    INFO_SIZE  = 28,
    DIR_BYTES  = 16,
};

static u8 blob[BLOB_SIZE];

// Microsoft C/C++ MSF 7.00\r\n\x1A DS\0\0\0
static const u8 kMagic[32] = {'M', 'i', 'c', 'r', 'o', 's', 'o', 'f', 't',  ' ',  'C',  '/', 'C', '+',  '+',  ' ',
                              'M', 'S', 'F', ' ', '7', '.', '0', '0', '\r', '\n', 0x1A, 'D', 'S', '\0', '\0', '\0'};

static const u8 kGuid[16] =
    {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x00};

static void build_msf_blob(void) {
    MemSet(blob, 0, sizeof(blob));

    // --- Superblock (page 0) -------------------------------------------------
    MemCopy(blob, kMagic, 32);
    wr_u32(&blob[32], BLOCK_SIZE);
    wr_u32(&blob[36], 1);         // free_block_map_block
    wr_u32(&blob[40], NUM_PAGES); // num_blocks
    wr_u32(&blob[44], DIR_BYTES); // num_directory_bytes
    wr_u32(&blob[48], 0);         // unknown
    wr_u32(&blob[52], BLOCK_MAP); // block_map_addr

    // --- Block-map page (page 3) ---------------------------------------------
    // One u32: the page index where the directory bytes live.
    wr_u32(&blob[BLOCK_MAP * BLOCK_SIZE], DIR_PAGE);

    // --- Stream directory (page 4) -------------------------------------------
    //   u32 num_streams           = 2
    //   u32 stream_sizes[0]       = 0   (empty stream)
    //   u32 stream_sizes[1]       = 28  (PDB Info)
    //   u32 stream1_block_id      = 5   (info content lives at page 5)
    u8 *dir = &blob[DIR_PAGE * BLOCK_SIZE];
    wr_u32(&dir[0], 2);
    wr_u32(&dir[4], 0);
    wr_u32(&dir[8], INFO_SIZE);
    wr_u32(&dir[12], INFO_PAGE);

    // --- PDB Info stream (page 5) --------------------------------------------
    //   u32 version       = 20040203 ("VC110")
    //   u32 signature     = 0xdeadbeef
    //   u32 age           = 0x42
    //   u8  guid[16]      = kGuid
    u8 *info = &blob[INFO_PAGE * BLOCK_SIZE];
    wr_u32(&info[0], 20040203);
    wr_u32(&info[4], 0xdeadbeef);
    wr_u32(&info[8], 0x42);
    MemCopy(&info[12], kGuid, 16);
}

bool test_pdb_parses_minimal_msf(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    build_msf_blob();

    Pdb  pdb;
    bool ok = PdbOpenFromMemoryCopy(&pdb, blob, sizeof(blob), base);
    if (!ok) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    ok = pdb.block_size == BLOCK_SIZE;
    ok = ok && pdb.num_streams == 2;
    ok = ok && pdb.info.version == 20040203;
    ok = ok && pdb.info.signature == 0xdeadbeef;
    ok = ok && pdb.info.age == 0x42;
    ok = ok && MemCompare(pdb.info.guid, kGuid, 16) == 0;

    PdbDeinit(&pdb);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

bool test_pdb_rejects_bad_magic(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    u8 garbage[256];
    MemSet(garbage, 0xCC, sizeof(garbage));

    Pdb  pdb;
    bool ok = !PdbOpenFromMemoryCopy(&pdb, garbage, sizeof(garbage), base);

    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// Larger blob: same MSF wrapper, but with enough streams (DBI, Publics-aka-
// SymRecord, SectionHdr) to exercise the function-name walker end-to-end.
// ---------------------------------------------------------------------------

enum {
    F_BLOCK_SIZE    = 512,
    F_NUM_PAGES     = 11,
    F_BLOB_SIZE     = F_BLOCK_SIZE * F_NUM_PAGES,
    F_BLOCK_MAP     = 3,
    F_DIR_PAGE      = 4,
    F_INFO_PAGE     = 5,
    F_DBI_PAGE      = 6,
    F_SYMREC_PAGE   = 7,
    F_SECHDR_PAGE   = 8,
    F_INFO_STREAM   = 1,
    F_DBI_STREAM    = 3,
    F_SYMREC_STREAM = 4,
    F_SECHDR_STREAM = 5,
    F_NUM_STREAMS   = 6,
    F_INFO_SIZE     = 28,
    // DBI header (64) + ModInfo(0) + SectionContrib(0) + SectionMap(0) +
    // SourceInfo(0) + TypeServerMap(0) + EC(0) + OptionalDbgHeader(12) =
    // 76 bytes.
    F_DBI_SIZE    = 76,
    F_SECHDR_SIZE = 40, // one IMAGE_SECTION_HEADER
    // S_PUB32 record for "my_function":
    //   u16 rec_len  = 2 (kind) + 4 (flags) + 4 (off) + 2 (seg) + 12 (name + NUL) = 24
    //   u16 rec_kind = 0x110E
    //   u32 flags
    //   u32 offset
    //   u16 segment
    //   "my_function\0"
    // Total = 26 bytes.
    F_SYMREC_SIZE = 26,
};

static u8 fblob[F_BLOB_SIZE];

static void wr_u16(u8 *p, u16 v) {
    p[0] = (u8)(v & 0xff);
    p[1] = (u8)(v >> 8);
}

static void build_full_pdb_blob(void) {
    MemSet(fblob, 0, sizeof(fblob));

    // Directory size: 4 (count) + N_STREAMS*4 (sizes) + block_id_count*4
    // block ids: stream0=0, stream1=1, stream2=0, stream3=1, stream4=1,
    //            stream5=1 -> 4 ids -> 16 bytes
    // total: 4 + 24 + 16 = 44 bytes
    const u32 dir_bytes = 44;

    // --- Superblock --------------------------------------------------------
    MemCopy(fblob, kMagic, 32);
    wr_u32(&fblob[32], F_BLOCK_SIZE);
    wr_u32(&fblob[36], 1);           // free_block_map_block
    wr_u32(&fblob[40], F_NUM_PAGES); // num_blocks
    wr_u32(&fblob[44], dir_bytes);
    wr_u32(&fblob[48], 0);
    wr_u32(&fblob[52], F_BLOCK_MAP);

    // --- Block-map page (one index = the directory page) -------------------
    wr_u32(&fblob[F_BLOCK_MAP * F_BLOCK_SIZE], F_DIR_PAGE);

    // --- Stream directory --------------------------------------------------
    u8 *dir = &fblob[F_DIR_PAGE * F_BLOCK_SIZE];
    wr_u32(&dir[0], F_NUM_STREAMS);
    // sizes
    wr_u32(&dir[4 + 0 * 4], 0);
    wr_u32(&dir[4 + 1 * 4], F_INFO_SIZE);
    wr_u32(&dir[4 + 2 * 4], 0);
    wr_u32(&dir[4 + 3 * 4], F_DBI_SIZE);
    wr_u32(&dir[4 + 4 * 4], F_SYMREC_SIZE);
    wr_u32(&dir[4 + 5 * 4], F_SECHDR_SIZE);
    // block ids (one per non-nil non-zero-size stream)
    u8 *bids = dir + 4 + F_NUM_STREAMS * 4;
    wr_u32(&bids[0 * 4], F_INFO_PAGE);
    wr_u32(&bids[1 * 4], F_DBI_PAGE);
    wr_u32(&bids[2 * 4], F_SYMREC_PAGE);
    wr_u32(&bids[3 * 4], F_SECHDR_PAGE);

    // --- PDB Info stream (#1) ----------------------------------------------
    u8 *info = &fblob[F_INFO_PAGE * F_BLOCK_SIZE];
    wr_u32(&info[0], 20040203);
    wr_u32(&info[4], 0xfeedface);
    wr_u32(&info[8], 1);
    MemCopy(&info[12], kGuid, 16);

    // --- DBI stream (#3) ---------------------------------------------------
    // 64-byte header followed by an OptionalDbgHeader of u16 indices; we
    // need at least index #5 (SectionHdr) so >= 12 bytes (6 entries).
    u8 *dbi = &fblob[F_DBI_PAGE * F_BLOCK_SIZE];
    // VersionSignature = -1, VersionHeader = 19990903 (V70).
    wr_u32(&dbi[0], 0xFFFFFFFFu);
    wr_u32(&dbi[4], 19990903);
    wr_u32(&dbi[8], 1);                // Age
    wr_u16(&dbi[12], 0);               // GlobalStreamIndex
    wr_u16(&dbi[14], 0);               // BuildNumber
    wr_u16(&dbi[16], 0);               // PublicStreamIndex (we use SymRecord directly)
    wr_u16(&dbi[18], 0);               // PdbDllVersion
    wr_u16(&dbi[20], F_SYMREC_STREAM); // SymRecordStream <- the important one
    wr_u16(&dbi[22], 0);               // PdbDllRbld
    // ModInfo, SectContrib, SectMap, SourceInfo, TypeServerMap all 0.
    wr_u32(&dbi[24], 0);
    wr_u32(&dbi[28], 0);
    wr_u32(&dbi[32], 0);
    wr_u32(&dbi[36], 0);
    wr_u32(&dbi[40], 0);
    wr_u32(&dbi[44], 0);      // MFCTypeServerIndex
    wr_u32(&dbi[48], 12);     // OptionalDbgHeaderSize
    wr_u32(&dbi[52], 0);      // ECSubstreamSize
    wr_u16(&dbi[56], 0);      // Flags
    wr_u16(&dbi[58], 0x8664); // Machine
    wr_u32(&dbi[60], 0);      // Padding
    // OptionalDbgHeader (12 bytes = 6 u16 entries). Index 5 = SectionHdr.
    wr_u16(&dbi[64 + 0 * 2], 0xFFFF);
    wr_u16(&dbi[64 + 1 * 2], 0xFFFF);
    wr_u16(&dbi[64 + 2 * 2], 0xFFFF);
    wr_u16(&dbi[64 + 3 * 2], 0xFFFF);
    wr_u16(&dbi[64 + 4 * 2], 0xFFFF);
    wr_u16(&dbi[64 + 5 * 2], F_SECHDR_STREAM);

    // --- SymRecord stream (#4) ---------------------------------------------
    u8 *sym = &fblob[F_SYMREC_PAGE * F_BLOCK_SIZE];
    // rec_len = 24, rec_kind = 0x110E (S_PUB32)
    wr_u16(&sym[0], 24);
    wr_u16(&sym[2], 0x110E);
    wr_u32(&sym[4], 0x2);                  // Flags = FUNCTION
    wr_u32(&sym[8], 0x100);                // Offset within section
    wr_u16(&sym[12], 1);                   // Segment (1-based)
    const char name[] = "my_function";
    MemCopy(&sym[14], name, sizeof(name)); // includes NUL

    // --- SectionHdr stream (#5) --------------------------------------------
    u8 *sec = &fblob[F_SECHDR_PAGE * F_BLOCK_SIZE];
    // IMAGE_SECTION_HEADER: name(8) + VirtualSize(4) + VirtualAddress(4) +
    //                      SizeOfRawData(4) + PointerToRawData(4) +
    //                      PointerToRelocs(4) + PointerToLineNum(4) +
    //                      NumRelocs(2) + NumLineNum(2) + Characteristics(4)
    const u8 sname[8] = {'.', 't', 'e', 'x', 't', 0, 0, 0};
    MemCopy(sec, sname, 8);
    wr_u32(&sec[8], 0x2000);       // VirtualSize
    wr_u32(&sec[12], 0x1000);      // VirtualAddress
    wr_u32(&sec[16], 0x2000);      // SizeOfRawData
    wr_u32(&sec[20], 0x400);       // PointerToRawData
    wr_u32(&sec[36], 0x60000020u); // Characteristics
}

bool test_pdb_extracts_pub32_function_name(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    build_full_pdb_blob();

    Pdb  pdb;
    bool ok = PdbOpenFromMemoryCopy(&pdb, fblob, sizeof(fblob), base);
    if (!ok) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    ok = VecLen(&pdb.functions) == 1;
    if (ok) {
        const PdbFunction *f = VecPtrAt(&pdb.functions, 0);
        ok                   = ok && f->rva == 0x1100 && f->name && ZstrCompare(f->name, "my_function") == 0;
    }

    // Direct lookup by RVA.
    if (ok) {
        const PdbFunction *f = PdbResolveRva(&pdb, 0x1100);
        ok                   = f && ZstrCompare(f->name, "my_function") == 0;
    }

    PdbDeinit(&pdb);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// Multi-function blob: three S_PUB32 records pushed in non-sorted RVA
// order. Exercises the sort + binary-search RVA resolver, which the
// single-function blob above cannot distinguish from a degenerate
// (return-the-only-entry) implementation.
// ---------------------------------------------------------------------------

// Write one S_PUB32 record at `*sym` and advance it. Returns nothing;
// `*sym` points past the record on return.
static void write_pub32(u8 **sym, u32 offset, u16 segment, Zstr name) {
    u32 namelen = (u32)ZstrLen(name) + 1; // include NUL
    u16 rec_len = (u16)(2 + 4 + 4 + 2 + namelen);
    u8 *p       = *sym;
    wr_u16(&p[0], rec_len);
    wr_u16(&p[2], 0x110E); // S_PUB32
    wr_u32(&p[4], 0x2);    // flags = FUNCTION
    wr_u32(&p[8], offset);
    wr_u16(&p[12], segment);
    MemCopy(&p[14], name, namelen);
    *sym = p + 2 + rec_len;
}

enum {
    M_BLOCK_SIZE    = 512,
    M_NUM_PAGES     = 11,
    M_BLOB_SIZE     = M_BLOCK_SIZE * M_NUM_PAGES,
    M_BLOCK_MAP     = 3,
    M_DIR_PAGE      = 4,
    M_INFO_PAGE     = 5,
    M_DBI_PAGE      = 6,
    M_SYMREC_PAGE   = 7,
    M_SECHDR_PAGE   = 8,
    M_SYMREC_STREAM = 4,
    M_SECHDR_STREAM = 5,
    M_NUM_STREAMS   = 6,
    M_INFO_SIZE     = 28,
    M_DBI_SIZE      = 76,
    M_SECHDR_SIZE   = 40,
};

static u8 mblob[M_BLOB_SIZE];

static void build_multi_pdb_blob(u32 *out_symrec_size) {
    MemSet(mblob, 0, sizeof(mblob));

    // --- SymRecord stream (#4): three out-of-order publics --------------
    u8 *sym = &mblob[M_SYMREC_PAGE * M_BLOCK_SIZE];
    u8 *p   = sym;
    // Section .text is VA 0x1000; RVA = 0x1000 + offset.
    write_pub32(&p, 0x300, 1, "fn_gamma"); // rva 0x1300
    write_pub32(&p, 0x100, 1, "fn_alpha"); // rva 0x1100
    write_pub32(&p, 0x200, 1, "fn_beta");  // rva 0x1200
    u32 symrec_size  = (u32)(p - sym);
    *out_symrec_size = symrec_size;

    // Directory size: 4 (count) + N_STREAMS*4 (sizes) + 4 block ids*4.
    const u32 dir_bytes = 4 + M_NUM_STREAMS * 4 + 4 * 4;

    MemCopy(mblob, kMagic, 32);
    wr_u32(&mblob[32], M_BLOCK_SIZE);
    wr_u32(&mblob[36], 1);
    wr_u32(&mblob[40], M_NUM_PAGES);
    wr_u32(&mblob[44], dir_bytes);
    wr_u32(&mblob[48], 0);
    wr_u32(&mblob[52], M_BLOCK_MAP);

    wr_u32(&mblob[M_BLOCK_MAP * M_BLOCK_SIZE], M_DIR_PAGE);

    u8 *dir = &mblob[M_DIR_PAGE * M_BLOCK_SIZE];
    wr_u32(&dir[0], M_NUM_STREAMS);
    wr_u32(&dir[4 + 0 * 4], 0);
    wr_u32(&dir[4 + 1 * 4], M_INFO_SIZE);
    wr_u32(&dir[4 + 2 * 4], 0);
    wr_u32(&dir[4 + 3 * 4], M_DBI_SIZE);
    wr_u32(&dir[4 + 4 * 4], symrec_size);
    wr_u32(&dir[4 + 5 * 4], M_SECHDR_SIZE);
    u8 *bids = dir + 4 + M_NUM_STREAMS * 4;
    wr_u32(&bids[0 * 4], M_INFO_PAGE);
    wr_u32(&bids[1 * 4], M_DBI_PAGE);
    wr_u32(&bids[2 * 4], M_SYMREC_PAGE);
    wr_u32(&bids[3 * 4], M_SECHDR_PAGE);

    u8 *info = &mblob[M_INFO_PAGE * M_BLOCK_SIZE];
    wr_u32(&info[0], 20040203);
    wr_u32(&info[4], 0xfeedface);
    wr_u32(&info[8], 1);
    MemCopy(&info[12], kGuid, 16);

    u8 *dbi = &mblob[M_DBI_PAGE * M_BLOCK_SIZE];
    wr_u32(&dbi[0], 0xFFFFFFFFu);
    wr_u32(&dbi[4], 19990903);
    wr_u32(&dbi[8], 1);
    wr_u16(&dbi[20], M_SYMREC_STREAM);
    wr_u32(&dbi[48], 12); // OptionalDbgHeaderSize
    wr_u16(&dbi[58], 0x8664);
    wr_u16(&dbi[64 + 0 * 2], 0xFFFF);
    wr_u16(&dbi[64 + 1 * 2], 0xFFFF);
    wr_u16(&dbi[64 + 2 * 2], 0xFFFF);
    wr_u16(&dbi[64 + 3 * 2], 0xFFFF);
    wr_u16(&dbi[64 + 4 * 2], 0xFFFF);
    wr_u16(&dbi[64 + 5 * 2], M_SECHDR_STREAM);

    u8      *sec      = &mblob[M_SECHDR_PAGE * M_BLOCK_SIZE];
    const u8 sname[8] = {'.', 't', 'e', 'x', 't', 0, 0, 0};
    MemCopy(sec, sname, 8);
    wr_u32(&sec[8], 0x2000);  // VirtualSize
    wr_u32(&sec[12], 0x1000); // VirtualAddress
    wr_u32(&sec[16], 0x2000);
    wr_u32(&sec[20], 0x400);
    wr_u32(&sec[36], 0x60000020u);
}

// Contract: with multiple functions, each RVA resolves to the function
// whose [rva, rva+size) range covers it -- not merely "some" function.
// Also: an RVA below the first function is unresolved (NULL).
bool test_pdb_resolves_among_multiple_functions(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    u32 symrec_size = 0;
    build_multi_pdb_blob(&symrec_size);

    Pdb  pdb;
    bool ok = PdbOpenFromMemoryCopy(&pdb, mblob, sizeof(mblob), base);
    if (!ok) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    ok = VecLen(&pdb.functions) == 3;

    // Functions are stored sorted by RVA regardless of input order.
    if (ok) {
        const PdbFunction *f0 = VecPtrAt(&pdb.functions, 0);
        const PdbFunction *f1 = VecPtrAt(&pdb.functions, 1);
        const PdbFunction *f2 = VecPtrAt(&pdb.functions, 2);
        ok                    = ok && f0->rva == 0x1100 && ZstrCompare(f0->name, "fn_alpha") == 0;
        ok                    = ok && f1->rva == 0x1200 && ZstrCompare(f1->name, "fn_beta") == 0;
        ok                    = ok && f2->rva == 0x1300 && ZstrCompare(f2->name, "fn_gamma") == 0;
    }

    // Exact starts.
    if (ok) {
        const PdbFunction *a = PdbResolveRva(&pdb, 0x1100);
        const PdbFunction *b = PdbResolveRva(&pdb, 0x1200);
        const PdbFunction *c = PdbResolveRva(&pdb, 0x1300);
        ok                   = ok && a && ZstrCompare(a->name, "fn_alpha") == 0;
        ok                   = ok && b && ZstrCompare(b->name, "fn_beta") == 0;
        ok                   = ok && c && ZstrCompare(c->name, "fn_gamma") == 0;
    }

    // Interior addresses resolve to the *containing* function, proving
    // the resolver picks the greatest rva <= query rather than the
    // first/only entry.
    if (ok) {
        const PdbFunction *mid_a = PdbResolveRva(&pdb, 0x11FF); // inside alpha
        const PdbFunction *mid_b = PdbResolveRva(&pdb, 0x12AB); // inside beta
        ok                       = ok && mid_a && ZstrCompare(mid_a->name, "fn_alpha") == 0;
        ok                       = ok && mid_b && ZstrCompare(mid_b->name, "fn_beta") == 0;
    }

    // Below the first function -> unresolved.
    if (ok)
        ok = PdbResolveRva(&pdb, 0x1000) == NULL;

    PdbDeinit(&pdb);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Contract: an MSF whose superblock advertises an unsupported page size
// (not one of 512/1024/2048/4096) is rejected. Build a valid magic +
// superblock but with block_size = 333.
bool test_pdb_rejects_bad_block_size(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    u8 buf[256];
    MemSet(buf, 0, sizeof(buf));
    MemCopy(buf, kMagic, 32);
    wr_u32(&buf[32], 333); // bogus block size
    wr_u32(&buf[36], 1);
    wr_u32(&buf[40], 1);
    wr_u32(&buf[44], 0);
    wr_u32(&buf[48], 0);
    wr_u32(&buf[52], 0);

    Pdb  pdb;
    bool ok = !PdbOpenFromMemoryCopy(&pdb, buf, sizeof(buf), base);

    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Contract: a blob shorter than the MSF superblock is rejected (no
// over-read on a truncated file).
bool test_pdb_rejects_truncated_superblock(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    u8 buf[16];
    MemCopy(buf, kMagic, 16); // partial magic, far short of 56-byte superblock

    Pdb  pdb;
    bool ok = !PdbOpenFromMemoryCopy(&pdb, buf, sizeof(buf), base);

    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ===========================================================================
// Mutation-hardening tests (batch 1): parse_dbi_header.
//
// Each test builds a synthetic MSF/PDB byte blob whose DBI stream (#3)
// carries KNOWN field values, then drives PdbOpenFromMemoryCopy and
// inspects the downstream-observable result (the resolved publics in
// pdb.functions). The DBI header is the only thing that varies between
// tests; the surrounding MSF container, SymRecord stream (#4) and
// SectionHdr stream (#5) are held fixed so any change in the resolved
// function count is attributable to parse_dbi_header's accounting.
// ===========================================================================

enum {
    D1_BLOCK_SIZE  = 512,
    D1_NUM_PAGES   = 9,
    D1_BLOB_SIZE   = D1_BLOCK_SIZE * D1_NUM_PAGES,
    D1_BLOCK_MAP   = 3,
    D1_DIR_PAGE    = 4,
    D1_INFO_PAGE   = 5,
    D1_DBI_PAGE    = 6,
    D1_SYMREC_PAGE = 7,
    D1_SECHDR_PAGE = 8,
    D1_INFO_STREAM = 1,
    D1_DBI_STREAM  = 3,
    D1_SYMREC_STR  = 4,
    D1_SECHDR_STR  = 5,
    D1_NUM_STREAMS = 6,
    D1_INFO_SIZE   = 28,
    D1_SECHDR_SIZE = 40, // one IMAGE_SECTION_HEADER
    // S_PUB32 "my_function": 2(kind)+4(flags)+4(off)+2(seg)+12(name+NUL)=24
    // body, total record = 2(len)+24 = 26 bytes.
    D1_SYMREC_SIZE = 26,
};

typedef struct DbiParams {
    u32 dbi_size;     // advertised size of stream #3 (the DBI stream)
    u32 mod_size;     // ModInfoSize
    u32 seccontrib;   // SectionContributionSize
    u32 secmap;       // SectionMapSize
    u32 srcinfo;      // SourceInfoSize
    u32 tsm;          // TypeServerMapSize
    u32 ec_size;      // ECSubstreamSize
    u32 optdbg_size;  // OptionalDbgHeaderSize (advertised)
    u32 optdbg_off;   // byte offset where OptionalDbgHeader is physically planted
    u16 sechdr_index; // value to plant at OptionalDbgHeader[5] (SectionHdr index)
    u8  filler;       // byte to fill the substream region between header and optdbg
} DbiParams;

static u8 gblob[D1_BLOB_SIZE];

// Build the full blob; returns nothing. `p` describes the DBI stream.
static void build_blob_m1(const DbiParams *p) {
    MemSet(gblob, 0, sizeof(gblob));

    const u32 dir_bytes = 4 + D1_NUM_STREAMS * 4 + 4 * 4; // 44

    // --- Superblock ---------------------------------------------------------
    MemCopy(gblob, kMagic, 32);
    wr_u32(&gblob[32], D1_BLOCK_SIZE);
    wr_u32(&gblob[36], 1);            // free_block_map_block
    wr_u32(&gblob[40], D1_NUM_PAGES); // num_blocks
    wr_u32(&gblob[44], dir_bytes);
    wr_u32(&gblob[48], 0);
    wr_u32(&gblob[52], D1_BLOCK_MAP);

    // --- Block-map page -----------------------------------------------------
    wr_u32(&gblob[D1_BLOCK_MAP * D1_BLOCK_SIZE], D1_DIR_PAGE);

    // --- Stream directory ---------------------------------------------------
    u8 *dir = &gblob[D1_DIR_PAGE * D1_BLOCK_SIZE];
    wr_u32(&dir[0], D1_NUM_STREAMS);
    wr_u32(&dir[4 + 0 * 4], 0);
    wr_u32(&dir[4 + 1 * 4], D1_INFO_SIZE);
    wr_u32(&dir[4 + 2 * 4], 0);
    wr_u32(&dir[4 + 3 * 4], p->dbi_size);
    wr_u32(&dir[4 + 4 * 4], D1_SYMREC_SIZE);
    wr_u32(&dir[4 + 5 * 4], D1_SECHDR_SIZE);
    u8 *bids = dir + 4 + D1_NUM_STREAMS * 4;
    wr_u32(&bids[0 * 4], D1_INFO_PAGE);   // stream #1
    wr_u32(&bids[1 * 4], D1_DBI_PAGE);    // stream #3
    wr_u32(&bids[2 * 4], D1_SYMREC_PAGE); // stream #4
    wr_u32(&bids[3 * 4], D1_SECHDR_PAGE); // stream #5

    // --- PDB Info stream (#1) ----------------------------------------------
    u8 *info = &gblob[D1_INFO_PAGE * D1_BLOCK_SIZE];
    wr_u32(&info[0], 20040203);
    wr_u32(&info[4], 0xfeedface);
    wr_u32(&info[8], 1);
    MemCopy(&info[12], kGuid, 16);

    // --- DBI stream (#3) ----------------------------------------------------
    u8 *dbi = &gblob[D1_DBI_PAGE * D1_BLOCK_SIZE];
    // Fill the whole substream region (between the 64-byte header and the
    // OptionalDbgHeader) with `filler` so any mis-located read lands on a
    // recognisable, invalid SectionHdr index (0xFF -> 0xFFFF).
    MemSet(dbi + 64, p->filler, (p->optdbg_off > 64 ? p->optdbg_off - 64 : 0) + p->optdbg_size + 16);

    wr_u32(&dbi[0], 0xFFFFFFFFu);    // VersionSignature = -1
    wr_u32(&dbi[4], 19990903);       // VersionHeader (V70)
    wr_u32(&dbi[8], 1);              // Age
    wr_u16(&dbi[12], 0);             // GlobalStreamIndex
    wr_u16(&dbi[14], 0);             // BuildNumber
    wr_u16(&dbi[16], 0);             // PublicStreamIndex
    wr_u16(&dbi[18], 0);             // PdbDllVersion
    wr_u16(&dbi[20], D1_SYMREC_STR); // SymRecordStream
    wr_u16(&dbi[22], 0);             // PdbDllRbld
    wr_u32(&dbi[24], p->mod_size);
    wr_u32(&dbi[28], p->seccontrib);
    wr_u32(&dbi[32], p->secmap);
    wr_u32(&dbi[36], p->srcinfo);
    wr_u32(&dbi[40], p->tsm);
    wr_u32(&dbi[44], 0);      // MFCTypeServerIndex (not a size)
    wr_u32(&dbi[48], p->optdbg_size);
    wr_u32(&dbi[52], p->ec_size);
    wr_u16(&dbi[56], 0);      // Flags
    wr_u16(&dbi[58], 0x8664); // Machine
    wr_u32(&dbi[60], 0);      // Padding

    // OptionalDbgHeader at the (parameterised) physical offset. 6 u16
    // entries; index 5 is the SectionHdr stream index. Entries 0..4 are
    // 0xFFFF (absent).
    u8 *od = dbi + p->optdbg_off;
    wr_u16(&od[0 * 2], 0xFFFF);
    wr_u16(&od[1 * 2], 0xFFFF);
    wr_u16(&od[2 * 2], 0xFFFF);
    wr_u16(&od[3 * 2], 0xFFFF);
    wr_u16(&od[4 * 2], 0xFFFF);
    wr_u16(&od[5 * 2], p->sechdr_index);

    // --- SymRecord stream (#4): one S_PUB32 "my_function" ------------------
    u8 *sym = &gblob[D1_SYMREC_PAGE * D1_BLOCK_SIZE];
    wr_u16(&sym[0], 24);     // rec_len
    wr_u16(&sym[2], 0x110E); // S_PUB32
    wr_u32(&sym[4], 0x2);    // Flags = FUNCTION
    wr_u32(&sym[8], 0x100);  // Offset within section
    wr_u16(&sym[12], 1);     // Segment (1-based)
    const char name[] = "my_function";
    MemCopy(&sym[14], name, sizeof(name));

    // --- SectionHdr stream (#5): one .text section -------------------------
    u8      *sec      = &gblob[D1_SECHDR_PAGE * D1_BLOCK_SIZE];
    const u8 sname[8] = {'.', 't', 'e', 'x', 't', 0, 0, 0};
    MemCopy(sec, sname, 8);
    wr_u32(&sec[8], 0x2000);  // VirtualSize
    wr_u32(&sec[12], 0x1000); // VirtualAddress
    wr_u32(&sec[16], 0x2000);
    wr_u32(&sec[20], 0x400);
    wr_u32(&sec[36], 0x60000020u);
}

// Open the current gblob and return the number of resolved functions, or
// -1 if PdbOpenFromMemoryCopy failed outright. `*out_rva0` receives the
// rva of the first function when count >= 1.
static int open_and_count(Allocator *base, u32 *out_rva0) {
    Pdb pdb;
    if (!PdbOpenFromMemoryCopy(&pdb, gblob, sizeof(gblob), base))
        return -1;
    int n = (int)VecLen(&pdb.functions);
    if (n >= 1 && out_rva0)
        *out_rva0 = ((const PdbFunction *)VecPtrAt(&pdb.functions, 0))->rva;
    PdbDeinit(&pdb);
    return n;
}

// Baseline params: a clean, in-bounds DBI with nonzero, mutually
// distinct substream sizes and the OptionalDbgHeader planted directly
// after them. Resolves exactly one function ("my_function" @ rva 0x1100).
//
// optdbg_off = 64 + 16 + 24 + 32 + 40 + 48 + 8 = 232
// dbi_size   = optdbg_off + optdbg_size = 232 + 12 = 244
static DbiParams baseline_params(void) {
    DbiParams p    = {0};
    p.mod_size     = 16;
    p.seccontrib   = 24;
    p.secmap       = 32;
    p.srcinfo      = 40;
    p.tsm          = 48;
    p.ec_size      = 8;
    p.optdbg_size  = 12;
    p.optdbg_off   = 232;
    p.dbi_size     = 244;
    p.sechdr_index = D1_SECHDR_STR; // 5
    p.filler       = 0xFF;          // mis-located reads see 0xFFFF (invalid)
    return p;
}

// Sanity / accounting baseline. With distinctive nonzero substream sizes
// summed to locate the OptionalDbgHeader, exactly one function resolves.
//
// Kills the six `+` (cxx_add_to_sub) operators on the optdbg_off sum
// (line 420): flipping any one shifts the computed offset into the 0xFF
// filler, so the SectionHdr index reads as 0xFFFF (out of range) and no
// function resolves -> count drops from 1 to 0.
bool test_pd1_substream_sum_locates_optdbg(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    DbiParams p = baseline_params();
    build_blob_m1(&p);

    u32  rva = 0;
    int  n   = open_and_count(base, &rva);
    bool ok  = (n == 1) && (rva == 0x1100);

    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// The DBI's SymRecordStream index (read by BufReadFmt directly into
// r.symrec_stream) must be the one walked for publics. Planted as 4;
// the function resolves. (Guards the field-extraction wiring.)
bool test_pd1_symrec_index_drives_publics(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    DbiParams p = baseline_params();
    build_blob_m1(&p);

    u32  rva = 0;
    int  n   = open_and_count(base, &rva);
    bool ok  = (n == 1) && (rva == 0x1100);

    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Kills line 421 `cxx_add_to_sub` (`optdbg_off + optdbg_size > dbi_size`).
//
// Craft a DBI whose advertised OptionalDbgHeaderSize makes
// optdbg_off + optdbg_size OVERRUN dbi_size, so the real parser rejects
// the substream layout and resolves no function. With `+` flipped to `-`
// the (much smaller) optdbg_off - optdbg_size would slip under dbi_size
// and the parser would proceed to read a valid SectionHdr index and
// resolve a function. Real code -> 0 functions.
//
//   optdbg_off = 64 (all substreams 0), optdbg_size = 60, dbi_size = 100
//   original: 64 + 60 = 124 > 100  -> reject  (0 funcs)
//   mutant  : 64 - 60 =   4 > 100  -> proceed (1 func)
bool test_pd1_optdbg_overrun_rejected(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    DbiParams p    = {0};
    p.dbi_size     = 100;
    p.optdbg_size  = 60;
    p.optdbg_off   = 64;            // substreams all zero
    p.sechdr_index = D1_SECHDR_STR; // valid index, present for the mutant to find
    p.filler       = 0x00;
    build_blob_m1(&p);

    int  n  = open_and_count(base, NULL);
    bool ok = (n == 0);

    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Kills line 428 `cxx_add_to_sub` (`sec_hdr_off + 2 > optdbg_off + optdbg_size`).
//
// sec_hdr_off = optdbg_off + 10, so the guard reduces to `12 > optdbg_size`
// (original) vs `8 > optdbg_size` (mutant). Pick optdbg_size = 10:
//   original: 12 > 10 -> reject  (0 funcs)
//   mutant  :  8 > 10 -> proceed (1 func, since the index physically fits)
// dbi_size is large enough (80) that the SectionHdr index at offset 74 is
// physically readable, isolating this guard from the line-421 check.
bool test_pd1_sechdr_offset_bound_rejected(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    DbiParams p    = {0};
    p.dbi_size     = 80;
    p.optdbg_size  = 10;
    p.optdbg_off   = 64;
    p.sechdr_index = D1_SECHDR_STR;
    p.filler       = 0x00;
    build_blob_m1(&p);

    int  n  = open_and_count(base, NULL);
    bool ok = (n == 0);

    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Kills line 434 `cxx_lshift_to_rshift` in the SectionHdr-index assembly
// `(u16)b0 | (u16)b1 << 8`.
//
// Plant SectionHdr index bytes = {0x05, 0x01} => value 0x0105 = 261.
//   original (<<): 0x05 | (0x01 << 8) = 261  -> 261 >= num_streams(6),
//                  load_section_table rejects -> 0 functions.
//   mutant   (>>): 0x05 | (0x01 >> 8) =   5  -> valid SectionHdr stream
//                  -> 1 function.
// Real code -> 0 functions; the high byte is load-bearing.
bool test_pd1_sechdr_index_high_byte_oob(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    DbiParams p    = baseline_params();
    p.sechdr_index = 0x0105; // 261: high byte set, out of range
    build_blob_m1(&p);

    int  n  = open_and_count(base, NULL);
    bool ok = (n == 0);

    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ===========================================================================
// Mutation-hardening tests (batch 2): superblock decode, stream-directory
// reconstruction, and per-stream metadata parsing.
// ===========================================================================

enum {
    B_BS         = 512,
    B_BLOCK_MAP  = 3,
    B_DIR_PAGE   = 4,
    B_FIRST_DATA = 5,
    B_MAX_PAGES  = 32,
    // Sized for the worst case: block_size 4096 blobs (a few pages) and
    // the 512-byte multi-block blobs (up to ~12 pages). 4096 * 16 covers
    // both comfortably.
    B_BLOB_CAP = 4096 * 16,
};

static u8 g_blob[B_BLOB_CAP];

#define PDB_NIL 0xFFFFFFFFu

// Build a blob with `n` streams whose sizes are in `sizes[]`. Returns
// the total blob byte length used. Each non-empty (size>0, != NIL)
// stream gets one dedicated content page filled with `0xA0 + index`
// bytes so its reconstructed content is identifiable.
static u32 build_blob_m2(u32 bs, const u32 *sizes, u32 n, u32 *out_dir_bytes, u32 *out_num_pages) {
    MemSet(g_blob, 0, sizeof(g_blob));

    // How many content pages? One per non-empty stream.
    u32 content_pages = 0;
    for (u32 i = 0; i < n; ++i)
        if (sizes[i] != 0 && sizes[i] != PDB_NIL)
            content_pages += 1;

    u32 num_pages = B_FIRST_DATA + content_pages;

    // Directory bytes: count(4) + sizes(n*4) + block-id table(content_pages*4).
    u32 dir_bytes = 4 + n * 4 + content_pages * 4;

    // --- Superblock (page 0) ---------------------------------------------
    MemCopy(g_blob, kMagic, 32);
    wr_u32(&g_blob[32], bs);
    wr_u32(&g_blob[36], 1);           // free_block_map_block
    wr_u32(&g_blob[40], num_pages);   // num_blocks
    wr_u32(&g_blob[44], dir_bytes);   // num_directory_bytes
    wr_u32(&g_blob[48], 0);           // unknown
    wr_u32(&g_blob[52], B_BLOCK_MAP); // block_map_addr

    // --- Block-map page (page 3): single u32 = directory page index -----
    wr_u32(&g_blob[B_BLOCK_MAP * bs], B_DIR_PAGE);

    // --- Stream directory (page 4) --------------------------------------
    u8 *dir = &g_blob[B_DIR_PAGE * bs];
    wr_u32(&dir[0], n);
    for (u32 i = 0; i < n; ++i)
        wr_u32(&dir[4 + i * 4], sizes[i]);

    // --- Block-id table + content pages ---------------------------------
    u8 *bids      = dir + 4 + n * 4;
    u32 data_page = B_FIRST_DATA;
    u32 bid_idx   = 0;
    for (u32 i = 0; i < n; ++i) {
        if (sizes[i] == 0 || sizes[i] == PDB_NIL)
            continue;
        wr_u32(&bids[bid_idx * 4], data_page);
        // Fill the content page with a recognizable pattern.
        u8 *content = &g_blob[data_page * bs];
        MemSet(content, (u8)(0xA0 + i), sizes[i] < bs ? sizes[i] : bs);
        bid_idx   += 1;
        data_page += 1;
    }

    if (out_dir_bytes)
        *out_dir_bytes = dir_bytes;
    if (out_num_pages)
        *out_num_pages = num_pages;
    return num_pages * bs;
}

static u32 g_mb_blockids[64];
static u32 g_mb_blockid_count;

static u32 build_blob_multiblock(const u32 *sizes, u32 n) {
    const u32 bs = B_BS;
    MemSet(g_blob, 0, sizeof(g_blob));
    g_mb_blockid_count = 0;

    // Count total content pages = sum of ceil(size/bs) over non-NIL streams.
    u32 total_blocks = 0;
    for (u32 i = 0; i < n; ++i) {
        if (sizes[i] == 0 || sizes[i] == PDB_NIL)
            continue;
        total_blocks += (sizes[i] + bs - 1) / bs;
    }

    u32 num_pages = B_FIRST_DATA + total_blocks;
    u32 dir_bytes = 4 + n * 4 + total_blocks * 4;

    // --- Superblock ------------------------------------------------------
    MemCopy(g_blob, kMagic, 32);
    wr_u32(&g_blob[32], bs);
    wr_u32(&g_blob[36], 1);
    wr_u32(&g_blob[40], num_pages);
    wr_u32(&g_blob[44], dir_bytes);
    wr_u32(&g_blob[48], 0);
    wr_u32(&g_blob[52], B_BLOCK_MAP);

    // --- Block-map page --------------------------------------------------
    wr_u32(&g_blob[B_BLOCK_MAP * bs], B_DIR_PAGE);

    // --- Directory: count + sizes + block-id table -----------------------
    u8 *dir = &g_blob[B_DIR_PAGE * bs];
    wr_u32(&dir[0], n);
    for (u32 i = 0; i < n; ++i)
        wr_u32(&dir[4 + i * 4], sizes[i]);

    u8 *bids      = dir + 4 + n * 4;
    u32 bid_idx   = 0;
    u32 data_page = B_FIRST_DATA;
    for (u32 i = 0; i < n; ++i) {
        if (sizes[i] == 0 || sizes[i] == PDB_NIL)
            continue;
        u32 blocks = (sizes[i] + bs - 1) / bs;
        for (u32 k = 0; k < blocks; ++k) {
            wr_u32(&bids[bid_idx * 4], data_page);
            g_mb_blockids[g_mb_blockid_count++] = data_page;
            // Fill each page with a recognizable pattern.
            MemSet(&g_blob[data_page * bs], (u8)(0xA0 + i), bs);
            bid_idx   += 1;
            data_page += 1;
        }
    }

    return num_pages * bs;
}

// Decoded superblock fields (block_size, num_streams) and the directory
// reconstruction byte-count match the bytes we wrote -- pins the
// multi-byte LE assembly and the directory-size plumbing.
static bool test_pd2_superblock_fields_exact(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    u32 sizes[3]  = {0, 28, 64};
    u32 dir_bytes = 0, num_pages = 0;
    u32 blob_len = build_blob_m2(B_BS, sizes, 3, &dir_bytes, &num_pages);

    Pdb  pdb;
    bool ok = PdbOpenFromMemoryCopy(&pdb, g_blob, blob_len, base);
    if (ok) {
        ok = ok && pdb.block_size == B_BS;
        ok = ok && pdb.num_streams == 3;
        ok = ok && pdb.stream_dir_size == dir_bytes;
        PdbDeinit(&pdb);
    }
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Each of the four supported block sizes opens. This pins the
// `block_size != 512 && != 1024 && != 2048 && != 4096` chain: turning
// any `!=` into `==` makes the matching size get rejected.
static bool open_with_block_size(u32 bs) {
    DefaultAllocator alloc    = DefaultAllocatorInit();
    Allocator       *base     = ALLOCATOR_OF(&alloc);
    u32              sizes[2] = {0, 28};
    u32              blob_len = build_blob_m2(bs, sizes, 2, NULL, NULL);
    Pdb              pdb;
    bool             ok = PdbOpenFromMemoryCopy(&pdb, g_blob, blob_len, base);
    if (ok) {
        ok = (pdb.block_size == bs) && (pdb.num_streams == 2);
        PdbDeinit(&pdb);
    }
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

static bool test_pd2_block_size_512_opens(void) {
    return open_with_block_size(512);
}
static bool test_pd2_block_size_1024_opens(void) {
    return open_with_block_size(1024);
}
static bool test_pd2_block_size_2048_opens(void) {
    return open_with_block_size(2048);
}
static bool test_pd2_block_size_4096_opens(void) {
    return open_with_block_size(4096);
}

// An unsupported block size (a power of two outside the set, e.g. 256)
// is rejected. Pins the validity guard against a true accept.
static bool test_pd2_block_size_256_rejected(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    u8 buf[256];
    MemSet(buf, 0, sizeof(buf));
    MemCopy(buf, kMagic, 32);
    wr_u32(&buf[32], 256);
    wr_u32(&buf[36], 1);
    wr_u32(&buf[40], 1);
    wr_u32(&buf[44], 0);
    wr_u32(&buf[48], 0);
    wr_u32(&buf[52], 0);

    Pdb  pdb;
    bool ok = !PdbOpenFromMemoryCopy(&pdb, buf, sizeof(buf), base);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Every byte of the 32-byte MSF magic is load-bearing: flipping any
// single byte makes the magic compare fail and the open is rejected.
// This pins the magic byte-compare (==/!= per byte inside MemCompare's
// caller guard).
static bool test_pd2_magic_each_byte_matters(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    u32  sizes[2] = {0, 28};
    bool ok       = true;
    for (u32 b = 0; b < 32 && ok; ++b) {
        u32 blob_len  = build_blob_m2(B_BS, sizes, 2, NULL, NULL);
        g_blob[b]    ^= 0xFF; // corrupt one magic byte
        Pdb  pdb;
        bool opened = PdbOpenFromMemoryCopy(&pdb, g_blob, blob_len, base);
        if (opened) {
            PdbDeinit(&pdb);
            ok = false; // corrupted magic must NOT open
        }
    }
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A blob exactly the superblock size with valid magic+size is the lower
// boundary of the length guard. We can't fully open it (no directory),
// but a one-byte-shorter blob must also be rejected, exercising the
// `< SUPERBLOCK_SIZE` guard's reject side without an over-read.
static bool test_pd2_truncated_below_superblock_rejected(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    u8 buf[55]; // one short of the 56-byte superblock
    MemSet(buf, 0, sizeof(buf));
    MemCopy(buf, kMagic, 32);
    wr_u32(&buf[32], B_BS);

    Pdb  pdb;
    bool ok = !PdbOpenFromMemoryCopy(&pdb, buf, sizeof(buf), base);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A directory block id that points exactly one block PAST the end of
// the blob must be rejected. Real code's `off + block_size > BufLength`
// catches it; mutating the `+` to `-` would accept an out-of-range
// pointer. We craft a blob whose block-map page names a block index ==
// num_pages (one past the last valid page).
static bool test_pd2_dir_block_id_one_past_rejected(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    u32 sizes[2]  = {0, 28};
    u32 num_pages = 0;
    u32 blob_len  = build_blob_m2(B_BS, sizes, 2, NULL, &num_pages);
    // Point the block-map's directory entry at one-past-end.
    wr_u32(&g_blob[B_BLOCK_MAP * B_BS], num_pages);

    Pdb  pdb;
    bool ok = !PdbOpenFromMemoryCopy(&pdb, g_blob, blob_len, base);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// The block at the LAST valid index (off + block_size == BufLength)
// must open successfully -- the bounds compare is `>` not `>=`. Our
// builder already places the final content page flush against the end
// of the blob, so a successful open with correct decoded content pins
// the boundary's accept side.
static bool test_pd2_last_block_in_bounds_opens(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    u32 sizes[2] = {0, 28};
    u32 blob_len = build_blob_m2(B_BS, sizes, 2, NULL, NULL);

    Pdb  pdb;
    bool ok = PdbOpenFromMemoryCopy(&pdb, g_blob, blob_len, base);
    if (ok) {
        // Last content page (stream 1) is the final page in the blob.
        ok = pdb.num_streams == 2 && pdb.stream_sizes[1] == 28;
        PdbDeinit(&pdb);
    }
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// dir_stream_blocks_count equals the number of directory blocks. With a
// single-page directory that's exactly 1, which differs from any const
// the assignment could be mutated to.
static bool test_pd2_dir_stream_blocks_count_one(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    u32 sizes[2] = {0, 28};
    u32 blob_len = build_blob_m2(B_BS, sizes, 2, NULL, NULL);

    Pdb  pdb;
    bool ok = PdbOpenFromMemoryCopy(&pdb, g_blob, blob_len, base);
    if (ok) {
        ok = pdb.dir_stream_blocks_count == 1;
        PdbDeinit(&pdb);
    }
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// Two-block, partial-tail, NON-ADJACENT directory blob.
// ---------------------------------------------------------------------------

enum {
    T_BS        = 512,
    T_BLOCK_MAP = 3,
    T_DIR_PG0   = 4,
    T_DIR_PG1   = 10, // non-adjacent to page 4
    T_DATA_PG   = 11,
    T_NUM_PAGES = 12,
    T_BLOB      = T_BS * T_NUM_PAGES,
};

static u8 g_tblob[T_BLOB];

// num_streams = 150 -> sizes table ends at 4 + 150*4 = 604; the single
// block-id sits at offset 604..608, so dir_bytes = 608. The second
// directory block is the partial tail [512, 608) = 96 bytes, and the
// block-id at offset 604 is 92 bytes into that tail (well past 42).
enum {
    T_NUM_STREAMS = 150,
    T_DIR_BYTES   = 4 + T_NUM_STREAMS * 4 + 4, // 608
    // Stream 1 doubles as the PDB Info stream, so it must be >= 28 bytes.
    T_DATA_SIZE = 28,
};

static void build_two_block_dir(void) {
    MemSet(g_tblob, 0, sizeof(g_tblob));

    MemCopy(g_tblob, kMagic, 32);
    wr_u32(&g_tblob[32], T_BS);
    wr_u32(&g_tblob[36], 1);
    wr_u32(&g_tblob[40], T_NUM_PAGES);
    wr_u32(&g_tblob[44], T_DIR_BYTES);
    wr_u32(&g_tblob[48], 0);
    wr_u32(&g_tblob[52], T_BLOCK_MAP);

    // Block-map page: two NON-adjacent directory page indices.
    wr_u32(&g_tblob[T_BLOCK_MAP * T_BS + 0], T_DIR_PG0);
    wr_u32(&g_tblob[T_BLOCK_MAP * T_BS + 4], T_DIR_PG1);

    // Build the directory bytes contiguously, then split across pages.
    u8 dir[T_DIR_BYTES];
    MemSet(dir, 0, sizeof(dir));
    wr_u32(&dir[0], T_NUM_STREAMS);
    // All streams empty except stream 1, which has a single block.
    for (u32 i = 0; i < T_NUM_STREAMS; ++i)
        wr_u32(&dir[4 + i * 4], 0);
    wr_u32(&dir[4 + 1 * 4], T_DATA_SIZE); // stream 1 size
    // The single block-id (for stream 1) sits at the very end of the
    // directory -- in the partial tail block.
    wr_u32(&dir[4 + T_NUM_STREAMS * 4], T_DATA_PG);

    // Split: first 512 bytes to page 4, remaining 96 bytes to page 10.
    MemCopy(&g_tblob[T_DIR_PG0 * T_BS], dir, T_BS);
    MemCopy(&g_tblob[T_DIR_PG1 * T_BS], dir + T_BS, T_DIR_BYTES - T_BS);

    // Stream 1 content.
    MemSet(&g_tblob[T_DATA_PG * T_BS], 0x5A, T_DATA_SIZE);
}

// The two-page directory reconstructs correctly: num_streams (start of
// block 0), stream 1's size (block 0), and the stream-1 block-id (which
// lives in the PARTIAL tail of block 1). dir_stream_blocks_count == 2.
// Because the directory pages are non-adjacent and the block-id is at
// the tail, any wrong copy-loop bound, offset, or clamp corrupts the
// decoded block-id and fails the assertions below.
static bool test_pd2_two_block_directory(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    build_two_block_dir();

    Pdb  pdb;
    bool ok = PdbOpenFromMemoryCopy(&pdb, g_tblob, sizeof(g_tblob), base);
    if (ok) {
        ok = ok && pdb.num_streams == T_NUM_STREAMS;
        ok = ok && pdb.dir_stream_blocks_count == 2;
        ok = ok && pdb.stream_dir_size == T_DIR_BYTES;
        ok = ok && pdb.stream_sizes[1] == T_DATA_SIZE;
        ok = ok && pdb.stream_block_counts[1] == 1;
        ok = ok && pdb.stream_blocks[1] != NULL;
        if (ok)
            ok = pdb.stream_blocks[1][0] == T_DATA_PG;
        PdbDeinit(&pdb);
    }
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Mixed stream sizes (including a multi-block stream and a NIL stream)
// are decoded exactly: num_streams, each stream_sizes[i],
// stream_block_counts[i] (ceil(size/block_size)), and NIL streams get a
// NULL block list with count 0.
static bool test_pd2_stream_metadata_exact(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    // Stream 2 spans 3 blocks (size in (2*512, 3*512]); stream 3 is NIL.
    u32 sizes[5] = {0, 28, 2 * 512 + 100, PDB_NIL, 512};
    u32 blob_len = build_blob_multiblock(sizes, 5);

    Pdb  pdb;
    bool ok = PdbOpenFromMemoryCopy(&pdb, g_blob, blob_len, base);
    if (ok) {
        ok = ok && pdb.num_streams == 5;
        ok = ok && pdb.stream_sizes[0] == 0;
        ok = ok && pdb.stream_sizes[1] == 28;
        ok = ok && pdb.stream_sizes[2] == 2 * 512 + 100;
        ok = ok && pdb.stream_sizes[3] == PDB_NIL;
        ok = ok && pdb.stream_sizes[4] == 512;
        // ceil(size/512): 0->0, 28->1, 1124->3, NIL->0, 512->1.
        ok = ok && pdb.stream_block_counts[0] == 0;
        ok = ok && pdb.stream_block_counts[1] == 1;
        ok = ok && pdb.stream_block_counts[2] == 3;
        ok = ok && pdb.stream_block_counts[3] == 0;
        ok = ok && pdb.stream_block_counts[4] == 1;
        // NIL stream: NULL block list.
        ok = ok && pdb.stream_blocks[3] == NULL;
        ok = ok && pdb.stream_blocks[1] != NULL;
        ok = ok && pdb.stream_blocks[2] != NULL;
        PdbDeinit(&pdb);
    }
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// The block-id list for a multi-block stream is a contiguous run and is
// recorded in order. Read each entry back through stream_blocks[i] and
// confirm the cursor advanced by exactly cnt entries (pins the
// per-stream cursor advance `block_cursor += cnt*4` and the count).
static bool test_pd2_multiblock_block_list_contiguous(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    // stream 1 -> 2 blocks, stream 2 -> 1 block.
    u32 sizes[3] = {0, 512 + 10, 100};
    u32 blob_len = build_blob_multiblock(sizes, 3);

    Pdb  pdb;
    bool ok = PdbOpenFromMemoryCopy(&pdb, g_blob, blob_len, base);
    if (ok) {
        ok = ok && pdb.stream_block_counts[1] == 2;
        ok = ok && pdb.stream_block_counts[2] == 1;
        // stream 1's two block ids and stream 2's one block id are the
        // contiguous values we wrote (recorded below in g_mb_blockids).
        if (ok) {
            ok = pdb.stream_blocks[1][0] == g_mb_blockids[0] && pdb.stream_blocks[1][1] == g_mb_blockids[1] &&
                 pdb.stream_blocks[2][0] == g_mb_blockids[2];
        }
        PdbDeinit(&pdb);
    }
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A single-stream PDB (num_streams == 1) parses and stops early
// (num_streams == 0 returns true with no arrays; 1 means the sizes loop
// runs once). Pins the `num_streams == 0` early-out vs the loop body.
static bool test_pd2_single_stream(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    u32 sizes[1] = {28};
    u32 blob_len = build_blob_multiblock(sizes, 1);

    Pdb  pdb;
    bool ok = PdbOpenFromMemoryCopy(&pdb, g_blob, blob_len, base);
    if (ok) {
        ok = ok && pdb.num_streams == 1;
        ok = ok && pdb.stream_sizes[0] == 28;
        ok = ok && pdb.stream_block_counts[0] == 1;
        PdbDeinit(&pdb);
    }
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A directory whose advertised num_streams implies a sizes table that
// runs past the directory bytes is rejected (truncated sizes table).
// Pins the `expected > stream_dir_size` guard's reject side.
static bool test_pd2_truncated_sizes_table_rejected(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    // Build a normal 2-stream blob, then overwrite the directory's
    // num_streams field with a huge value that the dir bytes can't hold.
    u32 sizes[2] = {0, 28};
    u32 blob_len = build_blob_m2(B_BS, sizes, 2, NULL, NULL);
    u8 *dir      = &g_blob[B_DIR_PAGE * B_BS];
    wr_u32(&dir[0], 1000); // claims 1000 streams; dir page can't hold them

    Pdb  pdb;
    bool ok = !PdbOpenFromMemoryCopy(&pdb, g_blob, blob_len, base);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A directory whose block-id table is truncated (sizes claim more
// blocks than the directory bytes carry) is rejected. Pins the
// `expected + total_block_words*4 > stream_dir_size` guard.
static bool test_pd2_truncated_blockid_table_rejected(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    // Normal 2-stream blob: stream 1 size 28 -> 1 block. Inflate stream
    // 1's size so it now claims many blocks the dir can't list.
    u32 sizes[2] = {0, 28};
    u32 blob_len = build_blob_m2(B_BS, sizes, 2, NULL, NULL);
    u8 *dir      = &g_blob[B_DIR_PAGE * B_BS];
    wr_u32(&dir[4 + 1 * 4], 100 * 512); // 100 blocks claimed, none listed

    Pdb  pdb;
    bool ok = !PdbOpenFromMemoryCopy(&pdb, g_blob, blob_len, base);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A directory carrying ONLY the stream count (num_streams == 0, dir
// bytes == 4) opens: parse_directory's `stream_dir_size < 4` guard
// passes at exactly 4 (boundary), then the `num_streams == 0` early-out
// returns success with no per-stream arrays. Pins the `< 4` boundary's
// accept side.
static bool test_pd2_zero_streams_dir_exactly_4(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    // Hand-build: superblock + block-map(->dir page) + dir page holding
    // just a u32 count == 0. dir_bytes = 4.
    enum {
        Z_BS    = 512,
        Z_PAGES = 6,
        Z_MAP   = 3,
        Z_DIR   = 4
    };
    MemSet(g_blob, 0, sizeof(g_blob));
    MemCopy(g_blob, kMagic, 32);
    wr_u32(&g_blob[32], Z_BS);
    wr_u32(&g_blob[36], 1);
    wr_u32(&g_blob[40], Z_PAGES);
    wr_u32(&g_blob[44], 4); // dir_bytes == 4 (count only)
    wr_u32(&g_blob[48], 0);
    wr_u32(&g_blob[52], Z_MAP);
    wr_u32(&g_blob[Z_MAP * Z_BS], Z_DIR);
    wr_u32(&g_blob[Z_DIR * Z_BS], 0); // num_streams = 0

    Pdb  pdb;
    bool ok = PdbOpenFromMemoryCopy(&pdb, g_blob, Z_PAGES * Z_BS, base);
    if (ok) {
        ok = pdb.num_streams == 0;
        PdbDeinit(&pdb);
    }
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A directory whose bytes hold ONLY the count + sizes table (no
// block-id table) opens when every stream is empty/NIL: here
// `expected == stream_dir_size` exactly, so the `expected >
// stream_dir_size` guard must NOT reject at the boundary. Pins the `>`
// (vs `>=`) on the sizes-table bound. Stream 1 is NIL so the PDB-info
// step is skipped.
static bool test_pd2_sizes_table_exact_fit(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    // 3 streams, all empty/NIL -> total_blocks 0 -> dir_bytes = 4+12 = 16
    // and expected = 4 + 3*4 = 16 == stream_dir_size.
    u32 sizes[3] = {0, PDB_NIL, 0};
    u32 blob_len = build_blob_m2(B_BS, sizes, 3, NULL, NULL);

    Pdb  pdb;
    bool ok = PdbOpenFromMemoryCopy(&pdb, g_blob, blob_len, base);
    if (ok) {
        ok = ok && pdb.num_streams == 3;
        ok = ok && pdb.stream_sizes[1] == PDB_NIL;
        ok = ok && pdb.stream_block_counts[1] == 0;
        ok = ok && pdb.stream_blocks[1] == NULL;
        PdbDeinit(&pdb);
    }
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A VALID blob in which the block-id table's byte span exceeds
// `expected` (i.e. total_block_words*4 > 4 + num_streams*4). Real code's
// guard is `expected + words*4 > stream_dir_size` (an ADD): here the sum
// equals stream_dir_size exactly, so it accepts. Mutating the `+` to `-`
// makes `expected - words*4` underflow to a huge u64 and reject this
// perfectly valid PDB. Build stream 1 spanning 4 blocks with only one
// other (empty) stream so expected (12) < words*4 (16).
static bool test_pd2_blockwords_exceed_expected_opens(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    // sizes: stream 0 empty, stream 1 = 2000 bytes -> ceil(2000/512) = 4
    // blocks. expected = 4 + 2*4 = 12; words*4 = 16 > 12.
    u32 sizes[2] = {0, 2000};
    u32 blob_len = build_blob_multiblock(sizes, 2);

    Pdb  pdb;
    bool ok = PdbOpenFromMemoryCopy(&pdb, g_blob, blob_len, base);
    if (ok) {
        ok = ok && pdb.num_streams == 2;
        ok = ok && pdb.stream_sizes[1] == 2000;
        ok = ok && pdb.stream_block_counts[1] == 4;
        PdbDeinit(&pdb);
    }
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// 128-directory-block blob: num_dir_blocks == 128, so the block-map page
// holds exactly 128 u32 indices == 512 bytes == one full page.
// ---------------------------------------------------------------------------

enum {
    H_BS         = 512,
    H_BLOCK_MAP  = 3,
    H_DIR_FIRST  = 4,
    H_DIR_BLOCKS = 128,
    H_DIR_BYTES  = H_BS * H_DIR_BLOCKS,        // 65536 -> ceil/512 = 128
    H_NUM_PAGES  = H_DIR_FIRST + H_DIR_BLOCKS, // 132
    H_BLOB       = H_BS * H_NUM_PAGES,
};

static u8 g_hblob[H_BLOB];

static bool test_pd2_dir_blockmap_fills_one_page(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    MemSet(g_hblob, 0, sizeof(g_hblob));
    MemCopy(g_hblob, kMagic, 32);
    wr_u32(&g_hblob[32], H_BS);
    wr_u32(&g_hblob[36], 1);
    wr_u32(&g_hblob[40], H_NUM_PAGES);
    wr_u32(&g_hblob[44], H_DIR_BYTES);
    wr_u32(&g_hblob[48], 0);
    wr_u32(&g_hblob[52], H_BLOCK_MAP);

    // Block-map page: 128 directory page indices, filling the page.
    for (u32 i = 0; i < H_DIR_BLOCKS; ++i)
        wr_u32(&g_hblob[H_BLOCK_MAP * H_BS + i * 4], H_DIR_FIRST + i);

    // Directory content: one stream (index 0), NIL so no info/DBI work.
    // count(4) + sizes(1*4); rest of the 65536 dir bytes stay zero.
    u8 *dir0 = &g_hblob[H_DIR_FIRST * H_BS];
    wr_u32(&dir0[0], 1);       // num_streams = 1
    wr_u32(&dir0[4], PDB_NIL); // stream 0 is NIL -> no blocks

    Pdb  pdb;
    bool ok = PdbOpenFromMemoryCopy(&pdb, g_hblob, sizeof(g_hblob), base);
    if (ok) {
        ok = ok && pdb.dir_stream_blocks_count == H_DIR_BLOCKS;
        ok = ok && pdb.num_streams == 1;
        ok = ok && pdb.stream_sizes[0] == PDB_NIL;
        PdbDeinit(&pdb);
    }
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ===========================================================================
// Mutation-hardening tests (batch 3): PUBLICS / functions / section-table.
// ===========================================================================

// Write one S_PUB32 record at `*sym` and advance `*sym` past it.
// `rec_kind` overridable so a non-PUB32 record can be injected.
static void write_pub32_kind(u8 **sym, u16 rec_kind, u32 flags, u32 offset, u16 segment, Zstr name) {
    u32 namelen = (u32)ZstrLen(name) + 1; // include NUL
    u16 rec_len = (u16)(2 + 4 + 4 + 2 + namelen);
    u8 *p       = *sym;
    wr_u16(&p[0], rec_len);
    wr_u16(&p[2], rec_kind);
    wr_u32(&p[4], flags);
    wr_u32(&p[8], offset);
    wr_u16(&p[12], segment);
    MemCopy(&p[14], name, namelen);
    *sym = p + 2 + rec_len;
}

// Write a minimal 4-byte record (rec_len = 2: only the kind field, no body)
// and advance. `< 2` keeps this record (skips it as non-PUB32 and walks on);
// a `<= 2` would treat it as malformed and stop the whole walk.
static void write_minirec(u8 **sym, u16 rec_kind) {
    u8 *p = *sym;
    wr_u16(&p[0], 2); // rec_len = 2
    wr_u16(&p[2], rec_kind);
    *sym = p + 4;     // 2 (len field) + rec_len(2)
}

// Write an S_PUB32 whose name region [body+10 .. record end) has NO NUL
// terminator -- every byte is non-zero. Such a record must be REJECTED by
// the name-termination scan. `name_fill` non-NUL bytes are written.
static void write_pub32_unterminated(u8 **sym, u32 offset, u16 segment, u32 name_fill) {
    u16 rec_len = (u16)(2 + 4 + 4 + 2 + name_fill);
    u8 *p       = *sym;
    wr_u16(&p[0], rec_len);
    wr_u16(&p[2], 0x110E);
    wr_u32(&p[4], 0x2);
    wr_u32(&p[8], offset);
    wr_u16(&p[12], segment);
    u32 i;
    for (i = 0; i < name_fill; ++i)
        p[14 + i] = 0x41; // 'A', never a NUL -> unterminated
    *sym = p + 2 + rec_len;
}

// Write one IMAGE_SECTION_HEADER at `sec` (40 bytes).
static void write_section(u8 *sec, Zstr name8, u32 vsize, u32 vaddr) {
    u8  nm[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    u32 i;
    for (i = 0; i < 8 && name8[i]; ++i)
        nm[i] = (u8)name8[i];
    MemCopy(sec, nm, 8);
    wr_u32(&sec[8], vsize);  // VirtualSize
    wr_u32(&sec[12], vaddr); // VirtualAddress
    wr_u32(&sec[16], vsize); // SizeOfRawData
    wr_u32(&sec[20], 0x400); // PointerToRawData
    wr_u32(&sec[36], 0x60000020u);
}

enum {
    G_BLOCK_SIZE    = 512,
    G_NUM_PAGES     = 11,
    G_BLOB_SIZE     = G_BLOCK_SIZE * G_NUM_PAGES,
    G_BLOCK_MAP     = 3,
    G_DIR_PAGE      = 4,
    G_INFO_PAGE     = 5,
    G_DBI_PAGE      = 6,
    G_SYMREC_PAGE   = 7,
    G_SECHDR_PAGE   = 8,
    G_INFO_STREAM   = 1,
    G_DBI_STREAM    = 3,
    G_SYMREC_STREAM = 4,
    G_SECHDR_STREAM = 5,
    G_NUM_STREAMS   = 6,
    G_INFO_SIZE     = 28,
    G_DBI_SIZE      = 76,
};

// Build the fixed MSF scaffolding (superblock, directory, info, DBI) into
// `blob` with the given symrec/sechdr sizes. The SymRecord page and
// SectionHdr page bodies must already be written by the caller before this
// call (this only writes the wrapper that references their sizes).
static void build_scaffolding(u8 *blob, u32 symrec_size, u32 sechdr_size) {
    const u32 dir_bytes = 4 + G_NUM_STREAMS * 4 + 4 * 4;

    MemCopy(blob, kMagic, 32);
    wr_u32(&blob[32], G_BLOCK_SIZE);
    wr_u32(&blob[36], 1);
    wr_u32(&blob[40], G_NUM_PAGES);
    wr_u32(&blob[44], dir_bytes);
    wr_u32(&blob[48], 0);
    wr_u32(&blob[52], G_BLOCK_MAP);

    wr_u32(&blob[G_BLOCK_MAP * G_BLOCK_SIZE], G_DIR_PAGE);

    u8 *dir = &blob[G_DIR_PAGE * G_BLOCK_SIZE];
    wr_u32(&dir[0], G_NUM_STREAMS);
    wr_u32(&dir[4 + 0 * 4], 0);
    wr_u32(&dir[4 + 1 * 4], G_INFO_SIZE);
    wr_u32(&dir[4 + 2 * 4], 0);
    wr_u32(&dir[4 + 3 * 4], G_DBI_SIZE);
    wr_u32(&dir[4 + 4 * 4], symrec_size);
    wr_u32(&dir[4 + 5 * 4], sechdr_size);
    u8 *bids = dir + 4 + G_NUM_STREAMS * 4;
    wr_u32(&bids[0 * 4], G_INFO_PAGE);
    wr_u32(&bids[1 * 4], G_DBI_PAGE);
    wr_u32(&bids[2 * 4], G_SYMREC_PAGE);
    wr_u32(&bids[3 * 4], G_SECHDR_PAGE);

    u8 *info = &blob[G_INFO_PAGE * G_BLOCK_SIZE];
    wr_u32(&info[0], 20040203);
    wr_u32(&info[4], 0xfeedface);
    wr_u32(&info[8], 1);
    MemCopy(&info[12], kGuid, 16);

    u8 *dbi = &blob[G_DBI_PAGE * G_BLOCK_SIZE];
    wr_u32(&dbi[0], 0xFFFFFFFFu);
    wr_u32(&dbi[4], 19990903);
    wr_u32(&dbi[8], 1);
    wr_u16(&dbi[20], G_SYMREC_STREAM);
    wr_u32(&dbi[48], 12); // OptionalDbgHeaderSize
    wr_u16(&dbi[58], 0x8664);
    wr_u16(&dbi[64 + 0 * 2], 0xFFFF);
    wr_u16(&dbi[64 + 1 * 2], 0xFFFF);
    wr_u16(&dbi[64 + 2 * 2], 0xFFFF);
    wr_u16(&dbi[64 + 3 * 2], 0xFFFF);
    wr_u16(&dbi[64 + 4 * 2], 0xFFFF);
    wr_u16(&dbi[64 + 5 * 2], G_SECHDR_STREAM);
}

static u8 g_blob_filter[G_BLOB_SIZE];

static void build_filter_blob(void) {
    MemSet(g_blob_filter, 0, sizeof(g_blob_filter));

    u8 *sym = &g_blob_filter[G_SYMREC_PAGE * G_BLOCK_SIZE];
    u8 *p   = sym;
    write_pub32(&p, 0x200, 1, "fn_two");                                    // rva 0x1200
    write_pub32_kind(&p, 0x1107, 0, 0x999, 1, "data_sym");                  // non-PUB32 -> skip
    write_pub32_kind(&p, 0x110E, 0 /*not fn-flagged*/, 0x100, 1, "fn_one"); // rva 0x1100
    write_pub32(&p, 0x100, 2, "fn_four");                                   // rva 0x5100 (seg 2)
    write_pub32(&p, 0x300, 1, "fn_three");                                  // rva 0x1300
    u32 symrec_size = (u32)(p - sym);

    u8 *sec = &g_blob_filter[G_SECHDR_PAGE * G_BLOCK_SIZE];
    write_section(sec + 0, ".text", 0x4000, 0x1000);  // segment 1
    write_section(sec + 40, ".data", 0x4000, 0x5000); // segment 2
    u32 sechdr_size = 80;

    build_scaffolding(g_blob_filter, symrec_size, sechdr_size);
}

bool test_pd3_filter_and_count(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    build_filter_blob();

    Pdb  pdb;
    bool ok = PdbOpenFromMemoryCopy(&pdb, g_blob_filter, sizeof(g_blob_filter), base);
    if (!ok) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    // Exactly four function symbols; the non-PUB32 data symbol is excluded.
    ok = VecLen(&pdb.functions) == 4;
    if (ok) {
        const PdbFunction *f0 = VecPtrAt(&pdb.functions, 0);
        const PdbFunction *f1 = VecPtrAt(&pdb.functions, 1);
        const PdbFunction *f2 = VecPtrAt(&pdb.functions, 2);
        const PdbFunction *f3 = VecPtrAt(&pdb.functions, 3);
        ok                    = ok && f0->rva == 0x1100 && ZstrCompare(f0->name, "fn_one") == 0;
        ok                    = ok && f1->rva == 0x1200 && ZstrCompare(f1->name, "fn_two") == 0;
        ok                    = ok && f2->rva == 0x1300 && ZstrCompare(f2->name, "fn_three") == 0;
        ok                    = ok && f3->rva == 0x5100 && ZstrCompare(f3->name, "fn_four") == 0;
    }

    // No "data_sym" must ever appear.
    if (ok) {
        for (size i = 0; i < VecLen(&pdb.functions); ++i) {
            const PdbFunction *f = VecPtrAt(&pdb.functions, i);
            if (ZstrCompare(f->name, "data_sym") == 0) {
                ok = false;
                break;
            }
        }
    }

    PdbDeinit(&pdb);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

static u8 g_blob_sect[G_BLOB_SIZE];

static void build_sect_blob(void) {
    MemSet(g_blob_sect, 0, sizeof(g_blob_sect));

    u8 *sym = &g_blob_sect[G_SYMREC_PAGE * G_BLOCK_SIZE];
    u8 *p   = sym;
    write_pub32(&p, 0x040, 1, "in_text");  // rva 0x1040
    write_pub32(&p, 0x010, 2, "in_rdata"); // rva 0x8010
    u32 symrec_size = (u32)(p - sym);

    u8 *sec = &g_blob_sect[G_SECHDR_PAGE * G_BLOCK_SIZE];
    write_section(sec + 0, ".text", 0x1000, 0x1000);   // segment 1
    write_section(sec + 40, ".rdata", 0x1000, 0x8000); // segment 2
    u32 sechdr_size = 80;

    build_scaffolding(g_blob_sect, symrec_size, sechdr_size);
}

bool test_pd3_section_indexing(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    build_sect_blob();

    Pdb  pdb;
    bool ok = PdbOpenFromMemoryCopy(&pdb, g_blob_sect, sizeof(g_blob_sect), base);
    if (!ok) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    ok = VecLen(&pdb.functions) == 2;
    if (ok) {
        // sorted: 0x1040 (text) then 0x8010 (rdata)
        const PdbFunction *f0 = VecPtrAt(&pdb.functions, 0);
        const PdbFunction *f1 = VecPtrAt(&pdb.functions, 1);
        ok                    = ok && f0->rva == 0x1040 && ZstrCompare(f0->name, "in_text") == 0;
        ok                    = ok && f1->rva == 0x8010 && ZstrCompare(f1->name, "in_rdata") == 0;
    }

    // Resolve distinctly: a query at 0x8010 must hit in_rdata, proving the
    // second section's VA (0x8000) was used -- not section[0]'s 0x1000.
    if (ok) {
        const PdbFunction *r = PdbResolveRva(&pdb, 0x8010);
        ok                   = r && ZstrCompare(r->name, "in_rdata") == 0;
    }
    if (ok) {
        const PdbFunction *t = PdbResolveRva(&pdb, 0x1040);
        ok                   = t && ZstrCompare(t->name, "in_text") == 0;
    }

    PdbDeinit(&pdb);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

static u8 g_blob_oob[G_BLOB_SIZE];

static void build_oob_seg_blob(void) {
    MemSet(g_blob_oob, 0, sizeof(g_blob_oob));

    u8 *sym = &g_blob_oob[G_SYMREC_PAGE * G_BLOCK_SIZE];
    u8 *p   = sym;
    write_pub32(&p, 0x100, 1, "valid_fn");   // rva 0x1100, seg 1 OK
    write_pub32(&p, 0x100, 2, "bad_seg_fn"); // seg 2 -> no section -> dropped
    write_pub32(&p, 0x200, 1, "valid_fn2");  // rva 0x1200, seg 1 OK
    u32 symrec_size = (u32)(p - sym);

    u8 *sec = &g_blob_oob[G_SECHDR_PAGE * G_BLOCK_SIZE];
    write_section(sec + 0, ".text", 0x4000, 0x1000); // only segment 1
    u32 sechdr_size = 40;

    build_scaffolding(g_blob_oob, symrec_size, sechdr_size);
}

bool test_pd3_out_of_range_segment_dropped(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    build_oob_seg_blob();

    Pdb  pdb;
    bool ok = PdbOpenFromMemoryCopy(&pdb, g_blob_oob, sizeof(g_blob_oob), base);
    if (!ok) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    // Only the two segment-1 functions survive.
    ok = VecLen(&pdb.functions) == 2;
    if (ok) {
        const PdbFunction *f0 = VecPtrAt(&pdb.functions, 0);
        const PdbFunction *f1 = VecPtrAt(&pdb.functions, 1);
        ok                    = ok && f0->rva == 0x1100 && ZstrCompare(f0->name, "valid_fn") == 0;
        ok                    = ok && f1->rva == 0x1200 && ZstrCompare(f1->name, "valid_fn2") == 0;
    }
    if (ok) {
        for (size i = 0; i < VecLen(&pdb.functions); ++i) {
            const PdbFunction *f = VecPtrAt(&pdb.functions, i);
            if (ZstrCompare(f->name, "bad_seg_fn") == 0) {
                ok = false;
                break;
            }
        }
    }

    PdbDeinit(&pdb);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

static u8 g_blob_size[G_BLOB_SIZE];

static void build_size_blob(void) {
    MemSet(g_blob_size, 0, sizeof(g_blob_size));

    u8 *sym = &g_blob_size[G_SYMREC_PAGE * G_BLOCK_SIZE];
    u8 *p   = sym;
    // out of order on purpose
    write_pub32(&p, 0x380, 1, "fn_c"); // 0x1380
    write_pub32(&p, 0x100, 1, "fn_a"); // 0x1100
    write_pub32(&p, 0x300, 1, "fn_b"); // 0x1300
    u32 symrec_size = (u32)(p - sym);

    u8 *sec = &g_blob_size[G_SECHDR_PAGE * G_BLOCK_SIZE];
    write_section(sec + 0, ".text", 0x4000, 0x1000);
    u32 sechdr_size = 40;

    build_scaffolding(g_blob_size, symrec_size, sechdr_size);
}

bool test_pd3_function_sizes(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    build_size_blob();

    Pdb  pdb;
    bool ok = PdbOpenFromMemoryCopy(&pdb, g_blob_size, sizeof(g_blob_size), base);
    if (!ok) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    ok = VecLen(&pdb.functions) == 3;
    if (ok) {
        const PdbFunction *a = VecPtrAt(&pdb.functions, 0);
        const PdbFunction *b = VecPtrAt(&pdb.functions, 1);
        const PdbFunction *c = VecPtrAt(&pdb.functions, 2);
        ok                   = ok && a->rva == 0x1100 && a->size == 0x200; // 0x1300 - 0x1100
        ok                   = ok && b->rva == 0x1300 && b->size == 0x80;  // 0x1380 - 0x1300
        ok                   = ok && c->rva == 0x1380 && c->size == 0;     // trailing
    }

    // Interior address inside fn_a's [0x1100, 0x1300) resolves to fn_a;
    // an address at fn_b's start resolves to fn_b. This depends on the
    // computed sizes being correct.
    if (ok) {
        const PdbFunction *mid = PdbResolveRva(&pdb, 0x12FF);
        ok                     = mid && ZstrCompare(mid->name, "fn_a") == 0;
    }
    if (ok) {
        const PdbFunction *atb = PdbResolveRva(&pdb, 0x1300);
        ok                     = atb && ZstrCompare(atb->name, "fn_b") == 0;
    }
    // Trailing function: any rva >= its start resolves to it.
    if (ok) {
        const PdbFunction *tail = PdbResolveRva(&pdb, 0x9999);
        ok                      = tail && ZstrCompare(tail->name, "fn_c") == 0;
    }

    PdbDeinit(&pdb);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

static u8 g_blob_sort[G_BLOB_SIZE];

static void build_sort_blob(void) {
    MemSet(g_blob_sort, 0, sizeof(g_blob_sort));

    u8 *sym = &g_blob_sort[G_SYMREC_PAGE * G_BLOCK_SIZE];
    u8 *p   = sym;
    // Strictly descending offsets so the input is fully reverse-sorted.
    write_pub32(&p, 0x500, 1, "e"); // 0x1500
    write_pub32(&p, 0x400, 1, "d"); // 0x1400
    write_pub32(&p, 0x300, 1, "c"); // 0x1300
    write_pub32(&p, 0x200, 1, "b"); // 0x1200
    write_pub32(&p, 0x100, 1, "a"); // 0x1100
    u32 symrec_size = (u32)(p - sym);

    u8 *sec = &g_blob_sort[G_SECHDR_PAGE * G_BLOCK_SIZE];
    write_section(sec + 0, ".text", 0x4000, 0x1000);
    u32 sechdr_size = 40;

    build_scaffolding(g_blob_sort, symrec_size, sechdr_size);
}

bool test_pd3_sort_ascending(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    build_sort_blob();

    Pdb  pdb;
    bool ok = PdbOpenFromMemoryCopy(&pdb, g_blob_sort, sizeof(g_blob_sort), base);
    if (!ok) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    ok = VecLen(&pdb.functions) == 5;

    // Strictly ascending RVAs, despite descending input order.
    if (ok) {
        u32 prev = 0;
        for (size i = 0; i < VecLen(&pdb.functions); ++i) {
            const PdbFunction *f = VecPtrAt(&pdb.functions, i);
            if (i > 0 && !(f->rva > prev)) {
                ok = false;
                break;
            }
            prev = f->rva;
        }
    }
    // First is the smallest, last is the largest -- a reversed comparator
    // would flip these.
    if (ok) {
        const PdbFunction *first = VecPtrAt(&pdb.functions, 0);
        const PdbFunction *last  = VecPtrAt(&pdb.functions, 4);
        ok                       = ok && first->rva == 0x1100 && ZstrCompare(first->name, "a") == 0;
        ok                       = ok && last->rva == 0x1500 && ZstrCompare(last->name, "e") == 0;
    }

    PdbDeinit(&pdb);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

static u8 g_blob_mal[G_BLOB_SIZE];

static void build_mal_blob(void) {
    MemSet(g_blob_mal, 0, sizeof(g_blob_mal));

    u8 *sym = &g_blob_mal[G_SYMREC_PAGE * G_BLOCK_SIZE];
    u8 *p   = sym;
    write_minirec(&p, 0x0006);                 // bare 2-byte record -> skip
    write_pub32(&p, 0x100, 1, "fn_keep");      // rva 0x1100
    write_pub32_unterminated(&p, 0x200, 1, 8); // no NUL -> rejected
    write_pub32(&p, 0x300, 1, "fn_keep2");     // rva 0x1300
    u32 symrec_size = (u32)(p - sym);

    u8 *sec = &g_blob_mal[G_SECHDR_PAGE * G_BLOCK_SIZE];
    write_section(sec + 0, ".text", 0x4000, 0x1000);
    u32 sechdr_size = 40;

    build_scaffolding(g_blob_mal, symrec_size, sechdr_size);
}

bool test_pd3_malformed_records(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    build_mal_blob();

    Pdb  pdb;
    bool ok = PdbOpenFromMemoryCopy(&pdb, g_blob_mal, sizeof(g_blob_mal), base);
    if (!ok) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    // Exactly the two well-formed functions survive; the stub is skipped
    // (walk did NOT stop at it) and the unterminated-name record is rejected.
    ok = VecLen(&pdb.functions) == 2;
    if (ok) {
        const PdbFunction *f0 = VecPtrAt(&pdb.functions, 0);
        const PdbFunction *f1 = VecPtrAt(&pdb.functions, 1);
        ok                    = ok && f0->rva == 0x1100 && ZstrCompare(f0->name, "fn_keep") == 0;
        ok                    = ok && f1->rva == 0x1300 && ZstrCompare(f1->name, "fn_keep2") == 0;
    }
    // No all-'A' garbage name leaked in from the unterminated record.
    if (ok) {
        for (size i = 0; i < VecLen(&pdb.functions); ++i) {
            const PdbFunction *f = VecPtrAt(&pdb.functions, i);
            if (ZstrCompare(f->name, "AAAAAAAA") == 0) {
                ok = false;
                break;
            }
        }
    }

    PdbDeinit(&pdb);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

static u8 g_blob_ovf[G_BLOB_SIZE];

static void build_ovf_blob(void) {
    MemSet(g_blob_ovf, 0, sizeof(g_blob_ovf));

    u8 *sym = &g_blob_ovf[G_SYMREC_PAGE * G_BLOCK_SIZE];
    u8 *p   = sym;
    write_pub32(&p, 0x0000FFFF, 1, "fn_max"); // 0xFFFF0000 + 0xFFFF = 0xFFFFFFFF
    write_pub32(&p, 0x00010000, 1, "fn_ovf"); // 0xFFFF0000 + 0x10000 = 0x100000000 (drop)
    write_pub32(&p, 0x0050, 2, "fn_low");     // 0x2000 + 0x50 = 0x2050
    u32 symrec_size = (u32)(p - sym);

    u8 *sec = &g_blob_ovf[G_SECHDR_PAGE * G_BLOCK_SIZE];
    write_section(sec + 0, ".text", 0x1000, 0xFFFF0000u); // segment 1
    write_section(sec + 40, ".data", 0x1000, 0x2000);     // segment 2
    u32 sechdr_size = 80;

    build_scaffolding(g_blob_ovf, symrec_size, sechdr_size);
}

bool test_pd3_rva_overflow_boundary(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    build_ovf_blob();

    Pdb  pdb;
    bool ok = PdbOpenFromMemoryCopy(&pdb, g_blob_ovf, sizeof(g_blob_ovf), base);
    if (!ok) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    // fn_ovf is dropped; fn_max (exactly 4G-1) and fn_low survive.
    ok = VecLen(&pdb.functions) == 2;
    if (ok) {
        const PdbFunction *f0 = VecPtrAt(&pdb.functions, 0);
        const PdbFunction *f1 = VecPtrAt(&pdb.functions, 1);
        ok                    = ok && f0->rva == 0x2050u && ZstrCompare(f0->name, "fn_low") == 0;
        ok                    = ok && f1->rva == 0xFFFFFFFFu && ZstrCompare(f1->name, "fn_max") == 0;
    }
    if (ok) {
        for (size i = 0; i < VecLen(&pdb.functions); ++i) {
            const PdbFunction *f = VecPtrAt(&pdb.functions, i);
            if (ZstrCompare(f->name, "fn_ovf") == 0) {
                ok = false;
                break;
            }
        }
    }
    // The function past the dropped one (fn_low) must be intact -- proves
    // `cur = next` advanced correctly through the overflow branch.
    if (ok) {
        const PdbFunction *r = PdbResolveRva(&pdb, 0x2050);
        ok                   = r && ZstrCompare(r->name, "fn_low") == 0;
    }

    PdbDeinit(&pdb);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ===========================================================================
// Mutation-hardening tests (batch 4): stream-read / open / info / resolve.
// ===========================================================================

// Write a non-PUB32 filler record of total size `total` bytes (>= 4) at
// `*sym` and advance past it. The walker skips it (unknown kind) but
// still advances `cur`, which lets us position a later record across a
// block boundary. Body bytes are 0xAA so a mis-stitch is visible.
static void write_filler(u8 **sym, u16 total) {
    u8 *p       = *sym;
    u16 rec_len = (u16)(total - 2);
    wr_u16(&p[0], rec_len);
    wr_u16(&p[2], 0x0001); // S_COMPILE (not PUB32) -> ignored by walker
    for (u16 i = 4; i < total; ++i)
        p[i] = 0xAA;
    *sym = p + total;
}

enum {
    X_BLOCK_SIZE = 512,
    // Pages: 0 superblock, 1 fbm, 2 reserved, 3 block-map, 4 dir,
    //        5 info, 6 dbi, 7 sechdr,
    //        8 SymRecord block #0, 9 GAP (filler), 10 SymRecord block #1.
    X_NUM_PAGES   = 11,
    X_BLOB_SIZE   = X_BLOCK_SIZE * X_NUM_PAGES,
    X_BLOCK_MAP   = 3,
    X_DIR_PAGE    = 4,
    X_INFO_PAGE   = 5,
    X_DBI_PAGE    = 6,
    X_SECHDR_PAGE = 7,
    X_SYM_PAGE0   = 8,
    X_SYM_PAGE1   = 10, // non-adjacent to page 8 (page 9 is a decoy gap)
    X_INFO_STREAM = 1,
    X_DBI_STREAM  = 3,
    X_SYM_STREAM  = 4,
    X_SEC_STREAM  = 5,
    X_NUM_STREAMS = 6,
    X_INFO_SIZE   = 28,
    X_DBI_SIZE    = 76,
    X_SECHDR_SIZE = 40,
    // SymRecord stream is two full pages (1024 bytes). The first record
    // straddles offset 512.
    X_SYM_SIZE = 1024,
};

static u8 xblob[X_BLOB_SIZE];

// Build a PDB whose SymRecord stream spans two non-adjacent pages with a
// function record straddling the 512-byte block boundary.
static void build_crossblock_blob(void) {
    MemSet(xblob, 0, sizeof(xblob));

    // Fill the decoy gap page (9) and unused regions with a distinctive
    // 0xEE pattern so an accidental over-read of page 8 into page 9
    // would corrupt the straddling record's tail.
    MemSet(&xblob[9 * X_BLOCK_SIZE], 0xEE, X_BLOCK_SIZE);

    // Directory: 4 (count) + 6*4 (sizes) + 5 block ids * 4.
    //   block ids: info(1), dbi(1), symrec(2 -> non-adjacent), sechdr(1)
    const u32 dir_bytes = 4 + X_NUM_STREAMS * 4 + 5 * 4;

    // --- Superblock --------------------------------------------------------
    MemCopy(xblob, kMagic, 32);
    wr_u32(&xblob[32], X_BLOCK_SIZE);
    wr_u32(&xblob[36], 1);
    wr_u32(&xblob[40], X_NUM_PAGES);
    wr_u32(&xblob[44], dir_bytes);
    wr_u32(&xblob[48], 0);
    wr_u32(&xblob[52], X_BLOCK_MAP);

    // --- Block-map page ----------------------------------------------------
    wr_u32(&xblob[X_BLOCK_MAP * X_BLOCK_SIZE], X_DIR_PAGE);

    // --- Stream directory --------------------------------------------------
    u8 *dir = &xblob[X_DIR_PAGE * X_BLOCK_SIZE];
    wr_u32(&dir[0], X_NUM_STREAMS);
    wr_u32(&dir[4 + 0 * 4], 0);
    wr_u32(&dir[4 + 1 * 4], X_INFO_SIZE);
    wr_u32(&dir[4 + 2 * 4], 0);
    wr_u32(&dir[4 + 3 * 4], X_DBI_SIZE);
    wr_u32(&dir[4 + 4 * 4], X_SYM_SIZE);
    wr_u32(&dir[4 + 5 * 4], X_SECHDR_SIZE);
    // Block ids, in stream order. SymRecord gets TWO non-adjacent ids.
    u8 *bids = dir + 4 + X_NUM_STREAMS * 4;
    wr_u32(&bids[0 * 4], X_INFO_PAGE);   // stream 1
    wr_u32(&bids[1 * 4], X_DBI_PAGE);    // stream 3
    wr_u32(&bids[2 * 4], X_SYM_PAGE0);   // stream 4, block 0
    wr_u32(&bids[3 * 4], X_SYM_PAGE1);   // stream 4, block 1 (non-adjacent)
    wr_u32(&bids[4 * 4], X_SECHDR_PAGE); // stream 5

    // --- PDB Info stream (#1) ----------------------------------------------
    u8 *info = &xblob[X_INFO_PAGE * X_BLOCK_SIZE];
    wr_u32(&info[0], 20040203);
    wr_u32(&info[4], 0xfeedface);
    wr_u32(&info[8], 1);
    MemCopy(&info[12], kGuid, 16);

    // --- DBI stream (#3) ---------------------------------------------------
    u8 *dbi = &xblob[X_DBI_PAGE * X_BLOCK_SIZE];
    wr_u32(&dbi[0], 0xFFFFFFFFu);
    wr_u32(&dbi[4], 19990903);
    wr_u32(&dbi[8], 1);
    wr_u16(&dbi[20], X_SYM_STREAM);
    wr_u32(&dbi[48], 12); // OptionalDbgHeaderSize
    wr_u16(&dbi[58], 0x8664);
    wr_u16(&dbi[64 + 0 * 2], 0xFFFF);
    wr_u16(&dbi[64 + 1 * 2], 0xFFFF);
    wr_u16(&dbi[64 + 2 * 2], 0xFFFF);
    wr_u16(&dbi[64 + 3 * 2], 0xFFFF);
    wr_u16(&dbi[64 + 4 * 2], 0xFFFF);
    wr_u16(&dbi[64 + 5 * 2], X_SEC_STREAM);

    // --- SectionHdr stream (#5) --------------------------------------------
    u8      *sec      = &xblob[X_SECHDR_PAGE * X_BLOCK_SIZE];
    const u8 sname[8] = {'.', 't', 'e', 'x', 't', 0, 0, 0};
    MemCopy(sec, sname, 8);
    wr_u32(&sec[8], 0x4000);  // VirtualSize
    wr_u32(&sec[12], 0x1000); // VirtualAddress
    wr_u32(&sec[16], 0x4000);
    wr_u32(&sec[20], 0x400);
    wr_u32(&sec[36], 0x60000020u);

    // --- SymRecord stream (#4): laid out as ONE contiguous logical
    // stream that physically lives across page8 [0..512) and page10
    // [0..512). The writer fills a flat scratch then copies the two
    // halves into the two non-adjacent pages.
    u8 sym_flat[X_SYM_SIZE];
    MemSet(sym_flat, 0, sizeof(sym_flat));

    // Records are packed contiguously from offset 0 so the walker never
    // hits a zero rec_len before reaching them. A leading filler record
    // pushes the first PUB32 record to start at offset 500, so its body
    // and name straddle the 512-byte block boundary.
    u8 *p = sym_flat;
    write_filler(&p, 500); // advances cur to offset 500
    // PUB32 record straddling offset 512 (starts at 500, name spills
    // into the second block). rva = 0x1000 + 0x2000 = 0x3000.
    write_pub32(&p, 0x2000, 1, "straddling_boundary_function");
    // PUB32 record entirely inside the second block. rva = 0x2000.
    write_pub32(&p, 0x1000, 1, "second_block_function");

    MemCopy(&xblob[X_SYM_PAGE0 * X_BLOCK_SIZE], sym_flat, X_BLOCK_SIZE);
    MemCopy(&xblob[X_SYM_PAGE1 * X_BLOCK_SIZE], sym_flat + X_BLOCK_SIZE, X_SYM_SIZE - X_BLOCK_SIZE);
}

// Contract: a SymRecord stream spanning two NON-ADJACENT blocks is read
// back byte-exact across the 512-byte boundary, so a function record
// whose name straddles the boundary is recovered intact.
//
// Kills: stream_read per-block copy length set to a constant (chunk=42)
// and the `offset += chunk` -> `offset -= chunk` advance (the second
// block would never be reached / bytes corrupted at the boundary).
bool test_pd4_crossblock_straddling_function(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    build_crossblock_blob();

    Pdb  pdb;
    bool ok = PdbOpenFromMemoryCopy(&pdb, xblob, sizeof(xblob), base);
    if (!ok) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    // Both functions must be present, sorted by rva.
    ok = VecLen(&pdb.functions) == 2;
    if (ok) {
        const PdbFunction *f0 = VecPtrAt(&pdb.functions, 0); // rva 0x2000
        const PdbFunction *f1 = VecPtrAt(&pdb.functions, 1); // rva 0x3000
        ok                    = ok && f0->rva == 0x2000 && ZstrCompare(f0->name, "second_block_function") == 0;
        ok                    = ok && f1->rva == 0x3000 && ZstrCompare(f1->name, "straddling_boundary_function") == 0;
    }

    // The straddling function name was assembled from bytes on BOTH
    // sides of the block boundary; resolve it to be sure.
    if (ok) {
        const PdbFunction *f = PdbResolveRva(&pdb, 0x3000);
        ok                   = f && ZstrCompare(f->name, "straddling_boundary_function") == 0;
    }

    PdbDeinit(&pdb);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Contract: the info stream's full 28-byte prefix (version, signature,
// age) plus the 16-byte GUID are decoded byte-exact from the multi-byte
// little-endian layout.
bool test_pd4_info_fields_decoded_exact(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    build_crossblock_blob();
    // Distinctive info values so every byte position is checked.
    u8 *info = &xblob[X_INFO_PAGE * X_BLOCK_SIZE];
    wr_u32(&info[0], 0x01020304); // version
    wr_u32(&info[4], 0x05060708); // signature
    wr_u32(&info[8], 0x090A0B0C); // age

    Pdb  pdb;
    bool ok = PdbOpenFromMemoryCopy(&pdb, xblob, sizeof(xblob), base);
    if (!ok) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    const PdbInfo *pi = PdbInfoStream(&pdb);
    ok                = pi->version == 0x01020304;
    ok                = ok && pi->signature == 0x05060708;
    ok                = ok && pi->age == 0x090A0B0C;
    ok                = ok && MemCompare(pi->guid, kGuid, 16) == 0;

    PdbDeinit(&pdb);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

enum {
    S_BLOCK_SIZE = 512,
    S_NUM_PAGES  = 6,
    S_BLOB_SIZE  = S_BLOCK_SIZE * S_NUM_PAGES,
    S_BLOCK_MAP  = 3,
    S_DIR_PAGE   = 4,
    S_INFO_PAGE  = 5,
    S_INFO_SIZE  = 28,
    S_DIR_BYTES  = 16,
};

static u8 sblob[S_BLOB_SIZE];

// `info_block_id` lets a caller point stream #1 at a bogus page to make
// the info-stream read fail.
static void build_info_blob(u32 info_block_id) {
    MemSet(sblob, 0, sizeof(sblob));

    MemCopy(sblob, kMagic, 32);
    wr_u32(&sblob[32], S_BLOCK_SIZE);
    wr_u32(&sblob[36], 1);
    wr_u32(&sblob[40], S_NUM_PAGES);
    wr_u32(&sblob[44], S_DIR_BYTES);
    wr_u32(&sblob[48], 0);
    wr_u32(&sblob[52], S_BLOCK_MAP);

    wr_u32(&sblob[S_BLOCK_MAP * S_BLOCK_SIZE], S_DIR_PAGE);

    u8 *dir = &sblob[S_DIR_PAGE * S_BLOCK_SIZE];
    wr_u32(&dir[0], 2);
    wr_u32(&dir[4], 0);
    wr_u32(&dir[8], S_INFO_SIZE);
    wr_u32(&dir[12], info_block_id);

    u8 *info = &sblob[S_INFO_PAGE * S_BLOCK_SIZE];
    wr_u32(&info[0], 20040203);
    wr_u32(&info[4], 0xdeadbeef);
    wr_u32(&info[8], 0x42);
    MemCopy(&info[12], kGuid, 16);
}

// Contract: when the info stream is declared >= 28 bytes but its block
// list points out of range, the info-stream read FAILS and the whole
// open is REJECTED (parse_pdb_info returns false; not a soft accept).
//
// Kills: `bool ok = false` -> `ok = 42` (initialised true). With the
// mutation, the early `break` after the failed stream_read would leave
// `ok` true and the open would wrongly succeed.
bool test_pd4_info_read_failure_rejected(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    // Point stream #1 at page 999 -- far past EOF -> block_ptr NULL ->
    // stream_read fails inside parse_pdb_info.
    build_info_blob(999);

    Pdb  pdb;
    bool open_ok = PdbOpenFromMemoryCopy(&pdb, sblob, sizeof(sblob), base);
    // Real code: rejected.
    bool ok = !open_ok;
    if (open_ok)
        PdbDeinit(&pdb);

    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Sanity: the same blob with a VALID info block id opens and decodes the
// info stream. Confirms test_pd4_info_read_failure_rejected isolates the
// read failure (not some unrelated rejection).
bool test_pd4_info_blob_opens_when_valid(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    build_info_blob(S_INFO_PAGE);

    Pdb  pdb;
    bool ok = PdbOpenFromMemoryCopy(&pdb, sblob, sizeof(sblob), base);
    if (!ok) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }
    ok = pdb.num_streams == 2 && pdb.info.version == 20040203 && pdb.info.age == 0x42 &&
         MemCompare(pdb.info.guid, kGuid, 16) == 0;

    PdbDeinit(&pdb);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Contract: a FAILED open frees everything it allocated along the way
// (PdbDeinit runs in the `fail:` path). After a late failure, no
// allocation outlives the call.
//
// Kills: removing `PdbDeinit(out)` from the fail path -> stream_dir /
// per-stream arrays / taken Buf leak.
bool test_pd4_failed_open_frees_all(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *base = ALLOCATOR_OF(&dbg);

    // Blob that parses superblock + directory (allocating stream_dir and
    // the per-stream arrays) but fails in parse_pdb_info because stream
    // #1's block id is bogus.
    build_info_blob(999);

    // Use the L-form so the taken Buf is the allocator's responsibility:
    // a copy buffer through `base`, handed to PdbOpenFromMemory.
    Buf in = BufInit(base);
    if (!BufReserve(&in, sizeof(sblob))) {
        DebugAllocatorDeinit(&dbg);
        return false;
    }
    MemCopy(BufData(&in), sblob, sizeof(sblob));
    BufResize(&in, (size)sizeof(sblob));

    Pdb  pdb;
    bool open_ok = PdbOpenFromMemory(&pdb, &in);
    // Expect failure (bogus info block id).
    bool ok = !open_ok;
    if (open_ok)
        PdbDeinit(&pdb);

    // Crux: the fail path must have freed every allocation, including the
    // taken data buffer and the per-stream arrays.
    ok = ok && DebugAllocatorLiveCount(&dbg) == 0;

    DebugAllocatorDeinit(&dbg);
    return ok;
}

// Contract: PdbDeinit frees the owned name pool (the Str holding
// function-name bytes), leaving no outstanding allocation.
//
// Kills: removing `StrDeinit(&self->name_pool)` from PdbDeinit -> the
// name-pool buffer leaks.
bool test_pd4_deinit_frees_name_pool(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *base = ALLOCATOR_OF(&dbg);

    build_crossblock_blob(); // populates name_pool with two function names

    Pdb  pdb;
    bool ok = PdbOpenFromMemoryCopy(&pdb, xblob, sizeof(xblob), base);
    if (!ok) {
        DebugAllocatorDeinit(&dbg);
        return false;
    }
    // The name pool must be non-trivial (names were stored).
    ok = VecLen(&pdb.functions) == 2;

    PdbDeinit(&pdb);

    // Everything PdbDeinit owns -- including the name pool -- must be gone.
    ok = ok && DebugAllocatorLiveCount(&dbg) == 0;

    DebugAllocatorDeinit(&dbg);
    return ok;
}

// CWD-relative (not /tmp): Windows has no /tmp, and the CWD is the
// portable writable scratch location (same convention as FileOpenTemp).
#define X_TMP_VALID "pd4_valid_pdb.bin"
#define X_TMP_JUNK  "pd4_junk_pdb.bin"

// Contract: a valid PDB written to disk opens via the path-based
// constructor and exposes the right info.
//
// Kills: `FileReadAndClose(path,&data) < 0` -> `>= 0` (a successful read
// returns >= 0; the mutant would treat every successful read as a
// failure and refuse to open).
bool test_pd4_open_valid_file(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    build_crossblock_blob();
    if (FileWriteAndClose(X_TMP_VALID, xblob, (u64)sizeof(xblob)) < 0) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    Pdb  pdb;
    bool ok = PdbOpen(&pdb, X_TMP_VALID, base);
    if (ok) {
        ok = VecLen(&pdb.functions) == 2 && pdb.info.signature == 0xfeedface;
        PdbDeinit(&pdb);
    }

    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Contract: a readable file whose CONTENTS are not a valid PDB is
// rejected -- the open returns false even though the read succeeded.
//
// Kills: `return PdbOpenFromMemory(out, &data)` having its result
// replaced by a truthy constant. The call still runs (and fails), but
// the mutant would report success on garbage content.
bool test_pd4_open_valid_path_junk_content(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    u8 junk[256];
    MemSet(junk, 0xCC, sizeof(junk)); // bad magic
    if (FileWriteAndClose(X_TMP_JUNK, junk, (u64)sizeof(junk)) < 0) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    Pdb  pdb;
    bool ok = !PdbOpen(&pdb, X_TMP_JUNK, base);

    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Contract: the first function's exact start RVA resolves to it, and an
// RVA just below the first function is unresolved. Reinforces the
// resolve path with the cross-block blob's functions.
bool test_pd4_resolve_first_and_below(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    build_crossblock_blob();

    Pdb  pdb;
    bool ok = PdbOpenFromMemoryCopy(&pdb, xblob, sizeof(xblob), base);
    if (!ok) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    const PdbFunction *first = PdbResolveRva(&pdb, 0x2000);
    ok                       = first && ZstrCompare(first->name, "second_block_function") == 0;
    ok                       = ok && PdbResolveRva(&pdb, 0x1FFF) == NULL; // below first
    // Interior of the second-block function resolves to it (greatest
    // rva <= query, not the first/only entry).
    if (ok) {
        const PdbFunction *mid = PdbResolveRva(&pdb, 0x2500);
        ok                     = mid && ZstrCompare(mid->name, "second_block_function") == 0;
    }

    PdbDeinit(&pdb);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ===========================================================================
// Mutation-hardening tests (batch 5): section-table / walk-publics survivors.
// ===========================================================================

enum {
    PG_SIZE      = 512,
    PG_COUNT     = 11,
    P5_BLOB_SIZE = PG_SIZE * PG_COUNT,
    PG_BMAP      = 3,
    PG_DIR       = 4,
    PG_INFO      = 5,
    PG_DBI       = 6,
    PG_SYM       = 7,
    PG_SEC       = 8,
    ST_INFO      = 1,
    ST_DBI       = 3,
    ST_SYM       = 4,
    ST_SEC       = 5,
    N_STREAMS    = 6,
    P5_INFO_SIZE = 28,
    DBI_SIZE     = 76,
};

typedef struct SecSpec {
    u32 virtual_size;
    u32 virtual_address;
} SecSpec;

typedef struct PubSpec {
    u32  offset;
    u16  segment;
    Zstr name;
} PubSpec;

// Build a full PDB blob into `blob` (size P5_BLOB_SIZE). `secs`/`n_secs`
// describe the SectionHdr stream; `pubs`/`n_pubs` the S_PUB32 records.
// `sec_block_id_override`: if non-zero, the section-header stream's single
// block id is set to this (out-of-file) page so stream_read fails while
// the directory size accounting stays consistent.
// `symrec_stream_override`: if non-zero, the DBI's advertised SymRecord
// stream index is set to this (used to force walk_publics to fail).
static void build_pdb(
    u8            *blob,
    const SecSpec *secs,
    u32            n_secs,
    const PubSpec *pubs,
    u32            n_pubs,
    u32            sec_block_id_override,
    u16            symrec_stream_override
) {
    MemSet(blob, 0, P5_BLOB_SIZE);

    // --- SectionHdr stream (#5) ---------------------------------------
    u8 *sec = &blob[PG_SEC * PG_SIZE];
    for (u32 i = 0; i < n_secs; ++i) {
        u8 *h = sec + i * 40;
        // name[8] left zero
        wr_u32(&h[8], secs[i].virtual_size);
        wr_u32(&h[12], secs[i].virtual_address);
    }
    u32 sec_size = n_secs * 40;

    // --- SymRecord stream (#4) ----------------------------------------
    u8 *sym = &blob[PG_SYM * PG_SIZE];
    u8 *sp  = sym;
    for (u32 i = 0; i < n_pubs; ++i) {
        u32 namelen = (u32)ZstrLen(pubs[i].name) + 1; // include NUL
        u16 rec_len = (u16)(2 + 4 + 4 + 2 + namelen);
        wr_u16(&sp[0], rec_len);
        wr_u16(&sp[2], 0x110E);                       // S_PUB32
        wr_u32(&sp[4], 0x2);                          // flags = FUNCTION
        wr_u32(&sp[8], pubs[i].offset);
        wr_u16(&sp[12], pubs[i].segment);
        MemCopy(&sp[14], pubs[i].name, namelen);
        sp += 2 + rec_len;
    }
    u32 sym_size = (u32)(sp - sym);

    // --- Directory ----------------------------------------------------
    // 4 (count) + N_STREAMS*4 (sizes) + 4 block ids * 4.
    const u32 dir_bytes = 4 + N_STREAMS * 4 + 4 * 4;

    MemCopy(blob, kMagic, 32);
    wr_u32(&blob[32], PG_SIZE);
    wr_u32(&blob[36], 1);
    wr_u32(&blob[40], PG_COUNT);
    wr_u32(&blob[44], dir_bytes);
    wr_u32(&blob[48], 0);
    wr_u32(&blob[52], PG_BMAP);

    wr_u32(&blob[PG_BMAP * PG_SIZE], PG_DIR);

    u8 *dir = &blob[PG_DIR * PG_SIZE];
    wr_u32(&dir[0], N_STREAMS);
    wr_u32(&dir[4 + 0 * 4], 0);
    wr_u32(&dir[4 + 1 * 4], P5_INFO_SIZE);
    wr_u32(&dir[4 + 2 * 4], 0);
    wr_u32(&dir[4 + 3 * 4], DBI_SIZE);
    wr_u32(&dir[4 + 4 * 4], sym_size);
    wr_u32(&dir[4 + 5 * 4], sec_size);
    u8 *bids = dir + 4 + N_STREAMS * 4;
    wr_u32(&bids[0 * 4], PG_INFO);
    wr_u32(&bids[1 * 4], PG_DBI);
    wr_u32(&bids[2 * 4], PG_SYM);
    wr_u32(&bids[3 * 4], sec_block_id_override ? sec_block_id_override : PG_SEC);

    // --- PDB Info (#1) ------------------------------------------------
    u8 *info = &blob[PG_INFO * PG_SIZE];
    wr_u32(&info[0], 20040203);
    wr_u32(&info[4], 0xfeedface);
    wr_u32(&info[8], 1);
    MemCopy(&info[12], kGuid, 16);

    // --- DBI (#3) -----------------------------------------------------
    u8 *dbi = &blob[PG_DBI * PG_SIZE];
    wr_u32(&dbi[0], 0xFFFFFFFFu);
    wr_u32(&dbi[4], 19990903);
    wr_u32(&dbi[8], 1);
    wr_u16(&dbi[20], symrec_stream_override ? symrec_stream_override : ST_SYM); // SymRecordStream
    wr_u32(&dbi[48], 12);                                                       // OptionalDbgHeaderSize
    wr_u16(&dbi[58], 0x8664);
    wr_u16(&dbi[64 + 0 * 2], 0xFFFF);
    wr_u16(&dbi[64 + 1 * 2], 0xFFFF);
    wr_u16(&dbi[64 + 2 * 2], 0xFFFF);
    wr_u16(&dbi[64 + 3 * 2], 0xFFFF);
    wr_u16(&dbi[64 + 4 * 2], 0xFFFF);
    wr_u16(&dbi[64 + 5 * 2], ST_SEC); // SectionHdr stream index

    (void)ST_INFO;
    (void)ST_DBI;
}

static u8 g_blob_m5[P5_BLOB_SIZE];

// Multi-section RVA mapping: a public in the LAST of several sections
// must resolve to that section's virtual_address + offset. Exercises the
// load_section_table -> walk_publics segment-indexing path across more
// than one section.
bool test_pf_multi_section_last_segment_rva(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    SecSpec secs[6];
    for (u32 i = 0; i < 6; ++i) {
        secs[i].virtual_size    = 0x800;
        secs[i].virtual_address = 0x1000u * (i + 1); // 0x1000 .. 0x6000
    }
    PubSpec pubs[1] = {
        {0x10, 6, "sixth"}, // rva 0x6010 (section 6)
    };
    build_pdb(g_blob_m5, secs, 6, pubs, 1, 0, 0);

    Pdb  pdb;
    bool ok = PdbOpenFromMemoryCopy(&pdb, g_blob_m5, P5_BLOB_SIZE, base);
    if (ok) {
        ok = VecLen(&pdb.functions) == 1;
        if (ok) {
            const PdbFunction *f = VecPtrAt(&pdb.functions, 0);
            ok                   = f->rva == 0x6010 && ZstrCompare(f->name, "sixth") == 0;
        }
        PdbDeinit(&pdb);
    }
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Site 3 (482) read_ok: keep the section stream size a clean 3*40=120
// (one block, directory accounting consistent) but point its single
// block id at an out-of-file page. block_ptr returns NULL inside
// stream_read, so read_ok = false -> load_section_table returns NULL ->
// 0 functions. Mutant read_ok=42(true) would proceed to decode the
// (never-filled) buffer and surface a bogus function instead of none.
bool test_pf_section_stream_read_failure_yields_no_functions(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    SecSpec secs[3] = {
        {0x800, 0x1000},
        {0x800, 0x2000},
        {0x800, 0x3000},
    };
    PubSpec pubs[1] = {
        {0x20, 1, "fn"},
    };
    // Section stream's block id -> page 100 (file has only 11 pages).
    build_pdb(g_blob_m5, secs, 3, pubs, 1, /*sec_block_id_override=*/100, 0);

    Pdb  pdb;
    bool ok = PdbOpenFromMemoryCopy(&pdb, g_blob_m5, P5_BLOB_SIZE, base);
    if (ok) {
        ok = VecLen(&pdb.functions) == 0;
        PdbDeinit(&pdb);
    }
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Site 5 (593) cxx_assign_const `cur = next` -> `cur = 42` in the
// RVA-overflow skip path. First public's section VA + offset overflows
// u32 (rva64 > 0xFFFFFFFF) so it is skipped via `cur = next`, followed
// by a valid public. Real: 1 function ("good") at the right rva. Mutant:
// cur=42 misaligns the cursor so the second record is not parsed at its
// true boundary -> wrong count/name.
bool test_pf_rva_overflow_skip_advances_cursor(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    SecSpec secs[2] = {
        {0x1000, 0xFFFFFF00u}, // section 1: VA near u32 max
        {0x1000,      0x1000}, // section 2: normal
    };
    PubSpec pubs[2] = {
        {0x200, 1, "overflow_me"}, // 0xFFFFFF00 + 0x200 > 0xFFFFFFFF -> skipped
        { 0x40, 2,        "good"}, // rva 0x1040
    };
    build_pdb(g_blob_m5, secs, 2, pubs, 2, 0, 0);

    Pdb  pdb;
    bool ok = PdbOpenFromMemoryCopy(&pdb, g_blob_m5, P5_BLOB_SIZE, base);
    if (ok) {
        ok = VecLen(&pdb.functions) == 1;
        if (ok) {
            const PdbFunction *f = VecPtrAt(&pdb.functions, 0);
            ok                   = f->rva == 0x1040 && ZstrCompare(f->name, "good") == 0;
        }
        PdbDeinit(&pdb);
    }
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Site 8 (671) cxx_lt_to_ge `i + 1 < VecLen` -> `>=`. The guard gates
// the next-rva size fill. With three sorted functions, the first two are
// non-last; their size must equal next.rva - this.rva. The `>=` mutant
// leaves non-last sizes 0. Assert the exact sizes.
bool test_pf_function_sizes_from_next_rva(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    SecSpec secs[1] = {
        {0x4000, 0x1000},
    };
    PubSpec pubs[3] = {
        {0x300, 1, "gamma"}, // rva 0x1300
        {0x100, 1, "alpha"}, // rva 0x1100
        {0x200, 1,  "beta"}, // rva 0x1200
    };
    build_pdb(g_blob_m5, secs, 1, pubs, 3, 0, 0);

    Pdb  pdb;
    bool ok = PdbOpenFromMemoryCopy(&pdb, g_blob_m5, P5_BLOB_SIZE, base);
    if (ok) {
        ok = VecLen(&pdb.functions) == 3;
        if (ok) {
            const PdbFunction *a = VecPtrAt(&pdb.functions, 0);            // alpha 0x1100
            const PdbFunction *b = VecPtrAt(&pdb.functions, 1);            // beta  0x1200
            const PdbFunction *c = VecPtrAt(&pdb.functions, 2);            // gamma 0x1300
            ok                   = ok && a->rva == 0x1100 && a->size == 0x100;
            ok                   = ok && b->rva == 0x1200 && b->size == 0x100;
            ok                   = ok && c->rva == 0x1300 && c->size == 0; // last -> 0
        }
        PdbDeinit(&pdb);
    }
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Happy-path: a valid named public must be accepted and surfaced.
bool test_pf_named_public_accepted(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    SecSpec secs[1] = {
        {0x4000, 0x1000},
    };
    PubSpec pubs[1] = {
        {0x10, 1, "named_fn"}, // rva 0x1010
    };
    build_pdb(g_blob_m5, secs, 1, pubs, 1, 0, 0);

    Pdb  pdb;
    bool ok = PdbOpenFromMemoryCopy(&pdb, g_blob_m5, P5_BLOB_SIZE, base);
    if (ok) {
        ok = VecLen(&pdb.functions) == 1;
        if (ok) {
            const PdbFunction *f = VecPtrAt(&pdb.functions, 0);
            ok                   = f->rva == 0x1010 && ZstrCompare(f->name, "named_fn") == 0;
        }
        PdbDeinit(&pdb);
    }
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Site 7 (645) cxx_init_const `bool ok = walk_publics(...)` -> `ok = 42`.
// Drive walk_publics down its failure path: advertise a SymRecord stream
// index that is out of range (>= num_streams). The section table still
// loads (section stream is valid), then walk_publics hits
// `symrec_stream >= num_streams -> return false`. Real code: ok = false
// -> parse_pdb_functions returns false -> PdbOpenFromMemoryCopy fails.
// Mutant ok = 42 (truthy) -> the `if (!ok)` error path is skipped, so
// the open spuriously SUCCEEDS. Assert the open is rejected.
bool test_pf_walk_publics_failure_propagates(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    SecSpec secs[1] = {
        {0x4000, 0x1000},
    };
    PubSpec pubs[1] = {
        {0x10, 1, "named_fn"},
    };
    // SymRecord stream index 99 -> out of range (num_streams == 6).
    build_pdb(g_blob_m5, secs, 1, pubs, 1, 0, /*symrec_override=*/99);

    Pdb  pdb;
    bool rejected = !PdbOpenFromMemoryCopy(&pdb, g_blob_m5, P5_BLOB_SIZE, base);
    if (!rejected)
        PdbDeinit(&pdb);
    DefaultAllocatorDeinit(&alloc);
    return rejected; // real: open rejected (walk_publics failed)
}

int main(void) {
    WriteFmt("[INFO] Starting Pdb tests\n\n");

    TestFunction tests[] = {
        test_pdb_parses_minimal_msf,
        test_pdb_rejects_bad_magic,
        test_pdb_extracts_pub32_function_name,
        test_pdb_resolves_among_multiple_functions,
        test_pdb_rejects_bad_block_size,
        test_pdb_rejects_truncated_superblock,
        test_pd1_substream_sum_locates_optdbg,
        test_pd1_symrec_index_drives_publics,
        test_pd1_optdbg_overrun_rejected,
        test_pd1_sechdr_offset_bound_rejected,
        test_pd1_sechdr_index_high_byte_oob,
        test_pd2_superblock_fields_exact,
        test_pd2_block_size_512_opens,
        test_pd2_block_size_1024_opens,
        test_pd2_block_size_2048_opens,
        test_pd2_block_size_4096_opens,
        test_pd2_block_size_256_rejected,
        test_pd2_magic_each_byte_matters,
        test_pd2_truncated_below_superblock_rejected,
        test_pd2_dir_block_id_one_past_rejected,
        test_pd2_last_block_in_bounds_opens,
        test_pd2_dir_stream_blocks_count_one,
        test_pd2_two_block_directory,
        test_pd2_stream_metadata_exact,
        test_pd2_multiblock_block_list_contiguous,
        test_pd2_single_stream,
        test_pd2_truncated_sizes_table_rejected,
        test_pd2_truncated_blockid_table_rejected,
        test_pd2_zero_streams_dir_exactly_4,
        test_pd2_sizes_table_exact_fit,
        test_pd2_blockwords_exceed_expected_opens,
        test_pd2_dir_blockmap_fills_one_page,
        test_pd3_filter_and_count,
        test_pd3_section_indexing,
        test_pd3_out_of_range_segment_dropped,
        test_pd3_function_sizes,
        test_pd3_sort_ascending,
        test_pd3_malformed_records,
        test_pd3_rva_overflow_boundary,
        test_pd4_crossblock_straddling_function,
        test_pd4_info_fields_decoded_exact,
        test_pd4_info_read_failure_rejected,
        test_pd4_info_blob_opens_when_valid,
        test_pd4_failed_open_frees_all,
        test_pd4_deinit_frees_name_pool,
        test_pd4_open_valid_file,
        test_pd4_open_valid_path_junk_content,
        test_pd4_resolve_first_and_below,
        test_pf_multi_section_last_segment_rva,
        test_pf_section_stream_read_failure_yields_no_functions,
        test_pf_rva_overflow_skip_advances_cursor,
        test_pf_function_sizes_from_next_rva,
        test_pf_named_public_accepted,
        test_pf_walk_publics_failure_propagates,
    };

    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), NULL, 0, "Pdb");
}
