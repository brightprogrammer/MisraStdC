#include <Misra/Std/Container/Bits.h>
#include <Misra/Std/Log.h>

#include <stdio.h>
#include <Misra/Types.h>

// Include test utilities
#include "../Util/TestRunner.h"

// Function prototypes
bool test_Bits_type_basic(void);
bool test_Bits_validate(void);

// Test basic Bits type functionality
bool test_Bits_type_basic(void) {
    printf("Testing basic Bits type functionality\n");

    // Create a Bitstor
    Bits Bits = BitsInit();

    // Check initial state
    bool result = (Bits.length == 0 && Bits.capacity == 0 && Bits.data == NULL && Bits.byte_size == 0);

    // Clean up
    BitsDeinit(&Bits);

    return result;
}

// Test ValidateBits macro
bool test_Bits_validate(void) {
    printf("Testing ValidateBits macro\n");

    // Create a valid Bitstor
    Bits Bits = BitsInit();

    // This should not abort
    ValidateBits(&Bits);

    // Clean up
    BitsDeinit(&Bits);

    // Note: We can't easily test the negative case (invalid Bitstor)
    // as it would abort the program

    return true;
}

// Main function that runs all tests
int main(void) {
    printf("[INFO] Starting Bits.Type tests\n\n");

    // Array of test functions
    TestFunction tests[] = {test_Bits_type_basic, test_Bits_validate};

    int total_tests = sizeof(tests) / sizeof(tests[0]);

    // Run all tests using the centralized test driver
    return run_test_suite(tests, total_tests, NULL, 0, "Bits.Type");
}
