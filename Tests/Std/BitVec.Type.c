#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Container/BitVec.h>
#include <Misra/Std/Log.h>

#include <Misra/Types.h>

// Include test utilities
#include "../Util/TestRunner.h"

// Function prototypes
bool test_bitvec_type_basic(void);
bool test_bitvec_validate(void);
bool test_validate_memoization_skips_structural(void);
bool test_structural_byte_size_check_aborts(void);

// Test basic BitVec type functionality
bool test_bitvec_type_basic(void) {
    WriteFmt("Testing basic BitVec type functionality\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    // Create a bitvector
    BitVec bitvec = BitVecInit(ALLOCATOR_OF(&alloc));

    // Check initial state
    bool result =
        (BitVecLen(&bitvec) == 0 && BitVecCapacity(&bitvec) == 0 && BitVecData(&bitvec) == NULL &&
         BitVecByteSize(&bitvec) == 0);

    // Clean up
    BitVecDeinit(&bitvec);
    DefaultAllocatorDeinit(&alloc);

    return result;
}

// Test ValidateBitVec macro
bool test_bitvec_validate(void) {
    WriteFmt("Testing ValidateBitVec macro\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    // Create a valid bitvector
    BitVec bitvec = BitVecInit(ALLOCATOR_OF(&alloc));

    // This should not abort
    ValidateBitVec(&bitvec);

    // Clean up
    BitVecDeinit(&bitvec);
    DefaultAllocatorDeinit(&alloc);

    // Note: We can't easily test the negative case (invalid bitvector)
    // as it would abort the program

    return true;
}

// ---- 1943:23 cxx_and_to_or ----------------------------------------------
// ValidateBitVec memoizes: with MAGIC_VALIDATED_BIT clear it trusts the
// prior structural check and returns early (line 1943-1945). The mutant
// `!(magic | VALIDATED_BIT)` is always false, so it ALWAYS re-runs the
// structural validator. Hand-build a bitvec whose validated bit is CLEAR
// but whose byte_size is too small for its capacity: the real code skips
// the structural check and returns normally; the mutant runs it and aborts
// at the byte_size check. A normal (non-aborting) test therefore passes on
// real code and dies (longjmp) under the mutant.
bool test_validate_memoization_skips_structural(void) {
    WriteFmt("Testing ValidateBitVec honours the validated-bit memoization (1943:23)\n");

    BitVec bv = {0};
    // Structurally inconsistent: capacity claims 400 bits but byte_size 1
    // only backs 8 bits. length 0 + data NULL keeps the earlier structural
    // checks (length>capacity, length>0&&!data, data[0] read) inert, so the
    // ONLY check that would fire is the byte_size-vs-capacity one.
    bv.length    = 0;
    bv.capacity  = 400;
    bv.data      = NULL;
    bv.byte_size = 1;
    bv.allocator = NULL;
    // Valid magic with the validated bit CLEAR -> real code returns early.
    bv.__magic = BITVEC_MAGIC;

    // Real: validated bit clear -> early return, no structural check, no abort.
    // Mutant: always runs structural -> 1*8 < 400 -> LOG_FATAL.
    ValidateBitVec(&bv);

    return true; // reached only when no abort occurred (real code)
}

// ---- 1926:22 cxx_gt_to_le (DEADEND) -------------------------------------
// validate_bitvec_structural guards the byte_size check with `capacity > 0`.
// The mutant `capacity <= 0` (i.e. == 0) disables the check for every real
// bitvec (capacity > 0), so a too-small byte_size is no longer caught.
// Hand-build an invalid bitvec WITH the validated bit set so the structural
// validator actually runs: real code aborts at the byte_size check; the
// mutant skips it and returns. Deadend => expects the abort.
bool test_structural_byte_size_check_aborts(void) {
    WriteFmt("Testing validate_bitvec_structural catches an undersized byte_size (1926:22)\n");

    BitVec bv    = {0};
    bv.length    = 0;
    bv.capacity  = 400;
    bv.data      = NULL;
    bv.byte_size = 1; // 1*8 == 8 < 400 -> structurally invalid
    bv.allocator = NULL;
    // Validated bit SET so validate_bitvec_structural runs.
    bv.__magic = BITVEC_MAGIC | MAGIC_VALIDATED_BIT;

    // Real code aborts here; the mutant returns normally.
    ValidateBitVec(&bv);

    return false; // should never reach here on real code
}

// Main function that runs all tests
int main(void) {
    WriteFmt("[INFO] Starting BitVec.Type tests\n\n");

    // Array of test functions
    TestFunction tests[] = {test_bitvec_type_basic, test_bitvec_validate, test_validate_memoization_skips_structural};

    // Deadend tests trigger exactly one abort each.
    TestFunction deadend_tests[] = {test_structural_byte_size_check_aborts};

    int total_tests         = sizeof(tests) / sizeof(tests[0]);
    int total_deadend_tests = sizeof(deadend_tests) / sizeof(deadend_tests[0]);

    // Run all tests using the centralized test driver
    return run_test_suite(tests, total_tests, deadend_tests, total_deadend_tests, "BitVec.Type");
}
