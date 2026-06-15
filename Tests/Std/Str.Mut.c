/// file : tests/std/str.mut.c
///
/// Targeted mutation-kill tests for Str: each test drives an input that makes
/// a specific bucket-A survivor (operator swap / scalar-call replacement / leak)
/// produce an observably-wrong result, so the mutant fails the assertion. These
/// were flagged killable by the original campaign but left as ignores; this file
/// converts them to real kills. Distinct contract from the existing Str.* tests
/// -- do NOT duplicate them.

#include <Misra.h>
#include <Misra/Std/Allocator/Debug.h>
#include <Misra/Std/Allocator/Default.h>

#include "../Util/TestRunner.h"

static DefaultAllocator alloc;

// ---- 533:51 cxx_eq_to_ne -------------------------------------------------
// skip_prefix, base-2 case:
//   if (prefix_char == 'b' || prefix_char == 'B')  -> '==' becomes '!='.
// skip_prefix only runs when the caller fixes an explicit base, so feed a
// "0b" prefixed binary literal with config.base == 2. Real: the 'b' prefix is
// recognised and skipped, "0b101" parses as 101b == 5. Mutant: prefix_char
// 'b' makes `'b' != 'b'` false and `'b' == 'B'` false, so the prefix is NOT
// skipped; the parser then meets a 'b' that is not a base-2 digit and fails.
// ---- 533:51 cxx_eq_to_ne (uppercase 'B' arm) ----------------------------
// skip_prefix, base-2 case has TWO comparisons on one line:
//   if (prefix_char == 'b' || prefix_char == 'B')
// col 51 is the SECOND `==` (the 'B' arm). The earlier test feeds lowercase
// "0b...", exercising only the first comparison, so the 'B' arm survives.
// Feed an UPPERCASE "0B101" with explicit base 2. Real: 'B' is recognised and
// the prefix is skipped, "101" base 2 == 5. Mutant (`prefix_char != 'B'`):
// with prefix_char 'B', `'B' == 'b'` is false and `'B' != 'B'` is false, so
// the prefix is NOT skipped; the parser then meets 'B', not a base-2 digit,
// and fails.
bool test_to_u64_base2_uppercase_prefix_skipped(void);
bool test_to_u64_base2_uppercase_prefix_skipped(void) {
    WriteFmt("Testing StrToU64 skips 0B prefix on explicit base 2 (533:51)\n");

    Str            s      = StrInitFromZstr("0B101", &alloc);
    StrParseConfig config = {.base = 2};
    u64            value  = 0;
    bool           ok     = StrToU64(&s, &value, &config);

    // Real: uppercase prefix skipped, "101" base 2 == 5.
    bool result = ok && (value == 5);

    StrDeinit(&s);
    return result;
}

// ---- 1108:14 cxx_init_const ----------------------------------------------
// StrToF64, exponent block:
//   bool have_exp_digits = false;   -> initialiser value replaced with 42.
// 42 is truthy, so have_exp_digits starts "true" and the "Missing exponent
// digits" guard never fires. Feed "1e": an 'e' with no exponent digits.
// Real: have_exp_digits stays false -> parse fails. Mutant: starts true ->
// reports success with value 1.0.
bool test_to_f64_missing_exponent_digits_fails(void);
bool test_to_f64_missing_exponent_digits_fails(void) {
    WriteFmt("Testing StrToF64 rejects missing exponent digits (1108:14)\n");

    Str  s     = StrInitFromZstr("1e", &alloc);
    f64  value = 0.0;
    bool ok    = StrToF64(&s, &value, NULL);

    // Real: "1e" has no exponent digits -> parse must fail.
    bool result = (ok == false);

    StrDeinit(&s);
    return result;
}

int main(void) {
    alloc = DefaultAllocatorInit();

    TestFunction tests[] = {
        test_to_u64_base2_uppercase_prefix_skipped,
        test_to_f64_missing_exponent_digits_fails,
    };

    int total = sizeof(tests) / sizeof(tests[0]);
    int rc    = run_test_suite(tests, total, NULL, 0, "Str.Mut");
    DefaultAllocatorDeinit(&alloc);
    return rc;
}
