/// file : tests/std/argparse.mut.c
/// Targeted mutation-kill tests for ArgParse: drive inputs that make surviving
/// mutants produce observably-wrong results. Distinct from existing ArgParse
/// tests -- no dups. Each test pins exactly one survivor site that the old
/// (timeout-corrupted) mull run mis-tagged but is in fact killable.
#include <Misra.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Std/ArgParse.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Std/File.h>
#include <Misra/Std/Io.h>
#include <Misra/Std/Log.h>

#include <unistd.h>

#include "../Util/TestRunner.h"

// ---------------------------------------------------------------------------
// --help capture: print_help writes the usage table to fd 2 (FileStderr).
// Redirect fd 2 into a pipe, run --help, drain the pipe back into a Str.
// (Same technique the existing ArgParse.c suite uses.)
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
        WriteFmt("[mut] help mismatch:\n");
        WriteFmt("---- expected ({} bytes) ----\n{}\n", ZstrLen(expected), expected);
        WriteFmt("---- actual   ({} bytes) ----\n{}\n", StrLen(&out), StrBegin(&out));
    }
    StrDeinit(&out);
    DefaultAllocatorDeinit(&a);
    return ok;
}

// ---------------------------------------------------------------------------
// 640:9  ArgParseRun synthetic help spec: help.role = ARG_ROLE_FLAG -> 42
//
// The auto-registered --help spec is given ARG_ROLE_FLAG. print_help's
// any_option scan counts a Flag/Optional/Count to decide whether to emit
// " [OPTIONS]" in the usage line. For a positional-only parser the synthetic
// help flag is the ONLY option-role spec, so it is exactly what makes
// " [OPTIONS]" appear. A role of 42 (not FLAG) drops it from the scan, so the
// usage line loses " [OPTIONS]". Byte-exact capture pins the usage line.
//
// (Verified: applying the literal `help.role = 42;` mutant and rebuilding only
// Tests/ArgParse.Mut makes this test fail; reverting restores the pass.)
// ---------------------------------------------------------------------------
static bool test_mut_help_role_drives_options_tag(void) {
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

int main(void) {
    TestFunction tests[] = {
        test_mut_help_role_drives_options_tag,
    };
    TestFunction deadend_tests[] = {0};
    (void)deadend_tests;
    return run_test_suite(tests, (int)(sizeof(tests) / sizeof(tests[0])), NULL, 0, "ArgParse.Mut");
}
