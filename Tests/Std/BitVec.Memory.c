#include <Misra/Std/Container/BitVec.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Log.h>
#include <Misra/Types.h>

// Include test utilities
#include "../Util/TestRunner.h"

// Function prototypes
bool test_bitvec_shrink_to_fit(void);
bool test_bitvec_reserve(void);
bool test_bitvec_swap(void);
bool test_bitvec_clone(void);
bool test_bitvec_clone_inherits_allocator_config(void);
bool test_bitvec_shrink_to_fit_edge_cases(void);
bool test_bitvec_reserve_edge_cases(void);
bool test_bitvec_swap_edge_cases(void);
bool test_bitvec_clone_edge_cases(void);
bool test_bitvec_memory_stress_test(void);
bool test_bitvec_memory_null_failures(void);
bool test_bitvec_swap_null_failures(void);
bool test_bitvec_clone_null_failures(void);

// Test BitVecShrinkToFit function
bool test_bitvec_shrink_to_fit(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecShrinkToFit\n");

    BitVec bv = BitVecInit(ALLOCATOR_OF(&alloc));

    // Add some bits
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecPush(&bv, true);

    // Force capacity to be larger than needed by reserving space
    BitVecReserve(&bv, 100);

    // Check that capacity is larger than length
    u64  initial_capacity = BitVecCapacity(&bv);
    bool result           = (initial_capacity >= 100) && (BitVecLen(&bv) == 3);

    // Shrink to fit
    BitVecShrinkToFit(&bv);

    // Check that capacity is now closer to length
    result = result && (BitVecCapacity(&bv) < initial_capacity);
    result = result && (BitVecCapacity(&bv) >= BitVecLen(&bv));

    // Check that data is still intact
    result = result && (BitVecLen(&bv) == 3);
    result = result && (BitVecGet(&bv, 0) == true);
    result = result && (BitVecGet(&bv, 1) == false);
    result = result && (BitVecGet(&bv, 2) == true);

    // Clean up
    BitVecDeinit(&bv);

    DefaultAllocatorDeinit(&alloc);

    return result;
}

bool test_bitvec_reserve(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecReserve\n");

    BitVec bv = BitVecInit(ALLOCATOR_OF(&alloc));

    // Add some bits
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);

    // Set capacity to a specific value
    BitVecReserve(&bv, 50);

    // Check that capacity was set correctly
    bool result = (BitVecCapacity(&bv) >= 50) && (BitVecLen(&bv) == 2);

    // Check that data is still intact
    result = result && (BitVecGet(&bv, 0) == true);
    result = result && (BitVecGet(&bv, 1) == false);

    // Try to set capacity smaller than current length (should not shrink below length)
    BitVecReserve(&bv, 1);

    // Capacity should still accommodate at least the current length
    result = result && (BitVecCapacity(&bv) >= BitVecLen(&bv));
    result = result && (BitVecLen(&bv) == 2);

    // Data should still be intact
    result = result && (BitVecGet(&bv, 0) == true);
    result = result && (BitVecGet(&bv, 1) == false);

    // Clean up
    BitVecDeinit(&bv);

    DefaultAllocatorDeinit(&alloc);

    return result;
}

// Test BitVecSwap function
bool test_bitvec_swap(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecSwap\n");

    BitVec bv1 = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec bv2 = BitVecInit(ALLOCATOR_OF(&alloc));

    // Set up first bitvector
    BitVecPush(&bv1, true);
    BitVecPush(&bv1, false);
    BitVecPush(&bv1, true);

    // Set up second bitvector
    BitVecPush(&bv2, false);
    BitVecPush(&bv2, false);

    // Store original states
    u64 bv1_orig_length = BitVecLen(&bv1);
    u64 bv2_orig_length = BitVecLen(&bv2);

    // Swap the bitvectors
    BitVecSwap(&bv1, &bv2);

    // Check that they swapped
    bool result = (BitVecLen(&bv1) == bv2_orig_length) && (BitVecLen(&bv2) == bv1_orig_length);

    // Check bv1 (should now have bv2's original content)
    result = result && (BitVecLen(&bv1) == 2);
    result = result && (BitVecGet(&bv1, 0) == false);
    result = result && (BitVecGet(&bv1, 1) == false);

    // Check bv2 (should now have bv1's original content)
    result = result && (BitVecLen(&bv2) == 3);
    result = result && (BitVecGet(&bv2, 0) == true);
    result = result && (BitVecGet(&bv2, 1) == false);
    result = result && (BitVecGet(&bv2, 2) == true);

    // Clean up
    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);

    DefaultAllocatorDeinit(&alloc);

    return result;
}

// Test BitVecClone function
bool test_bitvec_clone(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecClone\n");

    BitVec original = BitVecInit(ALLOCATOR_OF(&alloc));

    // Set up original bitvector
    BitVecPush(&original, true);
    BitVecPush(&original, false);
    BitVecPush(&original, true);
    BitVecPush(&original, false);

    // Clone the bitvector
    BitVec clone = BitVecClone(&original);

    // Check that clone has same content as original
    bool result = (BitVecLen(&clone) == BitVecLen(&original));

    for (u64 i = 0; i < BitVecLen(&original); i++) {
        result = result && (BitVecGet(&clone, i) == BitVecGet(&original, i));
    }

    // Verify they are independent by modifying original
    BitVecPush(&original, true);

    // Clone should remain unchanged
    result = result && (BitVecLen(&clone) == 4) && (BitVecLen(&original) == 5);
    result = result && (BitVecGet(&clone, 0) == true);
    result = result && (BitVecGet(&clone, 1) == false);
    result = result && (BitVecGet(&clone, 2) == true);
    result = result && (BitVecGet(&clone, 3) == false);

    // Modify clone to verify independence
    BitVecSet(&clone, 0, false);

    // Original should remain unchanged at index 0
    result = result && (BitVecGet(&original, 0) == true);
    result = result && (BitVecGet(&clone, 0) == false);

    // Clean up
    BitVecDeinit(&original);
    BitVecDeinit(&clone);

    DefaultAllocatorDeinit(&alloc);

    return result;
}

bool test_bitvec_clone_inherits_allocator_config(void) {
    WriteFmt("Testing BitVecClone allocator inheritance\n");

    DefaultAllocator alloc = DefaultAllocatorInit();
    // intentional bypass: no public setter on `Allocator` for effort /
    // retry_limit -- pre-seeded directly so the inheritance path below
    // can be observed end-to-end.
    alloc.base.effort      = ALLOCATOR_EFFORT_RETRY_FALLBACK;
    alloc.base.retry_limit = 9;

    BitVec original = BitVecInit(ALLOCATOR_OF(&alloc));

    BitVecPush(&original, true);
    BitVecPush(&original, false);
    BitVecPush(&original, true);

    BitVec clone = BitVecClone(&original);

    // Clone should share the same Allocator* and therefore see identical
    // configuration fields on the base allocator.
    bool result = BitVecLen(&clone) == BitVecLen(&original) && BitVecCapacity(&clone) >= BitVecLen(&original) &&
                  BitVecAllocator(&clone) == BitVecAllocator(&original) &&
                  BitVecAllocator(&clone)->allocate == BitVecAllocator(&original)->allocate &&
                  BitVecAllocator(&clone)->remap == BitVecAllocator(&original)->remap &&
                  BitVecAllocator(&clone)->deallocate == BitVecAllocator(&original)->deallocate &&
                  BitVecAllocator(&clone)->effort == BitVecAllocator(&original)->effort &&
                  BitVecAllocator(&clone)->retry_limit == BitVecAllocator(&original)->retry_limit &&
                  BitVecGet(&clone, 0) == true && BitVecGet(&clone, 1) == false && BitVecGet(&clone, 2) == true;

    BitVecDeinit(&original);
    BitVecDeinit(&clone);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Edge case tests
bool test_bitvec_shrink_to_fit_edge_cases(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecShrinkToFit edge cases\n");

    BitVec bv     = BitVecInit(ALLOCATOR_OF(&alloc));
    bool   result = true;

    // Test shrink on empty bitvec
    BitVecShrinkToFit(&bv);
    result = result && (BitVecLen(&bv) == 0) && (BitVecCapacity(&bv) >= 0);

    // Test shrink on single element
    BitVecPush(&bv, true);
    BitVecShrinkToFit(&bv);
    result = result && (BitVecLen(&bv) == 1) && (BitVecCapacity(&bv) >= 1);
    result = result && (BitVecGet(&bv, 0) == true);

    // Test multiple shrinks (should be safe)
    BitVecShrinkToFit(&bv);
    BitVecShrinkToFit(&bv);
    result = result && (BitVecLen(&bv) == 1);

    // Test shrink after reserve and clear
    BitVecReserve(&bv, 1000);
    BitVecClear(&bv);
    BitVecShrinkToFit(&bv);
    result = result && (BitVecLen(&bv) == 0);

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_bitvec_reserve_edge_cases(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecReserve edge cases\n");

    BitVec bv     = BitVecInit(ALLOCATOR_OF(&alloc));
    bool   result = true;

    // Test set capacity on empty bitvec
    BitVecReserve(&bv, 100);
    result = result && (BitVecCapacity(&bv) >= 100) && (BitVecLen(&bv) == 0);

    // BitVecReserve is grow-only; use BitVecClear to drop length to 0.
    BitVecClear(&bv);
    result = result && (BitVecLen(&bv) == 0);

    for (int i = 0; i < 10; i++) {
        BitVecPush(&bv, i % 2 == 0);
    }
    u64 original_length = BitVecLen(&bv);

    result = result && (BitVecLen(&bv) == original_length);
    for (u64 i = 0; i < BitVecLen(&bv); i++) {
        result = result && (BitVecGet(&bv, i) == (i % 2 == 0));
    }

    // Test setting very large capacity
    BitVecReserve(&bv, 10000);
    result = result && (BitVecCapacity(&bv) >= 10000);

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_bitvec_swap_edge_cases(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecSwap edge cases\n");

    BitVec bv1    = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec bv2    = BitVecInit(ALLOCATOR_OF(&alloc));
    bool   result = true;

    // Test swap with both empty
    BitVecSwap(&bv1, &bv2);
    result = result && (BitVecLen(&bv1) == 0) && (BitVecLen(&bv2) == 0);

    // Test swap with one empty, one non-empty
    BitVecPush(&bv1, true);
    BitVecPush(&bv1, false);
    BitVecSwap(&bv1, &bv2);

    result = result && (BitVecLen(&bv1) == 0);
    result = result && (BitVecLen(&bv2) == 2);
    result = result && (BitVecGet(&bv2, 0) == true);
    result = result && (BitVecGet(&bv2, 1) == false);

    // Test swap with large data
    BitVecClear(&bv1);
    for (int i = 0; i < 1000; i++) {
        BitVecPush(&bv1, i % 3 == 0);
    }

    BitVecSwap(&bv1, &bv2);
    result = result && (BitVecLen(&bv1) == 2) && (BitVecLen(&bv2) == 1000);
    result = result && (BitVecGet(&bv2, 0) == true); // 0 % 3 == 0
    result = result && (BitVecGet(&bv2, 999) == (999 % 3 == 0));

    // Test swapping with itself (should be safe)
    BitVecSwap(&bv1, &bv1);
    result = result && (BitVecLen(&bv1) == 2);

    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_bitvec_clone_edge_cases(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecClone edge cases\n");

    BitVec bv     = BitVecInit(ALLOCATOR_OF(&alloc));
    bool   result = true;

    // Test clone empty bitvec
    BitVec clone1 = BitVecClone(&bv);
    result        = result && (BitVecLen(&clone1) == 0);
    BitVecDeinit(&clone1);

    // Test clone single element
    BitVecPush(&bv, true);
    BitVec clone2 = BitVecClone(&bv);
    result        = result && (BitVecLen(&clone2) == 1);
    result        = result && (BitVecGet(&clone2, 0) == true);
    BitVecDeinit(&clone2);

    // Test clone large data
    BitVecClear(&bv);
    for (int i = 0; i < 1000; i++) {
        BitVecPush(&bv, i % 2 == 0);
    }

    BitVec clone3 = BitVecClone(&bv);
    result        = result && (BitVecLen(&clone3) == 1000);

    // Verify all bits match
    for (u64 i = 0; i < 1000; i++) {
        result = result && (BitVecGet(&clone3, i) == BitVecGet(&bv, i));
    }

    // Test independence - modify original
    BitVecSet(&bv, 0, !BitVecGet(&bv, 0));
    result = result && (BitVecGet(&clone3, 0) != BitVecGet(&bv, 0));

    BitVecDeinit(&clone3);
    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_bitvec_memory_stress_test(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVec memory stress test\n");

    bool result = true;

    for (int cycle = 0; cycle < 10; cycle++) {
        BitVec bv1 = BitVecInit(ALLOCATOR_OF(&alloc));
        BitVec bv2 = BitVecInit(ALLOCATOR_OF(&alloc));

        // Add random-sized data
        for (int i = 0; i < cycle * 10; i++) {
            BitVecPush(&bv1, i % 2 == 0);
            BitVecPush(&bv2, i % 3 == 0);
        }

        // Clone and swap
        BitVec clone = BitVecClone(&bv1);
        BitVecSwap(&bv1, &bv2);

        BitVecReserve(&bv1, cycle * 20);
        BitVecShrinkToFit(&bv2);

        // Verify data integrity
        result = result && (BitVecLen(&clone) == cycle * 10);
        if (cycle > 0) {
            result = result && (BitVecGet(&clone, 0) == true); // 0 % 2 == 0
        }

        BitVecDeinit(&bv1);
        BitVecDeinit(&bv2);
        BitVecDeinit(&clone);
    }

    DefaultAllocatorDeinit(&alloc);

    return result;
}

// Deadend tests
bool test_bitvec_memory_null_failures(void) {
    WriteFmt("Testing BitVec memory NULL pointer handling\n");

    // Test NULL bitvec pointer - should abort
    BitVecShrinkToFit(NULL);

    return false;
}

bool test_bitvec_swap_null_failures(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVec swap NULL handling\n");

    BitVec bv = BitVecInit(ALLOCATOR_OF(&alloc));

    // Test NULL pointer - should abort
    BitVecSwap(NULL, &bv);

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

bool test_bitvec_clone_null_failures(void) {
    WriteFmt("Testing BitVec clone NULL handling\n");

    // Test NULL pointer - should abort
    BitVecClone(NULL);

    return false;
}

// Main function that runs all tests
int main(void) {
    WriteFmt("[INFO] Starting BitVec.Memory tests\n\n");

    // Array of normal test functions
    TestFunction tests[] = {
        test_bitvec_shrink_to_fit,
        test_bitvec_reserve,
        test_bitvec_swap,
        test_bitvec_clone,
        test_bitvec_clone_inherits_allocator_config,
        test_bitvec_shrink_to_fit_edge_cases,
        test_bitvec_reserve_edge_cases,
        test_bitvec_swap_edge_cases,
        test_bitvec_clone_edge_cases,
        test_bitvec_memory_stress_test
    };

    // Array of deadend test functions
    TestFunction deadend_tests[] = {
        test_bitvec_memory_null_failures,
        test_bitvec_swap_null_failures,
        test_bitvec_clone_null_failures
    };

    int total_tests         = sizeof(tests) / sizeof(tests[0]);
    int total_deadend_tests = sizeof(deadend_tests) / sizeof(deadend_tests[0]);

    // Run all tests using the centralized test driver
    return run_test_suite(tests, total_tests, deadend_tests, total_deadend_tests, "BitVec.Memory");
}
