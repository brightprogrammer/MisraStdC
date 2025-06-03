/// file      : Tests/Util/TestRunner.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Test utilities for running potentially failing tests in separate processes

#include "TestRunner.h"

#include <stdio.h>
#include <stdlib.h>

// Platform-specific includes and definitions
#if defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
    #include <process.h>
    #define PLATFORM_WINDOWS 1
#else
    #include <sys/wait.h>
    #include <unistd.h>
    #include <signal.h>
    #define PLATFORM_UNIX 1
#endif

#ifdef PLATFORM_WINDOWS

// Windows implementation using CreateProcess
bool test_deadend(TestFunction test_func, bool expect_failure) {
    if (!test_func) {
        printf("[ERROR] test_deadend: NULL test function provided\n");
        return false;
    }

    // Set up structured exception handling
    __try {
        bool result = test_func();
        
        // Convert bool result to exit code semantics
        int exit_code = result ? 0 : 1;
        
        if (expect_failure) {
            // We expected failure, so success if exit_code != 0
            return (exit_code != 0);
        } else {
            // We expected success, so success if exit_code == 0
            return (exit_code == 0);
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        if (expect_failure) {
            return true;  // Expected failure, got crash
        } else {
            return false; // Expected success, got crash
        }
    }
}

#elif defined(PLATFORM_UNIX)

// Unix/Linux/macOS implementation using fork
bool test_deadend(TestFunction test_func, bool expect_failure) {
    if (!test_func) {
        printf("[ERROR] test_deadend: NULL test function provided\n");
        return false;
    }

    pid_t pid = fork();
    
    if (pid == -1) {
        // Fork failed
        perror("[ERROR] test_deadend: fork failed");
        return false;
    }
    
    if (pid == 0) {
        // Child process
        
        // Set up signal handlers to exit cleanly on common signals
        signal(SIGPIPE, SIG_DFL);
        signal(SIGTERM, SIG_DFL);
        signal(SIGINT, SIG_DFL);
        
        // Run the test function
        bool result = test_func();
        
        // Convert bool result to exit code
        int exit_code = result ? 0 : 1;
        
        // Exit with the result code
        _exit(exit_code);
    } else {
        // Parent process
        int status;
        pid_t waited_pid = waitpid(pid, &status, 0);
        
        if (waited_pid == -1) {
            perror("[ERROR] test_deadend: waitpid failed");
            return false;
        }
        
        int exit_code;
        
        if (WIFEXITED(status)) {
            // Normal exit
            exit_code = WEXITSTATUS(status);
        } else if (WIFSIGNALED(status)) {
            // Killed by signal
            exit_code = 128 + WTERMSIG(status);  // Convention: 128 + signal number
        } else {
            // Other termination (should not happen)
            exit_code = -1;
        }
        
        // Determine if test passed based on expectation
        if (expect_failure) {
            // We expected failure, so success if exit_code != 0
            return (exit_code != 0);
        } else {
            // We expected success, so success if exit_code == 0
            return (exit_code == 0);
        }
    }
}

#else
#error "Unsupported platform for test_deadend implementation"
#endif 

/// Run an array of simple tests
int simple_test_driver(TestFunction* tests, int count) {
    if (!tests) {
        printf("[ERROR] simple_test_driver: NULL tests array provided\n");
        return count; // All tests failed
    }

    int passed = 0;
    int failed = 0;

    // Run all tests and accumulate results
    for (int i = 0; i < count; i++) {
        printf("[TEST %d/%d] ", i + 1, count);
        bool result = tests[i]();
        if (result) {
            printf("[PASS]\n\n");
            passed++;
        } else {
            printf("[FAIL]\n\n");
            failed++;
        }
    }

    // Print summary
    printf("[SUMMARY] Total: %d, Passed: %d, Failed: %d\n", count, passed, failed);

    return failed;
}

/// Run an array of deadend tests (all expecting failure)
int deadend_test_driver(TestFunction* tests, int count) {
    if (!tests) {
        printf("[ERROR] deadend_test_driver: NULL tests array provided\n");
        return count; // All tests failed
    }

    printf("\n[INFO] Testing deadend scenarios\n\n");

    int passed = 0;
    int failed = 0;

    // Run all deadend tests (expecting failure)
    for (int i = 0; i < count; i++) {
        printf("[TEST %d/%d] ", i + 1, count);
        bool result = test_deadend(tests[i], true); // All deadend tests expect failure
        if (result) {
            printf("[PASS]\n\n");
            passed++;
        } else {
            printf("[FAIL]\n\n");
            failed++;
        }
    }

    // Print summary
    printf("[SUMMARY] Deadend tests - Total: %d, Passed: %d, Failed: %d\n", count, passed, failed);

    return failed;
} 