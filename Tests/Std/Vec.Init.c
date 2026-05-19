#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Container/Vec.h>
#include <Misra/Std/Log.h>

#include <Misra/Types.h> // For LVAL macro

// Include test utilities
#include "../Util/TestRunner.h"

// Define a test struct for our vector tests
typedef struct {
    int   id;
    float value;
} TestItem;

// Define custom copy init and deinit functions for TestItem
bool TestItemCopyInit(TestItem *dst, TestItem *src) {
    if (!dst || !src)
        return false;
    dst->id    = src->id;
    dst->value = src->value;
    return true;
}

void TestItemDeinit(TestItem *item) {
    if (!item)
        return;
    // Nothing to free in this simple struct
    // But in real cases with pointers, we would free them here
}

static DefaultAllocator alloc;

// Function prototypes
bool test_vec_init_basic(void);
bool test_vec_init_aligned(void);
bool test_vec_init_with_deep_copy(void);
bool test_vec_init_aligned_with_deep_copy(void);
bool test_vec_init_optional_allocator(void);
bool test_vec_init_stack(void);
bool test_vec_init_clone(void);

// Test basic vector initialization
bool test_vec_init_basic(void) {
    WriteFmt("Testing VecInit\n");

    // Test with int type
    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Check initial state
    bool result =
        (vec.length == 0 && vec.capacity == 0 && vec.data == NULL && vec.allocator->alignment == 1 &&
         vec.copy_init == NULL && vec.copy_deinit == NULL);

    // Clean up
    VecDeinit(&vec);

    // Test with struct type
    typedef Vec(TestItem) TestVec;
    TestVec test_vec = VecInit(&alloc);

    // Check initial state
    result =
        result && (test_vec.length == 0 && test_vec.capacity == 0 && test_vec.data == NULL &&
                   test_vec.allocator->alignment == 1 && test_vec.copy_init == NULL && test_vec.copy_deinit == NULL);

    // Clean up
    VecDeinit(&test_vec);

    return result;
}

// Test aligned vector initialization
bool test_vec_init_aligned(void) {
    WriteFmt("Testing VecInit with aligned allocator\n");

    HeapAllocator aligned4  = HeapAllocatorInitAligned(4);
    HeapAllocator aligned16 = HeapAllocatorInitAligned(16);

    // Test with int type and 4-byte alignment
    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&aligned4);

    // Check initial state
    bool result =
        (vec.length == 0 && vec.capacity == 0 && vec.data == NULL && vec.allocator->alignment == 4 &&
         vec.copy_init == NULL && vec.copy_deinit == NULL);

    // Clean up
    VecDeinit(&vec);

    // Test with struct type and 16-byte alignment
    typedef Vec(TestItem) TestVec;
    TestVec test_vec = VecInit(&aligned16);

    // Check initial state
    result =
        result && (test_vec.length == 0 && test_vec.capacity == 0 && test_vec.data == NULL &&
                   test_vec.allocator->alignment == 16 && test_vec.copy_init == NULL && test_vec.copy_deinit == NULL);

    // Clean up
    VecDeinit(&test_vec);

    HeapAllocatorDeinit(&aligned4);
    HeapAllocatorDeinit(&aligned16);
    return result;
}

// Test vector initialization with deep copy functions
bool test_vec_init_with_deep_copy(void) {
    WriteFmt("Testing VecInitWithDeepCopy\n");

    // Test with struct type and custom copy/deinit functions
    typedef Vec(TestItem) TestVec;
    TestVec vec = VecInitWithDeepCopy(TestItemCopyInit, TestItemDeinit, &alloc);

    // Check initial state
    bool result =
        (vec.length == 0 && vec.capacity == 0 && vec.data == NULL && vec.allocator->alignment == 1 &&
         vec.copy_init == (GenericCopyInit)TestItemCopyInit && vec.copy_deinit == (GenericCopyDeinit)TestItemDeinit);

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Test vector initialization with alignment and deep copy functions
bool test_vec_init_aligned_with_deep_copy(void) {
    WriteFmt("Testing VecInit with aligned allocator and deep copy\n");

    HeapAllocator aligned8 = HeapAllocatorInitAligned(8);

    // Test with struct type, custom copy/deinit functions, and 8-byte alignment
    typedef Vec(TestItem) TestVec;
    TestVec vec = VecInitWithDeepCopy(TestItemCopyInit, TestItemDeinit, &aligned8);

    // Check initial state
    bool result =
        (vec.length == 0 && vec.capacity == 0 && vec.data == NULL && vec.allocator->alignment == 8 &&
         vec.copy_init == (GenericCopyInit)TestItemCopyInit && vec.copy_deinit == (GenericCopyDeinit)TestItemDeinit);

    // Clean up
    VecDeinit(&vec);

    HeapAllocatorDeinit(&aligned8);
    return result;
}

// Test vector initialization variants with an explicit optional allocator
bool test_vec_init_optional_allocator(void) {
    WriteFmt("Testing VecInit optional allocator\n");

    typedef Vec(TestItem) TestVec;

    // Build several heaps with distinct configuration; the test verifies
    // that the pointer-based allocator stored in the vector reflects each
    // allocator's settings.
    HeapAllocator h_default = HeapAllocatorInit();
    HeapAllocator h8        = HeapAllocatorInitAligned(8);
    HeapAllocator h16       = HeapAllocatorInitAligned(16);
    HeapAllocator h32       = HeapAllocatorInitAligned(32);
    HeapAllocator h64       = HeapAllocatorInitAligned(64);

    h_default.base.retry_limit = 17;
    h8.base.retry_limit        = 17;
    h16.base.retry_limit       = 17;
    h32.base.retry_limit       = 17;
    h64.base.retry_limit       = 17;

    TestVec vec_a = VecInit(&h_default);
    TestVec vec_b = VecInitT(vec_b, &h_default);
    TestVec vec_c = VecInitWithDeepCopy(TestItemCopyInit, TestItemDeinit, &h_default);
    TestVec vec_d = VecInitWithDeepCopyT(vec_d, TestItemCopyInit, TestItemDeinit, &h_default);
    TestVec vec_e = VecInit(&h8);
    TestVec vec_f = VecInitT(vec_f, &h16);
    TestVec vec_g = VecInitWithDeepCopy(TestItemCopyInit, TestItemDeinit, &h32);
    TestVec vec_h = VecInitWithDeepCopyT(vec_h, TestItemCopyInit, TestItemDeinit, &h64);

    bool result = (vec_a.allocator->retry_limit == 17) && (vec_b.allocator->retry_limit == 17);
    result      = result && (vec_c.allocator->retry_limit == 17) && (vec_d.allocator->retry_limit == 17);
    result      = result && (vec_e.allocator->retry_limit == 17) && (vec_f.allocator->retry_limit == 17);
    result      = result && (vec_g.allocator->retry_limit == 17) && (vec_h.allocator->retry_limit == 17);
    result      = result && (vec_e.allocator->alignment == 8) && (vec_f.allocator->alignment == 16);
    result      = result && (vec_g.allocator->alignment == 32) && (vec_h.allocator->alignment == 64);
    result      = result && (vec_c.copy_init == (GenericCopyInit)TestItemCopyInit);
    result      = result && (vec_d.copy_deinit == (GenericCopyDeinit)TestItemDeinit);

    VecDeinit(&vec_a);
    VecDeinit(&vec_b);
    VecDeinit(&vec_c);
    VecDeinit(&vec_d);
    VecDeinit(&vec_e);
    VecDeinit(&vec_f);
    VecDeinit(&vec_g);
    VecDeinit(&vec_h);

    HeapAllocatorDeinit(&h_default);
    HeapAllocatorDeinit(&h8);
    HeapAllocatorDeinit(&h16);
    HeapAllocatorDeinit(&h32);
    HeapAllocatorDeinit(&h64);
    return result;
}

// Test vector stack initialization
bool test_vec_init_stack(void) {
    WriteFmt("Testing VecInitStack\n");

    bool result = true;

    // Test with basic int type
    typedef Vec(int) IntVec;
    IntVec vec;

    VecInitStack(vec, &alloc, 10, {
        // Inside the scope where the stack vector is valid

        // Check initial state
        if (vec.length != 0 || vec.capacity != 10 || vec.data == NULL || vec.allocator->alignment != 1 ||
            vec.copy_init != NULL || vec.copy_deinit != NULL) {
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

    VecInitStack(test_vec, &alloc, 5, {
        // Inside the scope where the stack vector is valid

        // Check initial state
        if (test_vec.length != 0 || test_vec.capacity != 5 || test_vec.data == NULL ||
            test_vec.allocator->alignment != 1 || test_vec.copy_init != NULL || test_vec.copy_deinit != NULL) {
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
    WriteFmt("Testing vector cloning\n");

    // Create a source vector
    typedef Vec(int) IntVec;
    IntVec src = VecInit(&alloc);

    // Add some data
    VecPushBackR(&src, 1);
    VecPushBackR(&src, 2);
    VecPushBackR(&src, 3);

    // Create a destination vector
    IntVec clone = VecInit(&alloc);

    // Clone the source vector into the destination
    VecPushBackArrR(&clone, src.data, src.length);

    // Check that the clone has the same data but different memory
    bool result =
        (clone.length == src.length && clone.capacity >= src.length && clone.data != src.data &&
         clone.allocator->alignment == src.allocator->alignment);

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
    WriteFmt("[INFO] Starting Vec.Init tests\n\n");

    alloc = DefaultAllocatorInit();

    // Array of test functions
    TestFunction tests[] = {
        test_vec_init_basic,
        test_vec_init_aligned,
        test_vec_init_with_deep_copy,
        test_vec_init_aligned_with_deep_copy,
        test_vec_init_optional_allocator,
        test_vec_init_stack,
        test_vec_init_clone
    };

    int total_tests = sizeof(tests) / sizeof(tests[0]);

    // Run all tests using the centralized test driver
    int rc = run_test_suite(tests, total_tests, NULL, 0, "Vec.Init");
    DefaultAllocatorDeinit(&alloc);
    return rc;
}
