// PE/COFF parser blind-mutant hardening tests. Each test below kills a
// surviving mutant that the behavioural suite (Tests/Std/Pe.c) does not
// detect. We assemble synthetic PE32+ images byte-by-byte so the tests
// run on Linux without a Windows toolchain.

#include <Misra.h>
#include <Misra/Parsers/Pe.h>
#include <Misra/Std/Allocator/Debug.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Memory.h>
#include <Misra/Std/Zstr.h>

#include "../Util/TestRunner.h"

// Little-endian writers into a flat buffer.
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

enum {
    BLOB_SIZE         = 0x800,
    DOS_E_LFANEW_OFF  = 0x3C,
    NT_OFF            = 0x80,
    FILE_HDR_OFF      = NT_OFF + 4,
    OPT_HDR_OFF       = FILE_HDR_OFF + 20,
    OPT_HDR_SIZE_PEPP = 240,
    SECTION_TBL_OFF   = OPT_HDR_OFF + OPT_HDR_SIZE_PEPP,
    SECTION_ENTSIZE   = 40,
};

// One section descriptor written into the section table.
typedef struct SecDesc {
    char name[8]; // raw 8 bytes (may be a full 8 with no NUL)
    u32  va;
} SecDesc;

// Headers + `n` section headers, NO debug data directory (so codeview
// decoding is skipped and the parse depends only on the section table).
static void build_blob(u8 *out, const SecDesc *secs, u16 n) {
    out[0] = 'M';
    out[1] = 'Z';
    wr_u32(&out[DOS_E_LFANEW_OFF], NT_OFF);

    out[NT_OFF + 0] = 'P';
    out[NT_OFF + 1] = 'E';
    out[NT_OFF + 2] = 0;
    out[NT_OFF + 3] = 0;

    wr_u16(&out[FILE_HDR_OFF + 0], 0x8664); // Machine = x86-64
    wr_u16(&out[FILE_HDR_OFF + 2], n);      // NumberOfSections
    wr_u16(&out[FILE_HDR_OFF + 16], OPT_HDR_SIZE_PEPP);
    wr_u16(&out[FILE_HDR_OFF + 18], 0x2022);

    u8 *opt = &out[OPT_HDR_OFF];
    wr_u16(&opt[0], 0x20B); // Magic = PE32+
    opt[2] = 14;
    wr_u32(&opt[16], 0x1000);
    wr_u32(&opt[20], 0x1000);
    wr_u64(&opt[24], 0x140000000ull); // ImageBase
    wr_u32(&opt[32], 0x1000);
    wr_u32(&opt[36], 0x200);
    wr_u16(&opt[40], 6);
    wr_u16(&opt[48], 6);
    wr_u32(&opt[56], 0x10000);
    wr_u32(&opt[60], 0x400);
    wr_u16(&opt[68], 3);
    wr_u32(&opt[108], 16); // NumberOfRvaAndSizes
    // DataDirectory[6] (DEBUG) left as 0 -> codeview decode skipped.

    for (u16 i = 0; i < n; ++i) {
        u8 *sec = &out[SECTION_TBL_OFF + (u32)i * SECTION_ENTSIZE];
        MemCopy(sec, secs[i].name, 8);
        wr_u32(&sec[12], secs[i].va);  // virtual_address
        wr_u32(&sec[36], 0x40000040u); // characteristics
    }
}

// ---------------------------------------------------------------------------
// pe_find_section_zstr: loop upper bound at L580 (`i < VecLen`).
// ---------------------------------------------------------------------------

// 580:24 cxx_lt_to_le. The search loop runs `for (i = 0; i < VecLen; ++i)`.
// A lt->le mutant (`i <= VecLen`) executes one extra iteration at
// i == VecLen, reading the slack slot one past the last section. The
// sections Vec always keeps a spare, ZERO-FILLED slot at index VecLen
// (Vec grows pow2 with a +1 sentinel that reserve_vec MemSets to 0), so
// that slot's `name` reads as the empty string "".
//
// We search for "" against sections whose real names are all non-empty.
// Real code: no section matches "" -> returns NULL after VecLen
// iterations. The mutant reads the zero slack slot at i == VecLen, where
// ZstrCompare(name, "") == 0, and wrongly returns a non-NULL pointer.
// Asserting NULL kills the mutant.
static bool test_pe_blind_find_empty_name_returns_null(void) {
    DebugAllocator alloc = DebugAllocatorInit();
    Allocator     *base  = ALLOCATOR_OF(&alloc);

    SecDesc secs[2];
    MemSet(secs, 0, sizeof(secs));
    MemCopy(secs[0].name, ".text\0\0\0", 8);
    secs[0].va = 0x1000;
    MemCopy(secs[1].name, ".rdata\0\0", 8);
    secs[1].va = 0x2000;

    u8 blob[BLOB_SIZE];
    MemSet(blob, 0, sizeof(blob));
    build_blob(blob, secs, 2);

    Pe pe;
    if (!PeOpenFromMemoryCopy(&pe, blob, sizeof(blob), base)) {
        DebugAllocatorDeinit(&alloc);
        return false;
    }

    // The named sections resolve (sanity), but the empty name does not.
    bool ok = PeFindSection(&pe, ".text") != NULL;
    ok      = ok && PeFindSection(&pe, ".rdata") != NULL;
    ok      = ok && PeFindSection(&pe, "") == NULL; // L580 upper bound

    PeDeinit(&pe);
    DebugAllocatorDeinit(&alloc);
    return ok;
}

int main(void) {
    WriteFmt("[INFO] Starting Pe.Blind tests\n\n");

    TestFunction tests[] = {
        test_pe_blind_find_empty_name_returns_null,
    };

    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), NULL, 0, "Pe.Blind");
}
