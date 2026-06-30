#include <Misra/Std/Container/BitVec.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Log.h>
#include <Misra/Types.h>

// Include test utilities
#include "../../Util/TestRunner.h"

// Function prototypes
bool test_bitvec_init(void);
bool test_bitvec_deinit(void);
bool test_bitvec_reserve(void);
bool test_bitvec_clear(void);
bool test_bitvec_resize(void);
bool test_bitvec_init_edge_cases(void);
bool test_bitvec_deinit_edge_cases(void);
bool test_bitvec_reserve_edge_cases(void);
bool test_bitvec_clear_edge_cases(void);
bool test_bitvec_resize_edge_cases(void);
bool test_bitvec_multiple_cycles(void);

// Test BitVecInit function
bool test_bitvec_init(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecInit\n");

    // Test basic initialization
    BitVec bv = BitVecInit(ALLOCATOR_OF(&alloc));

    // Check initial state
    bool result = (BitVecLen(&bv) == 0);
    result      = result && (BitVecCapacity(&bv) == 0);
    result      = result && (BitVecData(&bv) == NULL);
    result      = result && (BitVecByteSize(&bv) == 0);

    // Clean up
    BitVecDeinit(&bv);

    DefaultAllocatorDeinit(&alloc);

    return result;
}

// Test BitVecDeinit function
bool test_bitvec_deinit(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecDeinit\n");

    BitVec bv = BitVecInit(ALLOCATOR_OF(&alloc));

    // Add some data to make sure deinitialization works with allocated memory
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecPush(&bv, true);

    // Check that data was allocated
    bool result = (BitVecLen(&bv) == 3) && (BitVecData(&bv) != NULL);

    // Deinitialize
    BitVecDeinit(&bv);

    // After deinitialization, the bitvector should be in a safe state
    // Note: We can't easily test that memory was freed without causing issues,
    // but we can check that the structure is reset to safe values
    result = result && (BitVecLen(&bv) == 0);
    result = result && (BitVecCapacity(&bv) == 0);
    result = result && (BitVecData(&bv) == NULL);
    result = result && (BitVecByteSize(&bv) == 0);

    DefaultAllocatorDeinit(&alloc);

    return result;
}

// Test BitVecReserve function
bool test_bitvec_reserve(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecReserve\n");

    BitVec bv = BitVecInit(ALLOCATOR_OF(&alloc));

    // Reserve space for 50 bits
    BitVecReserve(&bv, 50);

    // Check that capacity was increased
    bool result = (BitVecCapacity(&bv) >= 50);
    result      = result && (BitVecLen(&bv) == 0);     // Length should still be 0
    result      = result && (BitVecData(&bv) != NULL); // Memory should be allocated

    // Add some bits to make sure the reserved space works
    for (int i = 0; i < 10; i++) {
        BitVecPush(&bv, (i % 2 == 0));
    }

    result = result && (BitVecLen(&bv) == 10);
    result = result && (BitVecCapacity(&bv) >= 50); // Should still have the reserved capacity

    // Test reserving less than current capacity (should be no-op)
    u64 original_capacity = BitVecCapacity(&bv);
    BitVecReserve(&bv, 25);
    result = result && (BitVecCapacity(&bv) == original_capacity);

    // Clean up
    BitVecDeinit(&bv);

    DefaultAllocatorDeinit(&alloc);

    return result;
}

// Test BitVecClear function
bool test_bitvec_clear(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecClear\n");

    BitVec bv = BitVecInit(ALLOCATOR_OF(&alloc));

    // Add some data
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);

    // Check initial state
    bool result            = (BitVecLen(&bv) == 4) && (BitVecData(&bv) != NULL);
    u64  original_capacity = BitVecCapacity(&bv);

    // Clear the bitvector
    BitVecClear(&bv);

    // Check that length is 0 but capacity and memory allocation remain
    result = result && (BitVecLen(&bv) == 0);
    result = result && (BitVecCapacity(&bv) == original_capacity);
    result = result && (BitVecData(&bv) != NULL); // Memory should still be allocated

    // Test that we can still add data after clearing
    BitVecPush(&bv, true);
    result = result && (BitVecLen(&bv) == 1);
    result = result && (BitVecGet(&bv, 0) == true);

    // Clean up
    BitVecDeinit(&bv);

    DefaultAllocatorDeinit(&alloc);

    return result;
}

// Test BitVecReu64 function
bool test_bitvec_resize(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecResize\n");

    BitVec bv = BitVecInit(ALLOCATOR_OF(&alloc));

    // Add some initial data
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecPush(&bv, true);

    // Test resizing to larger size
    BitVecResize(&bv, 6);

    // Check that length was increased and new bits have the default value
    bool result = (BitVecLen(&bv) == 6);
    result      = result && (BitVecGet(&bv, 0) == true);  // Original data
    result      = result && (BitVecGet(&bv, 1) == false); // Original data
    result      = result && (BitVecGet(&bv, 2) == true);  // Original data
    result      = result && (BitVecGet(&bv, 3) == false); // New data (likely default to false)
    result      = result && (BitVecGet(&bv, 4) == false); // New data (likely default to false)
    result      = result && (BitVecGet(&bv, 5) == false); // New data (likely default to false)

    // Test resizing to smaller size
    BitVecResize(&bv, 2);

    // Check that length was decreased and data was truncated
    result = result && (BitVecLen(&bv) == 2);
    result = result && (BitVecGet(&bv, 0) == true);  // Original data preserved
    result = result && (BitVecGet(&bv, 1) == false); // Original data preserved

    // Test resizing to same size (should be no-op)
    BitVecResize(&bv, 2);
    result = result && (BitVecLen(&bv) == 2);

    // Clean up
    BitVecDeinit(&bv);

    DefaultAllocatorDeinit(&alloc);

    return result;
}

// Edge case tests - boundary conditions and unusual but valid inputs
bool test_bitvec_init_edge_cases(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecInit edge cases\n");

    // Test multiple initializations
    BitVec bv1 = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec bv2 = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec bv3 = BitVecInit(ALLOCATOR_OF(&alloc));

    bool result = (BitVecLen(&bv1) == 0) && (BitVecLen(&bv2) == 0) && (BitVecLen(&bv3) == 0);
    result      = result && (BitVecData(&bv1) == NULL) && (BitVecData(&bv2) == NULL) && (BitVecData(&bv3) == NULL);

    // Clean up all
    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);
    BitVecDeinit(&bv3);

    DefaultAllocatorDeinit(&alloc);

    return result;
}

bool test_bitvec_reserve_edge_cases(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecReserve edge cases\n");

    BitVec bv     = BitVecInit(ALLOCATOR_OF(&alloc));
    bool   result = true;

    // Test reserving 0 (should be safe no-op)
    BitVecReserve(&bv, 0);
    result = result && (BitVecCapacity(&bv) == 0) && (BitVecData(&bv) == NULL);

    // Test reserving 1 bit (minimum meaningful size)
    BitVecReserve(&bv, 1);
    result = result && (BitVecCapacity(&bv) >= 1) && (BitVecData(&bv) != NULL);

    // Test very large but reasonable reservation
    BitVecReserve(&bv, 10000);
    result = result && (BitVecCapacity(&bv) >= 10000);

    // Test reserving same u64 repeatedly (should be no-op)
    u64 cap_before = BitVecCapacity(&bv);
    BitVecReserve(&bv, BitVecCapacity(&bv));
    BitVecReserve(&bv, BitVecCapacity(&bv));
    result = result && (BitVecCapacity(&bv) == cap_before);

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_bitvec_resize_edge_cases(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecReu64 edge cases\n");

    BitVec bv     = BitVecInit(ALLOCATOR_OF(&alloc));
    bool   result = true;

    // Test reu64 to 0 (should clear but keep memory)
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecResize(&bv, 0);
    result = result && (BitVecLen(&bv) == 0);

    // Test reu64 from 0 to non-zero
    BitVecResize(&bv, 5);
    result = result && (BitVecLen(&bv) == 5);
    // New bits should be false
    for (u64 i = 0; i < 5; i++) {
        result = result && (BitVecGet(&bv, i) == false);
    }

    // Test reu64 to same size
    BitVecResize(&bv, 5);
    result = result && (BitVecLen(&bv) == 5);

    // Test large resize
    BitVecResize(&bv, 1000);
    result = result && (BitVecLen(&bv) == 1000);

    // Test shrinking from large size
    BitVecResize(&bv, 10);
    result = result && (BitVecLen(&bv) == 10);

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_bitvec_clear_edge_cases(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecClear edge cases\n");

    BitVec bv     = BitVecInit(ALLOCATOR_OF(&alloc));
    bool   result = true;

    // Test clear on empty bitvec
    BitVecClear(&bv);
    result = result && (BitVecLen(&bv) == 0);

    // Test clear after single bit
    BitVecPush(&bv, true);
    BitVecClear(&bv);
    result = result && (BitVecLen(&bv) == 0);

    // Test multiple clears
    BitVecClear(&bv);
    BitVecClear(&bv);
    result = result && (BitVecLen(&bv) == 0);

    // Test clear after large data
    for (int i = 0; i < 1000; i++) {
        BitVecPush(&bv, i % 2);
    }
    BitVecClear(&bv);
    result = result && (BitVecLen(&bv) == 0);

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_bitvec_multiple_cycles(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVec multiple init/deinit cycles\n");

    bool result = true;

    // Test multiple init/deinit cycles
    for (int cycle = 0; cycle < 100; cycle++) {
        BitVec bv = BitVecInit(ALLOCATOR_OF(&alloc));

        // Add some data
        for (int i = 0; i < cycle % 10; i++) {
            BitVecPush(&bv, i % 2);
        }

        result = result && (BitVecLen(&bv) == (size)(cycle % 10));
        BitVecDeinit(&bv);
    }

    DefaultAllocatorDeinit(&alloc);

    return result;
}

// Deadend tests - verify expected failures occur gracefully
bool test_bitvec_null_pointer_failures(void) {
    WriteFmt("Testing BitVec NULL pointer handling\n");

    // Test NULL pointer passed to functions that should validate
    // These should trigger aborts if validation is working

    // Test NULL bitvec pointer - should abort
    BitVecDeinit(NULL);

    // If we reach here, validation didn't work as expected
    return false;
}

bool test_bitvec_invalid_operations(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVec invalid operations\n");

    // BitVecReserve is now a fallible API that returns bool on allocation
    // failure rather than aborting. The Must variant aborts, so use it here
    // to validate the deadend abort path.
    BitVec bv = BitVecInit(ALLOCATOR_OF(&alloc));

    BitVecMustReserve(&bv, SIZE_MAX);

    // If we reach here, validation didn't work as expected
    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

bool test_bitvec_set_operations_failures(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVec set operations on invalid indices\n");

    // BitVecResize is now a fallible API that returns bool on allocation
    // failure rather than aborting. The Must variant aborts, so use it here
    // to validate the deadend abort path.
    BitVec bv = BitVecInit(ALLOCATOR_OF(&alloc));

    BitVecMustResize(&bv, SIZE_MAX);

    // If we reach here, validation didn't work as expected
    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// Main function that runs all tests
int main(void) {
    WriteFmt("[INFO] Starting BitVec.Init tests\n\n");

    // Array of normal test functions
    TestFunction tests[] = {
        test_bitvec_init,
        test_bitvec_deinit,
        test_bitvec_reserve,
        test_bitvec_clear,
        test_bitvec_resize,
        test_bitvec_init_edge_cases,
        test_bitvec_reserve_edge_cases,
        test_bitvec_resize_edge_cases,
        test_bitvec_clear_edge_cases,
        test_bitvec_multiple_cycles
    };

    // Array of deadend test functions (expected failure scenarios)
    TestFunction deadend_tests[] = {
        test_bitvec_null_pointer_failures,
        test_bitvec_invalid_operations,
        test_bitvec_set_operations_failures
    };

    int total_tests         = sizeof(tests) / sizeof(tests[0]);
    int total_deadend_tests = sizeof(deadend_tests) / sizeof(deadend_tests[0]);

    // Run all tests using the centralized test driver
    return run_test_suite(tests, total_tests, deadend_tests, total_deadend_tests, "BitVec.Init");
}
