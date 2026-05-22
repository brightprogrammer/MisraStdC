#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Container/Vec.h>
#include <Misra/Std/Log.h>
#include <Misra/Types.h>

// Include test utilities
#include "../Util/TestRunner.h"

// Function prototypes
bool test_vec_try_reduce_space(void);
bool test_vec_resize(void);
bool test_vec_reserve(void);
bool test_vec_clear(void);

// Test VecTryReduceSpace function
bool test_vec_try_reduce_space(void) {
    WriteFmt("Testing VecTryReduceSpace\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Reserve more space than needed
    VecReserve(&vec, 100);

    // Add some data
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++) {
        VecPushBackR(&vec, values[i]);
    }

    // Original capacity should be at least 100
    // (no public capacity accessor — reading field directly)
    bool result = (vec.capacity >= 100);

    // Try to reduce space
    VecTryReduceSpace(&vec);

    // Capacity should now be closer to the actual length
    result = result && (vec.capacity < 100) && (vec.capacity >= VecLen(&vec));

    // Check that the data is still intact
    for (size i = 0; i < VecLen(&vec); i++) {
        result = result && (VecAt(&vec, i) == values[i]);
    }

    // Clean up
    VecDeinit(&vec);

    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test VecResize function
bool test_vec_resize(void) {
    WriteFmt("Testing VecResize\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Add some data
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++) {
        VecPushBackR(&vec, values[i]);
    }

    // Initial length should be 5
    bool result = (VecLen(&vec) == 5);

    // Resize to a smaller length
    VecResize(&vec, 3);

    // Length should now be 3
    result = result && (VecLen(&vec) == 3);

    // First 3 elements should be unchanged
    for (size i = 0; i < 3; i++) {
        result = result && (VecAt(&vec, i) == values[i]);
    }

    // Resize to a larger length
    VecResize(&vec, 8);

    // Length should now be 8
    result = result && (VecLen(&vec) == 8);

    // First 3 elements should still be the same
    for (size i = 0; i < 3; i++) {
        result = result && (VecAt(&vec, i) == values[i]);
    }

    // Clean up
    VecDeinit(&vec);

    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test VecReserve function
bool test_vec_reserve(void) {
    WriteFmt("Testing VecReserve\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Initial capacity should be 0
    // (no public capacity accessor — reading field directly)
    bool result = (vec.capacity == 0);

    // Reserve space for 50 elements
    VecReserve(&vec, 50);

    // Capacity should now be at least 50
    result = result && (vec.capacity >= 50);

    // Length should still be 0
    result = result && (VecLen(&vec) == 0);

    // Add some data
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++) {
        VecPushBackR(&vec, values[i]);
    }

    // Capacity should still be at least 50
    result = result && (vec.capacity >= 50);

    // Length should now be 5
    result = result && (VecLen(&vec) == 5);

    // Reserve less space (should be a no-op)
    VecReserve(&vec, 20);

    // Capacity should still be at least 50
    result = result && (vec.capacity >= 50);

    // Clean up
    VecDeinit(&vec);

    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test VecClear function
bool test_vec_clear(void) {
    WriteFmt("Testing VecClear\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Add some data
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++) {
        VecPushBackR(&vec, values[i]);
    }

    // Initial length should be 5
    bool result = (VecLen(&vec) == 5);

    // Remember the capacity (no public capacity accessor — reading field directly)
    size original_capacity = vec.capacity;

    // Clear the vector
    VecClear(&vec);

    // Length should now be 0
    result = result && (VecLen(&vec) == 0);

    // Capacity should remain the same
    result = result && (vec.capacity == original_capacity);

    // Data pointer should still be valid
    result = result && (VecBegin(&vec) != NULL);

    // Clean up
    VecDeinit(&vec);

    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Main function that runs all tests
int main(void) {
    WriteFmt("[INFO] Starting Vec.Memory tests\n\n");

    // Array of test functions
    TestFunction tests[] = {test_vec_try_reduce_space, test_vec_resize, test_vec_reserve, test_vec_clear};

    int total_tests = sizeof(tests) / sizeof(tests[0]);

    // Run all tests using the centralized test driver
    return run_test_suite(tests, total_tests, NULL, 0, "Vec.Memory");
}
