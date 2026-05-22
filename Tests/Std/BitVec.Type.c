#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Container/BitVec.h>
#include <Misra/Std/Log.h>

#include <Misra/Types.h>

// Include test utilities
#include "../Util/TestRunner.h"

// Function prototypes
bool test_bitvec_type_basic(void);
bool test_bitvec_validate(void);

// Test basic BitVec type functionality
bool test_bitvec_type_basic(void) {
    WriteFmt("Testing basic BitVec type functionality\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    // Create a bitvector
    BitVec bitvec = BitVecInit(ALLOCATOR_OF(&alloc));

    // Check initial state
    bool result =
        (BitVecLen(&bitvec) == 0 && BitVecCapacity(&bitvec) == 0 && BitVecData(&bitvec) == NULL &&
         bitvec.byte_size == 0);

    // Clean up
    BitVecDeinit(&bitvec);
    DefaultAllocatorDeinit(&alloc);

    return result;
}

// Test ValidateBitVec macro
bool test_bitvec_validate(void) {
    WriteFmt("Testing ValidateBitVec macro\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    // Create a valid bitvector
    BitVec bitvec = BitVecInit(ALLOCATOR_OF(&alloc));

    // This should not abort
    ValidateBitVec(&bitvec);

    // Clean up
    BitVecDeinit(&bitvec);
    DefaultAllocatorDeinit(&alloc);

    // Note: We can't easily test the negative case (invalid bitvector)
    // as it would abort the program

    return true;
}

// Main function that runs all tests
int main(void) {
    WriteFmt("[INFO] Starting BitVec.Type tests\n\n");

    // Array of test functions
    TestFunction tests[] = {test_bitvec_type_basic, test_bitvec_validate};

    int total_tests = sizeof(tests) / sizeof(tests[0]);

    // Run all tests using the centralized test driver
    return run_test_suite(tests, total_tests, NULL, 0, "BitVec.Type");
}
