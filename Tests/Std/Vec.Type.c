#include <Misra/Std/Container/Vec.h>
#include <Misra/Std/Log.h>
#include <stdio.h>
#include <Misra/Types.h>

// Include test utilities
#include "../Util/TestRunner.h"

// Define a test struct for our vector tests
typedef struct {
    int   id;
    float value;
} TestItem;

// Function prototypes
bool test_vec_type_basic(void);
bool test_vec_validate(void);

// Test basic Vec type functionality
bool test_vec_type_basic(void) {
    WriteFmt("Testing basic Vec type functionality\n");

    // Define a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit();

    // Check initial state
    bool result =
        (vec.length == 0 && vec.capacity == 0 && vec.data == NULL && vec.allocator.alignment == 1 &&
         vec.copy_init == NULL && vec.copy_deinit == NULL);

    // Clean up
    VecDeinit(&vec);

    // Test with a struct type
    typedef Vec(TestItem) TestVec;
    TestVec test_vec = VecInit();

    // Check initial state
    result =
        result && (test_vec.length == 0 && test_vec.capacity == 0 && test_vec.data == NULL &&
                   test_vec.allocator.alignment == 1 && test_vec.copy_init == NULL && test_vec.copy_deinit == NULL);

    // Clean up
    VecDeinit(&test_vec);

    return result;
}

// Test ValidateVec macro
bool test_vec_validate(void) {
    WriteFmt("Testing ValidateVec macro\n");

    // Create a valid vector
    typedef Vec(int) IntVec;
    IntVec vec = VecInit();

    // This should not abort
    ValidateVec(&vec);

    // Clean up
    VecDeinit(&vec);

    // Note: We can't easily test the negative case (invalid vector)
    // as it would abort the program

    return true;
}

// Main function that runs all tests
int main(void) {
    WriteFmt("[INFO] Starting Vec.Type tests\n\n");

    // Array of test functions
    TestFunction tests[] = {test_vec_type_basic, test_vec_validate};

    int total_tests = sizeof(tests) / sizeof(tests[0]);

    // Run all tests using the centralized test driver
    return run_test_suite(tests, total_tests, NULL, 0, "Vec.Type");
}
