#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Container/Vec.h>
#include <Misra/Std/Log.h>
#include <Misra/Types.h> // For LVAL macro

// Include test utilities
#include "../Util/TestRunner.h"

// Function prototypes
bool test_vec_swap_items(void);
bool test_vec_reverse(void);
bool test_vec_sort(void);

// ---- swap_vec upper-bound deadend (Vec.Mutants4) ------------------------
bool test_swap_idx_equal_length_aborts(void);

// ---- swap_vec first-index upper-bound deadend (Vec.Mut) -----------------
bool test_swap_idx1_equal_length_aborts(void);

// Comparison function for sorting integers in ascending order
int compare_ints_asc(const void *a, const void *b) {
    int val_a = *(const int *)a;
    int val_b = *(const int *)b;
    return val_a - val_b;
}

// Comparison function for sorting integers in descending order
int compare_ints_desc(const void *a, const void *b) {
    int val_a = *(const int *)a;
    int val_b = *(const int *)b;
    return val_b - val_a;
}

// Test VecSwapItems function
static DefaultAllocator alloc;

bool test_vec_swap_items(void) {
    WriteFmt("Testing VecSwapItems\n");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Add some data
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++) {
        VecPushBackR(&vec, values[i]);
    }

    // Swap first and last elements
    VecSwapItems(&vec, 0, 4);

    // Check that the elements were swapped
    bool result = (VecAt(&vec, 0) == 50 && VecAt(&vec, 4) == 10);

    // Swap two elements in the middle
    VecSwapItems(&vec, 1, 3);

    // Check that the elements were swapped
    result = result && (VecAt(&vec, 1) == 40 && VecAt(&vec, 3) == 20);

    // Check that the middle element is unchanged
    result = result && (VecAt(&vec, 2) == 30);

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Test VecReverse function
bool test_vec_reverse(void) {
    WriteFmt("Testing VecReverse\n");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Add some data
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++) {
        VecPushBackR(&vec, values[i]);
    }

    // Reverse the vector
    VecReverse(&vec);

    // Check that the elements are reversed
    bool result = true;
    for (size i = 0; i < VecLen(&vec); i++) {
        result = result && (VecAt(&vec, i) == values[4 - i]);
    }

    // Reverse again to get back to the original order
    VecReverse(&vec);

    // Check that the elements are back in the original order
    for (size i = 0; i < VecLen(&vec); i++) {
        result = result && (VecAt(&vec, i) == values[i]);
    }

    // Test with a vector of even length
    VecClear(&vec);
    int even_values[] = {10, 20, 30, 40};
    for (int i = 0; i < 4; i++) {
        VecPushBackR(&vec, even_values[i]);
    }

    // Reverse the vector
    VecReverse(&vec);

    // Check that the elements are reversed
    for (size i = 0; i < VecLen(&vec); i++) {
        result = result && (VecAt(&vec, i) == even_values[3 - i]);
    }

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Test VecSort function
bool test_vec_sort(void) {
    WriteFmt("Testing VecSort\n");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Add some data in unsorted order
    int values[] = {30, 10, 50, 20, 40};
    for (int i = 0; i < 5; i++) {
        VecPushBackR(&vec, values[i]);
    }

    // Sort the vector in ascending order
    VecSort(&vec, compare_ints_asc);

    // Check that the elements are sorted
    bool result       = true;
    int  sorted_asc[] = {10, 20, 30, 40, 50};
    for (size i = 0; i < VecLen(&vec); i++) {
        result = result && (VecAt(&vec, i) == sorted_asc[i]);
    }

    // Sort the vector in descending order
    VecSort(&vec, compare_ints_desc);

    // Check that the elements are sorted in descending order
    int sorted_desc[] = {50, 40, 30, 20, 10};
    for (size i = 0; i < VecLen(&vec); i++) {
        result = result && (VecAt(&vec, i) == sorted_desc[i]);
    }

    // Clean up
    VecDeinit(&vec);

    return result;
}

// swap_vec @ 476:37 cxx_ge_to_gt (`idx2 >= vec->length` -> `idx2 > vec->length`):
// index equal to length (the sentinel slot, out of bounds) wrongly passes. Real
// code LOG_FATALs on idx2 == length.
bool test_swap_idx_equal_length_aborts(void) {
    WriteFmt("Testing swap rejects idx == length\n");

    typedef Vec(u32) U32Vec;
    U32Vec vec = VecInit(&alloc);
    for (u32 i = 0; i < 3; i++) {
        VecPushBackR(&vec, i);
    }

    VecSwapItems(&vec, 0, 3); // idx2 == length -> must abort

    // Unreachable on real code.
    VecDeinit(&vec);
    return false;
}

// ---- 476:14 cxx_ge_to_gt -------------------------------------------------
// swap_vec, lower bound on the FIRST index:
//   if (idx1 >= vec->length || idx2 >= vec->length)  -- the `idx1 >=` becomes
//   `idx1 >`. The existing Vec.Ops test covers the idx2 comparison (col 37) by
//   passing idx1 == 0 (in range); it never exercises idx1's boundary, so the
//   col-14 mutant survives it. Drive idx1 == length (the out-of-bounds sentinel
//   slot) with idx2 == 0 (in range). Real: idx1 >= length aborts. Mutant:
//   idx1 > length is false and idx2 >= length is false, so the guard passes and
//   it swaps an out-of-bounds slot -- so the abort must come from real code.
bool test_swap_idx1_equal_length_aborts(void) {
    WriteFmt("Testing swap rejects idx1 == length (476:14)\n");

    typedef Vec(u32) U32Vec;
    U32Vec vec = VecInit(&alloc);
    for (u32 i = 0; i < 3; i++) {
        VecPushBackR(&vec, i);
    }

    VecSwapItems(&vec, 3, 0); // idx1 == length -> must abort

    // Unreachable on real code.
    VecDeinit(&vec);
    return false;
}

// Main function that runs all tests
int main(void) {
    alloc = DefaultAllocatorInit();
    WriteFmt("[INFO] Starting Vec.Ops tests\n\n");

    // Array of test functions
    TestFunction tests[] = {test_vec_swap_items, test_vec_reverse, test_vec_sort};

    TestFunction deadend_tests[] = {test_swap_idx_equal_length_aborts, test_swap_idx1_equal_length_aborts};

    int total_tests   = sizeof(tests) / sizeof(tests[0]);
    int deadend_count = sizeof(deadend_tests) / sizeof(deadend_tests[0]);

    // Run all tests using the centralized test driver
    int rc = run_test_suite(tests, total_tests, deadend_tests, deadend_count, "Vec.Ops");
    DefaultAllocatorDeinit(&alloc);
    return rc;
}
