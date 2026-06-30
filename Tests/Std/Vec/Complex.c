#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Allocator/Heap.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Std/Container/Vec.h>
#include <Misra/Std/Memory.h>
#include <Misra/Std/Log.h>
#include <Misra/Types.h> // For LVAL macro

// Include test utilities
#include "../../Util/TestRunner.h"

// File-scoped HeapAllocator backing every fixture-owned `name` / `values`
// allocation. The fixture deliberately keeps `name` as a raw `char *` and
// `values` as a raw `int *` so the test exercises Vec's deep-copy
// (`copy_init`) / `copy_deinit` callbacks on a type whose nested storage
// the container itself doesn't own. Initialised on first use; released
// by the OS at process exit.
static HeapAllocator g_fixture_heap;
static bool          g_fixture_heap_ready = false;

static Allocator *fixture_alloc(void) {
    if (!g_fixture_heap_ready) {
        g_fixture_heap       = HeapAllocatorInit();
        g_fixture_heap_ready = true;
    }
    return ALLOCATOR_OF(&g_fixture_heap);
}

#define fixture_malloc(n) AllocatorAlloc(fixture_alloc(), (n), false)
#define fixture_free(p)   AllocatorFree(fixture_alloc(), (p))

// Cast off const: `zstr_dup` returns a `Zstr` because the project convention
// treats freshly-allocated strings as read-only by default. This test fixture
// owns the storage and needs a writable handle to feed into a raw `char *`
// struct field, so the cast is correct here.
static inline char *ZstrDupAlloc(Zstr s) {
    return (char *)zstr_dup(s, fixture_alloc());
}


// Define a complex structure with nested pointers
typedef struct {
    char *name;       // Dynamically allocated string
    int  *values;     // Dynamically allocated array
    size  num_values; // Size of the values array
} ComplexItem;

// Copy init function for ComplexItem
bool ComplexItemCopyInit(ComplexItem *dst, ComplexItem *src) {
    if (!dst || !src)
        return false;

    // Copy name
    if (src->name) {
        size name_len = ZstrLen(src->name);
        dst->name     = fixture_malloc(name_len + 1);
        if (!dst->name)
            return false;
        MemCopy(dst->name, src->name, name_len + 1);
    } else {
        dst->name = NULL;
    }

    // Copy values array
    dst->num_values = src->num_values;
    if (src->values && src->num_values > 0) {
        dst->values = fixture_malloc(src->num_values * sizeof(int));
        if (!dst->values) {
            fixture_free(dst->name);
            dst->name = NULL;
            return false;
        }
        MemCopy(dst->values, src->values, src->num_values * sizeof(int));
    } else {
        dst->values = NULL;
    }

    return true;
}

// Deinit function for ComplexItem
void ComplexItemDeinit(ComplexItem *item) {
    if (!item)
        return;

    // Free name
    if (item->name) {
        fixture_free(item->name);
        item->name = NULL;
    }

    // Free values array
    if (item->values) {
        fixture_free(item->values);
        item->values = NULL;
    }

    item->num_values = 0;
}

// Helper function to create a ComplexItem.
// This fixture uses `fixture_malloc` / `fixture_free` (a separate, test-owned
// HeapAllocator) for both `name` and `values` so the test owns the entire
// allocation lifecycle. The library's containers are exercised elsewhere in
// the file with the standard Misra `Allocator *` plumbing; this fixture
// only proves the container's deep-copy hooks call user destructors against
// independently-allocated payloads.
ComplexItem InitComplexItem(Zstr name, int *values, size num_values) {
    ComplexItem item = {0};

    if (name) {
        size name_len = ZstrLen(name);
        item.name     = (char *)fixture_malloc(name_len + 1);
        if (item.name) {
            MemCopy(item.name, name, name_len);
            item.name[name_len] = '\0';
        }
    }

    if (values && num_values > 0) {
        item.values = (int *)fixture_malloc(num_values * sizeof(int));
        if (item.values) {
            MemCopy(item.values, values, num_values * sizeof(int));
            item.num_values = num_values;
        }
    }

    return item;
}

// Helper function to check if two ComplexItems are equal
bool ComplexItemsEqual(ComplexItem *a, ComplexItem *b) {
    if (!a || !b)
        return false;

    // Check name
    if ((a->name && !b->name) || (!a->name && b->name))
        return false;
    if (a->name && b->name && ZstrCompare(a->name, b->name) != 0)
        return false;

    // Check values array
    if (a->num_values != b->num_values)
        return false;
    if (a->num_values > 0) {
        if ((a->values && !b->values) || (!a->values && b->values))
            return false;
        if (a->values && b->values) {
            for (size i = 0; i < a->num_values; i++) {
                if (a->values[i] != b->values[i])
                    return false;
            }
        }
    }

    return true;
}

// Function prototypes
bool test_complex_vec_init(void);
bool test_complex_vec_push(void);
bool test_complex_vec_insert(void);
bool test_complex_vec_merge(void);
bool test_lvalue_operations(void);
bool test_fast_operations(void);
bool test_delete_operations(void);
bool test_edge_cases(void);
bool test_lvalue_zero_on_take_pushback(void);
bool test_lvalue_zero_on_take_insert(void);
bool test_lvalue_zero_on_take_fast_insert(void);
bool test_lvalue_zero_on_take_pushfront(void);
bool test_lvalue_zero_on_take_merge(void);
bool test_lvalue_zero_on_take_array_ops(void);

// Test initialization with complex structure
static DefaultAllocator alloc;

bool test_complex_vec_init(void) {
    WriteFmt("Testing vector initialization with complex structure\n");

    // Create a vector of ComplexItem with deep copy functions
    typedef Vec(ComplexItem) ComplexVec;
    ComplexVec vec = VecInitWithDeepCopy(ComplexItemCopyInit, ComplexItemDeinit, &alloc);

    // Check initial state
    bool result =
        (VecLen(&vec) == 0 && VecCapacity(&vec) == 0 && VecBegin(&vec) == NULL &&
         VecCopyInit(&vec) == (GenericCopyInit)ComplexItemCopyInit &&
         VecCopyDeinit(&vec) == (GenericCopyDeinit)ComplexItemDeinit);

    // Create a test item
    int         values[] = {1, 2, 3};
    ComplexItem item     = InitComplexItem("Test Item", values, 3);

    // Add the item to the vector
    VecPushBackR(&vec, item);

    // Check that the vector now has one item
    result = result && (VecLen(&vec) == 1);

    // Check that the item was copied correctly
    result = result && ComplexItemsEqual(&VecAt(&vec, 0), &item);

    // Modify the original item and verify the vector's copy is independent
    fixture_free(item.name);
    item.name      = ZstrDupAlloc("Modified");
    item.values[0] = 99;

    // The vector's copy should still have the original values
    result = result && (ZstrCompare(VecAt(&vec, 0).name, "Test Item") == 0);
    result = result && (VecAt(&vec, 0).values[0] == 1);

    // Clean up
    ComplexItemDeinit(&item);
    VecDeinit(&vec);

    return result;
}

// Test push operations with complex structure
bool test_complex_vec_push(void) {
    WriteFmt("Testing push operations with complex structure\n");

    // Create a vector of ComplexItem with deep copy functions
    typedef Vec(ComplexItem) ComplexVec;
    ComplexVec vec = VecInitWithDeepCopy(ComplexItemCopyInit, ComplexItemDeinit, &alloc);

    // Create test items
    int values1[] = {10, 20, 30};
    int values2[] = {40, 50, 60};
    int values3[] = {70, 80, 90};

    ComplexItem item1 = InitComplexItem("Item 1", values1, 3);
    ComplexItem item2 = InitComplexItem("Item 2", values2, 3);
    ComplexItem item3 = InitComplexItem("Item 3", values3, 3);

    // Push items to the vector
    VecPushBackR(&vec, item1);
    VecPushFrontR(&vec, item2);
    VecPushBackR(&vec, item3);

    // Check vector length
    bool result = (VecLen(&vec) == 3);

    // Check items order: item2, item1, item3
    result = result && (ZstrCompare(VecAt(&vec, 0).name, "Item 2") == 0);
    result = result && (ZstrCompare(VecAt(&vec, 1).name, "Item 1") == 0);
    result = result && (ZstrCompare(VecAt(&vec, 2).name, "Item 3") == 0);

    // Check values
    result = result && (VecAt(&vec, 0).values[0] == 40);
    result = result && (VecAt(&vec, 1).values[0] == 10);
    result = result && (VecAt(&vec, 2).values[0] == 70);

    // Clean up
    ComplexItemDeinit(&item1);
    ComplexItemDeinit(&item2);
    ComplexItemDeinit(&item3);
    VecDeinit(&vec);

    return result;
}

// Test insert operations with complex structure
bool test_complex_vec_insert(void) {
    WriteFmt("Testing insert operations with complex structure\n");

    // Create a vector of ComplexItem with deep copy functions
    typedef Vec(ComplexItem) ComplexVec;
    ComplexVec vec = VecInitWithDeepCopy(ComplexItemCopyInit, ComplexItemDeinit, &alloc);

    // Create test items
    int values1[] = {10, 20, 30};
    int values2[] = {40, 50, 60};
    int values3[] = {70, 80, 90};

    ComplexItem item1 = InitComplexItem("Item 1", values1, 3);
    ComplexItem item2 = InitComplexItem("Item 2", values2, 3);
    ComplexItem item3 = InitComplexItem("Item 3", values3, 3);

    // Add item1 to the vector
    VecPushBackR(&vec, item1);

    // Insert item2 at index 0
    VecInsertR(&vec, item2, 0);

    // Insert item3 in the middle
    VecInsertR(&vec, item3, 1);

    // Check vector length
    bool result = (VecLen(&vec) == 3);

    // Check items order: item2, item3, item1
    result = result && (ZstrCompare(VecAt(&vec, 0).name, "Item 2") == 0);
    result = result && (ZstrCompare(VecAt(&vec, 1).name, "Item 3") == 0);
    result = result && (ZstrCompare(VecAt(&vec, 2).name, "Item 1") == 0);

    // Clean up
    ComplexItemDeinit(&item1);
    ComplexItemDeinit(&item2);
    ComplexItemDeinit(&item3);
    VecDeinit(&vec);

    return result;
}

// Test merge operations with complex structure
bool test_complex_vec_merge(void) {
    WriteFmt("Testing merge operations with complex structure\n");

    // Create two vectors of ComplexItem with deep copy functions
    typedef Vec(ComplexItem) ComplexVec;
    ComplexVec vec1 = VecInitWithDeepCopy(ComplexItemCopyInit, ComplexItemDeinit, &alloc);
    ComplexVec vec2 = VecInitWithDeepCopy(ComplexItemCopyInit, ComplexItemDeinit, &alloc);

    // Create test items
    int values1[] = {10, 20, 30};
    int values2[] = {40, 50, 60};
    int values3[] = {70, 80, 90};

    ComplexItem item1 = InitComplexItem("Item 1", values1, 3);
    ComplexItem item2 = InitComplexItem("Item 2", values2, 3);
    ComplexItem item3 = InitComplexItem("Item 3", values3, 3);

    // Add items to vectors
    VecPushBackR(&vec1, item1);
    VecPushBackR(&vec2, item2);
    VecPushBackR(&vec2, item3);

    // Merge vec2 into vec1
    VecMergeR(&vec1, &vec2);

    // Check vector lengths
    bool result = (VecLen(&vec1) == 3);
    result      = result && (VecLen(&vec2) == 2); // VecMergeR doesn't modify source vector

    // Check items in vec1: item1, item2, item3
    result = result && (ZstrCompare(VecAt(&vec1, 0).name, "Item 1") == 0);
    result = result && (ZstrCompare(VecAt(&vec1, 1).name, "Item 2") == 0);
    result = result && (ZstrCompare(VecAt(&vec1, 2).name, "Item 3") == 0);

    // Now test VecMergeL which transfers ownership
    ComplexVec vec3 = VecInitWithDeepCopy(NULL, ComplexItemDeinit, &alloc);
    ComplexVec vec4 = VecInitWithDeepCopy(NULL, ComplexItemDeinit, &alloc);

    // Create more test items
    int values4[] = {100, 110, 120};
    int values5[] = {130, 140, 150};

    ComplexItem item4 = InitComplexItem("Item 4", values4, 3);
    ComplexItem item5 = InitComplexItem("Item 5", values5, 3);

    // Add items to vectors
    VecPushBackL(&vec3, item4);
    VecPushBackL(&vec4, item5);

    // Check that item 4 and 5 are no longer valid
    result = result && (item4.name == NULL && item4.num_values == 0 && item4.values == NULL);
    result = result && (item5.name == NULL && item5.num_values == 0 && item5.values == NULL);

    // Merge vec4 into vec3 with ownership transfer
    VecMergeL(&vec3, &vec4);

    // Check vector lengths
    result = result && (VecLen(&vec3) == 2);
    result = result && (VecLen(&vec4) == 0); // VecMergeL resets source vector
    result = result && (VecBegin(&vec4) == NULL);

    // Check items in vec3: item4, item5
    result = result && (ZstrCompare(VecAt(&vec3, 0).name, "Item 4") == 0);
    result = result && (ZstrCompare(VecAt(&vec3, 1).name, "Item 5") == 0);

    // Clean up
    ComplexItemDeinit(&item1);
    ComplexItemDeinit(&item2);
    ComplexItemDeinit(&item3);
    VecDeinit(&vec1);
    VecDeinit(&vec2);
    VecDeinit(&vec3);
    VecDeinit(&vec4);

    return result;
}

// Test L-value operations
bool test_lvalue_operations(void) {
    WriteFmt("Testing L-value operations\n");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Test VecPushBackL
    int val1 = 10;
    VecPushBackL(&vec, val1);

    // Test VecPushFrontL
    int val2 = 20;
    VecPushFrontL(&vec, val2);

    // Test VecInsertL
    int val3 = 30;
    VecInsertL(&vec, val3, 1);

    // Check vector length
    bool result = (VecLen(&vec) == 3);

    // Check items order: val2, val3, val1
    result = result && (VecAt(&vec, 0) == 20);
    result = result && (VecAt(&vec, 1) == 30);
    result = result && (VecAt(&vec, 2) == 10);

    // Test array operations
    int arr[] = {40, 50, 60};

    // Test VecPushBackArrL
    VecPushBackArrL(&vec, arr, 3);

    // Check vector length
    result = result && (VecLen(&vec) == 6);

    // Check items: val2, val3, val1, arr[0], arr[1], arr[2]
    result = result && (VecAt(&vec, 3) == 40);
    result = result && (VecAt(&vec, 4) == 50);
    result = result && (VecAt(&vec, 5) == 60);

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Test fast operations
bool test_fast_operations(void) {
    WriteFmt("Testing fast operations\n");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Add some initial elements
    for (int i = 0; i < 5; i++) {
        VecPushBackR(&vec, i * 10);
    }

    // Test VecInsertFast
    // Use a temporary variable to avoid r-value issues
    int temp = 99;
    VecInsertFastR(&vec, temp, 2);

    // Check vector length
    bool result = (VecLen(&vec) == 6);

    // Check that the element was inserted
    result = result && (VecAt(&vec, 2) == 99);

    // Use regular insert range instead of fast insert range
    // This is more reliable after other operations
    int arr[] = {100, 200, 300};
    VecInsertRange(&vec, arr, 1, 3);

    // Check vector length
    result = result && (VecLen(&vec) == 9);

    // Check that the array was inserted
    result = result && (VecAt(&vec, 1) == 100);
    result = result && (VecAt(&vec, 2) == 200);
    result = result && (VecAt(&vec, 3) == 300);

    // Clean up
    VecDeinit(&vec);

    // Now test VecInsertRangeFastR in isolation
    IntVec vec2 = VecInit(&alloc);

    // Add some initial elements
    for (int i = 0; i < 5; i++) {
        VecPushBackR(&vec2, i * 10);
    }

    // Ensure we have enough capacity to avoid reallocation during the test
    VecReserve(&vec2, VecLen(&vec2) + 10);

    // Try inserting just one element first with fast insert
    int single_val = 42;
    VecInsertFastR(&vec2, single_val, 2);

    result = result && (VecLen(&vec2) == 6);
    result = result && (VecAt(&vec2, 2) == 42);

    // Now try the range insert with a small array
    int small_arr[] = {111, 222};

    // Use VecInsertRangeFastR with a small array
    VecInsertRangeFastR(&vec2, small_arr, 1, 2);

    // Check vector length
    result = result && (VecLen(&vec2) == 8);

    // Check that the array was inserted
    result = result && (VecAt(&vec2, 1) == 111);
    result = result && (VecAt(&vec2, 2) == 222);

    // Clean up
    VecDeinit(&vec2);

    return result;
}

// Test delete operations
bool test_delete_operations(void) {
    WriteFmt("Testing delete operations\n");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Add some initial elements
    int values[] = {10, 20, 30, 40, 50, 60, 70, 80, 90};
    VecPushBackArrR(&vec, values, 9);

    // Test VecDelete (regular delete)
    VecDelete(&vec, 2); // Delete 30

    // Check vector length after deletion
    bool result = (VecLen(&vec) == 8);

    // Check that the element was deleted and elements shifted
    result = result && (VecAt(&vec, 0) == 10);
    result = result && (VecAt(&vec, 1) == 20);
    result = result && (VecAt(&vec, 2) == 40); // 30 was deleted, so now 40 is at index 2

    // Test VecDeleteRange (regular delete range)
    VecDeleteRange(&vec, 0, 2); // Delete 10 and 20

    // Check vector length after range deletion
    result = result && (VecLen(&vec) == 6);

    // Check that elements were deleted and remaining elements shifted
    result = result && (VecAt(&vec, 0) == 40);
    result = result && (VecAt(&vec, 1) == 50);

    // Clean up this vector
    VecDeinit(&vec);

    // Create a new vector for testing fast delete operations
    vec = VecInitT(vec, &alloc);

    // Add elements again
    VecPushBackArrR(&vec, values, 9);

    // Test VecDeleteFast (fast delete)
    // This replaces the deleted element with the last element
    VecDeleteFast(&vec, 2); // Delete 30

    // Check vector length after fast deletion
    result = result && (VecLen(&vec) == 8);

    // Check that the element was deleted and replaced with the last element (90)
    result = result && (VecAt(&vec, 0) == 10);
    result = result && (VecAt(&vec, 1) == 20);
    result = result && (VecAt(&vec, 2) == 90); // 30 was replaced with 90
    result = result && (VecAt(&vec, 7) == 80); // Last element is now 80

    // Test VecDeleteRangeFast (fast delete range)
    VecDeleteRangeFast(&vec, 0, 2); // Delete 10 and 20

    // Check vector length after fast range deletion
    result = result && (VecLen(&vec) == 6);

    // Check that elements were deleted and replaced with elements from the end
    // The exact order depends on the implementation, but length should be correct

    // Clean up
    VecDeinit(&vec);

    // Test with an L-value index
    vec = VecInitT(vec, &alloc);

    // Add elements again
    VecPushBackArrR(&vec, values, 9);

    // Create an L-value index to use with regular VecDelete
    int index_to_delete = 3; // Delete 40

    // Use regular VecDelete with the L-value index
    VecDelete(&vec, index_to_delete);

    // Check vector length after deletion
    result = result && (VecLen(&vec) == 8);

    // Check that the element was deleted
    result = result && (VecAt(&vec, 3) == 50); // 40 was deleted, so now 50 is at index 3

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Test edge cases
bool test_edge_cases(void) {
    WriteFmt("Testing edge cases\n");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Test pushing to empty vector
    VecPushBackR(&vec, 10);
    bool result = (VecLen(&vec) == 1 && VecAt(&vec, 0) == 10);

    // Test pushing with zero count
    // Create a small array and use count=0
    int small_arr[1] = {42};
    VecPushBackArrR(&vec, small_arr, 0);
    result = result && (VecLen(&vec) == 1); // Length should not change

    // Test inserting at end index
    VecInsertR(&vec, 20, VecLen(&vec));
    result = result && (VecLen(&vec) == 2 && VecAt(&vec, 1) == 20);

    // Test with large number of elements
    VecDeinit(&vec);
    vec = VecInitT(vec, &alloc);

    // Push a large number of elements
    const int large_count = 1000;
    for (int i = 0; i < large_count; i++) {
        VecPushBackR(&vec, i);
    }

    result = result && (VecLen(&vec) == large_count);

    // Verify all elements
    bool all_correct = true;
    for (int i = 0; i < large_count; i++) {
        if (VecAt(&vec, i) != i) {
            all_correct = false;
            break;
        }
    }
    result = result && all_correct;

    // Test with zero-capacity vector
    VecDeinit(&vec);
    vec = VecInitT(vec, &alloc);

    // Reserve zero capacity
    VecReserve(&vec, 0);
    result = result && (VecCapacity(&vec) == 0);

    // Push an element (should auto-resize)
    VecPushBackR(&vec, 42);
    result = result && (VecLen(&vec) == 1 && VecAt(&vec, 0) == 42);

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Test VecPushBackL zero-on-take behavior with complex structures
bool test_lvalue_zero_on_take_pushback(void) {
    WriteFmt("Testing VecPushBackL zero-on-take with complex structures\n");

    // Create a test item
    int         values[] = {10, 20, 30};
    ComplexItem item     = InitComplexItem("Test Item", values, 3);

    // Create a temporary vector with no copy_init but with copy_deinit for proper cleanup
    typedef Vec(ComplexItem) ComplexVec;
    ComplexVec temp_vec = VecInitWithDeepCopy(NULL, ComplexItemDeinit, &alloc);

    // Insert with L-value semantics (vector takes ownership)
    VecPushBackL(&temp_vec, item);

    // Check that the item was zeroed
    bool result = (item.name == NULL);
    result      = result && (item.values == NULL);
    result      = result && (item.num_values == 0);

    // Clean up the temporary vector
    VecDeinit(&temp_vec);

    return result;
}

// Test VecInsertL zero-on-take behavior with complex structures
bool test_lvalue_zero_on_take_insert(void) {
    WriteFmt("Testing VecInsertL zero-on-take with complex structures\n");

    // Create a test item
    int         values[] = {40, 50, 60};
    ComplexItem item     = InitComplexItem("Another Item", values, 3);

    // Create a vector with no copy_init but with copy_deinit for proper cleanup
    typedef Vec(ComplexItem) ComplexVec;
    ComplexVec vec = VecInitWithDeepCopy(NULL, ComplexItemDeinit, &alloc);

    // First, create a dummy item and add it to the vector
    ComplexItem dummy = {0};
    dummy.name        = ZstrDupAlloc("Dummy");
    dummy.values      = NULL;
    dummy.num_values  = 0;

    // Add the dummy item using L-value semantics
    VecPushBackL(&vec, dummy);

    // Now insert our test item at position 0 using L-value semantics
    VecInsertL(&vec, item, 0);

    // Check that the item was zeroed
    bool result = (item.name == NULL);
    result      = result && (item.values == NULL);
    result      = result && (item.num_values == 0);

    // Clean up the vector
    VecDeinit(&vec);

    return result;
}

// Test VecInsertFastL zero-on-take behavior with complex structures
bool test_lvalue_zero_on_take_fast_insert(void) {
    WriteFmt("Testing VecInsertFastL zero-on-take with complex structures\n");
    bool result = true;

    // Create a vector with no copy_init but with copy_deinit for proper cleanup
    typedef Vec(ComplexItem) ComplexVec;
    ComplexVec vec = VecInitWithDeepCopy(NULL, ComplexItemDeinit, &alloc);

    // Make sure we have enough capacity to avoid reallocation issues
    VecReserve(&vec, 10);

    // Create several dummy items to populate the vector
    ComplexItem dummy1 = {0};
    dummy1.name        = ZstrDupAlloc("Dummy1");

    ComplexItem dummy2 = {0};
    dummy2.name        = ZstrDupAlloc("Dummy2");

    ComplexItem dummy3 = {0};
    dummy3.name        = ZstrDupAlloc("Dummy3");

    // Add the dummy items using L-value semantics
    VecPushBackL(&vec, dummy1);
    VecPushBackL(&vec, dummy2);
    VecPushBackL(&vec, dummy3);

    // Test 1: Insert at the beginning
    int         values1[] = {10, 20, 30};
    ComplexItem item1     = InitComplexItem("Fast Item 1", values1, 3);
    VecInsertFastL(&vec, item1, 0);

    // Check that the item was zeroed
    result = result && (item1.name == NULL);
    result = result && (item1.values == NULL);
    result = result && (item1.num_values == 0);

    // Test 2: Insert in the middle
    int         values2[] = {40, 50, 60};
    ComplexItem item2     = InitComplexItem("Fast Item 2", values2, 3);
    VecInsertFastL(&vec, item2, 2);

    // Check that the item was zeroed
    result = result && (item2.name == NULL);
    result = result && (item2.values == NULL);
    result = result && (item2.num_values == 0);

    // Test 3: Insert at the end (this is actually an append operation)
    int         values3[] = {70, 80, 90};
    ComplexItem item3     = InitComplexItem("Fast Item 3", values3, 3);
    VecInsertFastL(&vec, item3, VecLen(&vec));

    // Check that the item was zeroed
    result = result && (item3.name == NULL);
    result = result && (item3.values == NULL);
    result = result && (item3.num_values == 0);

    // Verify vector integrity - should have 6 items now
    result = result && (VecLen(&vec) == 6);

    // Clean up the vector
    VecDeinit(&vec);

    return result;
}

// Test VecPushFrontL zero-on-take behavior with complex structures
bool test_lvalue_zero_on_take_pushfront(void) {
    WriteFmt("Testing VecPushFrontL zero-on-take with complex structures\n");

    // Create a test item
    int         values[] = {100, 110, 120};
    ComplexItem item     = InitComplexItem("Front Item", values, 3);

    // Create a vector with no copy_init but with copy_deinit for proper cleanup
    typedef Vec(ComplexItem) ComplexVec;
    ComplexVec vec = VecInitWithDeepCopy(NULL, ComplexItemDeinit, &alloc);

    // Add a dummy item first
    ComplexItem dummy = {0};
    dummy.name        = ZstrDupAlloc("Dummy");
    VecPushBackL(&vec, dummy);

    // Insert with L-value semantics at the front (vector takes ownership)
    VecPushFrontL(&vec, item);

    // Check that the item was zeroed
    bool result = (item.name == NULL);
    result      = result && (item.values == NULL);
    result      = result && (item.num_values == 0);

    // Clean up the vector
    VecDeinit(&vec);

    return result;
}

// Test VecMergeL zero-on-take behavior with complex structures
bool test_lvalue_zero_on_take_merge(void) {
    WriteFmt("Testing VecMergeL zero-on-take with complex structures\n");

    // Create a vector with no copy_init but with copy_deinit for proper cleanup
    typedef Vec(ComplexItem) ComplexVec;
    ComplexVec vec1 = VecInitWithDeepCopy(NULL, ComplexItemDeinit, &alloc);
    ComplexVec vec2 = VecInitWithDeepCopy(NULL, ComplexItemDeinit, &alloc);

    // Create test items for vec2
    int values1[] = {130, 140, 150};
    int values2[] = {160, 170, 180};

    ComplexItem item1 = InitComplexItem("Merge Item 1", values1, 3);
    ComplexItem item2 = InitComplexItem("Merge Item 2", values2, 3);

    // Add items to vec2
    VecPushBackL(&vec2, item1);
    VecPushBackL(&vec2, item2);

    // Verify items were cleared after being added to vec2
    bool result = (item1.name == NULL && item1.values == NULL && item1.num_values == 0);
    result      = result && (item2.name == NULL && item2.values == NULL && item2.num_values == 0);

    // Now merge vec2 into vec1 with L-value semantics
    VecMergeL(&vec1, &vec2);

    // Check that vec2 is now empty (data has been transferred)
    result = result && (VecLen(&vec2) == 0);
    result = result && (VecBegin(&vec2) == NULL);

    // Clean up
    VecDeinit(&vec1);
    VecDeinit(&vec2);

    return result;
}

// Test array operations with L-value semantics
bool test_lvalue_zero_on_take_array_ops(void) {
    WriteFmt("Testing array operations with L-value semantics\n");

    // Create a vector with no copy_init but with copy_deinit for proper cleanup
    typedef Vec(ComplexItem) ComplexVec;
    ComplexVec vec = VecInitWithDeepCopy(NULL, ComplexItemDeinit, &alloc);

    // Create an array of complex items
    ComplexItem items[3];

    // Initialize the array items
    int values1[] = {10, 20, 30};
    int values2[] = {40, 50, 60};
    int values3[] = {70, 80, 90};

    items[0] = InitComplexItem("Array Item 1", values1, 3);
    items[1] = InitComplexItem("Array Item 2", values2, 3);
    items[2] = InitComplexItem("Array Item 3", values3, 3);

    // Test VecPushBackArrL
    VecPushBackArrL(&vec, items, 3);

    // Check that all items were zeroed
    bool result = true;
    for (int i = 0; i < 3; i++) {
        result = result && (items[i].name == NULL);
        result = result && (items[i].values == NULL);
        result = result && (items[i].num_values == 0);
    }

    // Clean up the vector
    VecDeinit(&vec);

    return result;
}

// ===========================================================================
// clone_vec (deep-copy) mutation-hardening suite (from Vec.Mutants6)
// ===========================================================================

// clone_vec @ 201:21 (==0 -> !=0): a non-empty src must be fully cloned.
bool test_clone_copies_first_element(void) {
    WriteFmt("Testing clone copies a non-empty source\n");

    DefaultAllocator local = DefaultAllocatorInit();

    typedef Vec(int) IntVec;
    IntVec src      = VecInit(&local);
    int    values[] = {1, 2, 3};
    for (int i = 0; i < 3; i++) {
        VecPushBackR(&src, values[i]);
    }

    IntVec dst = VecInit(&local);
    VecInitClone(&dst, &src);

    // Under the mutant src->length != 0 returns true immediately and dst is
    // left empty (length 0).
    bool result = (VecLen(&dst) == 3) && (VecAt(&dst, 0) == 1) && (VecAt(&dst, 1) == 2) && (VecAt(&dst, 2) == 3);

    VecDeinit(&src);
    VecDeinit(&dst);
    DefaultAllocatorDeinit(&local);
    return result;
}

// clone_vec @ 209:15 (copy loop init 0 -> const 42).
bool test_clone_includes_index_zero(void) {
    WriteFmt("Testing clone copy loop starts at index zero\n");

    DefaultAllocator local = DefaultAllocatorInit();

    typedef Vec(int) IntVec;
    IntVec src      = VecInit(&local);
    int    values[] = {1, 2, 3};
    for (int i = 0; i < 3; i++) {
        VecPushBackR(&src, values[i]);
    }

    IntVec dst = VecInit(&local);
    VecInitClone(&dst, &src);

    // Real: dst has all 3 elements starting at index 0. Mutant (init 42):
    // 42 < 3 is false, loop skipped, dst length 0.
    bool result = (VecLen(&dst) == 3) && (VecAt(&dst, 0) == 1);

    VecDeinit(&src);
    VecDeinit(&dst);
    DefaultAllocatorDeinit(&local);
    return result;
}

// clone_vec @ 209:24 (i < length -> i >= length).
bool test_clone_loop_runs_for_nonempty(void) {
    WriteFmt("Testing clone copy loop condition\n");

    DefaultAllocator local = DefaultAllocatorInit();

    typedef Vec(int) IntVec;
    IntVec src      = VecInit(&local);
    int    values[] = {1, 2, 3};
    for (int i = 0; i < 3; i++) {
        VecPushBackR(&src, values[i]);
    }

    IntVec dst = VecInit(&local);
    VecInitClone(&dst, &src);

    // Mutant (i >= length): 0 >= 3 is false, loop never runs, dst empty.
    bool result = (VecLen(&dst) == 3);

    VecDeinit(&src);
    VecDeinit(&dst);
    DefaultAllocatorDeinit(&local);
    return result;
}

// clone_vec @ 209:40 (i++ -> i--).
bool test_clone_visits_every_element(void) {
    WriteFmt("Testing clone visits every source element\n");

    DefaultAllocator local = DefaultAllocatorInit();

    typedef Vec(int) IntVec;
    IntVec src      = VecInit(&local);
    int    values[] = {1, 2, 3};
    for (int i = 0; i < 3; i++) {
        VecPushBackR(&src, values[i]);
    }

    IntVec dst = VecInit(&local);
    VecInitClone(&dst, &src);

    // Real: dst length 3, all values present. Mutant (i--): dst length 1.
    bool result = (VecLen(&dst) == 3) && (VecAt(&dst, 0) == 1) && (VecAt(&dst, 1) == 2) && (VecAt(&dst, 2) == 3);

    VecDeinit(&src);
    VecDeinit(&dst);
    DefaultAllocatorDeinit(&local);
    return result;
}

// clone_vec @ 210:14 (insert_range_into_vec(...) call replaced by 0).
bool test_clone_insert_populates_dst(void) {
    WriteFmt("Testing clone appends each element to dst\n");

    DefaultAllocator local = DefaultAllocatorInit();

    typedef Vec(int) IntVec;
    IntVec src      = VecInit(&local);
    int    values[] = {1, 2, 3};
    for (int i = 0; i < 3; i++) {
        VecPushBackR(&src, values[i]);
    }

    IntVec dst    = VecInit(&local);
    bool   cloned = VecInitClone(&dst, &src);

    // Real: cloned true, dst length 3. Mutant: insert expression is 0 so the
    // clone returns false with dst empty.
    bool result = cloned && (VecLen(&dst) == 3);

    VecDeinit(&src);
    VecDeinit(&dst);
    DefaultAllocatorDeinit(&local);
    return result;
}

// Main function that runs all tests
int main(void) {
    alloc = DefaultAllocatorInit();
    WriteFmt("[INFO] Starting Vec.Complex tests\n\n");

    // Array of test functions
    TestFunction tests[] = {
        test_complex_vec_init,
        test_complex_vec_push,
        test_complex_vec_insert,
        test_complex_vec_merge,
        test_lvalue_operations,
        test_fast_operations,
        test_delete_operations,
        test_edge_cases,
        test_lvalue_zero_on_take_pushback,
        test_lvalue_zero_on_take_insert,
        test_lvalue_zero_on_take_fast_insert,
        test_lvalue_zero_on_take_pushfront,
        test_lvalue_zero_on_take_merge,
        test_lvalue_zero_on_take_array_ops,
        // clone_vec (deep-copy) mutation-hardening (Vec.Mutants6)
        test_clone_copies_first_element,
        test_clone_includes_index_zero,
        test_clone_loop_runs_for_nonempty,
        test_clone_visits_every_element,
        test_clone_insert_populates_dst
    };

    int total_tests = sizeof(tests) / sizeof(tests[0]);

    // Run all tests using the centralized test driver
    int rc = run_test_suite(tests, total_tests, NULL, 0, "Vec.Complex");
    DefaultAllocatorDeinit(&alloc);
    return rc;
}
