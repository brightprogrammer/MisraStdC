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

    Pdb pdb;
    bool    ok = PdbOpenFromMemoryCopy(&pdb, blob, sizeof(blob), base);
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

    Pdb pdb;
    bool    ok = !PdbOpenFromMemoryCopy(&pdb, garbage, sizeof(garbage), base);

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

    Pdb pdb;
    bool    ok = PdbOpenFromMemoryCopy(&pdb, fblob, sizeof(fblob), base);
    if (!ok) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    ok = pdb.functions.length == 1;
    if (ok) {
        const PdbFunction *f = &pdb.functions.data[0];
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

int main(void) {
    WriteFmt("[INFO] Starting Pdb tests\n\n");

    TestFunction tests[] = {
        test_pdb_parses_minimal_msf,
        test_pdb_rejects_bad_magic,
        test_pdb_extracts_pub32_function_name,
    };

    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), NULL, 0, "Pdb");
}
