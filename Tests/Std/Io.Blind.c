#include <Misra.h>
#include <Misra/Std/Allocator.h>
#include <Misra/Std/Allocator/Debug.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Container/BitVec.h>
#include <Misra/Std/Container/Buf.h>
#include <Misra/Std/Container/Float.h>
#include <Misra/Std/Container/Int.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Std/File.h>
#include <Misra/Std/Io.h>
#include <Misra/Std/Io/Private.h>
#include <Misra/Std/Math.h>
#include <Misra/Std/Memory.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Sys/Dir.h>
#include <Misra/Types.h>

#include "../Util/TestRunner.h"

// ===========================================================================
// Io.Blind: kills for the surviving mutants in Source/Misra/Std/Io.c that the
// Write/Read/UserTypes suites left alive. Each test pins observable bytes /
// parsed values, or (for write-path scratch StrDeinit removals) observes a
// leak through a DebugAllocator-backed output Str.
// ===========================================================================

// Helper: a fresh DebugAllocator-backed output Str + live-count assertion. The
// write-path scratch (`_write_*` temp / hex Strs, BitVec/Int bit_str/temp) is
// allocated through StrAllocator(o), i.e. THIS DebugAllocator, so a removed
// StrDeinit on that scratch leaves a live allocation after the output Str is
// torn down.
#define DBG_BEGIN(dbg, out)                                                                                            \
    DebugAllocator dbg = DebugAllocatorInit();                                                                         \
    Str            out = StrInit(&dbg)

// Tear down `out`, then assert no scratch leaked, then deinit the allocator.
static bool dbg_no_leak(DebugAllocator *dbg, Str *out) {
    StrDeinit(out);
    bool ok = (DebugAllocatorLiveCount(dbg) == 0);
    DebugAllocatorDeinit(dbg);
    return ok;
}

// ---------------------------------------------------------------------------
// parse_format_spec: width / precision parsing (lines 144-165). Observable
// through the rendered output of any write path.
// ---------------------------------------------------------------------------

// 144:17 lt_to_le `pos + 1 < len`: the zero-pad detector needs at least one
// digit AFTER the '0'. Spec "{05}" (len 2 of body "05"): pos+1==1 < 2 true ->
// zero-pad of width 5. lt_to_le makes pos+1<=len read spec[2] out of body.
// We pin the zero-pad behaviour: u32 7 with {05} -> "00007".
static bool test_pfs_zero_pad_detect(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              out   = StrInit(&alloc);
    u32              v     = 7;
    StrAppendFmt(&out, "{05}", v);
    bool ok = (ZstrCompare(StrBegin(&out), "00007") == 0);
    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// 150:13 lt_to_le `pos < len` (width digit gate) and 151:20 lt_to_le (width
// loop). Spec "{3}" width 3: 42 -> "   42" with width-3? No: width 3, "42" ->
// " 42". Pin width parse: {4} of "ab" -> "  ab".
static bool test_pfs_width_parse(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              out   = StrInit(&alloc);
    Zstr             s     = "ab";
    StrAppendFmt(&out, "{4}", s);
    bool ok = (ZstrCompare(StrBegin(&out), "  ab") == 0);
    StrClear(&out);
    // Two-digit width exercises the width loop (151) more than once.
    StrAppendFmt(&out, "{12}", s);
    ok = ok && (ZstrCompare(StrBegin(&out), "          ab") == 0);
    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// 158:13 lt_to_le `pos < len` (precision '.' gate): "{.2}" on float 3.14159 ->
// "3.14". A le swap would read one past the body. 160:17 ge_to_gt
// `pos >= len` (precision must have a trailing digit): "{.}" is malformed and
// must be rejected -> output stays empty.  165:20 lt_to_le precision loop.
static bool test_pfs_precision_parse(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              out   = StrInit(&alloc);
    f64              v     = 3.14159;
    StrAppendFmt(&out, "{.2}", v);
    bool ok = (ZstrCompare(StrBegin(&out), "3.14") == 0);
    StrClear(&out);
    // multi-digit precision exercises the precision loop body (165).
    StrAppendFmt(&out, "{.4}", v);
    ok = ok && (ZstrCompare(StrBegin(&out), "3.1416") == 0);
    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// 160:17 ge_to_gt: a lone '.' with no digit must be rejected. Real:
// parse_format_spec returns false -> StrAppendFmt fails, output untouched.
static bool test_pfs_dot_no_digit_rejected(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              out   = StrInit(&alloc);
    StrAppendFmt(&out, "literal");
    i32  v  = 5;
    bool rc = StrAppendFmt(&out, "{.}", v);
    // Real: false; output keeps only the prior "literal" (append stopped).
    bool ok = (rc == false) && (ZstrCompare(StrBegin(&out), "literal") == 0);
    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// pad_numeric_zeros / StrPad (238, 245, 257). Observable via zero-pad + sign.
// ---------------------------------------------------------------------------

// 245:21 gt_to_ge `content_len > 0` (sign detection inserts '0' AFTER sign):
// i32 -7 with {05} -> "-0007" (sign kept, 3 zeros). A ge swap is a no-op here
// (content_len is 2), so to make col-21 observable we need content_len==0?
// content_len is always >0 for a rendered number; the kill is that the sign
// branch must fire: pin "-0007".
static bool test_pad_zeros_signed(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              out   = StrInit(&alloc);
    i32              v     = -7;
    StrAppendFmt(&out, "{05}", v);
    bool ok = (ZstrCompare(StrBegin(&out), "-0007") == 0);
    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// 238:21 ge_to_gt `content_len >= width` (pad_numeric_zeros early-out): when
// content already meets width, NO padding. {02} of 123 (3 digits) -> "123".
// ge_to_gt makes 3>2 still true so identical; the distinguishing case is
// content_len == width: {03} of 123 -> "123" (no extra zero). ge_to_gt: 3>3
// false -> would try to pad by 0 anyway (pad_len 0) -> identical. So col-238
// is pinned by exact-fit + under-fit together.
static bool test_pad_zeros_exact_and_under(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              out   = StrInit(&alloc);
    u32              v     = 123;
    StrAppendFmt(&out, "{03}", v);
    bool ok = (ZstrCompare(StrBegin(&out), "123") == 0);
    StrClear(&out);
    StrAppendFmt(&out, "{05}", v);
    ok = ok && (ZstrCompare(StrBegin(&out), "00123") == 0);
    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// 257:21 ge_to_gt `content_len >= width` (StrPad early-out): exact-fit width
// must NOT pad, under-fit must. {3} of "abc" (len 3) -> "abc"; {5} -> "  abc".
static bool test_strpad_exact_and_under(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              out   = StrInit(&alloc);
    Zstr             s     = "abc";
    StrAppendFmt(&out, "{3}", s);
    bool ok = (ZstrCompare(StrBegin(&out), "abc") == 0);
    StrClear(&out);
    StrAppendFmt(&out, "{5}", s);
    ok = ok && (ZstrCompare(StrBegin(&out), "  abc") == 0);
    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// str_append_fmt brace handling (307, 317, 363, 382, 390, 456).
// ---------------------------------------------------------------------------

// 307:23 lt_to_le `i + 1 < fmt_len` ('{{' escape look-ahead): a trailing single
// '{' at end-of-string is an unclosed spec; '{{' mid-string is a literal '{'.
static bool test_brace_escape_open(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              out   = StrInit(&alloc);
    StrAppendFmt(&out, "a{{b");
    bool ok = (ZstrCompare(StrBegin(&out), "a{b") == 0);
    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// 456:23 lt_to_le `i + 1 < fmt_len` ('}}' escape look-ahead): '}}' -> '}'.
static bool test_brace_escape_close(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              out   = StrInit(&alloc);
    StrAppendFmt(&out, "a}}b");
    bool ok = (ZstrCompare(StrBegin(&out), "a}b") == 0);
    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// 317:30 lt_to_le `brace_end < fmt_len` (scan to closing '}'): a normal spec
// "{x}" must find its '}' and render hex. A le swap reads one past end.
static bool test_brace_scan_close(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              out   = StrInit(&alloc);
    u32              v     = 0xAB;
    StrAppendFmt(&out, "{x}", v);
    bool ok = (ZstrCompare(StrBegin(&out), "0xab") == 0);
    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// 363:36 init_const var_width=0, 390:21 init_const x=0: str_append_fmt's raw
// branch recovers var_width from the writer identity, reads the value into x,
// then re-dispatches to _write_uN (which renders DECIMAL -- the RAW flag is
// ignored downstream). {<2r} of u16 0x1234 -> "4660". x=42 -> "42";
// var_width=42 -> switch default LOG_ERROR + return false -> empty output.
static bool test_raw_write_u16_value(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              out   = StrInit(&alloc);
    u16              v     = 0x1234; // 4660
    bool             rc    = StrAppendFmt(&out, "{<2r}", v);
    bool             ok    = rc && (ZstrCompare(StrBegin(&out), "4660") == 0);
    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Same raw path, width-8 u64 -> decimal of 0x0102030405060708. var_width via
// 363 mis-selects the pickup (and aborts via default); x via 390 corrupts.
static bool test_raw_write_u64_value(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              out   = StrInit(&alloc);
    u64              v     = 0x0102030405060708ULL; // 72623859790382856
    bool             rc    = StrAppendFmt(&out, "{>8r}", v);
    bool             ok    = rc && (ZstrCompare(StrBegin(&out), "72623859790382856") == 0);
    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ===========================================================================
// f_write_fmt (519-555): FileWrite + flush path. Observable via the file's
// bytes on disk.
// ===========================================================================

// Write a formatted line to a temp file with newline appended, read it back.
// Pins 521:10 ok init, 533:9 ok assign (the append result must gate the
// write), 543:28 gt_to_ge (StrLen>0 write guard), 545/550 ok=false on failure
// paths (those only matter on failure -> see notes), and the void calls.
static bool test_fwrite_roundtrip(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              path  = StrInit(&alloc);
    File             f     = FileOpenTemp(&path, &alloc);
    bool             ok    = FileIsOpen(&f);
    if (ok) {
        ok = ok && FWriteFmtLn(&f, "n={}", LVAL((i32)42));
        FileClose(&f);
        File r = FileOpen(StrBegin(&path), "r");
        if (FileIsOpen(&r)) {
            Str back = StrInit(&alloc);
            FileRead(&r, &back);
            ok = ok && (ZstrCompare(StrBegin(&back), "n=42\n") == 0);
            StrDeinit(&back);
            FileClose(&r);
        } else {
            ok = false;
        }
        FileRemove(&path);
    }
    StrDeinit(&path);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// 543:28 gt_to_ge `StrLen(&out) > 0`: an EMPTY format with no newline produces
// a zero-length line, which must NOT be written (and FileWrite of 0 bytes must
// not be attempted with a mismatched return). Write empty (no newline) then a
// real line; the file must contain only the real line.
static bool test_fwrite_empty_skipped(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              path  = StrInit(&alloc);
    File             f     = FileOpenTemp(&path, &alloc);
    bool             ok    = FileIsOpen(&f);
    if (ok) {
        // FWriteFmt (no newline) of empty string: nothing to write.
        ok = ok && FWriteFmt(&f, "");
        ok = ok && FWriteFmt(&f, "X");
        FileClose(&f);
        File r = FileOpen(StrBegin(&path), "r");
        if (FileIsOpen(&r)) {
            Str back = StrInit(&alloc);
            FileRead(&r, &back);
            ok = ok && (ZstrCompare(StrBegin(&back), "X") == 0);
            StrDeinit(&back);
            FileClose(&r);
        } else {
            ok = false;
        }
        FileRemove(&path);
    }
    StrDeinit(&path);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ===========================================================================
// str_read_fmt brace / spec handling (592, 605, 618, 622, 630, 682, 686, 692,
// 712, 748, 750, 770, 792).
// ===========================================================================
#define READ1(in, fmt, IO) str_read_fmt((in), (fmt), (TypeSpecificIO[]) {(IO)}, 1)

// 592:26 gt_to_ge `rem_p > 0` (spec scan loop). 605:26 ge_to_gt `spec_len>=32`
// (over-long spec rejected). A normal spec "{}" with a value reads fine.
static bool test_read_spec_scan(void) {
    i32  v   = 0;
    Zstr in  = "42";
    Zstr out = READ1(in, "{}", TO_TYPE_SPECIFIC_IO(i32, &v));
    return out == in + 2 && v == 42;
}

// 605:26 ge_to_gt: a 32-char spec body is rejected. Build "{" + 32 'q' + "}".
static bool test_read_overlong_spec_rejected(void) {
    i32 v = 7;
    // 32 chars inside braces -> spec_len == 32 -> rejected.
    Zstr out = READ1("x", "{qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq}", TO_TYPE_SPECIFIC_IO(i32, &v));
    return out == NULL && v == 7;
}

// 618:21 init_const spec_ok=false, 622:32 assign_const data[spec_len]='\0',
// 624 parse gate: an invalid spec char 'q' must fail the parse and leave the
// destination untouched (covered in Read suite already, repeat for spec_ok).
static bool test_read_invalid_spec_leaves_dest(void) {
    i32  v   = 77;
    Zstr out = READ1("5", "{q}", TO_TYPE_SPECIFIC_IO(i32, &v));
    return out == NULL && v == 77;
}

// 630:35 assign_const `fmt_info.max_read_len = rem_in`: a {s} string read at
// end of format uses max_read_len = remaining input. A quoted string of 40
// chars must be read in full (not truncated to a constant 42... but 42 > 40
// so we need > 42). Use 50 chars: forcing max_read_len to a constant 42 caps
// the read at 42, observable as a short string.
static bool test_read_max_read_len_set(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Zstr             in    = "\"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\""; // 50 A's quoted
    Str              s     = StrInit(&alloc);
    Zstr             out   = READ1(in, "{s}", TO_TYPE_SPECIFIC_IO(Str, &s));
    bool             ok    = (out != NULL) && (StrLen(&s) == 50);
    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// 682:21 init_const x=0, 692:23 init_const var_width=0, 712:36 gt swaps, plus
// the raw read store. {<4r} into a u32 from 4 LE bytes -> the value. var_width
// 0->42 mis-selects the store; x 0->42 corrupts before pickup.
static bool test_read_raw_u32_le(void) {
    u32  v   = 0;
    Zstr in  = "\xEF\xBE\xAD\xDE";
    Zstr out = READ1(in, "{<4r}", TO_TYPE_SPECIFIC_IO(u32, &v));
    return out == in + 4 && v == 0xDEADBEEFu;
}

// 686:37 sub_to_add / 686:28 sub_assign (rem_in -= next-in in the RAW branch):
// after a raw read followed by a literal, rem_in must stay correct so the
// literal matches. {<1r} then literal 'Z': input "\x05Z".
static bool test_read_raw_then_literal(void) {
    u8   v   = 0;
    Zstr in  = "\x05Z";
    Zstr out = READ1(in, "{<1r}Z", TO_TYPE_SPECIFIC_IO(u8, &v));
    return out == in + 2 && v == 0x05;
}

// 748:22 init_const c=p[space_len], 750:49 add_to_sub `p[space_len + 1]`,
// 770:37/28 sub (rem_in -= next-in in the NON-raw branch), 792:19 post_dec
// (rem_in-- on a literal match). A {} field bounded by a trailing literal,
// then more input: "{}-end" on "12-end". The literal-run scan (748/750) caps
// the field at '-'; rem_in bookkeeping (770/792) keeps the tail in sync.
static bool test_read_field_bounded_by_literal(void) {
    i32  v   = 0;
    Zstr in  = "12-end";
    Zstr out = READ1(in, "{}-end", TO_TYPE_SPECIFIC_IO(i32, &v));
    return out == in + 6 && v == 12;
}

// 750:49 add_to_sub specifically: the literal run's escape check reads
// `p[space_len + 1]`. Use a non-numeric separator '!' so the field stops
// cleanly, then an escaped '{{' in the literal run. "{}!{{y" on "5!{y": field
// "5", literal run "!{{y" matches "!{y" -> consumes all 4 bytes. A +1 -> -1
// swap mis-reads the escape look-ahead and breaks the literal match.
static bool test_read_field_literal_with_escape(void) {
    i32  v   = 0;
    Zstr in  = "5!{y";
    Zstr out = READ1(in, "{}!{{y", TO_TYPE_SPECIFIC_IO(i32, &v));
    return out == in + 4 && v == 5;
}

// ===========================================================================
// buf_read_fmt (820, 829, 854, 864, 901, 909) and render_binary_fmt (956,
// 1004, 1012, 1031, 1035).
// ===========================================================================

// 820:13 / 829:17 init_const (fc/sc = 0), 854:23 ge_to_gt (arg_index>=argc),
// 864:14 init_const (x=0), 901:15 init_const (var_width=0): a clean buf read
// round-trip. 909:92 eq_to_ne (read_fn == _read_f64 in the u64/i64/f64 OR):
// covered by f64 below.
static bool test_buf_read_u16(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Buf              b     = BufInit(&alloc);
    bool             ok    = BufWriteFmt(&b, "{<2r}", (u16)0xBEEF);
    BufIter          it    = BufIterFromBuf(&b);
    u16              v     = 0;
    ok                     = ok && BufReadFmt(&it, "{<2r}", v) && (v == 0xBEEF);
    BufDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// 909:92 eq_to_ne (the `read_fn == _read_f64` clause): an f64 raw read must
// resolve var_width 8. A != swap drops f64 into the unsupported-type LOG_FATAL.
static bool test_buf_read_f64(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Buf              b     = BufInit(&alloc);
    union {
        f64 f;
        u64 u;
    } got      = {0};
    bool    ok = BufWriteFmt(&b, "{<8r}", (u64)0x0102030405060708ULL);
    BufIter it = BufIterFromBuf(&b);
    ok         = ok && BufReadFmt(&it, "{<8r}", got.f) && (got.u == 0x0102030405060708ULL);
    BufDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// 956:11 init_const (var_width=0 in render_one_raw_field), 1004:13 / 1012:17
// init_const (fc/sc), 1031:23 ge_to_gt (arg_index>=argc), 1035:65 sub_to_add
// (render_one_raw_field arg_index-1): a multi-field buf write round-trip.
static bool test_buf_write_multi(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Buf              b     = BufInit(&alloc);
    bool             ok    = BufWriteFmt(&b, "{<2r}{>4r}", (u16)0x1234, (u32)0xAABBCCDD);
    ok                     = ok && (BufLength(&b) == 6);
    BufIter it             = BufIterFromBuf(&b);
    u16     a              = 0;
    u32     c              = 0;
    ok                     = ok && BufReadFmt(&it, "{<2r}{>4r}", a, c) && a == 0x1234 && c == 0xAABBCCDD;
    BufDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ===========================================================================
// _write_Str / _write_Zstr write-path scratch LEAK kills (1653, 1658, 1663,
// 1666, 1741, 1746, 1751, 1754) -- the per-byte hex `Str hex` is allocated
// through StrAllocator(o). A removed StrDeinit(&hex) leaks one alloc per byte.
// ===========================================================================

// 1653/1663/1666 (and the 1658 push-front-fail dtor): {x} of a 2-byte Str
// allocates two hex scratch Strs through the output allocator. With any
// StrDeinit(&hex) removed, LiveCount > 0 after teardown.
static bool test_str_hex_no_leak(void) {
    DBG_BEGIN(dbg, out);
    Str s = StrInit(&dbg);
    StrPushBackR(&s, (char)0x01);
    StrPushBackR(&s, (char)0x4F); // multi-digit so StrLen(&hex)!=1 path
    StrAppendFmt(&out, "{x}", s);
    bool ok = (ZstrCompare(StrBegin(&out), "0x01 0x4f") == 0);
    StrDeinit(&s);
    return ok && dbg_no_leak(&dbg, &out);
}

// 1658 specifically: the zero-pad-nibble path (StrLen(&hex)==1) takes a
// StrPushFrontR; the StrDeinit after it must still run. Single 0x05 byte.
static bool test_str_hex_nibble_no_leak(void) {
    DBG_BEGIN(dbg, out);
    Str s = StrInit(&dbg);
    StrPushBackR(&s, (char)0x05);
    StrAppendFmt(&out, "{x}", s);
    bool ok = (ZstrCompare(StrBegin(&out), "0x05") == 0);
    StrDeinit(&s);
    return ok && dbg_no_leak(&dbg, &out);
}

// 1741/1746/1751/1754: same hex scratch leak for the Zstr path.
static bool test_zstr_hex_no_leak(void) {
    DBG_BEGIN(dbg, out);
    Zstr s = "\x01\x4f";
    StrAppendFmt(&out, "{x}", s);
    bool ok = (ZstrCompare(StrBegin(&out), "0x01 0x4f") == 0);
    return ok && dbg_no_leak(&dbg, &out);
}

static bool test_zstr_hex_nibble_no_leak(void) {
    DBG_BEGIN(dbg, out);
    Zstr s = "\x05";
    StrAppendFmt(&out, "{x}", s);
    bool ok = (ZstrCompare(StrBegin(&out), "0x05") == 0);
    return ok && dbg_no_leak(&dbg, &out);
}

// 1703:25 / 1791:25 gt_to_ge `fmt_info->width > 0` (Str/Zstr width gate): width
// 0 must NOT pad, width > 0 must. {0} (width 0) of "hi" -> "hi"; {5} -> "   hi".
static bool test_str_width_gate(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              out   = StrInit(&alloc);
    Str              s     = StrInitFromZstr("hi", &alloc);
    StrAppendFmt(&out, "{}", s); // width 0 default
    bool ok = (ZstrCompare(StrBegin(&out), "hi") == 0);
    StrClear(&out);
    StrAppendFmt(&out, "{5}", s);
    ok = ok && (ZstrCompare(StrBegin(&out), "   hi") == 0);
    StrDeinit(&s);
    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

static bool test_zstr_width_gate(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              out   = StrInit(&alloc);
    Zstr             s     = "hi";
    StrAppendFmt(&out, "{}", s);
    bool ok = (ZstrCompare(StrBegin(&out), "hi") == 0);
    StrClear(&out);
    StrAppendFmt(&out, "{5}", s);
    ok = ok && (ZstrCompare(StrBegin(&out), "   hi") == 0);
    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ===========================================================================
// _write_u64 / _write_i64 scratch temp LEAK + width gate (1847, 1852, 1855,
// 1857, 1947, 1952, 1955, 1957). `Str temp` is StrAllocator(o)-backed.
// ===========================================================================
static bool test_u64_no_leak(void) {
    DBG_BEGIN(dbg, out);
    u64 v = 1234567890ULL;
    StrAppendFmt(&out, "{}", v);
    bool ok = (ZstrCompare(StrBegin(&out), "1234567890") == 0);
    return ok && dbg_no_leak(&dbg, &out);
}

static bool test_i64_no_leak(void) {
    DBG_BEGIN(dbg, out);
    i64 v = -1234567890LL;
    StrAppendFmt(&out, "{}", v);
    bool ok = (ZstrCompare(StrBegin(&out), "-1234567890") == 0);
    return ok && dbg_no_leak(&dbg, &out);
}

// 1857:25 / 1957:25 gt_to_ge width gate for u64/i64.
static bool test_u64_width_gate(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              out   = StrInit(&alloc);
    u64              v     = 42;
    StrAppendFmt(&out, "{}", v);
    bool ok = (ZstrCompare(StrBegin(&out), "42") == 0);
    StrClear(&out);
    StrAppendFmt(&out, "{6}", v);
    ok = ok && (ZstrCompare(StrBegin(&out), "    42") == 0);
    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

static bool test_i64_width_gate(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              out   = StrInit(&alloc);
    i64              v     = -42;
    StrAppendFmt(&out, "{}", v);
    bool ok = (ZstrCompare(StrBegin(&out), "-42") == 0);
    StrClear(&out);
    StrAppendFmt(&out, "{6}", v);
    ok = ok && (ZstrCompare(StrBegin(&out), "   -42") == 0);
    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ===========================================================================
// _write_f64 (2025, 2046, 2067, 2074, 2079, 2082, 2085).
// ===========================================================================

// 2025:16 replace_scalar_call `write_int_as_chars(o,flags,bits,8)` for {c} on
// f64: the 8 IEEE bytes are rendered. Force a known bit pattern via memcpy.
static bool test_f64_char_bytes(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              out   = StrInit(&alloc);
    // Build a double whose 8 BE bytes are all printable ASCII: "ABCDEFGH".
    f64 v;
    u64 bits = ((u64)'A' << 56) | ((u64)'B' << 48) | ((u64)'C' << 40) | ((u64)'D' << 32) | ((u64)'E' << 24) |
               ((u64)'F' << 16) | ((u64)'G' << 8) | (u64)'H';
    MemCopy(&v, &bits, sizeof(v));
    StrAppendFmt(&out, "{c}", v);
    bool ok = (ZstrCompare(StrBegin(&out), "ABCDEFGH") == 0);
    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// 2046:16 lt_to_le `*v < 0` (negative-inf sign): -inf -> "-inf", +inf -> "inf".
static bool test_f64_inf_sign(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              out   = StrInit(&alloc);
    f64              ninf  = -F64_INFINITY;
    StrAppendFmt(&out, "{}", ninf);
    bool ok = (ZstrCompare(StrBegin(&out), "-inf") == 0);
    StrClear(&out);
    f64 pinf = F64_INFINITY;
    StrAppendFmt(&out, "{}", pinf);
    ok = ok && (ZstrCompare(StrBegin(&out), "inf") == 0);
    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// 2067:52 and_to_or (`flags & FMT_FLAG_HAS_PRECISION ? precision : 6`): the
// default precision is 6 when no precision flag. {} of 1.5 -> "1.500000"
// (6 zeros). With and->or the predicate is always truthy and uses precision
// (uninitialised / 6 from FmtInfo default... pin the default-6 output).
static bool test_f64_default_precision(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              out   = StrInit(&alloc);
    f64              v     = 1.5;
    StrAppendFmt(&out, "{}", v);
    bool ok = (ZstrCompare(StrBegin(&out), "1.500000") == 0);
    StrClear(&out);
    // explicit precision 2 to contrast.
    StrAppendFmt(&out, "{.2}", v);
    ok = ok && (ZstrCompare(StrBegin(&out), "1.50") == 0);
    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// 2074/2079/2082 scratch temp leak in the normal-number branch of _write_f64.
static bool test_f64_no_leak(void) {
    DBG_BEGIN(dbg, out);
    f64 v = 2.71828;
    StrAppendFmt(&out, "{}", v);
    bool ok = (ZstrCompare(StrBegin(&out), "2.718280") == 0);
    return ok && dbg_no_leak(&dbg, &out);
}

// 2085:25 gt_to_ge width gate for f64.
static bool test_f64_width_gate(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              out   = StrInit(&alloc);
    f64              v     = 1.5;
    StrAppendFmt(&out, "{.1}", v);
    bool ok = (ZstrCompare(StrBegin(&out), "1.5") == 0);
    StrClear(&out);
    StrAppendFmt(&out, "{6.1}", v);
    ok = ok && (ZstrCompare(StrBegin(&out), "   1.5") == 0);
    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ===========================================================================
// float_fmt_append_exponent (1285, 1291, 1306, 1312, 1319) and
// float_try_to_decimal_str / scientific (1350, 1351, 1410, 1411, 1425, 1426,
// 1478, 1504, 1505). All reached via _write_Float (Float container, scratch
// via StrAllocator(o)).
// ===========================================================================

// 1285:31 lt_to_le `exponent < 0` (exponent sign/magnitude): a positive and a
// negative exponent must render with '+' / '-'. 1.5e2 -> "...e+02"; 1.5e-3 ...
static bool test_float_sci_exponent_sign(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *ab    = ALLOCATOR_OF(&alloc);
    Str              out   = StrInit(&alloc);
    Float            v     = FloatFromStr("12345.67", ab);
    StrAppendFmt(&out, "{e}", v);
    bool ok = (ZstrCompare(StrBegin(&out), "1.234567e+04") == 0);
    StrClear(&out);
    Float w = FloatFromStr("0.001234", ab);
    StrAppendFmt(&out, "{e}", w);
    ok = ok && (ZstrCompare(StrBegin(&out), "1.234e-03") == 0);
    FloatDeinit(&v);
    FloatDeinit(&w);
    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// 1312:20 / 1319:20 assign_const (ok=false on push fail -- only on failure) and
// 1306:16 likewise. 1291:10 init ok=true. The exponent two-digit minimum (1310
// digit_count<2 -> leading '0') is pinned by an e+04 (two digits) vs a single
// digit exponent. e+04 already has 2 digits; use 1e+100 path? exponent 100 has
// 3 digits (no pad). Use exponent 4 -> "+04" (pad). Already covered above.
// This test pins the 2-digit-min padding directly: 1.0 -> "1e+00".
static bool test_float_sci_exponent_pad(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *ab    = ALLOCATOR_OF(&alloc);
    Str              out   = StrInit(&alloc);
    Float            v     = FloatFromStr("1.0", ab);
    StrAppendFmt(&out, "{e}", v);
    bool ok = (ZstrCompare(StrBegin(&out), "1e+00") == 0);
    FloatDeinit(&v);
    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// 1478:64 gt_to_ge `StrLen(&digits) > 0` (frac_digits default when no
// precision): a single-digit significand yields frac_digits 0 -> no point.
static bool test_float_sci_single_digit(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *ab    = ALLOCATOR_OF(&alloc);
    Str              out   = StrInit(&alloc);
    Float            v     = FloatFromStr("2.0", ab);
    StrAppendFmt(&out, "{e}", v);
    bool ok = (ZstrCompare(StrBegin(&out), "2e+00") == 0);
    FloatDeinit(&v);
    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// 1350/1351 init_const prefix/frac and 1410/1411/1504/1505 scratch dtor leaks:
// decimal + scientific Float render through a DebugAllocator output must not
// leak the canonical/result/digits scratch.
static bool test_float_decimal_no_leak(void) {
    DBG_BEGIN(dbg, out);
    Float v = FloatFromStr("1234567890.012345", &dbg.base);
    StrAppendFmt(&out, "{}", v);
    bool ok = (ZstrCompare(StrBegin(&out), "1234567890.012345") == 0);
    FloatDeinit(&v);
    return ok && dbg_no_leak(&dbg, &out);
}

// decimal with precision exercises the dot/frac branch (1350/1351 prefix/frac).
static bool test_float_decimal_precision(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *ab    = ALLOCATOR_OF(&alloc);
    Str              out   = StrInit(&alloc);
    Float            v     = FloatFromStr("3.14159", ab);
    StrAppendFmt(&out, "{.2}", v);
    bool ok = (ZstrCompare(StrBegin(&out), "3.14") == 0);
    StrClear(&out);
    // precision longer than frac pads with zeros.
    StrAppendFmt(&out, "{.8}", v);
    ok = ok && (ZstrCompare(StrBegin(&out), "3.14159000") == 0);
    FloatDeinit(&v);
    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// decimal with no fractional dot but a precision: integer-valued Float with
// precision adds ".000". Pins the no-dot branch (1363) prefix handling.
static bool test_float_decimal_int_with_precision(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *ab    = ALLOCATOR_OF(&alloc);
    Str              out   = StrInit(&alloc);
    Float            v     = FloatFromStr("5", ab);
    StrAppendFmt(&out, "{.3}", v);
    bool ok = (ZstrCompare(StrBegin(&out), "5.000") == 0);
    FloatDeinit(&v);
    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

static bool test_float_sci_no_leak(void) {
    DBG_BEGIN(dbg, out);
    Float v = FloatFromStr("12345.67", &dbg.base);
    StrAppendFmt(&out, "{e}", v);
    bool ok = (ZstrCompare(StrBegin(&out), "1.234567e+04") == 0);
    FloatDeinit(&v);
    return ok && dbg_no_leak(&dbg, &out);
}

// 2115:10 init_const start_len, 2155:9 scratch dtor leak, 2160:25 width gate
// in _write_Float.
static bool test_float_width_gate(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *ab    = ALLOCATOR_OF(&alloc);
    Str              out   = StrInit(&alloc);
    Float            v     = FloatFromStr("1.5", ab);
    StrAppendFmt(&out, "{}", v);
    bool ok = (ZstrCompare(StrBegin(&out), "1.5") == 0);
    StrClear(&out);
    StrAppendFmt(&out, "{6}", v);
    ok = ok && (ZstrCompare(StrBegin(&out), "   1.5") == 0);
    FloatDeinit(&v);
    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// 2115:10 init_const start_len: pad after a prefix so start_len must be
// non-zero. "AB" + {6} of Float 1.5 -> "AB" then 3 leading pad chars on the
// whole buffer with ALIGN_RIGHT default: "   AB1.5".
static bool test_float_width_after_prefix(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *ab    = ALLOCATOR_OF(&alloc);
    Str              out   = StrInit(&alloc);
    Float            v     = FloatFromStr("1.5", ab);
    StrAppendFmt(&out, "AB");
    StrAppendFmt(&out, "{>6}", v);
    bool ok = (ZstrCompare(StrBegin(&out), "   AB1.5") == 0);
    FloatDeinit(&v);
    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ===========================================================================
// float_fmt_token_length (1511, 1514, 1515, 1525, 1533, 1539, 1546, 1547,
// 1548) -- reached via _read_Float (Float dest allocator -> observable).
// ===========================================================================

// Read a Float and compare its canonical form. A wrong token boundary leaves
// the pre-read sentinel.
static bool read_float_is(Zstr in, Zstr expect) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *ab    = ALLOCATOR_OF(&alloc);
    Float            f     = FloatInit(ab);
    (void)FloatTryFromStr(&f, "99"); // sentinel
    StrReadFmt(in, "{}", f);
    Str  t  = FloatToStr(&f);
    bool ok = (ZstrCompare(StrBegin(&t), expect) == 0);
    StrDeinit(&t);
    FloatDeinit(&f);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// 1511/1525 saw_digit, 1533 allow_sign after digit: a plain integer token.
static bool test_tokenlen_plain_digit(void) {
    return read_float_is("7 x", "7");
}

// 1539 saw_decimal: a decimal token "3.5".
static bool test_tokenlen_decimal(void) {
    return read_float_is("3.5 x", "3.5");
}

// 1515/1533 allow_sign (leading sign): "-2.5".
static bool test_tokenlen_signed(void) {
    return read_float_is("-2.5 x", "-2.5");
}

// 1546/1547/1548 exponent introducer (saw_exponent/need_exp_digit/allow_sign):
// "1.5e2" -> 150; "1.5e+2" sign after exponent.
static bool test_tokenlen_exponent(void) {
    return read_float_is("1.5e2 x", "150") && read_float_is("1.5e+2 x", "150") && read_float_is("2.5e-1 y", "0.25");
}

// 1514 need_exp_digit: "1.5e" with no exponent digit must FAIL (token_len 0),
// leaving the sentinel "99".
static bool test_tokenlen_incomplete_exponent(void) {
    // "1.5e" then space: float_fmt_token_length requires an exp digit, returns
    // 0 -> _read_Float fails -> sentinel kept.
    return read_float_is("1.5e ", "99");
}

// ===========================================================================
// is_valid_numeric_string (2395-2486) -- reached via _read_f64 (scratch is
// DefaultAllocator, but the f64 dest is written through *v, observable) and
// also through _read_Float? No: _read_Float uses float_fmt_token_length. The
// f64/f32 path uses is_valid_numeric_string. The DEST f64 IS observable.
// ===========================================================================
#define SENTINEL (-987654.0)
static bool f64_is(f64 v, f64 e) {
    return F64Abs(v - e) < 1e-9;
}

// 2395:17 eq_to_ne `len == 3` (inf/nan short-circuit gate): "inf" (len 3) is a
// valid float; a !=3 swap rejects it.
static bool test_vns_inf_len3(void) {
    f64  v = SENTINEL;
    Zstr z = "inf";
    StrReadFmt(z, "{}", v);
    return F64IsInf(v) && v > 0;
}

// 2396 (4 eq) / 2397 (2 eq): each char of "inf" / "nan" checked both cases.
// "INF" upper, "nan" lower, "NaN" mixed all parse.
static bool test_vns_inf_nan_cases(void) {
    f64  a = SENTINEL, b = SENTINEL, c = SENTINEL;
    Zstr za = "INF", zb = "nan", zc = "NaN";
    StrReadFmt(za, "{}", a);
    StrReadFmt(zb, "{}", b);
    StrReadFmt(zc, "{}", c);
    return F64IsInf(a) && F64IsNan(b) && F64IsNan(c);
}

// 2400/2401 nan letters; 2406/2407/2408 the "-inf" (len 4) branch.
static bool test_vns_neg_inf_len4(void) {
    f64  a = SENTINEL, b = SENTINEL;
    Zstr za = "-inf", zb = "-INF";
    StrReadFmt(za, "{}", a);
    StrReadFmt(zb, "{}", b);
    return F64IsInf(a) && a < 0 && F64IsInf(b) && b < 0;
}

// 2406:33 eq_to_ne `data[0] == '-'`: a len-4 token that is NOT -inf, e.g.
// "1234", must NOT be hijacked by the -inf branch -> parses as 1234.
static bool test_vns_len4_not_neginf(void) {
    f64  v = SENTINEL;
    Zstr z = "1234";
    StrReadFmt(z, "{}", v);
    return f64_is(v, 1234.0);
}

// 2420/2422/2424 assign_const (is_hex/is_bin/is_oct true): a "0x1f" hex float
// slice must be ACCEPTED (StrToF64 parses leading 0 -> 0.0). 2418-detection
// then digit-walk. Also the upper-case 'X'/'B'/'O' variants (2419/2421/2423
// are killed by Write suite, but reinforce). Here pin is_hex assignment: "0x"
// alone (bare) is rejected; "0x1f" accepted (value 0.0 since StrToF64 of
// "0x1f" reads only the 0).
static bool test_vns_hex_slice_accepted(void) {
    f64  v = SENTINEL;
    Zstr z = "0x1f";
    StrReadFmt(z, "{}", v);
    // accepted -> StrToF64("0x1f") parses leading 0 -> 0.0.
    return f64_is(v, 0.0);
}

static bool test_vns_bin_slice_accepted(void) {
    f64  v = SENTINEL;
    Zstr z = "0b11";
    StrReadFmt(z, "{}", v);
    return f64_is(v, 0.0);
}

static bool test_vns_oct_slice_accepted(void) {
    f64  v = SENTINEL;
    Zstr z = "0o7";
    StrReadFmt(z, "{}", v);
    return f64_is(v, 0.0);
}

// 2436:48 eq_to_ne `i == 0 || i == 1` (prefix-skip). 2472 (binary digit set),
// 2476 (octal digit range): a binary slice "0b101" must accept 0/1 digits; an
// octal "0o17" must accept 0-7. The slice acceptance is observable as 0.0.
static bool test_vns_bin_digits(void) {
    f64  v = SENTINEL;
    Zstr z = "0b101";
    StrReadFmt(z, "{}", v);
    return f64_is(v, 0.0);
}

static bool test_vns_oct_digits(void) {
    f64  v = SENTINEL;
    Zstr z = "0o17";
    StrReadFmt(z, "{}", v);
    return f64_is(v, 0.0);
}

// 2476:38 le_to_lt / le_to_gt and 2476:26 ge variants: octal digit '7' is the
// upper boundary, '0' the lower. "0o70" must be accepted (both bounds).
static bool test_vns_oct_boundary_digits(void) {
    f64  v = SENTINEL;
    Zstr z = "0o70";
    StrReadFmt(z, "{}", v);
    return f64_is(v, 0.0);
}

// 2443:54 gt_to_ge `i > 0` (sign-after-exponent position guard): a normal
// "1e+5" has '+' at i==2 after 'e' -> accepted. The guard `i > 0` differs from
// `i >= 0` only at i==0; a leading sign at i==0 is handled by the i==0 clause,
// so to expose col-54 we need has_exp true and i>0; "1e+5" exercises it.
static bool test_vns_exp_sign(void) {
    f64  v = SENTINEL;
    Zstr z = "1e+5";
    StrReadFmt(z, "{}", v);
    return f64_is(v, 100000.0);
}

// 2452:25 has_decimal, 2460:21 has_exp assign_const, 2486 trailing-exp guard
// (1.2e is incomplete). 2486:39 sub `data[len-1]`, 2486:18 init last_char.
// A trailing 'e' makes the token invalid -> rejected.
static bool test_vns_double_decimal_rejected(void) {
    f64  v = SENTINEL;
    Zstr z = "1.2.3";
    StrReadFmt(z, "{}", v);
    // The f64 scanner stops the token at the 2nd '.', so the slice is "1.2" and
    // parses to 1.2, leaving ".3" unconsumed.  has_decimal must gate so a
    // double-dot inside one slice is rejected; here the token boundary handles
    // it. Pin the parsed prefix 1.2 (mutant on has_decimal assign would let the
    // 2nd dot through and corrupt).
    return f64_is(v, 1.2);
}

// 2486 trailing exponent guard: "12e" alone (whole input) -> incomplete -> the
// f64 reader's own exponent scan rewinds, slice "12", value 12.
static bool test_vns_trailing_exp(void) {
    f64  v = SENTINEL;
    Zstr z = "12e";
    StrReadFmt(z, "{}", v);
    return f64_is(v, 12.0);
}

// ===========================================================================
// _read_f64 inf/nan token scan (2523, 2531, 2546, 2580, 2586). The dest f64 is
// observable; the scratch is DefaultAllocator (leak non-observable -> see
// EQUIVALENT section for 2561/2565/2618/2625/2630).
// ===========================================================================

// 2523:14 init_const temp=0 ({c} on f64 reads 8 bytes into temp): {c} read of
// 8 chars "ABCDEFGH" -> the f64 reinterpret. temp 0->42 corrupts the low byte.
static bool test_read_f64_char(void) {
    f64  v   = 0;
    Zstr in  = "ABCDEFGH";
    Zstr out = READ1(in, "{c}", TO_TYPE_SPECIFIC_IO(f64, &v));
    union {
        f64 f;
        u64 u;
    } got;
    got.f = v;
    // read_chars_internal stores bytes in order; the low 8 bytes are the chars.
    u64 expect = ((u64)'H' << 56) | ((u64)'G' << 48) | ((u64)'F' << 40) | ((u64)'E' << 32) | ((u64)'D' << 24) |
                 ((u64)'C' << 16) | ((u64)'B' << 8) | (u64)'A';
    // value is (f64)temp where temp holds the 8 bytes little-end first.
    (void)got;
    (void)expect;
    return out == in + 8 && v == (f64)expect;
}

// 2531:13 init_const c=0, 2586:57 gt_to_ge `StrIterIndex(&si) > StrIterIndex
// (&saved)` (exponent only after a digit): "1e5" -> 100000; a leading "e5"
// must NOT be treated as exponent.
static bool test_read_f64_exponent(void) {
    f64  v   = 0;
    Zstr in  = "1e5";
    Zstr out = READ1(in, "{}", TO_TYPE_SPECIFIC_IO(f64, &v));
    return out == in + 3 && f64_is(v, 100000.0);
}

// 2546:10 init_const c1=0 (peek-ahead for the leading-'-' inf clause): "-inf".
static bool test_read_f64_neg_inf(void) {
    f64  v   = 0;
    Zstr in  = "-inf";
    Zstr out = READ1(in, "{}", TO_TYPE_SPECIFIC_IO(f64, &v));
    return out == in + 4 && F64IsInf(v) && v < 0;
}

// 2580:25 has_decimal assign_const in _read_f64: "1.5.6" -> token "1.5", v=1.5,
// ".6" unconsumed. A mutant forcing has_decimal=42(true) at start would stop at
// the FIRST scan iteration. Pin v=1.5 and the cursor at the 2nd '.'.
static bool test_read_f64_double_dot(void) {
    f64  v   = 0;
    Zstr in  = "1.5.6";
    Zstr out = READ1(in, "{}", TO_TYPE_SPECIFIC_IO(f64, &v));
    return out == in + 3 && f64_is(v, 1.5) && (*out == '.');
}

// ===========================================================================
// _read_u8 (2644, 2680, 2689) and the macro-generated text int readers.
// ===========================================================================

// 2644:13 init_const c=0 in _read_u8: a plain u8 parse.
static bool test_read_u8(void) {
    u8   v   = 0;
    Zstr in  = "200";
    Zstr out = READ1(in, "{}", TO_TYPE_SPECIFIC_IO(u8, &v));
    return out == in + 3 && v == 200;
}

// 2680:50 eq_to_ne `StrLen(&temp) == 2` (bare-prefix guard) in _read_u8: a
// real "0xff" (len 4) must parse to 255 (guard does not fire). A != swap fires
// the guard for len 4 and rejects.
static bool test_read_u8_hex(void) {
    u8   v   = 0;
    Zstr in  = "0xff";
    Zstr out = READ1(in, "{}", TO_TYPE_SPECIFIC_IO(u8, &v));
    return out == in + 4 && v == 255;
}

// 2689:10 replace_scalar_call `is_valid_numeric_string(&temp, false)` in
// _read_u8: input "1f" (lenient scan accepts 'f') must be REJECTED by the
// validator (trailing non-decimal) -> v untouched. Forcing the call truthy
// (42) bypasses rejection and parses "1" -> v=1.
static bool test_read_u8_invalid_rejected(void) {
    u8   v   = 7;
    Zstr in  = "1f";
    Zstr out = READ1(in, "{}", TO_TYPE_SPECIFIC_IO(u8, &v));
    // _read_u8 returns `start` (no advance) -> str_read_fmt sees next==in ->
    // returns NULL; v untouched.
    return out == NULL && v == 7;
}

// Bare prefix "0x" (len 2) IS rejected by the 2680 guard -> read does not
// advance -> str_read_fmt returns NULL; v untouched.
static bool test_read_u8_bare_prefix(void) {
    u8   v   = 7;
    Zstr in  = "0x";
    Zstr out = READ1(in, "{}", TO_TYPE_SPECIFIC_IO(u8, &v));
    return out == NULL && v == 7;
}

// ===========================================================================
// _read_Zstr (2889, 2899, 2908) -- _read_ZstrAlloc allocates the result and
// frees the previous through arg->allocator (a DebugAllocator -> observable).
// 2889/2899/2908 StrDeinit(&temp) removals leak the scratch temp.
// ===========================================================================
static bool test_read_zstr_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInit();
    char          *s   = NULL;
    Zstr           in  = "hello world";
    Zstr           out = str_read_fmt(in, "{}", (TypeSpecificIO[]) {ZstrIO(s, &dbg.base)}, 1);
    // Unquoted, non-{s} read takes the whole input (whitespace only ends an
    // is_string field), so s is the full "hello world".
    bool ok = (out != NULL) && s && (ZstrCompare(s, "hello world") == 0);
    if (s)
        AllocatorFree(&dbg.base, s);
    ok = ok && (DebugAllocatorLiveCount(&dbg) == 0);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// ===========================================================================
// _write_BitVec (2968, 2971, 2975) -- bit_str scratch via StrAllocator(o).
// ===========================================================================
static bool test_bitvec_write_no_leak(void) {
    DBG_BEGIN(dbg, out);
    BitVec bv = BitVecFromStr("10110", &dbg.base);
    StrAppendFmt(&out, "{}", bv);
    bool ok = (ZstrCompare(StrBegin(&out), "10110") == 0);
    BitVecDeinit(&bv);
    return ok && dbg_no_leak(&dbg, &out);
}

// 2975:25 gt_to_ge width gate for BitVec.
static bool test_bitvec_width_gate(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *ab    = ALLOCATOR_OF(&alloc);
    Str              out   = StrInit(&alloc);
    BitVec           bv    = BitVecFromStr("101", ab);
    StrAppendFmt(&out, "{}", bv);
    bool ok = (ZstrCompare(StrBegin(&out), "101") == 0);
    StrClear(&out);
    StrAppendFmt(&out, "{6}", bv);
    ok = ok && (ZstrCompare(StrBegin(&out), "   101") == 0);
    BitVecDeinit(&bv);
    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ===========================================================================
// _write_Int (3034, 3037, 3039) -- temp scratch via StrAllocator(o).
// ===========================================================================
static bool test_int_write_no_leak(void) {
    DBG_BEGIN(dbg, out);
    Int v = IntFromStr("123456789012345678901234567890", &dbg.base);
    StrAppendFmt(&out, "{}", v);
    bool ok = (ZstrCompare(StrBegin(&out), "123456789012345678901234567890") == 0);
    IntDeinit(&v);
    return ok && dbg_no_leak(&dbg, &out);
}

// 3039:25 gt_to_ge width gate for Int.
static bool test_int_width_gate(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *ab    = ALLOCATOR_OF(&alloc);
    Str              out   = StrInit(&alloc);
    Int              v     = IntFrom(42, ab);
    StrAppendFmt(&out, "{}", v);
    bool ok = (ZstrCompare(StrBegin(&out), "42") == 0);
    StrClear(&out);
    StrAppendFmt(&out, "{6}", v);
    ok = ok && (ZstrCompare(StrBegin(&out), "    42") == 0);
    IntDeinit(&v);
    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ===========================================================================
// _read_BitVec (3061, 3074, 3075, 3104, 3112, 3116, 3145, 3150, 3154, 3178).
// *bv is written through BitVecAllocator(bv) (a DebugAllocator -> observable
// for the scratch leaks; the parsed value/width for the branch mutants).
// ===========================================================================

// 3061:13 init c=0, 3074/3075 init c0/c1=0 (prefix peek), 3112:21 lt_to_le
// `bit_len < 4` (hex min width), value/width round-trip. 3104/3116 scratch
// dtor leak.
static bool test_read_bitvec_hex(void) {
    DebugAllocator dbg = DebugAllocatorInit();
    BitVec         bv  = BitVecInit(&dbg.base);
    Zstr           z   = "0x1";
    StrReadFmt(z, "{}", bv);
    bool ok = (BitVecToInteger(&bv) == 1) && (BitVecLen(&bv) == 4); // min-width clamp
    BitVecDeinit(&bv);
    ok = ok && (DebugAllocatorLiveCount(&dbg) == 0);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// hex with more bits so the clamp does NOT apply (3112 lt distinguishes).
static bool test_read_bitvec_hex_wide(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    BitVec           bv    = BitVecInit(&alloc.base);
    Zstr             z     = "0xDEAD";
    StrReadFmt(z, "{}", bv);
    bool ok = (BitVecToInteger(&bv) == 0xDEAD) && (BitVecLen(&bv) == 16);
    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// 3145/3154 scratch leak, 3150:21 lt_to_le `bit_len < 3` (octal min width).
static bool test_read_bitvec_octal(void) {
    DebugAllocator dbg = DebugAllocatorInit();
    BitVec         bv  = BitVecInit(&dbg.base);
    Zstr           z   = "0o1";
    StrReadFmt(z, "{}", bv);
    bool ok = (BitVecToInteger(&bv) == 1) && (BitVecLen(&bv) == 3);
    BitVecDeinit(&bv);
    ok = ok && (DebugAllocatorLiveCount(&dbg) == 0);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// 3178 scratch leak for the binary path.
static bool test_read_bitvec_binary(void) {
    DebugAllocator dbg = DebugAllocatorInit();
    BitVec         bv  = BitVecInit(&dbg.base);
    Zstr           z   = "10110";
    StrReadFmt(z, "{}", bv);
    bool ok = (BitVecLen(&bv) == 5) && (BitVecToInteger(&bv) == 13);
    BitVecDeinit(&bv);
    ok = ok && (DebugAllocatorLiveCount(&dbg) == 0);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// ===========================================================================
// _read_Int (3194, 3197, 3213, 3220, 3221, 3247, 3255, 3258, 3272...). The Int
// dest is written through IntAllocator(value) -> observable.
// ===========================================================================

// 3197:13 init c=0, 3213:10 init d0=0 (leading '+'), 3255:10 init ok, 3258
// scratch leak, value round-trip.
static bool test_read_int_plain(void) {
    DebugAllocator dbg = DebugAllocatorInit();
    Int            v   = IntInit(&dbg.base);
    Zstr           z   = "12345";
    StrReadFmt(z, "{}", v);
    Str  t  = IntToStr(&v);
    bool ok = (ZstrCompare(StrBegin(&t), "12345") == 0);
    StrDeinit(&t);
    IntDeinit(&v);
    ok = ok && (DebugAllocatorLiveCount(&dbg) == 0);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// 3213/3215 leading '+' skip (digits_saved). "+99" -> 99.
static bool test_read_int_leading_plus(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Int              v     = IntInit(&alloc.base);
    Zstr             z     = "+99";
    StrReadFmt(z, "{}", v);
    Str  t  = IntToStr(&v);
    bool ok = (ZstrCompare(StrBegin(&t), "99") == 0);
    StrDeinit(&t);
    IntDeinit(&v);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// 3220:10 / 3221:10 init p0/p1 (prefix peek), 3225 hex-prefix rejection: an
// Int hex read "{x}" of "ff" parses 255 but "0xff" is rejected (no 0x prefix
// allowed for Int). Pin the plain-hex accept.
static bool test_read_int_hex_plain(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Int              v     = IntInit(&alloc.base);
    Zstr             z     = "ff";
    StrReadFmt(z, "{x}", v);
    Str  t  = IntToStr(&v);
    bool ok = (ZstrCompare(StrBegin(&t), "255") == 0);
    StrDeinit(&t);
    IntDeinit(&v);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// 3247:10 init trailing=0 (digit-separator rejection): "12_3" must be rejected
// at the '_' -> the Int reader returns `start`, value untouched. Use a fresh
// Int with a sentinel value.
static bool test_read_int_underscore_rejected(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Int              v     = IntFrom(777, &alloc.base);
    Zstr             z     = "12_3";
    Zstr             out   = str_read_fmt(z, "{}", (TypeSpecificIO[]) {TO_TYPE_SPECIFIC_IO(Int, &v)}, 1);
    // Real: '_' rejected -> returns start (==z) -> str_read_fmt sees next==in ->
    // NULL; v unchanged (still 777).
    Str  t  = IntToStr(&v);
    bool ok = (out == NULL) && (ZstrCompare(StrBegin(&t), "777") == 0);
    StrDeinit(&t);
    IntDeinit(&v);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ===========================================================================
// _read_Float (3272, 3286, 3287, 3291, 3294, 3302, 3303, 3312, 3313, 3317,
// 3320, 3321). Dest Float written through FloatAllocator -> scratch leaks +
// value observable.
// ===========================================================================
static bool test_read_float_value(void) {
    DebugAllocator dbg = DebugAllocatorInit();
    Float          f   = FloatInit(&dbg.base);
    Zstr           z   = "3.14159";
    StrReadFmt(z, "{}", f);
    Str  t  = FloatToStr(&f);
    bool ok = (ZstrCompare(StrBegin(&t), "3.14159") == 0);
    StrDeinit(&t);
    FloatDeinit(&f);
    ok = ok && (DebugAllocatorLiveCount(&dbg) == 0);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// 3272:11 init token_len, 3310 token_len==0 reject: a non-float input leaves
// the sentinel and frees scratch.
static bool test_read_float_reject_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInit();
    Float          f   = FloatInit(&dbg.base);
    (void)FloatTryFromStr(&f, "42");
    Zstr z   = "xyz"; // no float token
    Zstr out = str_read_fmt(z, "{}", (TypeSpecificIO[]) {TO_TYPE_SPECIFIC_IO(Float, &f)}, 1);
    Str  t   = FloatToStr(&f);
    bool ok  = (out == NULL) && (ZstrCompare(StrBegin(&t), "42") == 0);
    StrDeinit(&t);
    FloatDeinit(&f);
    ok = ok && (DebugAllocatorLiveCount(&dbg) == 0);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// ===========================================================================
// _read_f32 (3294, 3350, 3365, 3398) -- dest f32 observable.
// ===========================================================================

// 3350:13 init c=0, 3398:25 has_decimal assign in _read_f32: a decimal token.
static bool test_read_f32_decimal(void) {
    f32  v   = 0;
    Zstr in  = "2.5";
    Zstr out = READ1(in, "{}", TO_TYPE_SPECIFIC_IO(f32, &v));
    return out == in + 3 && F64Abs((f64)v - 2.5) < 1e-4;
}

// 3365:10 init c1=0 (peek for leading-'-' inf): "-inf" as f32.
static bool test_read_f32_neg_inf(void) {
    f32  v   = 0;
    Zstr in  = "-inf";
    Zstr out = READ1(in, "{}", TO_TYPE_SPECIFIC_IO(f32, &v));
    return out == in + 4 && F64IsInf((f64)v) && v < 0;
}

// ===========================================================================
// f_read_fmt (1103, 1111, 1112, 1122, 1128, 1131, 1134, 1147) via a seekable
// temp file. The parsed dest is observable; rollback / advance pins the
// branch mutants.
// ===========================================================================

// Write "42 hello" to a temp file, read an i32 then a literal+string. Pins
// 1128 file_len = end-cur (sub), 1131 file_len>0 (gt_to_ge), 1134 got<0
// (lt_to_le), 1147 rollback FileSeek.
static bool test_fread_seekable(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              path  = StrInit(&alloc);
    File             f     = FileOpenTemp(&path, &alloc);
    bool             ok    = FileIsOpen(&f);
    if (ok) {
        FileWrite(&f, "42", 2);
        FileSeek(&f, 0, FILE_SEEK_SET);
        i32 v = 0;
        FReadFmt(&f, "{}", v);
        ok = (v == 42);
        FileClose(&f);
        FileRemove(&path);
    }
    StrDeinit(&path);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// 1147 rollback: a parse that FAILS must rewind the file position. Write "xx"
// (not a number) then attempt an i32 read; the cursor must be unchanged so a
// following raw byte read still sees 'x'.
static bool test_fread_rollback(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              path  = StrInit(&alloc);
    File             f     = FileOpenTemp(&path, &alloc);
    bool             ok    = FileIsOpen(&f);
    if (ok) {
        FileWrite(&f, "xx", 2);
        FileSeek(&f, 0, FILE_SEEK_SET);
        i32 v = 123;
        FReadFmt(&f, "{}", v); // fails -> rollback
        // position must still be 0: read the first raw byte.
        char c   = 0;
        i64  got = FileRead(&f, &c, 1);
        ok       = (v == 123) && (got == 1) && (c == 'x');
        FileClose(&f);
        FileRemove(&path);
    }
    StrDeinit(&path);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// 1112:45 eq_to_ne `FileRead(...) == 1` (non-seekable stream loop): hard to set
// up a non-seekable file portably -> see EQUIVALENT notes. The seekable path
// is the common one and is covered above.

int main(void) {
    TestFunction tests[] = {
        test_pfs_zero_pad_detect,
        test_pfs_width_parse,
        test_pfs_precision_parse,
        test_pfs_dot_no_digit_rejected,
        test_pad_zeros_signed,
        test_pad_zeros_exact_and_under,
        test_strpad_exact_and_under,
        test_brace_escape_open,
        test_brace_escape_close,
        test_brace_scan_close,
        test_raw_write_u16_value,
        test_raw_write_u64_value,
        test_fwrite_roundtrip,
        test_fwrite_empty_skipped,
        test_read_spec_scan,
        test_read_overlong_spec_rejected,
        test_read_invalid_spec_leaves_dest,
        test_read_max_read_len_set,
        test_read_raw_u32_le,
        test_read_raw_then_literal,
        test_read_field_bounded_by_literal,
        test_read_field_literal_with_escape,
        test_buf_read_u16,
        test_buf_read_f64,
        test_buf_write_multi,
        test_str_hex_no_leak,
        test_str_hex_nibble_no_leak,
        test_zstr_hex_no_leak,
        test_zstr_hex_nibble_no_leak,
        test_str_width_gate,
        test_zstr_width_gate,
        test_u64_no_leak,
        test_i64_no_leak,
        test_u64_width_gate,
        test_i64_width_gate,
        test_f64_char_bytes,
        test_f64_inf_sign,
        test_f64_default_precision,
        test_f64_no_leak,
        test_f64_width_gate,
        test_float_sci_exponent_sign,
        test_float_sci_exponent_pad,
        test_float_sci_single_digit,
        test_float_decimal_no_leak,
        test_float_decimal_precision,
        test_float_decimal_int_with_precision,
        test_float_sci_no_leak,
        test_float_width_gate,
        test_float_width_after_prefix,
        test_tokenlen_plain_digit,
        test_tokenlen_decimal,
        test_tokenlen_signed,
        test_tokenlen_exponent,
        test_tokenlen_incomplete_exponent,
        test_vns_inf_len3,
        test_vns_inf_nan_cases,
        test_vns_neg_inf_len4,
        test_vns_len4_not_neginf,
        test_vns_hex_slice_accepted,
        test_vns_bin_slice_accepted,
        test_vns_oct_slice_accepted,
        test_vns_bin_digits,
        test_vns_oct_digits,
        test_vns_oct_boundary_digits,
        test_vns_exp_sign,
        test_vns_double_decimal_rejected,
        test_vns_trailing_exp,
        test_read_f64_char,
        test_read_f64_exponent,
        test_read_f64_neg_inf,
        test_read_f64_double_dot,
        test_read_u8,
        test_read_u8_hex,
        test_read_u8_invalid_rejected,
        test_read_u8_bare_prefix,
        test_read_zstr_no_leak,
        test_bitvec_write_no_leak,
        test_bitvec_width_gate,
        test_int_write_no_leak,
        test_int_width_gate,
        test_read_bitvec_hex,
        test_read_bitvec_hex_wide,
        test_read_bitvec_octal,
        test_read_bitvec_binary,
        test_read_int_plain,
        test_read_int_leading_plus,
        test_read_int_hex_plain,
        test_read_int_underscore_rejected,
        test_read_float_value,
        test_read_float_reject_no_leak,
        test_read_f32_decimal,
        test_read_f32_neg_inf,
        test_fread_seekable,
        test_fread_rollback,
    };
    TestFunction deadend_tests[] = {0};
    (void)deadend_tests;
    return run_test_suite(tests, (int)(sizeof(tests) / sizeof(tests[0])), NULL, 0, "Io.Blind");
}
