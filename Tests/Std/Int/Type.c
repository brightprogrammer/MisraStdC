#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Container/Int.h>
#include <Misra/Std/Container/BitVec.h>
#include <Misra/Std/Log.h>
#include <Misra/Types.h>

#include "../../Util/TestRunner.h"

bool test_int_init(void);
bool test_int_clear(void);
bool test_int_clone(void);
bool test_int_clone_inherits_allocator_config(void);
bool test_m27_normalize_trims_via_add(void);
bool test_fe_96_u64_bits_construct(void);

bool test_int_init(void) {
    WriteFmt("Testing IntInit\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value = IntInit(&alloc.base);

    bool result = IntIsZero(&value);
    result      = result && (IntBitLength(&value) == 0);

    IntDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_clear(void) {
    WriteFmt("Testing IntClear\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value = IntFromBinary("101101", &alloc.base);

    IntClear(&value);

    bool result = IntIsZero(&value);
    result      = result && (IntBitLength(&value) == 0);

    IntDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_clone(void) {
    WriteFmt("Testing IntClone\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int original = IntFromBinary("1011", &alloc.base);
    Int clone    = IntClone(&original);

    bool result = IntEQ(&clone, &original);
    result      = result && (IntToU64(&clone) == 11);

    IntShiftLeft(&original, 1);

    result = result && !IntEQ(&clone, &original);
    result = result && (IntToU64(&clone) == 11);
    result = result && (IntToU64(&original) == 22);

    IntDeinit(&original);
    IntDeinit(&clone);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_clone_inherits_allocator_config(void) {
    WriteFmt("Testing IntClone allocator inheritance\n");

    DefaultAllocator alloc = DefaultAllocatorInit();
    // intentional bypass: no public setter on `Allocator` for effort /
    // retry_limit -- pre-seeded directly so the inheritance path below
    // can be observed end-to-end.
    alloc.base.effort      = ALLOCATOR_EFFORT_RETRY_FALLBACK;
    alloc.base.retry_limit = 5;

    Int original = IntInit(&alloc);

    BitVecPush(&original.bits, true);
    BitVecPush(&original.bits, false);
    BitVecPush(&original.bits, true);

    Int clone = IntClone(&original);

    bool result =
        BitVecLen(&clone.bits) == BitVecLen(&original.bits) && IntAllocator(&clone) == IntAllocator(&original) &&
        IntAllocator(&clone)->allocate == IntAllocator(&original)->allocate &&
        IntAllocator(&clone)->remap == IntAllocator(&original)->remap &&
        IntAllocator(&clone)->deallocate == IntAllocator(&original)->deallocate &&
        IntAllocator(&clone)->effort == IntAllocator(&original)->effort &&
        IntAllocator(&clone)->retry_limit == IntAllocator(&original)->retry_limit &&
        BitVecGet(&clone.bits, 0) == true && BitVecGet(&clone.bits, 1) == false && BitVecGet(&clone.bits, 2) == true;

    IntDeinit(&original);
    IntDeinit(&clone);
    DefaultAllocatorDeinit(&alloc);
    return result;
}


// Kills cxx_replace_scalar_call on int_normalize's resize length
//   line 219:  BitVecResize(INT_BITS(value), int_significant_bits(value));
// int_normalize runs at the tail of int_add, so the sum's bit length is the
// significant-bit count of the result. IntAdd(255, 1) == 256 == 0b100000000,
// whose only significant bit is bit 8 -> IntBitLength == 9.
//   - replacing int_significant_bits(value) with a scalar makes int_normalize
//     resize to a wrong, fixed length, so the value and its IntBitLength stop
//     matching 256 / 9 (e.g. scalar 0 -> resize to 0 bits -> value 0).
// Real code yields 256 with bit length 9; the mutant diverges on both checks.
bool test_m27_normalize_trims_via_add(void) {
    WriteFmt("Testing int_normalize resize length via IntAdd(255,1)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int a   = IntFrom(255, &alloc.base);
    Int b   = IntFrom(1, &alloc.base);
    Int sum = IntInit(&alloc.base);

    IntAdd(&sum, &a, &b);

    bool result = (IntToU64(&sum) == 256);
    result      = result && (IntBitLength(&sum) == 9);

    IntDeinit(&a);
    IntDeinit(&b);
    IntDeinit(&sum);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 96: int_u64_bits counts the set bits of a u64 via `bits++`. The
// faithful mutation `bits--` underflows the counter to a near-2^64 value,
// so int_try_from_u64 asks BitVecTryFromInteger for an impossible bit
// count and fails; IntFrom then yields an empty Int. Real code sizes 255
// to 8 bits. Observe the bit length and value of a freshly constructed
// integer.
bool test_fe_96_u64_bits_construct(void) {
    WriteFmt("Testing int_u64_bits construction width\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int v255 = IntFrom(255, &alloc.base);

    bool result = IntBitLength(&v255) == 8;
    result      = result && (IntToU64(&v255) == 255);

    IntDeinit(&v255);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

int main(void) {
    WriteFmt("[INFO] Starting Int.Type tests\n\n");

    TestFunction tests[] = {
        test_int_init,
        test_int_clear,
        test_int_clone,
        test_int_clone_inherits_allocator_config,
        test_m27_normalize_trims_via_add,
        test_fe_96_u64_bits_construct,
    };

    int total_tests = sizeof(tests) / sizeof(tests[0]);
    return run_test_suite(tests, total_tests, NULL, 0, "Int.Type");
}
