// Blind mutation-hardening tests for Source/Misra/Parsers/Pdb.c.
//
// Each test builds a crafted MSF/PDB byte blob and drives the PUBLIC
// API (PdbOpenFromMemoryCopy / PdbResolveRva / PdbOpen). Observable
// divergence is read from the resolved functions, open success/failure,
// or the DebugAllocator's overflow counter (heap writes past an
// allocation corrupt a trailing canary, surfaced as Overflows > 0).

#include <Misra.h>
#include <Misra/Parsers/Pdb.h>
#include <Misra/Std/Allocator/Debug.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Container/Buf.h>
#include <Misra/Std/Memory.h>
#include <Misra/Std/Zstr.h>

#include "../Util/TestRunner.h"

// ---------------------------------------------------------------------------
// LE writers
// ---------------------------------------------------------------------------

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

static const u8 kMagic[32] = {'M', 'i', 'c', 'r', 'o', 's', 'o', 'f', 't',  ' ',  'C',  '/', 'C', '+',  '+',  ' ',
                              'M', 'S', 'F', ' ', '7', '.', '0', '0', '\r', '\n', 0x1A, 'D', 'S', '\0', '\0', '\0'};

static const u8 kGuid[16] =
    {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x00};

#define NIL 0xFFFFFFFFu

// ---------------------------------------------------------------------------
// Generic blob builder. Streams are laid out as: stream i has size
// sizes[i]; each non-NIL non-zero-size stream gets ceil(size/bs)
// consecutive content pages. The directory itself may span several
// pages (the block-map lists each).
// ---------------------------------------------------------------------------

typedef struct Blob {
    u8 *bytes;
    u32 len;
    u32 bs;
    u32 first_page[256];
    u32 page_count[256];
} Blob;

static u8 g_buf[4096 * 64];

static Blob build_blob(u32 bs, const u32 *sizes, u32 n, u32 dir_bytes_override) {
    Blob b;
    MemSet(&b, 0, sizeof(b));
    MemSet(g_buf, 0, sizeof(g_buf));
    b.bytes = g_buf;
    b.bs    = bs;

    const u32 block_map_page = 3;
    const u32 dir_page       = 4;

    u32 total_content = 0;
    for (u32 i = 0; i < n; ++i) {
        if (sizes[i] == 0 || sizes[i] == NIL) {
            b.page_count[i] = 0;
            continue;
        }
        u32 pc           = (sizes[i] + bs - 1) / bs;
        b.page_count[i]  = pc;
        total_content   += pc;
    }

    u32 dir_bytes      = 4 + n * 4 + total_content * 4;
    u32 num_dir_blocks = (dir_bytes + bs - 1) / bs;
    if (num_dir_blocks == 0)
        num_dir_blocks = 1;
    u32 first_data = dir_page + num_dir_blocks;
    u32 num_pages  = first_data + total_content;
    u32 adv_dir    = dir_bytes_override ? dir_bytes_override : dir_bytes;

    MemCopy(g_buf, kMagic, 32);
    wr_u32(&g_buf[32], bs);
    wr_u32(&g_buf[36], 1);
    wr_u32(&g_buf[40], num_pages);
    wr_u32(&g_buf[44], adv_dir);
    wr_u32(&g_buf[48], 0);
    wr_u32(&g_buf[52], block_map_page);

    for (u32 k = 0; k < num_dir_blocks; ++k)
        wr_u32(&g_buf[block_map_page * bs + k * 4], dir_page + k);

    // Directory pages are physically consecutive here, so the bytes the
    // reader reassembles match a single contiguous write.
    u8 *dir = &g_buf[dir_page * bs];
    wr_u32(&dir[0], n);
    for (u32 i = 0; i < n; ++i)
        wr_u32(&dir[4 + i * 4], sizes[i]);

    u8 *bids      = dir + 4 + n * 4;
    u32 bid_idx   = 0;
    u32 data_page = first_data;
    for (u32 i = 0; i < n; ++i) {
        if (sizes[i] == 0 || sizes[i] == NIL)
            continue;
        b.first_page[i] = data_page;
        for (u32 k = 0; k < b.page_count[i]; ++k) {
            wr_u32(&bids[bid_idx * 4], data_page);
            bid_idx   += 1;
            data_page += 1;
        }
    }

    b.len = num_pages * bs;
    return b;
}

static u8 *stream_page(Blob *b, u32 idx) {
    return &b->bytes[b->first_page[idx] * b->bs];
}

static void put_info(Blob *b) {
    u8 *info = stream_page(b, 1);
    wr_u32(&info[0], 20040203);
    wr_u32(&info[4], 0xfeedface);
    wr_u32(&info[8], 1);
    MemCopy(&info[12], kGuid, 16);
}

// DBI stream (#3) with all substream sizes 0 (so optdbg_off = 64) and
// optdbg_size = 12. `symrec` / `sechdr` planted in the header / optdbg.
static void put_dbi(Blob *b, u16 symrec, u16 sechdr) {
    u8 *dbi = stream_page(b, 3);
    wr_u32(&dbi[0], 0xFFFFFFFFu);
    wr_u32(&dbi[4], 19990903);
    wr_u32(&dbi[8], 1);
    wr_u16(&dbi[20], symrec);
    wr_u32(&dbi[48], 12); // OptionalDbgHeaderSize
    wr_u16(&dbi[58], 0x8664);
    wr_u16(&dbi[64 + 0 * 2], 0xFFFF);
    wr_u16(&dbi[64 + 1 * 2], 0xFFFF);
    wr_u16(&dbi[64 + 2 * 2], 0xFFFF);
    wr_u16(&dbi[64 + 3 * 2], 0xFFFF);
    wr_u16(&dbi[64 + 4 * 2], 0xFFFF);
    wr_u16(&dbi[64 + 5 * 2], sechdr);
}

static void put_section_at(u8 *sec, u32 va) {
    const u8 sname[8] = {'.', 't', 'e', 'x', 't', 0, 0, 0};
    MemCopy(sec, sname, 8);
    wr_u32(&sec[8], 0x2000);
    wr_u32(&sec[12], va);
}

static void put_section(Blob *b, u32 stream_idx, u32 va) {
    put_section_at(stream_page(b, stream_idx), va);
}

static u32 put_pub32(u8 *dst, u32 offset, u16 segment, Zstr name) {
    u32 namelen = (u32)ZstrLen(name) + 1;
    u16 rec_len = (u16)(2 + 4 + 4 + 2 + namelen);
    wr_u16(&dst[0], rec_len);
    wr_u16(&dst[2], 0x110E);
    wr_u32(&dst[4], 0x2);
    wr_u32(&dst[8], offset);
    wr_u16(&dst[12], segment);
    MemCopy(&dst[14], name, namelen);
    return 2 + rec_len;
}

static int open_count(Allocator *base, Blob *b) {
    Pdb pdb;
    if (!PdbOpenFromMemoryCopy(&pdb, b->bytes, b->len, base))
        return -1;
    int n = (int)VecLen(&pdb.functions);
    PdbDeinit(&pdb);
    return n;
}

// ===========================================================================
// Multi-block stream read (lines 121, 128) -- a SymRecord stream wider than
// one block, with a record straddling the block boundary, must be read and
// resolved intact.
// ===========================================================================
static bool test_multiblock_symrec_resolves(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    u32  sizes[6] = {0, 28, 0, 76, 600, 40};
    Blob b        = build_blob(512, sizes, 6, 0);
    put_info(&b);
    put_dbi(&b, 4, 5);
    put_section(&b, 5, 0x1000);
    static char longname[541];
    for (u32 i = 0; i < 540; ++i)
        longname[i] = (char)('A' + (i % 26));
    longname[540] = '\0';
    put_pub32(stream_page(&b, 4), 0x100, 1, longname);

    Pdb pdb;
    if (!PdbOpenFromMemoryCopy(&pdb, b.bytes, b.len, base)) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }
    bool ok = VecLen(&pdb.functions) == 1;
    if (ok) {
        const PdbFunction *f = VecPtrAt(&pdb.functions, 0);
        ok                   = f->rva == 0x1100 && ZstrCompare(f->name, longname) == 0;
    }
    PdbDeinit(&pdb);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ===========================================================================
// block_count substitution (line 115) -- a SymRecord stream deeper than 42
// blocks. With block_count forced to 42, reading the deep blocks trips
// `page >= 42` (line 121) and stream_read fails -> walk fails -> open fails.
// Real code (true block_count) reads all blocks and resolves the function.
// ===========================================================================
static bool test_deep_block_count_used(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    // SymRecord stream = 50 blocks * 512 = 25600 bytes (> 42 blocks). A
    // single PUB32 lives at the START; the walker still reads the WHOLE
    // stream (stream_read of `sz`), so blocks 42..49 must be reachable.
    enum {
        SYMSZ = 50 * 512
    };
    u32  sizes[6] = {0, 28, 0, 76, SYMSZ, 40};
    Blob b        = build_blob(512, sizes, 6, 0);
    put_info(&b);
    put_dbi(&b, 4, 5);
    put_section(&b, 5, 0x1000);
    put_pub32(stream_page(&b, 4), 0x100, 1, "deepblk");

    Pdb pdb;
    if (!PdbOpenFromMemoryCopy(&pdb, b.bytes, b.len, base)) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }
    bool ok = VecLen(&pdb.functions) == 1;
    if (ok) {
        const PdbFunction *f = VecPtrAt(&pdb.functions, 0);
        ok                   = f->rva == 0x1100 && ZstrCompare(f->name, "deepblk") == 0;
    }
    PdbDeinit(&pdb);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ===========================================================================
// Multi-block directory reconstruction (lines 219, 224) under DebugAllocator.
// A directory wider than one block with a PARTIAL last block exercises the
// tail-copy clamp. The `done + want > num_dir_bytes` -> `done - want` (219),
// `> -> <=` (219), and `num_dir_bytes - done` -> `+` (224) mutants all make
// the final MemCopy overrun stream_dir; the canary check flags it.
// ===========================================================================
static bool test_multiblock_directory_no_overrun(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *base = ALLOCATOR_OF(&dbg);

    // n=130 streams -> dir_bytes = 4 + 130*4 + 4*4 = 540 (bs=512): block 0
    // full (512), block 1 partial (28). The partial tail drives 219/224.
    enum {
        N = 130
    };
    u32 sizes[N];
    MemSet(sizes, 0, sizeof(sizes));
    sizes[1] = 28;
    sizes[3] = 76;
    sizes[4] = 26;
    sizes[5] = 40;
    Blob b   = build_blob(512, sizes, N, 0);
    put_info(&b);
    put_dbi(&b, 4, 5);
    put_section(&b, 5, 0x1000);
    put_pub32(stream_page(&b, 4), 0x100, 1, "deepfn");

    Pdb  pdb;
    bool opened = PdbOpenFromMemoryCopy(&pdb, b.bytes, b.len, base);
    bool ok     = opened && VecLen(&pdb.functions) == 1 && pdb.num_streams == N;
    if (ok) {
        const PdbFunction *f = VecPtrAt(&pdb.functions, 0);
        ok                   = f->rva == 0x1100 && ZstrCompare(f->name, "deepfn") == 0;
    }
    if (opened)
        PdbDeinit(&pdb);
    // No heap write may have overrun any allocation (canary intact).
    ok = ok && DebugAllocatorOverflows(&dbg) == 0;
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// ===========================================================================
// per-stream array allocation sizes (lines 259, 260, 261) under
// DebugAllocator. With > 10 streams the natural allocations exceed 42
// bytes; the `= 42` init makes a tiny allocation that the per-stream write
// loop overruns (stream_sizes[i] @275, stream_blocks[i] @307,
// stream_block_counts[i] @308). The canary catches the overrun.
// ===========================================================================
static bool test_many_streams_array_alloc_sized(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *base = ALLOCATOR_OF(&dbg);

    // 40 streams: 40*4 = 160 bytes (sizes/counts), 40*8 = 320 (ptrs), all
    // far past 42. Functional streams 1/3/4/5; the rest empty.
    enum {
        N = 40
    };
    u32 sizes[N];
    MemSet(sizes, 0, sizeof(sizes));
    sizes[1] = 28;
    sizes[3] = 76;
    sizes[4] = 26;
    sizes[5] = 40;
    Blob b   = build_blob(512, sizes, N, 0);
    put_info(&b);
    put_dbi(&b, 4, 5);
    put_section(&b, 5, 0x1000);
    put_pub32(stream_page(&b, 4), 0x100, 1, "fn");

    Pdb  pdb;
    bool opened = PdbOpenFromMemoryCopy(&pdb, b.bytes, b.len, base);
    bool ok     = opened && VecLen(&pdb.functions) == 1 && pdb.num_streams == N;
    if (opened)
        PdbDeinit(&pdb);
    ok = ok && DebugAllocatorOverflows(&dbg) == 0;
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// ===========================================================================
// load_section_table allocation size + loop bound (lines 470, 488) under
// DebugAllocator. Many sections -> `out` is n*sizeof(SectionRva). The
// `out_bytes = 42` init (470) and `i <= n` loop (488) both write past the
// allocation; the canary catches it. Real code resolves via section #1.
// ===========================================================================
static bool test_many_sections_no_overrun(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *base = ALLOCATOR_OF(&dbg);

    enum {
        NSEC = 32
    };
    u32  sizes[6] = {0, 28, 0, 76, 26, NSEC * 40};
    Blob b        = build_blob(512, sizes, 6, 0);
    put_info(&b);
    put_dbi(&b, 4, 5);
    u8 *sec = stream_page(&b, 5);
    for (u32 i = 0; i < NSEC; ++i) {
        const u8 sname[8] = {'.', 's', 0, 0, 0, 0, 0, 0};
        MemCopy(&sec[i * 40], sname, 8);
        wr_u32(&sec[i * 40 + 8], 0x2000);
        wr_u32(&sec[i * 40 + 12], 0x1000 + i * 0x4000);
    }
    put_pub32(stream_page(&b, 4), 0x100, 1, "fn");

    Pdb  pdb;
    bool opened = PdbOpenFromMemoryCopy(&pdb, b.bytes, b.len, base);
    bool ok     = opened && VecLen(&pdb.functions) == 1;
    if (ok) {
        const PdbFunction *f = VecPtrAt(&pdb.functions, 0);
        ok                   = f->rva == 0x1100 && ZstrCompare(f->name, "fn") == 0;
    }
    if (opened)
        PdbDeinit(&pdb);
    ok = ok && DebugAllocatorOverflows(&dbg) == 0;
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// ===========================================================================
// SymRecord stream index == num_streams (line 539). The `>` mutant lets the
// out-of-range index through; walk_publics fails on the bad stream and open
// fails. Real code rejects (>=) and would resolve normally if in range.
// ===========================================================================
static bool test_symrec_index_equals_num_streams(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    u32  sizes[6] = {0, 28, 0, 76, 26, 40};
    Blob b        = build_blob(512, sizes, 6, 0);
    put_info(&b);
    put_dbi(&b, 6, 5); // symrec index == num_streams(6)
    put_section(&b, 5, 0x1000);
    put_pub32(stream_page(&b, 4), 0x100, 1, "fn");

    int  n  = open_count(base, &b);
    bool ok = (n == -1);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ===========================================================================
// parse_dbi_header r.ok reset (line 406, `r.ok = false` -> 42 i.e. true).
//
// BufReadFmt sets r.symrec_stream before the optdbg bounds check; r.ok is
// reset to false at 406, then the optdbg-overrun check (line 421) returns
// `r` early WITHOUT setting r.section_hdr_stream (it stays 0). Real code:
// dbi.ok == false -> parse_pdb_functions leaves functions empty. The mutant
// leaves dbi.ok truthy, so it proceeds using section_hdr_stream == 0. We
// plant a VALID section table at stream #0 and a valid PUB32 at the
// (already-read) symrec stream, so the mutant resolves 1 function while
// real code resolves 0.
// ===========================================================================
static bool test_dbi_ok_reset_on_optdbg_overrun(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    // Stream 0 is a VALID 40-byte section table (the mutant's wrong
    // section_hdr_stream == 0 would read it). Stream 4 is the symrec.
    u32  sizes[6] = {40, 28, 0, 76, 26, 40};
    Blob b        = build_blob(512, sizes, 6, 0);
    put_info(&b);
    put_dbi(&b, 4, 5);
    // Section table at BOTH stream 0 and stream 5 (VA 0x1000 each).
    put_section(&b, 0, 0x1000);
    put_section(&b, 5, 0x1000);
    put_pub32(stream_page(&b, 4), 0x100, 1, "fn");
    // Force the optdbg-overrun reject (line 421): optdbg_size = 60,
    // optdbg_off = 64, dbi_size = 76 -> 124 > 76.
    wr_u32(&stream_page(&b, 3)[48], 60);

    int  n  = open_count(base, &b);
    bool ok = (n == 0); // real: DBI rejected -> 0 functions.
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ===========================================================================
// walk_publics name NUL-search end boundary (line 600, `p < end` -> `<=`).
//
// A PUB32 whose name has NO NUL inside [cur+14, next) but a 0 byte exactly
// at buf[next]. real `<`: no NUL -> name rejected -> 0 funcs. mutant `<=`:
// reads buf[next]==0 -> name accepted -> 1 func.
// ===========================================================================
static bool test_name_nul_search_excludes_end(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    u32  sizes[6] = {0, 28, 0, 76, 64, 40};
    Blob b        = build_blob(512, sizes, 6, 0);
    put_info(&b);
    put_dbi(&b, 4, 5);
    put_section(&b, 5, 0x1000);

    // rec_len = 16, body = 14; name region [cur+14, cur+18) = 4 non-NUL
    // bytes, buf[cur+18] = 0 (zeroed page).
    u8 *sym = stream_page(&b, 4);
    wr_u16(&sym[0], 16);
    wr_u16(&sym[2], 0x110E);
    wr_u32(&sym[4], 0x2);
    wr_u32(&sym[8], 0x100);
    wr_u16(&sym[12], 1);
    sym[14] = 'a';
    sym[15] = 'b';
    sym[16] = 'c';
    sym[17] = 'd';

    int  n  = open_count(base, &b);
    bool ok = (n == 0);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

int main(void) {
    TestFunction tests[] = {
        test_multiblock_symrec_resolves,
        test_deep_block_count_used,
        test_multiblock_directory_no_overrun,
        test_many_streams_array_alloc_sized,
        test_many_sections_no_overrun,
        test_symrec_index_equals_num_streams,
        test_dbi_ok_reset_on_optdbg_overrun,
        test_name_nul_search_excludes_end,
    };
    TestFunction deadend_tests[] = {0};
    (void)deadend_tests;
    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), NULL, 0, "Pdb.Blind");
}
