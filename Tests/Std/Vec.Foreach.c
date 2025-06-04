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

// Deadend test prototypes (tests that should crash due to out-of-bounds access)
bool test_vec_foreach_out_of_bounds_access(void);
bool test_vec_foreach_idx_out_of_bounds_access(void);
bool test_vec_foreach_idx_basic_out_of_bounds_access(void);
bool test_vec_foreach_reverse_idx_out_of_bounds_access(void);
bool test_vec_foreach_ptr_idx_out_of_bounds_access(void);
bool test_vec_foreach_ptr_reverse_idx_out_of_bounds_access(void);
bool test_vec_foreach_ptr_in_range_idx_out_of_bounds_access(void);

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

    // Use VecForeachReverseIdx to verify indices and values in reverse
    bool result = true;
    VecForeachReverseIdx(&vec, item, idx, { result = result && (item == values[idx]); });

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

    // Use VecForeachPtrReverse to negate the values in reverse order
    VecForeachPtrReverse(&vec, item_ptr, { *item_ptr = -*item_ptr; });

    // Check that the values in the vector are negated
    bool result = true;
    for (size i = 0; i < vec.length; i++) {
        result = result && (VecAt(&vec, i) == -values[i]);
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

    // Use VecForeachPtrReverseIdx to set each value to its reverse index
    VecForeachPtrReverseIdx(&vec, item_ptr, idx, { *item_ptr = vec.length - idx - 1; });

    // Check that the values in the vector are set to their reverse indices
    bool result = true;
    for (size i = 0; i < vec.length; i++) {
        result = result && (VecAt(&vec, i) == vec.length - i - 1);
    }

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Deadend test: Make idx go out of bounds in VecForeachInRangeIdx by shrinking vector during iteration
bool test_vec_foreach_out_of_bounds_access(void) {
    printf("Testing VecForeachInRangeIdx where idx goes out of bounds (should crash)\n");

    typedef Vec(int) IntVec;
    IntVec vec = VecInit();

    // Add several elements
    for (int i = 0; i < 10; i++) {
        VecPushBackR(&vec, i * 10);
    }

    // Use VecForeachInRangeIdx which captures the 'end' parameter (10) at the start
    // Even if we shrink the vector, the loop will continue until idx reaches 10
    size original_length = vec.length;  // Capture this as 10
    VecForeachInRangeIdx(&vec, val, idx, 0, original_length, {
        printf("Accessing idx %zu (vec.length=%zu): %d\n", idx, vec.length, val);
        
        // When we reach idx=3, drastically shrink the vector to length 2
        // But VecForeachInRangeIdx will continue until idx reaches original_length (10)
        if (idx == 3) {
            VecResize(&vec, 2);  // Shrink to only 2 elements
            printf("Vector resized to length %zu, but range iteration will continue to idx %zu...\n", vec.length, original_length);
        }
        
        // When idx >= 2 (after resize), VecForeachInRangeIdx will detect:
        // if ((idx) >= (v)->length) LOG_FATAL(...)
        // This should cause a fatal error when idx >= vec.length
    });

    // Should never reach here if idx goes out of bounds
    VecDeinit(&vec);
    return false;
}

// Deadend test: Make idx go out of bounds in VecForeachInRangeIdx by deleting elements
bool test_vec_foreach_idx_out_of_bounds_access(void) {
    printf("Testing VecForeachInRangeIdx with element deletion where idx goes out of bounds (should crash)\n");

    typedef Vec(int) IntVec;
    IntVec vec = VecInit();

    // Add several elements
    for (int i = 0; i < 8; i++) {
        VecPushBackR(&vec, i * 20);
    }

    // Use VecForeachInRangeIdx with a fixed range that will become invalid
    // when we delete elements during iteration
    size original_length = vec.length;  // Capture this as 8
    VecForeachInRangeIdx(&vec, val, idx, 0, original_length, {
        printf("Accessing idx %zu (vec.length=%zu): %d\n", idx, vec.length, val);
        
        // When we reach idx=2, delete several elements from the beginning
        // This will make the higher indices invalid
        if (idx == 2) {
            VecDeleteRange(&vec, 0, 5);  // Remove first 5 elements
            printf("Deleted first 5 elements, new length=%zu, but range iteration will continue to idx %zu...\n", vec.length, original_length);
        }
        
        // When idx >= 3 (after deletion), VecForeachInRangeIdx will detect:
        // if ((idx) >= (v)->length) LOG_FATAL(...)
        // This should cause a fatal error when idx >= vec.length
    });

    // Should never reach here if bounds checking triggers
    VecDeinit(&vec);
    return false;
}

// Deadend test: Make idx go out of bounds in VecForeachReverseIdx by modifying vector during iteration
bool test_vec_foreach_reverse_idx_out_of_bounds_access(void) {
    printf("Testing VecForeachReverseIdx where idx goes out of bounds (should crash)\n");

    typedef Vec(int) IntVec;
    IntVec vec = VecInit();

    // Add several elements
    for (int i = 0; i < 6; i++) {
        VecPushBackR(&vec, i * 15);
    }

    // VecForeachReverseIdx has explicit bounds checking: if ((idx) >= (v)->length) LOG_FATAL(...)
    VecForeachReverseIdx(&vec, val, idx, {
        printf("Accessing idx %zu (vec.length=%zu): %d\n", idx, vec.length, val);
        
        // When we reach idx=3, drastically shrink the vector
        // This will make subsequent iterations invalid since idx will still decrement
        // but the vector length is now smaller
        if (idx == 3) {
            VecResize(&vec, 2);  // Shrink to only 2 elements
            printf("Vector resized to length %zu during reverse iteration...\n", vec.length);
        }
        
        // When idx >= vec.length, the bounds check will trigger:
        // if ((idx) >= (v)->length) LOG_FATAL(...)
    });

    // Should never reach here if bounds checking triggers
    VecDeinit(&vec);
    return false;
}

// Deadend test: Make idx go out of bounds in VecForeachPtrIdx by modifying vector during iteration
bool test_vec_foreach_ptr_idx_out_of_bounds_access(void) {
    printf("Testing VecForeachPtrIdx where idx goes out of bounds (should crash)\n");

    typedef Vec(int) IntVec;
    IntVec vec = VecInit();

    // Add several elements
    for (int i = 0; i < 7; i++) {
        VecPushBackR(&vec, i * 25);
    }

    // VecForeachPtrIdx has explicit bounds checking: if ((idx) >= (v)->length) LOG_FATAL(...)
    VecForeachPtrIdx(&vec, val_ptr, idx, {
        printf("Accessing idx %zu (vec.length=%zu): %d\n", idx, vec.length, *val_ptr);
        
        // When we reach idx=3, shrink the vector to make the CURRENT idx invalid
        // The bounds check happens after the body, so it will check if idx=3 >= new_length
        if (idx == 3) {
            VecResize(&vec, 3);  // Shrink so that idx=3 becomes out of bounds (valid indices: 0,1,2)
            printf("Vector resized to length %zu, current idx=%zu is now out of bounds...\n", vec.length, idx);
        }
        
        // When idx >= vec.length, the bounds check will trigger:
        // if ((idx) >= (v)->length) LOG_FATAL(...)
    });

    // Should never reach here if bounds checking triggers
    VecDeinit(&vec);
    return false;
}

// Deadend test: Make idx go out of bounds in VecForeachPtrReverseIdx by modifying vector during iteration
bool test_vec_foreach_ptr_reverse_idx_out_of_bounds_access(void) {
    printf("Testing VecForeachPtrReverseIdx where idx goes out of bounds (should crash)\n");

    typedef Vec(int) IntVec;
    IntVec vec = VecInit();

    // Add several elements
    for (int i = 0; i < 8; i++) {
        VecPushBackR(&vec, i * 35);
    }

    // VecForeachPtrReverseIdx has explicit bounds checking: if ((idx) >= (v)->length) LOG_FATAL(...)
    VecForeachPtrReverseIdx(&vec, val_ptr, idx, {
        printf("Accessing idx %zu (vec.length=%zu): %d\n", idx, vec.length, *val_ptr);
        
        // When we reach idx=5, shrink the vector significantly
        if (idx == 5) {
            VecResize(&vec, 3);  // Shrink to only 3 elements
            printf("Vector resized to length %zu during reverse ptr iteration...\n", vec.length);
        }
        
        // When idx >= vec.length, the bounds check will trigger:
        // if ((idx) >= (v)->length) LOG_FATAL(...)
    });

    // Should never reach here if bounds checking triggers
    VecDeinit(&vec);
    return false;
}

// Deadend test: Make idx go out of bounds in VecForeachPtrInRangeIdx by modifying vector during iteration
bool test_vec_foreach_ptr_in_range_idx_out_of_bounds_access(void) {
    printf("Testing VecForeachPtrInRangeIdx where idx goes out of bounds (should crash)\n");

    typedef Vec(int) IntVec;
    IntVec vec = VecInit();

    // Add several elements
    for (int i = 0; i < 9; i++) {
        VecPushBackR(&vec, i * 45);
    }

    // Use VecForeachPtrInRangeIdx with a fixed range that becomes invalid when we modify the vector
    size original_length = vec.length;  // Capture this as 9
    VecForeachPtrInRangeIdx(&vec, val_ptr, idx, 0, original_length, {
        printf("Accessing idx %zu (vec.length=%zu): %d\n", idx, vec.length, *val_ptr);
        
        // When we reach idx=3, delete several elements
        if (idx == 3) {
            VecDeleteRange(&vec, 0, 6);  // Remove first 6 elements  
            printf("Deleted first 6 elements, new length=%zu, but range ptr iteration continues to idx %zu...\n", vec.length, original_length);
        }
        
        // When idx >= vec.length, the bounds check will trigger:
        // if ((idx) >= (v)->length) LOG_FATAL(...)
    });

    // Should never reach here if bounds checking triggers
    VecDeinit(&vec);
    return false;
}

// Deadend test: Make idx go out of bounds in basic VecForeachIdx by modifying vector during iteration
bool test_vec_foreach_idx_basic_out_of_bounds_access(void) {
    printf("Testing basic VecForeachIdx where idx goes out of bounds (should crash)\n");

    typedef Vec(int) IntVec;
    IntVec vec = VecInit();

    // Add several elements
    for (int i = 0; i < 5; i++) {
        VecPushBackR(&vec, i * 30);
    }

    // Basic VecForeachIdx now has explicit bounds checking: if ((idx) >= (v)->length) LOG_FATAL(...)
    VecForeachIdx(&vec, val, idx, {
        printf("Accessing idx %zu (vec.length=%zu): %d\n", idx, vec.length, val);
        
        // When we reach idx=2, drastically shrink the vector
        // This will make subsequent iterations invalid 
        if (idx == 2) {
            VecResize(&vec, 1);  // Shrink to only 1 element
            printf("Vector resized to length %zu, but basic foreach iteration continues...\n", vec.length);
        }
        
        // When idx >= vec.length, the bounds check will trigger:
        // if ((idx) >= (v)->length) LOG_FATAL(...)
    });

    // Should never reach here if bounds checking triggers
    VecDeinit(&vec);
    return false;
}

// Main function that runs all tests
int main(void) {
    printf("[INFO] Starting Vec.Foreach tests\n\n");

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

    // Array of deadend test functions (tests that should crash)
    TestFunction deadend_tests[] = {
        test_vec_foreach_out_of_bounds_access,
        test_vec_foreach_idx_out_of_bounds_access,
        test_vec_foreach_idx_basic_out_of_bounds_access,
        test_vec_foreach_reverse_idx_out_of_bounds_access,
        test_vec_foreach_ptr_idx_out_of_bounds_access,
        test_vec_foreach_ptr_reverse_idx_out_of_bounds_access,
        test_vec_foreach_ptr_in_range_idx_out_of_bounds_access
    };

    int total_tests = sizeof(tests) / sizeof(tests[0]);
    int deadend_count = sizeof(deadend_tests) / sizeof(deadend_tests[0]);

    // Run all tests using the centralized test driver
    return run_test_suite(tests, total_tests, deadend_tests, deadend_count, "Vec.Foreach");
}