#include <Misra/Std/Container/Vec.h>
#include <Misra/Std/Log.h>
#include <stdbool.h>
#include <stdio.h>
#include <Misra/Types.h> // For LVAL macro

// Include test utilities
#include "../Util/TestRunner.h"

// Deadend test prototypes (tests that should crash due to out-of-bounds access)
bool test_vec_foreach_out_of_bounds_access(void);
bool test_vec_foreach_idx_out_of_bounds_access(void);
bool test_vec_foreach_idx_basic_out_of_bounds_access(void);
bool test_vec_foreach_reverse_idx_out_of_bounds_access(void);
bool test_vec_foreach_ptr_idx_out_of_bounds_access(void);
bool test_vec_foreach_ptr_reverse_idx_out_of_bounds_access(void);
bool test_vec_foreach_ptr_in_range_idx_out_of_bounds_access(void);

// Deadend test: Make idx go out of bounds during VecForeach by modifying vector during iteration
bool test_vec_foreach_out_of_bounds_access(void) {
    printf("Testing VecForeach where modification causes out of bounds access (should crash)\n");

    typedef Vec(int) IntVec;
    IntVec vec = VecInit();

    // Add some elements
    for (int i = 0; i < 5; i++) {
        VecPushBackR(&vec, i * 10);
    }

    // VecForeach doesn't use an explicit index but we can still cause issues
    int iteration_count = 0;
    VecForeach(&vec, val, {
        printf("Iteration %d (vec.length=%zu): %d\n", iteration_count, vec.length, val);

        // After 2nd iteration, shrink the vector dramatically
        if (iteration_count == 2) {
            VecResize(&vec, 2); // Shrink to only 2 elements
            printf("Vector resized to length %zu during foreach iteration...\n", vec.length);
        }
        iteration_count++;

        // This will eventually cause bounds checking to trigger
    });

    // Should never reach here if bounds checking triggers
    VecDeinit(&vec);
    return false;
}

// Deadend test: Make idx go out of bounds in VecForeachIdx by modifying vector during iteration
bool test_vec_foreach_idx_out_of_bounds_access(void) {
    printf("Testing VecForeachIdx where idx goes out of bounds (should crash)\n");

    typedef Vec(int) IntVec;
    IntVec vec = VecInit();

    // Add some elements
    for (int i = 0; i < 6; i++) {
        VecPushBackR(&vec, i * 20);
    }

    // VecForeachIdx has explicit bounds checking: if ((idx) >= (v)->length) LOG_FATAL(...)
    VecForeachIdx(&vec, val, idx, {
        printf("Accessing idx %zu (vec.length=%zu): %d\n", idx, vec.length, val);

        // When we reach idx=2, drastically shrink the vector to make the current idx invalid
        // The bounds check happens after the body, so it will check if idx=2 >= new_length
        if (idx == 2) {
            VecResize(&vec, 2); // Shrink so that idx=2 becomes out of bounds (valid indices: 0,1)
            printf("Vector resized to length %zu, current idx=%zu is now out of bounds...\n", vec.length, idx);
        }

        // When idx >= vec.length, the bounds check will trigger:
        // if ((idx) >= (v)->length) LOG_FATAL(...)
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
            VecResize(&vec, 2); // Shrink to only 2 elements
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
            VecResize(&vec, 3); // Shrink so that idx=3 becomes out of bounds (valid indices: 0,1,2)
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
            VecResize(&vec, 3); // Shrink to only 3 elements
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
    size original_length = vec.length; // Capture this as 9
    VecForeachPtrInRangeIdx(&vec, val_ptr, idx, 0, original_length, {
        printf("Accessing idx %zu (vec.length=%zu): %d\n", idx, vec.length, *val_ptr);

        // When we reach idx=3, delete several elements
        if (idx == 3) {
            VecDeleteRange(&vec, 0, 6); // Remove first 6 elements
            printf(
                "Deleted first 6 elements, new length=%zu, but range ptr iteration continues to idx %zu...\n",
                vec.length,
                original_length
            );
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
            VecResize(&vec, 1); // Shrink to only 1 element
            printf("Vector resized to length %zu, but basic foreach iteration continues...\n", vec.length);
        }

        // When idx >= vec.length, the bounds check will trigger:
        // if ((idx) >= (v)->length) LOG_FATAL(...)
    });

    // Should never reach here if bounds checking triggers
    VecDeinit(&vec);
    return false;
}

// Main function that runs all deadend tests
int main(void) {
    printf("[INFO] Starting Vec.Foreach.Deadend tests\n\n");

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

    int deadend_count = sizeof(deadend_tests) / sizeof(deadend_tests[0]);

    // Run all deadend tests using the centralized test driver
    return run_test_suite(NULL, 0, deadend_tests, deadend_count, "Vec.Foreach.Deadend");
}