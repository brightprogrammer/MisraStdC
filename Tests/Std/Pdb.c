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

    PdbFile pdb;
    bool    ok = PdbFileOpenFromMemory(&pdb, blob, sizeof(blob), base);
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

    PdbFileDeinit(&pdb);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

bool test_pdb_rejects_bad_magic(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    u8 garbage[256];
    MemSet(garbage, 0xCC, sizeof(garbage));

    PdbFile pdb;
    bool    ok = !PdbFileOpenFromMemory(&pdb, garbage, sizeof(garbage), base);

    DefaultAllocatorDeinit(&alloc);
    return ok;
}

int main(void) {
    WriteFmt("[INFO] Starting Pdb tests\n\n");

    TestFunction tests[] = {
        test_pdb_parses_minimal_msf,
        test_pdb_rejects_bad_magic,
    };

    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), NULL, 0, "Pdb");
}
