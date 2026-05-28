#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Container/Vec.h>
#include <Misra/Std/Log.h>

#include <Misra/Types.h> // For LVAL macro

// Include test utilities for deadend testing
#include "../Util/TestRunner.h"

// Function prototypes
bool test_vec_pop_back(void);
bool test_vec_pop_front(void);
bool test_vec_delete(void);
bool test_vec_delete_fast(void);
bool test_vec_delete_range(void);
bool test_vec_delete_range_fast(void);
bool test_vec_delete_last(void);
bool test_lvalue_delete_operations(void);
bool test_rvalue_delete_operations(void);
bool test_lvalue_fast_delete_operations(void);
bool test_rvalue_fast_delete_operations(void);
bool test_lvalue_delete_range_operations(void);
bool test_rvalue_delete_range_operations(void);
bool test_lvalue_fast_delete_range_operations(void);
bool test_rvalue_fast_delete_range_operations(void);

// Test VecPopBack function
static DefaultAllocator alloc;

bool test_vec_pop_back(void) {
    WriteFmtLn("Testing VecPopBack");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Add some data
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++) {
        int val = values[i];
        VecPushBack(&vec, val);
    }

    // Initial length should be 5
    bool result = (VecLen(&vec) == 5);

    // Pop from the back
    int popped;
    VecPopBack(&vec, &popped);

    // Check popped value
    result = result && (popped == 50);

    // Check new length
    result = result && (VecLen(&vec) == 4);

    // Check remaining elements
    for (u64 i = 0; i < VecLen(&vec); i++) {
        result = result && (VecAt(&vec, i) == values[i]);
    }

    // Pop again
    VecPopBack(&vec, &popped);

    // Check popped value
    result = result && (popped == 40);

    // Check new length
    result = result && (VecLen(&vec) == 3);

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Test VecPopFront function
bool test_vec_pop_front(void) {
    WriteFmtLn("Testing VecPopFront");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Add some data
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++) {
        int val = values[i];
        VecPushBack(&vec, val);
    }

    // Initial length should be 5
    bool result = (VecLen(&vec) == 5);

    // Pop from the front
    int popped;
    VecPopFront(&vec, &popped);

    // Check popped value
    result = result && (popped == 10);

    // Check new length
    result = result && (VecLen(&vec) == 4);

    // Check remaining elements
    for (u64 i = 0; i < VecLen(&vec); i++) {
        result = result && (VecAt(&vec, i) == values[i + 1]);
    }

    // Pop again
    VecPopFront(&vec, &popped);

    // Check popped value
    result = result && (popped == 20);

    // Check new length
    result = result && (VecLen(&vec) == 3);

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Test VecDelete function
bool test_vec_delete(void) {
    WriteFmtLn("Testing VecDelete");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Add some data
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++) {
        int val = values[i];
        VecPushBack(&vec, val);
    }

    // Initial length should be 5
    bool result = (VecLen(&vec) == 5);

    // Delete element at index 2 (value 30)
    VecDelete(&vec, 2);

    // Check new length
    result = result && (VecLen(&vec) == 4);

    // Check remaining elements (should be [10, 20, 40, 50])
    int expected1[] = {10, 20, 40, 50};
    for (u64 i = 0; i < VecLen(&vec); i++) {
        result = result && (VecAt(&vec, i) == expected1[i]);
    }

    // Delete element at index 0 (value 10)
    VecDelete(&vec, 0);

    // Check new length
    result = result && (VecLen(&vec) == 3);

    // Check remaining elements (should be [20, 40, 50])
    int expected2[] = {20, 40, 50};
    for (u64 i = 0; i < VecLen(&vec); i++) {
        result = result && (VecAt(&vec, i) == expected2[i]);
    }

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Test VecDeleteFast function
bool test_vec_delete_fast(void) {
    WriteFmtLn("Testing VecDeleteFast");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Add some data
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++) {
        int val = values[i];
        VecPushBack(&vec, val);
    }

    // Initial length should be 5
    bool result = (VecLen(&vec) == 5);

    // Delete element at index 1 (value 20) using fast delete
    VecDeleteFast(&vec, 1);

    // Check new length
    result = result && (VecLen(&vec) == 4);

    // With fast delete, the last element is moved to the deleted position
    // So the vector should now be [10, 50, 30, 40]
    int expected[] = {10, 50, 30, 40};
    for (u64 i = 0; i < VecLen(&vec); i++) {
        result = result && (VecAt(&vec, i) == expected[i]);
    }

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Test VecDeleteRange function
bool test_vec_delete_range(void) {
    WriteFmtLn("Testing VecDeleteRange");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Add some data
    int values[] = {10, 20, 30, 40, 50, 60, 70};
    for (int i = 0; i < 7; i++) {
        int val = values[i];
        VecPushBack(&vec, val);
    }

    // Initial length should be 7
    bool result = (VecLen(&vec) == 7);

    // Delete range from index 2 to 4 (values 30, 40, 50)
    VecDeleteRange(&vec, 2, 3);

    // Check new length
    result = result && (VecLen(&vec) == 4);

    // Check remaining elements (should be [10, 20, 60, 70])
    int expected[] = {10, 20, 60, 70};
    for (u64 i = 0; i < VecLen(&vec); i++) {
        result = result && (VecAt(&vec, i) == expected[i]);
    }

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Test VecDeleteRangeFast
bool test_vec_delete_range_fast(void) {
    WriteFmtLn("Testing VecDeleteRangeFast");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Add some data
    for (int i = 0; i < 10; i++) {
        int val = i * 10;
        VecPushBack(&vec, val);
    }

    // Initial length should be 10
    bool result = (VecLen(&vec) == 10);

    // Print before state
    WriteFmt("Before fast range delete: ");
    for (u64 i = 0; i < VecLen(&vec); i++) {
        WriteFmt("{} ", VecAt(&vec, i));
    }
    WriteFmt("\n");

    // Test VecDeleteRangeFast - delete 3 elements starting at index 2
    int start_index = 2;
    int count       = 3;

    // Remember the values that will be moved from the end
    int end_values[3];
    for (int i = 0; i < count; i++) {
        end_values[i] = VecAt(&vec, VecLen(&vec) - count + i);
    }

    VecDeleteRangeFast(&vec, start_index, count);

    // Print after state
    WriteFmt("After fast range delete: ");
    for (u64 i = 0; i < VecLen(&vec); i++) {
        WriteFmt("{} ", VecAt(&vec, i));
    }
    WriteFmt("\n");

    // Check length after deletion
    result = result && (VecLen(&vec) == 7);

    // Check that the last 3 elements moved to the deleted positions
    for (int i = 0; i < count; i++) {
        result = result && (VecAt(&vec, start_index + i) == end_values[i]);
    }

    // Verify all values that should still be present
    bool values_found[10] = {false};
    for (u64 i = 0; i < VecLen(&vec); i++) {
        int val   = VecAt(&vec, i);
        int index = val / 10;
        if (index >= 0 && index < 10) {
            values_found[index] = true;
        }
    }

    // Values 2, 3, 4 should be removed
    for (int i = 0; i < 10; i++) {
        if (i == 2 || i == 3 || i == 4) {
            result = result && !values_found[i];
        } else {
            result = result && values_found[i];
        }
    }

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Test VecDeleteLast function
bool test_vec_delete_last(void) {
    WriteFmtLn("Testing VecDeleteLast");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Add some data
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++) {
        int val = values[i];
        VecPushBack(&vec, val);
    }

    // Initial length should be 5
    bool result = (VecLen(&vec) == 5);

    // Delete the last element
    VecDeleteLast(&vec);

    // Check new length
    result = result && (VecLen(&vec) == 4);

    // Check remaining elements
    for (u64 i = 0; i < VecLen(&vec); i++) {
        result = result && (VecAt(&vec, i) == values[i]);
    }

    // Delete the last element again
    VecDeleteLast(&vec);

    // Check new length
    result = result && (VecLen(&vec) == 3);

    // Check remaining elements
    for (u64 i = 0; i < VecLen(&vec); i++) {
        result = result && (VecAt(&vec, i) == values[i]);
    }

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Test L-value standard delete operations
bool test_lvalue_delete_operations(void) {
    WriteFmtLn("Testing L-value standard delete operations");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Add some data
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++) {
        VecPushBack(&vec, values[i]);
    }

    // Initial length should be 5
    bool result = (VecLen(&vec) == 5);

    // Test L-value delete operation
    int index_to_delete = 2; // Delete 30
    VecDelete(&vec, index_to_delete);

    // Check vector after L-value deletion
    result = result && (VecLen(&vec) == 4);

    // Check remaining elements (should be [10, 20, 40, 50])
    int expected[] = {10, 20, 40, 50};
    for (u64 i = 0; i < VecLen(&vec); i++) {
        result = result && (VecAt(&vec, i) == expected[i]);
    }

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Test R-value standard delete operations
bool test_rvalue_delete_operations(void) {
    WriteFmtLn("Testing R-value standard delete operations");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Add some data
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++) {
        VecPushBack(&vec, values[i]);
    }

    // Initial length should be 5
    bool result = (VecLen(&vec) == 5);

    // Test R-value delete operation
    VecDelete(&vec, 2); // Delete 30

    // Check vector after deletion
    result = result && (VecLen(&vec) == 4);

    // Check remaining elements (should be [10, 20, 40, 50])
    int expected[] = {10, 20, 40, 50};
    for (u64 i = 0; i < VecLen(&vec); i++) {
        result = result && (VecAt(&vec, i) == expected[i]);
    }

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Test L-value fast delete operations
bool test_lvalue_fast_delete_operations(void) {
    WriteFmtLn("Testing L-value fast delete operations");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Add some data
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++) {
        VecPushBack(&vec, values[i]);
    }

    // Initial length should be 5
    bool result = (VecLen(&vec) == 5);

    // Print before state
    WriteFmt("Before L-value fast delete: ");
    for (u64 i = 0; i < VecLen(&vec); i++) {
        WriteFmt("{} ", VecAt(&vec, i));
    }
    WriteFmt("\n");

    // Test L-value fast delete operation
    int fast_index       = 2;                             // Delete 30
    int valueToBeDeleted = VecAt(&vec, fast_index);
    int lastValue        = VecAt(&vec, VecLen(&vec) - 1); // Should move to deleted position
    VecDeleteFast(&vec, fast_index);

    // Print after state
    WriteFmt("After L-value fast delete: ");
    for (u64 i = 0; i < VecLen(&vec); i++) {
        WriteFmt("{} ", VecAt(&vec, i));
    }
    WriteFmt("\n");

    // Check vector after L-value fast deletion
    result = result && (VecLen(&vec) == 4);

    // Verify the deleted value is no longer present
    bool containsValue = false;
    for (u64 i = 0; i < VecLen(&vec); i++) {
        if (VecAt(&vec, i) == valueToBeDeleted) {
            containsValue = true;
            break;
        }
    }
    result = result && !containsValue;

    // Check that the value at the deleted position is now the last value
    result = result && (VecAt(&vec, fast_index) == lastValue);
    WriteFmtLn(
        "Value at deleted position ({}) is now {} (expected {})\n",
        fast_index,
        VecAt(&vec, fast_index),
        lastValue
    );

    // Verify all expected values (except the deleted one and the moved one) are still present
    int expected_values[] = {10, 20, 40}; // 30 was deleted, 50 was moved
    for (int i = 0; i < 3; i++) {
        bool found = false;
        for (u64 j = 0; j < VecLen(&vec); j++) {
            if (VecAt(&vec, j) == expected_values[i]) {
                found = true;
                break;
            }
        }
        result = result && found;
        if (!found) {
            WriteFmtLn("Value {} should be present but was not found", expected_values[i]);
        }
    }

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Test R-value fast delete operations
bool test_rvalue_fast_delete_operations(void) {
    WriteFmtLn("Testing R-value fast delete operations");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Add some data
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++) {
        VecPushBack(&vec, values[i]);
    }

    // Initial length should be 5
    bool result = (VecLen(&vec) == 5);

    // Print before state
    WriteFmt("Before R-value fast delete: ");
    for (u64 i = 0; i < VecLen(&vec); i++) {
        WriteFmt("{} ", VecAt(&vec, i));
    }
    WriteFmt("\n");

    // Remember the value to be deleted and the last value
    int valueToBeDeleted = VecAt(&vec, 2);                // 30
    int lastValue        = VecAt(&vec, VecLen(&vec) - 1); // Should move to deleted position

    // Test R-value fast delete operation
    VecDeleteFast(&vec, 2);

    // Print after state
    WriteFmt("After R-value fast delete: ");
    for (u64 i = 0; i < VecLen(&vec); i++) {
        WriteFmt("{} ", VecAt(&vec, i));
    }
    WriteFmt("\n");

    // Check length
    result = result && (VecLen(&vec) == 4);

    // Verify the deleted value is no longer present
    bool containsValue = false;
    for (u64 i = 0; i < VecLen(&vec); i++) {
        if (VecAt(&vec, i) == valueToBeDeleted) {
            containsValue = true;
            break;
        }
    }
    result = result && !containsValue;

    // Check that the value at the deleted position is now the last value
    result = result && (VecAt(&vec, 2) == lastValue);
    WriteFmtLn("Value at deleted position (2) is now {} (expected {})\n", VecAt(&vec, 2), lastValue);

    // Verify all expected values (except the deleted one and the moved one) are still present
    int expected_values[] = {10, 20, 40}; // 30 was deleted, 50 was moved
    for (int i = 0; i < 3; i++) {
        bool found = false;
        for (u64 j = 0; j < VecLen(&vec); j++) {
            if (VecAt(&vec, j) == expected_values[i]) {
                found = true;
                break;
            }
        }
        result = result && found;
        if (!found) {
            WriteFmtLn("Value {} should be present but was not found", expected_values[i]);
        }
    }

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Test L-value delete range operations
bool test_lvalue_delete_range_operations(void) {
    WriteFmtLn("Testing L-value delete range operations");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Add some data
    int values[] = {10, 20, 30, 40, 50, 60, 70};
    for (int i = 0; i < 7; i++) {
        VecPushBack(&vec, values[i]);
    }

    // Initial length should be 7
    bool result = (VecLen(&vec) == 7);

    // Test L-value delete range operation
    int start_index = 2;
    int count       = 3;
    VecDeleteRange(&vec, start_index, count); // Delete 30, 40, 50

    // Check vector after L-value range deletion
    result = result && (VecLen(&vec) == 4);

    // Expected result: [10, 20, 60, 70]
    int expected[] = {10, 20, 60, 70};
    for (u64 i = 0; i < VecLen(&vec); i++) {
        result = result && (VecAt(&vec, i) == expected[i]);
    }

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Test R-value delete range operations
bool test_rvalue_delete_range_operations(void) {
    WriteFmtLn("Testing R-value delete range operations");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Add some data
    int values[] = {10, 20, 30, 40, 50, 60, 70};
    for (int i = 0; i < 7; i++) {
        VecPushBack(&vec, values[i]);
    }

    // Initial length should be 7
    bool result = (VecLen(&vec) == 7);

    // Test R-value delete range operation
    VecDeleteRange(&vec, 2, 3); // Delete 30, 40, 50

    // Check vector after R-value range deletion
    result = result && (VecLen(&vec) == 4);

    // Expected result: [10, 20, 60, 70]
    int expected[] = {10, 20, 60, 70};
    for (u64 i = 0; i < VecLen(&vec); i++) {
        result = result && (VecAt(&vec, i) == expected[i]);
    }

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Test L-value fast delete range operations
bool test_lvalue_fast_delete_range_operations(void) {
    WriteFmtLn("Testing L-value fast delete range operations");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Add some data
    int values[] = {10, 20, 30, 40, 50, 60, 70};
    for (int i = 0; i < 7; i++) {
        VecPushBack(&vec, values[i]);
    }

    // Initial length should be 7
    bool result = (VecLen(&vec) == 7);

    // Print before state
    WriteFmt("Before L-value fast range delete: ");
    for (u64 i = 0; i < VecLen(&vec); i++) {
        WriteFmt("{} ", VecAt(&vec, i));
    }
    WriteFmt("\n");

    // Values that should be deleted (30, 40, 50)
    int valuesToDelete[] = {values[2], values[3], values[4]};

    // Test L-value fast delete range operation
    int fast_start = 2;
    int fast_count = 3;
    VecDeleteRangeFast(&vec, fast_start, fast_count);

    // Print after state
    WriteFmt("After L-value fast range delete: ");
    for (u64 i = 0; i < VecLen(&vec); i++) {
        WriteFmt("{} ", VecAt(&vec, i));
    }
    WriteFmt("\n");

    // Check vector after L-value fast range deletion
    result = result && (VecLen(&vec) == 4);

    // Verify the deleted values are no longer present
    for (int i = 0; i < 3; i++) {
        bool found = false;
        for (u64 j = 0; j < VecLen(&vec); j++) {
            if (VecAt(&vec, j) == valuesToDelete[i]) {
                found = true;
                break;
            }
        }
        result = result && !found;
        if (found) {
            WriteFmtLn("Value {} should be deleted but was found", valuesToDelete[i]);
        }
    }

    // Verify all other values are still present
    int remainingValues[] = {10, 20, 60, 70};
    for (int i = 0; i < 4; i++) {
        bool found = false;
        for (u64 j = 0; j < VecLen(&vec); j++) {
            if (VecAt(&vec, j) == remainingValues[i]) {
                found = true;
                break;
            }
        }
        result = result && found;
        if (!found) {
            WriteFmtLn("Value {} should be present but was not found", remainingValues[i]);
        }
    }

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Test R-value fast delete range operations
bool test_rvalue_fast_delete_range_operations(void) {
    WriteFmtLn("Testing R-value fast delete range operations");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Add some data
    int values[] = {10, 20, 30, 40, 50, 60, 70};
    for (int i = 0; i < 7; i++) {
        VecPushBack(&vec, values[i]);
    }

    // Initial length should be 7
    bool result = (VecLen(&vec) == 7);

    // Print before state
    WriteFmt("Before R-value fast range delete: ");
    for (u64 i = 0; i < VecLen(&vec); i++) {
        WriteFmt("{} ", VecAt(&vec, i));
    }
    WriteFmt("\n");

    // Values that should be deleted (30, 40, 50)
    int valuesToDelete[] = {values[2], values[3], values[4]};

    // Test R-value fast delete range operation
    VecDeleteRangeFast(&vec, 2, 3);

    // Print after state
    WriteFmt("After R-value fast range delete: ");
    for (u64 i = 0; i < VecLen(&vec); i++) {
        WriteFmt("{} ", VecAt(&vec, i));
    }
    WriteFmt("\n");

    // Check vector after R-value fast range deletion
    result = result && (VecLen(&vec) == 4);

    // Verify the deleted values are no longer present
    for (int i = 0; i < 3; i++) {
        bool found = false;
        for (u64 j = 0; j < VecLen(&vec); j++) {
            if (VecAt(&vec, j) == valuesToDelete[i]) {
                found = true;
                break;
            }
        }
        result = result && !found;
        if (found) {
            WriteFmtLn("Value {} should be deleted but was found", valuesToDelete[i]);
        }
    }

    // Verify all other values are still present
    int remainingValues[] = {10, 20, 60, 70};
    for (int i = 0; i < 4; i++) {
        bool found = false;
        for (u64 j = 0; j < VecLen(&vec); j++) {
            if (VecAt(&vec, j) == remainingValues[i]) {
                found = true;
                break;
            }
        }
        result = result && found;
        if (!found) {
            WriteFmtLn("Value {} should be present but was not found", remainingValues[i]);
        }
    }

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Main function that runs all tests
int main(void) {
    // Array of normal test functions
    TestFunction normal_tests[] = {
        test_vec_pop_back,
        test_vec_pop_front,
        test_vec_delete,
        test_vec_delete_fast,
        test_vec_delete_range,
        test_vec_delete_range_fast,
        test_vec_delete_last,
        test_lvalue_delete_operations,
        test_rvalue_delete_operations,
        test_lvalue_fast_delete_operations,
        test_rvalue_fast_delete_operations,
        test_lvalue_delete_range_operations,
        test_rvalue_delete_range_operations,
        test_lvalue_fast_delete_range_operations,
        test_rvalue_fast_delete_range_operations
    };

    int normal_count = sizeof(normal_tests) / sizeof(normal_tests[0]);

    alloc    = DefaultAllocatorInit();
    int rc = run_test_suite(normal_tests, normal_count, NULL, 0, "Vec.Remove");
    DefaultAllocatorDeinit(&alloc);
    return rc;
}
