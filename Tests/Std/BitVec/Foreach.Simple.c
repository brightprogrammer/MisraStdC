#include <Misra/Std/Container/BitVec.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Log.h>
#include <Misra/Types.h>

// Include test utilities
#include "../../Util/TestRunner.h"

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

// BitVecRunLengths test prototypes
bool test_bitvec_run_lengths_basic(void);
bool test_bitvec_run_lengths_vec(void);
bool test_bitvec_run_lengths_edge_cases(void);
bool test_bitvec_run_lengths_boundary_conditions(void);
bool test_bitvec_run_lengths_stress_test(void);
bool test_bitvec_run_lengths_null_bv(void);
bool test_bitvec_run_lengths_null_runs(void);
bool test_bitvec_run_lengths_null_values(void);
bool test_bitvec_run_lengths_zero_max_runs(void);


// Test BitVecForeachIdx macro
bool test_bitvec_foreach_idx(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecForeachIdx macro\n");

    BitVec bv = BitVecInit(ALLOCATOR_OF(&alloc));

    // Add test pattern: true, false, true, false
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);

    // Test forward iteration with index
    u64  count           = 0;
    bool pattern_correct = true;

    BitVecForeachIdx(&bv, bit, idx) {
        if (idx == 0 || idx == 2) {
            pattern_correct = pattern_correct && (bit == true);
        } else {
            pattern_correct = pattern_correct && (bit == false);
        }
        count++;
    }

    bool result = (count == 4) && pattern_correct;

    // Clean up
    BitVecDeinit(&bv);

    DefaultAllocatorDeinit(&alloc);

    return result;
}

// Test BitVecForeach macro
bool test_bitvec_foreach(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecForeach macro\n");

    BitVec bv = BitVecInit(ALLOCATOR_OF(&alloc));

    // Add test pattern: true, false, true
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecPush(&bv, true);

    // Test forward iteration without explicit index
    u64 true_count  = 0;
    u64 false_count = 0;

    BitVecForeach(&bv, bit) {
        if (bit) {
            true_count++;
        } else {
            false_count++;
        }
    }

    bool result = (true_count == 2) && (false_count == 1);

    // Clean up
    BitVecDeinit(&bv);

    DefaultAllocatorDeinit(&alloc);

    return result;
}

// Test BitVecForeachReverseIdx macro
bool test_bitvec_foreach_reverse_idx(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecForeachReverseIdx macro\n");

    BitVec bv = BitVecInit(ALLOCATOR_OF(&alloc));

    // Add test pattern: true, false, true, false
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);

    // Test reverse iteration with index
    u64  count              = 0;
    bool first_bit_is_false = false; // Should be last bit when iterating in reverse

    BitVecForeachReverseIdx(&bv, bit, idx) {
        if (count == 0) {
            first_bit_is_false = (bit == false) && (idx == 3);
        }
        count++;
    }

    bool result = (count == 4) && first_bit_is_false;

    // Clean up
    BitVecDeinit(&bv);

    DefaultAllocatorDeinit(&alloc);

    return result;
}

// Test BitVecForeachReverse macro
bool test_bitvec_foreach_reverse(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecForeachReverse macro\n");

    BitVec bv = BitVecInit(ALLOCATOR_OF(&alloc));

    // Add test pattern: true, false, true
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecPush(&bv, true);

    // Test reverse iteration
    u64  count         = 0;
    bool first_is_true = false; // Should be the last bit (true)

    BitVecForeachReverse(&bv, bit) {
        if (count == 0) {
            first_is_true = (bit == true);
        }
        count++;
    }

    bool result = (count == 3) && first_is_true;

    // Clean up
    BitVecDeinit(&bv);

    DefaultAllocatorDeinit(&alloc);

    return result;
}

// Test BitVecForeachInRangeIdx macro
bool test_bitvec_foreach_in_range_idx(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecForeachInRangeIdx macro\n");

    BitVec bv = BitVecInit(ALLOCATOR_OF(&alloc));

    // Add test pattern: true, false, true, false, true
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecPush(&bv, true);

    // Test range iteration from index 1 to 4 (exclusive)
    u64  count         = 0;
    bool range_correct = true;

    BitVecForeachInRangeIdx(&bv, bit, idx, 1, 4) {
        // Should iterate over indices 1, 2, 3
        // Values: false, true, false
        if (idx == 1 || idx == 3) {
            range_correct = range_correct && (bit == false);
        } else if (idx == 2) {
            range_correct = range_correct && (bit == true);
        }
        count++;
    }

    bool result = (count == 3) && range_correct;

    // Clean up
    BitVecDeinit(&bv);

    DefaultAllocatorDeinit(&alloc);

    return result;
}

// Test BitVecForeachInRange macro
bool test_bitvec_foreach_in_range(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecForeachInRange macro\n");

    BitVec bv = BitVecInit(ALLOCATOR_OF(&alloc));

    // Add test pattern: false, true, true, false, true
    BitVecPush(&bv, false);
    BitVecPush(&bv, true);
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecPush(&bv, true);

    // Test range iteration from index 1 to 3 (exclusive)
    u64 true_count  = 0;
    u64 false_count = 0;

    BitVecForeachInRange(&bv, bit, 1, 3) {
        // Should iterate over indices 1, 2
        // Values: true, true
        if (bit) {
            true_count++;
        } else {
            false_count++;
        }
    }

    bool result = (true_count == 2) && (false_count == 0);

    // Clean up
    BitVecDeinit(&bv);

    DefaultAllocatorDeinit(&alloc);

    return result;
}

// Edge case tests
bool test_bitvec_foreach_edge_cases(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVec foreach edge cases\n");

    BitVec bv     = BitVecInit(ALLOCATOR_OF(&alloc));
    bool   result = true;
    int    count  = 0;

    // Test foreach on empty bitvec
    BitVecForeach(&bv, bit) {
        (void)bit;
        count++; // Should not execute
    }
    result = result && (count == 0);

    // Test foreach on single element
    BitVecPush(&bv, true);
    count = 0;
    BitVecForeach(&bv, bit) {
        count++;
        result = result && (bit == true);
    }
    result = result && (count == 1);

    // Test foreach on large data
    BitVecClear(&bv);
    for (int i = 0; i < 1000; i++) {
        BitVecPush(&bv, i % 2 == 0);
    }

    count = 0;
    BitVecForeach(&bv, bit) {
        (void)bit;
        count++;
    }
    result = result && (count == 1000);

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_bitvec_foreach_idx_edge_cases(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVec foreach idx edge cases\n");

    BitVec bv       = BitVecInit(ALLOCATOR_OF(&alloc));
    bool   result   = true;
    u64    last_idx = SIZE_MAX;

    // Test foreach idx on empty bitvec
    BitVecForeachIdx(&bv, bit, idx) {
        (void)bit;
        result = false; // Should not execute
    }

    // Test foreach idx on single element
    BitVecPush(&bv, false);
    BitVecForeachIdx(&bv, bit, idx) {
        result   = result && (idx == 0);
        result   = result && (bit == false);
        last_idx = idx;
    }
    result = result && (last_idx == 0);

    // Test foreach idx ordering
    BitVecClear(&bv);
    for (int i = 0; i < 10; i++) {
        BitVecPush(&bv, i % 2 == 0);
    }

    u64 expected_idx = 0;
    BitVecForeachIdx(&bv, bit, idx) {
        (void)bit;
        result = result && (idx == expected_idx);
        expected_idx++;
    }

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_bitvec_foreach_reverse_edge_cases(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVec foreach reverse edge cases\n");

    BitVec bv     = BitVecInit(ALLOCATOR_OF(&alloc));
    bool   result = true;

    // Test reverse foreach on empty bitvec
    BitVecForeachReverse(&bv, bit) {
        (void)bit;
        result = false; // Should not execute
    }

    // Test reverse foreach on single element
    BitVecPush(&bv, true);
    int count = 0;
    BitVecForeachReverse(&bv, bit) {
        count++;
        result = result && (bit == true);
    }
    result = result && (count == 1);

    // Test reverse ordering
    BitVecClear(&bv);
    BitVecPush(&bv, true);                          // idx 0
    BitVecPush(&bv, false);                         // idx 1
    BitVecPush(&bv, true);                          // idx 2

    bool expected_sequence[] = {true, false, true}; // Reverse order
    int  seq_idx             = 0;
    BitVecForeachReverse(&bv, bit) {
        result = result && (bit == expected_sequence[seq_idx]);
        seq_idx++;
    }

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_bitvec_foreach_range_edge_cases(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVec foreach range edge cases\n");

    BitVec bv     = BitVecInit(ALLOCATOR_OF(&alloc));
    bool   result = true;

    // Setup test data
    for (int i = 0; i < 10; i++) {
        BitVecPush(&bv, i % 2 == 0);
    }

    // Test range with start == end (should not execute)
    int count = 0;
    BitVecForeachInRange(&bv, bit, 5, 5) {
        (void)bit;
        count++; // Should not execute
    }
    result = result && (count == 0);

    // Test range with single element
    count = 0;
    BitVecForeachInRange(&bv, bit, 3, 4) {
        count++;
        result = result && (bit == false); // 3 % 2 != 0
    }
    result = result && (count == 1);

    // Test range at boundaries
    count = 0;
    BitVecForeachInRange(&bv, bit, 0, 2) {
        (void)bit;
        count++;
    }
    result = result && (count == 2);

    // Test range at the end of bitvector
    count = 0;
    BitVecForeachInRange(&bv, bit, 8, 10) {
        (void)bit;
        count++;
    }
    result = result && (count == 2); // Should iterate over indices 8,9

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_bitvec_foreach_stress_test(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVec foreach stress test\n");

    bool result = true;

    for (int sz = 0; sz < 100; sz += 10) {
        BitVec bv = BitVecInit(ALLOCATOR_OF(&alloc));

        // Create bitvec of varying sz
        for (int i = 0; i < sz; i++) {
            BitVecPush(&bv, i % 3 == 0);
        }

        // Test all foreach variants
        int count1 = 0, count2 = 0, count3 = 0, count4 = 0;

        BitVecForeach(&bv, bitval) {
            (void)bitval;
            count1++;
        }
        BitVecForeachIdx(&bv, bitval, i) {
            (void)bitval;
            count2++;
        }
        BitVecForeachReverse(&bv, bitval) {
            (void)bitval;
            count3++;
        }
        BitVecForeachReverseIdx(&bv, bitval, i) {
            (void)bitval;
            count4++;
        }

        result = result && (count1 == sz);
        result = result && (count2 == sz);
        result = result && (count3 == sz);
        result = result && (count4 == sz);

        BitVecDeinit(&bv);
    }

    DefaultAllocatorDeinit(&alloc);

    return result;
}

// BitVecRunLengths test implementations

bool test_bitvec_run_lengths_basic(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecRunLengths basic functionality\n");

    BitVec bv     = BitVecInit(ALLOCATOR_OF(&alloc));
    bool   result = true;

    // Test pattern: 11100101 (3 true, 2 false, 1 true, 1 false, 1 true)
    BitVecPush(&bv, true);  // 0
    BitVecPush(&bv, true);  // 1
    BitVecPush(&bv, true);  // 2
    BitVecPush(&bv, false); // 3
    BitVecPush(&bv, false); // 4
    BitVecPush(&bv, true);  // 5
    BitVecPush(&bv, false); // 6
    BitVecPush(&bv, true);  // 7

    u64  runs[10];
    bool values[10];
    u64  count = BitVecRunLengths(&bv, runs, values, 10);

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

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Vec form: same pattern as the raw test, but writes BitVecRun records
// into a Vec so we cover the new overload's full path.
bool test_bitvec_run_lengths_vec(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    WriteFmt("Testing BitVecRunLengths Vec form\n");

    BitVec bv = BitVecInit(base);
    BitVecPush(&bv, true);
    BitVecPush(&bv, true);
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecPush(&bv, false);
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecPush(&bv, true);

    BitVecRuns runs   = VecInitT(runs, base);
    bool       result = BitVecRunLengths(&bv, &runs);
    result            = result && VecLen(&runs) == 5;
    if (result) {
        result = result && VecPtrAt(&runs, 0)->length == 3 && VecPtrAt(&runs, 0)->value == true;
        result = result && VecPtrAt(&runs, 1)->length == 2 && VecPtrAt(&runs, 1)->value == false;
        result = result && VecPtrAt(&runs, 2)->length == 1 && VecPtrAt(&runs, 2)->value == true;
        result = result && VecPtrAt(&runs, 3)->length == 1 && VecPtrAt(&runs, 3)->value == false;
        result = result && VecPtrAt(&runs, 4)->length == 1 && VecPtrAt(&runs, 4)->value == true;
    }

    VecDeinit(&runs);
    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_bitvec_run_lengths_edge_cases(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecRunLengths edge cases\n");

    bool result = true;

    // Test 1: Empty bitvector
    BitVec empty_bv = BitVecInit(ALLOCATOR_OF(&alloc));
    u64    runs[5];
    bool   values[5];
    u64    count = BitVecRunLengths(&empty_bv, runs, values, 5);
    result       = result && (count == 0);
    BitVecDeinit(&empty_bv);

    // Test 2: Single bit (true)
    BitVec single_bv = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVecPush(&single_bv, true);
    count  = BitVecRunLengths(&single_bv, runs, values, 5);
    result = result && (count == 1);
    result = result && (runs[0] == 1 && values[0] == true);
    BitVecDeinit(&single_bv);

    // Test 3: Single bit (false)
    BitVec single_false_bv = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVecPush(&single_false_bv, false);
    count  = BitVecRunLengths(&single_false_bv, runs, values, 5);
    result = result && (count == 1);
    result = result && (runs[0] == 1 && values[0] == false);
    BitVecDeinit(&single_false_bv);

    // Test 4: All same bits (all true)
    BitVec all_true_bv = BitVecInit(ALLOCATOR_OF(&alloc));
    for (int i = 0; i < 10; i++) {
        BitVecPush(&all_true_bv, true);
    }
    count  = BitVecRunLengths(&all_true_bv, runs, values, 5);
    result = result && (count == 1);
    result = result && (runs[0] == 10 && values[0] == true);
    BitVecDeinit(&all_true_bv);

    // Test 5: Alternating bits (0101010)
    BitVec alternating_bv = BitVecInit(ALLOCATOR_OF(&alloc));
    for (int i = 0; i < 7; i++) {
        BitVecPush(&alternating_bv, i % 2 == 0);
    }
    u64  alt_runs[10];
    bool alt_values[10];
    count  = BitVecRunLengths(&alternating_bv, alt_runs, alt_values, 10);
    result = result && (count == 7); // Each bit is its own run
    // Verify alternating pattern
    for (u64 i = 0; i < count && i < 7; i++) {
        result = result && (alt_runs[i] == 1);
        result = result && (alt_values[i] == (i % 2 == 0));
    }
    BitVecDeinit(&alternating_bv);

    DefaultAllocatorDeinit(&alloc);

    return result;
}

bool test_bitvec_run_lengths_boundary_conditions(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecRunLengths boundary conditions\n");

    BitVec bv     = BitVecInit(ALLOCATOR_OF(&alloc));
    bool   result = true;

    // Create pattern with many runs: 10101010 (8 runs)
    for (int i = 0; i < 8; i++) {
        BitVecPush(&bv, i % 2 == 0);
    }

    // Test with limited max_runs
    u64  runs[3];
    bool values[3];
    u64  count = BitVecRunLengths(&bv, runs, values, 3);

    // Should only capture first 3 runs due to limit
    result = result && (count == 3);
    result = result && (runs[0] == 1 && values[0] == true);  // First run
    result = result && (runs[1] == 1 && values[1] == false); // Second run
    result = result && (runs[2] == 1 && values[2] == true);  // Third run

    // Test with max_runs = 1
    count  = BitVecRunLengths(&bv, runs, values, 1);
    result = result && (count == 1);
    result = result && (runs[0] == 1 && values[0] == true);

    // Test with exact number of runs needed
    u64  large_runs[8];
    bool large_values[8];
    count  = BitVecRunLengths(&bv, large_runs, large_values, 8);
    result = result && (count == 8);

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_bitvec_run_lengths_stress_test(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecRunLengths stress test\n");

    bool result = true;

    // Test with large bitvector
    BitVec large_bv = BitVecInit(ALLOCATOR_OF(&alloc));

    // Create pattern that results in many runs
    // Pattern: blocks of 5 same bits, alternating true/false
    for (int block = 0; block < 100; block++) {
        bool value = (block % 2 == 0);
        for (int i = 0; i < 5; i++) {
            BitVecPush(&large_bv, value);
        }
    }

    u64  runs[200];
    bool values[200];
    u64  count = BitVecRunLengths(&large_bv, runs, values, 200);

    // Should have 100 runs (100 blocks)
    result = result && (count == 100);

    // Verify each run has length 5 and alternating values
    for (u64 i = 0; i < count && i < 100; i++) {
        result = result && (runs[i] == 5);
        result = result && (values[i] == (i % 2 == 0));
    }

    // Test with complex random-like pattern
    BitVecClear(&large_bv);

    // Pattern: 1111000011100010000111 (variable length runs)
    // Run 1: 4 trues, Run 2: 4 falses, Run 3: 3 trues, Run 4: 3 falses, Run 5: 1 true, Run 6: 4 falses, Run 7: 3 trues
    bool pattern[]   = {1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 1};
    int  pattern_len = sizeof(pattern) / sizeof(pattern[0]);

    for (int i = 0; i < pattern_len; i++) {
        BitVecPush(&large_bv, pattern[i]);
    }

    u64  small_runs[10];
    bool small_values[10];
    count = BitVecRunLengths(&large_bv, small_runs, small_values, 10);

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

    BitVecDeinit(&large_bv);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Main function that runs all simple tests
int main(void) {
    WriteFmt("[INFO] Starting BitVec.Foreach.Simple tests\n\n");

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
        test_bitvec_foreach_stress_test,
        test_bitvec_run_lengths_basic,
        test_bitvec_run_lengths_vec,
        test_bitvec_run_lengths_edge_cases,
        test_bitvec_run_lengths_boundary_conditions,
        test_bitvec_run_lengths_stress_test
    };

    int total_tests = sizeof(tests) / sizeof(tests[0]);

    // Run simple tests using the centralized test driver
    return run_test_suite(tests, total_tests, NULL, 0, "BitVec.Foreach.Simple");
}
