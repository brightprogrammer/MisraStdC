#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Std/ArgParse.h>
#include <Misra/Std/Log.h>

#include "../Util/TestRunner.h"

// Build argv with argv[0] = program name. The casts mimic real argv,
// which is `char **` rather than `Zstr *`. We always pass
// string literals; the parser never writes through them.
#define ARGV(...)                                                                                                      \
    (char *[]) {                                                                                                       \
        (char *)"prog", __VA_ARGS__                                                                                    \
    }

#define ARGC(...) (1 + (int)(sizeof((char *[]) {__VA_ARGS__}) / sizeof(char *)))

// ----------------------------------------------------------------------------
// Each verb in isolation
// ----------------------------------------------------------------------------

static bool test_required_long_space(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    Allocator       *A = ALLOCATOR_OF(&a);
    ArgParse         p = ArgParseInit("prog", NULL, A);

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
    Allocator       *A = ALLOCATOR_OF(&a);
    ArgParse         p = ArgParseInit("prog", NULL, A);

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
    Allocator       *A = ALLOCATOR_OF(&a);
    ArgParse         p = ArgParseInit("prog", NULL, A);

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
    Allocator       *A = ALLOCATOR_OF(&a);
    ArgParse         p = ArgParseInit("prog", NULL, A);

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
    Allocator       *A = ALLOCATOR_OF(&a);
    ArgParse         p = ArgParseInit("prog", NULL, A);

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
    Allocator       *A = ALLOCATOR_OF(&a);
    ArgParse         p = ArgParseInit("prog", NULL, A);

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
    Allocator       *A = ALLOCATOR_OF(&a);
    ArgParse         p = ArgParseInit("prog", NULL, A);

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
    Allocator       *A = ALLOCATOR_OF(&a);
    ArgParse         p = ArgParseInit("prog", NULL, A);

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
    Allocator       *A = ALLOCATOR_OF(&a);
    ArgParse         p = ArgParseInit("prog", NULL, A);

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
    Allocator       *A = ALLOCATOR_OF(&a);
    ArgParse         p = ArgParseInit("cp", NULL, A);

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
    Allocator       *A = ALLOCATOR_OF(&a);
    ArgParse         p = ArgParseInit("cp", NULL, A);

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
    Allocator       *A = ALLOCATOR_OF(&a);
    ArgParse         p = ArgParseInit("prog", NULL, A);

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
    Allocator       *A = ALLOCATOR_OF(&a);
    ArgParse         p = ArgParseInit("prog", NULL, A);

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
    Allocator       *A = ALLOCATOR_OF(&a);
    ArgParse         p = ArgParseInit("prog", NULL, A);

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
    Allocator       *A = ALLOCATOR_OF(&a);
    ArgParse         p = ArgParseInit("prog", NULL, A);

    Str name = StrInit(A);
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
    Allocator       *A = ALLOCATOR_OF(&a);
    ArgParse         p = ArgParseInit("prog", NULL, A);

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
    Allocator       *A = ALLOCATOR_OF(&a);
    ArgParse         p = ArgParseInit("cp", NULL, A);

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
    Allocator       *A = ALLOCATOR_OF(&a);
    ArgParse         p = ArgParseInit("prog", NULL, A);

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
    Allocator       *A = ALLOCATOR_OF(&a);
    ArgParse         p = ArgParseInit("prog", NULL, A);

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
    Allocator       *A = ALLOCATOR_OF(&a);
    ArgParse         p = ArgParseInit("prog", NULL, A);

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
    Allocator       *A = ALLOCATOR_OF(&a);
    ArgParse         p = ArgParseInit("prog", NULL, A);

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
// "--" separator forces remaining tokens to positional
// ----------------------------------------------------------------------------

static bool test_double_dash_separator(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    Allocator       *A = ALLOCATOR_OF(&a);
    ArgParse         p = ArgParseInit("cat", NULL, A);

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
    Allocator       *A = ALLOCATOR_OF(&a);
    ArgParse         p = ArgParseInit("prog", "test prog", A);

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
        test_double_dash_separator,
        test_help_returns_help_code,
    };

    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), NULL, 0, "ArgParse");
}
