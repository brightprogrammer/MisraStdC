#include <Misra.h>
#include <Misra/Parsers/Tzif.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Container/Buf.h>
#include <Misra/Std/Io.h>

#include "../Util/TestRunner.h"

// Append the 15 reserved header bytes (all zero).
static void put_reserved15(Buf *b) {
    BufAppendFmt(b, "{>8r}{>4r}{>2r}{>1r}", (u64)0, (u32)0, (u16)0, (u8)0);
}

// Append the six big-endian u32 counts.
static void put_counts(Buf *b, u32 timecnt, u32 typecnt) {
    BufAppendFmt(b, "{>4r}{>4r}{>4r}{>4r}{>4r}{>4r}", (u32)0, (u32)0, (u32)0, timecnt, typecnt, (u32)0);
}

// Two transitions (idx 1 then 0) and two ttinfos: STD (isdst 0) and DST
// (isdst 1). `tw` is the transition-time width path being built.
static void put_body(Buf *b, bool wide, i64 t0, i64 t1, i32 std_off, i32 dst_off) {
    if (wide) {
        BufAppendFmt(b, "{>8r}{>8r}", t0, t1);
    } else {
        BufAppendFmt(b, "{>4r}{>4r}", (i32)t0, (i32)t1);
    }
    BufAppendFmt(b, "{>1r}{>1r}", (u8)1, (u8)0);               // type indices: DST then STD
    BufAppendFmt(b, "{>4r}{>1r}{>1r}", std_off, (u8)0, (u8)0); // ttinfo[0] STD
    BufAppendFmt(b, "{>4r}{>1r}{>1r}", dst_off, (u8)1, (u8)0); // ttinfo[1] DST
}

static void build_v1(Buf *b) {
    BufAppendFmt(b, "TZif{>1r}", (u8)0);                  // magic + version 1
    put_reserved15(b);
    put_counts(b, 2, 2);
    put_body(b, false, 1000000, 2000000, -18000, -14400); // -05:00 / -04:00
}

static void build_v2(Buf *b) {
    // v1 header with NONZERO satellite counts (isutcnt=2, isstdcnt=2,
    // leapcnt=1, timecnt=2, typecnt=2, charcnt=4) so the v1-block-size
    // computation is exercised: a wrong size mislands the v2 header and
    // the parse fails. v1_data = 2*4 + 2*1 + 2*6 + 4 + 1*8 + 2 + 2 = 38.
    BufAppendFmt(b, "TZif{>1r}", (u8)'2');
    put_reserved15(b);
    BufAppendFmt(b, "{>4r}{>4r}{>4r}{>4r}{>4r}{>4r}", (u32)2, (u32)2, (u32)1, (u32)2, (u32)2, (u32)4);
    BufAppendFmt(b, "{>8r}{>8r}{>8r}{>8r}{>4r}{>2r}", (u64)0, (u64)0, (u64)0, (u64)0, (u32)0, (u16)0); // 38 bytes
    // v2 header + 8-byte-time data block (the authoritative one).
    BufAppendFmt(b, "TZif{>1r}", (u8)'2');
    put_reserved15(b);
    put_counts(b, 2, 2);
    put_body(b, true, 1000000000, 2000000000, 19800, 23400); // +05:30 / +06:30
}

static bool resolve(Buf *b, i64 t, i32 *off) {
    return TzifOffsetFromBuf(BufData(b), BufLength(b), t, off);
}

static bool test_v1_resolve(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    Buf              b = BufInit(&a);
    build_v1(&b);
    i32  off;
    bool ok = true;
    off     = 0;
    ok      = ok && resolve(&b, 500000, &off) && off == -18000;  // before t0 -> STD fallback
    off     = 0;
    ok      = ok && resolve(&b, 1000000, &off) && off == -14400; // exactly t0 -> DST (boundary)
    off     = 0;
    ok      = ok && resolve(&b, 1500000, &off) && off == -14400; // [t0,t1) -> DST
    off     = 0;
    ok      = ok && resolve(&b, 2500000, &off) && off == -18000; // >= t1 -> STD
    BufDeinit(&b);
    DefaultAllocatorDeinit(&a);
    return ok;
}

static bool test_v2_resolve(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    Buf              b = BufInit(&a);
    build_v2(&b);
    i32  off;
    bool ok = true;
    off     = 0;
    ok      = ok && resolve(&b, 500000000, &off) && off == 19800;  // before t0 -> STD fallback
    off     = 0;
    ok      = ok && resolve(&b, 1500000000, &off) && off == 23400; // [t0,t1) -> DST
    off     = 0;
    ok      = ok && resolve(&b, 2500000000, &off) && off == 19800; // >= t1 -> STD
    BufDeinit(&b);
    DefaultAllocatorDeinit(&a);
    return ok;
}

// A wrong magic is a soft failure that leaves the output untouched.
static bool test_bad_magic(void) {
    u8  junk[44] = {'X', 'Z', 'i', 'f'};
    i32 off      = 12345;
    return !TzifOffsetFromBuf(junk, sizeof(junk), 0, &off) && off == 12345;
}

// A header shorter than 44 bytes fails without reading past the buffer.
static bool test_truncated_header(void) {
    u8  tiny[10] = {'T', 'Z', 'i', 'f', 0};
    i32 off      = 999;
    return !TzifOffsetFromBuf(tiny, sizeof(tiny), 0, &off) && off == 999;
}

int main(void) {
    TestFunction tests[] = {
        test_v1_resolve,
        test_v2_resolve,
        test_bad_magic,
        test_truncated_header,
    };
    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), NULL, 0, "Tzif");
}
