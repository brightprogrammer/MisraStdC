#include <Misra/Std/Container/Vec.h>
#include <Misra/Std/Log.h>
#include <stdbool.h>
#include <stdio.h>
#include <Misra/Types.h> // For LVAL macro

// Define a test struct for our vector tests
typedef struct {
    int   id;
    float value;
} TestItem;

// Define custom copy init and deinit functions for TestItem
bool TestItemCopyInit(TestItem* dst, TestItem* src) {
    if (!dst || !src)
        return false;
    dst->id    = src->id;
    dst->value = src->value;
    return true;
}

void TestItemDeinit(TestItem* item) {
    if (!item)
        return;
    // Nothing to free in this simple struct
    // But in real cases with pointers, we would free them here
}

// Function prototypes
bool test_vec_init_basic(void);
bool test_vec_init_aligned(void);
bool test_vec_init_with_deep_copy(void);
bool test_vec_init_aligned_with_deep_copy(void);
bool test_vec_init_stack(void);
bool test_vec_init_clone(void);

// Test basic vector initialization
bool test_vec_init_basic(void) {
    printf("Testing VecInit\n");

    // Test with int type
    typedef Vec(int) IntVec;
    IntVec vec = VecInit();

    // Check initial state
    bool result =
        (vec.length == 0 && vec.capacity == 0 && vec.data == NULL && vec.alignment == 1 && vec.copy_init == NULL &&
         vec.copy_deinit == NULL);

    // Clean up
    VecDeinit(&vec);

    // Test with struct type
    typedef Vec(TestItem) TestVec;
    TestVec test_vec = VecInit();

    // Check initial state
    result = result && (test_vec.length == 0 && test_vec.capacity == 0 && test_vec.data == NULL &&
                        test_vec.alignment == 1 && test_vec.copy_init == NULL && test_vec.copy_deinit == NULL);

    // Clean up
    VecDeinit(&test_vec);

    return result;
}

// Test aligned vector initialization
bool test_vec_init_aligned(void) {
    printf("Testing VecInitAligned\n");

    // Test with int type and 4-byte alignment
    typedef Vec(int) IntVec;
    IntVec vec = VecInitAligned(4);

    // Check initial state
    bool result =
        (vec.length == 0 && vec.capacity == 0 && vec.data == NULL && vec.alignment == 4 && vec.copy_init == NULL &&
         vec.copy_deinit == NULL);

    // Clean up
    VecDeinit(&vec);

    // Test with struct type and 16-byte alignment
    typedef Vec(TestItem) TestVec;
    TestVec test_vec = VecInitAligned(16);

    // Check initial state
    result = result && (test_vec.length == 0 && test_vec.capacity == 0 && test_vec.data == NULL &&
                        test_vec.alignment == 16 && test_vec.copy_init == NULL && test_vec.copy_deinit == NULL);

    // Clean up
    VecDeinit(&test_vec);

    return result;
}

// Test vector initialization with deep copy functions
bool test_vec_init_with_deep_copy(void) {
    printf("Testing VecInitWithDeepCopy\n");

    // Test with struct type and custom copy/deinit functions
    typedef Vec(TestItem) TestVec;
    TestVec vec = VecInitWithDeepCopy(TestItemCopyInit, TestItemDeinit);

    // Check initial state
    bool result =
        (vec.length == 0 && vec.capacity == 0 && vec.data == NULL && vec.alignment == 1 &&
         vec.copy_init == (GenericCopyInit)TestItemCopyInit && vec.copy_deinit == (GenericCopyDeinit)TestItemDeinit);

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Test vector initialization with alignment and deep copy functions
bool test_vec_init_aligned_with_deep_copy(void) {
    printf("Testing VecInitAlignedWithDeepCopy\n");

    // Test with struct type, custom copy/deinit functions, and 8-byte alignment
    typedef Vec(TestItem) TestVec;
    TestVec vec = VecInitAlignedWithDeepCopy(TestItemCopyInit, TestItemDeinit, 8);

    // Check initial state
    bool result =
        (vec.length == 0 && vec.capacity == 0 && vec.data == NULL && vec.alignment == 8 &&
         vec.copy_init == (GenericCopyInit)TestItemCopyInit && vec.copy_deinit == (GenericCopyDeinit)TestItemDeinit);

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Test vector stack initialization
bool test_vec_init_stack(void) {
    printf("Testing VecInitStack\n");

    bool result = true;

    // Test with basic int type
    typedef Vec(int) IntVec;
    IntVec vec;

    VecInitStack(vec, 10, {
        // Inside the scope where the stack vector is valid

        // Check initial state
        if (vec.length != 0 || vec.capacity != 10 || vec.data == NULL || vec.alignment != 1 || vec.copy_init != NULL ||
            vec.copy_deinit != NULL) {
            result = false;
        }

        // Add some data
        VecPushBackR(&vec, 10);
        VecPushBackR(&vec, 20);
        VecPushBackR(&vec, 30);

        // Check that the data was added correctly
        if (vec.length != 3 || VecAt(&vec, 0) != 10 || VecAt(&vec, 1) != 20 || VecAt(&vec, 2) != 30) {
            result = false;
        }

        // No need to call VecDeinit for stack-based vectors
    });

    // After the scope, vec should be zeroed out
    if (vec.data != NULL || vec.length != 0 || vec.capacity != 0) {
        result = false;
    }

    // Test with struct type
    typedef Vec(TestItem) TestVec;
    TestVec test_vec;

    VecInitStack(test_vec, 5, {
        // Inside the scope where the stack vector is valid

        // Check initial state
        if (test_vec.length != 0 || test_vec.capacity != 5 || test_vec.data == NULL || test_vec.alignment != 1 ||
            test_vec.copy_init != NULL || test_vec.copy_deinit != NULL) {
            result = false;
        }

        // Add a test item
        TestItem item = {0};
        item.id       = 1;
        item.value    = 3.14;
        VecPushBackR(&test_vec, item);

        // Check that the item was added correctly
        if (test_vec.length != 1 || VecAt(&test_vec, 0).id != 1 || VecAt(&test_vec, 0).value != 3.14f) {
            result = false;
        }

        // No need to call VecDeinit for stack-based vectors
    });

    // After the scope, test_vec should be zeroed out
    if (test_vec.data != NULL || test_vec.length != 0 || test_vec.capacity != 0) {
        result = false;
    }

    return result;
}

// Test vector clone initialization
bool test_vec_init_clone(void) {
    printf("Testing vector cloning\n");

    // Create a source vector
    typedef Vec(int) IntVec;
    IntVec src = VecInit();

    // Add some data
    VecPushBackR(&src, 1);
    VecPushBackR(&src, 2);
    VecPushBackR(&src, 3);

    // Create a destination vector
    IntVec clone = VecInit();

    // Clone the source vector into the destination
    VecPushBackArrR(&clone, src.data, src.length);

    // Check that the clone has the same data but different memory
    bool result =
        (clone.length == src.length && clone.capacity >= src.length && clone.data != src.data &&
         clone.alignment == src.alignment);

    // Check the actual data
    if (result) {
        for (size i = 0; i < src.length; i++) {
            if (VecAt(&clone, i) != VecAt(&src, i)) {
                result = false;
                break;
            }
        }
    }

    // Clean up
    VecDeinit(&src);
    VecDeinit(&clone);

    return result;
}

// Main function that runs all tests
int main(void) {
    printf("[INFO] Starting Vec.Init tests\n\n");

    // Array of test functions
    bool (*tests[])(void) = {
        test_vec_init_basic,
        test_vec_init_aligned,
        test_vec_init_with_deep_copy,
        test_vec_init_aligned_with_deep_copy,
        test_vec_init_stack,
        test_vec_init_clone
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
