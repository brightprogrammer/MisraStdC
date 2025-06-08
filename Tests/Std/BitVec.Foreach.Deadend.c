#include <Misra/Std/Container/Bits.h>
#include <Misra/Std/Log.h>

#include <stdio.h>
#include <Misra/Types.h>

// Include test utilities
#include "../Util/TestRunner.h"

// Function prototypes for deadend tests
bool test_Bits_foreach_invalid_usage(void);
bool test_Bits_run_lengths_null_bv(void);
bool test_Bits_run_lengths_null_runs(void);
bool test_Bits_run_lengths_null_values(void);
bool test_Bits_run_lengths_zero_max_runs(void);

// Deadend tests for BitsRunLengths

bool test_Bits_run_lengths_null_bv(void) {
    printf("Testing BitsRunLengths with NULL Bitstor\n");

    u64  runs[5];
    bool values[5];

    // This should cause LOG_FATAL and terminate the program
    BitsRunLengths(NULL, runs, values, 5);

    return true; // Should never reach here
}

bool test_Bits_run_lengths_null_runs(void) {
    printf("Testing BitsRunLengths with NULL runs array\n");

    Bits bv = BitsInit();
    BitsPush(&bv, true);
    bool values[5];

    // This should cause LOG_FATAL and terminate the program
    BitsRunLengths(&bv, NULL, values, 5);

    BitsDeinit(&bv);
    return true; // Should never reach here
}

bool test_Bits_run_lengths_null_values(void) {
    printf("Testing BitsRunLengths with NULL values array\n");

    Bits bv = BitsInit();
    BitsPush(&bv, true);
    u64 runs[5];

    // This should cause LOG_FATAL and terminate the program
    BitsRunLengths(&bv, runs, NULL, 5);

    BitsDeinit(&bv);
    return true; // Should never reach here
}

bool test_Bits_run_lengths_zero_max_runs(void) {
    printf("Testing BitsRunLengths with zero max_runs\n");

    Bits bv = BitsInit();
    BitsPush(&bv, true);
    u64  runs[5];
    bool values[5];

    // This should cause LOG_FATAL and terminate the program
    BitsRunLengths(&bv, runs, values, 0);

    BitsDeinit(&bv);
    return true; // Should never reach here
}

// Deadend tests - Note: foreach macros don't have traditional NULL checks
// since they operate on the Bits structure directly, but we can test
// with invalid macro usage scenarios

bool test_Bits_foreach_invalid_usage(void) {
    printf("Testing Bits foreach with invalid Bits\n");

    // Test foreach with invalid Bits (length > 0 but data is NULL)
    Bits bv = {.length = 5, .capacity = 10, .data = NULL, .byte_size = 0};

    // This should abort due to ValidateBits check
    int count = 0;
    BitsForeach(&bv, bit, {
        (void)bit;
        count++;
    });

    // Should not reach here
    (void)count; // Silence unused variable warning
    return false;
}

// Main function that runs all deadend tests
int main(void) {
    printf("[INFO] Starting Bits.Foreach.Deadend tests\n\n");

    // Array of deadend test functions
    TestFunction deadend_tests[] = {
        test_Bits_foreach_invalid_usage,
        test_Bits_run_lengths_null_bv,
        test_Bits_run_lengths_null_runs,
        test_Bits_run_lengths_null_values,
        test_Bits_run_lengths_zero_max_runs
    };

    int total_deadend_tests = sizeof(deadend_tests) / sizeof(deadend_tests[0]);

    // Run all deadend tests using the centralized test driver
    return run_test_suite(NULL, 0, deadend_tests, total_deadend_tests, "Bits.Foreach.Deadend");
}
