#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Container/Vec.h>
#include <Misra/Std/Log.h>
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
bool test_vec_init_clone_inherits_allocator_config(void);
bool test_lvalue_rvalue_operations(void);
bool test_lvalue_memset_after_insertion(void);

// Test VecPushBack function
static DefaultAllocator alloc;

bool test_vec_push_back(void) {
    WriteFmt("Testing VecPushBack\n");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Push some elements to the back
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++) {
        VecPushBackR(&vec, values[i]);
    }

    // Check length
    bool result = (VecLen(&vec) == 5);

    // Check elements in order
    for (size i = 0; i < VecLen(&vec); i++) {
        result = result && (VecAt(&vec, i) == values[i]);
    }

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Test VecPushFront function
bool test_vec_push_front(void) {
    WriteFmt("Testing VecPushFront\n");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Push some elements to the front
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++) {
        VecPushFrontR(&vec, values[i]);
    }

    // Check length
    bool result = (VecLen(&vec) == 5);

    // Check elements in reverse order (since we pushed to front)
    for (size i = 0; i < VecLen(&vec); i++) {
        result = result && (VecAt(&vec, i) == values[4 - i]);
    }

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Test VecInsert function
bool test_vec_insert(void) {
    WriteFmt("Testing VecInsert\n");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Insert at index 0 (empty vector)
    VecInsertR(&vec, 10, 0);

    // Check first element
    bool result = (VecLen(&vec) == 1 && VecAt(&vec, 0) == 10);

    // Insert at the end
    VecInsertR(&vec, 30, 1);

    // Check elements
    result = result && (VecLen(&vec) == 2 && VecAt(&vec, 0) == 10 && VecAt(&vec, 1) == 30);

    // Insert in the middle
    VecInsertR(&vec, 20, 1);

    // Check all elements
    result = result && (VecLen(&vec) == 3);
    result = result && (VecAt(&vec, 0) == 10);
    result = result && (VecAt(&vec, 1) == 20);
    result = result && (VecAt(&vec, 2) == 30);

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Test VecPushBackArr function
bool test_vec_push_back_arr(void) {
    WriteFmt("Testing VecPushBackArr\n");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Push an array to the back
    int values[] = {10, 20, 30, 40, 50};
    VecPushBackArrR(&vec, values, 5);

    // Check length
    bool result = (VecLen(&vec) == 5);

    // Check elements in order
    for (size i = 0; i < VecLen(&vec); i++) {
        result = result && (VecAt(&vec, i) == values[i]);
    }

    // Push another array to the back
    int more_values[] = {60, 70, 80};
    VecPushBackArrR(&vec, more_values, 3);

    // Check length
    result = result && (VecLen(&vec) == 8);

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
    WriteFmt("Testing VecPushFrontArr\n");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Push an array to the front of empty vector
    int values[] = {10, 20, 30, 40, 50};
    VecPushFrontArrR(&vec, values, 5);

    // Check length
    bool result = (VecLen(&vec) == 5);

    // Check elements in order
    for (size i = 0; i < VecLen(&vec); i++) {
        result = result && (VecAt(&vec, i) == values[i]);
    }

    // Push another array to the front
    int more_values[] = {60, 70, 80};
    VecPushFrontArrR(&vec, more_values, 3);

    // Check length
    result = result && (VecLen(&vec) == 8);

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
    WriteFmt("Testing VecInsertRange at specific index\n");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Push some elements first
    VecPushBackR(&vec, 10);
    VecPushBackR(&vec, 20);

    // Push an array at a specific index
    int values[] = {30, 40, 50};
    VecInsertRangeR(&vec, values, 1, 3);

    // Check length
    bool result = (VecLen(&vec) == 5);

    // Expected result: [10, 30, 40, 50, 20]
    int expected[] = {10, 30, 40, 50, 20};

    // Check all elements
    for (size i = 0; i < VecLen(&vec); i++) {
        result = result && (VecAt(&vec, i) == expected[i]);
    }

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Test VecInsertRange function for inserting from another vector
bool test_vec_insert_range(void) {
    WriteFmt("Testing VecInsertRange from another vector\n");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Add some initial elements
    int initial[] = {10, 20, 30};
    VecPushBackArrR(&vec, initial, 3);

    // Create another vector with elements to insert
    IntVec src          = VecInit(&alloc);
    int    src_values[] = {40, 50, 60};
    VecPushBackArrR(&src, src_values, 3);

    // Insert range in the middle
    VecInsertRangeR(&vec, VecBegin(&src), 1, VecLen(&src));

    // Check length
    bool result = (VecLen(&vec) == 6);

    // Expected result: [10, 40, 50, 60, 20, 30]
    int expected[] = {10, 40, 50, 60, 20, 30};

    // Check all elements
    for (size i = 0; i < VecLen(&vec); i++) {
        result = result && (VecAt(&vec, i) == expected[i]);
    }

    // Clean up
    VecDeinit(&vec);
    VecDeinit(&src);

    return result;
}

// Test VecMerge function
bool test_vec_merge(void) {
    WriteFmt("Testing VecMerge\n");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec1 = VecInit(&alloc);

    // Add some elements to first vector
    int values1[] = {10, 20, 30};
    VecPushBackArrR(&vec1, values1, 3);

    // Create second vector
    IntVec vec2 = VecInit(&alloc);

    // Add some elements to second vector
    int values2[] = {40, 50, 60};
    VecPushBackArrR(&vec2, values2, 3);

    // Merge vec2 into vec1
    VecMergeR(&vec1, &vec2);

    // Check lengths
    bool result = (VecLen(&vec1) == 6);
    result      = result && (VecLen(&vec2) == 3); // VecMergeR doesn't modify source vector

    // Expected result in vec1: [10, 20, 30, 40, 50, 60]
    int expected[] = {10, 20, 30, 40, 50, 60};

    // Check all elements in vec1
    for (size i = 0; i < VecLen(&vec1); i++) {
        result = result && (VecAt(&vec1, i) == expected[i]);
    }

    // Clean up
    VecDeinit(&vec1);
    VecDeinit(&vec2);

    return result;
}

// Test that a manually-cloned Vec shares its allocator configuration.
// (Originally exercised `VecInitClone`, which is currently unavailable; we
// emulate the clone by reusing the source's allocator pointer and copying
// the elements manually.)
bool test_vec_init_clone_inherits_allocator_config(void) {
    WriteFmt("Testing manual clone allocator inheritance\n");

    typedef Vec(int) IntVec;

    HeapAllocator local_heap    = HeapAllocatorInit();
    local_heap.base.effort      = ALLOCATOR_EFFORT_RETRY_FALLBACK;
    local_heap.base.retry_limit = 11;

    IntVec src      = VecInit(&local_heap);
    int    values[] = {10, 20, 30};
    VecPushBackArrR(&src, values, 3);

    // Build dst on the SAME allocator as src, then clone the data.
    // VecPushBackArrR treats the source as a flat C array of elements,
    // so alignment must be 1 (default) for src.data to be a contiguous
    // int[]. Stronger alignment is exercised separately - it's not what
    // this test is asserting.
    IntVec dst      = VecInit(VecAllocator(&src));
    // intentional bypass: testing hook propagation; no public VecSetCopyHooks mutator exists
    dst.copy_init   = src.copy_init;
    dst.copy_deinit = src.copy_deinit;
    bool cloned     = VecPushBackArrR(&dst, VecBegin(&src), VecLen(&src));

    bool allocator_matches = VecAllocator(&dst) == VecAllocator(&src);

    bool result = cloned && VecCopyInit(&dst) == VecCopyInit(&src) && VecCopyDeinit(&dst) == VecCopyDeinit(&src) &&
                  VecAllocator(&dst)->effort == ALLOCATOR_EFFORT_RETRY_FALLBACK &&
                  VecAllocator(&dst)->retry_limit == 11 && allocator_matches && VecLen(&src) == 3 &&
                  VecAt(&src, 0) == 10 && VecAt(&src, 1) == 20 && VecAt(&src, 2) == 30 && VecLen(&dst) == 3 &&
                  VecAt(&dst, 0) == 10 && VecAt(&dst, 1) == 20 && VecAt(&dst, 2) == 30;

    VecDeinit(&src);
    VecDeinit(&dst);
    HeapAllocatorDeinit(&local_heap);
    return result;
}

// Test L-value and R-value operations
bool test_lvalue_rvalue_operations(void) {
    WriteFmt("Testing L-value and R-value operations\n");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Test R-value insert operations
    VecPushBackR(&vec, LVAL(42));

    // Check that the element was added
    bool result = (VecLen(&vec) == 1 && VecAt(&vec, 0) == 42);

    // Test L-value insert operations
    int l_value = 100;
    VecPushBackL(&vec, l_value);

    // Check that the element was added
    result = result && (VecLen(&vec) == 2 && VecAt(&vec, 1) == 100);

    // Test R-value insert at index
    VecInsertR(&vec, LVAL(50), 1);

    // Check that the element was inserted
    result = result && (VecLen(&vec) == 3);
    result = result && (VecAt(&vec, 0) == 42);
    result = result && (VecAt(&vec, 1) == 50);
    result = result && (VecAt(&vec, 2) == 100);

    // Test L-value insert at index
    int insert_value = 75;
    VecInsertL(&vec, insert_value, 2);

    // Check that the element was inserted
    result = result && (VecLen(&vec) == 4);
    result = result && (VecAt(&vec, 0) == 42);
    result = result && (VecAt(&vec, 1) == 50);
    result = result && (VecAt(&vec, 2) == 75);
    result = result && (VecAt(&vec, 3) == 100);

    // Test R-value fast insert
    VecInsertFastR(&vec, LVAL(60), 1);

    // Check that the element was inserted
    result = result && (VecLen(&vec) == 5);
    result = result && (VecAt(&vec, 1) == 60);

    // Test L-value fast insert
    int fast_value = 80;
    VecInsertFastL(&vec, fast_value, 3);

    // Check that the element was inserted
    result = result && (VecLen(&vec) == 6);
    result = result && (VecAt(&vec, 3) == 80);

    // Test array operations with L-values and R-values
    int arr[] = {200, 300, 400};

    // R-value array operations
    VecPushBackArrR(&vec, arr, 3);

    // Check that the elements were added
    result = result && (VecLen(&vec) == 9);
    result = result && (VecAt(&vec, 6) == 200);
    result = result && (VecAt(&vec, 7) == 300);
    result = result && (VecAt(&vec, 8) == 400);

    // L-value array operations
    VecPushFrontArrL(&vec, arr, 3);

    // Check that the elements were added
    result = result && (VecLen(&vec) == 12);
    result = result && (VecAt(&vec, 0) == 200);
    result = result && (VecAt(&vec, 1) == 300);
    result = result && (VecAt(&vec, 2) == 400);

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Test that L-value insertions properly memset values to 0 after insertion
bool test_lvalue_memset_after_insertion(void) {
    WriteFmt("Testing L-value memset after insertion\n");

    // Create a vector of integers without copy_init
    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

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
    IntVec vec2         = VecInit(&alloc);
    int    merge_vals[] = {140, 150, 160};
    for (int i = 0; i < 3; i++) {
        VecPushBackR(&vec2, merge_vals[i]);
    }

    // Merge with L-value semantics
    VecMergeL(&vec, &vec2);

    // Check that the source vector is cleared
    result = result && (VecLen(&vec2) == 0);
    result = result && (VecBegin(&vec2) == NULL);

    // Clean up
    VecDeinit(&vec);
    VecDeinit(&vec2);

    return result;
}

// Main function that runs all tests
int main(void) {
    alloc = DefaultAllocatorInit();
    WriteFmt("[INFO] Starting Vec.Insert tests\n\n");

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
        test_vec_init_clone_inherits_allocator_config,
        test_lvalue_rvalue_operations,
        test_lvalue_memset_after_insertion
    };

    int total_tests = sizeof(tests) / sizeof(tests[0]);

    // Run all tests using the centralized test driver
    int __rc = run_test_suite(tests, total_tests, NULL, 0, "Vec.Insert");
    DefaultAllocatorDeinit(&alloc);
    return __rc;
}
