#include <Misra/Std/Container/BitVec.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Log.h>
#include <Misra/Types.h>

// Include test utilities
#include "../../Util/TestRunner.h"

// Function prototypes
bool test_bitvec_shrink_to_fit(void);
bool test_bitvec_reserve(void);
bool test_reserve_exact_capacity(void);
bool test_reserve_byte_size(void);
bool test_bitvec_swap(void);
bool test_bitvec_clone(void);
bool test_bitvec_clone_inherits_allocator_config(void);
bool test_bitvec_shrink_to_fit_edge_cases(void);
bool test_bitvec_reserve_edge_cases(void);
bool test_bitvec_swap_edge_cases(void);
bool test_bitvec_clone_edge_cases(void);
bool test_bitvec_memory_stress_test(void);
bool test_bitvec_memory_null_failures(void);
bool test_bitvec_swap_null_failures(void);
bool test_bitvec_clone_null_failures(void);
bool test_resize_regrow_clears_stale_tail_bits(void);
bool test_reserve_zero_returns_true(void);
bool test_resize_null_aborts(void);
bool test_reserve_null_aborts(void);
bool test_shrink_large_preserves_bits(void);
bool test_shrink_large_stays_valid(void);
bool test_swap_large_into_other_stays_valid(void);
bool test_swap_invalid_second_arg_aborts(void);
bool test_tryclone_invalid_source_aborts(void);

// Test BitVecShrinkToFit function
bool test_bitvec_shrink_to_fit(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecShrinkToFit\n");

    BitVec bv = BitVecInit(ALLOCATOR_OF(&alloc));

    // Add some bits
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecPush(&bv, true);

    // Force capacity to be larger than needed by reserving space
    BitVecReserve(&bv, 100);

    // Check that capacity is larger than length
    u64  initial_capacity = BitVecCapacity(&bv);
    bool result           = (initial_capacity >= 100) && (BitVecLen(&bv) == 3);

    // Shrink to fit
    BitVecShrinkToFit(&bv);

    // Check that capacity is now closer to length
    result = result && (BitVecCapacity(&bv) < initial_capacity);
    result = result && (BitVecCapacity(&bv) >= BitVecLen(&bv));

    // Check that data is still intact
    result = result && (BitVecLen(&bv) == 3);
    result = result && (BitVecGet(&bv, 0) == true);
    result = result && (BitVecGet(&bv, 1) == false);
    result = result && (BitVecGet(&bv, 2) == true);

    // Clean up
    BitVecDeinit(&bv);

    DefaultAllocatorDeinit(&alloc);

    return result;
}

bool test_bitvec_reserve(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecReserve\n");

    BitVec bv = BitVecInit(ALLOCATOR_OF(&alloc));

    // Add some bits
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);

    // Set capacity to a specific value
    BitVecReserve(&bv, 50);

    // Check that capacity was set correctly
    bool result = (BitVecCapacity(&bv) >= 50) && (BitVecLen(&bv) == 2);

    // Check that data is still intact
    result = result && (BitVecGet(&bv, 0) == true);
    result = result && (BitVecGet(&bv, 1) == false);

    // Try to set capacity smaller than current length (should not shrink below length)
    BitVecReserve(&bv, 1);

    // Capacity should still accommodate at least the current length
    result = result && (BitVecCapacity(&bv) >= BitVecLen(&bv));
    result = result && (BitVecLen(&bv) == 2);

    // Data should still be intact
    result = result && (BitVecGet(&bv, 0) == true);
    result = result && (BitVecGet(&bv, 1) == false);

    // Clean up
    BitVecDeinit(&bv);

    DefaultAllocatorDeinit(&alloc);

    return result;
}

// BitVecReserve sets capacity to exactly the requested bit count (not just >=).
bool test_reserve_exact_capacity(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    BitVec bv = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVecReserve(&bv, 64);

    bool result = (BitVecCapacity(&bv) == 64);
    result      = result && (BitVecData(&bv) != NULL);
    result      = result && (BitVecLen(&bv) == 0);

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// BitVecReserve's byte_size is an exact contract: BYTES_FOR_BITS(64) == 8.
bool test_reserve_byte_size(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    BitVec bv = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVecReserve(&bv, 64);

    bool result = (BitVecByteSize(&bv) == 8);

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test BitVecSwap function
bool test_bitvec_swap(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecSwap\n");

    BitVec bv1 = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec bv2 = BitVecInit(ALLOCATOR_OF(&alloc));

    // Set up first bitvector
    BitVecPush(&bv1, true);
    BitVecPush(&bv1, false);
    BitVecPush(&bv1, true);

    // Set up second bitvector
    BitVecPush(&bv2, false);
    BitVecPush(&bv2, false);

    // Store original states
    u64 bv1_orig_length = BitVecLen(&bv1);
    u64 bv2_orig_length = BitVecLen(&bv2);

    // Swap the bitvectors
    BitVecSwap(&bv1, &bv2);

    // Check that they swapped
    bool result = (BitVecLen(&bv1) == bv2_orig_length) && (BitVecLen(&bv2) == bv1_orig_length);

    // Check bv1 (should now have bv2's original content)
    result = result && (BitVecLen(&bv1) == 2);
    result = result && (BitVecGet(&bv1, 0) == false);
    result = result && (BitVecGet(&bv1, 1) == false);

    // Check bv2 (should now have bv1's original content)
    result = result && (BitVecLen(&bv2) == 3);
    result = result && (BitVecGet(&bv2, 0) == true);
    result = result && (BitVecGet(&bv2, 1) == false);
    result = result && (BitVecGet(&bv2, 2) == true);

    // Clean up
    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);

    DefaultAllocatorDeinit(&alloc);

    return result;
}

// Test BitVecClone function
bool test_bitvec_clone(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecClone\n");

    BitVec original = BitVecInit(ALLOCATOR_OF(&alloc));

    // Set up original bitvector
    BitVecPush(&original, true);
    BitVecPush(&original, false);
    BitVecPush(&original, true);
    BitVecPush(&original, false);

    // Clone the bitvector
    BitVec clone = BitVecClone(&original);

    // Check that clone has same content as original
    bool result = (BitVecLen(&clone) == BitVecLen(&original));

    for (u64 i = 0; i < BitVecLen(&original); i++) {
        result = result && (BitVecGet(&clone, i) == BitVecGet(&original, i));
    }

    // Verify they are independent by modifying original
    BitVecPush(&original, true);

    // Clone should remain unchanged
    result = result && (BitVecLen(&clone) == 4) && (BitVecLen(&original) == 5);
    result = result && (BitVecGet(&clone, 0) == true);
    result = result && (BitVecGet(&clone, 1) == false);
    result = result && (BitVecGet(&clone, 2) == true);
    result = result && (BitVecGet(&clone, 3) == false);

    // Modify clone to verify independence
    BitVecSet(&clone, 0, false);

    // Original should remain unchanged at index 0
    result = result && (BitVecGet(&original, 0) == true);
    result = result && (BitVecGet(&clone, 0) == false);

    // Clean up
    BitVecDeinit(&original);
    BitVecDeinit(&clone);

    DefaultAllocatorDeinit(&alloc);

    return result;
}

bool test_bitvec_clone_inherits_allocator_config(void) {
    WriteFmt("Testing BitVecClone allocator inheritance\n");

    DefaultAllocator alloc = DefaultAllocatorInit();
    // intentional bypass: no public setter on `Allocator` for effort /
    // retry_limit -- pre-seeded directly so the inheritance path below
    // can be observed end-to-end.
    alloc.base.effort      = ALLOCATOR_EFFORT_RETRY_FALLBACK;
    alloc.base.retry_limit = 9;

    BitVec original = BitVecInit(ALLOCATOR_OF(&alloc));

    BitVecPush(&original, true);
    BitVecPush(&original, false);
    BitVecPush(&original, true);

    BitVec clone = BitVecClone(&original);

    // Clone should share the same Allocator* and therefore see identical
    // configuration fields on the base allocator.
    bool result = BitVecLen(&clone) == BitVecLen(&original) && BitVecCapacity(&clone) >= BitVecLen(&original) &&
                  BitVecAllocator(&clone) == BitVecAllocator(&original) &&
                  BitVecAllocator(&clone)->allocate == BitVecAllocator(&original)->allocate &&
                  BitVecAllocator(&clone)->remap == BitVecAllocator(&original)->remap &&
                  BitVecAllocator(&clone)->deallocate == BitVecAllocator(&original)->deallocate &&
                  BitVecAllocator(&clone)->effort == BitVecAllocator(&original)->effort &&
                  BitVecAllocator(&clone)->retry_limit == BitVecAllocator(&original)->retry_limit &&
                  BitVecGet(&clone, 0) == true && BitVecGet(&clone, 1) == false && BitVecGet(&clone, 2) == true;

    BitVecDeinit(&original);
    BitVecDeinit(&clone);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Edge case tests
bool test_bitvec_shrink_to_fit_edge_cases(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecShrinkToFit edge cases\n");

    BitVec bv     = BitVecInit(ALLOCATOR_OF(&alloc));
    bool   result = true;

    // Test shrink on empty bitvec
    BitVecShrinkToFit(&bv);
    result = result && (BitVecLen(&bv) == 0) && (BitVecCapacity(&bv) >= 0);

    // Test shrink on single element
    BitVecPush(&bv, true);
    BitVecShrinkToFit(&bv);
    result = result && (BitVecLen(&bv) == 1) && (BitVecCapacity(&bv) >= 1);
    result = result && (BitVecGet(&bv, 0) == true);

    // Test multiple shrinks (should be safe)
    BitVecShrinkToFit(&bv);
    BitVecShrinkToFit(&bv);
    result = result && (BitVecLen(&bv) == 1);

    // Test shrink after reserve and clear
    BitVecReserve(&bv, 1000);
    BitVecClear(&bv);
    BitVecShrinkToFit(&bv);
    result = result && (BitVecLen(&bv) == 0);

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_bitvec_reserve_edge_cases(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecReserve edge cases\n");

    BitVec bv     = BitVecInit(ALLOCATOR_OF(&alloc));
    bool   result = true;

    // Test set capacity on empty bitvec
    BitVecReserve(&bv, 100);
    result = result && (BitVecCapacity(&bv) >= 100) && (BitVecLen(&bv) == 0);

    // BitVecReserve is grow-only; use BitVecClear to drop length to 0.
    BitVecClear(&bv);
    result = result && (BitVecLen(&bv) == 0);

    for (int i = 0; i < 10; i++) {
        BitVecPush(&bv, i % 2 == 0);
    }
    u64 original_length = BitVecLen(&bv);

    result = result && (BitVecLen(&bv) == original_length);
    for (u64 i = 0; i < BitVecLen(&bv); i++) {
        result = result && (BitVecGet(&bv, i) == (i % 2 == 0));
    }

    // Test setting very large capacity
    BitVecReserve(&bv, 10000);
    result = result && (BitVecCapacity(&bv) >= 10000);

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_bitvec_swap_edge_cases(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecSwap edge cases\n");

    BitVec bv1    = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec bv2    = BitVecInit(ALLOCATOR_OF(&alloc));
    bool   result = true;

    // Test swap with both empty
    BitVecSwap(&bv1, &bv2);
    result = result && (BitVecLen(&bv1) == 0) && (BitVecLen(&bv2) == 0);

    // Test swap with one empty, one non-empty
    BitVecPush(&bv1, true);
    BitVecPush(&bv1, false);
    BitVecSwap(&bv1, &bv2);

    result = result && (BitVecLen(&bv1) == 0);
    result = result && (BitVecLen(&bv2) == 2);
    result = result && (BitVecGet(&bv2, 0) == true);
    result = result && (BitVecGet(&bv2, 1) == false);

    // Test swap with large data
    BitVecClear(&bv1);
    for (int i = 0; i < 1000; i++) {
        BitVecPush(&bv1, i % 3 == 0);
    }

    BitVecSwap(&bv1, &bv2);
    result = result && (BitVecLen(&bv1) == 2) && (BitVecLen(&bv2) == 1000);
    result = result && (BitVecGet(&bv2, 0) == true); // 0 % 3 == 0
    result = result && (BitVecGet(&bv2, 999) == (999 % 3 == 0));

    // Test swapping with itself (should be safe)
    BitVecSwap(&bv1, &bv1);
    result = result && (BitVecLen(&bv1) == 2);

    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_bitvec_clone_edge_cases(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecClone edge cases\n");

    BitVec bv     = BitVecInit(ALLOCATOR_OF(&alloc));
    bool   result = true;

    // Test clone empty bitvec
    BitVec clone1 = BitVecClone(&bv);
    result        = result && (BitVecLen(&clone1) == 0);
    BitVecDeinit(&clone1);

    // Test clone single element
    BitVecPush(&bv, true);
    BitVec clone2 = BitVecClone(&bv);
    result        = result && (BitVecLen(&clone2) == 1);
    result        = result && (BitVecGet(&clone2, 0) == true);
    BitVecDeinit(&clone2);

    // Test clone large data
    BitVecClear(&bv);
    for (int i = 0; i < 1000; i++) {
        BitVecPush(&bv, i % 2 == 0);
    }

    BitVec clone3 = BitVecClone(&bv);
    result        = result && (BitVecLen(&clone3) == 1000);

    // Verify all bits match
    for (u64 i = 0; i < 1000; i++) {
        result = result && (BitVecGet(&clone3, i) == BitVecGet(&bv, i));
    }

    // Test independence - modify original
    BitVecSet(&bv, 0, !BitVecGet(&bv, 0));
    result = result && (BitVecGet(&clone3, 0) != BitVecGet(&bv, 0));

    BitVecDeinit(&clone3);
    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_bitvec_memory_stress_test(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVec memory stress test\n");

    bool result = true;

    for (int cycle = 0; cycle < 10; cycle++) {
        BitVec bv1 = BitVecInit(ALLOCATOR_OF(&alloc));
        BitVec bv2 = BitVecInit(ALLOCATOR_OF(&alloc));

        // Add random-sized data
        for (int i = 0; i < cycle * 10; i++) {
            BitVecPush(&bv1, i % 2 == 0);
            BitVecPush(&bv2, i % 3 == 0);
        }

        // Clone and swap
        BitVec clone = BitVecClone(&bv1);
        BitVecSwap(&bv1, &bv2);

        BitVecReserve(&bv1, cycle * 20);
        BitVecShrinkToFit(&bv2);

        // Verify data integrity
        result = result && (BitVecLen(&clone) == cycle * 10);
        if (cycle > 0) {
            result = result && (BitVecGet(&clone, 0) == true); // 0 % 2 == 0
        }

        BitVecDeinit(&bv1);
        BitVecDeinit(&bv2);
        BitVecDeinit(&clone);
    }

    DefaultAllocatorDeinit(&alloc);

    return result;
}

// Deadend tests
bool test_bitvec_memory_null_failures(void) {
    WriteFmt("Testing BitVec memory NULL pointer handling\n");

    // Test NULL bitvec pointer - should abort
    BitVecShrinkToFit(NULL);

    return false;
}

bool test_bitvec_swap_null_failures(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVec swap NULL handling\n");

    BitVec bv = BitVecInit(ALLOCATOR_OF(&alloc));

    // Test NULL pointer - should abort
    BitVecSwap(NULL, &bv);

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

bool test_bitvec_clone_null_failures(void) {
    WriteFmt("Testing BitVec clone NULL handling\n");

    // Test NULL pointer - should abort
    BitVecClone(NULL);

    return false;
}

// Resize grow must clean the stale high bits of the old partial tail byte
// so a later resize-grow reads zeros, not garbage. Push a full byte of 1s,
// shrink the length below the byte boundary (leaving bits 5..7 set in the
// backing byte), then grow back. Bits 5..7 must read false.
//
// Kills: 124:28 gt_to_le (mask guard skipped for length>0),
//        125:17 init_const (last_bit_offset := 42 -> 0xFF mask, no clear),
//        126:33 ne_to_eq (mask skipped when offset != 0),
//        128:52 lshift_to_rshift ((1u >> off) - 1 -> 0xFF mask, no clear).
bool test_resize_regrow_clears_stale_tail_bits(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing resize-grow clears stale tail bits\n");

    BitVec bv = BitVecInit(ALLOCATOR_OF(&alloc));

    // Fill a full byte with ones so bits 5,6,7 are set in the backing byte.
    for (int i = 0; i < 8; i++) {
        BitVecPush(&bv, true);
    }

    // Shrink length below the byte boundary -- the high bits stay set in
    // the backing byte but are now past the logical length.
    BitVecResize(&bv, 5);

    // Grow back to 8. The grow path must mask off the stale 5..7 bits.
    BitVecResize(&bv, 8);

    bool result = (BitVecLen(&bv) == 8);
    result      = result && (BitVecGet(&bv, 5) == false);
    result      = result && (BitVecGet(&bv, 6) == false);
    result      = result && (BitVecGet(&bv, 7) == false);

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// BitVecReserve(bv, 0) is a no-op that must report success. The early
// `n <= capacity` return is what makes the zero-request succeed without
// touching the allocator.
//
// Kills: 140:11 le_to_lt (n < capacity drops the n == 0 == capacity case
//        into a zero-size realloc that fails).
bool test_reserve_zero_returns_true(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecReserve with zero capacity returns true\n");

    BitVec bv = BitVecInit(ALLOCATOR_OF(&alloc));

    bool result = (BitVecReserve(&bv, 0) == true);
    result      = result && (BitVecLen(&bv) == 0);

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// BitVecResize must validate its argument. A NULL handle has to abort,
// not be silently accepted.
//
// Kills: 107:5 remove_void_call (ValidateBitVec dropped).
bool test_resize_null_aborts(void) {
    WriteFmt("Testing BitVecResize NULL handle aborts\n");

    BitVecResize(NULL, 5);

    return false; // Should never reach here
}

// BitVecReserve must validate its argument. A NULL handle has to abort.
//
// Kills: 139:5 remove_void_call (ValidateBitVec dropped).
bool test_reserve_null_aborts(void) {
    WriteFmt("Testing BitVecReserve NULL handle aborts\n");

    BitVecReserve(NULL, 5);

    return false; // Should never reach here
}

// BitVecShrinkToFit on a large bitvector must take the realloc path
// (new_byte_size != byte_size) without losing data. Kills the
// `new_byte_size = 42` (line 176) and `byte_size = 42` (line 195)
// mutants: under those the backing buffer is truncated / mis-sized and
// the high bits are corrupted or the next validate aborts.
bool test_shrink_large_preserves_bits(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecShrinkToFit preserves bits on a large vector\n");

    BitVec bv = BitVecInit(ALLOCATOR_OF(&alloc));

    // 400 bits => doubling growth yields capacity 512, byte_size 64,
    // while the data only needs BYTES_FOR_BITS(400) == 50 bytes. So the
    // shrink takes the realloc path (50 != 64) and capacity (512) > length.
    for (int i = 0; i < 400; i++) {
        BitVecPush(&bv, i % 2 == 0);
    }

    BitVecShrinkToFit(&bv);

    bool result = (BitVecLen(&bv) == 400);

    // Every bit, including the high bits in the last byte, must survive.
    for (int i = 0; i < 400 && result; i++) {
        result = result && (BitVecGet(&bv, (u64)i) == (i % 2 == 0));
    }

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// After shrinking a large bitvector, a follow-up operation that runs the
// structural validator must not abort. Kills the `byte_size = 42`
// mutant (line 195): with capacity 400 and byte_size 42, the validator
// sees 42*8 == 336 < 400 and aborts spuriously.
bool test_shrink_large_stays_valid(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecShrinkToFit keeps the vector structurally valid\n");

    BitVec bv = BitVecInit(ALLOCATOR_OF(&alloc));

    for (int i = 0; i < 400; i++) {
        BitVecPush(&bv, i % 3 == 0);
    }

    BitVecShrinkToFit(&bv);

    // ShrinkToFit marks the vector dirty, so the next access runs the
    // structural validator. On real code this returns the bit; under the
    // mutant it aborts.
    bool result = (BitVecGet(&bv, 399) == (399 % 3 == 0));
    result      = result && (BitVecCountOnes(&bv) == BitVecCountOnes(&bv));

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Swapping a large bitvector into another must leave the destination
// structurally valid (byte_size big enough for its new capacity). Kills
// the `bv1->byte_size = 42` mutant (line 216): after the swap bv1 holds
// the large capacity but byte_size 42, so the next validate aborts.
bool test_swap_large_into_other_stays_valid(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecSwap keeps a swapped-in large vector valid\n");

    BitVec small = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec large = BitVecInit(ALLOCATOR_OF(&alloc));

    BitVecPush(&small, true);
    BitVecPush(&small, false);

    // Large enough that 42*8 == 336 < capacity once swapped in.
    for (int i = 0; i < 400; i++) {
        BitVecPush(&large, i % 2 == 0);
    }

    // After this, `small` holds the 400-bit data and the large capacity.
    BitVecSwap(&small, &large);

    // Swap marks both dirty; the next access on `small` runs the
    // structural validator. Real code passes; the mutant aborts.
    bool result = (BitVecLen(&small) == 400);
    result      = result && (BitVecGet(&small, 399) == (399 % 2 == 0));
    result      = result && (BitVecGet(&small, 0) == true);

    BitVecDeinit(&small);
    BitVecDeinit(&large);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// DEADEND: BitVecSwap must validate its second argument. Kills the
// removal of `ValidateBitVec(bv2)` (line 201): with a zeroed (bad-magic)
// second argument, real code aborts at the validate site; the mutant
// skips validation and swaps garbage, returning normally.
bool test_swap_invalid_second_arg_aborts(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecSwap aborts on an invalid second argument\n");

    BitVec good = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVecPush(&good, true);

    BitVec bad = {0}; // magic 0 => invalid bitvec

    // Should abort inside ValidateBitVec(bv2).
    BitVecSwap(&good, &bad);

    return false; // Should never reach here
}

// DEADEND: BitVecTryClone must validate its source. Kills the removal of
// `ValidateBitVec(bv)` (line 223): with a zeroed (bad-magic) source,
// real code aborts; the mutant skips validation and (since the bogus
// length is 0) returns true.
bool test_tryclone_invalid_source_aborts(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecTryClone aborts on an invalid source\n");

    BitVec out = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec bad = {0}; // magic 0 => invalid bitvec

    // Should abort inside ValidateBitVec(bv).
    BitVecTryClone(&out, &bad);

    return false; // Should never reach here
}

// Main function that runs all tests
int main(void) {
    WriteFmt("[INFO] Starting BitVec.Memory tests\n\n");

    // Array of normal test functions
    TestFunction tests[] = {
        test_bitvec_shrink_to_fit,
        test_bitvec_reserve,
        test_reserve_exact_capacity,
        test_reserve_byte_size,
        test_bitvec_swap,
        test_bitvec_clone,
        test_bitvec_clone_inherits_allocator_config,
        test_bitvec_shrink_to_fit_edge_cases,
        test_bitvec_reserve_edge_cases,
        test_bitvec_swap_edge_cases,
        test_bitvec_clone_edge_cases,
        test_bitvec_memory_stress_test,
        test_resize_regrow_clears_stale_tail_bits,
        test_reserve_zero_returns_true,
        test_shrink_large_preserves_bits,
        test_shrink_large_stays_valid,
        test_swap_large_into_other_stays_valid
    };

    // Array of deadend test functions
    TestFunction deadend_tests[] = {
        test_bitvec_memory_null_failures,
        test_bitvec_swap_null_failures,
        test_bitvec_clone_null_failures,
        test_resize_null_aborts,
        test_reserve_null_aborts,
        test_swap_invalid_second_arg_aborts,
        test_tryclone_invalid_source_aborts
    };

    int total_tests         = sizeof(tests) / sizeof(tests[0]);
    int total_deadend_tests = sizeof(deadend_tests) / sizeof(deadend_tests[0]);

    // Run all tests using the centralized test driver
    return run_test_suite(tests, total_tests, deadend_tests, total_deadend_tests, "BitVec.Memory");
}
