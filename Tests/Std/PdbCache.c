// PdbCache end-to-end test. Builds matching PE + PDB blobs in
// memory, writes them to temp files, then asks the cache to resolve
// a runtime IP through the chain:
//
//   ip -> PE.codeview -> PDB on disk -> Pdb.functions -> name
//
// This is the same chain the Windows Backtrace path would run, minus
// the OS calls that find `(module_path, module_base)` from a raw IP.

#include <Misra.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Std/Memory.h>
#include <Misra/Sys.h>
#include <Misra/Sys/PdbCache.h>


#include "../Util/TestRunner.h"

// Resolve the system temp directory. Windows ships TEMP / TMP env
// vars; POSIX uses TMPDIR with a /tmp fallback. Returned string is
// borrowed (env-var lifetime) or a static literal -- caller must not
// free.
static Zstr tmp_dir_path(void) {
#if PLATFORM_WINDOWS
    Zstr p = EnvGet("TEMP");
    if (p && *p)
        return p;
    p = EnvGet("TMP");
    if (p && *p)
        return p;
    return "C:/Windows/Temp";
#else
    Zstr p = EnvGet("TMPDIR");
    if (p && *p)
        return p;
    return "/tmp";
#endif
}

// Compose `<tmp_dir>/<name>` into `out`. Forward slash works on both
// POSIX and Win32 (CRT + kernel APIs accept either separator).
static void tmp_path_join(char *out, size out_size, Zstr name) {
    Zstr base    = tmp_dir_path();
    size baselen = 0;
    while (base[baselen])
        ++baselen;
    size namelen = 0;
    while (name[namelen])
        ++namelen;
    if (baselen + 1 + namelen + 1 > out_size) {
        out[0] = '\0';
        return;
    }
    MemCopy(out, base, baselen);
    out[baselen] = '/';
    MemCopy(out + baselen + 1, name, namelen);
    out[baselen + 1 + namelen] = '\0';
}

// -----------------------------------------------------------------------------
// Common helpers
// -----------------------------------------------------------------------------

static void wr_u16(u8 *p, u16 v) {
    p[0] = (u8)(v & 0xff);
    p[1] = (u8)(v >> 8);
}
static void wr_u32(u8 *p, u32 v) {
    p[0] = (u8)(v & 0xff);
    p[1] = (u8)(v >> 8);
    p[2] = (u8)(v >> 16);
    p[3] = (u8)(v >> 24);
}
static void wr_u64(u8 *p, u64 v) {
    for (int i = 0; i < 8; ++i)
        p[i] = (u8)((v >> (i * 8)) & 0xff);
}

static const u8 kGuid[16] =
    {0xde, 0xad, 0xbe, 0xef, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb};
static const u32 kAge = 7;

static bool write_file(Zstr path, const u8 *data, u64 size) {
    // Use Misra's File API so this test runs under -nostdlib too
    // (no libc fopen/fwrite/fclose).
    File f = FileOpen(path, "w");
    if (!FileIsOpen(&f))
        return false;
    bool ok = (u64)FileWrite(&f, data, size) == size;
    FileClose(&f);
    return ok;
}

// -----------------------------------------------------------------------------
// PE blob with a CodeView record pointing at `pdb_path`.
// -----------------------------------------------------------------------------

enum {
    PE_BLOB_SIZE       = 0x800,
    PE_NT_OFF          = 0x80,
    PE_FILE_HDR_OFF    = PE_NT_OFF + 4,
    PE_OPT_HDR_OFF     = PE_FILE_HDR_OFF + 20,
    PE_OPT_HDR_SIZE    = 240,
    PE_SECTION_TBL_OFF = PE_OPT_HDR_OFF + PE_OPT_HDR_SIZE,
    PE_DEBUG_RAW_OFF   = 0x400,
    PE_DEBUG_DIR_RVA   = 0x1000,
    PE_DEBUG_DIR_SIZE  = 28,
    PE_CV_RAW_OFF      = 0x420,
    PE_CV_RVA          = 0x1020,
};

static u8 pe_blob[PE_BLOB_SIZE];

static void build_pe_blob(Zstr pdb_path) {
    MemSet(pe_blob, 0, sizeof(pe_blob));

    pe_blob[0] = 'M';
    pe_blob[1] = 'Z';
    wr_u32(&pe_blob[0x3C], PE_NT_OFF);
    pe_blob[PE_NT_OFF + 0] = 'P';
    pe_blob[PE_NT_OFF + 1] = 'E';

    wr_u16(&pe_blob[PE_FILE_HDR_OFF + 0], 0x8664);
    wr_u16(&pe_blob[PE_FILE_HDR_OFF + 2], 1);
    wr_u16(&pe_blob[PE_FILE_HDR_OFF + 16], PE_OPT_HDR_SIZE);
    wr_u16(&pe_blob[PE_FILE_HDR_OFF + 18], 0x2022);

    u8 *opt = &pe_blob[PE_OPT_HDR_OFF];
    wr_u16(&opt[0], 0x20B);
    wr_u32(&opt[4], 0x100);
    wr_u32(&opt[16], 0x1000);
    wr_u32(&opt[20], 0x1000);
    wr_u64(&opt[24], 0);
    wr_u32(&opt[56], 0x10000);
    wr_u32(&opt[108], 16);
    wr_u32(&opt[112 + 6 * 8 + 0], PE_DEBUG_DIR_RVA);
    wr_u32(&opt[112 + 6 * 8 + 4], PE_DEBUG_DIR_SIZE);

    u8        *sec   = &pe_blob[PE_SECTION_TBL_OFF];
    const char nm[8] = {'.', 'd', 'e', 'b', 'u', 'g', 0, 0};
    MemCopy(sec, nm, 8);
    wr_u32(&sec[8], 0x200);
    wr_u32(&sec[12], PE_DEBUG_DIR_RVA);
    wr_u32(&sec[16], 0x100);
    wr_u32(&sec[20], PE_DEBUG_RAW_OFF);

    u8 *dbg = &pe_blob[PE_DEBUG_RAW_OFF];
    wr_u32(&dbg[12], 2);                // IMAGE_DEBUG_TYPE_CODEVIEW
    wr_u32(&dbg[16], 4 + 16 + 4 + 256); // generous SizeOfData
    wr_u32(&dbg[20], PE_CV_RVA);
    wr_u32(&dbg[24], PE_CV_RAW_OFF);

    u8 *cv = &pe_blob[PE_CV_RAW_OFF];
    cv[0]  = 'R';
    cv[1]  = 'S';
    cv[2]  = 'D';
    cv[3]  = 'S';
    MemCopy(&cv[4], kGuid, 16);
    wr_u32(&cv[20], kAge);
    u64 path_len = ZstrLen(pdb_path);
    MemCopy(&cv[24], pdb_path, path_len + 1);
}

// -----------------------------------------------------------------------------
// PDB blob: minimal MSF with PDB Info (matching kGuid/kAge), DBI, SymRec,
// SectionHdr -- same layout as the Pdb.c test, parameterized for one
// function name "winproc" at RVA 0x1100.
// -----------------------------------------------------------------------------

enum {
    PDB_BLOCK_SIZE  = 512,
    PDB_NUM_PAGES   = 11,
    PDB_BLOB_SIZE   = PDB_BLOCK_SIZE * PDB_NUM_PAGES,
    PDB_BLOCK_MAP   = 3,
    PDB_DIR_PAGE    = 4,
    PDB_INFO_PAGE   = 5,
    PDB_DBI_PAGE    = 6,
    PDB_SYMREC_PAGE = 7,
    PDB_SECHDR_PAGE = 8,
    PDB_NUM_STREAMS = 6,
    PDB_INFO_SIZE   = 28,
    PDB_DBI_SIZE    = 76,
    PDB_SECHDR_SIZE = 40,
};

static const u8 kPdbMsfMagic[32] = {'M', 'i', 'c',  'r',  'o',  's', 'o', 'f',  't',  ' ', 'C',
                                    '/', 'C', '+',  '+',  ' ',  'M', 'S', 'F',  ' ',  '7', '.',
                                    '0', '0', '\r', '\n', 0x1A, 'D', 'S', '\0', '\0', '\0'};

static u8 pdb_blob[PDB_BLOB_SIZE];

static void build_pdb_blob(Zstr func_name, u32 func_rva) {
    MemSet(pdb_blob, 0, sizeof(pdb_blob));

    // Compute S_PUB32 record size based on function name length.
    u64 name_len    = ZstrLen(func_name);
    u16 rec_body    = (u16)(2 + 4 + 4 + 2 + (u16)(name_len + 1)); // kind+flags+off+seg+name
    u32 symrec_size = (u32)(2 + rec_body);                        // includes the rec_len field itself

    // Directory size: 4 (count) + 6*4 (sizes) + 4 ids * 4 = 44.
    const u32 dir_bytes = 4 + PDB_NUM_STREAMS * 4 + 4 * 4;

    // Superblock
    MemCopy(pdb_blob, kPdbMsfMagic, 32);
    wr_u32(&pdb_blob[32], PDB_BLOCK_SIZE);
    wr_u32(&pdb_blob[36], 1);
    wr_u32(&pdb_blob[40], PDB_NUM_PAGES);
    wr_u32(&pdb_blob[44], dir_bytes);
    wr_u32(&pdb_blob[52], PDB_BLOCK_MAP);

    wr_u32(&pdb_blob[PDB_BLOCK_MAP * PDB_BLOCK_SIZE], PDB_DIR_PAGE);

    u8 *dir = &pdb_blob[PDB_DIR_PAGE * PDB_BLOCK_SIZE];
    wr_u32(&dir[0], PDB_NUM_STREAMS);
    wr_u32(&dir[4 + 0 * 4], 0);
    wr_u32(&dir[4 + 1 * 4], PDB_INFO_SIZE);
    wr_u32(&dir[4 + 2 * 4], 0);
    wr_u32(&dir[4 + 3 * 4], PDB_DBI_SIZE);
    wr_u32(&dir[4 + 4 * 4], symrec_size);
    wr_u32(&dir[4 + 5 * 4], PDB_SECHDR_SIZE);
    u8 *bids = dir + 4 + PDB_NUM_STREAMS * 4;
    wr_u32(&bids[0], PDB_INFO_PAGE);
    wr_u32(&bids[4], PDB_DBI_PAGE);
    wr_u32(&bids[8], PDB_SYMREC_PAGE);
    wr_u32(&bids[12], PDB_SECHDR_PAGE);

    // PDB Info stream
    u8 *info = &pdb_blob[PDB_INFO_PAGE * PDB_BLOCK_SIZE];
    wr_u32(&info[0], 20040203);
    wr_u32(&info[4], 0);
    wr_u32(&info[8], kAge);
    MemCopy(&info[12], kGuid, 16);

    // DBI stream
    u8 *dbi = &pdb_blob[PDB_DBI_PAGE * PDB_BLOCK_SIZE];
    wr_u32(&dbi[0], 0xFFFFFFFFu);
    wr_u32(&dbi[4], 19990903);
    wr_u32(&dbi[8], kAge);
    wr_u16(&dbi[20], 4);         // SymRecord stream
    wr_u32(&dbi[48], 12);
    wr_u16(&dbi[58], 0x8664);
    wr_u16(&dbi[64 + 5 * 2], 5); // SectionHdr stream

    // SymRecord stream: one S_PUB32 record
    u8 *sym = &pdb_blob[PDB_SYMREC_PAGE * PDB_BLOCK_SIZE];
    wr_u16(&sym[0], rec_body);
    wr_u16(&sym[2], 0x110E);
    wr_u32(&sym[4], 0x2);
    // Offset within section = func_rva - section.VirtualAddress (0x1000)
    wr_u32(&sym[8], func_rva - 0x1000);
    wr_u16(&sym[12], 1);
    MemCopy(&sym[14], func_name, name_len + 1);

    // SectionHdr stream: one IMAGE_SECTION_HEADER with VA=0x1000.
    u8      *sec      = &pdb_blob[PDB_SECHDR_PAGE * PDB_BLOCK_SIZE];
    const u8 sname[8] = {'.', 't', 'e', 'x', 't', 0, 0, 0};
    MemCopy(sec, sname, 8);
    wr_u32(&sec[8], 0x2000);
    wr_u32(&sec[12], 0x1000);
}

// -----------------------------------------------------------------------------
// Test
// -----------------------------------------------------------------------------

bool test_pdb_cache_resolves_via_codeview(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    char pe_path[1024];
    char pdb_path[1024];
    tmp_path_join(pe_path, sizeof(pe_path), "misra_pdbcache_test.exe");
    tmp_path_join(pdb_path, sizeof(pdb_path), "misra_pdbcache_test.pdb");

    build_pe_blob(pdb_path);
    build_pdb_blob("winproc", 0x1100);

    if (!write_file(pe_path, pe_blob, sizeof(pe_blob))) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }
    if (!write_file(pdb_path, pdb_blob, sizeof(pdb_blob))) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    PdbCache cache = PdbCacheInit(base);

    // module_base = 0x140000000 -> RVA = (ip - base). For func at
    // RVA 0x1100, ip = module_base + 0x1100.
    const u64 module_base = 0x140000000ull;
    const u64 ip          = module_base + 0x1100;
    Zstr name        = NULL;
    u32       offset      = 0;
    bool      ok          = PdbCacheResolve(&cache, (Zstr)pe_path, module_base, ip, &name, &offset);
    ok                    = ok && name && ZstrCompare(name, "winproc") == 0 && offset == 0;

    // Second resolution should hit the cache (not strictly verifiable
    // from outside, but at least confirm correctness is stable).
    const u64 ip2 = ip + 0x10;
    name          = NULL;
    offset        = 0;
    ok            = ok && PdbCacheResolve(&cache, (Zstr)pe_path, module_base, ip2, &name, &offset);
    ok            = ok && name && ZstrCompare(name, "winproc") == 0 && offset == 0x10;

    PdbCacheDeinit(&cache);
    DefaultAllocatorDeinit(&alloc);

    FileRemove((Zstr)pe_path);
    FileRemove((Zstr)pdb_path);
    return ok;
}

bool test_pdb_cache_rejects_unknown_module(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    char missing[1024];
    tmp_path_join(missing, sizeof(missing), "misra_pdbcache_missing_xyz.exe");

    PdbCache cache = PdbCacheInit(base);
    Zstr name = NULL;
    bool ok   = !PdbCacheResolve(&cache, (Zstr)missing, 0, 0x1000, &name, NULL);
    PdbCacheDeinit(&cache);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

int main(void) {
    WriteFmt("[INFO] Starting PdbCache tests\n\n");

    TestFunction tests[] = {
        test_pdb_cache_resolves_via_codeview,
        test_pdb_cache_rejects_unknown_module,
    };

    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), NULL, 0, "PdbCache");
}
