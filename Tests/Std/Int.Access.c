#include <Misra/Std/Container/Int.h>
#include <Misra/Std/Log.h>
#include <Misra/Types.h>

#include "../Util/TestRunner.h"

bool test_int_bit_length(void);
bool test_int_byte_length(void);
bool test_int_is_zero(void);
bool test_int_is_one(void);
bool test_int_parity(void);
bool test_int_fits_u64(void);
bool test_int_log2(void);
bool test_int_trailing_zero_count(void);
bool test_int_is_power_of_two(void);
bool test_int_log2_zero(void);

bool test_int_bit_length(void) {
    WriteFmt("Testing IntBitLength\n");

    Int value = IntFromBinary("00101000");

    bool result = IntBitLength(&value) == 6;

    IntDeinit(&value);
    return result;
}

bool test_int_byte_length(void) {
    WriteFmt("Testing IntByteLength\n");

    Int value = IntFromBinary("0001001000110100");

    bool result = IntByteLength(&value) == 2;

    IntDeinit(&value);
    return result;
}

bool test_int_is_zero(void) {
    WriteFmt("Testing IntIsZero\n");

    Int zero     = IntInit();
    Int non_zero = IntFrom(1);

    bool result = IntIsZero(&zero);
    result      = result && !IntIsZero(&non_zero);

    IntDeinit(&zero);
    IntDeinit(&non_zero);
    return result;
}

bool test_int_is_one(void) {
    WriteFmt("Testing IntIsOne\n");

    Int one = IntFrom(1);
    Int two = IntFrom(2);

    bool result = IntIsOne(&one);
    result      = result && !IntIsOne(&two);

    IntDeinit(&one);
    IntDeinit(&two);
    return result;
}

bool test_int_parity(void) {
    WriteFmt("Testing Int parity helpers\n");

    Int even = IntFrom(42);
    Int odd  = IntFrom(43);

    bool result = IntIsEven(&even);
    result      = result && !IntIsOdd(&even);
    result      = result && IntIsOdd(&odd);
    result      = result && !IntIsEven(&odd);

    IntDeinit(&even);
    IntDeinit(&odd);
    return result;
}

bool test_int_fits_u64(void) {
    WriteFmt("Testing IntFitsU64\n");

    Int small = IntFrom(UINT64_MAX);
    Int big   = IntFrom(1);

    IntShiftLeft(&big, 64);

    bool result = IntFitsU64(&small);
    result      = result && !IntFitsU64(&big);

    IntDeinit(&small);
    IntDeinit(&big);
    return result;
}

bool test_int_log2(void) {
    WriteFmt("Testing IntLog2\n");

    Int value = IntFrom(1025);

    bool result = IntLog2(&value) == 10;

    IntDeinit(&value);
    return result;
}

bool test_int_trailing_zero_count(void) {
    WriteFmt("Testing IntTrailingZeroCount\n");

    Int value = IntFromBinary("1010000");
    Int zero  = IntInit();

    bool result = IntTrailingZeroCount(&value) == 4;
    result      = result && (IntTrailingZeroCount(&zero) == 0);

    IntDeinit(&value);
    IntDeinit(&zero);
    return result;
}

bool test_int_is_power_of_two(void) {
    WriteFmt("Testing IntIsPowerOfTwo\n");

    Int one   = IntFrom(1);
    Int power = IntFrom(1);
    Int other = IntFrom(24);
    Int zero  = IntInit();

    IntShiftLeft(&power, 20);

    bool result = IntIsPowerOfTwo(&one);
    result      = result && IntIsPowerOfTwo(&power);
    result      = result && !IntIsPowerOfTwo(&other);
    result      = result && !IntIsPowerOfTwo(&zero);

    IntDeinit(&one);
    IntDeinit(&power);
    IntDeinit(&other);
    IntDeinit(&zero);
    return result;
}

bool test_int_log2_zero(void) {
    WriteFmt("Testing IntLog2 zero handling\n");

    Int value = IntInit();

    IntLog2(&value);
    return false;
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
    };

    TestFunction deadend_tests[] = {
        test_int_log2_zero,
    };

    int total_tests         = sizeof(tests) / sizeof(tests[0]);
    int total_deadend_tests = sizeof(deadend_tests) / sizeof(deadend_tests[0]);

    return run_test_suite(tests, total_tests, deadend_tests, total_deadend_tests, "Int.Access");
}
