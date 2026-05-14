#include <Misra/Std/Container/BitVec.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Log.h>

#include <stdio.h>
#include <Misra/Types.h> // For size and other type definitions

// Include test utilities
#include "../Util/TestRunner.h"

// Function prototypes for deadend tests
bool test_bitvec_find_deadend_tests(void);
bool test_bitvec_predicate_deadend_tests(void);
bool test_bitvec_longest_run_deadend_tests(void);
bool test_bitvec_access_null_failures(void);
bool test_bitvec_set_null_failures(void);
bool test_bitvec_flip_null_failures(void);
bool test_bitvec_get_bounds_failures(void);
bool test_bitvec_set_bounds_failures(void);
bool test_bitvec_flip_bounds_failures(void);
bool test_bitvec_get_large_index_failures(void);
bool test_bitvec_set_large_index_failures(void);
bool test_bitvec_flip_edge_index_failures(void);
bool test_bitvec_count_null_failures(void);
bool test_bitvec_get_max_index_failures(void);

// Deadend tests - testing NULL pointers and invalid conditions that should cause fatal errors
bool test_bitvec_find_deadend_tests(void) {
    WriteFmt("Testing BitVecFind deadend scenarios\n");

    // This should cause LOG_FATAL and terminate the program
    BitVecFind(NULL, true);

    return true; // Should never reach here
}

bool test_bitvec_predicate_deadend_tests(void) {
    WriteFmt("Testing BitVec predicate deadend scenarios\n");

    // This should cause LOG_FATAL and terminate the program
    BitVecAll(NULL, true);

    return true; // Should never reach here
}

bool test_bitvec_longest_run_deadend_tests(void) {
    WriteFmt("Testing BitVecLongestRun deadend scenarios\n");

    // This should cause LOG_FATAL and terminate the program
    BitVecLongestRun(NULL, true);

    return true; // Should never reach here
}

// Deadend tests
bool test_bitvec_access_null_failures(void) {
    WriteFmt("Testing BitVec access NULL pointer handling\n");

    // Test NULL bitvec pointer - should abort
    BitVecGet(NULL, 0);

    return false;
}

bool test_bitvec_set_null_failures(void) {
    WriteFmt("Testing BitVec set NULL pointer handling\n");

    // Test NULL bitvec pointer - should abort
    BitVecSet(NULL, 0, true);

    return false;
}

bool test_bitvec_flip_null_failures(void) {
    WriteFmt("Testing BitVec flip NULL pointer handling\n");

    // Test NULL bitvec pointer - should abort
    BitVecFlip(NULL, 0);

    return false;
}

bool test_bitvec_get_bounds_failures(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVec get bounds checking\n");

    BitVec bv = BitVecInit(ALLOCATOR_OF(&alloc));

    // Test get from empty bitvec - should abort
    BitVecGet(&bv, 0);

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

bool test_bitvec_set_bounds_failures(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVec set bounds checking\n");

    BitVec bv = BitVecInit(ALLOCATOR_OF(&alloc));

    // Test set on empty bitvec - should abort
    BitVecSet(&bv, 0, true);

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

bool test_bitvec_flip_bounds_failures(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVec flip bounds checking\n");

    BitVec bv = BitVecInit(ALLOCATOR_OF(&alloc));

    // Test flip on empty bitvec - should abort
    BitVecFlip(&bv, 0);

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// NEW: More specific bounds checking deadend tests
bool test_bitvec_get_large_index_failures(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVec get with large out-of-bounds index\n");

    BitVec bv = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecPush(&bv, true);

    // Test with index way beyond length (3) - should abort
    BitVecGet(&bv, 1000);

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

bool test_bitvec_set_large_index_failures(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVec set with large out-of-bounds index\n");

    BitVec bv = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);

    // Test with index way beyond length (2) - should abort
    BitVecSet(&bv, 500, true);

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

bool test_bitvec_flip_edge_index_failures(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVec flip with edge case out-of-bounds index\n");

    BitVec bv = BitVecInit(ALLOCATOR_OF(&alloc));
    for (int i = 0; i < 10; i++) {
        BitVecPush(&bv, i % 2 == 0);
    }

    // Test with index exactly at length (invalid) - should abort
    BitVecFlip(&bv, 10);

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

bool test_bitvec_count_null_failures(void) {
    WriteFmt("Testing BitVec count operations with NULL pointer\n");

    // Test NULL bitvec pointer - should abort
    BitVecCountOnes(NULL);

    return false;
}

bool test_bitvec_get_max_index_failures(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVec get with maximum index value\n");

    BitVec bv = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVecPush(&bv, true);

    // Test with maximum possible index value - should abort
    BitVecGet(&bv, SIZE_MAX);

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// Main function that runs all deadend tests
int main(void) {
    WriteFmt("[INFO] Starting BitVec.Access.Deadend tests\n\n");

    // Deadend tests that would cause program termination
    TestFunction deadend_tests[] = {
        test_bitvec_find_deadend_tests,
        test_bitvec_predicate_deadend_tests,
        test_bitvec_longest_run_deadend_tests,
        test_bitvec_access_null_failures,
        test_bitvec_set_null_failures,
        test_bitvec_flip_null_failures,
        test_bitvec_get_bounds_failures,
        test_bitvec_set_bounds_failures,
        test_bitvec_flip_bounds_failures,
        test_bitvec_get_large_index_failures,
        test_bitvec_set_large_index_failures,
        test_bitvec_flip_edge_index_failures,
        test_bitvec_count_null_failures,
        test_bitvec_get_max_index_failures
    };

    int total_deadend_tests = sizeof(deadend_tests) / sizeof(deadend_tests[0]);

    // Run all deadend tests using the centralized test driver
    return run_test_suite(NULL, 0, deadend_tests, total_deadend_tests, "BitVec.Access.Deadend");
}
