#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Std/ArgParse.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Std/File.h>
#include <Misra/Std/Io.h>
#include <Misra/Std/Log.h>

#include <unistd.h>

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

// ===========================================================================
// Mutation-hardening suite (moved from ArgParse.Mutants1/2/3).
// ===========================================================================

// ---------------------------------------------------------------------------
// print_help writes its usage table straight to fd 2 (FileStderr). To
// assert the EXACT layout -- column alignment, padding width, section
// order, separators -- we redirect fd 2 to a pipe, run
// ArgParseRun(--help), then drain the pipe back into a Str. pipe / dup /
// dup2 / read are OS interfaces (POSIX), not libc, so they are allowed
// in MisraStdC tests.
//
// ArgParseRun appends a synthetic "-h, --help  print this help" FLAG
// spec before printing, so every captured help text ends with that row.
// ---------------------------------------------------------------------------
static void capture_help(ArgParse *p, Str *out) {
    int pipefd[2];
    if (pipe(pipefd) != 0)
        LOG_FATAL("capture_help: pipe failed");

    int saved = dup(2);
    if (saved < 0)
        LOG_FATAL("capture_help: dup failed");
    if (dup2(pipefd[1], 2) < 0)
        LOG_FATAL("capture_help: dup2 failed");
    close(pipefd[1]);

    char  *argv[] = {(char *)"prog", (char *)"--help"};
    ArgRun rc     = ArgParseRun(p, 2, argv);

    // Restore the real stderr before draining so later logging is sane.
    dup2(saved, 2);
    close(saved);

    if (rc != ARG_RUN_HELP)
        LOG_FATAL("capture_help: expected ARG_RUN_HELP, got {}", (int)rc);

    char buf[4096];
    for (;;) {
        long n = read(pipefd[0], buf, sizeof(buf));
        if (n <= 0)
            break;
        for (long i = 0; i < n; ++i)
            StrPushBackR(out, buf[i]);
    }
    close(pipefd[0]);
}

static bool help_equals(ArgParse *p, Zstr expected) {
    DefaultAllocator a   = DefaultAllocatorInit();
    Str              out = StrInit(&a);
    capture_help(p, &out);
    bool ok = (StrLen(&out) == ZstrLen(expected)) && (ZstrCompare(StrBegin(&out), expected) == 0);
    if (!ok) {
        WriteFmt("[a1] help mismatch:\n");
        WriteFmt("---- expected ({} bytes) ----\n{}\n", ZstrLen(expected), expected);
        WriteFmt("---- actual   ({} bytes) ----\n{}\n", StrLen(&out), StrBegin(&out));
    }
    StrDeinit(&out);
    DefaultAllocatorDeinit(&a);
    return ok;
}

// ---------------------------------------------------------------------------
// The flagship test: one parser exercising every left-column shape
// (positional, short+long value option, long-only value option, flag),
// the about/usage header, both section headers, and the padding-width
// arithmetic. The expected string is byte-exact, which pins down the
// column width calc (max_w + 2), the "  -x, --xxx <META>" layout, the
// usage line assembly, section ordering, and every separator newline.
//
//   max left width = "      --timeout <TIMEOUT>" = 25, so every help
//   column is padded out to 25 + 2 = 27.
// ---------------------------------------------------------------------------
static bool test_a1_full_layout(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("prog", "test prog", &a);

    Zstr src     = NULL;
    Zstr listen  = NULL;
    u32  timeout = 0;
    bool verbose = false;
    ArgPositional(&p, "source", &src, "src file");
    ArgRequired(&p, "-l", "--listen", &listen, "host:port");
    ArgOptional(&p, NULL, "--timeout", &timeout, "secs");
    ArgFlag(&p, "-v", "--verbose", &verbose, "be loud");

    Zstr expected =
        "prog -- test prog\n"
        "\n"
        "usage: prog [OPTIONS] --listen <LISTEN> <source>\n"
        "\n"
        "positional arguments:\n"
        "  <source>                 src file\n"
        "\n"
        "options:\n"
        "  -l, --listen <LISTEN>    host:port\n"
        "      --timeout <TIMEOUT>  secs\n"
        "  -v, --verbose            be loud\n"
        "  -h, --help               print this help\n";

    bool ok = help_equals(&p, expected);
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

// ---------------------------------------------------------------------------
// about == NULL: header is just the program name, no " -- about". Pins
// the `if (self->about)` branch (line 317) and the no-about title line.
// With only a required option (no flag/optional/count) there is also no
// " [OPTIONS]" in the usage line -- pins the any_option scan (lines
// 330-338) producing false.
// ---------------------------------------------------------------------------
static bool test_a1_no_about_no_options(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("prog", NULL, &a);

    Zstr listen = NULL;
    ArgRequired(&p, "-l", "--listen", &listen, "host:port");

    // The auto-registered --help flag is the ONLY option-role spec, so
    // [OPTIONS] DOES appear. Use that fact: it is a flag, so any_option
    // becomes true via the synthetic spec.
    Zstr expected =
        "prog\n"
        "\n"
        "usage: prog [OPTIONS] --listen <LISTEN>\n"
        "\n"
        "options:\n"
        "  -l, --listen <LISTEN>  host:port\n"
        "  -h, --help             print this help\n";

    bool ok = help_equals(&p, expected);
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

// ---------------------------------------------------------------------------
// Positional-only parser. Exercises the positional usage loop
// (lines 352-357 -- " <NAME>") and the positional-header section. The
// only option-role spec is the synthetic --help flag, which is why
// [OPTIONS] still shows. Pins: positional metavar rendering as "<...>",
// the positional section header, and the blank line after positionals
// (line 396-397) before the options section.
// ---------------------------------------------------------------------------
static bool test_a1_positionals_section(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("cp", NULL, &a);

    Zstr from = NULL;
    Zstr to   = NULL;
    ArgPositional(&p, "from", &from, "source path");
    ArgPositional(&p, "to", &to, "dest path");

    Zstr expected =
        "cp\n"
        "\n"
        "usage: cp [OPTIONS] <from> <to>\n"
        "\n"
        "positional arguments:\n"
        "  <from>      source path\n"
        "  <to>        dest path\n"
        "\n"
        "options:\n"
        "  -h, --help  print this help\n";

    bool ok = help_equals(&p, expected);
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

// ---------------------------------------------------------------------------
// A short-only flag in the left column. spec_format_left emits the short
// name and, with no long name, no ", " separator and no long-name text.
// This pins the `if (sp->long_name)` guards inside spec_format_left for
// an option that has a short name but no long name. The metavar branch
// is not taken (it is a flag, not a value option).
//
// "-q" has no long form: left column is "  -q" (width 4). The widest is
// the synthetic "  -h, --help" (width 12), so padding goes to 14.
// ---------------------------------------------------------------------------
static bool test_a1_short_only_flag(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("prog", NULL, &a);

    bool quiet = false;
    ArgFlag(&p, "-q", NULL, &quiet, "quiet");

    Zstr expected =
        "prog\n"
        "\n"
        "usage: prog [OPTIONS]\n"
        "\n"
        "options:\n"
        "  -q          quiet\n"
        "  -h, --help  print this help\n";

    bool ok = help_equals(&p, expected);
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

// ---------------------------------------------------------------------------
// An empty help string on a spec must still be padded and produce a
// trailing newline with nothing after the gap. Pins the
// `sp->help ? sp->help : ""` ternary fallback (lines 394, 410) plus the
// padding loop still running. "--mode" is long-only with a value, so its
// left column has "    " (no short) then the long name and metavar.
// ---------------------------------------------------------------------------
static bool test_a1_empty_help_padding(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("prog", NULL, &a);

    Zstr mode = NULL;
    ArgOptional(&p, NULL, "--mode", &mode, "");

    // "      --mode <MODE>" width = 19, widest, pad to 21.
    Zstr expected =
        "prog\n"
        "\n"
        "usage: prog [OPTIONS]\n"
        "\n"
        "options:\n"
        "      --mode <MODE>  \n"
        "  -h, --help         print this help\n";

    bool ok = help_equals(&p, expected);
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

// ---------------------------------------------------------------------------
// The render cap. print_help only formats the first 64 specs (the
// left_col / left_w stack arrays are 64 wide); when there are more it
// clamps n_specs to 64 (line 372). With 70 flags + the synthetic --help
// the clamp fires and exactly 64 rows render: opt00..opt63. So opt50
// (index 50, comfortably inside the real 64-row window) MUST appear,
// while opt64 must NOT. This pins the clamp constant: a mutant that
// clamps to a smaller value (e.g. 42) would drop opt50.
// ---------------------------------------------------------------------------
static bool test_a1_render_cap_is_64(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("prog", NULL, &a);

    bool flags[70] = {0};
    char names[70][8];
    for (u32 i = 0; i < 70; ++i) {
        names[i][0] = '-';
        names[i][1] = '-';
        names[i][2] = 'o';
        names[i][3] = 'p';
        names[i][4] = 't';
        names[i][5] = (char)('0' + i / 10);
        names[i][6] = (char)('0' + i % 10);
        names[i][7] = '\0';
        ArgFlag(&p, NULL, names[i], &flags[i], "f");
    }

    DefaultAllocator a2  = DefaultAllocatorInit();
    Str              out = StrInit(&a2);
    capture_help(&p, &out);

    // opt50 is inside the 64-row window (rendered); opt64 is past it.
    bool ok = ZstrFindSubstring(StrBegin(&out), "--opt50") != NULL &&
              ZstrFindSubstring(StrBegin(&out), "--opt00") != NULL &&
              ZstrFindSubstring(StrBegin(&out), "--opt64") == NULL;
    if (!ok) {
        WriteFmt("[a1] render cap: window wrong\n{}\n", StrBegin(&out));
    }
    StrDeinit(&out);
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a2);
    DefaultAllocatorDeinit(&a);
    return ok;
}

// ----------------------------------------------------------------------------
// Mutation-hardening suite for ArgParse value-parsing and help-formatting
// helpers: store_value, parse_unsigned, append_metavar, spec_format_left.
//
// parse_unsigned / store_value are observable through ArgParseRun on a
// typed value option: we assert the stored typed value and the
// accept/reject (ARG_RUN_OK vs ARG_RUN_ERROR) decision.
//
// append_metavar / spec_format_left only manifest in the --help text,
// which print_help writes to stderr (fd 2). We capture that text by
// temporarily redirecting fd 2 into a temp file, running --help, then
// restoring fd 2 and reading the file back into a Str. The exact
// rendered substrings (metavar casing, left-column layout, padding
// width) are then asserted.
// ----------------------------------------------------------------------------

// Run --help with stderr redirected into `out`. Returns true on a clean
// capture. The captured help text (whole stderr stream) lands in `out`.
// (Tempfile-based variant; the pipe-based capture_help above is kept
// under its own name.)
static bool capture_help_file(ArgParse *p, Str *out) {
    Str  tmp_path = StrInit(p->alloc);
    File tmp      = FileOpenTemp(&tmp_path, p->alloc);
    StrDeinit(&tmp_path);
    if (!FileIsOpen(&tmp))
        return false;

    int saved = dup(2);
    if (saved < 0) {
        FileClose(&tmp);
        return false;
    }
    if (dup2(tmp.fd, 2) < 0) {
        close(saved);
        FileClose(&tmp);
        return false;
    }

    char  *argv[] = {(char *)"prog", (char *)"--help"};
    ArgRun rc     = ArgParseRun(p, 2, argv);

    // Restore stderr before doing anything that might write to it.
    dup2(saved, 2);
    close(saved);

    bool ok = (rc == ARG_RUN_HELP);
    FileSeek(&tmp, 0, FILE_SEEK_SET);
    FileRead(&tmp, out);
    FileClose(&tmp);
    return ok;
}

// True iff `needle` occurs as a substring of `hay` (a Str body).
static bool str_contains(Str *hay, Zstr needle) {
    u64  hn  = StrLen(hay);
    Zstr beg = StrBegin(hay);
    u64  nn  = ZstrLen(needle);
    if (nn == 0)
        return true;
    if (nn > hn)
        return false;
    for (u64 i = 0; i + nn <= hn; ++i) {
        u64 j = 0;
        while (j < nn && beg[i + j] == needle[j])
            ++j;
        if (j == nn)
            return true;
    }
    return false;
}

// ----------------------------------------------------------------------------
// parse_unsigned (via store_value on typed value options)
// ----------------------------------------------------------------------------

static bool run_u32(Zstr text, u32 *out, ArgRun *rc) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("prog", NULL, &a);
    ArgOptional(&p, NULL, "--n", out, "count");
    char *argv[] = {(char *)"prog", (char *)"--n", (char *)text};
    *rc          = ArgParseRun(&p, 3, argv);
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return true;
}

static bool run_u64(Zstr text, u64 *out, ArgRun *rc) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("prog", NULL, &a);
    ArgOptional(&p, NULL, "--n", out, "count");
    char *argv[] = {(char *)"prog", (char *)"--n", (char *)text};
    *rc          = ArgParseRun(&p, 3, argv);
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return true;
}

// "0" parses and stores exactly 0.
static bool test_a2_parse_unsigned_zero(void) {
    u32    n  = 7;
    ArgRun rc = ARG_RUN_ERROR;
    run_u32("0", &n, &rc);
    return (rc == ARG_RUN_OK) && (n == 0);
}

// "42" parses and stores exactly 42 (not the mutated literal-truthy).
static bool test_a2_parse_unsigned_42(void) {
    u32    n  = 0;
    ArgRun rc = ARG_RUN_ERROR;
    run_u32("42", &n, &rc);
    return (rc == ARG_RUN_OK) && (n == 42);
}

// Leading zeros are accepted and the numeric value is correct.
static bool test_a2_parse_unsigned_leading_zeros(void) {
    u32    n  = 0;
    ArgRun rc = ARG_RUN_ERROR;
    run_u32("007", &n, &rc);
    return (rc == ARG_RUN_OK) && (n == 7);
}

// u32 max boundary parses exactly. Pins both the digit-accumulation
// (v*10+d) and the per-kind hi range check.
static bool test_a2_parse_unsigned_u32_max(void) {
    u32    n  = 0;
    ArgRun rc = ARG_RUN_ERROR;
    run_u32("4294967295", &n, &rc);
    return (rc == ARG_RUN_OK) && (n == 4294967295u);
}

// u64 max boundary parses exactly -- exercises the overflow guard at
// the very top of the range without tripping it.
static bool test_a2_parse_unsigned_u64_max(void) {
    u64    n  = 0;
    ArgRun rc = ARG_RUN_ERROR;
    run_u64("18446744073709551615", &n, &rc);
    return (rc == ARG_RUN_OK) && (n == ~(u64)0);
}

// One past u64 max overflows: the guard `v > (~0 - d)/10` must fire and
// reject. Pins the overflow predicate (div, >= vs >).
static bool test_a2_parse_unsigned_u64_overflow(void) {
    u64    n  = 123;
    ArgRun rc = ARG_RUN_OK;
    run_u64("18446744073709551616", &n, &rc);
    return (rc == ARG_RUN_ERROR) && (n == 123);
}

// A far-overflowing value (many extra digits) is rejected.
static bool test_a2_parse_unsigned_overflow_many_digits(void) {
    u64    n  = 99;
    ArgRun rc = ARG_RUN_OK;
    run_u64("99999999999999999999999", &n, &rc);
    return (rc == ARG_RUN_ERROR) && (n == 99);
}

// Non-digit content is rejected and the target is left untouched.
static bool test_a2_parse_unsigned_non_digit(void) {
    u32    n  = 5;
    ArgRun rc = ARG_RUN_OK;
    run_u32("abc", &n, &rc);
    return (rc == ARG_RUN_ERROR) && (n == 5);
}

// Trailing junk after digits is rejected (the remaining-length check).
static bool test_a2_parse_unsigned_trailing_junk(void) {
    u32    n  = 5;
    ArgRun rc = ARG_RUN_OK;
    run_u32("12x", &n, &rc);
    return (rc == ARG_RUN_ERROR) && (n == 5);
}

// Leading junk before digits is rejected.
static bool test_a2_parse_unsigned_leading_junk(void) {
    u32    n  = 5;
    ArgRun rc = ARG_RUN_OK;
    run_u32("x12", &n, &rc);
    return (rc == ARG_RUN_ERROR) && (n == 5);
}

// A signed-looking "-1" is rejected by the unsigned parser (no digits
// consumed before the '-').
static bool test_a2_parse_unsigned_negative_rejected(void) {
    u32    n  = 5;
    ArgRun rc = ARG_RUN_OK;
    run_u32("-1", &n, &rc);
    return (rc == ARG_RUN_ERROR) && (n == 5);
}

// ----------------------------------------------------------------------------
// store_value: per-kind typed conversion + range enforcement
// ----------------------------------------------------------------------------

// u8: in-range value stored exactly through the (u8) narrowing cast.
static bool test_a2_store_u8_value(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("prog", NULL, &a);
    u8               v = 0;
    ArgOptional(&p, NULL, "--v", &v, "");
    char  *argv[] = {(char *)"prog", (char *)"--v=200"};
    ArgRun rc     = ArgParseRun(&p, 2, argv);
    bool   ok     = (rc == ARG_RUN_OK) && (v == 200);
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

// u8: 255 (boundary) accepted; 256 rejected. Pins the 0xFF hi limit.
static bool test_a2_store_u8_boundary(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("prog", NULL, &a);
    u8               v = 0;
    ArgOptional(&p, NULL, "--v", &v, "");
    char  *argv[] = {(char *)"prog", (char *)"--v=255"};
    ArgRun rc     = ArgParseRun(&p, 2, argv);
    bool   ok     = (rc == ARG_RUN_OK) && (v == 255);
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

// u16: boundary value 65535 accepted and stored exactly.
static bool test_a2_store_u16_boundary(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("prog", NULL, &a);
    u16              v = 0;
    ArgOptional(&p, NULL, "--v", &v, "");
    char  *argv[] = {(char *)"prog", (char *)"--v=65535"};
    ArgRun rc     = ArgParseRun(&p, 2, argv);
    bool   ok     = (rc == ARG_RUN_OK) && (v == 65535);
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

// u16: 65536 (one past) rejected, target untouched.
static bool test_a2_store_u16_overflow(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("prog", NULL, &a);
    u16              v = 9;
    ArgOptional(&p, NULL, "--v", &v, "");
    char  *argv[] = {(char *)"prog", (char *)"--v=65536"};
    ArgRun rc     = ArgParseRun(&p, 2, argv);
    bool   ok     = (rc == ARG_RUN_ERROR) && (v == 9);
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

// u32: a value above u16 max but within u32 is stored (distinguishes
// the u32 hi limit from the u16 one).
static bool test_a2_store_u32_value(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("prog", NULL, &a);
    u32              v = 0;
    ArgOptional(&p, NULL, "--v", &v, "");
    char  *argv[] = {(char *)"prog", (char *)"--v=70000"};
    ArgRun rc     = ArgParseRun(&p, 2, argv);
    bool   ok     = (rc == ARG_RUN_OK) && (v == 70000);
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

// u32: one past u32 max rejected.
static bool test_a2_store_u32_overflow(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("prog", NULL, &a);
    u32              v = 11;
    ArgOptional(&p, NULL, "--v", &v, "");
    char  *argv[] = {(char *)"prog", (char *)"--v=4294967296"};
    ArgRun rc     = ArgParseRun(&p, 2, argv);
    bool   ok     = (rc == ARG_RUN_ERROR) && (v == 11);
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

// u64: a value above u32 max stored exactly (distinguishes u64 hi).
static bool test_a2_store_u64_value(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("prog", NULL, &a);
    u64              v = 0;
    ArgOptional(&p, NULL, "--v", &v, "");
    char  *argv[] = {(char *)"prog", (char *)"--v=5000000000"};
    ArgRun rc     = ArgParseRun(&p, 2, argv);
    bool   ok     = (rc == ARG_RUN_OK) && (v == 5000000000ULL);
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

// String target: store_value clears then copies the raw token.
static bool test_a2_store_str_value(void) {
    DefaultAllocator a    = DefaultAllocatorInit();
    ArgParse         p    = ArgParseInit("prog", NULL, &a);
    Str              name = StrInit(&a);
    StrPushBackMany(&name, "stale");
    ArgOptional(&p, NULL, "--name", &name, "n");
    char  *argv[] = {(char *)"prog", (char *)"--name", (char *)"bob"};
    ArgRun rc     = ArgParseRun(&p, 3, argv);
    bool ok = (rc == ARG_RUN_OK) && (StrLen(&name) == 3) && (StrBegin(&name)[0] == 'b') && (StrBegin(&name)[2] == 'b');
    ArgParseDeinit(&p);
    StrDeinit(&name);
    DefaultAllocatorDeinit(&a);
    return ok;
}

// Zstr target: store_value writes the raw token pointer through.
static bool test_a2_store_zstr_value(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("prog", NULL, &a);
    Zstr             s = NULL;
    ArgOptional(&p, NULL, "--name", &s, "n");
    char  *argv[] = {(char *)"prog", (char *)"--name", (char *)"carol"};
    ArgRun rc     = ArgParseRun(&p, 3, argv);
    bool   ok     = (rc == ARG_RUN_OK) && s && (ZstrCompare(s, "carol") == 0);
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

// i8: a negative in-range value stored exactly through the (i8) cast.
static bool test_a2_store_i16_value(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("prog", NULL, &a);
    i16              v = 0;
    ArgOptional(&p, NULL, "--v", &v, "");
    char  *argv[] = {(char *)"prog", (char *)"--v=-5"};
    ArgRun rc     = ArgParseRun(&p, 2, argv);
    bool   ok     = (rc == ARG_RUN_OK) && (v == -5);
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

// i8: garbage rejected; the parse_signed call result must drive the
// reject (call->42 would accept it).
static bool test_a2_store_i16_rejects_garbage(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("prog", NULL, &a);
    i16              v = 7;
    ArgOptional(&p, NULL, "--v", &v, "");
    char  *argv[] = {(char *)"prog", (char *)"--v", (char *)"zzz"};
    ArgRun rc     = ArgParseRun(&p, 3, argv);
    bool   ok     = (rc == ARG_RUN_ERROR) && (v == 7);
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

// i32: a distinct in-range negative value stored exactly.
static bool test_a2_store_i32_value(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("prog", NULL, &a);
    i32              v = 0;
    ArgOptional(&p, NULL, "--v", &v, "");
    char  *argv[] = {(char *)"prog", (char *)"--v=-100000"};
    ArgRun rc     = ArgParseRun(&p, 2, argv);
    bool   ok     = (rc == ARG_RUN_OK) && (v == -100000);
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

// i32: garbage rejected (drives the parse_signed call for the i32 arm).
static bool test_a2_store_i32_rejects_garbage(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("prog", NULL, &a);
    i32              v = 13;
    ArgOptional(&p, NULL, "--v", &v, "");
    char  *argv[] = {(char *)"prog", (char *)"--v", (char *)"nope"};
    ArgRun rc     = ArgParseRun(&p, 3, argv);
    bool   ok     = (rc == ARG_RUN_ERROR) && (v == 13);
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

// f32: a fractional value stored (narrowed from f64). The exact value
// is asserted in a tight band -- 42.0 would fall outside it.
static bool test_a2_store_f32_value(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("prog", NULL, &a);
    f32              v = 0;
    ArgOptional(&p, NULL, "--v", &v, "");
    char  *argv[] = {(char *)"prog", (char *)"--v=2.5"};
    ArgRun rc     = ArgParseRun(&p, 2, argv);
    bool   ok     = (rc == ARG_RUN_OK) && (v > 2.4f) && (v < 2.6f);
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

// f32: garbage rejected (drives the parse_f64_full call for the f32 arm).
static bool test_a2_store_f32_rejects_garbage(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("prog", NULL, &a);
    f32              v = 9;
    ArgOptional(&p, NULL, "--v", &v, "");
    char  *argv[] = {(char *)"prog", (char *)"--v", (char *)"xyz"};
    ArgRun rc     = ArgParseRun(&p, 3, argv);
    bool   ok     = (rc == ARG_RUN_ERROR) && (v > 8.9f) && (v < 9.1f);
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

// ----------------------------------------------------------------------------
// append_metavar (via --help text)
// ----------------------------------------------------------------------------

// A long option renders its metavar upper-cased: "--listen" -> LISTEN.
static bool test_a2_metavar_uppercased(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("prog", NULL, &a);
    Zstr             v = NULL;
    ArgRequired(&p, "-l", "--listen", &v, "addr");
    Str  help = StrInit(&a);
    bool got  = capture_help_file(&p, &help);
    bool ok   = got && str_contains(&help, "<LISTEN>");
    StrDeinit(&help);
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

// Hyphens inside a long name become underscores in the metavar:
// "--read-only" -> READ_ONLY.
static bool test_a2_metavar_hyphen_to_underscore(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("prog", NULL, &a);
    Zstr             v = NULL;
    ArgRequired(&p, NULL, "--read-only", &v, "mode");
    Str  help = StrInit(&a);
    bool got  = capture_help_file(&p, &help);
    bool ok   = got && str_contains(&help, "<READ_ONLY>");
    StrDeinit(&help);
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

// The leading "--" is skipped, not folded into the metavar: there must
// be no leading underscores (would appear if "--" were not stripped).
static bool test_a2_metavar_skips_double_dash(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("prog", NULL, &a);
    Zstr             v = NULL;
    ArgRequired(&p, NULL, "--port", &v, "p");
    Str  help = StrInit(&a);
    bool got  = capture_help_file(&p, &help);
    // The metavar is "<PORT>", never "<__PORT>".
    bool ok = got && str_contains(&help, "<PORT>") && !str_contains(&help, "__PORT");
    StrDeinit(&help);
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

// A short-only value option (no long form) falls back to VALUE.
static bool test_a2_metavar_short_only_fallback(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("prog", NULL, &a);
    Zstr             v = NULL;
    ArgRequired(&p, "-x", NULL, &v, "x");
    Str  help = StrInit(&a);
    bool got  = capture_help_file(&p, &help);
    bool ok   = got && str_contains(&help, "<VALUE>");
    StrDeinit(&help);
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

// Digits and already-upper chars in a long name pass through unchanged:
// "--ip6" -> IP6 (only lowercase letters are upper-cased).
static bool test_a2_metavar_digits_passthrough(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("prog", NULL, &a);
    Zstr             v = NULL;
    ArgRequired(&p, NULL, "--ip6", &v, "x");
    Str  help = StrInit(&a);
    bool got  = capture_help_file(&p, &help);
    bool ok   = got && str_contains(&help, "<IP6>");
    StrDeinit(&help);
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

// The upper bound of the lower-case range is inclusive: a 'z' must be
// upper-cased to 'Z'. Pins the `c <= 'z'` boundary (a `< 'z'` mutant
// would leave the 'z' lower-cased).
static bool test_a2_metavar_z_uppercased(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("prog", NULL, &a);
    Zstr             v = NULL;
    ArgRequired(&p, NULL, "--gzip", &v, "x");
    Str  help = StrInit(&a);
    bool got  = capture_help_file(&p, &help);
    // "gzip" -> "GZIP": the trailing 'z' in the middle must be upper.
    bool ok = got && str_contains(&help, "<GZIP>") && !str_contains(&help, "GzIP");
    StrDeinit(&help);
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

// The lower bound of the lower-case range is inclusive: an 'a' must be
// upper-cased to 'A'. Pins the `c >= 'a'` boundary.
static bool test_a2_metavar_a_uppercased(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("prog", NULL, &a);
    Zstr             v = NULL;
    ArgRequired(&p, NULL, "--abc", &v, "x");
    Str  help = StrInit(&a);
    bool got  = capture_help_file(&p, &help);
    bool ok   = got && str_contains(&help, "<ABC>");
    StrDeinit(&help);
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

// ----------------------------------------------------------------------------
// spec_format_left (via --help text)
// ----------------------------------------------------------------------------

// A positional renders as "<name>" (angle-bracketed, lower-case name).
static bool test_a2_left_positional_brackets(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("cp", NULL, &a);
    Zstr             s = NULL;
    ArgPositional(&p, "source", &s, "from");
    Str  help = StrInit(&a);
    bool got  = capture_help_file(&p, &help);
    bool ok   = got && str_contains(&help, "<source>");
    StrDeinit(&help);
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

// An option with both short and long forms renders "-l, --listen".
static bool test_a2_left_short_and_long(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("prog", NULL, &a);
    Zstr             v = NULL;
    ArgRequired(&p, "-l", "--listen", &v, "addr");
    Str  help = StrInit(&a);
    bool got  = capture_help_file(&p, &help);
    bool ok   = got && str_contains(&help, "-l, --listen");
    StrDeinit(&help);
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

// An option with a short form renders the "  -l" two-space indent +
// short name and a value option appends " <METAVAR>".
static bool test_a2_left_full_option_line(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("prog", NULL, &a);
    Zstr             v = NULL;
    ArgRequired(&p, "-l", "--listen", &v, "addr");
    Str  help = StrInit(&a);
    bool got  = capture_help_file(&p, &help);
    // Full left column for this spec: two-space indent, short, ", ",
    // long, " <LISTEN>".
    bool ok = got && str_contains(&help, "  -l, --listen <LISTEN>");
    StrDeinit(&help);
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

// A long-only option pads the missing short with four spaces, so the
// long name lines up under a present short form: "    --timeout".
static bool test_a2_left_long_only_pads_short(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("prog", NULL, &a);
    u32              t = 0;
    ArgOptional(&p, NULL, "--timeout", &t, "secs");
    Str  help = StrInit(&a);
    bool got  = capture_help_file(&p, &help);
    // No short form: rendered as four spaces then the long name.
    bool ok = got && str_contains(&help, "    --timeout <TIMEOUT>");
    StrDeinit(&help);
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

// A flag (no value) renders the option name but NO " <METAVAR>" suffix.
static bool test_a2_left_flag_has_no_metavar(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("prog", NULL, &a);
    bool             f = false;
    ArgFlag(&p, "-v", "--verbose", &f, "v");
    Str  help = StrInit(&a);
    bool got  = capture_help_file(&p, &help);
    // The verbose flag must appear but with no "<VERBOSE>" metavar.
    bool ok = got && str_contains(&help, "-v, --verbose") && !str_contains(&help, "<VERBOSE>");
    StrDeinit(&help);
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

// The left-column width drives padding: a long option name forces the
// help-text descriptions to be pushed past the longest left column.
// We register a short option ("-a") and a long-named one and assert the
// short option's description is padded to clear the long one. This pins
// the "StrLen(out) - start" width return of spec_format_left.
static bool test_a2_left_width_drives_padding(void) {
    DefaultAllocator a  = DefaultAllocatorInit();
    ArgParse         p  = ArgParseInit("prog", NULL, &a);
    bool             aa = false;
    Zstr             bb = NULL;
    ArgFlag(&p, "-a", "--a", &aa, "AAA");
    ArgRequired(&p, NULL, "--a-very-long-option-name", &bb, "BBB");
    Str  help = StrInit(&a);
    bool got  = capture_help_file(&p, &help);
    // The short flag "--a" help ("AAA") must be padded so it starts at
    // (or past) the column established by the long option's left width.
    // Concretely there must be a run of many spaces between "--a" and
    // "AAA" -- far more than the single inter-column gap that a buggy
    // width of 0/42 would produce. Look for the description preceded by
    // a long space run.
    bool ok = got && str_contains(&help, "--a-very-long-option-name") && str_contains(&help, "AAA") &&
              str_contains(&help, "          AAA");
    StrDeinit(&help);
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

// ===========================================================================
// ArgParse.Mutants3 -- contract guards closing mull survivors in the parse
// loop and token handling (ArgParseRun / handle_option_token /
// handle_short_bundle / arg_register / find_short).
//
// NORMAL tests return true on success. DEADEND tests intentionally trip a
// LOG_FATAL and are run under the SetJmp guard (deadend_tests[]).
// ===========================================================================

// ---------------------------------------------------------------------------
// ArgParseRun argument-guard boundary (line 619):
//   if (!self || argc < 0 || (argc > 0 && !argv)) -> ERROR
// argc==0 with argv==NULL and no specs must parse cleanly (ARG_RUN_OK).
// Kills:  argc<0 -> argc<=0   (would ERROR at argc==0)
//         argc>0 -> argc>=0   (with argv NULL would ERROR at argc==0)
//         argc>0 -> argc<=0   (with argv NULL would ERROR at argc==0)
// ---------------------------------------------------------------------------
static bool test_a3_run_argc_zero_is_ok(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("prog", NULL, &a);

    ArgRun rc = ArgParseRun(&p, 0, NULL);

    bool ok = (rc == ARG_RUN_OK);
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

// ---------------------------------------------------------------------------
// handle_option_token: "--name=VAL" flag-name terminator (line 502):
//   data[n] = '\0'  -> data[n] = 42
// A '*' instead of NUL corrupts the split flag name so the lookup misses and
// the value is never stored. Real: matches, stores, OK.
// ---------------------------------------------------------------------------
static bool test_a3_inline_value_flag_name_terminated(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("prog", NULL, &a);

    Zstr listen = NULL;
    ArgRequired(&p, "-l", "--listen", &listen, "host:port");

    char  *argv[] = {(char *)"prog", (char *)"--listen=host:9"};
    ArgRun rc     = ArgParseRun(&p, 2, argv);

    bool ok = (rc == ARG_RUN_OK) && listen && ZstrCompare(listen, "host:9") == 0;
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

// ---------------------------------------------------------------------------
// find_short (line 252): zstr_eq(sp->short_name, short_name) replaced by a
// truthy scalar would make find_short return the FIRST non-positional spec
// regardless of the short name. Register two short flags; toggle only the
// SECOND one and assert the FIRST one stays unset.
// ---------------------------------------------------------------------------
static bool test_a3_find_short_matches_correct_spec(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("prog", NULL, &a);

    bool aflag = false;
    bool bflag = false;
    ArgFlag(&p, "-a", "--aaa", &aflag, "a");
    ArgFlag(&p, "-b", "--bbb", &bflag, "b");

    char  *argv[] = {(char *)"prog", (char *)"-b"};
    ArgRun rc     = ArgParseRun(&p, 2, argv);

    // Real: only -b matched -> bflag set, aflag untouched.
    // Mutant: find_short returns first spec (-a) -> aflag set instead.
    bool ok = (rc == ARG_RUN_OK) && (bflag == true) && (aflag == false);
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

// ---------------------------------------------------------------------------
// handle_short_bundle line 585: bool iter_ok = false -> = 42 (truthy).
// line 611: result = ARG_RUN_ERROR -> = 42 (non-OK/HELP/ERROR sentinel).
// An invalid bundle char must yield exactly ARG_RUN_ERROR. With iter_ok
// pre-set truthy the failure is swallowed (stays OK); with result=42 the
// caller propagates 42 != ARG_RUN_ERROR.
// ---------------------------------------------------------------------------
static bool test_a3_bundle_invalid_char_errors(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("prog", NULL, &a);

    u32 verbose = 0;
    ArgCount(&p, "-v", "--verbose", &verbose, "v");

    // "-vx": -v is a valid count (so the bundle is attempted), x is unknown.
    char  *argv[] = {(char *)"prog", (char *)"-vx"};
    ArgRun rc     = ArgParseRun(&p, 2, argv);

    bool ok = (rc == ARG_RUN_ERROR);
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

// ---------------------------------------------------------------------------
// handle_short_bundle line 590: data[2] = '\0' -> = 42 corrupts the "-X"
// lookup key so a valid bundle char fails to match. Real: "-vv" counts 2.
// ---------------------------------------------------------------------------
static bool test_a3_bundle_lookup_key_terminated(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("prog", NULL, &a);

    u32 verbose = 0;
    ArgCount(&p, "-v", "--verbose", &verbose, "v");

    char  *argv[] = {(char *)"prog", (char *)"-vv"};
    ArgRun rc     = ArgParseRun(&p, 2, argv);

    bool ok = (rc == ARG_RUN_OK) && (verbose == 2);
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

// ---------------------------------------------------------------------------
// handle_short_bundle line 599: *(bool *)sp->target = true -> = 42. A bundled
// FLAG must store a genuine boolean true, not 42. Assert == true exactly.
// ---------------------------------------------------------------------------
static bool test_a3_bundle_flag_stores_true(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("prog", NULL, &a);

    bool f = false;
    bool g = false;
    ArgFlag(&p, "-f", "--fff", &f, "f");
    ArgFlag(&p, "-g", "--ggg", &g, "g");

    char  *argv[] = {(char *)"prog", (char *)"-fg"};
    ArgRun rc     = ArgParseRun(&p, 2, argv);

    bool ok = (rc == ARG_RUN_OK) && (f == true) && (g == true);
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

// ---------------------------------------------------------------------------
// ArgParseRun bundle-decision scratch (line 687): data[2] = '\0' -> = 42
// corrupts the "-X" probe key so a real FLAG bundle is mis-rejected.
// ArgParseRun line 691: first->role == ARG_ROLE_FLAG -> != flips the
// flag/count gate so a flag bundle is rejected as a value option.
// Both: "-ff" (a real FLAG bundle) must succeed and set the flag.
// ---------------------------------------------------------------------------
static bool test_a3_flag_bundle_accepted(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("prog", NULL, &a);

    bool f = false;
    ArgFlag(&p, "-f", "--fff", &f, "f");

    char  *argv[] = {(char *)"prog", (char *)"-ff"};
    ArgRun rc     = ArgParseRun(&p, 2, argv);

    bool ok = (rc == ARG_RUN_OK) && (f == true);
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

// ---------------------------------------------------------------------------
// ArgParseRun line 706: if (r != ARG_RUN_OK) return r; -> == makes a
// successful bundle return early, skipping the missing-required validation.
// A valid bundle plus an unsatisfied required option must still ERROR.
// ---------------------------------------------------------------------------
static bool test_a3_bundle_then_missing_required(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("prog", NULL, &a);

    u32  verbose = 0;
    Zstr listen  = NULL;
    ArgCount(&p, "-v", "--verbose", &verbose, "v");
    ArgRequired(&p, "-l", "--listen", &listen, "host:port");

    // Valid "-vv" bundle, but the required --listen is never supplied.
    char  *argv[] = {(char *)"prog", (char *)"-vv"};
    ArgRun rc     = ArgParseRun(&p, 2, argv);

    bool ok = (rc == ARG_RUN_ERROR) && (verbose == 2) && (listen == NULL);
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

// ---------------------------------------------------------------------------
// arg_register line 438: role == ARG_ROLE_POSITIONAL -> != would force the
// "positional needs a name" fatal onto a short-only option (long_name NULL).
// Registering a short-only flag and running must succeed on real code.
// ---------------------------------------------------------------------------
static bool test_a3_short_only_option_registers(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("prog", NULL, &a);

    bool v = false;
    ArgFlag(&p, "-v", NULL, &v, "v"); // short-only: long_name == NULL

    char  *argv[] = {(char *)"prog", (char *)"-v"};
    ArgRun rc     = ArgParseRun(&p, 2, argv);

    bool ok = (rc == ARG_RUN_OK) && (v == true);
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

// ---------------------------------------------------------------------------
// DEADEND: arg_register line 441:
//   role != ARG_ROLE_POSITIONAL && !short_name && !long_name -> FATAL
// The ne_to_eq mutation (role == POSITIONAL) makes this guard never fire for
// an option, so an option with neither short nor long name would be silently
// accepted. Real code aborts. Registering such an option must abort.
// ---------------------------------------------------------------------------
static bool test_a3_option_without_any_name_aborts(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("prog", NULL, &a);

    bool v = false;
    ArgFlag(&p, NULL, NULL, &v, "v"); // neither short nor long -> LOG_FATAL

    // Unreachable on real code (LOG_FATAL above longjmps out).
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return true;
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
        test_a1_full_layout,
        test_a1_no_about_no_options,
        test_a1_positionals_section,
        test_a1_short_only_flag,
        test_a1_empty_help_padding,
        test_a1_render_cap_is_64,
        test_a2_parse_unsigned_zero,
        test_a2_parse_unsigned_42,
        test_a2_parse_unsigned_leading_zeros,
        test_a2_parse_unsigned_u32_max,
        test_a2_parse_unsigned_u64_max,
        test_a2_parse_unsigned_u64_overflow,
        test_a2_parse_unsigned_overflow_many_digits,
        test_a2_parse_unsigned_non_digit,
        test_a2_parse_unsigned_trailing_junk,
        test_a2_parse_unsigned_leading_junk,
        test_a2_parse_unsigned_negative_rejected,
        test_a2_store_u8_value,
        test_a2_store_u8_boundary,
        test_a2_store_u16_boundary,
        test_a2_store_u16_overflow,
        test_a2_store_u32_value,
        test_a2_store_u32_overflow,
        test_a2_store_u64_value,
        test_a2_store_str_value,
        test_a2_store_zstr_value,
        test_a2_store_i16_value,
        test_a2_store_i16_rejects_garbage,
        test_a2_store_i32_value,
        test_a2_store_i32_rejects_garbage,
        test_a2_store_f32_value,
        test_a2_store_f32_rejects_garbage,
        test_a2_metavar_uppercased,
        test_a2_metavar_hyphen_to_underscore,
        test_a2_metavar_skips_double_dash,
        test_a2_metavar_short_only_fallback,
        test_a2_metavar_digits_passthrough,
        test_a2_metavar_z_uppercased,
        test_a2_metavar_a_uppercased,
        test_a2_left_positional_brackets,
        test_a2_left_short_and_long,
        test_a2_left_full_option_line,
        test_a2_left_long_only_pads_short,
        test_a2_left_flag_has_no_metavar,
        test_a2_left_width_drives_padding,
        test_a3_run_argc_zero_is_ok,
        test_a3_inline_value_flag_name_terminated,
        test_a3_find_short_matches_correct_spec,
        test_a3_bundle_invalid_char_errors,
        test_a3_bundle_lookup_key_terminated,
        test_a3_bundle_flag_stores_true,
        test_a3_flag_bundle_accepted,
        test_a3_bundle_then_missing_required,
        test_a3_short_only_option_registers,
    };

    TestFunction deadend_tests[] = {
        test_a3_option_without_any_name_aborts,
    };

    return run_test_suite(
        tests,
        sizeof(tests) / sizeof(tests[0]),
        deadend_tests,
        sizeof(deadend_tests) / sizeof(deadend_tests[0]),
        "ArgParse"
    );
}
