#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Container/Buf.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Memory.h>
#include <Misra/Types.h>

#include "../Util/TestRunner.h"

// ---------------------------------------------------------------------------
// Single-byte / multi-byte writers
// ---------------------------------------------------------------------------

bool test_buf_init_clear(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Buf              b     = BufInit(&alloc);
    bool             ok    = BufLength(&b) == 0;
    BufWriteU8(&b, 0x42);
    BufWriteU8(&b, 0x43);
    ok = ok && BufLength(&b) == 2 && BufData(&b)[0] == 0x42 && BufData(&b)[1] == 0x43;
    BufClear(&b);
    ok = ok && BufLength(&b) == 0;
    BufDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

bool test_buf_write_u_le_be(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Buf              b     = BufInit(&alloc);
    BufWriteU16LE(&b, 0x1234);
    BufWriteU16BE(&b, 0x1234);
    BufWriteU32LE(&b, 0xDEADBEEF);
    BufWriteU32BE(&b, 0xDEADBEEF);
    BufWriteU64LE(&b, 0x0102030405060708ull);
    BufWriteU64BE(&b, 0x0102030405060708ull);
    const u8 expect[] = {
        0x34, 0x12,                                     // u16 LE
        0x12, 0x34,                                     // u16 BE
        0xEF, 0xBE, 0xAD, 0xDE,                         // u32 LE
        0xDE, 0xAD, 0xBE, 0xEF,                         // u32 BE
        0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, // u64 LE
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, // u64 BE
    };
    bool ok = BufLength(&b) == sizeof(expect) && MemCompare(BufData(&b), expect, sizeof(expect)) == 0;
    BufDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

bool test_buf_write_leb128(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Buf              b     = BufInit(&alloc);
    BufWriteULeb128(&b, 0);
    BufWriteULeb128(&b, 127);
    BufWriteULeb128(&b, 128);
    BufWriteULeb128(&b, 16384);
    const u8 expect_uleb[] = {0x00, 0x7F, 0x80, 0x01, 0x80, 0x80, 0x01};
    if (BufLength(&b) != sizeof(expect_uleb) || MemCompare(BufData(&b), expect_uleb, sizeof(expect_uleb)) != 0) {
        BufDeinit(&b);
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    BufClear(&b);
    BufWriteSLeb128(&b, 0);
    BufWriteSLeb128(&b, -1);
    BufWriteSLeb128(&b, 64);
    BufWriteSLeb128(&b, -64);
    const u8 expect_sleb[] = {0x00, 0x7F, 0xC0, 0x00, 0x40};
    bool ok = BufLength(&b) == sizeof(expect_sleb) && MemCompare(BufData(&b), expect_sleb, sizeof(expect_sleb)) == 0;

    BufDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

bool test_buf_write_cstr(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Buf              b     = BufInit(&alloc);
    BufWriteCstr(&b, "hi");
    const u8 expect[] = {'h', 'i', 0};
    bool     ok       = BufLength(&b) == sizeof(expect) && MemCompare(BufData(&b), expect, sizeof(expect)) == 0;
    BufDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// Cursor reads
// ---------------------------------------------------------------------------

bool test_buf_read_round_trip(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Buf              b     = BufInit(&alloc);
    BufWriteU16LE(&b, 0xABCD);
    BufWriteU32BE(&b, 0x12345678);
    BufWriteU64LE(&b, 0xFEEDFACECAFEBEEFull);

    BufIter it = BufIterFromBuf(&b);
    u16     v16;
    u32     v32;
    u64     v64;
    bool    ok = BufReadU16LE(&it, &v16) && v16 == 0xABCD;
    ok         = ok && BufReadU32BE(&it, &v32) && v32 == 0x12345678;
    ok         = ok && BufReadU64LE(&it, &v64) && v64 == 0xFEEDFACECAFEBEEFull;
    ok         = ok && IterRemainingLength(&it) == 0;

    BufDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

bool test_buf_read_leb128_round_trip(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Buf              b     = BufInit(&alloc);
    BufWriteULeb128(&b, 624485);
    BufWriteSLeb128(&b, -123456);

    BufIter it = BufIterFromBuf(&b);
    u64     uv;
    i64     sv;
    bool    ok = BufReadULeb128(&it, &uv) && uv == 624485;
    ok         = ok && BufReadSLeb128(&it, &sv) && sv == -123456;
    ok         = ok && IterRemainingLength(&it) == 0;

    BufDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

bool test_buf_read_cstr_round_trip(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Buf              b     = BufInit(&alloc);
    BufWriteCstr(&b, "hello");
    BufWriteCstr(&b, "world");

    BufIter     it = BufIterFromBuf(&b);
    const char *s1 = BufReadCstr(&it);
    const char *s2 = BufReadCstr(&it);
    bool        ok = s1 && s2 && s1[0] == 'h' && s2[0] == 'w';
    ok             = ok && IterRemainingLength(&it) == 0;

    BufDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// FMT read/append/write/patch
// ---------------------------------------------------------------------------

bool test_buf_append_fmt(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Buf              b     = BufInit(&alloc);
    BufWriteU8(&b, 0x99); // existing byte; append must preserve
    bool     ok       = BufAppendFmt(&b, "{<2r}{>4r}", (u16)0xCAFE, (u32)0xDEADBEEF);
    const u8 expect[] = {0x99, 0xFE, 0xCA, 0xDE, 0xAD, 0xBE, 0xEF};
    ok                = ok && BufLength(&b) == sizeof(expect) && MemCompare(BufData(&b), expect, sizeof(expect)) == 0;
    BufDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

bool test_buf_write_fmt_clears(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Buf              b     = BufInit(&alloc);
    BufWriteU8(&b, 0xAA);
    BufWriteU8(&b, 0xBB);
    // Write replaces previous contents.
    bool     ok       = BufWriteFmt(&b, "{<2r}", (u16)0x1234);
    const u8 expect[] = {0x34, 0x12};
    ok                = ok && BufLength(&b) == sizeof(expect) && MemCompare(BufData(&b), expect, sizeof(expect)) == 0;
    BufDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

bool test_buf_patch_fmt(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Buf              b     = BufInit(&alloc);
    // Reserve a 4-byte length placeholder followed by 8 bytes of body.
    BufAppendFmt(&b, "{<4r}", (u32)0);
    BufWriteU64LE(&b, 0x1122334455667788ull);
    // Patch the placeholder with the real value.
    bool     ok       = BufPatchFmt(&b, 0, "{<4r}", (u32)0xCAFEBABE);
    const u8 expect[] = {
        0xBE,
        0xBA,
        0xFE,
        0xCA, // u32 LE placeholder, now patched
        0x88,
        0x77,
        0x66,
        0x55,
        0x44,
        0x33,
        0x22,
        0x11
    };
    ok = ok && BufLength(&b) == sizeof(expect) && MemCompare(BufData(&b), expect, sizeof(expect)) == 0;

    // Out-of-range patch must fail and leave the buf unchanged.
    Buf snapshot = BufInit(&alloc);
    BufPushBytes(&snapshot, BufData(&b), BufLength(&b));
    ok = ok && !BufPatchFmt(&b, BufLength(&b), "{<2r}", (u16)0);
    ok = ok && BufLength(&b) == BufLength(&snapshot);
    ok = ok && MemCompare(BufData(&b), BufData(&snapshot), BufLength(&b)) == 0;
    BufDeinit(&snapshot);

    BufDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

bool test_buf_read_fmt(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Buf              b     = BufInit(&alloc);
    BufAppendFmt(&b, "{<2r}{>4r}{<8r}", (u16)0x1234, (u32)0xDEADBEEF, (u64)0x0102030405060708ull);

    BufIter it = BufIterFromBuf(&b);
    u16     v16;
    u32     v32;
    u64     v64;
    bool    ok = BufReadFmt(&it, "{<2r}{>4r}{<8r}", v16, v32, v64);
    ok         = ok && v16 == 0x1234 && v32 == 0xDEADBEEF && v64 == 0x0102030405060708ull;
    ok         = ok && IterRemainingLength(&it) == 0;

    BufDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

bool test_buf_read_fmt_truncated_atomic(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Buf              b     = BufInit(&alloc);
    BufAppendFmt(&b, "{<2r}", (u16)0xABCD); // only 2 bytes; reader wants 6

    BufIter it    = BufIterFromBuf(&b);
    size    entry = it.pos;
    u16     v16   = 0;
    u32     v32   = 0;
    bool    ok    = !BufReadFmt(&it, "{<2r}{<4r}", v16, v32);
    // Atomic rollback: pos restored, output vars left as initialized.
    ok = ok && it.pos == entry && v32 == 0;

    BufDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

bool test_byte_iter_from_memory(void) {
    const u8 bytes[] = {0xAA, 0xBB, 0xCC};
    BufIter  it      = BufIterFromMemory(bytes, 3);
    u8       v;
    bool     ok = BufReadU8(&it, &v) && v == 0xAA;
    ok          = ok && BufReadU8(&it, &v) && v == 0xBB;
    ok          = ok && BufReadU8(&it, &v) && v == 0xCC;
    ok          = ok && !BufReadU8(&it, &v); // EOF
    return ok;
}

int main(void) {
    WriteFmt("[INFO] Starting Buf tests\n\n");
    TestFunction tests[] = {
        test_buf_init_clear,
        test_buf_write_u_le_be,
        test_buf_write_leb128,
        test_buf_write_cstr,
        test_buf_read_round_trip,
        test_buf_read_leb128_round_trip,
        test_buf_read_cstr_round_trip,
        test_buf_append_fmt,
        test_buf_write_fmt_clears,
        test_buf_patch_fmt,
        test_buf_read_fmt,
        test_buf_read_fmt_truncated_atomic,
        test_byte_iter_from_memory,
    };
    int total = sizeof(tests) / sizeof(tests[0]);
    return run_test_suite(tests, total, NULL, 0, "Buf");
}
