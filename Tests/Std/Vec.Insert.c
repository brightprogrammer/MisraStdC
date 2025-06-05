#include <Misra/Std/Container/Vec.h>
#include <Misra/Std/Log.h>
#include <stdio.h>
#include <string.h>
#include <Misra/Types.h> // For LVAL macro

// Include test utilities
#include "../Util/TestRunner.h"

// Function prototypes
bool test_vec_push_back(void);
bool test_vec_push_front(void);
bool test_vec_insert(void);
bool test_vec_push_back_arr(void);
bool test_vec_push_front_arr(void);
bool test_vec_push_arr(void);
bool test_vec_insert_range(void);
bool test_vec_merge(void);
bool test_lvalue_rvalue_operations(void);
bool test_lvalue_memset_after_insertion(void);

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

// Test L-value and R-value operations
bool test_lvalue_rvalue_operations(void) {
    printf("Testing L-value and R-value operations\n");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit();

    // Test R-value insert operations
    VecPushBackR(&vec, LVAL(42));

    // Check that the element was added
    bool result = (vec.length == 1 && VecAt(&vec, 0) == 42);

    // Test L-value insert operations
    int l_value = 100;
    VecPushBackL(&vec, l_value);

    // Check that the element was added
    result = result && (vec.length == 2 && VecAt(&vec, 1) == 100);

    // Test R-value insert at index
    VecInsertR(&vec, LVAL(50), 1);

    // Check that the element was inserted
    result = result && (vec.length == 3);
    result = result && (VecAt(&vec, 0) == 42);
    result = result && (VecAt(&vec, 1) == 50);
    result = result && (VecAt(&vec, 2) == 100);

    // Test L-value insert at index
    int insert_value = 75;
    VecInsertL(&vec, insert_value, 2);

    // Check that the element was inserted
    result = result && (vec.length == 4);
    result = result && (VecAt(&vec, 0) == 42);
    result = result && (VecAt(&vec, 1) == 50);
    result = result && (VecAt(&vec, 2) == 75);
    result = result && (VecAt(&vec, 3) == 100);

    // Test R-value fast insert
    VecInsertFastR(&vec, LVAL(60), 1);

    // Check that the element was inserted
    result = result && (vec.length == 5);
    result = result && (VecAt(&vec, 1) == 60);

    // Test L-value fast insert
    int fast_value = 80;
    VecInsertFastL(&vec, fast_value, 3);

    // Check that the element was inserted
    result = result && (vec.length == 6);
    result = result && (VecAt(&vec, 3) == 80);

    // Test array operations with L-values and R-values
    int arr[] = {200, 300, 400};

    // R-value array operations
    VecPushBackArrR(&vec, arr, 3);

    // Check that the elements were added
    result = result && (vec.length == 9);
    result = result && (VecAt(&vec, 6) == 200);
    result = result && (VecAt(&vec, 7) == 300);
    result = result && (VecAt(&vec, 8) == 400);

    // L-value array operations
    VecPushFrontArrL(&vec, arr, 3);

    // Check that the elements were added
    result = result && (vec.length == 12);
    result = result && (VecAt(&vec, 0) == 200);
    result = result && (VecAt(&vec, 1) == 300);
    result = result && (VecAt(&vec, 2) == 400);

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Test that L-value insertions properly memset values to 0 after insertion
bool test_lvalue_memset_after_insertion(void) {
    printf("Testing L-value memset after insertion\n");

    // Create a vector of integers without copy_init
    typedef Vec(int) IntVec;
    IntVec vec = VecInit();

    // Test VecPushBackL
    int val1 = 10;
    VecPushBackL(&vec, val1);
    bool result = (val1 == 0); // Should be memset to 0

    // Test VecPushFrontL
    int val2 = 20;
    VecPushFrontL(&vec, val2);
    result = result && (val2 == 0); // Should be memset to 0

    // Test VecInsertL
    int val3 = 30;
    VecInsertL(&vec, val3, 1);
    result = result && (val3 == 0); // Should be memset to 0

    // Test array operations
    int arr[] = {40, 50, 60};
    VecPushBackArrL(&vec, arr, 3);

    // Check that array elements are memset to 0
    result = result && (arr[0] == 0);
    result = result && (arr[1] == 0);
    result = result && (arr[2] == 0);

    // Test VecInsertFastL
    int val4 = 70;
    VecInsertFastL(&vec, val4, 2);
    result = result && (val4 == 0); // Should be memset to 0

    // Test VecInsertRangeL
    int range[] = {80, 90, 100};
    VecInsertRangeL(&vec, range, 1, 3);

    // Check that array elements are memset to 0
    result = result && (range[0] == 0);
    result = result && (range[1] == 0);
    result = result && (range[2] == 0);

    // Test VecInsertRangeFastL
    int fast_range[] = {110, 120, 130};
    VecInsertRangeFastL(&vec, fast_range, 3, 3);

    // Check that array elements are memset to 0
    result = result && (fast_range[0] == 0);
    result = result && (fast_range[1] == 0);
    result = result && (fast_range[2] == 0);

    // Test VecMergeL
    IntVec vec2         = VecInit();
    int    merge_vals[] = {140, 150, 160};
    for (int i = 0; i < 3; i++) {
        VecPushBackR(&vec2, merge_vals[i]);
    }

    // Merge with L-value semantics
    VecMergeL(&vec, &vec2);

    // Check that the source vector is cleared
    result = result && (vec2.length == 0);
    result = result && (vec2.data == NULL);

    // Clean up
    VecDeinit(&vec);
    VecDeinit(&vec2);

    return result;
}

// Main function that runs all tests
int main(void) {
    printf("[INFO] Starting Vec.Insert tests\n\n");

    // Array of test functions
    TestFunction tests[] = {
        test_vec_push_back,
        test_vec_push_front,
        test_vec_insert,
        test_vec_push_back_arr,
        test_vec_push_front_arr,
        test_vec_push_arr,
        test_vec_insert_range,
        test_vec_merge,
        test_lvalue_rvalue_operations,
        test_lvalue_memset_after_insertion
    };

    int total_tests = sizeof(tests) / sizeof(tests[0]);

    // Run all tests using the centralized test driver
    return run_test_suite(tests, total_tests, NULL, 0, "Vec.Insert");
}