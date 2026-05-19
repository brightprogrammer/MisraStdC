#include <Misra/Std/Container/BitVec.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Log.h>

#include <Misra/Types.h>

// Include test utilities
#include "../Util/TestRunner.h"

// Function prototypes for deadend tests
bool test_bitvec_foreach_invalid_usage(void);
bool test_bitvec_run_lengths_null_bv(void);
bool test_bitvec_run_lengths_null_runs(void);
bool test_bitvec_run_lengths_null_values(void);
bool test_bitvec_run_lengths_zero_max_runs(void);

// Deadend tests for BitVecRunLengths

bool test_bitvec_run_lengths_null_bv(void) {
    WriteFmt("Testing BitVecRunLengths with NULL bitvector\n");

    u64  runs[5];
    bool values[5];

    // This should cause LOG_FATAL and terminate the program
    BitVecRunLengths(NULL, runs, values, 5);

    return true; // Should never reach here
}

bool test_bitvec_run_lengths_null_runs(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecRunLengths with NULL runs array\n");

    BitVec bv = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVecPush(&bv, true);
    bool values[5];

    // This should cause LOG_FATAL and terminate the program
    BitVecRunLengths(&bv, NULL, values, 5);

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return true; // Should never reach here
}

bool test_bitvec_run_lengths_null_values(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecRunLengths with NULL values array\n");

    BitVec bv = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVecPush(&bv, true);
    u64 runs[5];

    // This should cause LOG_FATAL and terminate the program
    BitVecRunLengths(&bv, runs, NULL, 5);

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return true; // Should never reach here
}

bool test_bitvec_run_lengths_zero_max_runs(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecRunLengths with zero max_runs\n");

    BitVec bv = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVecPush(&bv, true);
    u64  runs[5];
    bool values[5];

    // This should cause LOG_FATAL and terminate the program
    BitVecRunLengths(&bv, runs, values, 0);

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return true; // Should never reach here
}

// Deadend tests - Note: foreach macros don't have traditional NULL checks
// since they operate on the bitvec structure directly, but we can test
// with invalid macro usage scenarios

bool test_bitvec_foreach_invalid_usage(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVec foreach with invalid bitvec\n");

    // Test foreach with invalid bitvec (length > 0 but data is NULL)
    BitVec bv   = BitVecInit(ALLOCATOR_OF(&alloc));
    bv.length   = 5;
    bv.capacity = 10;

    // This should abort due to ValidateBitVec check
    int count = 0;
    BitVecForeach(&bv, bit) {
        (void)bit;
        count++;
    }

    // Should not reach here
    (void)count; // Silence unused variable warning
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// Main function that runs all deadend tests
int main(void) {
    WriteFmt("[INFO] Starting BitVec.Foreach.Deadend tests\n\n");

    // Array of deadend test functions
    TestFunction deadend_tests[] = {
        test_bitvec_foreach_invalid_usage,
        test_bitvec_run_lengths_null_bv,
        test_bitvec_run_lengths_null_runs,
        test_bitvec_run_lengths_null_values,
        test_bitvec_run_lengths_zero_max_runs
    };

    int total_deadend_tests = sizeof(deadend_tests) / sizeof(deadend_tests[0]);

    DefaultAllocator alloc = DefaultAllocatorInit();
    typedef List(int) LI;
    LI li = ListInit(ALLOCATOR_OF(&alloc));
    ListForeach(&li, i) {
        (void)i;
    }
    ListDeinit(&li);
    DefaultAllocatorDeinit(&alloc);

    // Run all deadend tests using the centralized test driver
    return run_test_suite(NULL, 0, deadend_tests, total_deadend_tests, "BitVec.Foreach.Deadend");
}
