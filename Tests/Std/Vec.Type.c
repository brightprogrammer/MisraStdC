#include <Misra/Std/Container/Vec.h>
#include <Misra/Std/Log.h>
#include <stdbool.h>
#include <stdio.h>

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
    printf("Testing basic Vec type functionality\n");

    // Define a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit();

    // Check initial state
    bool result =
        (vec.length == 0 && vec.capacity == 0 && vec.data == NULL && vec.alignment == 1 && vec.copy_init == NULL &&
         vec.copy_deinit == NULL);

    // Clean up
    VecDeinit(&vec);

    // Test with a struct type
    typedef Vec(TestItem) TestVec;
    TestVec test_vec = VecInit();

    // Check initial state
    result = result && (test_vec.length == 0 && test_vec.capacity == 0 && test_vec.data == NULL &&
                        test_vec.alignment == 1 && test_vec.copy_init == NULL && test_vec.copy_deinit == NULL);

    // Clean up
    VecDeinit(&test_vec);

    return result;
}

// Test ValidateVec macro
bool test_vec_validate(void) {
    printf("Testing ValidateVec macro\n");

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
    printf("[INFO] Starting Vec.Type tests\n\n");

    // Array of test functions
    bool (*tests[])(void) = {test_vec_type_basic, test_vec_validate};

    int total_tests = sizeof(tests) / sizeof(tests[0]);
    int passed      = 0;
    int failed      = 0;

    // Run all tests and accumulate results
    for (int i = 0; i < total_tests; i++) {
        printf("[TEST %d/%d] ", i + 1, total_tests);
        bool result = tests[i]();
        if (result) {
            printf("[PASS]\n\n");
            passed++;
        } else {
            printf("[FAIL]\n\n");
            failed++;
        }
    }

    // Print summary
    printf("[SUMMARY] Total: %d, Passed: %d, Failed: %d\n", total_tests, passed, failed);

    // Return non-zero exit code if any test failed
    return failed > 0 ? 1 : 0;
}