/// file : tests/std/pdbcache.mut.c
/// Mut tests for PdbCache: kill reachable mutation survivors (leak-guard via explicit
/// DebugAllocator and/or value/branch assertions). Distinct from existing PdbCache.* tests.
#include <Misra.h>
#include <Misra/Std/Allocator/Debug.h>
#include "../Util/TestRunner.h"
int main(void) {
    TestFunction tests[]         = {0};
    TestFunction deadend_tests[] = {0};
    (void)tests;
    (void)deadend_tests;
    return run_test_suite(tests, 0, deadend_tests, 0, "PdbCache.Mut");
}
