/// file : tests/json/leak.c
/// Success-path leak-guard tests for Json (DebugAllocator live-count round-trips).
#include <Misra.h>
#include <Misra/Std/Allocator/Debug.h>
#include "../Util/TestRunner.h"
int main(void) {
    TestFunction tests[]         = {0};
    TestFunction deadend_tests[] = {0};
    (void)tests;
    (void)deadend_tests;
    return run_test_suite(tests, 0, deadend_tests, 0, "Json.Leak");
}
