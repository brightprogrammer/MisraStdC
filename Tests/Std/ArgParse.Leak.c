/// file : tests/std/argparse.leak.c
/// Leak tests for ArgParse: kill reachable mutation survivors by routing
/// every allocation through an explicit DebugAllocator and asserting the
/// live count is zero after teardown. A dropped internal *Deinit leaves a
/// live allocation, so the count assertion fails -> the mutant is killed.
/// Distinct from existing ArgParse.* tests (which use DefaultAllocator and
/// never observe a leak).
#include <Misra.h>
#include <Misra/Std/Allocator/Debug.h>
#include <Misra/Std/ArgParse.h>

#include "../Util/TestRunner.h"

// ---- 361:5 cxx_remove_void_call -----------------------------------------
// print_help allocates `usage` (a Str) from self->alloc and StrDeinit's it
// at line 361 before the function returns. Removing that Deinit leaks the
// usage Str's backing on EVERY --help. Drive --help through a parser whose
// allocator is a DebugAllocator and require live_count == 0 afterwards.
//
// A minimal parser (no specs) still builds and frees `usage`, so this test
// isolates the 361 Deinit from the per-spec left_col Deinit at 414.
// ---- 414:9 cxx_remove_void_call -----------------------------------------
// print_help builds one left-column Str per spec (left_col[i] from
// self->alloc at 374) and StrDeinit's each in the loop at 414. Removing
// that Deinit leaks every per-spec left-column Str. Register several specs
// so left_col is populated, drive --help, and require live_count == 0.
bool test_help_frees_left_col_strs(void);
bool test_help_frees_left_col_strs(void) {
    WriteFmt("Testing print_help frees per-spec left-column Strs (414:9)\n");

    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    ArgParse p = ArgParseInit("prog", "an about line", adbg);

    Zstr listen   = NULL;
    u32  timeout  = 0;
    bool verbose  = false;
    Zstr hostname = NULL;
    ArgRequired(&p, "-l", "--listen", &listen, "host:port to listen on");
    ArgOptional(&p, NULL, "--timeout", &timeout, "connection timeout");
    ArgFlag(&p, "-v", "--verbose", &verbose, "verbose logging");
    ArgPositional(&p, "hostname", &hostname, "name to resolve");

    char  *argv[] = {(char *)"prog", (char *)"--help"};
    ArgRun rc     = ArgParseRun(&p, 2, argv);

    ArgParseDeinit(&p);

    // With four registered specs (plus the auto --help spec) left_col is
    // populated; the 414 loop must free each. A removed Deinit leaks them.
    bool ok = (rc == ARG_RUN_HELP) && (DebugAllocatorLiveCount(&dbg) == 0);

    DebugAllocatorDeinit(&dbg);
    return ok;
}

int main(void) {
    TestFunction tests[] = {
        test_help_frees_left_col_strs,
    };
    TestFunction deadend_tests[] = {0};
    (void)deadend_tests;
    return run_test_suite(tests, (int)(sizeof(tests) / sizeof(tests[0])), NULL, 0, "ArgParse.Leak");
}
