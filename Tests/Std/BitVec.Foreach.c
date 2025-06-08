#include <Misra/Std/Container/Bits.h>
#include <Misra/Std/Log.h>
#include <stdio.h>
#include <Misra/Types.h>

// Include test utilities
#include "../Util/TestRunner.h"

// Function prototypes
bool test_Bits_foreach_idx(void);
bool test_Bits_foreach(void);
bool test_Bits_foreach_reverse_idx(void);
bool test_Bits_foreach_reverse(void);
bool test_Bits_foreach_in_range_idx(void);
bool test_Bits_foreach_in_range(void);
bool test_Bits_foreach_edge_cases(void);
bool test_Bits_foreach_idx_edge_cases(void);
bool test_Bits_foreach_reverse_edge_cases(void);
bool test_Bits_foreach_range_edge_cases(void);
bool test_Bits_foreach_stress_test(void);
bool test_Bits_foreach_invalid_usage(void);

// BitsRunLengths test prototypes
bool test_Bits_run_lengths_basic(void);
bool test_Bits_run_lengths_edge_cases(void);
bool test_Bits_run_lengths_boundary_conditions(void);
bool test_Bits_run_lengths_stress_test(void);
bool test_Bits_run_lengths_null_bv(void);
bool test_Bits_run_lengths_null_runs(void);
bool test_Bits_run_lengths_null_values(void);
bool test_Bits_run_lengths_zero_max_runs(void);


// Test BitsForeachIdx macro
bool test_Bits_foreach_idx(void) {
    printf("Testing BitsForeachIdx macro\n");

    Bits bv = BitsInit();

    // Add test pattern: true, false, true, false
    BitsPush(&bv, true);
    BitsPush(&bv, false);
    BitsPush(&bv, true);
    BitsPush(&bv, false);

    // Test forward iteration with index
    u64  count           = 0;
    bool pattern_correct = true;

    BitsForeachIdx(&bv, bit, idx, {
        if (idx == 0 || idx == 2) {
            pattern_correct = pattern_correct && (bit == true);
        } else {
            pattern_correct = pattern_correct && (bit == false);
        }
        count++;
    });

    bool result = (count == 4) && pattern_correct;

    // Clean up
    BitsDeinit(&bv);

    return result;
}

// Test BitsForeach macro
bool test_Bits_foreach(void) {
    printf("Testing BitsForeach macro\n");

    Bits bv = BitsInit();

    // Add test pattern: true, false, true
    BitsPush(&bv, true);
    BitsPush(&bv, false);
    BitsPush(&bv, true);

    // Test forward iteration without explicit index
    u64 true_count  = 0;
    u64 false_count = 0;

    BitsForeach(&bv, bit, {
        if (bit) {
            true_count++;
        } else {
            false_count++;
        }
    });

    bool result = (true_count == 2) && (false_count == 1);

    // Clean up
    BitsDeinit(&bv);

    return result;
}

// Test BitsForeachReverseIdx macro
bool test_Bits_foreach_reverse_idx(void) {
    printf("Testing BitsForeachReverseIdx macro\n");

    Bits bv = BitsInit();

    // Add test pattern: true, false, true, false
    BitsPush(&bv, true);
    BitsPush(&bv, false);
    BitsPush(&bv, true);
    BitsPush(&bv, false);

    // Test reverse iteration with index
    u64  count              = 0;
    bool first_bit_is_false = false; // Should be last bit when iterating in reverse

    BitsForeachReverseIdx(&bv, bit, idx, {
        if (count == 0) {
            first_bit_is_false = (bit == false) && (idx == 3);
        }
        count++;
    });

    bool result = (count == 4) && first_bit_is_false;

    // Clean up
    BitsDeinit(&bv);

    return result;
}

// Test BitsForeachReverse macro
bool test_Bits_foreach_reverse(void) {
    printf("Testing BitsForeachReverse macro\n");

    Bits bv = BitsInit();

    // Add test pattern: true, false, true
    BitsPush(&bv, true);
    BitsPush(&bv, false);
    BitsPush(&bv, true);

    // Test reverse iteration
    u64  count         = 0;
    bool first_is_true = false; // Should be the last bit (true)

    BitsForeachReverse(&bv, bit, {
        if (count == 0) {
            first_is_true = (bit == true);
        }
        count++;
    });

    bool result = (count == 3) && first_is_true;

    // Clean up
    BitsDeinit(&bv);

    return result;
}

// Test BitsForeachInRangeIdx macro
bool test_Bits_foreach_in_range_idx(void) {
    printf("Testing BitsForeachInRangeIdx macro\n");

    Bits bv = BitsInit();

    // Add test pattern: true, false, true, false, true
    BitsPush(&bv, true);
    BitsPush(&bv, false);
    BitsPush(&bv, true);
    BitsPush(&bv, false);
    BitsPush(&bv, true);

    // Test range iteration from index 1 to 4 (exclusive)
    u64  count         = 0;
    bool range_correct = true;

    BitsForeachInRangeIdx(&bv, bit, idx, 1, 4, {
        // Should iterate over indices 1, 2, 3
        // Values: false, true, false
        if (idx == 1 || idx == 3) {
            range_correct = range_correct && (bit == false);
        } else if (idx == 2) {
            range_correct = range_correct && (bit == true);
        }
        count++;
    });

    bool result = (count == 3) && range_correct;

    // Clean up
    BitsDeinit(&bv);

    return result;
}

// Test BitsForeachInRange macro
bool test_Bits_foreach_in_range(void) {
    printf("Testing BitsForeachInRange macro\n");

    Bits bv = BitsInit();

    // Add test pattern: false, true, true, false, true
    BitsPush(&bv, false);
    BitsPush(&bv, true);
    BitsPush(&bv, true);
    BitsPush(&bv, false);
    BitsPush(&bv, true);

    // Test range iteration from index 1 to 3 (exclusive)
    u64 true_count  = 0;
    u64 false_count = 0;

    BitsForeachInRange(&bv, bit, 1, 3, {
        // Should iterate over indices 1, 2
        // Values: true, true
        if (bit) {
            true_count++;
        } else {
            false_count++;
        }
    });

    bool result = (true_count == 2) && (false_count == 0);

    // Clean up
    BitsDeinit(&bv);

    return result;
}

// Edge case tests
bool test_Bits_foreach_edge_cases(void) {
    printf("Testing Bits foreach edge cases\n");

    Bits bv     = BitsInit();
    bool   result = true;
    int    count  = 0;

    // Test foreach on empty Bits
    BitsForeach(&bv, bit, {
        (void)bit;
        count++; // Should not execute
    });
    result = result && (count == 0);

    // Test foreach on single element
    BitsPush(&bv, true);
    count = 0;
    BitsForeach(&bv, bit, {
        count++;
        result = result && (bit == true);
    });
    result = result && (count == 1);

    // Test foreach on large data
    BitsClear(&bv);
    for (int i = 0; i < 1000; i++) {
        BitsPush(&bv, i % 2 == 0);
    }

    count = 0;
    BitsForeach(&bv, bit, {
        (void)bit;
        count++;
    });
    result = result && (count == 1000);

    BitsDeinit(&bv);
    return result;
}

bool test_Bits_foreach_idx_edge_cases(void) {
    printf("Testing Bits foreach idx edge cases\n");

    Bits bv       = BitsInit();
    bool   result   = true;
    u64    last_idx = SIZE_MAX;

    // Test foreach idx on empty Bits
    BitsForeachIdx(&bv, bit, idx, {
        (void)bit;
        result = false; // Should not execute
    });

    // Test foreach idx on single element
    BitsPush(&bv, false);
    BitsForeachIdx(&bv, bit, idx, {
        result   = result && (idx == 0);
        result   = result && (bit == false);
        last_idx = idx;
    });
    result = result && (last_idx == 0);

    // Test foreach idx ordering
    BitsClear(&bv);
    for (int i = 0; i < 10; i++) {
        BitsPush(&bv, i % 2 == 0);
    }

    u64 expected_idx = 0;
    BitsForeachIdx(&bv, bit, idx, {
        (void)bit;
        result = result && (idx == expected_idx);
        expected_idx++;
    });

    BitsDeinit(&bv);
    return result;
}

bool test_Bits_foreach_reverse_edge_cases(void) {
    printf("Testing Bits foreach reverse edge cases\n");

    Bits bv     = BitsInit();
    bool   result = true;

    // Test reverse foreach on empty Bits
    BitsForeachReverse(&bv, bit, {
        (void)bit;
        result = false; // Should not execute
    });

    // Test reverse foreach on single element
    BitsPush(&bv, true);
    int count = 0;
    BitsForeachReverse(&bv, bit, {
        count++;
        result = result && (bit == true);
    });
    result = result && (count == 1);

    // Test reverse ordering
    BitsClear(&bv);
    BitsPush(&bv, true);                          // idx 0
    BitsPush(&bv, false);                         // idx 1
    BitsPush(&bv, true);                          // idx 2

    bool expected_sequence[] = {true, false, true}; // Reverse order
    int  seq_idx             = 0;
    BitsForeachReverse(&bv, bit, {
        result = result && (bit == expected_sequence[seq_idx]);
        seq_idx++;
    });

    BitsDeinit(&bv);
    return result;
}

bool test_Bits_foreach_range_edge_cases(void) {
    printf("Testing Bits foreach range edge cases\n");

    Bits bv     = BitsInit();
    bool   result = true;

    // Setup test data
    for (int i = 0; i < 10; i++) {
        BitsPush(&bv, i % 2 == 0);
    }

    // Test range with start == end (should not execute)
    int count = 0;
    BitsForeachInRange(&bv, bit, 5, 5, {
        (void)bit;
        count++; // Should not execute
    });
    result = result && (count == 0);

    // Test range with single element
    count = 0;
    BitsForeachInRange(&bv, bit, 3, 4, {
        count++;
        result = result && (bit == false); // 3 % 2 != 0
    });
    result = result && (count == 1);

    // Test range at boundaries
    count = 0;
    BitsForeachInRange(&bv, bit, 0, 2, {
        (void)bit;
        count++;
    });
    result = result && (count == 2);

    // Test range at the end of Bitstor
    count = 0;
    BitsForeachInRange(&bv, bit, 8, 10, {
        (void)bit;
        count++;
    });
    result = result && (count == 2); // Should iterate over indices 8,9

    BitsDeinit(&bv);
    return result;
}

bool test_Bits_foreach_stress_test(void) {
    printf("Testing Bits foreach stress test\n");

    bool result = true;

    for (int sz = 0; sz < 100; sz += 10) {
        Bits bv = BitsInit();

        // Create Bits of varying sz
        for (int i = 0; i < sz; i++) {
            BitsPush(&bv, i % 3 == 0);
        }

        // Test all foreach variants
        int count1 = 0, count2 = 0, count3 = 0, count4 = 0;

        BitsForeach(&bv, bitval, {
            (void)bitval;
            count1++;
        });
        BitsForeachIdx(&bv, bitval, i, {
            (void)bitval;
            count2++;
        });
        BitsForeachReverse(&bv, bitval, {
            (void)bitval;
            count3++;
        });
        BitsForeachReverseIdx(&bv, bitval, i, {
            (void)bitval;
            count4++;
        });

        result = result && (count1 == sz);
        result = result && (count2 == sz);
        result = result && (count3 == sz);
        result = result && (count4 == sz);

        BitsDeinit(&bv);
    }

    return result;
}

// BitsRunLengths test implementations

bool test_Bits_run_lengths_basic(void) {
    printf("Testing BitsRunLengths basic functionality\n");

    Bits bv     = BitsInit();
    bool   result = true;

    // Test pattern: 11100101 (3 true, 2 false, 1 true, 1 false, 1 true)
    BitsPush(&bv, true);  // 0
    BitsPush(&bv, true);  // 1
    BitsPush(&bv, true);  // 2
    BitsPush(&bv, false); // 3
    BitsPush(&bv, false); // 4
    BitsPush(&bv, true);  // 5
    BitsPush(&bv, false); // 6
    BitsPush(&bv, true);  // 7

    u64  runs[10];
    bool values[10];
    u64  count = BitsRunLengths(&bv, runs, values, 10);

    // Should find 5 runs
    result = result && (count == 5);

    // Verify each run
    if (count >= 5) {
        result = result && (runs[0] == 3 && values[0] == true);  // 3 trues
        result = result && (runs[1] == 2 && values[1] == false); // 2 falses
        result = result && (runs[2] == 1 && values[2] == true);  // 1 true
        result = result && (runs[3] == 1 && values[3] == false); // 1 false
        result = result && (runs[4] == 1 && values[4] == true);  // 1 true
    }

    BitsDeinit(&bv);
    return result;
}

bool test_Bits_run_lengths_edge_cases(void) {
    printf("Testing BitsRunLengths edge cases\n");

    bool result = true;

    // Test 1: Empty Bitstor
    Bits empty_bv = BitsInit();
    u64    runs[5];
    bool   values[5];
    u64    count = BitsRunLengths(&empty_bv, runs, values, 5);
    result       = result && (count == 0);
    BitsDeinit(&empty_bv);

    // Test 2: Single bit (true)
    Bits single_bv = BitsInit();
    BitsPush(&single_bv, true);
    count  = BitsRunLengths(&single_bv, runs, values, 5);
    result = result && (count == 1);
    result = result && (runs[0] == 1 && values[0] == true);
    BitsDeinit(&single_bv);

    // Test 3: Single bit (false)
    Bits single_false_bv = BitsInit();
    BitsPush(&single_false_bv, false);
    count  = BitsRunLengths(&single_false_bv, runs, values, 5);
    result = result && (count == 1);
    result = result && (runs[0] == 1 && values[0] == false);
    BitsDeinit(&single_false_bv);

    // Test 4: All same bits (all true)
    Bits all_true_bv = BitsInit();
    for (int i = 0; i < 10; i++) {
        BitsPush(&all_true_bv, true);
    }
    count  = BitsRunLengths(&all_true_bv, runs, values, 5);
    result = result && (count == 1);
    result = result && (runs[0] == 10 && values[0] == true);
    BitsDeinit(&all_true_bv);

    // Test 5: Alternating bits (0101010)
    Bits alternating_bv = BitsInit();
    for (int i = 0; i < 7; i++) {
        BitsPush(&alternating_bv, i % 2 == 0);
    }
    u64  alt_runs[10];
    bool alt_values[10];
    count  = BitsRunLengths(&alternating_bv, alt_runs, alt_values, 10);
    result = result && (count == 7); // Each bit is its own run
    // Verify alternating pattern
    for (u64 i = 0; i < count && i < 7; i++) {
        result = result && (alt_runs[i] == 1);
        result = result && (alt_values[i] == (i % 2 == 0));
    }
    BitsDeinit(&alternating_bv);

    return result;
}

bool test_Bits_run_lengths_boundary_conditions(void) {
    printf("Testing BitsRunLengths boundary conditions\n");

    Bits bv     = BitsInit();
    bool   result = true;

    // Create pattern with many runs: 10101010 (8 runs)
    for (int i = 0; i < 8; i++) {
        BitsPush(&bv, i % 2 == 0);
    }

    // Test with limited max_runs
    u64  runs[3];
    bool values[3];
    u64  count = BitsRunLengths(&bv, runs, values, 3);

    // Should only capture first 3 runs due to limit
    result = result && (count == 3);
    result = result && (runs[0] == 1 && values[0] == true);  // First run
    result = result && (runs[1] == 1 && values[1] == false); // Second run
    result = result && (runs[2] == 1 && values[2] == true);  // Third run

    // Test with max_runs = 1
    count  = BitsRunLengths(&bv, runs, values, 1);
    result = result && (count == 1);
    result = result && (runs[0] == 1 && values[0] == true);

    // Test with exact number of runs needed
    u64  large_runs[8];
    bool large_values[8];
    count  = BitsRunLengths(&bv, large_runs, large_values, 8);
    result = result && (count == 8);

    BitsDeinit(&bv);
    return result;
}

bool test_Bits_run_lengths_stress_test(void) {
    printf("Testing BitsRunLengths stress test\n");

    bool result = true;

    // Test with large Bitstor
    Bits large_bv = BitsInit();

    // Create pattern that results in many runs
    // Pattern: blocks of 5 same bits, alternating true/false
    for (int block = 0; block < 100; block++) {
        bool value = (block % 2 == 0);
        for (int i = 0; i < 5; i++) {
            BitsPush(&large_bv, value);
        }
    }

    u64  runs[200];
    bool values[200];
    u64  count = BitsRunLengths(&large_bv, runs, values, 200);

    // Should have 100 runs (100 blocks)
    result = result && (count == 100);

    // Verify each run has length 5 and alternating values
    for (u64 i = 0; i < count && i < 100; i++) {
        result = result && (runs[i] == 5);
        result = result && (values[i] == (i % 2 == 0));
    }

    // Test with complex random-like pattern
    BitsClear(&large_bv);

    // Pattern: 1111000011100010000111 (variable length runs)
    // Run 1: 4 trues, Run 2: 4 falses, Run 3: 3 trues, Run 4: 3 falses, Run 5: 1 true, Run 6: 4 falses, Run 7: 3 trues
    bool pattern[]   = {1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 1};
    int  pattern_len = sizeof(pattern) / sizeof(pattern[0]);

    for (int i = 0; i < pattern_len; i++) {
        BitsPush(&large_bv, pattern[i]);
    }

    u64  small_runs[10];
    bool small_values[10];
    count = BitsRunLengths(&large_bv, small_runs, small_values, 10);

    // Expected runs: [4T, 4F, 3T, 3F, 1T, 4F, 3T] = 7 runs
    result = result && (count == 7);
    if (count >= 7) {
        result = result && (small_runs[0] == 4 && small_values[0] == true);
        result = result && (small_runs[1] == 4 && small_values[1] == false);
        result = result && (small_runs[2] == 3 && small_values[2] == true);
        result = result && (small_runs[3] == 3 && small_values[3] == false);
        result = result && (small_runs[4] == 1 && small_values[4] == true);
        result = result && (small_runs[5] == 4 && small_values[5] == false);
        result = result && (small_runs[6] == 3 && small_values[6] == true);
    }

    BitsDeinit(&large_bv);
    return result;
}

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

// Main function that runs all tests
int main(void) {
    printf("[INFO] Starting Bits.Foreach tests\n\n");

    // Array of normal test functions
    TestFunction tests[] = {
        test_Bits_foreach_idx,
        test_Bits_foreach,
        test_Bits_foreach_reverse_idx,
        test_Bits_foreach_reverse,
        test_Bits_foreach_in_range_idx,
        test_Bits_foreach_in_range,
        test_Bits_foreach_edge_cases,
        test_Bits_foreach_idx_edge_cases,
        test_Bits_foreach_reverse_edge_cases,
        test_Bits_foreach_range_edge_cases,
        test_Bits_foreach_stress_test,
        test_Bits_run_lengths_basic,
        test_Bits_run_lengths_edge_cases,
        test_Bits_run_lengths_boundary_conditions,
        test_Bits_run_lengths_stress_test
    };

    // Array of deadend test functions
    TestFunction deadend_tests[] = {
        test_Bits_foreach_invalid_usage,
        test_Bits_run_lengths_null_bv,
        test_Bits_run_lengths_null_runs,
        test_Bits_run_lengths_null_values,
        test_Bits_run_lengths_zero_max_runs
    };

    int total_tests         = sizeof(tests) / sizeof(tests[0]);
    int total_deadend_tests = sizeof(deadend_tests) / sizeof(deadend_tests[0]);

    // Run all tests using the centralized test driver
    return run_test_suite(tests, total_tests, deadend_tests, total_deadend_tests, "Bits.Foreach");
}

