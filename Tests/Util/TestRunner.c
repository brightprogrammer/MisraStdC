/// file      : Tests/Util/TestRunner.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Test utilities for running potentially failing tests using setjmp/longjmp

#include "TestRunner.h"
#include <Misra/Sys.h>
#include <Misra/Std.h>
#include <setjmp.h>

// Global jump buffer for capturing aborts
static jmp_buf g_test_abort_jmp;
static bool    g_abort_captured = false;

// Callback function that gets called instead of abort()
static void test_abort_handler(void) {
    g_abort_captured = true;
    longjmp(g_test_abort_jmp, 1);
}

// Run a specific test using setjmp/longjmp to capture LOG_FATAL aborts.
// `expect_failure=true`  -> pass iff the test aborts.
// `expect_failure=false` -> pass iff the test returns true without aborting.
// Both paths go through setjmp: a `false`-expectation that nevertheless
// aborts must unwind back here too, otherwise the abort handler would
// longjmp to an uninitialised buffer.
bool test_deadend(TestFunction test_func, bool expect_failure) {
    if (!test_func) {
        WriteFmt("[ERROR] test_deadend: NULL test function provided\n");
        return false;
    }

    // Install the abort capture handler for the duration of this call.
    OnAbort(test_abort_handler);

    bool test_result = false;
    if (setjmp(g_test_abort_jmp) == 0) {
        // First entry: run the test.
        bool returned = test_func();
        if (expect_failure) {
            WriteFmt("    [Unexpected success: Test completed without abort]\n");
            test_result = false; // Expected abort, got clean return.
        } else {
            WriteFmt("    [Success: Test completed normally]\n");
            test_result = returned; // Caller's bool is the verdict.
        }
    } else {
        // Re-entry via longjmp: the test triggered LOG_FATAL.
        if (expect_failure) {
            WriteFmt("    [Expected failure: Test aborted as expected]\n");
            test_result = true;  // Abort was the contract.
        } else {
            WriteFmt("    [Unexpected failure: Test aborted unexpectedly]\n");
            test_result = false;
        }
    }

    // Restore the default abort path before returning.
    OnAbort(NULL);

    return test_result;
}

/// Run an array of simple tests
int simple_test_driver(TestFunction *tests, int count) {
    if (!tests) {
        WriteFmt("[ERROR] simple_test_driver: NULL tests array provided\n");
        return count; // All tests failed
    }

    int passed = 0;
    int failed = 0;

    // Run all tests and accumulate results
    for (int i = 0; i < count; i++) {
        WriteFmt("[TEST {}/{}] ", i + 1, count);
        bool result = tests[i]();
        if (result) {
            WriteFmt("[PASS]\n\n");
            passed++;
        } else {
            WriteFmt("[FAIL]\n\n");
            failed++;
        }
    }

    // Print summary
    WriteFmt("[SUMMARY] Total: {}, Passed: {}, Failed: {}\n", count, passed, failed);

    return failed;
}

/// Run an array of deadend tests (all expecting failure)
int deadend_test_driver(TestFunction *tests, int count) {
    if (!tests) {
        WriteFmt("[ERROR] deadend_test_driver: NULL tests array provided\n");
        return count; // All tests failed
    }

    WriteFmt("\n[INFO] Testing deadend scenarios\n\n");

    int passed = 0;
    int failed = 0;

    // Run all deadend tests (expecting failure)
    for (int i = 0; i < count; i++) {
        WriteFmt("[TEST {}/{}] ", i + 1, count);
        bool result = test_deadend(tests[i], true); // All deadend tests expect failure
        if (result) {
            WriteFmt("[PASS]\n\n");
            passed++;
        } else {
            WriteFmt("[FAIL]\n\n");
            failed++;
        }
    }

    // Print summary
    WriteFmt("[SUMMARY] Deadend tests - Total: {}, Passed: {}, Failed: {}\n", count, passed, failed);

    return failed;
}

/// Main test driver - handles everything: normal tests and deadend tests
int run_test_suite(
    TestFunction *normal_tests,
    int           normal_count,
    TestFunction *deadend_tests,
    int           deadend_count,
    Zstr test_name
) {
    WriteFmt("[INFO] Starting {} tests\n\n", test_name ? test_name : "Test Suite");

    int total_failed = 0;

    // Run normal tests if any
    if (normal_tests && normal_count > 0) {
        int failed    = simple_test_driver(normal_tests, normal_count);
        total_failed += failed;
    }

    // Run deadend tests if any
    if (deadend_tests && deadend_count > 0) {
        int deadend_failed  = deadend_test_driver(deadend_tests, deadend_count);
        total_failed       += deadend_failed;
    }

    // Print final summary
    WriteFmt(
        "\n[FINAL SUMMARY] {} - Normal: {} tests, Deadend: {} tests, Total Failed: {}\n",
        test_name ? test_name : "Test Suite",
        normal_count,
        deadend_count,
        total_failed
    );

    // Return non-zero exit code if any test failed
    return total_failed > 0 ? 1 : 0;
}
