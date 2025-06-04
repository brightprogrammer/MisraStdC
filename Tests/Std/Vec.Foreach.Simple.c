#include <Misra/Std/Container/Vec.h>
#include <Misra/Std/Log.h>
#include <stdbool.h>
#include <stdio.h>
#include <Misra/Types.h> // For LVAL macro

// Include test utilities
#include "../Util/TestRunner.h"

// Function prototypes
bool test_vec_foreach(void);
bool test_vec_foreach_idx(void);
bool test_vec_foreach_ptr(void);
bool test_vec_foreach_ptr_idx(void);
bool test_vec_foreach_reverse(void);
bool test_vec_foreach_reverse_idx(void);
bool test_vec_foreach_ptr_reverse(void);
bool test_vec_foreach_ptr_reverse_idx(void);

// Test VecForeach macro
bool test_vec_foreach(void) {
    printf("Testing VecForeach\n");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit();

    // Add some data
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++) {
        VecPushBackR(&vec, values[i]);
    }

    // Use VecForeach to sum the values
    int sum = 0;
    VecForeach(&vec, item, { sum += item; });

    // Check the sum
    bool result = (sum == 150); // 10 + 20 + 30 + 40 + 50 = 150

    // Use VecForeach to double each value
    VecForeach(&vec, item, { item *= 2; });

    // Check that the values in the vector are unchanged (foreach uses value, not reference)
    for (size i = 0; i < vec.length; i++) {
        result = result && (VecAt(&vec, i) == values[i]);
    }

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Test VecForeachIdx macro
bool test_vec_foreach_idx(void) {
    printf("Testing VecForeachIdx\n");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit();

    // Add some data
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++) {
        VecPushBackR(&vec, values[i]);
    }

    // Use VecForeachIdx to verify indices and values
    bool result = true;
    VecForeachIdx(&vec, item, idx, { result = result && (item == values[idx]); });

    // Use VecForeachIdx to calculate weighted sum (value * index)
    int weighted_sum = 0;
    VecForeachIdx(&vec, item, idx, { weighted_sum += item * idx; });

    // Check the weighted sum
    // 10*0 + 20*1 + 30*2 + 40*3 + 50*4 = 0 + 20 + 60 + 120 + 200 = 400
    result = result && (weighted_sum == 400);

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Test VecForeachPtr macro
bool test_vec_foreach_ptr(void) {
    printf("Testing VecForeachPtr\n");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit();

    // Add some data
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++) {
        VecPushBackR(&vec, values[i]);
    }

    // Use VecForeachPtr to modify the values in the vector
    VecForeachPtr(&vec, item_ptr, { *item_ptr *= 2; });

    // Check that the values in the vector are doubled
    bool result = true;
    for (size i = 0; i < vec.length; i++) {
        result = result && (VecAt(&vec, i) == values[i] * 2);
    }

    // Use VecForeachPtr to calculate sum
    int sum = 0;
    VecForeachPtr(&vec, item_ptr, { sum += *item_ptr; });

    // Check the sum (should be doubled values)
    // 20 + 40 + 60 + 80 + 100 = 300
    result = result && (sum == 300);

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Test VecForeachPtrIdx macro
bool test_vec_foreach_ptr_idx(void) {
    printf("Testing VecForeachPtrIdx\n");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit();

    // Add some data
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++) {
        VecPushBackR(&vec, values[i]);
    }

    // Use VecForeachPtrIdx to set each value to its index
    VecForeachPtrIdx(&vec, item_ptr, idx, { *item_ptr = idx; });

    // Check that the values in the vector are set to their indices
    bool result = true;
    for (size i = 0; i < vec.length; i++) {
        result = result && (VecAt(&vec, i) == i);
    }

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Test VecForeachReverse macro
bool test_vec_foreach_reverse(void) {
    printf("Testing VecForeachReverse\n");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit();

    // Add some data
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++) {
        VecPushBackR(&vec, values[i]);
    }

    // Use VecForeachReverse to build a reversed array
    int reversed[5] = {0};
    int idx         = 0;
    VecForeachReverse(&vec, item, { reversed[idx++] = item; });

    // Check that the reversed array is correct
    bool result = true;
    for (int i = 0; i < 5; i++) {
        result = result && (reversed[i] == values[4 - i]);
    }

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Test VecForeachReverseIdx macro
bool test_vec_foreach_reverse_idx(void) {
    printf("Testing VecForeachReverseIdx\n");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit();

    // Add some data
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++) {
        VecPushBackR(&vec, values[i]);
    }

    // Use VecForeachReverseIdx to verify indices are correct in reverse
    bool result = true;
    VecForeachReverseIdx(&vec, item, idx, {
        result = result && (item == values[idx]);
        result = result && (VecAt(&vec, idx) == item);
    });

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Test VecForeachPtrReverse macro
bool test_vec_foreach_ptr_reverse(void) {
    printf("Testing VecForeachPtrReverse\n");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit();

    // Add some data
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++) {
        VecPushBackR(&vec, values[i]);
    }

    // Use VecForeachPtrReverse to increment values in reverse order
    int increment = 1;
    VecForeachPtrReverse(&vec, item_ptr, { *item_ptr += increment++; });

    // Values should now be: [15, 24, 33, 42, 51]
    // (50+1, 40+2, 30+3, 20+4, 10+5)
    int expected[] = {15, 24, 33, 42, 51};
    bool result = true;
    for (size i = 0; i < vec.length; i++) {
        result = result && (VecAt(&vec, i) == expected[i]);
    }

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Test VecForeachPtrReverseIdx macro
bool test_vec_foreach_ptr_reverse_idx(void) {
    printf("Testing VecForeachPtrReverseIdx\n");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit();

    // Add some data
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++) {
        VecPushBackR(&vec, values[i]);
    }

    // Use VecForeachPtrReverseIdx to set each value to its index + 100
    VecForeachPtrReverseIdx(&vec, item_ptr, idx, { *item_ptr = idx + 100; });

    // Check that the values are set correctly
    // Even though we iterate in reverse, idx represents the actual vector index
    // So: vec[4] = 104, vec[3] = 103, vec[2] = 102, vec[1] = 101, vec[0] = 100
    // Final vector: [100, 101, 102, 103, 104]
    int expected[] = {100, 101, 102, 103, 104};
    bool result = true;
    for (size i = 0; i < vec.length; i++) {
        result = result && (VecAt(&vec, i) == expected[i]);
    }

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Main function that runs all tests
int main(void) {
    printf("[INFO] Starting Vec.Foreach.Simple tests\n\n");

    // Array of normal test functions
    TestFunction tests[] = {
        test_vec_foreach,
        test_vec_foreach_idx,
        test_vec_foreach_ptr,
        test_vec_foreach_ptr_idx,
        test_vec_foreach_reverse,
        test_vec_foreach_reverse_idx,
        test_vec_foreach_ptr_reverse,
        test_vec_foreach_ptr_reverse_idx
    };

    int total_tests = sizeof(tests) / sizeof(tests[0]);

    // Run all tests using the centralized test driver
    return run_test_suite(tests, total_tests, NULL, 0, "Vec.Foreach.Simple");
} 