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
    bool result = (VecLen(&vec) == 0 && VecCapacity(&vec) == 0 && VecBegin(&vec) == NULL &&
                   VecAllocator(&vec)->alignment == 1 && VecCopyInit(&vec) == NULL && VecCopyDeinit(&vec) == NULL);

    // Clean up
    VecDeinit(&vec);

    // Test with struct type
    typedef Vec(TestItem) TestVec;
    TestVec test_vec = VecInit(&alloc);

    // Check initial state
    result = result &&
             (VecLen(&test_vec) == 0 && VecCapacity(&test_vec) == 0 && VecBegin(&test_vec) == NULL &&
              VecAllocator(&test_vec)->alignment == 1 && VecCopyInit(&test_vec) == NULL &&
              VecCopyDeinit(&test_vec) == NULL);

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
    bool result = (VecLen(&vec) == 0 && VecCapacity(&vec) == 0 && VecBegin(&vec) == NULL &&
                   VecAllocator(&vec)->alignment == 4 && VecCopyInit(&vec) == NULL && VecCopyDeinit(&vec) == NULL);

    // Clean up
    VecDeinit(&vec);

    // Test with struct type and 16-byte alignment
    typedef Vec(TestItem) TestVec;
    TestVec test_vec = VecInit(&aligned16);

    // Check initial state
    result = result &&
             (VecLen(&test_vec) == 0 && VecCapacity(&test_vec) == 0 && VecBegin(&test_vec) == NULL &&
              VecAllocator(&test_vec)->alignment == 16 && VecCopyInit(&test_vec) == NULL &&
              VecCopyDeinit(&test_vec) == NULL);

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
        (VecLen(&vec) == 0 && VecCapacity(&vec) == 0 && VecBegin(&vec) == NULL &&
         VecAllocator(&vec)->alignment == 1 && VecCopyInit(&vec) == (GenericCopyInit)TestItemCopyInit &&
         VecCopyDeinit(&vec) == (GenericCopyDeinit)TestItemDeinit);

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
        (VecLen(&vec) == 0 && VecCapacity(&vec) == 0 && VecBegin(&vec) == NULL &&
         VecAllocator(&vec)->alignment == 8 && VecCopyInit(&vec) == (GenericCopyInit)TestItemCopyInit &&
         VecCopyDeinit(&vec) == (GenericCopyDeinit)TestItemDeinit);

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

    bool result = (VecAllocator(&vec_a)->retry_limit == 17) && (VecAllocator(&vec_b)->retry_limit == 17);
    result      = result && (VecAllocator(&vec_c)->retry_limit == 17) && (VecAllocator(&vec_d)->retry_limit == 17);
    result      = result && (VecAllocator(&vec_e)->retry_limit == 17) && (VecAllocator(&vec_f)->retry_limit == 17);
    result      = result && (VecAllocator(&vec_g)->retry_limit == 17) && (VecAllocator(&vec_h)->retry_limit == 17);
    result      = result && (VecAllocator(&vec_e)->alignment == 8) && (VecAllocator(&vec_f)->alignment == 16);
    result      = result && (VecAllocator(&vec_g)->alignment == 32) && (VecAllocator(&vec_h)->alignment == 64);
    result      = result && (VecCopyInit(&vec_c) == (GenericCopyInit)TestItemCopyInit);
    result      = result && (VecCopyDeinit(&vec_d) == (GenericCopyDeinit)TestItemDeinit);

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

    // VecInitStack declares and scopes `vec` itself (for-chain idiom),
    // matching StrInitStack.
    VecInitStack(int, vec, 10) {
        // Stack-init: NULL allocator distinguishes from heap-init.
        if (VecLen(&vec) != 0 || VecCapacity(&vec) != 10 || VecBegin(&vec) == NULL ||
            VecAllocator(&vec) != NULL) {
            result = false;
        }

        VecPushBackR(&vec, 10);
        VecPushBackR(&vec, 20);
        VecPushBackR(&vec, 30);

        if (VecLen(&vec) != 3 || VecAt(&vec, 0) != 10 || VecAt(&vec, 1) != 20 || VecAt(&vec, 2) != 30) {
            result = false;
        }
    }

    // Test with struct type
    VecInitStack(TestItem, test_vec, 5) {
        if (VecLen(&test_vec) != 0 || VecCapacity(&test_vec) != 5 || VecBegin(&test_vec) == NULL ||
            VecAllocator(&test_vec) != NULL) {
            result = false;
        }

        TestItem item = {0};
        item.id       = 1;
        item.value    = 3.14;
        VecPushBackR(&test_vec, item);

        if (VecLen(&test_vec) != 1 || VecAt(&test_vec, 0).id != 1 || VecAt(&test_vec, 0).value != 3.14f) {
            result = false;
        }
    }

    // High-alignment struct: confirm the macro's `_Alignas(T)` actually
    // gives the backing buffer `_Alignof(T)`-byte alignment. `double`
    // forces an 8-byte alignment requirement on the slot layout.
    typedef struct {
        i32 a;
        f64 b;
    } AlignedItem;
    VecInitStack(AlignedItem, av, 4) {
        if ((size)(void *)VecBegin(&av) % _Alignof(AlignedItem) != 0) {
            result = false;
        }
        AlignedItem item = {.a = 7, .b = 2.71828};
        VecPushBackR(&av, item);
        if (VecLen(&av) != 1 || VecAt(&av, 0).a != 7 || VecAt(&av, 0).b != 2.71828) {
            result = false;
        }
    }

    // `break` exit path: the body breaks early; the backing buffer's
    // outer-for update still runs.
    VecInitStack(int, breakable, 8) {
        VecPushBackR(&breakable, 1);
        if (VecLen(&breakable) != 1) {
            result = false;
        }
        break; // exits cleanly; backing array still zeroed by outer update
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
    VecPushBackArrR(&clone, VecBegin(&src), VecLen(&src));

    // Check that the clone has the same data but different memory
    bool result =
        (VecLen(&clone) == VecLen(&src) && VecCapacity(&clone) >= VecLen(&src) && VecBegin(&clone) != VecBegin(&src) &&
         VecAllocator(&clone)->alignment == VecAllocator(&src)->alignment);

    // Check the actual data
    if (result) {
        for (size i = 0; i < VecLen(&src); i++) {
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
