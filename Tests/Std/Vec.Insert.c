#include <Misra/Std/Container/Vec.h>
#include <Misra/Std/Log.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <Misra/Types.h> // For LVAL macro

// Function prototypes
bool test_vec_push_back(void);
bool test_vec_push_front(void);
bool test_vec_insert(void);
bool test_vec_push_back_arr(void);
bool test_vec_push_front_arr(void);
bool test_vec_push_arr(void);
bool test_vec_insert_range(void);
bool test_vec_merge(void);

// Test VecPushBack function
bool test_vec_push_back(void) {
    printf("Testing VecPushBack\n");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit();

    // Push some elements to the back
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++) {
        VecPushBackR(&vec, values[i]);
    }

    // Check length
    bool result = (vec.length == 5);

    // Check elements in order
    for (size i = 0; i < vec.length; i++) {
        result = result && (VecAt(&vec, i) == values[i]);
    }

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Test VecPushFront function
bool test_vec_push_front(void) {
    printf("Testing VecPushFront\n");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit();

    // Push some elements to the front
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++) {
        VecPushFrontR(&vec, values[i]);
    }

    // Check length
    bool result = (vec.length == 5);

    // Check elements in reverse order (since we pushed to front)
    for (size i = 0; i < vec.length; i++) {
        result = result && (VecAt(&vec, i) == values[4 - i]);
    }

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Test VecInsert function
bool test_vec_insert(void) {
    printf("Testing VecInsert\n");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit();

    // Insert at index 0 (empty vector)
    VecInsertR(&vec, 10, 0);

    // Check first element
    bool result = (vec.length == 1 && VecAt(&vec, 0) == 10);

    // Insert at the end
    VecInsertR(&vec, 30, 1);

    // Check elements
    result = result && (vec.length == 2 && VecAt(&vec, 0) == 10 && VecAt(&vec, 1) == 30);

    // Insert in the middle
    VecInsertR(&vec, 20, 1);

    // Check all elements
    result = result && (vec.length == 3);
    result = result && (VecAt(&vec, 0) == 10);
    result = result && (VecAt(&vec, 1) == 20);
    result = result && (VecAt(&vec, 2) == 30);

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Test VecPushBackArr function
bool test_vec_push_back_arr(void) {
    printf("Testing VecPushBackArr\n");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit();

    // Push an array to the back
    int values[] = {10, 20, 30, 40, 50};
    VecPushBackArrR(&vec, values, 5);

    // Check length
    bool result = (vec.length == 5);

    // Check elements in order
    for (size i = 0; i < vec.length; i++) {
        result = result && (VecAt(&vec, i) == values[i]);
    }

    // Push another array to the back
    int more_values[] = {60, 70, 80};
    VecPushBackArrR(&vec, more_values, 3);

    // Check length
    result = result && (vec.length == 8);

    // Check all elements
    for (size i = 0; i < 5; i++) {
        result = result && (VecAt(&vec, i) == values[i]);
    }
    for (size i = 0; i < 3; i++) {
        result = result && (VecAt(&vec, i + 5) == more_values[i]);
    }

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Test VecPushFrontArr function
bool test_vec_push_front_arr(void) {
    printf("Testing VecPushFrontArr\n");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit();

    // Push an array to the front of empty vector
    int values[] = {10, 20, 30, 40, 50};
    VecPushFrontArrR(&vec, values, 5);

    // Check length
    bool result = (vec.length == 5);

    // Check elements in order
    for (size i = 0; i < vec.length; i++) {
        result = result && (VecAt(&vec, i) == values[i]);
    }

    // Push another array to the front
    int more_values[] = {60, 70, 80};
    VecPushFrontArrR(&vec, more_values, 3);

    // Check length
    result = result && (vec.length == 8);

    // Check all elements
    for (size i = 0; i < 3; i++) {
        result = result && (VecAt(&vec, i) == more_values[i]);
    }
    for (size i = 0; i < 5; i++) {
        result = result && (VecAt(&vec, i + 3) == values[i]);
    }

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Test VecInsertRange function for inserting at a specific index
bool test_vec_push_arr(void) {
    printf("Testing VecInsertRange at specific index\n");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit();

    // Push some elements first
    VecPushBackR(&vec, 10);
    VecPushBackR(&vec, 20);

    // Push an array at a specific index
    int values[] = {30, 40, 50};
    VecInsertRangeR(&vec, values, 1, 3);

    // Check length
    bool result = (vec.length == 5);

    // Expected result: [10, 30, 40, 50, 20]
    int expected[] = {10, 30, 40, 50, 20};

    // Check all elements
    for (size i = 0; i < vec.length; i++) {
        result = result && (VecAt(&vec, i) == expected[i]);
    }

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Test VecInsertRange function for inserting from another vector
bool test_vec_insert_range(void) {
    printf("Testing VecInsertRange from another vector\n");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit();

    // Add some initial elements
    int initial[] = {10, 20, 30};
    VecPushBackArrR(&vec, initial, 3);

    // Create another vector with elements to insert
    IntVec src          = VecInit();
    int    src_values[] = {40, 50, 60};
    VecPushBackArrR(&src, src_values, 3);

    // Insert range in the middle
    VecInsertRangeR(&vec, src.data, 1, src.length);

    // Check length
    bool result = (vec.length == 6);

    // Expected result: [10, 40, 50, 60, 20, 30]
    int expected[] = {10, 40, 50, 60, 20, 30};

    // Check all elements
    for (size i = 0; i < vec.length; i++) {
        result = result && (VecAt(&vec, i) == expected[i]);
    }

    // Clean up
    VecDeinit(&vec);
    VecDeinit(&src);

    return result;
}

// Test VecMerge function
bool test_vec_merge(void) {
    printf("Testing VecMerge\n");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec1 = VecInit();

    // Add some elements to first vector
    int values1[] = {10, 20, 30};
    VecPushBackArrR(&vec1, values1, 3);

    // Create second vector
    IntVec vec2 = VecInit();

    // Add some elements to second vector
    int values2[] = {40, 50, 60};
    VecPushBackArrR(&vec2, values2, 3);

    // Merge vec2 into vec1
    VecMergeR(&vec1, &vec2);

    // Check lengths
    bool result = (vec1.length == 6);
    result      = result && (vec2.length == 3); // VecMergeR doesn't modify source vector

    // Expected result in vec1: [10, 20, 30, 40, 50, 60]
    int expected[] = {10, 20, 30, 40, 50, 60};

    // Check all elements in vec1
    for (size i = 0; i < vec1.length; i++) {
        result = result && (VecAt(&vec1, i) == expected[i]);
    }

    // Clean up
    VecDeinit(&vec1);
    VecDeinit(&vec2);

    return result;
}

// Main function that runs all tests
int main(void) {
    printf("[INFO] Starting Vec.Insert tests\n\n");

    // Array of test functions
    bool (*tests[])(void) = {
        test_vec_push_back,
        test_vec_push_front,
        test_vec_insert,
        test_vec_push_back_arr,
        test_vec_push_front_arr,
        test_vec_push_arr,
        test_vec_insert_range,
        test_vec_merge
    };

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