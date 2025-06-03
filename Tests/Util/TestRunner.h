/// file      : Tests/Util/TestRunner.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Test utilities for running potentially failing tests in separate processes

#ifndef TESTS_UTIL_TEST_RUNNER_H
#define TESTS_UTIL_TEST_RUNNER_H

#include <stdbool.h>

///
/// Function pointer type for test functions that return bool
///
typedef bool (*TestFunction)(void);

///
/// Run a test function in a separate process (child process) to safely
/// handle potentially crashing or failing tests.
///
/// test_func[in]    : The test function to execute in child process
/// expect_failure[in]: If true, expects the test to fail (non-zero exit code)
///                     If false, expects the test to succeed (zero exit code)
///
/// SUCCESS: Returns true if the test behaved as expected:
///          - If expect_failure=false and child returned 0 (success)
///          - If expect_failure=true and child returned non-zero (failure)
/// FAILURE: Returns false if the test didn't behave as expected:
///          - If expect_failure=false and child returned non-zero (unexpected failure)
///          - If expect_failure=true and child returned 0 (unexpected success)
///          - If there was an error creating/managing the child process
///
/// TAGS: Testing, Process, Safety, Isolation
///
bool test_deadend(TestFunction test_func, bool expect_failure);

/// Run an array of simple tests
/// @param tests Array of test function pointers
/// @param count Number of tests in the array
/// @return Number of failed tests (0 = all passed)
int simple_test_driver(TestFunction* tests, int count);

/// Run an array of deadend tests (all expecting failure)
/// @param tests Array of test function pointers
/// @param count Number of tests in the array
/// @return Number of failed tests (0 = all passed)
int deadend_test_driver(TestFunction* tests, int count);

#endif // TESTS_UTIL_TEST_RUNNER_H 