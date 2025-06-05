#include <Misra/Std/Container/BitVec.h>
#include <Misra/Std/Log.h>
#include <stdio.h>
#include <Misra/Types.h>

// Include test utilities
#include "../Util/TestRunner.h"

// Function prototypes
bool test_bitvec_foreach_idx(void);
bool test_bitvec_foreach(void);
bool test_bitvec_foreach_reverse_idx(void);
bool test_bitvec_foreach_reverse(void);
bool test_bitvec_foreach_in_range_idx(void);
bool test_bitvec_foreach_in_range(void);
bool test_bitvec_foreach_edge_cases(void);
bool test_bitvec_foreach_idx_edge_cases(void);
bool test_bitvec_foreach_reverse_edge_cases(void);
bool test_bitvec_foreach_range_edge_cases(void);
bool test_bitvec_foreach_stress_test(void);
bool test_bitvec_foreach_invalid_usage(void);


// Test BitVecForeachIdx macro
bool test_bitvec_foreach_idx(void) {
    printf("Testing BitVecForeachIdx macro\n");

    BitVec bv = BitVecInit();

    // Add test pattern: true, false, true, false
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);

    // Test forward iteration with index
    u64 count           = 0;
    bool pattern_correct = true;

    BitVecForeachIdx(&bv, bit, idx, {
        if (idx == 0 || idx == 2) {
            pattern_correct = pattern_correct && (bit == true);
        } else {
            pattern_correct = pattern_correct && (bit == false);
        }
        count++;
    });

    bool result = (count == 4) && pattern_correct;

    // Clean up
    BitVecDeinit(&bv);

    return result;
}

// Test BitVecForeach macro
bool test_bitvec_foreach(void) {
    printf("Testing BitVecForeach macro\n");

    BitVec bv = BitVecInit();

    // Add test pattern: true, false, true
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecPush(&bv, true);

    // Test forward iteration without explicit index
    u64 true_count  = 0;
    u64 false_count = 0;

    BitVecForeach(&bv, bit, {
        if (bit) {
            true_count++;
        } else {
            false_count++;
        }
    });

    bool result = (true_count == 2) && (false_count == 1);

    // Clean up
    BitVecDeinit(&bv);

    return result;
}

// Test BitVecForeachReverseIdx macro
bool test_bitvec_foreach_reverse_idx(void) {
    printf("Testing BitVecForeachReverseIdx macro\n");

    BitVec bv = BitVecInit();

    // Add test pattern: true, false, true, false
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);

    // Test reverse iteration with index
    u64 count              = 0;
    bool first_bit_is_false = false; // Should be last bit when iterating in reverse

    BitVecForeachReverseIdx(&bv, bit, idx, {
        if (count == 0) {
            first_bit_is_false = (bit == false) && (idx == 3);
        }
        count++;
    });

    bool result = (count == 4) && first_bit_is_false;

    // Clean up
    BitVecDeinit(&bv);

    return result;
}

// Test BitVecForeachReverse macro
bool test_bitvec_foreach_reverse(void) {
    printf("Testing BitVecForeachReverse macro\n");

    BitVec bv = BitVecInit();

    // Add test pattern: true, false, true
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecPush(&bv, true);

    // Test reverse iteration
    u64 count         = 0;
    bool first_is_true = false; // Should be the last bit (true)

    BitVecForeachReverse(&bv, bit, {
        if (count == 0) {
            first_is_true = (bit == true);
        }
        count++;
    });

    bool result = (count == 3) && first_is_true;

    // Clean up
    BitVecDeinit(&bv);

    return result;
}

// Test BitVecForeachInRangeIdx macro
bool test_bitvec_foreach_in_range_idx(void) {
    printf("Testing BitVecForeachInRangeIdx macro\n");

    BitVec bv = BitVecInit();

    // Add test pattern: true, false, true, false, true
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecPush(&bv, true);

    // Test range iteration from index 1 to 4 (exclusive)
    u64 count         = 0;
    bool range_correct = true;

    BitVecForeachInRangeIdx(&bv, bit, idx, 1, 4, {
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
    BitVecDeinit(&bv);

    return result;
}

// Test BitVecForeachInRange macro
bool test_bitvec_foreach_in_range(void) {
    printf("Testing BitVecForeachInRange macro\n");

    BitVec bv = BitVecInit();

    // Add test pattern: false, true, true, false, true
    BitVecPush(&bv, false);
    BitVecPush(&bv, true);
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecPush(&bv, true);

    // Test range iteration from index 1 to 3 (exclusive)
    u64 true_count  = 0;
    u64 false_count = 0;

    BitVecForeachInRange(&bv, bit, 1, 3, {
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
    BitVecDeinit(&bv);

    return result;
}

// Edge case tests
bool test_bitvec_foreach_edge_cases(void) {
    printf("Testing BitVec foreach edge cases\n");

    BitVec bv     = BitVecInit();
    bool   result = true;
    int    count  = 0;

    // Test foreach on empty bitvec
    BitVecForeach(&bv, bit, {
        count++; // Should not execute
    });
    result = result && (count == 0);

    // Test foreach on single element
    BitVecPush(&bv, true);
    count = 0;
    BitVecForeach(&bv, bit, {
        count++;
        result = result && (bit == true);
    });
    result = result && (count == 1);

    // Test foreach on large data
    BitVecClear(&bv);
    for (int i = 0; i < 1000; i++) {
        BitVecPush(&bv, i % 2 == 0);
    }

    count = 0;
    BitVecForeach(&bv, bit, { count++; });
    result = result && (count == 1000);

    BitVecDeinit(&bv);
    return result;
}

bool test_bitvec_foreach_idx_edge_cases(void) {
    printf("Testing BitVec foreach idx edge cases\n");

    BitVec bv       = BitVecInit();
    bool   result   = true;
    u64   last_idx = SIZE_MAX;

    // Test foreach idx on empty bitvec
    BitVecForeachIdx(&bv, bit, idx, {
        result = false; // Should not execute
    });

    // Test foreach idx on single element
    BitVecPush(&bv, false);
    BitVecForeachIdx(&bv, bit, idx, {
        result   = result && (idx == 0);
        result   = result && (bit == false);
        last_idx = idx;
    });
    result = result && (last_idx == 0);

    // Test foreach idx ordering
    BitVecClear(&bv);
    for (int i = 0; i < 10; i++) {
        BitVecPush(&bv, i % 2 == 0);
    }

    u64 expected_idx = 0;
    BitVecForeachIdx(&bv, bit, idx, {
        result = result && (idx == expected_idx);
        expected_idx++;
    });

    BitVecDeinit(&bv);
    return result;
}

bool test_bitvec_foreach_reverse_edge_cases(void) {
    printf("Testing BitVec foreach reverse edge cases\n");

    BitVec bv     = BitVecInit();
    bool   result = true;

    // Test reverse foreach on empty bitvec
    BitVecForeachReverse(&bv, bit, {
        result = false; // Should not execute
    });

    // Test reverse foreach on single element
    BitVecPush(&bv, true);
    int count = 0;
    BitVecForeachReverse(&bv, bit, {
        count++;
        result = result && (bit == true);
    });
    result = result && (count == 1);

    // Test reverse ordering
    BitVecClear(&bv);
    BitVecPush(&bv, true);                          // idx 0
    BitVecPush(&bv, false);                         // idx 1
    BitVecPush(&bv, true);                          // idx 2

    bool expected_sequence[] = {true, false, true}; // Reverse order
    int  seq_idx             = 0;
    BitVecForeachReverse(&bv, bit, {
        result = result && (bit == expected_sequence[seq_idx]);
        seq_idx++;
    });

    BitVecDeinit(&bv);
    return result;
}

bool test_bitvec_foreach_range_edge_cases(void) {
    printf("Testing BitVec foreach range edge cases\n");

    BitVec bv     = BitVecInit();
    bool   result = true;

    // Setup test data
    for (int i = 0; i < 10; i++) {
        BitVecPush(&bv, i % 2 == 0);
    }

    // Test range with start == end (should not execute)
    int count = 0;
    BitVecForeachInRange(&bv, bit, 5, 5, {
        count++; // Should not execute
    });
    result = result && (count == 0);

    // Test range with single element
    count = 0;
    BitVecForeachInRange(&bv, bit, 3, 4, {
        count++;
        result = result && (bit == false); // 3 % 2 != 0
    });
    result = result && (count == 1);

    // Test range at boundaries
    count = 0;
    BitVecForeachInRange(&bv, bit, 0, 2, { count++; });
    result = result && (count == 2);

    // Test range at the end of bitvector
    count = 0;
    BitVecForeachInRange(&bv, bit, 8, 10, { count++; });
    result = result && (count == 2); // Should iterate over indices 8,9

    BitVecDeinit(&bv);
    return result;
}

bool test_bitvec_foreach_stress_test(void) {
    printf("Testing BitVec foreach stress test\n");

    bool result = true;

    for (int sz = 0; sz < 100; sz += 10) {
        BitVec bv = BitVecInit();

        // Create bitvec of varying sz
        for (int i = 0; i < sz; i++) {
            BitVecPush(&bv, i % 3 == 0);
        }

        // Test all foreach variants
        int count1 = 0, count2 = 0, count3 = 0, count4 = 0;

        BitVecForeach(&bv, bitval, { count1++; });
        BitVecForeachIdx(&bv, bitval, i, { count2++; });
        BitVecForeachReverse(&bv, bitval, { count3++; });
        BitVecForeachReverseIdx(&bv, bitval, i, { count4++; });

        result = result && (count1 == sz);
        result = result && (count2 == sz);
        result = result && (count3 == sz);
        result = result && (count4 == sz);

        BitVecDeinit(&bv);
    }

    return result;
}

// Deadend tests - Note: foreach macros don't have traditional NULL checks
// since they operate on the bitvec structure directly, but we can test
// with invalid macro usage scenarios

bool test_bitvec_foreach_invalid_usage(void) {
    printf("Testing BitVec foreach with invalid bitvec\n");

    // Test foreach with invalid bitvec (length > 0 but data is NULL)
    BitVec bv = {.length = 5, .capacity = 10, .data = NULL, .byte_size = 0};

    // This should abort due to ValidateBitVec check
    int count = 0;
    BitVecForeach(&bv, bit, { count++; });

    // Should not reach here
    return false;
}

// Main function that runs all tests
int main(void) {
    printf("[INFO] Starting BitVec.Foreach tests\n\n");

    // Array of normal test functions
    TestFunction tests[] = {
        test_bitvec_foreach_idx,
        test_bitvec_foreach,
        test_bitvec_foreach_reverse_idx,
        test_bitvec_foreach_reverse,
        test_bitvec_foreach_in_range_idx,
        test_bitvec_foreach_in_range,
        test_bitvec_foreach_edge_cases,
        test_bitvec_foreach_idx_edge_cases,
        test_bitvec_foreach_reverse_edge_cases,
        test_bitvec_foreach_range_edge_cases,
        test_bitvec_foreach_stress_test
    };

    // Array of deadend test functions
    TestFunction deadend_tests[] = {test_bitvec_foreach_invalid_usage};

    int total_tests         = sizeof(tests) / sizeof(tests[0]);
    int total_deadend_tests = sizeof(deadend_tests) / sizeof(deadend_tests[0]);

    // Run all tests using the centralized test driver
    return run_test_suite(tests, total_tests, deadend_tests, total_deadend_tests, "BitVec.Foreach");
}