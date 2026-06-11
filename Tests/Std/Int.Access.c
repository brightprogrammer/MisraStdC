#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Container/Int.h>
#include <Misra/Std/Log.h>
#include <Misra/Types.h>

#include "../Util/TestRunner.h"

bool        test_int_bit_length(void);
bool        test_int_byte_length(void);
bool        test_int_is_zero(void);
bool        test_int_is_one(void);
bool        test_int_parity(void);
bool        test_int_fits_u64(void);
bool        test_int_log2(void);
bool        test_int_trailing_zero_count(void);
bool        test_int_is_power_of_two(void);
bool        test_int_log2_zero(void);
static bool test_m2_root_rem_degree_one_clones_value(void);
bool        test_m23_hash_zero_is_fnv_offset_basis(void);
bool        test_m23_hash_xor_not_or(void);
bool        test_m23_shift_right_zero_is_noop_true(void);
bool        test_m24_shift_left_zero_noop(void);
static bool test_m28_bit_counts_known_value(void);
static bool test_m28_bit_length_invalid_deadend(void);
bool        test_m9_pow_u64_mod_multibit_exponent(void);

bool test_int_bit_length(void) {
    WriteFmt("Testing IntBitLength\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value = IntFromBinary("00101000", &alloc.base);

    bool result = IntBitLength(&value) == 6;

    IntDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_byte_length(void) {
    WriteFmt("Testing IntByteLength\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value = IntFromBinary("0001001000110100", &alloc.base);

    bool result = IntByteLength(&value) == 2;

    IntDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_is_zero(void) {
    WriteFmt("Testing IntIsZero\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int zero     = IntInit(&alloc.base);
    Int non_zero = IntFrom(1, &alloc.base);

    bool result = IntIsZero(&zero);
    result      = result && !IntIsZero(&non_zero);

    IntDeinit(&zero);
    IntDeinit(&non_zero);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_is_one(void) {
    WriteFmt("Testing IntIsOne\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int one = IntFrom(1, &alloc.base);
    Int two = IntFrom(2, &alloc.base);

    bool result = IntIsOne(&one);
    result      = result && !IntIsOne(&two);

    IntDeinit(&one);
    IntDeinit(&two);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_parity(void) {
    WriteFmt("Testing Int parity helpers\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int even = IntFrom(42, &alloc.base);
    Int odd  = IntFrom(43, &alloc.base);

    bool result = IntIsEven(&even);
    result      = result && !IntIsOdd(&even);
    result      = result && IntIsOdd(&odd);
    result      = result && !IntIsEven(&odd);

    IntDeinit(&even);
    IntDeinit(&odd);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_fits_u64(void) {
    WriteFmt("Testing IntFitsU64\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int small = IntFrom(UINT64_MAX, &alloc.base);
    Int big   = IntFrom(1, &alloc.base);

    IntShiftLeft(&big, 64);

    bool result = IntFitsU64(&small);
    result      = result && !IntFitsU64(&big);

    IntDeinit(&small);
    IntDeinit(&big);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_log2(void) {
    WriteFmt("Testing IntLog2\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int  value = IntFrom(1025, &alloc.base);
    bool error = true;

    bool result = IntLog2(&value, &error) == 10;
    result      = result && !error;

    IntDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_trailing_zero_count(void) {
    WriteFmt("Testing IntTrailingZeroCount\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value = IntFromBinary("1010000", &alloc.base);
    Int zero  = IntInit(&alloc.base);

    bool result = IntTrailingZeroCount(&value) == 4;
    result      = result && (IntTrailingZeroCount(&zero) == 0);

    IntDeinit(&value);
    IntDeinit(&zero);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_is_power_of_two(void) {
    WriteFmt("Testing IntIsPowerOfTwo\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int one   = IntFrom(1, &alloc.base);
    Int power = IntFrom(1, &alloc.base);
    Int other = IntFrom(24, &alloc.base);
    Int zero  = IntInit(&alloc.base);

    IntShiftLeft(&power, 20);

    bool result = IntIsPowerOfTwo(&one);
    result      = result && IntIsPowerOfTwo(&power);
    result      = result && !IntIsPowerOfTwo(&other);
    result      = result && !IntIsPowerOfTwo(&zero);

    IntDeinit(&one);
    IntDeinit(&power);
    IntDeinit(&other);
    IntDeinit(&zero);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_log2_zero(void) {
    WriteFmt("Testing IntLog2 zero handling\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int  value = IntInit(&alloc.base);
    bool error = false;

    bool result = IntLog2(&value, &error) == 0;
    result      = result && error;

    IntDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}


// Degree-1 path: root must become an exact clone of value and remainder 0,
// overwriting prior contents. Kills the IntTryClone replace-scalar (1720) and
// the int_replace removals (1725/1726).
static bool test_m2_root_rem_degree_one_clones_value(void) {
    WriteFmt("Testing IntRootRem degree==1 clones value, zero remainder\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value     = IntFrom(12345, &alloc.base);
    Int root      = IntFrom(99, &alloc.base);
    Int remainder = IntFrom(77, &alloc.base);

    bool ok     = IntRootRem(&root, &remainder, &value, 1);
    bool result = ok;
    result      = result && (IntToU64(&root) == 12345);
    result      = result && (IntCompare(&remainder, 0) == 0);

    IntDeinit(&value);
    IntDeinit(&root);
    IntDeinit(&remainder);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// L928 (init_const): the FNV-1a offset basis must be the canonical constant.
// For a zero Int there are no magnitude bytes, so the loop never runs and the
// hash equals the offset basis exactly.
bool test_m23_hash_zero_is_fnv_offset_basis(void) {
    WriteFmt("Testing int_hash zero == FNV offset basis\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value = IntFrom(0, &alloc.base);

    u64  h      = int_hash(&value, 0);
    bool result = (h == 1469598103934665603ULL);

    IntDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// L937 (xor_assign_to_or_assign): the byte must be folded in with XOR. We
// recompute the FNV-1a digest of value 255 (single byte 0xFF) using XOR and
// require int_hash to match. Under `|=` the low byte mixing differs
// (0x83 ^ 0xFF != 0x83 | 0xFF) so the digest diverges.
bool test_m23_hash_xor_not_or(void) {
    WriteFmt("Testing int_hash folds bytes with XOR\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value = IntFrom(255, &alloc.base);

    u64 expected  = 1469598103934665603ULL;
    expected     ^= (u64)0xFFu;
    expected     *= 1099511628211ULL;

    u64  h      = int_hash(&value, 0);
    bool result = (h == expected);

    IntDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// L1037 (replace_scalar_call): shifting right by zero positions must return
// the genuine BitVecResize result (true) and leave the value untouched.
// Replacing the call with a constant scalar (false/0) is observable.
bool test_m23_shift_right_zero_is_noop_true(void) {
    WriteFmt("Testing IntShiftRight by zero positions\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value = IntFrom(5, &alloc.base);

    bool ok     = IntShiftRight(&value, 0);
    bool result = ok && (IntToU64(&value) == 5);

    IntDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// ---------------------------------------------------------------------------
// IntShiftLeft zero-positions fast path.
// `if (positions == 0) return BitVecResize(INT_BITS(value), bits);` must
// return true and leave the value unchanged.
//
// Kills L1009:16 cxx_replace_scalar_call: the BitVecResize(...) return value
// is replaced with a scalar (false), so IntShiftLeft(&v, 0) wrongly fails.
// ---------------------------------------------------------------------------
bool test_m24_shift_left_zero_noop(void) {
    WriteFmt("Testing IntShiftLeft(&v, 0) returns true, value unchanged\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int  value = IntFrom(7u, &alloc.base);
    bool ok    = IntShiftLeft(&value, 0);

    bool fail = !ok;                                   // must report success.
    fail      = fail || (IntCompare(&value, 7u) != 0); // value preserved.

    IntDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return !fail;
}

///
/// Positive guard for int_significant_bits / IntTrailingZeroCount over a
/// known value, so the loop arithmetic itself stays guarded. 0b101000 =
/// 40: 6 significant bits, 3 trailing zeros.
///
static bool test_m28_bit_counts_known_value(void) {
    WriteFmt("Testing significant-bits and trailing-zero counts\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value = IntFrom((u64)40u, &alloc.base);

    bool result = (IntBitLength(&value) == 6u);
    result      = result && (IntTrailingZeroCount(&value) == 3u);

    IntDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

///
/// Deadend: int_significant_bits (reached via IntBitLength) validates its
/// argument before touching the backing store. A zeroed-but-non-NULL Int
/// has an invalid BitVec magic: real code aborts in ValidateInt. With the
/// validator removed, BitVecLen reads the zero length and the loop returns
/// 0 cleanly -- no abort. Using a zeroed (non-NULL) Int keeps the mutant on
/// a non-crashing path so the missing abort is observable.
///
static bool test_m28_bit_length_invalid_deadend(void) {
    WriteFmt("Testing IntBitLength validation on invalid Int\n");

    Int invalid = {0};

    (void)IntBitLength(&invalid);

    return false;
}

// Exercises a multi-bit exponent so both the odd-bit accumulate branch and the
// base squaring branch run multiple times; pins the exact reduced result.
// 7^20 mod 13 = 3.
bool test_m9_pow_u64_mod_multibit_exponent(void) {
    WriteFmt("Testing int_pow_u64_mod multi-bit exponent 7^20 mod 13\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int base         = IntFrom(7, &alloc.base);
    Int mod          = IntFrom(13, &alloc.base);
    Int result_value = IntInit(&alloc.base);

    bool ok = IntPowMod(&result_value, &base, 20u, &mod);

    bool result = ok && (IntToU64(&result_value) == 3);

    IntDeinit(&base);
    IntDeinit(&mod);
    IntDeinit(&result_value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

int main(void) {
    WriteFmt("[INFO] Starting Int.Access tests\n\n");

    TestFunction tests[] = {
        test_int_bit_length,
        test_int_byte_length,
        test_int_is_zero,
        test_int_is_one,
        test_int_parity,
        test_int_fits_u64,
        test_int_log2,
        test_int_trailing_zero_count,
        test_int_is_power_of_two,
        test_int_log2_zero,
        test_m2_root_rem_degree_one_clones_value,
        test_m23_hash_zero_is_fnv_offset_basis,
        test_m23_hash_xor_not_or,
        test_m23_shift_right_zero_is_noop_true,
        test_m24_shift_left_zero_noop,
        test_m28_bit_counts_known_value,
        test_m9_pow_u64_mod_multibit_exponent,
    };

    TestFunction deadend_tests[] = {
        test_m28_bit_length_invalid_deadend,
    };

    int total_tests         = sizeof(tests) / sizeof(tests[0]);
    int total_deadend_tests = sizeof(deadend_tests) / sizeof(deadend_tests[0]);
    return run_test_suite(tests, total_tests, deadend_tests, total_deadend_tests, "Int.Access");
}
