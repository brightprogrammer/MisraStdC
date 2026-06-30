// PdbCache end-to-end test. Builds matching PE + PDB blobs in
// memory, writes them to temp files, then asks the cache to resolve
// a runtime IP through the chain:
//
//   ip -> PE.codeview -> PDB on disk -> Pdb.functions -> name
//
// This is the same chain the Windows Backtrace path would run, minus
// the OS calls that find `(module_path, module_base)` from a raw IP.

#include <Misra.h>
#include <Misra/Std/Allocator/Budget.h>
#include <Misra/Std/Allocator/Debug.h>
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

// Build a valid PDB (matching kGuid/kAge) with one function at `func_rva`.
// The section base is `sec_va`, so the in-stream symbol offset is
// `func_rva - sec_va`. When `match_guid` is false the PDB Info GUID is
// corrupted so the (GUID, age) pairing check in the cache rejects the PDB
// even though PdbOpen itself still succeeds.
static void build_pdb_blob_va(Zstr func_name, u32 sec_va, u32 func_rva, bool match_guid) {
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
    if (!match_guid)
        info[12] ^= 0xFF;

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
    // Offset within section = func_rva - section.VirtualAddress (sec_va).
    wr_u32(&sym[8], func_rva - sec_va);
    wr_u16(&sym[12], 1);
    MemCopy(&sym[14], func_name, name_len + 1);

    // SectionHdr stream: one IMAGE_SECTION_HEADER with VA=sec_va.
    u8      *sec      = &pdb_blob[PDB_SECHDR_PAGE * PDB_BLOCK_SIZE];
    const u8 sname[8] = {'.', 't', 'e', 'x', 't', 0, 0, 0};
    MemCopy(sec, sname, 8);
    wr_u32(&sec[8], 0x2000);
    wr_u32(&sec[12], sec_va);
}

// Standard PDB: one function in a `.text` section based at RVA 0x1000,
// GUID/age matching the PE built by `build_pe_blob`.
static void build_pdb_blob(Zstr func_name, u32 func_rva) {
    build_pdb_blob_va(func_name, 0x1000, func_rva, true);
}

// Write a matching PE+PDB pair to disk. The PE's CodeView record points at
// `cv_path`: pass the on-disk `pdb_path` for the exact-path case, or a bogus
// path whose basename equals the sidecar to exercise the basename fallback.
static bool write_pe_pdb(Zstr pe_path, Zstr pdb_path, Zstr cv_path) {
    build_pe_blob(cv_path);
    build_pdb_blob("winproc", 0x1100);
    return write_file(pe_path, pe_blob, sizeof(pe_blob)) && write_file(pdb_path, pdb_blob, sizeof(pdb_blob));
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
    Zstr      name        = NULL;
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
    Zstr     name  = NULL;
    bool     ok    = !PdbCacheResolve(&cache, (Zstr)missing, 0, 0x1000, &name, NULL);
    PdbCacheDeinit(&cache);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// -----------------------------------------------------------------------------
// Basename fallback: the CodeView path is absent on disk, but a sidecar with
// the same basename sits next to the PE. The cache must take the fallback and
// still resolve. The PE+PDB are written under bare names in the cwd so the PE
// path carries NO directory separator: that drives the empty-dirname branch in
// the fallback (the composed path is the bare basename, not "/basename"), which
// the exact-path-only and absolute-path mutants both get wrong.
// -----------------------------------------------------------------------------
bool test_pdb_cache_resolves_via_basename_fallback(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    // Bare names (no separator), resolved relative to cwd.
    Zstr pe_name  = "misra_pdbcache_fb.exe";
    Zstr pdb_name = "misra_pdbcache_fb.pdb";

    // CodeView points at a path that does NOT exist; its basename matches the
    // sidecar we drop next to the PE.
    bool wrote = write_pe_pdb(pe_name, pdb_name, "Z:/no/such/dir/misra_pdbcache_fb.pdb");

    bool ok = false;
    if (wrote) {
        PdbCache  cache = PdbCacheInit(base);
        const u64 mbase = 0x140000000ull;
        Zstr      name  = NULL;
        u32       off   = 0;
        ok              = PdbCacheResolve(&cache, pe_name, mbase, mbase + 0x1100, &name, &off);
        ok              = ok && name && ZstrCompare(name, "winproc") == 0 && off == 0;
        PdbCacheDeinit(&cache);
    }

    FileRemove(pe_name);
    FileRemove(pdb_name);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// -----------------------------------------------------------------------------
// Leak-freedom when no PDB is found: the PE opens but neither the CodeView path
// nor a sidecar basename exists, so the lookup allocates a candidate path then
// bails. A DebugAllocator confirms every allocation is released by teardown
// (the dropped cleanup on the lookup-failure branch would leak the candidate).
// -----------------------------------------------------------------------------
bool test_pdb_cache_missing_pdb_no_leak(void) {
    DebugAllocator alloc    = DebugAllocatorInit();
    Allocator     *base     = ALLOCATOR_OF(&alloc);
    size           baseline = DebugAllocatorLiveCount(&alloc);

    char pe_path[1024];
    tmp_path_join(pe_path, sizeof(pe_path), "misra_pdbcache_noleak.exe");

    // CodeView names a non-existent path whose basename has no sidecar.
    build_pe_blob("Z:/no/such/dir/misra_pdbcache_noleak_absent.pdb");
    bool wrote = write_file(pe_path, pe_blob, sizeof(pe_blob));

    bool ok = false;
    if (wrote) {
        PdbCache cache = PdbCacheInit(base);
        Zstr     name  = NULL;
        // PE opens, PDB lookup fails -> resolve returns false.
        ok = !PdbCacheResolve(&cache, (Zstr)pe_path, 0x140000000ull, 0x140001100ull, &name, NULL);
        PdbCacheDeinit(&cache);
        // Teardown returns every allocation (the failed-lookup candidate too).
        ok = ok && DebugAllocatorLiveCount(&alloc) == baseline;
    }

    FileRemove((Zstr)pe_path);
    DebugAllocatorDeinit(&alloc);
    return ok;
}

// -----------------------------------------------------------------------------
// GUID/age mismatch is rejected without leaking the opened PDB. The sidecar is
// a valid PDB so PdbOpen succeeds, but its Info GUID disagrees with the PE's
// CodeView record. The cache must reject the pairing AND release the PDB it
// briefly opened; a DebugAllocator pins the leak-freedom (the dropped PdbDeinit
// on the mismatch branch leaks the PDB's parsed tables).
// -----------------------------------------------------------------------------
bool test_pdb_cache_guid_mismatch_no_leak(void) {
    DebugAllocator alloc    = DebugAllocatorInit();
    Allocator     *base     = ALLOCATOR_OF(&alloc);
    size           baseline = DebugAllocatorLiveCount(&alloc);

    char pe_path[1024];
    char pdb_path[1024];
    tmp_path_join(pe_path, sizeof(pe_path), "misra_pdbcache_mismatch.exe");
    tmp_path_join(pdb_path, sizeof(pdb_path), "misra_pdbcache_mismatch.pdb");

    build_pe_blob(pdb_path);
    // Valid PDB but with a corrupted Info GUID -> PdbOpen succeeds, pairing fails.
    build_pdb_blob_va("winproc", 0x1000, 0x1100, false);
    bool wrote = write_file(pe_path, pe_blob, sizeof(pe_blob)) && write_file(pdb_path, pdb_blob, sizeof(pdb_blob));

    bool ok = false;
    if (wrote) {
        PdbCache cache = PdbCacheInit(base);
        Zstr     name  = NULL;
        // PDB opens but GUID/age disagree -> resolve rejects it.
        ok = !PdbCacheResolve(&cache, (Zstr)pe_path, 0x140000000ull, 0x140001100ull, &name, NULL);
        PdbCacheDeinit(&cache);
        // The briefly-opened, then-rejected PDB must be freed.
        ok = ok && DebugAllocatorLiveCount(&alloc) == baseline;
    }

    FileRemove((Zstr)pe_path);
    FileRemove((Zstr)pdb_path);
    DebugAllocatorDeinit(&alloc);
    return ok;
}

// -----------------------------------------------------------------------------
// A second distinct module is cached at its own slot, and re-resolving it hits
// that slot instead of re-opening. We resolve module A, then B (B lands behind
// A in the entry list), snapshot the live-allocation count, then re-resolve B.
// A genuine cache hit allocates nothing; a broken find-loop that fails to walk
// past A would append a duplicate entry and re-open B's PE+PDB -> the live count
// jumps. We assert the re-resolution is correct AND allocates nothing.
// -----------------------------------------------------------------------------
bool test_pdb_cache_second_module_hits_cache(void) {
    DebugAllocator alloc = DebugAllocatorInit();
    Allocator     *base  = ALLOCATOR_OF(&alloc);

    char a_pe[1024], a_pdb[1024], b_pe[1024], b_pdb[1024];
    tmp_path_join(a_pe, sizeof(a_pe), "misra_pdbcache_A.exe");
    tmp_path_join(a_pdb, sizeof(a_pdb), "misra_pdbcache_A.pdb");
    tmp_path_join(b_pe, sizeof(b_pe), "misra_pdbcache_B.exe");
    tmp_path_join(b_pdb, sizeof(b_pdb), "misra_pdbcache_B.pdb");

    bool wrote = write_pe_pdb(a_pe, a_pdb, a_pdb) && write_pe_pdb(b_pe, b_pdb, b_pdb);

    bool ok = false;
    if (wrote) {
        PdbCache  cache = PdbCacheInit(base);
        const u64 mbase = 0x140000000ull;
        Zstr      name  = NULL;
        u32       off   = 0;

        bool ra = PdbCacheResolve(&cache, (Zstr)a_pe, mbase, mbase + 0x1100, &name, &off);
        bool rb = PdbCacheResolve(&cache, (Zstr)b_pe, mbase, mbase + 0x1100, &name, &off);

        // Both modules now open and cached; snapshot the live allocations.
        size mid = DebugAllocatorLiveCount(&alloc);

        // Re-resolve B at a different IP: must hit B's existing slot.
        Zstr name2 = NULL;
        bool rb2   = PdbCacheResolve(&cache, (Zstr)b_pe, mbase, mbase + 0x1108, &name2, &off);
        size after = DebugAllocatorLiveCount(&alloc);

        ok = ra && rb && rb2 && name2 && ZstrCompare(name2, "winproc") == 0 && after == mid;

        PdbCacheDeinit(&cache);
    }

    FileRemove((Zstr)a_pe);
    FileRemove((Zstr)a_pdb);
    FileRemove((Zstr)b_pe);
    FileRemove((Zstr)b_pdb);
    DebugAllocatorDeinit(&alloc);
    return ok;
}

// -----------------------------------------------------------------------------
// Teardown releases every cached resource. A resolve opens a PE and a PDB into
// the single cache entry, lifting the live count above baseline; PdbCacheDeinit
// must walk the entry list and free the PE, the PDB and the path of each entry,
// returning to baseline. A skipped teardown loop, or a dropped PDB free, leaks.
// -----------------------------------------------------------------------------
bool test_pdb_cache_deinit_frees_all(void) {
    DebugAllocator alloc    = DebugAllocatorInit();
    Allocator     *base     = ALLOCATOR_OF(&alloc);
    size           baseline = DebugAllocatorLiveCount(&alloc);

    char pe_path[1024];
    char pdb_path[1024];
    tmp_path_join(pe_path, sizeof(pe_path), "misra_pdbcache_deinit.exe");
    tmp_path_join(pdb_path, sizeof(pdb_path), "misra_pdbcache_deinit.pdb");

    bool wrote = write_pe_pdb(pe_path, pdb_path, pdb_path);

    bool ok = false;
    if (wrote) {
        PdbCache  cache = PdbCacheInit(base);
        const u64 mbase = 0x140000000ull;
        Zstr      name  = NULL;
        u32       off   = 0;
        bool      r     = PdbCacheResolve(&cache, (Zstr)pe_path, mbase, mbase + 0x1100, &name, &off);

        // A populated cache with an opened PE+PDB sits strictly above baseline.
        bool grew = r && DebugAllocatorLiveCount(&alloc) > baseline;

        PdbCacheDeinit(&cache);
        // Correct teardown releases every cached allocation.
        ok = grew && DebugAllocatorLiveCount(&alloc) == baseline;
    }

    FileRemove((Zstr)pe_path);
    FileRemove((Zstr)pdb_path);
    DebugAllocatorDeinit(&alloc);
    return ok;
}

// -----------------------------------------------------------------------------
// DISPROOF (adversarial audit): the white-box PdbCache.Internal.c deleted by
// f75d0c9 killed boundary/overload mutants in pdb_cache_resolve_* that the new
// public tests do NOT reach. Each test below re-kills one such mutant through
// the PUBLIC API only, proving the lost coverage was publicly recoverable and
// was silently dropped by the conversion.
// -----------------------------------------------------------------------------

// Kills PdbCache.c:152 (`runtime_ip < module_base` -> `<=`). A function placed
// at RVA 0 (section VA 0) means ip == module_base maps to RVA 0 and MUST
// resolve: real `<` lets base==base through; the `<=` mutant rejects it.
bool test_pdb_cache_resolve_at_module_base_boundary(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    char pe_path[1024];
    char pdb_path[1024];
    tmp_path_join(pe_path, sizeof(pe_path), "misra_pdbcache_baseb.exe");
    tmp_path_join(pdb_path, sizeof(pdb_path), "misra_pdbcache_baseb.pdb");

    build_pe_blob(pdb_path);
    build_pdb_blob_va("winzero", 0, 0, true);
    bool wrote = write_file(pe_path, pe_blob, sizeof(pe_blob)) && write_file(pdb_path, pdb_blob, sizeof(pdb_blob));

    bool ok = false;
    if (wrote) {
        PdbCache  cache = PdbCacheInit(base);
        const u64 mbase = 0x140000000ull;
        Zstr      name  = NULL;
        u32       off   = 0;
        // ip exactly at module_base -> RVA 0 -> resolves under real `<`.
        ok = PdbCacheResolve(&cache, (Zstr)pe_path, mbase, mbase, &name, &off);
        ok = ok && name && ZstrCompare(name, "winzero") == 0 && off == 0;
        PdbCacheDeinit(&cache);
    }

    FileRemove((Zstr)pe_path);
    FileRemove((Zstr)pdb_path);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Kills PdbCache.c:155 (`rva64 > 0xFFFFFFFFu` -> `>=`). A function at the max
// representable RVA (0xFFFFFFFF) MUST resolve when ip - module_base ==
// 0xFFFFFFFF: real `>` admits it; the `>=` mutant wrongly rejects the max RVA.
bool test_pdb_cache_resolve_max_rva_boundary(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    char pe_path[1024];
    char pdb_path[1024];
    tmp_path_join(pe_path, sizeof(pe_path), "misra_pdbcache_maxrva.exe");
    tmp_path_join(pdb_path, sizeof(pdb_path), "misra_pdbcache_maxrva.pdb");

    build_pe_blob(pdb_path);
    build_pdb_blob_va("winmax", 0x1000, 0xFFFFFFFFu, true);
    bool wrote = write_file(pe_path, pe_blob, sizeof(pe_blob)) && write_file(pdb_path, pdb_blob, sizeof(pdb_blob));

    bool ok = false;
    if (wrote) {
        PdbCache  cache = PdbCacheInit(base);
        const u64 mbase = 0x140000000ull;
        Zstr      name  = NULL;
        u32       off   = 0;
        ok              = PdbCacheResolve(&cache, (Zstr)pe_path, mbase, mbase + 0xFFFFFFFFull, &name, &off);
        ok              = ok && name && ZstrCompare(name, "winmax") == 0 && off == 0;
        PdbCacheDeinit(&cache);
    }

    FileRemove((Zstr)pe_path);
    FileRemove((Zstr)pdb_path);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Kills PdbCache.c:179 (the `Str` overload's delegated call replaced with 42).
// Resolving a rejected case (ip below module_base) through the `Str *` arm of
// PdbCacheResolve MUST return false; the scalar-call mutant returns 42 (truthy).
bool test_pdb_cache_resolve_str_overload_rejects_below_base(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    char pe_path[1024];
    char pdb_path[1024];
    tmp_path_join(pe_path, sizeof(pe_path), "misra_pdbcache_strov.exe");
    tmp_path_join(pdb_path, sizeof(pdb_path), "misra_pdbcache_strov.pdb");

    build_pe_blob(pdb_path);
    build_pdb_blob("winproc", 0x1100);
    bool wrote = write_file(pe_path, pe_blob, sizeof(pe_blob)) && write_file(pdb_path, pdb_blob, sizeof(pdb_blob));

    bool ok = false;
    if (wrote) {
        PdbCache  cache = PdbCacheInit(base);
        const u64 mbase = 0x140000000ull;
        StrInitStack(mod, 1100) {
            StrPushBackMany(&mod, (Zstr)pe_path);
            Zstr name = NULL;
            u32  off  = 0;
            // ip strictly below module_base -> delegated resolve returns false.
            ok = !PdbCacheResolve(&cache, &mod, mbase, mbase - 1, &name, &off);
        }
        PdbCacheDeinit(&cache);
    }

    FileRemove((Zstr)pe_path);
    FileRemove((Zstr)pdb_path);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Kills PdbCache.c:47 (`sys_append_dirname(out_path, pe_path)` removed). The PE
// sits in the temp dir and the CodeView path is bogus but its basename matches
// a sidecar dropped NEXT TO the PE (in the temp dir, not the cwd). The fallback
// must compose `<dirname(pe)>/<basename>`; dropping the dirname yields a bare
// basename that resolves against the cwd and is not found -> resolve fails.
bool test_pdb_cache_basename_fallback_uses_pe_dirname(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    char pe_path[1024];
    char pdb_path[1024];
    tmp_path_join(pe_path, sizeof(pe_path), "misra_pdbcache_dir.exe");
    tmp_path_join(pdb_path, sizeof(pdb_path), "misra_pdbcache_dir.pdb");

    // CodeView path is non-existent; its basename equals the temp-dir sidecar.
    build_pe_blob("Z:/no/such/dir/misra_pdbcache_dir.pdb");
    build_pdb_blob("winproc", 0x1100);
    bool wrote = write_file(pe_path, pe_blob, sizeof(pe_blob)) && write_file(pdb_path, pdb_blob, sizeof(pdb_blob));

    bool ok = false;
    if (wrote) {
        PdbCache  cache = PdbCacheInit(base);
        const u64 mbase = 0x140000000ull;
        Zstr      name  = NULL;
        u32       off   = 0;
        ok              = PdbCacheResolve(&cache, (Zstr)pe_path, mbase, mbase + 0x1100, &name, &off);
        ok              = ok && name && ZstrCompare(name, "winproc") == 0 && off == 0;
        PdbCacheDeinit(&cache);
    }

    FileRemove((Zstr)pe_path);
    FileRemove((Zstr)pdb_path);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Kills PdbCache.c:74 (`bool ok = PdbOpen(...)` initializer replaced by a
// constant, skipping the call). A sidecar that EXISTS but is not a valid PDB
// must make resolve fail cleanly (PdbOpen returns false). The init-const mutant
// skips PdbOpen, leaving a zeroed Pdb that the GUID check then tears down ->
// divergence. A DebugAllocator also pins leak-freedom on the open-failure path.
bool test_pdb_cache_invalid_sidecar_pdb_rejected(void) {
    DebugAllocator alloc    = DebugAllocatorInit();
    Allocator     *a        = ALLOCATOR_OF(&alloc);
    size           baseline = DebugAllocatorLiveCount(&alloc);

    char pe_path[1024];
    char pdb_path[1024];
    tmp_path_join(pe_path, sizeof(pe_path), "misra_pdbcache_badpdb.exe");
    tmp_path_join(pdb_path, sizeof(pdb_path), "misra_pdbcache_badpdb.pdb");

    build_pe_blob(pdb_path);
    // A sidecar that exists on disk but is NOT a valid PDB (no MSF magic).
    u8 junk[256];
    MemSet(junk, 0xAB, sizeof(junk));
    bool wrote = write_file(pe_path, pe_blob, sizeof(pe_blob)) && write_file(pdb_path, junk, sizeof(junk));

    bool ok = false;
    if (wrote) {
        PdbCache cache = PdbCacheInit(a);
        Zstr     name  = NULL;
        ok             = !PdbCacheResolve(&cache, (Zstr)pe_path, 0x140000000ull, 0x140001100ull, &name, NULL);
        PdbCacheDeinit(&cache);
        ok = ok && DebugAllocatorLiveCount(&alloc) == baseline;
    }

    FileRemove((Zstr)pe_path);
    FileRemove((Zstr)pdb_path);
    DebugAllocatorDeinit(&alloc);
    return ok;
}

// Kills PdbCache.c:96 (`ZstrCompare(...) == 0` -> `!= 0`) in the cache-entry
// match. Two distinct modules carry DISTINCT symbol names; resolving the second
// must return ITS name. Under `!= 0` the lookup matches the wrong (first) entry
// and returns the first module's symbol -> name mismatch.
bool test_pdb_cache_distinct_modules_resolve_independently(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    char a_pe[1024], a_pdb[1024], b_pe[1024], b_pdb[1024];
    tmp_path_join(a_pe, sizeof(a_pe), "misra_pdbcache_distA.exe");
    tmp_path_join(a_pdb, sizeof(a_pdb), "misra_pdbcache_distA.pdb");
    tmp_path_join(b_pe, sizeof(b_pe), "misra_pdbcache_distB.exe");
    tmp_path_join(b_pdb, sizeof(b_pdb), "misra_pdbcache_distB.pdb");

    build_pe_blob(a_pdb);
    build_pdb_blob("aproc", 0x1100);
    bool wrote = write_file(a_pe, pe_blob, sizeof(pe_blob)) && write_file(a_pdb, pdb_blob, sizeof(pdb_blob));
    build_pe_blob(b_pdb);
    build_pdb_blob("bproc", 0x1100);
    wrote = wrote && write_file(b_pe, pe_blob, sizeof(pe_blob)) && write_file(b_pdb, pdb_blob, sizeof(pdb_blob));

    bool ok = false;
    if (wrote) {
        PdbCache  cache = PdbCacheInit(base);
        const u64 mbase = 0x140000000ull;
        Zstr      na    = NULL;
        Zstr      nb    = NULL;
        u32       off   = 0;
        bool      ra    = PdbCacheResolve(&cache, (Zstr)a_pe, mbase, mbase + 0x1100, &na, &off);
        bool      rb    = PdbCacheResolve(&cache, (Zstr)b_pe, mbase, mbase + 0x1100, &nb, &off);
        ok              = ra && rb && na && nb && ZstrCompare(na, "aproc") == 0 && ZstrCompare(nb, "bproc") == 0;
        PdbCacheDeinit(&cache);
    }

    FileRemove((Zstr)a_pe);
    FileRemove((Zstr)a_pdb);
    FileRemove((Zstr)b_pe);
    FileRemove((Zstr)b_pdb);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Kills PdbCache.c:121 (`++i` -> `--i` in the deinit entry loop). Teardown
// must free EVERY cache entry, not just the first. Two distinct modules are
// resolved into two entries (each a fully opened PE+PDB+path), lifting the live
// count above baseline; PdbCacheDeinit must walk the whole entry list back to
// baseline. The `--i` mutant frees only entry[0] then wraps `i` to SIZE_MAX and
// exits, leaking entry[1]'s PE/PDB/path -> the post-deinit live count stays
// above baseline. (A one-entry teardown test cannot see this: with a single
// entry `++i` and `--i` both free exactly entry[0].)
bool test_pdb_cache_deinit_frees_all_entries(void) {
    DebugAllocator alloc    = DebugAllocatorInit();
    Allocator     *base     = ALLOCATOR_OF(&alloc);
    size           baseline = DebugAllocatorLiveCount(&alloc);

    char a_pe[1024], a_pdb[1024], b_pe[1024], b_pdb[1024];
    tmp_path_join(a_pe, sizeof(a_pe), "misra_pdbcache_dfa_A.exe");
    tmp_path_join(a_pdb, sizeof(a_pdb), "misra_pdbcache_dfa_A.pdb");
    tmp_path_join(b_pe, sizeof(b_pe), "misra_pdbcache_dfa_B.exe");
    tmp_path_join(b_pdb, sizeof(b_pdb), "misra_pdbcache_dfa_B.pdb");

    bool wrote = write_pe_pdb(a_pe, a_pdb, a_pdb) && write_pe_pdb(b_pe, b_pdb, b_pdb);

    bool ok = false;
    if (wrote) {
        PdbCache  cache = PdbCacheInit(base);
        const u64 mbase = 0x140000000ull;
        Zstr      name  = NULL;
        u32       off   = 0;

        // Two distinct modules -> two cache entries, each opening a PE+PDB.
        bool ra = PdbCacheResolve(&cache, (Zstr)a_pe, mbase, mbase + 0x1100, &name, &off);
        bool rb = PdbCacheResolve(&cache, (Zstr)b_pe, mbase, mbase + 0x1100, &name, &off);

        // A populated two-entry cache sits strictly above baseline.
        bool grew = ra && rb && DebugAllocatorLiveCount(&alloc) > baseline;

        PdbCacheDeinit(&cache);
        // Freeing EVERY entry (not just the first) returns to baseline.
        ok = grew && DebugAllocatorLiveCount(&alloc) == baseline;
    }

    FileRemove((Zstr)a_pe);
    FileRemove((Zstr)a_pdb);
    FileRemove((Zstr)b_pe);
    FileRemove((Zstr)b_pdb);
    DebugAllocatorDeinit(&alloc);
    return ok;
}

// cache_find_or_open's VecPushBackR-failure arm must free the just-copied
// module_path. Drive it with a BudgetAllocator carved to exactly one 16-byte
// slot: the Str copy of "/x" takes the slot, the entries-Vec growth (~1 KiB)
// then fails -> the cleanup arm runs. Probe afterwards: real code freed the
// slot (probe succeeds); the mutant that drops StrDeinit leaks it (probe fails).
static bool test_pdb_pushback_failure_frees_module_path(void) {
    u64             buf[4] = {0}; // 32 bytes -> exactly one 16-byte slot
    BudgetAllocator bp     = BudgetAllocatorInit((u8 *)buf, sizeof(buf), 16);
    if (BudgetAllocatorSlotCount(&bp) != 1)
        return false;             // precondition: the targeting needs a single slot

    PdbCache cache = PdbCacheInit(&bp);
    Zstr     name  = NULL;
    if (PdbCacheResolve(&cache, (char *)"/x", 0, 0x1000, &name, NULL))
        return false; // the entry must have failed to allocate

    void *probe = AllocatorAlloc(&bp, 8, 1);
    bool  freed = (probe != NULL);
    if (probe)
        AllocatorFree(&bp, probe);

    PdbCacheDeinit(&cache);
    BudgetAllocatorDeinit(&bp);
    return freed;
}

int main(void) {
    WriteFmt("[INFO] Starting PdbCache tests\n\n");

    TestFunction tests[] = {
        test_pdb_cache_resolves_via_codeview,
        test_pdb_cache_rejects_unknown_module,
        test_pdb_cache_resolves_via_basename_fallback,
        test_pdb_cache_missing_pdb_no_leak,
        test_pdb_cache_guid_mismatch_no_leak,
        test_pdb_cache_second_module_hits_cache,
        test_pdb_cache_deinit_frees_all,
        test_pdb_cache_resolve_at_module_base_boundary,
        test_pdb_cache_resolve_max_rva_boundary,
        test_pdb_cache_resolve_str_overload_rejects_below_base,
        test_pdb_cache_basename_fallback_uses_pe_dirname,
        test_pdb_cache_invalid_sidecar_pdb_rejected,
        test_pdb_cache_distinct_modules_resolve_independently,
        test_pdb_cache_deinit_frees_all_entries,
        test_pdb_pushback_failure_frees_module_path,
    };

    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), NULL, 0, "PdbCache");
}
