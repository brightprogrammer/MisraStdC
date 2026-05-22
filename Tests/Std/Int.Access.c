#include <Misra/Std/Allocator/Default.h>
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
    };

    int total_tests = sizeof(tests) / sizeof(tests[0]);
    return run_test_suite(tests, total_tests, NULL, 0, "Int.Access");
}
