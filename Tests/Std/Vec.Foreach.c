#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Container/Vec.h>
#include <Misra/Std/Log.h>

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

bool test_vec_foreach_out_of_bounds_access(void);
bool test_vec_foreach_idx_out_of_bounds_access(void);
bool test_vec_foreach_idx_basic_out_of_bounds_access(void);
bool test_vec_foreach_reverse_idx_out_of_bounds_access(void);
bool test_vec_foreach_ptr_idx_out_of_bounds_access(void);
bool test_vec_foreach_ptr_reverse_idx_out_of_bounds_access(void);
bool test_vec_foreach_ptr_in_range_idx_out_of_bounds_access(void);

// Test VecForeach macro
static DefaultAllocator alloc;

bool test_vec_foreach(void) {
    WriteFmt("Testing VecForeach\n");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Add some data
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++) {
        VecPushBackR(&vec, values[i]);
    }

    // Use VecForeach to sum the values
    int sum = 0;
    VecForeach(&vec, item) {
        sum += item;
    }

    // Check the sum
    bool result = (sum == 150); // 10 + 20 + 30 + 40 + 50 = 150

    // Use VecForeach to double each value
    VecForeach(&vec, item) {
        item *= 2;
    }

    // Check that the values in the vector are unchanged (foreach uses value, not reference)
    for (u64 i = 0; i < vec.length; i++) {
        result = result && (VecAt(&vec, i) == values[i]);
    }

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Test VecForeachIdx macro
bool test_vec_foreach_idx(void) {
    WriteFmt("Testing VecForeachIdx\n");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Add some data
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++) {
        VecPushBackR(&vec, values[i]);
    }

    // Use VecForeachIdx to verify indices and values
    bool result = true;
    VecForeachIdx(&vec, item, idx) {
        result = result && (item == values[idx]);
    };

    // Use VecForeachIdx to calculate weighted sum (value * index)
    int weighted_sum = 0;
    VecForeachIdx(&vec, item, idx) {
        weighted_sum += item * idx;
    }

    // Check the weighted sum
    // 10*0 + 20*1 + 30*2 + 40*3 + 50*4 = 0 + 20 + 60 + 120 + 200 = 400
    result = result && (weighted_sum == 400);

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Test VecForeachPtr macro
bool test_vec_foreach_ptr(void) {
    WriteFmt("Testing VecForeachPtr\n");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Add some data
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++) {
        VecPushBackR(&vec, values[i]);
    }

    // Use VecForeachPtr to modify the values in the vector
    VecForeachPtr(&vec, item_ptr) {
        *item_ptr *= 2;
    }

    // Check that the values in the vector are doubled
    bool result = true;
    for (u64 i = 0; i < vec.length; i++) {
        result = result && (VecAt(&vec, i) == values[i] * 2);
    }

    // Use VecForeachPtr to calculate sum
    int sum = 0;
    VecForeachPtr(&vec, item_ptr) {
        sum += *item_ptr;
    }

    // Check the sum (should be doubled values)
    // 20 + 40 + 60 + 80 + 100 = 300
    result = result && (sum == 300);

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Test VecForeachPtrIdx macro
bool test_vec_foreach_ptr_idx(void) {
    WriteFmt("Testing VecForeachPtrIdx\n");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Add some data
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++) {
        VecPushBackR(&vec, values[i]);
    }

    // Use VecForeachPtrIdx to set each value to its index
    VecForeachPtrIdx(&vec, item_ptr, idx) {
        *item_ptr = idx;
    }

    // Check that the values in the vector are set to their indices
    bool result = true;
    for (u64 i = 0; i < vec.length; i++) {
        result = result && (VecAt(&vec, i) == i);
    }

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Test VecForeachReverse macro
bool test_vec_foreach_reverse(void) {
    WriteFmt("Testing VecForeachReverse\n");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Add some data
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++) {
        VecPushBackR(&vec, values[i]);
    }

    // Use VecForeachReverse to build a reversed array
    int reversed[5] = {0};
    int idx         = 0;
    VecForeachReverse(&vec, item) {
        reversed[idx++] = item;
    }

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
    WriteFmt("Testing VecForeachReverseIdx\n");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Add some data
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++) {
        VecPushBackR(&vec, values[i]);
    }

    // Use VecForeachReverseIdx to verify indices are correct in reverse
    bool result = true;
    VecForeachReverseIdx(&vec, item, idx) {
        result = result && (item == values[idx]);
        result = result && (VecAt(&vec, idx) == item);
    }

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Test VecForeachPtrReverse macro
bool test_vec_foreach_ptr_reverse(void) {
    WriteFmt("Testing VecForeachPtrReverse\n");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Add some data
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++) {
        VecPushBackR(&vec, values[i]);
    }

    // Use VecForeachPtrReverse to increment values in reverse order
    int increment = 1;
    VecForeachPtrReverse(&vec, item_ptr) {
        *item_ptr += increment++;
    }

    // Values should now be: [15, 24, 33, 42, 51]
    // (50+1, 40+2, 30+3, 20+4, 10+5)
    int  expected[] = {15, 24, 33, 42, 51};
    bool result     = true;
    for (size i = 0; i < vec.length; i++) {
        result = result && (VecAt(&vec, i) == expected[i]);
    }

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Test VecForeachPtrReverseIdx macro
bool test_vec_foreach_ptr_reverse_idx(void) {
    WriteFmt("Testing VecForeachPtrReverseIdx\n");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Add some data
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++) {
        VecPushBackR(&vec, values[i]);
    }

    // Use VecForeachPtrReverseIdx to set each value to its index + 100
    VecForeachPtrReverseIdx(&vec, item_ptr, idx) {
        *item_ptr = idx + 100;
    }

    // Check that the values are set correctly
    // Even though we iterate in reverse, idx represents the actual vector index
    // So: vec[4] = 104, vec[3] = 103, vec[2] = 102, vec[1] = 101, vec[0] = 100
    // Final vector: [100, 101, 102, 103, 104]
    int  expected[] = {100, 101, 102, 103, 104};
    bool result     = true;
    for (size i = 0; i < vec.length; i++) {
        result = result && (VecAt(&vec, i) == expected[i]);
    }

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Make idx go out of bounds during VecForeach by modifying vector during iteration
bool test_vec_foreach_out_of_bounds_access(void) {
    WriteFmt("Testing VecForeach where modification causes out of bounds access (should crash)\n");

    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Add some elements
    for (int i = 0; i < 5; i++) {
        VecPushBackR(&vec, i * 10);
    }

    // VecForeach doesn't use an explicit index but we can still cause issues
    int iteration_count = 0;
    VecForeach(&vec, val) {
        WriteFmt("Iteration {} (vec.length={}): {}\n", iteration_count, vec.length, val);

        // After 2nd iteration, shrink the vector dramatically
        if (iteration_count == 2) {
            VecResize(&vec, 2); // Shrink to only 2 elements
            WriteFmt("Vector resized to length {} during foreach iteration...\n", vec.length);
        }

        // This will eventually cause bounds checking to trigger
        // loop will automatically terminate

        if (iteration_count > 2) {
            LOG_ERROR("Should've terminated");
            VecDeinit(&vec);
            return false;
        }

        iteration_count++;
    }

    // Should never reach here if bounds checking triggers
    VecDeinit(&vec);
    return true;
}

// Make idx go out of bounds in VecForeachIdx by modifying vector during iteration
bool test_vec_foreach_idx_out_of_bounds_access(void) {
    WriteFmt("Testing VecForeachIdx where idx goes out of bounds (should crash)\n");

    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Add some elements
    for (int i = 0; i < 6; i++) {
        VecPushBackR(&vec, i * 20);
    }

    // VecForeachIdx has explicit bounds checking: if ((idx) >= (v)->length) LOG_FATAL(...)
    VecForeachIdx(&vec, val, idx) {
        WriteFmt("Accessing idx {} (vec.length={}): {}\n", idx, vec.length, val);

        // When we reach idx=2, drastically shrink the vector to make the current idx invalid
        // The bounds check happens after the body, so it will check if idx=2 >= new_length
        if (idx == 2) {
            VecResize(&vec, 2); // Shrink so that idx=2 becomes out of bounds (valid indices: 0,1)
            WriteFmt("Vector resized to length {}, current idx={} is now out of bounds...\n", vec.length, idx);
        }

        // When idx >= vec.length, the bounds check will trigger:
        // loop will automatically terminate

        if (idx > 2) {
            LOG_ERROR("Should've terminated");
            VecDeinit(&vec);
            return false;
        }
    }

    // Should never reach here if bounds checking triggers
    VecDeinit(&vec);
    return true;
}

// Make idx go out of bounds in VecForeachReverseIdx by modifying vector during iteration
bool test_vec_foreach_reverse_idx_out_of_bounds_access(void) {
    WriteFmt("Testing VecForeachReverseIdx where idx goes out of bounds (should crash)\n");

    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Add several elements
    for (int i = 0; i < 6; i++) {
        VecPushBackR(&vec, i * 15);
    }

    // VecForeachReverseIdx has explicit bounds checking: if ((idx) >= (v)->length) LOG_FATAL(...)
    VecForeachReverseIdx(&vec, val, idx) {
        WriteFmt("Accessing idx {} (vec.length={}): {}\n", idx, vec.length, val);

        // When we reach idx=4, drastically shrink the vector
        // This will make subsequent iterations invalid since idx will still decrement
        // but the vector length is now smaller
        if (idx == 4) {
            VecResize(&vec, 2); // Shrink to only 2 elements
            WriteFmt("Vector resized to length {} during reverse iteration...\n", vec.length);
        }

        // When idx == 4 (> vec.length = 2), the bounds check will trigger:
        // loop will automatically terminate

        if (idx < 4) {
            LOG_ERROR("Should've terminated");
            VecDeinit(&vec);
            return false;
        }
    }

    // Should never reach here if bounds checking triggers
    VecDeinit(&vec);
    return true;
}

// Make idx go out of bounds in VecForeachPtrIdx by modifying vector during iteration
bool test_vec_foreach_ptr_idx_out_of_bounds_access(void) {
    WriteFmt("Testing VecForeachPtrIdx where idx goes out of bounds (should crash)\n");

    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Add several elements
    for (int i = 0; i < 7; i++) {
        VecPushBackR(&vec, i * 25);
    }

    // VecForeachPtrIdx has explicit bounds checking: if ((idx) >= (v)->length) LOG_FATAL(...)
    VecForeachPtrIdx(&vec, val_ptr, idx) {
        WriteFmt("Accessing idx {} (vec.length={}): {}\n", idx, vec.length, *val_ptr);

        // When we reach idx=3, shrink the vector to make the CURRENT idx invalid
        // The bounds check happens after the body, so it will check if idx=3 >= new_length
        if (idx == 3) {
            VecResize(&vec, 3); // Shrink so that idx=3 becomes out of bounds (valid indices: 0,1,2)
            WriteFmt("Vector resized to length {}, current idx={} is now out of bounds...\n", vec.length, idx);
        }

        // When idx >= vec.length, the bounds check will trigger:
        // loop will automatically terminate

        if (idx > 3) {
            LOG_ERROR("Should've terminated");
            VecDeinit(&vec);
            return false;
        }
    }

    // Should never reach here if bounds checking triggers
    VecDeinit(&vec);
    return true;
}

// Make idx go out of bounds in VecForeachPtrReverseIdx by modifying vector during iteration
bool test_vec_foreach_ptr_reverse_idx_out_of_bounds_access(void) {
    WriteFmt("Testing VecForeachPtrReverseIdx where idx goes out of bounds (should crash)\n");

    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Add several elements
    for (int i = 0; i < 8; i++) {
        VecPushBackR(&vec, i * 35);
    }

    // VecForeachPtrReverseIdx has explicit bounds checking: if ((idx) >= (v)->length) LOG_FATAL(...)
    VecForeachPtrReverseIdx(&vec, val_ptr, idx) {
        WriteFmt("Accessing idx {} (vec.length={}): {}\n", idx, vec.length, *val_ptr);

        // When we reach idx=5, shrink the vector significantly
        if (idx == 5) {
            VecResize(&vec, 3); // Shrink to only 3 elements
            WriteFmt("Vector resized to length {} during reverse ptr iteration...\n", vec.length);
        }

        // When idx == 5 (> vec.length), the bounds check will trigger:
        // loop will automatically terminate

        if (idx < 5) {
            LOG_ERROR("Should've terminated");
            VecDeinit(&vec);
            return false;
        }
    }

    // Should never reach here if bounds checking triggers
    VecDeinit(&vec);
    return true;
}

// Make idx go out of bounds in VecForeachPtrInRangeIdx by modifying vector during iteration
bool test_vec_foreach_ptr_in_range_idx_out_of_bounds_access(void) {
    WriteFmt("Testing VecForeachPtrInRangeIdx where idx goes out of bounds (should crash)\n");

    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Add several elements
    for (int i = 0; i < 9; i++) {
        VecPushBackR(&vec, i * 45);
    }

    // Use VecForeachPtrInRangeIdx with a fixed range that becomes invalid when we modify the vector
    size original_length = vec.length; // Capture this as 9
    VecForeachPtrInRangeIdx(&vec, val_ptr, idx, 0, original_length) {
        WriteFmt("Accessing idx {} (vec.length={}): {}\n", idx, vec.length, *val_ptr);

        // When we reach idx=3, delete several elements
        if (idx == 3) {
            VecDeleteRange(&vec, 0, 6); // Remove first 6 elements
            WriteFmt("Deleted first 6 elements, new length={}, idx = {}\n", vec.length, original_length, idx);
        }

        // When idx >= vec.length, the bounds check will trigger:
        // loop will automatically terminate

        if (idx > vec.length) {
            LOG_ERROR("Should've terminated");
            VecDeinit(&vec);
            return false;
        }
    }

    // Should never reach here if bounds checking triggers
    VecDeinit(&vec);
    return true;
}

// Make idx go out of bounds in basic VecForeachIdx by modifying vector during iteration
bool test_vec_foreach_idx_basic_out_of_bounds_access(void) {
    WriteFmt("Testing basic VecForeachIdx where idx goes out of bounds (should crash)\n");

    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Add several elements
    for (int i = 0; i < 5; i++) {
        VecPushBackR(&vec, i * 30);
    }

    // Basic VecForeachIdx now has explicit bounds checking: if ((idx) >= (v)->length) LOG_FATAL(...)
    VecForeachIdx(&vec, val, idx) {
        WriteFmt("Accessing idx {} (vec.length={}): {}\n", idx, vec.length, val);

        // When we reach idx=2, drastically shrink the vector
        // This will make subsequent iterations invalid
        if (idx == 2) {
            VecResize(&vec, 1); // Shrink to only 1 element
            WriteFmt("Vector resized to length {}, current index={}\n", vec.length, idx);
        }

        // When idx >= vec.length, the bounds check will trigger:
        // loop will automatically terminate

        if (idx > 2) {
            LOG_ERROR("Should've terminated");
            VecDeinit(&vec);
            return false;
        }
    }

    // Should never reach here if bounds checking triggers
    VecDeinit(&vec);
    return true;
}

// Main function that runs all tests
int main(void) {
    alloc = DefaultAllocatorInit();
    WriteFmt("[INFO] Starting Vec.Foreach.Simple tests\n\n");

    // Array of normal test functions
    TestFunction tests[] = {
        test_vec_foreach,
        test_vec_foreach_idx,
        test_vec_foreach_ptr,
        test_vec_foreach_ptr_idx,
        test_vec_foreach_reverse,
        test_vec_foreach_reverse_idx,
        test_vec_foreach_ptr_reverse,
        test_vec_foreach_ptr_reverse_idx,
        test_vec_foreach_out_of_bounds_access,
        test_vec_foreach_idx_out_of_bounds_access,
        test_vec_foreach_idx_basic_out_of_bounds_access,
        test_vec_foreach_reverse_idx_out_of_bounds_access,
        test_vec_foreach_ptr_idx_out_of_bounds_access,
        test_vec_foreach_ptr_reverse_idx_out_of_bounds_access,
        test_vec_foreach_ptr_in_range_idx_out_of_bounds_access
    };

    int total_tests = sizeof(tests) / sizeof(tests[0]);

    // Run all tests using the centralized test driver
    int __rc = run_test_suite(tests, total_tests, NULL, 0, "Vec.Foreach.Simple");
    DefaultAllocatorDeinit(&alloc);
    return __rc;
}
