/// file : tests/std/io.leak.c
///
/// Leak-guard tests for Io: route the format-output Str (and therefore the
/// formatter's internal scratch allocations, which come from `StrAllocator(o)`)
/// through an explicit DebugAllocator and assert
/// `DebugAllocatorLiveCount(&dbg) == 0 && DebugAllocatorLiveBytes(&dbg) == 0`
/// after full cleanup, to KILL mutation survivors that remove an internal
/// `*Deinit` on an input-reachable success branch (the leak-only proposals).
///
/// Only the WRITE side of Io allocates its scratch through the caller's
/// (injectable) allocator -- `_write_Float` -> float_try_to_{decimal,scientific}_str
/// take `StrAllocator(o)` and abandon a `canonical`/`digits` scratch Str on
/// success (`*out = result`), so the trailing `StrDeinit` is load-bearing.
/// The str_patch/f_write/f_read scratch paths, and the scalar readers
/// (`_read_u8` / `_read_f64`), use an internal `DefaultAllocatorInit()` with
/// no injectable hook, so their leak-only mutants are genuinely non-observable
/// and stay ignored. The ONE read-side exception is the typed-container
/// readers (`_read_Int` / `_read_Float`): they allocate their parse scratch
/// AND overwrite `*value` through the DESTINATION container's own allocator
/// (`IntAllocator(value)` / `FloatAllocator(value)`), which the caller
/// supplies -- so a removed `*Deinit(value)` on the success path leaks the
/// PRIOR value's storage and IS observable when the destination is backed by
/// a DebugAllocator (see the `test_leak_read_*_prior_value_freed` tests).
///
/// Distinct contract from the value-correctness tests in the sibling Io.*
/// files -- these assert allocator live-count, not rendered bytes.

#include <Misra.h>
#include <Misra/Std/Allocator/Debug.h>
#include <Misra/Std/Container/BitVec.h>
#include <Misra/Std/Container/Float.h>
#include <Misra/Std/Container/Int.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Io.h>

#include "../Util/TestRunner.h"

// Leak-only config: live-count tracking, NO per-alloc stack-trace / canary /
// freed-history -- avoids the backtrace-per-alloc cost under mull. Leak
// detection (LiveCount / LiveBytes) is unchanged.
#define LEAK_CFG                                                                                                       \
    ((DebugAllocatorConfig) {.capture_traces = false, .detect_overflow = false, .track_freed_history = false})

// ---------------------------------------------------------------------------
// float_try_to_scientific_str: nonzero success path abandons `digits`
// (int_try_to_str scratch) and does `*out = result` (Io.c:1499 StrDeinit).
// `{e}` of a nonzero Float renders through this path; with the output Str
// backed by a DebugAllocator, a removed StrDeinit(&digits) leaks the
// significand-string scratch -> live count != 0 after cleanup.
// ---------------------------------------------------------------------------
bool test_leak_sci_nonzero_digits_freed(void) {
    DebugAllocator dbg  = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    // Float backed by a SEPARATE heap so only the formatter scratch shows
    // up in dbg's live count.
    HeapAllocator fa = HeapAllocatorInit();
    Float         v  = FloatFromStr("12345.67", ALLOCATOR_OF(&fa));

    Str  out = StrInit(adbg);
    bool ok  = StrAppendFmt(&out, "{e}", v);
    ok       = ok && (StrLen(&out) > 0);

    StrDeinit(&out);
    ok = ok && (DebugAllocatorLiveCount(&dbg) == 0);
    ok = ok && (DebugAllocatorLiveBytes(&dbg) == 0);

    FloatDeinit(&v);
    HeapAllocatorDeinit(&fa);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// ---------------------------------------------------------------------------
// float_try_to_scientific_str: zero-value success path also abandons
// `digits` (Io.c:1462 StrDeinit) before `*out = result`. `{.3e}` of 0.0
// reaches the FloatIsZero branch.
// ---------------------------------------------------------------------------
bool test_leak_sci_zero_digits_freed(void) {
    DebugAllocator dbg  = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    HeapAllocator fa = HeapAllocatorInit();
    Float         v  = FloatFromStr("0.0", ALLOCATOR_OF(&fa));

    Str  out = StrInit(adbg);
    bool ok  = StrAppendFmt(&out, "{.3e}", v);
    ok       = ok && (StrLen(&out) > 0);

    StrDeinit(&out);
    ok = ok && (DebugAllocatorLiveCount(&dbg) == 0);
    ok = ok && (DebugAllocatorLiveBytes(&dbg) == 0);

    FloatDeinit(&v);
    HeapAllocatorDeinit(&fa);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// ---------------------------------------------------------------------------
// float_try_to_decimal_str: with-dot success path abandons `canonical`
// (float_try_to_str scratch) and does `*out = result` (Io.c:1404 StrDeinit).
// `{.2}` of a value whose canonical form HAS a '.' takes the dot branch.
// ---------------------------------------------------------------------------
bool test_leak_decimal_withdot_canonical_freed(void) {
    DebugAllocator dbg  = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    HeapAllocator fa = HeapAllocatorInit();
    Float         v  = FloatFromStr("3.14159", ALLOCATOR_OF(&fa));

    Str  out = StrInit(adbg);
    bool ok  = StrAppendFmt(&out, "{.2}", v);
    ok       = ok && (StrLen(&out) > 0);

    StrDeinit(&out);
    ok = ok && (DebugAllocatorLiveCount(&dbg) == 0);
    ok = ok && (DebugAllocatorLiveBytes(&dbg) == 0);

    FloatDeinit(&v);
    HeapAllocatorDeinit(&fa);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// ---------------------------------------------------------------------------
// float_try_to_decimal_str: no-dot success path abandons `canonical` and
// does `*out = result` (Io.c:1379 StrDeinit). An integer-valued Float whose
// canonical form has NO '.' but with an explicit precision takes the
// `if (!dot)` branch.
// ---------------------------------------------------------------------------
bool test_leak_decimal_nodot_canonical_freed(void) {
    DebugAllocator dbg  = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    HeapAllocator fa = HeapAllocatorInit();
    Float         v  = FloatFromStr("42", ALLOCATOR_OF(&fa));

    Str  out = StrInit(adbg);
    bool ok  = StrAppendFmt(&out, "{.2}", v);
    ok       = ok && (StrLen(&out) > 0);

    StrDeinit(&out);
    ok = ok && (DebugAllocatorLiveCount(&dbg) == 0);
    ok = ok && (DebugAllocatorLiveBytes(&dbg) == 0);

    FloatDeinit(&v);
    HeapAllocatorDeinit(&fa);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// ---------------------------------------------------------------------------
// READ side, typed-container reader exception to the "all reads use internal
// DefaultAllocator scratch" rule documented in the header above. _read_Int /
// _read_Float allocate their parse scratch -- and overwrite `*value` -- through
// the DESTINATION container's OWN allocator (IntAllocator(value) /
// FloatAllocator(value)), which the caller supplies. So a removed internal
// *Deinit on the read success path IS observable when the destination is backed
// by a DebugAllocator, unlike the scalar readers (_read_u8 / _read_f64 /
// f_read_fmt) which truly use an uninjectable DefaultAllocatorInit() scratch.
//
// _read_Int success path: `IntDeinit(value)` then `*value = parsed`
// (Io.c:3262). The destination Int is pre-seeded with a heap-backed value, so
// dropping that IntDeinit leaks the PRIOR significand limbs -> live count != 0
// after the final IntDeinit.
// ---------------------------------------------------------------------------
bool test_leak_read_int_prior_value_freed(void) {
    DebugAllocator dbg  = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    // Seed the destination with a heap-backed Int (multi-limb) so the old
    // storage is visible in dbg's live count before the reassignment.
    Int value = IntFromStr("123456789012345678901234567890", adbg);

    Zstr input = "42";
    Zstr start = input;
    StrReadFmt(input, "{}", value);
    bool ok = (input != start); // pointer advanced => read succeeded

    IntDeinit(&value);
    ok = ok && (DebugAllocatorLiveCount(&dbg) == 0);
    ok = ok && (DebugAllocatorLiveBytes(&dbg) == 0);

    DebugAllocatorDeinit(&dbg);
    return ok;
}

// ---------------------------------------------------------------------------
// _read_Float success path: `FloatDeinit(value)` then `*value = parsed`
// (Io.c:3325). The destination Float is pre-seeded with a heap-backed
// significand, so dropping that FloatDeinit leaks the PRIOR significand
// storage -> live count != 0 after the final FloatDeinit.
// ---------------------------------------------------------------------------
bool test_leak_read_float_prior_value_freed(void) {
    DebugAllocator dbg  = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    // Seed with a heap-backed Float (wide significand) so the old significand
    // storage is visible in dbg's live count before the reassignment.
    Float value = FloatFromStr("987654321098765432109876543210.5", adbg);

    Zstr input = "2.5";
    Zstr start = input;
    StrReadFmt(input, "{}", value);
    bool ok = (input != start); // pointer advanced => read succeeded

    FloatDeinit(&value);
    ok = ok && (DebugAllocatorLiveCount(&dbg) == 0);
    ok = ok && (DebugAllocatorLiveBytes(&dbg) == 0);

    DebugAllocatorDeinit(&dbg);
    return ok;
}

// ---------------------------------------------------------------------------
// WRITE side, _write_Zstr HEX path: each loop iteration builds a per-byte
// `hex` Str through `StrAllocator(o)` (StrFromU64 allocates the nibble
// string), merges it into `o`, then `StrDeinit(&hex)` (Io.c:1754). A
// multi-char Zstr formatted with `{x}` drives the loop >=2 times; with the
// output Str backed by a DebugAllocator, a removed StrDeinit(&hex) leaks the
// per-byte scratch -> live count != 0 after cleanup.
// ---------------------------------------------------------------------------
bool test_leak_write_zstr_hex_per_byte_freed(void) {
    DebugAllocator dbg  = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    Zstr s   = "AB"; // multi-byte Zstr -> hex loop runs per byte
    Str  out = StrInit(adbg);
    bool ok  = StrAppendFmt(&out, "{x}", s);
    ok       = ok && (StrLen(&out) > 0);

    StrDeinit(&out);
    ok = ok && (DebugAllocatorLiveCount(&dbg) == 0);
    ok = ok && (DebugAllocatorLiveBytes(&dbg) == 0);

    DebugAllocatorDeinit(&dbg);
    return ok;
}

// ---------------------------------------------------------------------------
// WRITE side, _write_i64 success path: the value is rendered into a scratch
// `temp` Str built through `StrAllocator(o)` (StrFromI64 allocates), merged
// into `o`, then `StrDeinit(&temp)` (Io.c:1955). `{}` of a signed integer
// reaches this; with the output Str backed by a DebugAllocator, a removed
// StrDeinit(&temp) leaks the formatting scratch -> live count != 0.
// ---------------------------------------------------------------------------
bool test_leak_write_i64_temp_freed(void) {
    DebugAllocator dbg  = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    i32  v   = -123456;
    Str  out = StrInit(adbg);
    bool ok  = StrAppendFmt(&out, "{}", v);
    ok       = ok && (StrLen(&out) > 0);

    StrDeinit(&out);
    ok = ok && (DebugAllocatorLiveCount(&dbg) == 0);
    ok = ok && (DebugAllocatorLiveBytes(&dbg) == 0);

    DebugAllocatorDeinit(&dbg);
    return ok;
}

// ---------------------------------------------------------------------------
// Remaining WRITE-side scratch leaks. Every `_write_<T>` builds its rendering
// in a scratch Str allocated through `StrAllocator(o)` and frees it with a
// trailing `StrDeinit`; with `o` backed by a DebugAllocator a removed Deinit
// leaks. One test per writer / format path, with container values backed by a
// SEPARATE heap so only the formatter scratch shows in dbg's live count.
// ---------------------------------------------------------------------------
#define LEAK_WRITE_PRELUDE()                                                                                           \
    DebugAllocator dbg  = DebugAllocatorInitWith(LEAK_CFG);                                                            \
    Allocator     *adbg = ALLOCATOR_OF(&dbg);                                                                          \
    Str            out  = StrInit(adbg);                                                                               \
    bool           ok   = true

#define LEAK_WRITE_EPILOGUE()                                                                                          \
    StrDeinit(&out);                                                                                                   \
    ok = ok && (DebugAllocatorLiveCount(&dbg) == 0) && (DebugAllocatorLiveBytes(&dbg) == 0);                           \
    DebugAllocatorDeinit(&dbg);                                                                                        \
    return ok

bool test_leak_write_u64_temp_freed(void) {
    LEAK_WRITE_PRELUDE();
    ok = ok && StrAppendFmt(&out, "{}", (u64)12345678901234ull) && (StrLen(&out) > 0);
    LEAK_WRITE_EPILOGUE();
}

bool test_leak_write_f64_temp_freed(void) {
    LEAK_WRITE_PRELUDE();
    ok = ok && StrAppendFmt(&out, "{}", (f64)3.14159) && (StrLen(&out) > 0);
    LEAK_WRITE_EPILOGUE();
}

bool test_leak_write_str_freed(void) {
    HeapAllocator va = HeapAllocatorInit();
    Str           s  = StrInit(ALLOCATOR_OF(&va));
    StrAppendFmt(&s, "payload");
    LEAK_WRITE_PRELUDE();
    ok = ok && StrAppendFmt(&out, "{}", s) && (StrLen(&out) > 0);
    StrDeinit(&out);
    ok = ok && (DebugAllocatorLiveCount(&dbg) == 0) && (DebugAllocatorLiveBytes(&dbg) == 0);
    DebugAllocatorDeinit(&dbg);
    StrDeinit(&s);
    HeapAllocatorDeinit(&va);
    return ok;
}

bool test_leak_write_int_freed(void) {
    HeapAllocator va = HeapAllocatorInit();
    Int           v  = IntFromStr("123456789012345678901234567890", ALLOCATOR_OF(&va));
    LEAK_WRITE_PRELUDE();
    ok = ok && StrAppendFmt(&out, "{}", v) && (StrLen(&out) > 0);
    StrDeinit(&out);
    ok = ok && (DebugAllocatorLiveCount(&dbg) == 0) && (DebugAllocatorLiveBytes(&dbg) == 0);
    DebugAllocatorDeinit(&dbg);
    IntDeinit(&v);
    HeapAllocatorDeinit(&va);
    return ok;
}

bool test_leak_write_float_freed(void) {
    HeapAllocator va = HeapAllocatorInit();
    Float         v  = FloatFromStr("2.718281828", ALLOCATOR_OF(&va));
    LEAK_WRITE_PRELUDE();
    ok = ok && StrAppendFmt(&out, "{}", v) && (StrLen(&out) > 0);
    StrDeinit(&out);
    ok = ok && (DebugAllocatorLiveCount(&dbg) == 0) && (DebugAllocatorLiveBytes(&dbg) == 0);
    DebugAllocatorDeinit(&dbg);
    FloatDeinit(&v);
    HeapAllocatorDeinit(&va);
    return ok;
}

bool test_leak_write_bitvec_freed(void) {
    HeapAllocator va = HeapAllocatorInit();
    BitVec        v  = BitVecInit(ALLOCATOR_OF(&va));
    BitVecResize(&v, 12);
    LEAK_WRITE_PRELUDE();
    ok = ok && StrAppendFmt(&out, "{}", v) && (StrLen(&out) > 0);
    StrDeinit(&out);
    ok = ok && (DebugAllocatorLiveCount(&dbg) == 0) && (DebugAllocatorLiveBytes(&dbg) == 0);
    DebugAllocatorDeinit(&dbg);
    BitVecDeinit(&v);
    HeapAllocatorDeinit(&va);
    return ok;
}

// ---------------------------------------------------------------------------
// Per-format-path WRITE-side scratch frees (mull A-kills): each writer
// allocates its render scratch through StrAllocator(o) on a SPECIFIC format
// flag and frees it with a trailing StrDeinit. Exercise that exact flag path.
// ---------------------------------------------------------------------------

// 1794: _write_Str `{x}` (FMT_FLAG_HEX) per-byte `hex` scratch.
bool test_leak_write_str_hex_per_byte_freed(void) {
    HeapAllocator sa = HeapAllocatorInit();
    Str           s  = StrInit(ALLOCATOR_OF(&sa));
    for (int i = 0; i < 8; ++i)
        StrPushBackR(&s, (char)0xAB);
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Str            out = StrInit(ALLOCATOR_OF(&dbg));
    bool           ok  = StrAppendFmt(&out, "{x}", s) && (StrLen(&out) > 0);
    StrDeinit(&out);
    ok = ok && (DebugAllocatorLiveCount(&dbg) == 0) && (DebugAllocatorLiveBytes(&dbg) == 0);
    DebugAllocatorDeinit(&dbg);
    StrDeinit(&s);
    HeapAllocatorDeinit(&sa);
    return ok;
}

// 2210: _write_f64 finite default-decimal `temp` scratch.
bool test_leak_write_f64_default_freed(void) {
    LEAK_WRITE_PRELUDE();
    ok = ok && StrAppendFmt(&out, "{}", 1e300) && (StrLen(&out) > 0);
    LEAK_WRITE_EPILOGUE();
}

// 2286: _write_Float default-decimal `temp` (via float_try_to_decimal_str).
bool test_leak_write_float_default_freed(void) {
    HeapAllocator fa = HeapAllocatorInit();
    Float         f  = FloatFromStr("12345678901234567890123456789012345678901234567890.5", ALLOCATOR_OF(&fa));
    LEAK_WRITE_PRELUDE();
    ok = ok && StrAppendFmt(&out, "{}", f) && (StrLen(&out) > 0);
    StrDeinit(&out);
    ok = ok && (DebugAllocatorLiveCount(&dbg) == 0) && (DebugAllocatorLiveBytes(&dbg) == 0);
    DebugAllocatorDeinit(&dbg);
    FloatDeinit(&f);
    HeapAllocatorDeinit(&fa);
    return ok;
}

// ---------------------------------------------------------------------------
// READ-side error/budget paths that free the DESTINATION Str (backed by the
// caller's allocator) -- the reader frees `s` on these branches, so the test
// must NOT free it again. The format-read budget binds before the input ends
// via an interior literal anchor, so the unterminated/error branch runs.
// ---------------------------------------------------------------------------

// 2408 / 2421 / 2466: quoted-string read whose budget saturates before the
// closing quote -> unterminated-quote branch frees the destination Str.
static bool leak_quoted_unterminated(Zstr input, Zstr fmt) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Str            s   = StrInit(ALLOCATOR_OF(&dbg));
    Zstr           p   = input;
    StrReadFmt(p, fmt, s); // reader frees s on the unterminated/error branch
    bool ok = (DebugAllocatorLiveCount(&dbg) == 0) && (DebugAllocatorLiveBytes(&dbg) == 0);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_leak_read_quoted_budget_escape_freed(void) {
    // open-quote + 24 \n escapes + 'E' + close-quote, fmt "{s}E".
    return leak_quoted_unterminated(
        "\"\\n\\n\\n\\n\\n\\n\\n\\n\\n\\n\\n\\n\\n\\n\\n\\n\\n\\n\\n\\n\\n\\n\\n\\nE\"",
        "{s}E"
    );
}

bool test_leak_read_quoted_budget_plain_freed(void) {
    return leak_quoted_unterminated("\"aaaaaaaaaaaaaaaaaaaaaaaa#\"", "{s}#");
}

bool test_leak_read_quoted_unterminated_freed(void) {
    return leak_quoted_unterminated("\"aaaaaaaaaaaaaaaaaaaaaaaa", "{s}");
}

// 2439: unquoted invalid-escape error path frees the destination Str.
bool test_leak_read_unquoted_bad_escape_freed(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Str            s   = StrInit(ALLOCATOR_OF(&dbg));
    Zstr           p   = "aaaaaaaaaaaaaaaaaaaaaaaa\\q"; // 24 'a' + invalid \q
    StrReadFmt(p, "{}", s);
    bool ok = (DebugAllocatorLiveCount(&dbg) == 0) && (DebugAllocatorLiveBytes(&dbg) == 0);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// 3232 / 3273: BitVec hex/oct read overflow error path frees the internal
// scratch (allocated through the destination BitVec's allocator).
static bool leak_bitvec_overflow(Zstr input) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    BitVec         bv  = BitVecInit(ALLOCATOR_OF(&dbg));
    Zstr           p   = input;
    StrReadFmt(p, "{}", bv);
    BitVecDeinit(&bv);
    bool ok = (DebugAllocatorLiveCount(&dbg) == 0) && (DebugAllocatorLiveBytes(&dbg) == 0);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_leak_read_bitvec_hex_overflow_freed(void) {
    return leak_bitvec_overflow("0xFFFFFFFFFFFFFFFFF"); // 17 hex digits > 64 bits
}

bool test_leak_read_bitvec_oct_overflow_freed(void) {
    return leak_bitvec_overflow("0o7777777777777777777777"); // 22 oct digits > 64 bits
}

// 3448: Float read whose token passes length-scan but FloatTryFromStr fails on
// exponent overflow -> fail branch frees the internal `temp` scratch.
bool test_leak_read_float_exp_overflow_freed(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Float          fv  = FloatInit(ALLOCATOR_OF(&dbg));
    Zstr           p   = "1e99999999999999999999999999999999999999999999999999";
    StrReadFmt(p, "{}", fv);
    FloatDeinit(&fv);
    bool ok = (DebugAllocatorLiveCount(&dbg) == 0) && (DebugAllocatorLiveBytes(&dbg) == 0);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

int main(void) {
    TestFunction tests[] = {
        test_leak_write_str_hex_per_byte_freed,
        test_leak_write_f64_default_freed,
        test_leak_write_float_default_freed,
        test_leak_read_quoted_budget_escape_freed,
        test_leak_read_quoted_budget_plain_freed,
        test_leak_read_quoted_unterminated_freed,
        test_leak_read_unquoted_bad_escape_freed,
        test_leak_read_bitvec_hex_overflow_freed,
        test_leak_read_bitvec_oct_overflow_freed,
        test_leak_read_float_exp_overflow_freed,
        test_leak_sci_nonzero_digits_freed,
        test_leak_sci_zero_digits_freed,
        test_leak_decimal_withdot_canonical_freed,
        test_leak_decimal_nodot_canonical_freed,
        test_leak_read_int_prior_value_freed,
        test_leak_read_float_prior_value_freed,
        test_leak_write_zstr_hex_per_byte_freed,
        test_leak_write_i64_temp_freed,
        test_leak_write_u64_temp_freed,
        test_leak_write_f64_temp_freed,
        test_leak_write_str_freed,
        test_leak_write_int_freed,
        test_leak_write_float_freed,
        test_leak_write_bitvec_freed,
    };
    TestFunction deadend_tests[] = {
        0,
    };
    (void)deadend_tests;
    return run_test_suite(tests, (int)(sizeof(tests) / sizeof(tests[0])), deadend_tests, 0, "Io.Leak");
}
