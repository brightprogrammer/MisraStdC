// Mutation-hardening for `Sys/PdbCache.c`: the PDB sidecar symbol cache.
//
// Covers find_pdb (CodeView path discovery + basename fallback),
// entry_open (PE/PDB open + leak-freedom), cache_find_or_open (cache
// hit walk), PdbCacheDeinit (frees every entry), and the resolve
// overloads (RVA computation + module_base boundary).
//
// The fixture PE + PDB builders mirror Tests/Std/PdbCache.c. We include
// the unit directly so the static helpers (find_pdb, entry_open,
// cache_find_or_open) are callable here for the structural assertions
// the public API can't reach. The test executable defines the public
// symbols via the include, so the linker never pulls the library object.

#include <Misra.h>
#include <Misra/Std/Allocator/Debug.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/File.h>
#include <Misra/Std/Memory.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Sys/PdbCache.h>

#include "../Util/TestRunner.h"

// Pull in the unit under test so the static helpers are callable here.
#include "../../Source/Misra/Sys/PdbCache.c"

// -----------------------------------------------------------------------------
// Temp-path helpers (mirrors Tests/Std/PdbCache.c)
// -----------------------------------------------------------------------------

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
// Fixture blob writers (mirrors Tests/Std/PdbCache.c)
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
    File f = FileOpen(path, "w");
    if (!FileIsOpen(&f))
        return false;
    bool ok = (u64)FileWrite(&f, data, size) == size;
    FileClose(&f);
    return ok;
}

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

// Build a PE whose CodeView record points at `pdb_path`. If `pdb_path`
// is "" the CodeView pdb-path is empty (used to drive the empty-basename
// branch in find_pdb).
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

// Build a valid PDB (matching kGuid/kAge) with one function at
// `func_rva` (the section base is `sec_va`, so the in-stream symbol
// offset is `func_rva - sec_va`). If `match_guid` is false the PDB Info
// GUID is corrupted so the PE/PDB pairing check in entry_open fails
// (PdbOpen still succeeds).
static void build_pdb_blob_va(Zstr func_name, u32 sec_va, u32 func_rva, bool match_guid) {
    MemSet(pdb_blob, 0, sizeof(pdb_blob));

    u64 name_len    = ZstrLen(func_name);
    u16 rec_body    = (u16)(2 + 4 + 4 + 2 + (u16)(name_len + 1));
    u32 symrec_size = (u32)(2 + rec_body);

    const u32 dir_bytes = 4 + PDB_NUM_STREAMS * 4 + 4 * 4;

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

    u8 *info = &pdb_blob[PDB_INFO_PAGE * PDB_BLOCK_SIZE];
    wr_u32(&info[0], 20040203);
    wr_u32(&info[4], 0);
    wr_u32(&info[8], kAge);
    MemCopy(&info[12], kGuid, 16);
    if (!match_guid)
        info[12] ^= 0xFF;

    u8 *dbi = &pdb_blob[PDB_DBI_PAGE * PDB_BLOCK_SIZE];
    wr_u32(&dbi[0], 0xFFFFFFFFu);
    wr_u32(&dbi[4], 19990903);
    wr_u32(&dbi[8], kAge);
    wr_u16(&dbi[20], 4);
    wr_u32(&dbi[48], 12);
    wr_u16(&dbi[58], 0x8664);
    wr_u16(&dbi[64 + 5 * 2], 5);

    u8 *sym = &pdb_blob[PDB_SYMREC_PAGE * PDB_BLOCK_SIZE];
    wr_u16(&sym[0], rec_body);
    wr_u16(&sym[2], 0x110E);
    wr_u32(&sym[4], 0x2);
    wr_u32(&sym[8], func_rva - sec_va);
    wr_u16(&sym[12], 1);
    MemCopy(&sym[14], func_name, name_len + 1);

    u8      *sec      = &pdb_blob[PDB_SECHDR_PAGE * PDB_BLOCK_SIZE];
    const u8 sname[8] = {'.', 't', 'e', 'x', 't', 0, 0, 0};
    MemCopy(sec, sname, 8);
    wr_u32(&sec[8], 0x2000);
    wr_u32(&sec[12], sec_va);
}

static void build_pdb_blob(Zstr func_name, u32 func_rva) {
    build_pdb_blob_va(func_name, 0x1000, func_rva, true);
}

// -----------------------------------------------------------------------------
// Fixture lifecycle helper: write PE + PDB to disk at the standard paths,
// where the PE's CodeView path points exactly at the PDB on disk.
// -----------------------------------------------------------------------------

typedef struct Fixture {
    char pe_path[1024];
    char pdb_path[1024];
} Fixture;

static bool fixture_write_exact(Fixture *fx, Zstr pe_name, Zstr pdb_name) {
    tmp_path_join(fx->pe_path, sizeof(fx->pe_path), pe_name);
    tmp_path_join(fx->pdb_path, sizeof(fx->pdb_path), pdb_name);
    build_pe_blob(fx->pdb_path);
    build_pdb_blob("winproc", 0x1100);
    if (!write_file(fx->pe_path, pe_blob, sizeof(pe_blob)))
        return false;
    if (!write_file(fx->pdb_path, pdb_blob, sizeof(pdb_blob)))
        return false;
    return true;
}

static void fixture_remove(Fixture *fx) {
    FileRemove((Zstr)fx->pe_path);
    FileRemove((Zstr)fx->pdb_path);
}

// =============================================================================
// find_pdb
// =============================================================================

// Line 43 (pdb_base[0]=='\0' -> !=) and line 48 (StrLen(out_path)>0).
// Drive the basename-fallback branch: the CodeView path does NOT exist
// on disk, but its basename DOES exist next to the PE. find_pdb must
// take the fallback and return true with a non-empty path.
//
// Under `==` -> `!=`, an empty basename (which never happens here, but
// the mutant flips truthiness for the real non-empty basename) makes
// find_pdb wrongly bail out -> false. We assert it returns true.
static bool test_pc_find_pdb_basename_fallback(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *base = ALLOCATOR_OF(&dbg);

    Fixture fx;
    char    pe_path[1024];
    char    pdb_side[1024];
    tmp_path_join(pe_path, sizeof(pe_path), "misra_pcm_fb.exe");
    tmp_path_join(pdb_side, sizeof(pdb_side), "misra_pcm_fb.pdb");

    // CodeView points at a path that does NOT exist on disk but has a
    // basename "misra_pcm_fb.pdb"; the real PDB sits next to the PE.
    build_pe_blob("Z:/no/such/dir/misra_pcm_fb.pdb");
    build_pdb_blob("winproc", 0x1100);
    bool wrote = write_file(pe_path, pe_blob, sizeof(pe_blob)) && write_file(pdb_side, pdb_blob, sizeof(pdb_blob));

    bool result = false;
    if (wrote) {
        Pe pe;
        if (PeOpen(&pe, pe_path, base)) {
            Str  out   = StrInit(base);
            bool found = find_pdb(&pe, pe_path, &out);
            // Real path: basename non-empty -> fallback finds the
            // sidecar; out is a non-empty path ending in the basename.
            result = found && StrLen(&out) > 0;
            StrDeinit(&out);
            PeDeinit(&pe);
        }
    }

    FileRemove((Zstr)pe_path);
    FileRemove((Zstr)pdb_side);
    (void)fx;
    DebugAllocatorDeinit(&dbg);
    return result;
}

// find_pdb reject side: a CodeView path whose basename does NOT exist
// next to the PE (and the exact path does not exist) must return false.
// This pins the final `return false` and complements the accept case in
// test 1, so the two together fix the find_pdb truth table at line 43:
// non-empty basename + sidecar present -> true; sidecar absent -> false.
static bool test_pc_find_pdb_missing_sidecar_rejected(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *base = ALLOCATOR_OF(&dbg);

    char pe_path[1024];
    tmp_path_join(pe_path, sizeof(pe_path), "misra_pcm_miss.exe");

    // CodeView points at a path that does not exist; its basename
    // "misra_pcm_miss_absent.pdb" also has NO sidecar next to the PE.
    build_pe_blob("Z:/no/such/dir/misra_pcm_miss_absent.pdb");
    bool wrote = write_file(pe_path, pe_blob, sizeof(pe_blob));

    bool result = false;
    if (wrote) {
        Pe pe;
        if (PeOpen(&pe, pe_path, base)) {
            Str  out   = StrInit(base);
            bool found = find_pdb(&pe, pe_path, &out);
            // Non-empty basename (passes line 43), composes the
            // alongside-PE path, which does not exist -> false.
            result = !found;
            StrDeinit(&out);
            PeDeinit(&pe);
        }
    }

    FileRemove((Zstr)pe_path);
    DebugAllocatorDeinit(&dbg);
    return result;
}

// Line 48 (StrLen(out_path) > 0 -> >=): drive the empty-dirname case.
// We hand find_pdb a `pe_path` with NO separator (a bare filename
// relative to cwd). sys_append_dirname then leaves `out_path` EMPTY:
//   - `> 0`  (real)   : false -> do NOT push '/' -> composed path is the
//                       bare basename -> opens the cwd sidecar -> true.
//   - `>= 0` (mutant) : true  -> push '/' -> composed path is "/basename"
//                       (absolute) -> does not exist -> false.
// The PE and sidecar are written as bare names in cwd so the relative
// open succeeds, then removed.
static bool test_pc_find_pdb_no_dirname_boundary(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *base = ALLOCATOR_OF(&dbg);

    // Bare names (no path separator), resolved relative to cwd.
    Zstr pe_name  = "misra_pcm_nodir.exe";
    Zstr pdb_name = "misra_pcm_nodir.pdb";

    // CodeView path absent on disk -> fall back to the basename
    // "misra_pcm_nodir.pdb" which we write in cwd.
    build_pe_blob("Z:/no/such/dir/misra_pcm_nodir.pdb");
    build_pdb_blob("winproc", 0x1100);
    bool wrote = write_file(pe_name, pe_blob, sizeof(pe_blob)) && write_file(pdb_name, pdb_blob, sizeof(pdb_blob));

    bool result = false;
    if (wrote) {
        Pe pe;
        if (PeOpen(&pe, pe_name, base)) {
            Str  out   = StrInit(base);
            bool found = find_pdb(&pe, pe_name, &out);
            // Real `> 0`: out_path is exactly the bare basename and
            // resolves; mutant `>= 0` prepends '/' and fails.
            result = found && StrLen(&out) > 0 && StrBegin(&out)[0] != '/';
            StrDeinit(&out);
            PeDeinit(&pe);
        }
    }

    FileRemove(pe_name);
    FileRemove(pdb_name);
    DebugAllocatorDeinit(&dbg);
    return result;
}

// =============================================================================
// entry_open
// =============================================================================

// Line 71 (StrDeinit removal on the find_pdb-failure branch): force
// find_pdb to FAIL inside entry_open (CodeView path absent, no sidecar
// next to the PE) and assert no allocation leaks. The dropped StrDeinit
// leaves `pdb_path` allocated -> live count > baseline.
static bool test_pc_entry_open_failure_no_leak(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *base = ALLOCATOR_OF(&dbg);

    char pe_path[1024];
    tmp_path_join(pe_path, sizeof(pe_path), "misra_pcm_noleak.exe");

    // CodeView points at a long, definitely-non-existent path so
    // find_pdb allocates `out_path` then fails (no sidecar on disk).
    build_pe_blob("Z:/no/such/dir/misra_pcm_noleak_sidecar_file.pdb");
    bool wrote = write_file(pe_path, pe_blob, sizeof(pe_blob));

    bool result = false;
    if (wrote) {
        PdbCacheEntry entry;
        MemSet(&entry, 0, sizeof(entry));
        bool inited = StrTryInitFromCstr(&entry.module_path, pe_path, ZstrLen(pe_path), base);
        if (inited) {
            size before = DebugAllocatorLiveCount(&dbg);
            bool opened = entry_open(&entry, base);
            // find_pdb fails -> entry_open returns false; pdb_path must
            // have been freed (line 71). PE itself stays open in entry.
            if (!opened) {
                if (entry.pe_open)
                    PeDeinit(&entry.pe);
                size after = DebugAllocatorLiveCount(&dbg);
                result     = (after <= before);
            }
            StrDeinit(&entry.module_path);
        }
    }

    FileRemove((Zstr)pe_path);
    DebugAllocatorDeinit(&dbg);
    return result;
}

// Line 74 (bool ok = PdbOpen(...) -> 42): drive a reachable PdbOpen
// FAILURE. The sidecar found next to the PE is NOT a valid PDB (garbage
// bytes), so real PdbOpen returns false and entry_open returns false.
// Under value-substitution `ok = 42` (truthy) the open is wrongly
// treated as success -> entry_open proceeds to the GUID check on an
// uninitialized pdb -> behavior diverges. We assert entry_open fails and
// pdb_open stays false.
static bool test_pc_entry_open_bad_pdb_fails(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *base = ALLOCATOR_OF(&dbg);

    char pe_path[1024];
    char pdb_side[1024];
    tmp_path_join(pe_path, sizeof(pe_path), "misra_pcm_badpdb.exe");
    tmp_path_join(pdb_side, sizeof(pdb_side), "misra_pcm_badpdb.pdb");

    // CodeView path absent -> fallback to basename next to the PE, which
    // exists but is NOT a valid PDB.
    build_pe_blob("Z:/no/such/dir/misra_pcm_badpdb.pdb");
    static const u8 garbage[64] = {0};
    bool wrote = write_file(pe_path, pe_blob, sizeof(pe_blob)) && write_file(pdb_side, garbage, sizeof(garbage));

    bool result = false;
    if (wrote) {
        PdbCacheEntry entry;
        MemSet(&entry, 0, sizeof(entry));
        bool inited = StrTryInitFromCstr(&entry.module_path, pe_path, ZstrLen(pe_path), base);
        if (inited) {
            bool opened = entry_open(&entry, base);
            result      = (!opened) && (!entry.pdb_open);
            if (entry.pe_open)
                PeDeinit(&entry.pe);
            StrDeinit(&entry.module_path);
        }
    }

    FileRemove((Zstr)pe_path);
    FileRemove((Zstr)pdb_side);
    DebugAllocatorDeinit(&dbg);
    return result;
}

// =============================================================================
// cache_find_or_open
// =============================================================================

// Line 94 (++i -> --i in the find loop): resolve the SAME module twice.
// The second resolve must HIT the existing entry (not append a new one),
// so VecLen stays 1. With `--i` the loop index underflows / never
// matches, so a duplicate entry is appended -> VecLen grows to 2.
static bool test_pc_cache_hit_no_duplicate_entry(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *base = ALLOCATOR_OF(&dbg);

    Fixture fx;
    bool    wrote = fixture_write_exact(&fx, "misra_pcm_hit.exe", "misra_pcm_hit.pdb");

    bool result = false;
    if (wrote) {
        PdbCache cache = PdbCacheInit(base);

        const u64 mbase = 0x140000000ull;
        Zstr      name1 = NULL, name2 = NULL;
        u32       off = 0;

        bool r1          = PdbCacheResolve(&cache, (Zstr)fx.pe_path, mbase, mbase + 0x1100, &name1, &off);
        size after_first = VecLen(&cache.entries);

        bool r2           = PdbCacheResolve(&cache, (Zstr)fx.pe_path, mbase, mbase + 0x1110, &name2, &off);
        size after_second = VecLen(&cache.entries);

        // Same module twice -> one entry only; second resolve is a hit.
        result = r1 && r2 && after_first == 1 && after_second == 1;

        PdbCacheDeinit(&cache);
    }

    fixture_remove(&fx);
    DebugAllocatorDeinit(&dbg);
    return result;
}

// Line 94 boundary, index>=1 form: open module A then module B, then
// re-resolve B (which lives at index 1). The find loop must walk past
// index 0 to match index 1, keeping VecLen at 2. Under `++i -> --i` the
// loop checks only index 0 (i underflows to SIZE_MAX and the guard
// exits), never matches B, and appends a duplicate -> VecLen grows to 3.
static bool test_pc_cache_hit_with_second_module(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *base = ALLOCATOR_OF(&dbg);

    Fixture fa, fb;
    bool    wrote = fixture_write_exact(&fa, "misra_pcm_A.exe", "misra_pcm_A.pdb") &&
                 fixture_write_exact(&fb, "misra_pcm_B.exe", "misra_pcm_B.pdb");

    bool result = false;
    if (wrote) {
        PdbCache  cache = PdbCacheInit(base);
        const u64 mbase = 0x140000000ull;
        Zstr      name  = NULL;
        u32       off   = 0;

        bool rA = PdbCacheResolve(&cache, (Zstr)fa.pe_path, mbase, mbase + 0x1100, &name, &off);
        bool rB = PdbCacheResolve(&cache, (Zstr)fb.pe_path, mbase, mbase + 0x1100, &name, &off);
        size n2 = VecLen(&cache.entries);
        // Re-resolve B, which sits at index 1: forces the find loop to
        // walk past index 0.
        bool rB2 = PdbCacheResolve(&cache, (Zstr)fb.pe_path, mbase, mbase + 0x1108, &name, &off);
        size n3  = VecLen(&cache.entries);

        // Two distinct modules -> 2 entries; re-resolving B must hit the
        // existing index-1 entry, not append a third.
        result = rA && rB && rB2 && n2 == 2 && n3 == 2;

        PdbCacheDeinit(&cache);
    }

    fixture_remove(&fa);
    fixture_remove(&fb);
    DebugAllocatorDeinit(&dbg);
    return result;
}

// =============================================================================
// PdbCacheDeinit
// =============================================================================

// Line 121 (i < VecLen -> i >= VecLen): populate the cache with one
// fully-opened entry, then Deinit must free PE + PDB + module_path,
// returning live allocations to the pre-cache baseline. `>=` skips the
// loop body (0 >= 1 false) -> entries never freed -> leak.
static bool test_pc_deinit_frees_all_entries(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *base = ALLOCATOR_OF(&dbg);

    Fixture fx;
    bool    wrote = fixture_write_exact(&fx, "misra_pcm_deinit.exe", "misra_pcm_deinit.pdb");

    bool result = false;
    if (wrote) {
        size before = DebugAllocatorLiveCount(&dbg);

        PdbCache  cache = PdbCacheInit(base);
        const u64 mbase = 0x140000000ull;
        Zstr      name  = NULL;
        u32       off   = 0;
        bool      r     = PdbCacheResolve(&cache, (Zstr)fx.pe_path, mbase, mbase + 0x1100, &name, &off);
        bool      grew  = VecLen(&cache.entries) == 1 && DebugAllocatorLiveCount(&dbg) > before;

        PdbCacheDeinit(&cache);
        size after = DebugAllocatorLiveCount(&dbg);

        // After Deinit every cached allocation is released.
        result = r && grew && after == before;
    }

    fixture_remove(&fx);
    DebugAllocatorDeinit(&dbg);
    return result;
}

// =============================================================================
// pdb_cache_resolve_zstr
// =============================================================================

// Line 147 (entry->module_base = module_base -> = 42): module_base is
// READ at line 152/154 to compute the RVA. With `= 42` the stored base
// is unused for the RVA (the local `module_base` param is still used at
// 152/154, so... ) -- module_base is the PARAMETER, not entry->. The RVA
// uses the parameter directly, so entry->module_base is currently
// write-only w.r.t. resolve. See ignore proof; this asserts a correct
// resolve as a guard but does not by itself kill 147.
static bool test_pc_resolve_correct_symbol(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *base = ALLOCATOR_OF(&dbg);

    Fixture fx;
    bool    wrote = fixture_write_exact(&fx, "misra_pcm_sym.exe", "misra_pcm_sym.pdb");

    bool result = false;
    if (wrote) {
        PdbCache  cache = PdbCacheInit(base);
        const u64 mbase = 0x140000000ull;
        Zstr      name  = NULL;
        u32       off   = 0;
        bool      r     = PdbCacheResolve(&cache, (Zstr)fx.pe_path, mbase, mbase + 0x1100, &name, &off);
        result          = r && name && ZstrCompare(name, "winproc") == 0 && off == 0;
        PdbCacheDeinit(&cache);
    }

    fixture_remove(&fx);
    DebugAllocatorDeinit(&dbg);
    return result;
}

// Line 152 (runtime_ip < module_base -> <=): resolve at the EXACT
// boundary ip == module_base, with a function placed at RVA 0 so the
// boundary outcome is observable:
//   - `<`  : (base) < (base) is false -> proceeds -> RVA 0 -> resolves
//            the function at RVA 0 -> returns true.
//   - `<=` : (base) <= (base) is true -> returns false (rejected).
// We assert the resolve at ip == module_base SUCCEEDS (real `<`).
static bool test_pc_resolve_at_module_base_boundary(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *base = ALLOCATOR_OF(&dbg);

    char pe_path[1024];
    char pdb_path[1024];
    tmp_path_join(pe_path, sizeof(pe_path), "misra_pcm_bound.exe");
    tmp_path_join(pdb_path, sizeof(pdb_path), "misra_pcm_bound.pdb");

    build_pe_blob(pdb_path);
    // Section VA 0, function at RVA 0 -> ip == module_base maps to RVA 0.
    build_pdb_blob_va("winzero", 0, 0, true);
    bool wrote = write_file(pe_path, pe_blob, sizeof(pe_blob)) && write_file(pdb_path, pdb_blob, sizeof(pdb_blob));

    bool result = false;
    if (wrote) {
        PdbCache  cache = PdbCacheInit(base);
        const u64 mbase = 0x140000000ull;
        u32       off   = 0;

        // (a) ip exactly at module_base -> RVA 0 -> resolves under real
        //     `<`, rejected under `<=`.
        Zstr nat = NULL;
        bool at  = PdbCacheResolve(&cache, (Zstr)pe_path, mbase, mbase, &nat, &off);

        // (b) ip strictly below module_base -> rejected under both (also
        //     guards the RVA-underflow path).
        Zstr nbelow = NULL;
        bool below  = PdbCacheResolve(&cache, (Zstr)pe_path, mbase, mbase - 1, &nbelow, &off);

        result = at && nat && ZstrCompare(nat, "winzero") == 0 && off == 0 && !below;

        PdbCacheDeinit(&cache);
    }

    FileRemove((Zstr)pe_path);
    FileRemove((Zstr)pdb_path);
    DebugAllocatorDeinit(&dbg);
    return result;
}

// =============================================================================
// pdb_cache_resolve_str
// =============================================================================

// Line 179 (replace_scalar_call -> 42): the Str overload must delegate to
// _zstr and resolve identically. `replace_scalar_call` substitutes the
// call result with 42 (truthy) -- a real FAILURE case makes the mutant
// diverge. Assert the Str overload returns the same TRUE result and name
// as the Zstr overload, AND that a rejected case returns false.
static bool test_pc_resolve_str_matches_zstr(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *base = ALLOCATOR_OF(&dbg);

    Fixture fx;
    bool    wrote = fixture_write_exact(&fx, "misra_pcm_str.exe", "misra_pcm_str.pdb");

    bool result = false;
    if (wrote) {
        PdbCache  cache = PdbCacheInit(base);
        const u64 mbase = 0x140000000ull;

        Str mod = StrInit(base);
        StrPushBackMany(&mod, fx.pe_path);

        Zstr name = NULL;
        u32  off  = 0;
        bool ok   = pdb_cache_resolve_str(&cache, &mod, mbase, mbase + 0x1100, &name, &off);

        // Rejected case: ip below base -> the Str overload must return
        // false (kills the truthy-42 substitution of the delegated call).
        Zstr name2 = NULL;
        bool rej   = pdb_cache_resolve_str(&cache, &mod, mbase, mbase - 1, &name2, &off);

        result = ok && name && ZstrCompare(name, "winproc") == 0 && off == 0 && !rej;

        StrDeinit(&mod);
        PdbCacheDeinit(&cache);
    }

    fixture_remove(&fx);
    DebugAllocatorDeinit(&dbg);
    return result;
}

// =============================================================================

int main(void) {
    WriteFmt("[INFO] Starting PdbCache.Mutants1 tests\n\n");

    TestFunction tests[] = {
        test_pc_find_pdb_basename_fallback,
        test_pc_find_pdb_missing_sidecar_rejected,
        test_pc_find_pdb_no_dirname_boundary,
        test_pc_entry_open_failure_no_leak,
        test_pc_entry_open_bad_pdb_fails,
        test_pc_cache_hit_no_duplicate_entry,
        test_pc_cache_hit_with_second_module,
        test_pc_deinit_frees_all_entries,
        test_pc_resolve_correct_symbol,
        test_pc_resolve_at_module_base_boundary,
        test_pc_resolve_str_matches_zstr,
    };

    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), NULL, 0, "PdbCache.Internal");
}
