#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Std/ArgParse.h>
#include <Misra/Std/Log.h>

#include "../Util/TestRunner.h"

// ----------------------------------------------------------------------------
// Each verb in isolation
// ----------------------------------------------------------------------------

static bool test_required_long_space(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("prog", NULL, &a);

    Zstr listen = NULL;
    ArgRequired(&p, "-l", "--listen", &listen, "host:port");

    char  *argv[] = {(char *)"prog", (char *)"--listen", (char *)"0.0.0.0:8080"};
    ArgRun rc     = ArgParseRun(&p, 3, argv);

    bool ok = (rc == ARG_RUN_OK) && listen && ZstrCompare(listen, "0.0.0.0:8080") == 0;
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

static bool test_required_long_equals(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("prog", NULL, &a);

    Zstr listen = NULL;
    ArgRequired(&p, "-l", "--listen", &listen, "host:port");

    char  *argv[] = {(char *)"prog", (char *)"--listen=0.0.0.0:8080"};
    ArgRun rc     = ArgParseRun(&p, 2, argv);

    bool ok = (rc == ARG_RUN_OK) && listen && ZstrCompare(listen, "0.0.0.0:8080") == 0;
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

static bool test_required_short(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("prog", NULL, &a);

    Zstr listen = NULL;
    ArgRequired(&p, "-l", "--listen", &listen, "host:port");

    char  *argv[] = {(char *)"prog", (char *)"-l", (char *)"127.0.0.1:9"};
    ArgRun rc     = ArgParseRun(&p, 3, argv);

    bool ok = (rc == ARG_RUN_OK) && listen && ZstrCompare(listen, "127.0.0.1:9") == 0;
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

static bool test_optional_default_preserved(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("prog", NULL, &a);

    u32 timeout = 30;
    ArgOptional(&p, NULL, "--timeout", &timeout, "seconds");

    char  *argv[] = {(char *)"prog"};
    ArgRun rc     = ArgParseRun(&p, 1, argv);

    bool ok = (rc == ARG_RUN_OK) && timeout == 30;
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

static bool test_optional_overrides_default(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("prog", NULL, &a);

    u32 timeout = 30;
    ArgOptional(&p, NULL, "--timeout", &timeout, "seconds");

    char  *argv[] = {(char *)"prog", (char *)"--timeout", (char *)"5"};
    ArgRun rc     = ArgParseRun(&p, 3, argv);

    bool ok = (rc == ARG_RUN_OK) && timeout == 5;
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

static bool test_flag_presence(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("prog", NULL, &a);

    bool verbose = false;
    ArgFlag(&p, "-v", "--verbose", &verbose, "verbose");

    char  *argv[] = {(char *)"prog", (char *)"--verbose"};
    ArgRun rc     = ArgParseRun(&p, 2, argv);

    bool ok = (rc == ARG_RUN_OK) && verbose == true;
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

static bool test_flag_absence(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("prog", NULL, &a);

    bool verbose = false;
    ArgFlag(&p, "-v", "--verbose", &verbose, "verbose");

    char  *argv[] = {(char *)"prog"};
    ArgRun rc     = ArgParseRun(&p, 1, argv);

    bool ok = (rc == ARG_RUN_OK) && verbose == false;
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

static bool test_count_repeated(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("prog", NULL, &a);

    u32 verbose = 0;
    ArgCount(&p, "-v", "--verbose", &verbose, "v");

    char  *argv[] = {(char *)"prog", (char *)"-v", (char *)"-v", (char *)"-v"};
    ArgRun rc     = ArgParseRun(&p, 4, argv);

    bool ok = (rc == ARG_RUN_OK) && verbose == 3;
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

static bool test_count_bundled(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("prog", NULL, &a);

    u32 verbose = 0;
    ArgCount(&p, "-v", "--verbose", &verbose, "v");

    char  *argv[] = {(char *)"prog", (char *)"-vvv"};
    ArgRun rc     = ArgParseRun(&p, 2, argv);

    bool ok = (rc == ARG_RUN_OK) && verbose == 3;
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

static bool test_positional_order(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("cp", NULL, &a);

    Zstr src = NULL;
    Zstr dst = NULL;
    ArgPositional(&p, "source", &src, "from");
    ArgPositional(&p, "dest", &dst, "to");

    char  *argv[] = {(char *)"cp", (char *)"a.txt", (char *)"b.txt"};
    ArgRun rc     = ArgParseRun(&p, 3, argv);

    bool ok = (rc == ARG_RUN_OK) && ZstrCompare(src, "a.txt") == 0 && ZstrCompare(dst, "b.txt") == 0;
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

static bool test_positional_with_interleaved_flag(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("cp", NULL, &a);

    Zstr src     = NULL;
    Zstr dst     = NULL;
    bool verbose = false;
    ArgPositional(&p, "source", &src, "from");
    ArgPositional(&p, "dest", &dst, "to");
    ArgFlag(&p, "-v", "--verbose", &verbose, "v");

    char  *argv[] = {(char *)"cp", (char *)"a.txt", (char *)"-v", (char *)"b.txt"};
    ArgRun rc     = ArgParseRun(&p, 4, argv);

    bool ok = (rc == ARG_RUN_OK) && ZstrCompare(src, "a.txt") == 0 && ZstrCompare(dst, "b.txt") == 0 && verbose == true;
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

// ----------------------------------------------------------------------------
// Type inference paths
// ----------------------------------------------------------------------------

static bool test_type_inferred_u32(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("prog", NULL, &a);

    u32 n = 0;
    ArgOptional(&p, NULL, "--n", &n, "count");

    char  *argv[] = {(char *)"prog", (char *)"--n=12345"};
    ArgRun rc     = ArgParseRun(&p, 2, argv);

    bool ok = (rc == ARG_RUN_OK) && n == 12345;
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

static bool test_type_inferred_i64_negative(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("prog", NULL, &a);

    i64 v = 0;
    ArgOptional(&p, NULL, "--v", &v, "v");

    char  *argv[] = {(char *)"prog", (char *)"--v", (char *)"-42"};
    ArgRun rc     = ArgParseRun(&p, 3, argv);

    bool ok = (rc == ARG_RUN_OK) && v == -42;
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

static bool test_type_inferred_f64(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("prog", NULL, &a);

    f64 ratio = 1.0;
    ArgOptional(&p, NULL, "--ratio", &ratio, "r");

    char  *argv[] = {(char *)"prog", (char *)"--ratio=2.5"};
    ArgRun rc     = ArgParseRun(&p, 2, argv);

    bool ok = (rc == ARG_RUN_OK) && ratio > 2.4 && ratio < 2.6;
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

static bool test_type_inferred_str(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("prog", NULL, &a);

    Str name = StrInit(&a);
    ArgOptional(&p, NULL, "--name", &name, "n");

    char  *argv[] = {(char *)"prog", (char *)"--name", (char *)"alice"};
    ArgRun rc     = ArgParseRun(&p, 3, argv);

    bool ok = (rc == ARG_RUN_OK) && StrLen(&name) == 5 && StrBegin(&name)[0] == 'a' && StrBegin(&name)[4] == 'e';
    ArgParseDeinit(&p);
    StrDeinit(&name);
    DefaultAllocatorDeinit(&a);
    return ok;
}

// ----------------------------------------------------------------------------
// Error paths
// ----------------------------------------------------------------------------

static bool test_missing_required(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("prog", NULL, &a);

    Zstr listen = NULL;
    ArgRequired(&p, "-l", "--listen", &listen, "");

    char  *argv[] = {(char *)"prog"};
    ArgRun rc     = ArgParseRun(&p, 1, argv);

    bool ok = (rc == ARG_RUN_ERROR) && listen == NULL;
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

static bool test_missing_positional(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("cp", NULL, &a);

    Zstr src = NULL;
    Zstr dst = NULL;
    ArgPositional(&p, "source", &src, "");
    ArgPositional(&p, "dest", &dst, "");

    char  *argv[] = {(char *)"cp", (char *)"a.txt"};
    ArgRun rc     = ArgParseRun(&p, 2, argv);

    bool ok = (rc == ARG_RUN_ERROR);
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

static bool test_unknown_option(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("prog", NULL, &a);

    bool v = false;
    ArgFlag(&p, "-v", "--verbose", &v, "");

    char  *argv[] = {(char *)"prog", (char *)"--bogus"};
    ArgRun rc     = ArgParseRun(&p, 2, argv);

    bool ok = (rc == ARG_RUN_ERROR);
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

static bool test_invalid_value_for_type(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("prog", NULL, &a);

    u32 n = 0;
    ArgOptional(&p, NULL, "--n", &n, "");

    char  *argv[] = {(char *)"prog", (char *)"--n", (char *)"abc"};
    ArgRun rc     = ArgParseRun(&p, 3, argv);

    bool ok = (rc == ARG_RUN_ERROR) && n == 0;
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

static bool test_u8_overflow(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("prog", NULL, &a);

    u8 v = 0;
    ArgOptional(&p, NULL, "--v", &v, "");

    char  *argv[] = {(char *)"prog", (char *)"--v=256"};
    ArgRun rc     = ArgParseRun(&p, 2, argv);

    bool ok = (rc == ARG_RUN_ERROR) && v == 0;
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

static bool test_too_many_positionals(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("prog", NULL, &a);

    Zstr x = NULL;
    ArgPositional(&p, "x", &x, "");

    char  *argv[] = {(char *)"prog", (char *)"first", (char *)"extra"};
    ArgRun rc     = ArgParseRun(&p, 3, argv);

    bool ok = (rc == ARG_RUN_ERROR);
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

// ----------------------------------------------------------------------------
// Bool value parsing. ArgOptional with a bool* target accepts the
// documented truthy/falsey spellings; anything else is a parse error.
// ----------------------------------------------------------------------------

static bool run_bool_value(Zstr text, bool *out) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("prog", NULL, &a);
    ArgOptional(&p, NULL, "--enable", out, "toggle");
    char  *argv[] = {(char *)"prog", (char *)"--enable", (char *)text};
    ArgRun rc     = ArgParseRun(&p, 3, argv);
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return rc == ARG_RUN_OK;
}

static bool test_bool_value_truthy_spellings(void) {
    Zstr truthy[] = {"true", "1", "yes", "on"};
    for (u32 i = 0; i < 4; i++) {
        bool v = false;
        if (!run_bool_value(truthy[i], &v) || v != true)
            return false;
    }
    return true;
}

static bool test_bool_value_falsey_spellings(void) {
    Zstr falsey[] = {"false", "0", "no", "off"};
    for (u32 i = 0; i < 4; i++) {
        bool v = true;
        if (!run_bool_value(falsey[i], &v) || v != false)
            return false;
    }
    return true;
}

static bool test_bool_value_garbage_rejected(void) {
    // A non-bool spelling must be rejected (parse error), and the
    // pre-existing target value must be left untouched.
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("prog", NULL, &a);
    bool             v = true;
    ArgOptional(&p, NULL, "--enable", &v, "toggle");
    char  *argv[] = {(char *)"prog", (char *)"--enable", (char *)"maybe"};
    ArgRun rc     = ArgParseRun(&p, 3, argv);
    bool   ok     = (rc == ARG_RUN_ERROR) && (v == true);
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

// ----------------------------------------------------------------------------
// ArgCount across the unsigned target widths. The u32 path is covered
// above; u8/u16/u64 counters must increment per occurrence too.
// ----------------------------------------------------------------------------

static bool test_count_u8_target(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("prog", NULL, &a);
    u8               n = 0;
    ArgCount(&p, "-v", "--verbose", &n, "v");
    char  *argv[] = {(char *)"prog", (char *)"-v", (char *)"-v"};
    ArgRun rc     = ArgParseRun(&p, 3, argv);
    bool   ok     = (rc == ARG_RUN_OK) && (n == 2);
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

static bool test_count_u16_target(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("prog", NULL, &a);
    u16              n = 0;
    ArgCount(&p, "-v", "--verbose", &n, "v");
    char  *argv[] = {(char *)"prog", (char *)"-vvvv"};
    ArgRun rc     = ArgParseRun(&p, 2, argv);
    bool   ok     = (rc == ARG_RUN_OK) && (n == 4);
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

static bool test_count_u64_target(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("prog", NULL, &a);
    u64              n = 0;
    ArgCount(&p, "-v", "--verbose", &n, "v");
    char  *argv[] = {(char *)"prog", (char *)"-v", (char *)"-v", (char *)"-v"};
    ArgRun rc     = ArgParseRun(&p, 4, argv);
    bool   ok     = (rc == ARG_RUN_OK) && (n == 3);
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

// ----------------------------------------------------------------------------
// Value-form error paths
// ----------------------------------------------------------------------------

static bool test_option_missing_trailing_value(void) {
    // "--listen" is the last token: there is no value to consume, so the
    // parse must error rather than read past argv.
    DefaultAllocator a      = DefaultAllocatorInit();
    ArgParse         p      = ArgParseInit("prog", NULL, &a);
    Zstr             listen = NULL;
    ArgRequired(&p, "-l", "--listen", &listen, "host:port");
    char  *argv[] = {(char *)"prog", (char *)"--listen"};
    ArgRun rc     = ArgParseRun(&p, 2, argv);
    bool   ok     = (rc == ARG_RUN_ERROR) && (listen == NULL);
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

static bool test_flag_rejects_inline_value(void) {
    // A boolean flag takes no value; "--verbose=1" must be rejected.
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("prog", NULL, &a);
    bool             v = false;
    ArgFlag(&p, "-v", "--verbose", &v, "v");
    char  *argv[] = {(char *)"prog", (char *)"--verbose=1"};
    ArgRun rc     = ArgParseRun(&p, 2, argv);
    bool   ok     = (rc == ARG_RUN_ERROR) && (v == false);
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

static bool test_count_rejects_inline_value(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("prog", NULL, &a);
    u32              n = 0;
    ArgCount(&p, "-v", "--verbose", &n, "v");
    char  *argv[] = {(char *)"prog", (char *)"--verbose=3"};
    ArgRun rc     = ArgParseRun(&p, 2, argv);
    bool   ok     = (rc == ARG_RUN_ERROR) && (n == 0);
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

static bool test_short_value_option_not_bundled(void) {
    // "-lX" sticking a value onto a short value option is not supported;
    // it must error rather than silently treat "lX" as bundled flags.
    DefaultAllocator a      = DefaultAllocatorInit();
    ArgParse         p      = ArgParseInit("prog", NULL, &a);
    Zstr             listen = NULL;
    ArgRequired(&p, "-l", "--listen", &listen, "host:port");
    char  *argv[] = {(char *)"prog", (char *)"-lhost"};
    ArgRun rc     = ArgParseRun(&p, 2, argv);
    bool   ok     = (rc == ARG_RUN_ERROR);
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

static bool test_signed_boundary_accepted(void) {
    // The documented signed range endpoints must parse; one past must
    // fail. Covers the range-check predicate in parse_signed.
    DefaultAllocator a  = DefaultAllocatorInit();
    ArgParse         p  = ArgParseInit("prog", NULL, &a);
    i16              lo = 0;
    i16              hi = 0;
    ArgOptional(&p, NULL, "--lo", &lo, "lo");
    ArgOptional(&p, NULL, "--hi", &hi, "hi");
    char  *argv[] = {(char *)"prog", (char *)"--lo=-32768", (char *)"--hi=32767"};
    ArgRun rc     = ArgParseRun(&p, 3, argv);
    bool   ok     = (rc == ARG_RUN_OK) && (lo == -32768) && (hi == 32767);
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

static bool test_signed_boundary_overflow_rejected(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("prog", NULL, &a);
    i16              v = 5;
    ArgOptional(&p, NULL, "--v", &v, "v");
    char  *argv[] = {(char *)"prog", (char *)"--v=32768"};
    ArgRun rc     = ArgParseRun(&p, 2, argv);
    bool   ok     = (rc == ARG_RUN_ERROR) && (v == 5);
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

// ----------------------------------------------------------------------------
// "--" separator forces remaining tokens to positional
// ----------------------------------------------------------------------------

static bool test_double_dash_separator(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("cat", NULL, &a);

    Zstr file = NULL;
    ArgPositional(&p, "file", &file, "input file");

    // "--unusual-name" would normally be parsed as an option; "--"
    // forces it to be a positional.
    char  *argv[] = {(char *)"cat", (char *)"--", (char *)"--unusual-name"};
    ArgRun rc     = ArgParseRun(&p, 3, argv);

    bool ok = (rc == ARG_RUN_OK) && ZstrCompare(file, "--unusual-name") == 0;
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

// ----------------------------------------------------------------------------
// Help: --help short-circuits with ARG_RUN_HELP (so the caller can
// `return 0`). Validation that prints look right is left for visual
// inspection; here we just check the return code.
// ----------------------------------------------------------------------------

static bool test_help_returns_help_code(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("prog", "test prog", &a);

    Zstr required = NULL;
    ArgRequired(&p, "-l", "--listen", &required, "host:port");

    char  *argv[] = {(char *)"prog", (char *)"--help"};
    ArgRun rc     = ArgParseRun(&p, 2, argv);

    // --help should beat the missing-required check.
    bool ok = (rc == ARG_RUN_HELP);
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

int main(void) {
    WriteFmt("[INFO] Starting ArgParse tests\n\n");

    TestFunction tests[] = {
        test_required_long_space,
        test_required_long_equals,
        test_required_short,
        test_optional_default_preserved,
        test_optional_overrides_default,
        test_flag_presence,
        test_flag_absence,
        test_count_repeated,
        test_count_bundled,
        test_positional_order,
        test_positional_with_interleaved_flag,
        test_type_inferred_u32,
        test_type_inferred_i64_negative,
        test_type_inferred_f64,
        test_type_inferred_str,
        test_missing_required,
        test_missing_positional,
        test_unknown_option,
        test_invalid_value_for_type,
        test_u8_overflow,
        test_too_many_positionals,
        test_bool_value_truthy_spellings,
        test_bool_value_falsey_spellings,
        test_bool_value_garbage_rejected,
        test_count_u8_target,
        test_count_u16_target,
        test_count_u64_target,
        test_option_missing_trailing_value,
        test_flag_rejects_inline_value,
        test_count_rejects_inline_value,
        test_short_value_option_not_bundled,
        test_signed_boundary_accepted,
        test_signed_boundary_overflow_rejected,
        test_double_dash_separator,
        test_help_returns_help_code,
    };

    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), NULL, 0, "ArgParse");
}
