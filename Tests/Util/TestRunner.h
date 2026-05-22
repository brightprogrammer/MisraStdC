/// file      : Tests/Util/TestRunner.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Test utilities for running potentially failing tests using setjmp/longjmp

#ifndef MISRA_TEST_RUNNER_H
#define MISRA_TEST_RUNNER_H

#include <Misra/Types.h>

///
/// Function pointer type for test functions.
///
/// SUCCESS : Function returns true if test passed, false if failed.
/// FAILURE : Function cannot fail - always returns boolean result.
///
/// TAGS: Testing, Function, Pointer
///
typedef bool (*TestFunction)(void);

///
/// Run a specific test function using setjmp/longjmp to capture aborts.
/// This is used for deadend tests that are expected to call LOG_FATAL.
///
/// test_func[in]     : Test function to execute.
/// expect_failure[in]: If true, expects the test to call Abort().
///
/// SUCCESS : Returns true if test behaved as expected.
/// FAILURE : Returns false if test did not behave as expected.
///
/// TAGS: Testing, Deadend, Control
///
bool test_deadend(TestFunction test_func, bool expect_failure);

///
/// Run an array of simple test functions.
///
/// tests[in] : Array of test function pointers.
/// count[in] : Number of tests in the array.
///
/// SUCCESS : Returns number of failed tests (0 = all passed).
/// FAILURE : Function cannot fail - always returns valid count.
///
/// TAGS: Testing, Driver, Simple
///
int simple_test_driver(TestFunction *tests, int count);

///
/// Run an array of deadend test functions (all expecting failure).
///
/// tests[in] : Array of test function pointers.
/// count[in] : Number of tests in the array.
///
/// SUCCESS : Returns number of failed tests (0 = all passed).
/// FAILURE : Function cannot fail - always returns valid count.
///
/// TAGS: Testing, Driver, Deadend
///
int deadend_test_driver(TestFunction *tests, int count);

///
/// Main test driver that handles both normal and deadend tests.
///
/// normal_tests[in]   : Array of normal test function pointers (can be NULL).
/// normal_count[in]   : Number of normal tests.
/// deadend_tests[in]  : Array of deadend test function pointers (can be NULL).
/// deadend_count[in]  : Number of deadend tests.
/// test_name[in]      : Name of the test suite for logging.
///
/// SUCCESS : Returns 0 if all tests passed.
/// FAILURE : Returns non-zero if some tests failed.
///
/// TAGS: Testing, Driver, Suite
///
int run_test_suite(
    TestFunction *normal_tests,
    int           normal_count,
    TestFunction *deadend_tests,
    int           deadend_count,
    const char   *test_name
);

#endif // MISRA_TEST_RUNNER_H