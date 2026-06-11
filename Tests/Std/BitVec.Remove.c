#include <Misra/Std/Container/BitVec.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Log.h>

#include <Misra/Types.h>

// Include test utilities
#include "../Util/TestRunner.h"

// Function prototypes
bool test_bitvec_pop(void);
bool test_bitvec_remove_single(void);
bool test_bitvec_remove_range(void);
bool test_bitvec_remove_first(void);
bool test_bitvec_remove_last(void);
bool test_bitvec_remove_all(void);
bool test_bitvec_pop_edge_cases(void);
bool test_bitvec_remove_single_edge_cases(void);
bool test_bitvec_remove_range_edge_cases(void);
bool test_bitvec_remove_first_last_edge_cases(void);
bool test_bitvec_remove_all_edge_cases(void);
bool test_bitvec_remove_null_failures(void);
bool test_bitvec_remove_range_null_failures(void);
bool test_bitvec_remove_invalid_range_failures(void);
bool test_remove_range_clamps_oversized_count(void);
bool test_remove_range_clamp_gap_count(void);
bool test_remove_range_shifts_tail_down(void);
bool test_remove_null_aborts(void);
bool test_remove_at_length_aborts(void);
bool test_remove_first_null_aborts(void);
bool test_clear_null_aborts(void);

// Test BitVecPop function
bool test_bitvec_pop(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecPop\n");

    BitVec bv = BitVecInit(ALLOCATOR_OF(&alloc));

    // Add some bits
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecPush(&bv, true);

    // Pop the last bit
    bool popped = BitVecPop(&bv);

    // Check result
    bool result = (popped == true) && (BitVecLen(&bv) == 2);
    result      = result && (BitVecGet(&bv, 0) == true);
    result      = result && (BitVecGet(&bv, 1) == false);

    // Pop another bit
    popped = BitVecPop(&bv);
    result = result && (popped == false) && (BitVecLen(&bv) == 1);
    result = result && (BitVecGet(&bv, 0) == true);

    // Pop the last bit
    popped = BitVecPop(&bv);
    result = result && (popped == true) && (BitVecLen(&bv) == 0);

    // Clean up
    BitVecDeinit(&bv);

    DefaultAllocatorDeinit(&alloc);

    return result;
}

// Test BitVecRemove single bit function
bool test_bitvec_remove_single(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecRemove (single bit)\n");

    BitVec bv = BitVecInit(ALLOCATOR_OF(&alloc));

    // Add some bits: true, false, true, false, true
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecPush(&bv, true);

    // Remove bit at index 2 (middle true)
    bool removed = BitVecRemove(&bv, 2);

    // Check result: true, false, false, true
    bool result = (removed == true) && (BitVecLen(&bv) == 4);
    result      = result && (BitVecGet(&bv, 0) == true);
    result      = result && (BitVecGet(&bv, 1) == false);
    result      = result && (BitVecGet(&bv, 2) == false);
    result      = result && (BitVecGet(&bv, 3) == true);

    // Remove bit at index 0 (first bit)
    removed = BitVecRemove(&bv, 0);
    result  = result && (removed == true) && (BitVecLen(&bv) == 3);
    result  = result && (BitVecGet(&bv, 0) == false);
    result  = result && (BitVecGet(&bv, 1) == false);
    result  = result && (BitVecGet(&bv, 2) == true);

    // Clean up
    BitVecDeinit(&bv);

    DefaultAllocatorDeinit(&alloc);

    return result;
}

// Test BitVecRemoveRange function
bool test_bitvec_remove_range(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecRemoveRange\n");

    BitVec bv = BitVecInit(ALLOCATOR_OF(&alloc));

    // Add some bits: true, false, true, true, false, true
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecPush(&bv, true);
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecPush(&bv, true);

    // Remove range from index 1 to 3 (3 bits)
    BitVecRemoveRange(&bv, 1, 3);

    // Check result: true, false, true (removed false, true, true)
    bool result = (BitVecLen(&bv) == 3);
    result      = result && (BitVecGet(&bv, 0) == true);
    result      = result && (BitVecGet(&bv, 1) == false);
    result      = result && (BitVecGet(&bv, 2) == true);

    // Clean up
    BitVecDeinit(&bv);

    DefaultAllocatorDeinit(&alloc);

    return result;
}

// Test BitVecRemoveFirst function
bool test_bitvec_remove_first(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecRemoveFirst\n");

    BitVec bv = BitVecInit(ALLOCATOR_OF(&alloc));

    // Add some bits: true, false, true, false, true
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecPush(&bv, true);

    // Remove first occurrence of false
    bool found = BitVecRemoveFirst(&bv, false);

    // Check result: true, true, false, true (removed first false at index 1)
    bool result = (found == true) && (BitVecLen(&bv) == 4);
    result      = result && (BitVecGet(&bv, 0) == true);
    result      = result && (BitVecGet(&bv, 1) == true);
    result      = result && (BitVecGet(&bv, 2) == false);
    result      = result && (BitVecGet(&bv, 3) == true);

    // Try to remove first occurrence of a value that doesn't exist (after removal)
    // Actually, false still exists at index 2, so let's remove all falses first
    BitVecRemoveFirst(&bv, false); // Remove the remaining false

    // Now try to remove false from a bitvector with only trues
    found  = BitVecRemoveFirst(&bv, false);
    result = result && (found == false) && (BitVecLen(&bv) == 3);

    // Clean up
    BitVecDeinit(&bv);

    DefaultAllocatorDeinit(&alloc);

    return result;
}

// Test BitVecRemoveLast function
bool test_bitvec_remove_last(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecRemoveLast\n");

    BitVec bv = BitVecInit(ALLOCATOR_OF(&alloc));

    // Add some bits: true, false, true, false, true
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecPush(&bv, true);

    // Remove last occurrence of false
    bool found = BitVecRemoveLast(&bv, false);

    // Check result: true, false, true, true (removed last false at index 3)
    bool result = (found == true) && (BitVecLen(&bv) == 4);
    result      = result && (BitVecGet(&bv, 0) == true);
    result      = result && (BitVecGet(&bv, 1) == false);
    result      = result && (BitVecGet(&bv, 2) == true);
    result      = result && (BitVecGet(&bv, 3) == true);

    // Remove last occurrence of true
    found = BitVecRemoveLast(&bv, true);

    // Check result: true, false, true (removed last true at index 3)
    result = result && (found == true) && (BitVecLen(&bv) == 3);
    result = result && (BitVecGet(&bv, 0) == true);
    result = result && (BitVecGet(&bv, 1) == false);
    result = result && (BitVecGet(&bv, 2) == true);

    // Clean up
    BitVecDeinit(&bv);

    DefaultAllocatorDeinit(&alloc);

    return result;
}

// Test BitVecRemoveAll function
bool test_bitvec_remove_all(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecRemoveAll\n");

    BitVec bv = BitVecInit(ALLOCATOR_OF(&alloc));

    // Add some bits: true, false, true, false, true, false
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);

    // Remove all false bits
    u64 removed_count = BitVecRemoveAll(&bv, false);

    // Check result: true, true, true (all false bits removed)
    bool result = (removed_count == 3) && (BitVecLen(&bv) == 3);
    result      = result && (BitVecGet(&bv, 0) == true);
    result      = result && (BitVecGet(&bv, 1) == true);
    result      = result && (BitVecGet(&bv, 2) == true);

    // Try to remove all false bits again (should return 0)
    removed_count = BitVecRemoveAll(&bv, false);
    result        = result && (removed_count == 0) && (BitVecLen(&bv) == 3);

    // Remove all true bits
    removed_count = BitVecRemoveAll(&bv, true);
    result        = result && (removed_count == 3) && (BitVecLen(&bv) == 0);

    // Clean up
    BitVecDeinit(&bv);

    DefaultAllocatorDeinit(&alloc);

    return result;
}

// Edge case tests
bool test_bitvec_pop_edge_cases(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecPop edge cases\n");

    BitVec bv     = BitVecInit(ALLOCATOR_OF(&alloc));
    bool   result = true;

    // Test pop single element
    BitVecPush(&bv, true);
    bool popped = BitVecPop(&bv);
    result      = result && (popped == true) && (BitVecLen(&bv) == 0);

    // Test multiple pops in sequence
    for (int i = 0; i < 100; i++) {
        BitVecPush(&bv, i % 2 == 0);
    }
    for (int i = 99; i >= 0; i--) {
        popped = BitVecPop(&bv);
        result = result && (popped == (i % 2 == 0));
        result = result && (BitVecLen(&bv) == (size)i);
    }

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_bitvec_remove_single_edge_cases(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecRemove edge cases\n");

    BitVec bv     = BitVecInit(ALLOCATOR_OF(&alloc));
    bool   result = true;

    // Test remove last element
    BitVecPush(&bv, true);
    bool removed = BitVecRemove(&bv, 0);
    result       = result && (removed == true) && (BitVecLen(&bv) == 0);

    // Test remove from large bitvec
    for (int i = 0; i < 1000; i++) {
        BitVecPush(&bv, i % 3 == 0);
    }

    // Remove middle element
    removed = BitVecRemove(&bv, 500);
    result  = result && (removed == (500 % 3 == 0)); // Should return the value of the removed bit
    result  = result && (BitVecLen(&bv) == 999);

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_bitvec_remove_range_edge_cases(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecRemoveRange edge cases\n");

    BitVec bv     = BitVecInit(ALLOCATOR_OF(&alloc));
    bool   result = true;

    // Test remove 0 elements (should be no-op)
    BitVecPush(&bv, true);
    BitVecRemoveRange(&bv, 0, 0);
    result = result && (BitVecLen(&bv) == 1);

    // Test remove entire bitvec
    BitVecClear(&bv);
    for (int i = 0; i < 10; i++) {
        BitVecPush(&bv, i % 2 == 0);
    }
    BitVecRemoveRange(&bv, 0, 10);
    result = result && (BitVecLen(&bv) == 0);

    // Test remove partial range
    for (int i = 0; i < 10; i++) {
        BitVecPush(&bv, i % 2 == 0);
    }
    BitVecRemoveRange(&bv, 1, 5);             // Remove 5 elements starting at index 1
    result = result && (BitVecLen(&bv) == 5); // Should have 5 elements left

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_bitvec_remove_first_last_edge_cases(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecRemoveFirst/Last edge cases\n");

    BitVec bv     = BitVecInit(ALLOCATOR_OF(&alloc));
    bool   result = true;

    // Test remove from empty bitvec
    bool found = BitVecRemoveFirst(&bv, true);
    result     = result && (found == false) && (BitVecLen(&bv) == 0);

    found  = BitVecRemoveLast(&bv, false);
    result = result && (found == false) && (BitVecLen(&bv) == 0);

    // Test remove when value doesn't exist
    BitVecPush(&bv, true);
    BitVecPush(&bv, true);
    found  = BitVecRemoveFirst(&bv, false);
    result = result && (found == false) && (BitVecLen(&bv) == 2);

    // Test remove single occurrence
    BitVecClear(&bv);
    BitVecPush(&bv, false);
    found  = BitVecRemoveFirst(&bv, false);
    result = result && (found == true) && (BitVecLen(&bv) == 0);

    // Test remove from large uniform data
    for (int i = 0; i < 1000; i++) {
        BitVecPush(&bv, true);
    }
    found  = BitVecRemoveFirst(&bv, true);
    result = result && (found == true) && (BitVecLen(&bv) == 999);

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_bitvec_remove_all_edge_cases(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecRemoveAll edge cases\n");

    BitVec bv     = BitVecInit(ALLOCATOR_OF(&alloc));
    bool   result = true;

    // Test remove all from empty bitvec
    u64 count = BitVecRemoveAll(&bv, true);
    result    = result && (count == 0) && (BitVecLen(&bv) == 0);

    // Test remove all when value doesn't exist
    BitVecPush(&bv, true);
    BitVecPush(&bv, true);
    count  = BitVecRemoveAll(&bv, false);
    result = result && (count == 0) && (BitVecLen(&bv) == 2);

    // Test remove all of uniform data
    BitVecClear(&bv);
    for (int i = 0; i < 100; i++) {
        BitVecPush(&bv, true);
    }
    count  = BitVecRemoveAll(&bv, true);
    result = result && (count == 100) && (BitVecLen(&bv) == 0);

    // Test remove all mixed data
    for (int i = 0; i < 1000; i++) {
        BitVecPush(&bv, i % 2 == 0);
    }
    count  = BitVecRemoveAll(&bv, false); // Remove odds
    result = result && (count == 500) && (BitVecLen(&bv) == 500);

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Deadend tests
bool test_bitvec_remove_null_failures(void) {
    WriteFmt("Testing BitVec remove NULL pointer handling\n");

    // Test NULL bitvec pointer - should abort
    BitVecPop(NULL);

    return false;
}

bool test_bitvec_remove_range_null_failures(void) {
    WriteFmt("Testing BitVec remove range NULL handling\n");

    // Test NULL bitvec pointer - should abort
    BitVecRemoveRange(NULL, 0, 1);

    return false;
}

bool test_bitvec_remove_invalid_range_failures(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVec remove invalid range handling\n");

    BitVec bv = BitVecInit(ALLOCATOR_OF(&alloc));

    // Test removing beyond capacity limit - should abort
    BitVecRemoveRange(&bv, SIZE_MAX, 1);

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

bool test_bitvec_pop_bounds_failures(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVec pop bounds checking\n");

    BitVec bv = BitVecInit(ALLOCATOR_OF(&alloc));

    // Test pop from empty bitvec - should abort
    BitVecPop(&bv);

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

bool test_bitvec_remove_bounds_failures(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVec remove bounds checking\n");

    BitVec bv = BitVecInit(ALLOCATOR_OF(&alloc));

    // Test remove from empty bitvec - should abort
    BitVecRemove(&bv, 0);

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

bool test_bitvec_remove_range_bounds_failures(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVec remove range bounds checking\n");

    BitVec bv = BitVecInit(ALLOCATOR_OF(&alloc));

    // Test remove range from empty bitvec - should abort
    BitVecRemoveRange(&bv, 0, 1);

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// 452:28 / 453:15 / 453:28 -- BitVecRemoveRange must clamp an oversized count to
// the number of bits actually remaining from idx. A wrong threshold or wrong
// clamp value over-removes (final length wrong / underflow).
bool test_remove_range_clamps_oversized_count(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecRemoveRange clamps oversized count\n");

    BitVec bv = BitVecInit(ALLOCATOR_OF(&alloc));

    // Length 10, all true.
    for (int i = 0; i < 10; i++) {
        BitVecPush(&bv, true);
    }

    // Remove starting at idx 2 with a count far larger than what remains.
    // Only 8 bits remain (indices 2..9), so result length must be 2.
    BitVecRemoveRange(&bv, 2, 100);

    bool result = (BitVecLen(&bv) == 2);
    result      = result && (BitVecGet(&bv, 0) == true);
    result      = result && (BitVecGet(&bv, 1) == true);

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 452:28 cxx_sub_to_add -- the clamp condition `count > bv->length - idx`
// becoming `count > bv->length + idx`. The existing oversized-count test uses
// count=100 which trips both forms, so it cannot see the mutation. This picks a
// count strictly inside the gap (length-idx < count <= length+idx): length=10,
// idx=2 -> real clamps when count>8, mutant only when count>12. With count=10
// the real code clamps to 8 (result length 2), the mutant skips the clamp and
// resizes to length-count = 0. Caller-observable surviving length.
bool test_remove_range_clamp_gap_count(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecRemoveRange clamp gap count\n");

    BitVec bv = BitVecInit(ALLOCATOR_OF(&alloc));

    // Length 10, all true.
    for (int i = 0; i < 10; i++) {
        BitVecPush(&bv, true);
    }

    // idx=2, count=10: 8 bits remain from idx 2, so the result must be length 2.
    BitVecRemoveRange(&bv, 2, 10);

    bool result = (BitVecLen(&bv) == 2);
    result      = result && (BitVecGet(&bv, 0) == true);
    result      = result && (BitVecGet(&bv, 1) == true);

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 456:14 init_const / 458:9 remove_void_call -- BitVecRemoveRange must shift the
// tail (bits after the removed region) down by `count`. A bad loop start or a
// dropped Set leaves the surviving bits in the wrong positions.
bool test_remove_range_shifts_tail_down(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecRemoveRange shifts tail down\n");

    BitVec bv = BitVecInit(ALLOCATOR_OF(&alloc));

    // Pattern: indices 0..9 = [1,0,1,0,0,1,0,1,1,0]
    bool pattern[] = {true, false, true, false, false, true, false, true, true, false};
    for (int i = 0; i < 10; i++) {
        BitVecPush(&bv, pattern[i]);
    }

    // Remove 3 bits starting at index 2 -> removes indices 2,3,4.
    // Survivors: [1,0] ++ [1,0,1,1,0] = [1,0,1,0,1,1,0], length 7.
    BitVecRemoveRange(&bv, 2, 3);

    bool result = (BitVecLen(&bv) == 7);
    result      = result && (BitVecGet(&bv, 0) == true);  // orig[0]
    result      = result && (BitVecGet(&bv, 1) == false); // orig[1]
    result      = result && (BitVecGet(&bv, 2) == true);  // orig[5]
    result      = result && (BitVecGet(&bv, 3) == false); // orig[6]
    result      = result && (BitVecGet(&bv, 4) == true);  // orig[7]
    result      = result && (BitVecGet(&bv, 5) == true);  // orig[8]
    result      = result && (BitVecGet(&bv, 6) == false); // orig[9]

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 424:5 remove_void_call -- BitVecRemove must validate its bitvec argument.
bool test_remove_null_aborts(void) {
    WriteFmt("Testing BitVecRemove NULL validation\n");

    BitVecRemove(NULL, 0);

    // If we reach here, validation did not abort.
    return false;
}

// 425:13 ge_to_gt -- BitVecRemove must reject idx == length (out of range).
// With >= relaxed to >, removing at idx == length is wrongly accepted.
bool test_remove_at_length_aborts(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecRemove rejects idx == length\n");

    BitVec bv = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecPush(&bv, true);

    // length is 3; idx 3 is out of range and must abort.
    BitVecRemove(&bv, 3);

    // If we reach here, validation did not abort.
    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// 465:5 remove_void_call -- BitVecRemoveFirst must validate its bitvec argument.
bool test_remove_first_null_aborts(void) {
    WriteFmt("Testing BitVecRemoveFirst NULL validation\n");

    BitVecRemoveFirst(NULL, true);

    // If we reach here, validation did not abort.
    return false;
}

// Kills 99:cxx_remove_void_call. BitVecClear's first statement is the
// function-level ValidateBitVec; a NULL handle must abort. Removing the
// validator would let the NULL deref slip silently.
bool test_clear_null_aborts(void) {
    BitVecClear((BitVec *)NULL);
    return false;
}

// Main function that runs all tests
int main(void) {
    WriteFmt("[INFO] Starting BitVec.Remove tests\n\n");

    // Array of normal test functions
    TestFunction tests[] = {
        test_bitvec_pop,
        test_bitvec_remove_single,
        test_bitvec_remove_range,
        test_bitvec_remove_first,
        test_bitvec_remove_last,
        test_bitvec_remove_all,
        test_bitvec_pop_edge_cases,
        test_bitvec_remove_single_edge_cases,
        test_bitvec_remove_range_edge_cases,
        test_bitvec_remove_first_last_edge_cases,
        test_bitvec_remove_all_edge_cases,
        test_remove_range_clamps_oversized_count,
        test_remove_range_clamp_gap_count,
        test_remove_range_shifts_tail_down
    };

    // Array of deadend test functions
    TestFunction deadend_tests[] = {
        test_bitvec_remove_null_failures,
        test_bitvec_remove_range_null_failures,
        test_bitvec_remove_invalid_range_failures,
        test_bitvec_pop_bounds_failures,
        test_bitvec_remove_bounds_failures,
        test_bitvec_remove_range_bounds_failures,
        test_remove_null_aborts,
        test_remove_at_length_aborts,
        test_remove_first_null_aborts,
        test_clear_null_aborts
    };

    int total_tests         = sizeof(tests) / sizeof(tests[0]);
    int total_deadend_tests = sizeof(deadend_tests) / sizeof(deadend_tests[0]);

    // Run all tests using the centralized test driver
    return run_test_suite(tests, total_tests, deadend_tests, total_deadend_tests, "BitVec.Remove");
}
