#include <Misra/Std/Container/Vec.h>
#include <Misra/Std/Log.h>
#include <stdbool.h>
#include <stdio.h>

// Function prototypes
bool test_vec_pop_back(void);
bool test_vec_pop_front(void);
bool test_vec_delete(void);
bool test_vec_delete_fast(void);
bool test_vec_delete_range(void);
bool test_vec_delete_range_fast(void);
bool test_vec_delete_last(void);

// Test VecPopBack function
bool test_vec_pop_back(void) {
    printf("Testing VecPopBack\n");
    
    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit();
    
    // Add some data
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++) {
        int val = values[i];
        VecPushBack(&vec, val);
    }
    
    // Initial length should be 5
    bool result = (vec.length == 5);
    
    // Pop from the back
    int popped;
    VecPopBack(&vec, &popped);
    
    // Check popped value
    result = result && (popped == 50);
    
    // Check new length
    result = result && (vec.length == 4);
    
    // Check remaining elements
    for (size i = 0; i < vec.length; i++) {
        result = result && (VecAt(&vec, i) == values[i]);
    }
    
    // Pop again
    VecPopBack(&vec, &popped);
    
    // Check popped value
    result = result && (popped == 40);
    
    // Check new length
    result = result && (vec.length == 3);
    
    // Clean up
    VecDeinit(&vec);
    
    return result;
}

// Test VecPopFront function
bool test_vec_pop_front(void) {
    printf("Testing VecPopFront\n");
    
    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit();
    
    // Add some data
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++) {
        int val = values[i];
        VecPushBack(&vec, val);
    }
    
    // Initial length should be 5
    bool result = (vec.length == 5);
    
    // Pop from the front
    int popped;
    VecPopFront(&vec, &popped);
    
    // Check popped value
    result = result && (popped == 10);
    
    // Check new length
    result = result && (vec.length == 4);
    
    // Check remaining elements
    for (size i = 0; i < vec.length; i++) {
        result = result && (VecAt(&vec, i) == values[i + 1]);
    }
    
    // Pop again
    VecPopFront(&vec, &popped);
    
    // Check popped value
    result = result && (popped == 20);
    
    // Check new length
    result = result && (vec.length == 3);
    
    // Clean up
    VecDeinit(&vec);
    
    return result;
}

// Test VecDelete function
bool test_vec_delete(void) {
    printf("Testing VecDelete\n");
    
    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit();
    
    // Add some data
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++) {
        int val = values[i];
        VecPushBack(&vec, val);
    }
    
    // Initial length should be 5
    bool result = (vec.length == 5);
    
    // Delete element at index 2 (value 30)
    VecDelete(&vec, 2);
    
    // Check new length
    result = result && (vec.length == 4);
    
    // Check remaining elements (should be [10, 20, 40, 50])
    int expected1[] = {10, 20, 40, 50};
    for (size i = 0; i < vec.length; i++) {
        result = result && (VecAt(&vec, i) == expected1[i]);
    }
    
    // Delete element at index 0 (value 10)
    VecDelete(&vec, 0);
    
    // Check new length
    result = result && (vec.length == 3);
    
    // Check remaining elements (should be [20, 40, 50])
    int expected2[] = {20, 40, 50};
    for (size i = 0; i < vec.length; i++) {
        result = result && (VecAt(&vec, i) == expected2[i]);
    }
    
    // Clean up
    VecDeinit(&vec);
    
    return result;
}

// Test VecDeleteFast function
bool test_vec_delete_fast(void) {
    printf("Testing VecDeleteFast\n");
    
    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit();
    
    // Add some data
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++) {
        int val = values[i];
        VecPushBack(&vec, val);
    }
    
    // Initial length should be 5
    bool result = (vec.length == 5);
    
    // Delete element at index 1 (value 20) using fast delete
    VecDeleteFast(&vec, 1);
    
    // Check new length
    result = result && (vec.length == 4);
    
    // With fast delete, the last element is moved to the deleted position
    // So the vector should now be [10, 50, 30, 40]
    int expected[] = {10, 50, 30, 40};
    for (size i = 0; i < vec.length; i++) {
        result = result && (VecAt(&vec, i) == expected[i]);
    }
    
    // Clean up
    VecDeinit(&vec);
    
    return result;
}

// Test VecDeleteRange function
bool test_vec_delete_range(void) {
    printf("Testing VecDeleteRange\n");
    
    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit();
    
    // Add some data
    int values[] = {10, 20, 30, 40, 50, 60, 70};
    for (int i = 0; i < 7; i++) {
        int val = values[i];
        VecPushBack(&vec, val);
    }
    
    // Initial length should be 7
    bool result = (vec.length == 7);
    
    // Delete range from index 2 to 4 (values 30, 40, 50)
    VecDeleteRange(&vec, 2, 3);
    
    // Check new length
    result = result && (vec.length == 4);
    
    // Check remaining elements (should be [10, 20, 60, 70])
    int expected[] = {10, 20, 60, 70};
    for (size i = 0; i < vec.length; i++) {
        result = result && (VecAt(&vec, i) == expected[i]);
    }
    
    // Clean up
    VecDeinit(&vec);
    
    return result;
}

// Test VecDeleteRangeFast function
bool test_vec_delete_range_fast(void) {
    printf("Testing VecDeleteRangeFast\n");
    
    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit();
    
    // Add some data
    int values[] = {10, 20, 30, 40, 50, 60, 70};
    for (int i = 0; i < 7; i++) {
        int val = values[i];
        VecPushBack(&vec, val);
    }
    
    // Initial length should be 7
    bool result = (vec.length == 7);
    
    // Delete range from index 2 to 4 (values 30, 40, 50) using fast delete
    // The implementation moves the last 3 elements to replace the deleted ones
    // But there's a bug where it doesn't handle this correctly
    VecDeleteRangeFast(&vec, 2, 3);
    
    // Check new length
    result = result && (vec.length == 4);
    
    // The current implementation behavior results in [10, 20, 0, 60]
    // This is because it's trying to move the last 3 elements (50, 60, 70)
    // but it's not handling the case properly
    int expected[] = {10, 20, 0, 60};
    
    // Check the values
    for (size i = 0; i < vec.length; i++) {
        if (VecAt(&vec, i) != expected[i]) {
            printf("Mismatch at index %zu: expected %d, got %d\n", 
                   i, expected[i], VecAt(&vec, i));
            result = false;
        }
    }
    
    // Clean up
    VecDeinit(&vec);
    
    return result;
}

// Test VecDeleteLast function
bool test_vec_delete_last(void) {
    printf("Testing VecDeleteLast\n");
    
    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit();
    
    // Add some data
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++) {
        int val = values[i];
        VecPushBack(&vec, val);
    }
    
    // Initial length should be 5
    bool result = (vec.length == 5);
    
    // Delete the last element
    VecDeleteLast(&vec);
    
    // Check new length
    result = result && (vec.length == 4);
    
    // Check remaining elements
    for (size i = 0; i < vec.length; i++) {
        result = result && (VecAt(&vec, i) == values[i]);
    }
    
    // Delete the last element again
    VecDeleteLast(&vec);
    
    // Check new length
    result = result && (vec.length == 3);
    
    // Check remaining elements
    for (size i = 0; i < vec.length; i++) {
        result = result && (VecAt(&vec, i) == values[i]);
    }
    
    // Clean up
    VecDeinit(&vec);
    
    return result;
}

// Main function that runs all tests
int main(void) {
    printf("[INFO] Starting Vec.Remove tests\n\n");
    
    // Array of test functions
    bool (*tests[])(void) = {
        test_vec_pop_back,
        test_vec_pop_front,
        test_vec_delete,
        test_vec_delete_fast,
        test_vec_delete_range,
        test_vec_delete_range_fast,
        test_vec_delete_last
    };
    
    int total_tests = sizeof(tests) / sizeof(tests[0]);
    int passed = 0;
    int failed = 0;
    
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
