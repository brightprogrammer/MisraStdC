#include <Misra/Std/Container/Vec.h>
#include <Misra/Std/Log.h>
#include <stdio.h>
#include <Misra/Types.h>

// Include test utilities
#include "../Util/TestRunner.h"

// Function prototypes
bool test_vec_at(void);
bool test_vec_ptr_at(void);
bool test_vec_first_last(void);
bool test_vec_begin_end(void);
bool test_vec_size_len(void);
bool test_vec_aligned_offset_at(void);

// Test VecAt function
bool test_vec_at(void) {
    WriteFmt("Testing VecAt\n");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit();

    // Add some data
    VecPushBackR(&vec, 10);
    VecPushBackR(&vec, 20);
    VecPushBackR(&vec, 30);
    VecPushBackR(&vec, 40);
    VecPushBackR(&vec, 50);

    // Check values using VecAt
    bool result = (VecAt(&vec, 0) == 10);
    result      = result && (VecAt(&vec, 1) == 20);
    result      = result && (VecAt(&vec, 2) == 30);
    result      = result && (VecAt(&vec, 3) == 40);
    result      = result && (VecAt(&vec, 4) == 50);

    // Modify a value using VecAt
    VecAt(&vec, 2) = 35;
    result         = result && (VecAt(&vec, 2) == 35);

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Test VecPtrAt function
bool test_vec_ptr_at(void) {
    WriteFmt("Testing VecPtrAt\n");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit();

    // Add some data
    VecPushBackR(&vec, 10);
    VecPushBackR(&vec, 20);
    VecPushBackR(&vec, 30);

    // Get pointers to elements
    int *ptr0 = VecPtrAt(&vec, 0);
    int *ptr1 = VecPtrAt(&vec, 1);
    int *ptr2 = VecPtrAt(&vec, 2);

    // Check values through pointers
    bool result = (*ptr0 == 10);
    result      = result && (*ptr1 == 20);
    result      = result && (*ptr2 == 30);

    // Modify values through pointers
    *ptr0 = 15;
    *ptr2 = 35;

    // Verify changes
    result = result && (VecAt(&vec, 0) == 15);
    result = result && (VecAt(&vec, 2) == 35);

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Test VecFirst and VecLast functions
bool test_vec_first_last(void) {
    WriteFmt("Testing VecFirst and VecLast\n");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit();

    // Add some data
    VecPushBackR(&vec, 10);
    VecPushBackR(&vec, 20);
    VecPushBackR(&vec, 30);

    // Check first and last elements
    bool result = (VecFirst(&vec) == 10);
    result      = result && (VecLast(&vec) == 30);

    // Modify first and last elements
    VecFirst(&vec) = 15;
    VecLast(&vec)  = 35;

    // Verify changes
    result = result && (VecAt(&vec, 0) == 15);
    result = result && (VecAt(&vec, 2) == 35);

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Test VecBegin and VecEnd functions
bool test_vec_begin_end(void) {
    WriteFmt("Testing VecBegin and VecEnd\n");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit();

    // Add some data
    VecPushBackR(&vec, 10);
    VecPushBackR(&vec, 20);
    VecPushBackR(&vec, 30);

    // Get begin and end pointers
    int  *begin = VecBegin(&vec);
    char *end   = (char *)VecEnd(&vec);

    // Check that begin points to the first element
    bool result = (*begin == 10);

    // Check that end - begin equals the size of the vector
    size vec_size = VecSize(&vec);
    result        = result && ((end - (char *)begin) == vec_size);

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Test VecSize and VecLen functions
bool test_vec_size_len(void) {
    WriteFmt("Testing VecSize and VecLen\n");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit();

    // Check initial size and length
    size vec_size = VecSize(&vec);
    size vec_len  = VecLen(&vec);
    bool result   = (vec_size == 0);
    result        = result && (vec_len == 0);

    // Add some data
    VecPushBackR(&vec, 10);
    VecPushBackR(&vec, 20);
    VecPushBackR(&vec, 30);

    // Check size and length after adding elements
    vec_size            = VecSize(&vec);
    vec_len             = VecLen(&vec);
    size aligned_offset = VecAlignedOffsetAt(&vec, vec.length);
    result              = result && (vec_size == aligned_offset);
    result              = result && (vec_len == vec.length);

    // Clean up
    VecDeinit(&vec);

    // Test with a vector with alignment > 1
    typedef Vec(int) AlignedIntVec;
    AlignedIntVec aligned_vec = VecInitAligned(8);

    // Add some data
    VecPushBackR(&aligned_vec, 10);
    VecPushBackR(&aligned_vec, 20);

    // Check size and length with alignment
    size aligned_vec_size  = VecSize(&aligned_vec);
    size aligned_vec_len   = VecLen(&aligned_vec);
    size aligned_offset_at = VecAlignedOffsetAt(&aligned_vec, aligned_vec.length);
    result                 = result && (aligned_vec_size == aligned_offset_at);
    result                 = result && (aligned_vec_len == aligned_vec.length);

    // Clean up
    VecDeinit(&aligned_vec);

    return result;
}

// Test VecAlignedOffsetAt function
bool test_vec_aligned_offset_at(void) {
    WriteFmt("Testing VecAlignedOffsetAt\n");

    // Create a vector of integers with default alignment (1)
    typedef Vec(int) IntVec;
    IntVec vec = VecInit();

    // Check offsets
    bool result = (VecAlignedOffsetAt(&vec, 0) == 0);
    result      = result && (VecAlignedOffsetAt(&vec, 1) == sizeof(int));
    result      = result && (VecAlignedOffsetAt(&vec, 2) == 2 * sizeof(int));

    // Clean up
    VecDeinit(&vec);

    // Create a vector with 8-byte alignment
    typedef Vec(int) AlignedIntVec;
    AlignedIntVec aligned_vec = VecInitAligned(8);

    // For 8-byte alignment, each int (4 bytes) should be padded to 8 bytes
    size aligned_size = ALIGN_UP(sizeof(int), 8);

    // Check offsets with alignment
    result = result && (VecAlignedOffsetAt(&aligned_vec, 0) == 0);
    result = result && (VecAlignedOffsetAt(&aligned_vec, 1) == aligned_size);
    result = result && (VecAlignedOffsetAt(&aligned_vec, 2) == 2 * aligned_size);

    // Clean up
    VecDeinit(&aligned_vec);

    return result;
}

// Main function that runs all tests
int main(void) {
    WriteFmt("[INFO] Starting Vec.Access tests\n\n");

    // Array of test functions
    TestFunction tests[] = {
        test_vec_at,
        test_vec_ptr_at,
        test_vec_first_last,
        test_vec_begin_end,
        test_vec_size_len,
        test_vec_aligned_offset_at
    };

    int total_tests = sizeof(tests) / sizeof(tests[0]);

    // Run all tests using the centralized test driver
    return run_test_suite(tests, total_tests, NULL, 0, "Vec.Access");
}
