#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Allocator/Debug.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Std/Container/Int.h>
#include <Misra/Std/Log.h>
#include <Misra/Types.h>

#include "../Util/TestRunner.h"

static DebugAllocatorConfig blind_cfg(void) {
    return (DebugAllocatorConfig) {.capture_traces = false, .detect_overflow = false, .track_freed_history = false};
}

#define LEAK_CLEAN(dbg) (DebugAllocatorLiveCount(&(dbg)) == 0 && DebugAllocatorLiveBytes(&(dbg)) == 0)

#define LEAK_CFG                                                                                                       \
    ((DebugAllocatorConfig) {.capture_traces = false, .detect_overflow = false, .track_freed_history = false})

bool test_int_shift_left_grows(void);
bool test_int_shift_right_shrinks(void);
bool test_int_add(void);
bool test_int_add_generic(void);
bool test_int_sub(void);
bool test_int_sub_generic(void);
bool test_int_sub_underflow_preserves_result(void);
bool test_int_mul(void);
bool test_int_mul_scalar(void);
bool test_int_mul_zero(void);
bool test_int_square(void);
bool test_int_pow_generic(void);
bool test_int_div_mod(void);
bool test_int_div(void);
bool test_int_div_exact(void);
bool test_int_div_exact_failure_preserves_result(void);
bool test_int_div_mod_scalar(void);
bool test_int_mod(void);
bool test_int_gcd(void);
bool test_int_lcm(void);
bool test_int_root(void);
bool test_int_root_rem(void);
bool test_int_sqrt(void);
bool test_int_sqrt_rem(void);
bool test_int_is_perfect_square(void);
bool test_int_is_perfect_power(void);
bool test_int_jacobi(void);
bool test_int_square_mod(void);
bool test_int_mod_add(void);
bool test_int_mod_sub(void);
bool test_int_mod_mul(void);
bool test_int_mod_div(void);
bool test_int_mod_scalar(void);
bool test_int_pow_mod_scalar(void);
bool test_int_pow_mod_integer_exponent(void);
bool test_int_mod_inv(void);
bool test_int_mod_sqrt(void);
bool test_int_mod_sqrt_no_solution(void);
bool test_int_is_probable_prime(void);
bool test_int_next_prime(void);
bool test_int_mod_inv_no_solution(void);
bool test_int_mod_div_no_solution(void);
bool test_int_add_null_result(void);
bool test_int_shift_left_null(void);
bool test_int_div_by_zero(void);
bool test_int_root_zero_degree(void);
bool test_int_div_scalar_zero_divisor(void);
bool test_int_mod_scalar_zero_modulus(void);
bool test_int_mod_div_zero_modulus(void);
bool test_int_jacobi_even_denominator(void);
bool test_int_pow_mod_scalar_zero_modulus(void);
bool test_int_pow_mod_integer_zero_modulus(void);

bool test_int_shift_left_grows(void) {
    WriteFmt("Testing IntShiftLeft\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value = IntFrom(3, &alloc.base);

    IntShiftLeft(&value, 4);

    bool result = IntToU64(&value) == 48;
    result      = result && (IntBitLength(&value) == 6);

    IntDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_shift_right_shrinks(void) {
    WriteFmt("Testing IntShiftRight\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value = IntFromBinary("110000", &alloc.base);

    IntShiftRight(&value, 4);

    bool result = IntToU64(&value) == 3;
    result      = result && (IntBitLength(&value) == 2);

    IntDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_add(void) {
    WriteFmt("Testing IntAdd\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int a            = IntFrom(255, &alloc.base);
    Int b            = IntFrom(1, &alloc.base);
    Int result_value = IntInit(&alloc.base);
    Str text         = StrInit(&alloc.base);

    IntAdd(&result_value, &a, &b);
    text = IntToBinary(&result_value);

    bool result = IntToU64(&result_value) == 256;
    result      = result && (ZstrCompare(StrBegin(&text), "100000000") == 0);

    StrDeinit(&text);
    IntDeinit(&a);
    IntDeinit(&b);
    IntDeinit(&result_value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_add_generic(void) {
    WriteFmt("Testing IntAdd generic dispatch\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int base         = IntFrom(40, &alloc.base);
    Int rhs          = IntFrom(2, &alloc.base);
    Int result_value = IntInit(&alloc.base);
    Int huge         = IntFromStr("123456789012345678901234567890", &alloc.base);
    Str text         = StrInit(&alloc.base);

    IntAdd(&result_value, &base, &rhs);
    bool result = IntToU64(&result_value) == 42;

    IntAdd(&result_value, &base, 2);
    result = result && (IntToU64(&result_value) == 42);

    IntAdd(&result_value, &base, -2);
    result = result && (IntToU64(&result_value) == 38);

    IntAdd(&result_value, &huge, 10);
    text   = IntToStr(&result_value);
    result = result && (ZstrCompare(StrBegin(&text), "123456789012345678901234567900") == 0);

    IntDeinit(&base);
    IntDeinit(&rhs);
    IntDeinit(&result_value);
    IntDeinit(&huge);
    StrDeinit(&text);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_sub(void) {
    WriteFmt("Testing IntSub\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int a            = IntFrom(256, &alloc.base);
    Int b            = IntFrom(1, &alloc.base);
    Int result_value = IntInit(&alloc.base);

    bool result = IntSub(&result_value, &a, &b);
    result      = result && (IntToU64(&result_value) == 255);

    IntDeinit(&a);
    IntDeinit(&b);
    IntDeinit(&result_value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_sub_generic(void) {
    WriteFmt("Testing IntSub generic dispatch\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int base         = IntFrom(40, &alloc.base);
    Int rhs          = IntFrom(2, &alloc.base);
    Int result_value = IntInit(&alloc.base);
    Int preserved    = IntFrom(99, &alloc.base);
    Int huge         = IntFromStr("12345678901234567890", &alloc.base);
    Str text         = StrInit(&alloc.base);

    bool result = IntSub(&result_value, &base, &rhs);
    result      = result && (IntToU64(&result_value) == 38);

    result = result && IntSub(&result_value, &base, 2u);
    result = result && (IntToU64(&result_value) == 38);

    result = result && IntSub(&result_value, &base, -2);
    result = result && (IntToU64(&result_value) == 42);

    result = result && IntSub(&result_value, &huge, 90);
    text   = IntToStr(&result_value);
    result = result && (ZstrCompare(StrBegin(&text), "12345678901234567800") == 0);

    result = result && !IntSub(&preserved, &base, 50);
    result = result && (IntToU64(&preserved) == 99);

    IntDeinit(&base);
    IntDeinit(&rhs);
    IntDeinit(&result_value);
    IntDeinit(&preserved);
    IntDeinit(&huge);
    StrDeinit(&text);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_sub_underflow_preserves_result(void) {
    WriteFmt("Testing IntSub underflow handling\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int a            = IntFrom(3, &alloc.base);
    Int b            = IntFrom(5, &alloc.base);
    Int result_value = IntFrom(99, &alloc.base);

    bool result = !IntSub(&result_value, &a, &b);
    result      = result && (IntToU64(&result_value) == 99);

    IntDeinit(&a);
    IntDeinit(&b);
    IntDeinit(&result_value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_mul(void) {
    WriteFmt("Testing IntMul\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int a            = IntFrom(21, &alloc.base);
    Int b            = IntFrom(6, &alloc.base);
    Int result_value = IntInit(&alloc.base);

    IntMul(&result_value, &a, &b);

    bool result = IntToU64(&result_value) == 126;

    IntDeinit(&a);
    IntDeinit(&b);
    IntDeinit(&result_value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_mul_scalar(void) {
    WriteFmt("Testing IntMul generic dispatch\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value        = IntFromStr("12345678901234567890", &alloc.base);
    Int result_value = IntInit(&alloc.base);
    Str text         = StrInit(&alloc.base);

    IntMul(&result_value, &value, 25u);
    text = IntToStr(&result_value);

    bool result = ZstrCompare(StrBegin(&text), "308641972530864197250") == 0;

    IntDeinit(&value);
    IntDeinit(&result_value);
    StrDeinit(&text);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_mul_zero(void) {
    WriteFmt("Testing IntMul with zero\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int a            = IntFrom(0, &alloc.base);
    Int b            = IntFrom(12345, &alloc.base);
    Int result_value = IntInit(&alloc.base);

    IntMul(&result_value, &a, &b);

    bool result = IntIsZero(&result_value);
    result      = result && (IntToU64(&result_value) == 0);

    IntDeinit(&a);
    IntDeinit(&b);
    IntDeinit(&result_value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_square(void) {
    WriteFmt("Testing IntSquare\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value        = IntFrom(12345, &alloc.base);
    Int result_value = IntInit(&alloc.base);

    IntSquare(&result_value, &value);

    bool result = IntToU64(&result_value) == 152399025;

    IntDeinit(&value);
    IntDeinit(&result_value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_pow_generic(void) {
    WriteFmt("Testing IntPow generic dispatch\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int base         = IntFrom(7, &alloc.base);
    Int exponent     = IntFrom(20, &alloc.base);
    Int result_value = IntInit(&alloc.base);
    Str text         = StrInit(&alloc.base);

    IntPow(&result_value, &base, 20u);
    text        = IntToStr(&result_value);
    bool result = ZstrCompare(StrBegin(&text), "79792266297612001") == 0;

    StrDeinit(&text);
    IntPow(&result_value, &base, &exponent);
    text   = IntToStr(&result_value);
    result = result && (ZstrCompare(StrBegin(&text), "79792266297612001") == 0);

    IntDeinit(&base);
    IntDeinit(&exponent);
    IntDeinit(&result_value);
    StrDeinit(&text);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_div_mod(void) {
    WriteFmt("Testing IntDivMod generic dispatch\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int dividend  = IntFromStr("12345678901234567890", &alloc.base);
    Int quotient  = IntInit(&alloc.base);
    Int remainder = IntInit(&alloc.base);
    Str qtext     = StrInit(&alloc.base);

    IntDivMod(&quotient, &remainder, &dividend, 97u);
    qtext = IntToStr(&quotient);

    bool result = ZstrCompare(StrBegin(&qtext), "127275040218913071") == 0;
    result      = result && (IntToU64(&remainder) == 3);

    StrDeinit(&qtext);
    IntDeinit(&dividend);
    IntDeinit(&quotient);
    IntDeinit(&remainder);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_div(void) {
    WriteFmt("Testing IntDiv generic dispatch\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int dividend     = IntFrom(126, &alloc.base);
    Int result_value = IntInit(&alloc.base);

    IntDiv(&result_value, &dividend, 10u);

    bool result = IntToU64(&result_value) == 12;

    IntDeinit(&dividend);
    IntDeinit(&result_value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_div_exact(void) {
    WriteFmt("Testing IntDivExact generic dispatch\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int dividend     = IntFromStr("12345678901234567890", &alloc.base);
    Int result_value = IntInit(&alloc.base);
    Str text         = StrInit(&alloc.base);

    bool result = IntDivExact(&result_value, &dividend, 90u);
    text        = IntToStr(&result_value);
    result      = result && (ZstrCompare(StrBegin(&text), "137174210013717421") == 0);

    IntDeinit(&dividend);
    IntDeinit(&result_value);
    StrDeinit(&text);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_div_exact_failure_preserves_result(void) {
    WriteFmt("Testing IntDivExact failure handling\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int dividend     = IntFrom(10, &alloc.base);
    Int divisor      = IntFrom(3, &alloc.base);
    Int result_value = IntFrom(99, &alloc.base);

    bool result = !IntDivExact(&result_value, &dividend, &divisor);
    result      = result && (IntToU64(&result_value) == 99);

    IntDeinit(&dividend);
    IntDeinit(&divisor);
    IntDeinit(&result_value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_div_mod_scalar(void) {
    WriteFmt("Testing IntDivMod scalar-divisor dispatch\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int dividend  = IntFromStr("12345678901234567890", &alloc.base);
    Int quotient  = IntInit(&alloc.base);
    Int remainder = IntInit(&alloc.base);
    Str text      = StrInit(&alloc.base);

    IntDivMod(&quotient, &remainder, &dividend, 97);
    text = IntToStr(&quotient);

    bool result = ZstrCompare(StrBegin(&text), "127275040218913071") == 0;
    result      = result && (IntToU64(&remainder) == 3);

    IntDeinit(&dividend);
    IntDeinit(&quotient);
    IntDeinit(&remainder);
    StrDeinit(&text);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_mod(void) {
    WriteFmt("Testing IntMod generic dispatch\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int dividend     = IntFrom(126, &alloc.base);
    Int result_value = IntInit(&alloc.base);

    IntMod(&result_value, &dividend, 10u);

    bool result = IntToU64(&result_value) == 6;

    IntDeinit(&dividend);
    IntDeinit(&result_value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_mod_scalar(void) {
    WriteFmt("Testing IntMod scalar-divisor dispatch\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value     = IntFromStr("12345678901234567890", &alloc.base);
    Int remainder = IntInit(&alloc.base);

    IntMod(&remainder, &value, 97u);

    IntDeinit(&value);
    bool result = IntToU64(&remainder) == 3;
    IntDeinit(&remainder);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_gcd(void) {
    WriteFmt("Testing IntGCD\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int a            = IntFrom(48, &alloc.base);
    Int b            = IntFrom(18, &alloc.base);
    Int result_value = IntInit(&alloc.base);

    IntGCD(&result_value, &a, &b);

    bool result = IntToU64(&result_value) == 6;

    IntDeinit(&a);
    IntDeinit(&b);
    IntDeinit(&result_value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_lcm(void) {
    WriteFmt("Testing IntLCM\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int a            = IntFrom(21, &alloc.base);
    Int b            = IntFrom(6, &alloc.base);
    Int result_value = IntInit(&alloc.base);

    IntLCM(&result_value, &a, &b);

    bool result = IntToU64(&result_value) == 42;

    IntDeinit(&a);
    IntDeinit(&b);
    IntDeinit(&result_value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_root(void) {
    WriteFmt("Testing IntRoot\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value        = IntFrom(4096, &alloc.base);
    Int result_value = IntInit(&alloc.base);

    IntRoot(&result_value, &value, 4);

    bool result = IntToU64(&result_value) == 8;

    IntDeinit(&value);
    IntDeinit(&result_value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_root_rem(void) {
    WriteFmt("Testing IntRootRem\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value     = IntFrom(200, &alloc.base);
    Int root      = IntInit(&alloc.base);
    Int remainder = IntInit(&alloc.base);

    IntRootRem(&root, &remainder, &value, 3);

    bool result = IntToU64(&root) == 5;
    result      = result && (IntToU64(&remainder) == 75);

    IntDeinit(&value);
    IntDeinit(&root);
    IntDeinit(&remainder);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_sqrt(void) {
    WriteFmt("Testing IntSqrt\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value        = IntFrom(200, &alloc.base);
    Int result_value = IntInit(&alloc.base);

    IntSqrt(&result_value, &value);

    bool result = IntToU64(&result_value) == 14;

    IntDeinit(&value);
    IntDeinit(&result_value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_sqrt_rem(void) {
    WriteFmt("Testing IntSqrtRem\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value     = IntFrom(200, &alloc.base);
    Int root      = IntInit(&alloc.base);
    Int remainder = IntInit(&alloc.base);

    IntSqrtRem(&root, &remainder, &value);

    bool result = IntToU64(&root) == 14;
    result      = result && (IntToU64(&remainder) == 4);

    IntDeinit(&value);
    IntDeinit(&root);
    IntDeinit(&remainder);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_is_perfect_square(void) {
    WriteFmt("Testing IntIsPerfectSquare\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int square     = IntFrom(144, &alloc.base);
    Int non_square = IntFrom(145, &alloc.base);

    bool result = IntIsPerfectSquare(&square);
    result      = result && !IntIsPerfectSquare(&non_square);

    IntDeinit(&square);
    IntDeinit(&non_square);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_is_perfect_power(void) {
    WriteFmt("Testing IntIsPerfectPower\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int power     = IntFrom(81, &alloc.base);
    Int non_power = IntFrom(82, &alloc.base);
    Int one       = IntFrom(1, &alloc.base);

    bool result = IntIsPerfectPower(&power);
    result      = result && !IntIsPerfectPower(&non_power);
    result      = result && IntIsPerfectPower(&one);

    IntDeinit(&power);
    IntDeinit(&non_power);
    IntDeinit(&one);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_jacobi(void) {
    WriteFmt("Testing IntJacobi\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int a = IntFrom(5, &alloc.base);
    Int p = IntFrom(7, &alloc.base);
    Int b = IntFrom(9, &alloc.base);
    Int n = IntFrom(21, &alloc.base);

    bool result = IntJacobi(&a, &p) == -1;
    result      = result && (IntJacobi(&b, &n) == 0);

    IntDeinit(&a);
    IntDeinit(&p);
    IntDeinit(&b);
    IntDeinit(&n);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_square_mod(void) {
    WriteFmt("Testing IntSquareMod\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value        = IntFrom(12345, &alloc.base);
    Int mod          = IntFrom(97, &alloc.base);
    Int result_value = IntInit(&alloc.base);

    IntSquareMod(&result_value, &value, &mod);

    bool result = IntToU64(&result_value) == 94;

    IntDeinit(&value);
    IntDeinit(&mod);
    IntDeinit(&result_value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_mod_add(void) {
    WriteFmt("Testing IntModAdd\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int a            = IntFrom(100, &alloc.base);
    Int b            = IntFrom(250, &alloc.base);
    Int m            = IntFrom(13, &alloc.base);
    Int result_value = IntInit(&alloc.base);

    IntModAdd(&result_value, &a, &b, &m);

    bool result = IntToU64(&result_value) == 12;

    IntDeinit(&a);
    IntDeinit(&b);
    IntDeinit(&m);
    IntDeinit(&result_value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_mod_sub(void) {
    WriteFmt("Testing IntModSub\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int a            = IntFrom(5, &alloc.base);
    Int b            = IntFrom(9, &alloc.base);
    Int m            = IntFrom(13, &alloc.base);
    Int result_value = IntInit(&alloc.base);

    IntModSub(&result_value, &a, &b, &m);

    bool result = IntToU64(&result_value) == 9;

    IntDeinit(&a);
    IntDeinit(&b);
    IntDeinit(&m);
    IntDeinit(&result_value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_mod_mul(void) {
    WriteFmt("Testing IntModMul\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int a            = IntFrom(123, &alloc.base);
    Int b            = IntFrom(456, &alloc.base);
    Int m            = IntFrom(97, &alloc.base);
    Int result_value = IntInit(&alloc.base);

    IntModMul(&result_value, &a, &b, &m);

    bool result = IntToU64(&result_value) == 22;

    IntDeinit(&a);
    IntDeinit(&b);
    IntDeinit(&m);
    IntDeinit(&result_value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_mod_div(void) {
    WriteFmt("Testing IntModDiv\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int a            = IntFrom(10, &alloc.base);
    Int b            = IntFrom(3, &alloc.base);
    Int m            = IntFrom(13, &alloc.base);
    Int result_value = IntInit(&alloc.base);
    Int check        = IntInit(&alloc.base);

    bool result = IntModDiv(&result_value, &a, &b, &m);
    result      = result && (IntToU64(&result_value) == 12);

    IntModMul(&check, &result_value, &b, &m);
    result = result && (IntCompare(&check, 10) == 0);

    IntDeinit(&a);
    IntDeinit(&b);
    IntDeinit(&m);
    IntDeinit(&result_value);
    IntDeinit(&check);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_pow_mod_scalar(void) {
    WriteFmt("Testing IntPowMod scalar-exponent dispatch\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int base         = IntFrom(7, &alloc.base);
    Int mod          = IntFrom(13, &alloc.base);
    Int result_value = IntInit(&alloc.base);

    IntPowMod(&result_value, &base, 20u, &mod);

    bool result = IntToU64(&result_value) == 3;

    IntDeinit(&base);
    IntDeinit(&mod);
    IntDeinit(&result_value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_pow_mod_integer_exponent(void) {
    WriteFmt("Testing IntPowMod Int-exponent dispatch\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int base         = IntFrom(4, &alloc.base);
    Int exp          = IntFrom(13, &alloc.base);
    Int mod          = IntFrom(497, &alloc.base);
    Int result_value = IntInit(&alloc.base);

    IntPowMod(&result_value, &base, &exp, &mod);

    bool result = IntToU64(&result_value) == 445;

    IntDeinit(&base);
    IntDeinit(&exp);
    IntDeinit(&mod);
    IntDeinit(&result_value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_mod_inv(void) {
    WriteFmt("Testing IntModInv\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value        = IntFrom(3, &alloc.base);
    Int mod          = IntFrom(11, &alloc.base);
    Int result_value = IntInit(&alloc.base);
    Int check        = IntInit(&alloc.base);

    bool result = IntModInv(&result_value, &value, &mod);
    result      = result && (IntToU64(&result_value) == 4);

    IntModMul(&check, &value, &result_value, &mod);
    result = result && (IntToU64(&check) == 1);

    IntDeinit(&value);
    IntDeinit(&mod);
    IntDeinit(&result_value);
    IntDeinit(&check);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_mod_sqrt(void) {
    WriteFmt("Testing IntModSqrt\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value = IntFrom(10, &alloc.base);
    Int mod   = IntFrom(13, &alloc.base);
    Int root  = IntInit(&alloc.base);
    Int check = IntInit(&alloc.base);

    bool result = IntModSqrt(&root, &value, &mod);
    IntSquareMod(&check, &root, &mod);
    result = result && (IntCompare(&check, 10) == 0);

    IntDeinit(&value);
    IntDeinit(&mod);
    IntDeinit(&root);
    IntDeinit(&check);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_mod_sqrt_no_solution(void) {
    WriteFmt("Testing IntModSqrt no-solution case\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value = IntFrom(3, &alloc.base);
    Int mod   = IntFrom(7, &alloc.base);
    Int root  = IntFrom(99, &alloc.base);

    bool result = !IntModSqrt(&root, &value, &mod);
    result      = result && (IntCompare(&root, 99) == 0);

    IntDeinit(&value);
    IntDeinit(&mod);
    IntDeinit(&root);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_is_probable_prime(void) {
    WriteFmt("Testing IntIsProbablePrime\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int prime     = IntFromStr("1000000007", &alloc.base);
    Int composite = IntFrom(561, &alloc.base);

    bool result = IntIsProbablePrime(&prime);
    result      = result && !IntIsProbablePrime(&composite);

    IntDeinit(&prime);
    IntDeinit(&composite);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_next_prime(void) {
    WriteFmt("Testing IntNextPrime\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value = IntFromStr("1000000000", &alloc.base);
    Int next  = IntInit(&alloc.base);
    Str text  = StrInit(&alloc.base);

    bool ok = IntNextPrime(&next, &value);
    text    = IntToStr(&next);

    bool result = ok && ZstrCompare(StrBegin(&text), "1000000007") == 0;

    IntDeinit(&value);
    IntDeinit(&next);
    StrDeinit(&text);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_mod_inv_no_solution(void) {
    WriteFmt("Testing IntModInv no-solution case\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value        = IntFrom(6, &alloc.base);
    Int mod          = IntFrom(15, &alloc.base);
    Int result_value = IntFrom(99, &alloc.base);

    bool result = !IntModInv(&result_value, &value, &mod);
    result      = result && (IntToU64(&result_value) == 99);

    IntDeinit(&value);
    IntDeinit(&mod);
    IntDeinit(&result_value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_mod_div_no_solution(void) {
    WriteFmt("Testing IntModDiv no-solution case\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int a            = IntFrom(1, &alloc.base);
    Int b            = IntFrom(6, &alloc.base);
    Int m            = IntFrom(15, &alloc.base);
    Int result_value = IntFrom(99, &alloc.base);

    bool result = !IntModDiv(&result_value, &a, &b, &m);
    result      = result && (IntCompare(&result_value, 99) == 0);

    IntDeinit(&a);
    IntDeinit(&b);
    IntDeinit(&m);
    IntDeinit(&result_value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_add_null_result(void) {
    WriteFmt("Testing IntAdd NULL result handling\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int a = IntFrom(1, &alloc.base);
    Int b = IntFrom(2, &alloc.base);

    IntAdd(NULL, &a, &b);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

bool test_int_shift_left_null(void) {
    WriteFmt("Testing IntShiftLeft NULL handling\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    IntShiftLeft(NULL, 1);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

bool test_int_div_by_zero(void) {
    WriteFmt("Testing Int division by zero handling\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int dividend  = IntFrom(1, &alloc.base);
    Int divisor   = IntInit(&alloc.base);
    Int quotient  = IntFrom(99, &alloc.base);
    Int remainder = IntFrom(77, &alloc.base);

    bool result = !IntDivMod(&quotient, &remainder, &dividend, &divisor);

    result = result && (IntCompare(&quotient, 99) == 0);
    result = result && (IntCompare(&remainder, 77) == 0);

    IntDeinit(&dividend);
    IntDeinit(&divisor);
    IntDeinit(&quotient);
    IntDeinit(&remainder);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_root_zero_degree(void) {
    WriteFmt("Testing IntRoot zero-degree handling\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int  value     = IntFrom(16, &alloc.base);
    Int  root      = IntFrom(99, &alloc.base);
    Int  remainder = IntFrom(77, &alloc.base);
    bool result    = !IntRootRem(&root, &remainder, &value, 0);

    result = result && (IntCompare(&root, 99) == 0);
    result = result && (IntCompare(&remainder, 77) == 0);

    IntDeinit(&value);
    IntDeinit(&root);
    IntDeinit(&remainder);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_div_scalar_zero_divisor(void) {
    WriteFmt("Testing IntDiv scalar zero-divisor handling\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int dividend = IntFrom(10, &alloc.base);
    Int quotient = IntFrom(99, &alloc.base);

    IntDiv(&quotient, &dividend, 0u);
    bool result = IntCompare(&quotient, 99) == 0;

    IntDeinit(&dividend);
    IntDeinit(&quotient);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_mod_scalar_zero_modulus(void) {
    WriteFmt("Testing IntMod scalar zero-modulus handling\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value        = IntFrom(10, &alloc.base);
    Int result_value = IntFrom(99, &alloc.base);

    IntMod(&result_value, &value, 0u);
    bool result = IntCompare(&result_value, 99) == 0;

    IntDeinit(&value);
    IntDeinit(&result_value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_mod_div_zero_modulus(void) {
    WriteFmt("Testing IntModDiv zero modulus handling\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int a            = IntFrom(10, &alloc.base);
    Int b            = IntFrom(3, &alloc.base);
    Int m            = IntInit(&alloc.base);
    Int result_value = IntFrom(99, &alloc.base);

    bool result = !IntModDiv(&result_value, &a, &b, &m);
    result      = result && (IntCompare(&result_value, 99) == 0);

    IntDeinit(&a);
    IntDeinit(&b);
    IntDeinit(&m);
    IntDeinit(&result_value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_jacobi_even_denominator(void) {
    WriteFmt("Testing IntJacobi even denominator handling\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int  a      = IntFrom(3, &alloc.base);
    Int  n      = IntFrom(8, &alloc.base);
    int  symbol = 99;
    bool error  = false;
    bool result = !IntTryJacobi(&symbol, &a, &n);

    result = result && (IntJacobi(&a, &n, &error) == 0);
    result = result && (symbol == 99);
    result = result && error;

    IntDeinit(&a);
    IntDeinit(&n);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_pow_mod_scalar_zero_modulus(void) {
    WriteFmt("Testing IntPowMod scalar-exponent zero modulus handling\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int base         = IntFrom(2, &alloc.base);
    Int mod          = IntInit(&alloc.base);
    Int result_value = IntInit(&alloc.base);

    IntPowMod(&result_value, &base, 8u, &mod);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

bool test_int_pow_mod_integer_zero_modulus(void) {
    WriteFmt("Testing IntPowMod Int-exponent zero modulus handling\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int base         = IntFrom(2, &alloc.base);
    Int exp          = IntFrom(8, &alloc.base);
    Int mod          = IntInit(&alloc.base);
    Int result_value = IntFrom(99, &alloc.base);

    bool result = !IntPowMod(&result_value, &base, &exp, &mod);
    result      = result && (IntCompare(&result_value, 99) == 0);

    IntDeinit(&base);
    IntDeinit(&exp);
    IntDeinit(&mod);
    IntDeinit(&result_value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// ---------------------------------------------------------------------------
// p == 3 (mod 4): closed-form root = a^((p+1)/4) mod p.
// p = 7, a = 2 -> 3 (since 3*3 = 9 == 2 mod 7).
// Kills: int_replace(result,&root) removal (line 2490), the (p%4==3) branch
// selector (2476), and the exponent/pow scalar calls on 2480-2481.
// ---------------------------------------------------------------------------
bool test_m1_modsqrt_p3mod4_residue(void) {
    WriteFmt("Testing IntModSqrt p==3 mod 4 residue (p=7,a=2)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value = IntFrom(2, &alloc.base);
    Int mod   = IntFrom(7, &alloc.base);
    Int root  = IntFrom(99, &alloc.base);
    Int check = IntInit(&alloc.base);

    bool result = IntModSqrt(&root, &value, &mod);
    IntSquareMod(&check, &root, &mod);

    // result written, returns true, and root^2 == value (mod p).
    result = result && (IntCompare(&check, 2) == 0);
    // root must lie in [0, p).
    result = result && (IntCompare(&root, 7) < 0);
    result = result && (IntCompare(&root, 0) >= 0);
    // root must actually have been overwritten away from the sentinel 99.
    result = result && (IntCompare(&root, 99) != 0);

    IntDeinit(&value);
    IntDeinit(&mod);
    IntDeinit(&root);
    IntDeinit(&check);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// p = 7, a = 3 is a non-residue -> returns false, result preserved.
// Kills the Jacobi guard (2471) and the modulus==2 misfire (2452).
bool test_m1_modsqrt_p3mod4_nonresidue_preserves_result(void) {
    WriteFmt("Testing IntModSqrt p==3 mod 4 non-residue (p=7,a=3)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value = IntFrom(3, &alloc.base);
    Int mod   = IntFrom(7, &alloc.base);
    Int root  = IntFrom(99, &alloc.base);

    bool result = !IntModSqrt(&root, &value, &mod);
    result      = result && (IntCompare(&root, 99) == 0);

    IntDeinit(&value);
    IntDeinit(&mod);
    IntDeinit(&root);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// ---------------------------------------------------------------------------
// p == 1 (mod 4): full Tonelli-Shanks. p = 17 -> q=1, m=4. a = 2 -> root 6
// (6*6 = 36 == 2 mod 17). m=4 forces the inner i-loop to iterate several
// times and the b-squaring j-loop to run, exercising loop bounds and the
// m = i contraction. Kills 2476 (forced p3 path gives wrong root), 2580,
// 2596 loop bounds, 2644 j-loop, 2720 m=i, 2723 ok assignment.
// ---------------------------------------------------------------------------
bool test_m1_modsqrt_p1mod4_tonelli_deep(void) {
    WriteFmt("Testing IntModSqrt Tonelli deep (p=17,a=2,m=4)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value = IntFrom(2, &alloc.base);
    Int mod   = IntFrom(17, &alloc.base);
    Int root  = IntFrom(99, &alloc.base);
    Int check = IntInit(&alloc.base);

    bool result = IntModSqrt(&root, &value, &mod);
    IntSquareMod(&check, &root, &mod);

    result = result && (IntCompare(&check, 2) == 0);
    result = result && (IntCompare(&root, 17) < 0);
    result = result && (IntCompare(&root, 0) >= 0);
    result = result && (IntCompare(&root, 99) != 0);

    IntDeinit(&value);
    IntDeinit(&mod);
    IntDeinit(&root);
    IntDeinit(&check);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// p = 17, a = 4 -> root 2. Here m=4 but the inner i-loop finds order at i=2,
// so i+1 < m and the b-squaring j-loop body actually executes once, and the
// m = i contraction shrinks m. Kills the j-loop bound (2644), the j-loop
// IntSquareMod (2647), and m = i (2720).
bool test_m1_modsqrt_p1mod4_tonelli_jloop(void) {
    WriteFmt("Testing IntModSqrt Tonelli j-loop (p=17,a=4)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value = IntFrom(4, &alloc.base);
    Int mod   = IntFrom(17, &alloc.base);
    Int root  = IntFrom(99, &alloc.base);
    Int check = IntInit(&alloc.base);

    bool result = IntModSqrt(&root, &value, &mod);
    IntSquareMod(&check, &root, &mod);

    result = result && (IntCompare(&check, 4) == 0);
    result = result && (IntCompare(&root, 17) < 0);
    result = result && (IntCompare(&root, 0) >= 0);

    IntDeinit(&value);
    IntDeinit(&mod);
    IntDeinit(&root);
    IntDeinit(&check);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// p = 97, a = 3 -> root 10. The outer while(t != 1) loop runs THREE times
// (m contracts 87->5->4->2), exercising repeated re-entry, the inner i-loop
// at varying m, the j-loop, and the final ok = (t == 1) decision. This is the
// strongest single vector for the Tonelli control flow (2580, 2596, 2644,
// 2647, 2720, 2723).
bool test_m1_modsqrt_p1mod4_tonelli_multi_outer(void) {
    WriteFmt("Testing IntModSqrt Tonelli multi-outer (p=97,a=3)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value = IntFrom(3, &alloc.base);
    Int mod   = IntFrom(97, &alloc.base);
    Int root  = IntFrom(99, &alloc.base);
    Int check = IntInit(&alloc.base);

    bool result = IntModSqrt(&root, &value, &mod);
    IntSquareMod(&check, &root, &mod);

    result = result && (IntCompare(&check, 3) == 0);
    result = result && (IntCompare(&root, 97) < 0);
    result = result && (IntCompare(&root, 0) >= 0);
    result = result && (IntCompare(&root, 99) != 0);

    IntDeinit(&value);
    IntDeinit(&mod);
    IntDeinit(&root);
    IntDeinit(&check);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// p = 257 -> q=1, m=8 (deepest split). a = 2 -> root 60. Three outer
// iterations, inner j-loop running up to 3 squarings. Maximises coverage of
// the loop counters (i, j) and the m contraction across iterations.
bool test_m1_modsqrt_p1mod4_tonelli_deepest(void) {
    WriteFmt("Testing IntModSqrt Tonelli deepest (p=257,a=2,m=8)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value = IntFrom(2, &alloc.base);
    Int mod   = IntFrom(257, &alloc.base);
    Int root  = IntFrom(99, &alloc.base);
    Int check = IntInit(&alloc.base);

    bool result = IntModSqrt(&root, &value, &mod);
    IntSquareMod(&check, &root, &mod);

    result = result && (IntCompare(&check, 2) == 0);
    result = result && (IntCompare(&root, 257) < 0);
    result = result && (IntCompare(&root, 0) >= 0);

    IntDeinit(&value);
    IntDeinit(&mod);
    IntDeinit(&root);
    IntDeinit(&check);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// p = 13 -> q=3, m=2. a = 10 -> root 6 or 7 (both square to 10 mod 13).
bool test_m1_modsqrt_p1mod4_tonelli_shallow(void) {
    WriteFmt("Testing IntModSqrt Tonelli (p=13,a=10,m=2)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value = IntFrom(10, &alloc.base);
    Int mod   = IntFrom(13, &alloc.base);
    Int root  = IntFrom(99, &alloc.base);
    Int check = IntInit(&alloc.base);

    bool result = IntModSqrt(&root, &value, &mod);
    IntSquareMod(&check, &root, &mod);

    result = result && (IntCompare(&check, 10) == 0);
    result = result && (IntCompare(&root, 13) < 0);

    IntDeinit(&value);
    IntDeinit(&mod);
    IntDeinit(&root);
    IntDeinit(&check);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// p = 13 (== 1 mod 4), a = 2 is a non-residue -> returns false, result kept.
// Kills the Jacobi guard reached before the Tonelli machinery (2471).
bool test_m1_modsqrt_p1mod4_nonresidue_preserves_result(void) {
    WriteFmt("Testing IntModSqrt p==1 mod 4 non-residue (p=13,a=2)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value = IntFrom(2, &alloc.base);
    Int mod   = IntFrom(13, &alloc.base);
    Int root  = IntFrom(99, &alloc.base);

    bool result = !IntModSqrt(&root, &value, &mod);
    result      = result && (IntCompare(&root, 99) == 0);

    IntDeinit(&value);
    IntDeinit(&mod);
    IntDeinit(&root);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// ---------------------------------------------------------------------------
// a == 0 (mod p): result must be set to 0 and return true.
// Kills int_replace(result,&zero) removal at line 2448.
// ---------------------------------------------------------------------------
bool test_m1_modsqrt_zero_value_sets_result_zero(void) {
    WriteFmt("Testing IntModSqrt value 0 sets result 0\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value = IntFrom(0, &alloc.base);
    Int mod   = IntFrom(7, &alloc.base);
    Int root  = IntFrom(99, &alloc.base);

    bool result = IntModSqrt(&root, &value, &mod);
    // result overwritten to exactly 0 (not the 99 sentinel).
    result = result && (IntCompare(&root, 0) == 0);

    IntDeinit(&value);
    IntDeinit(&mod);
    IntDeinit(&root);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// value that is a nonzero multiple of the modulus reduces to a == 0 via
// int_mod before the zero check; result must still become 0. Exercises the
// reduction (2441) feeding the a==0 fast path.
bool test_m1_modsqrt_multiple_of_modulus_sets_zero(void) {
    WriteFmt("Testing IntModSqrt 21 mod 7 -> root 0\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value = IntFrom(21, &alloc.base);
    Int mod   = IntFrom(7, &alloc.base);
    Int root  = IntFrom(99, &alloc.base);

    bool result = IntModSqrt(&root, &value, &mod);
    result      = result && (IntCompare(&root, 0) == 0);

    IntDeinit(&value);
    IntDeinit(&mod);
    IntDeinit(&root);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// ---------------------------------------------------------------------------
// modulus == 2: every odd a reduces to 1, even a to 0; result := a, true.
// a = 5 -> 5 mod 2 = 1, so root must be 1. Kills int_replace(result,&a) at
// 2453 and the (modulus==2) selector scalar call at 2452.
// ---------------------------------------------------------------------------
bool test_m1_modsqrt_modulus_two_sets_result(void) {
    WriteFmt("Testing IntModSqrt modulus 2 (a=5 -> root 1)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value = IntFrom(5, &alloc.base);
    Int mod   = IntFrom(2, &alloc.base);
    Int root  = IntFrom(99, &alloc.base);

    bool result = IntModSqrt(&root, &value, &mod);
    result      = result && (IntCompare(&root, 1) == 0);

    IntDeinit(&value);
    IntDeinit(&mod);
    IntDeinit(&root);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// ---------------------------------------------------------------------------
// Even (non-2) modulus -> not a prime field -> returns false, result kept.
// Kills the IntIsEven(modulus) guard at line 2464.
// ---------------------------------------------------------------------------
bool test_m1_modsqrt_even_modulus_fails(void) {
    WriteFmt("Testing IntModSqrt even modulus 8 fails\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value = IntFrom(1, &alloc.base);
    Int mod   = IntFrom(8, &alloc.base);
    Int root  = IntFrom(99, &alloc.base);

    bool result = !IntModSqrt(&root, &value, &mod);
    result      = result && (IntCompare(&root, 99) == 0);

    IntDeinit(&value);
    IntDeinit(&mod);
    IntDeinit(&root);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Odd composite modulus (9 = 3*3) -> not prime -> returns false, result kept.
// Kills the !prime guard / prime detection at lines 2458, 2464.
bool test_m1_modsqrt_composite_modulus_fails(void) {
    WriteFmt("Testing IntModSqrt composite modulus 9 fails\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value = IntFrom(4, &alloc.base);
    Int mod   = IntFrom(9, &alloc.base);
    Int root  = IntFrom(99, &alloc.base);

    bool result = !IntModSqrt(&root, &value, &mod);
    result      = result && (IntCompare(&root, 99) == 0);

    IntDeinit(&value);
    IntDeinit(&mod);
    IntDeinit(&root);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// ---------------------------------------------------------------------------
// modulus == 0 -> early false, result preserved (line 2433 guard).
// ---------------------------------------------------------------------------
bool test_m1_modsqrt_zero_modulus_fails_preserves_result(void) {
    WriteFmt("Testing IntModSqrt zero modulus fails\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value = IntFrom(4, &alloc.base);
    Int mod   = IntInit(&alloc.base);
    Int root  = IntFrom(99, &alloc.base);

    bool result = !IntModSqrt(&root, &value, &mod);
    result      = result && (IntCompare(&root, 99) == 0);

    IntDeinit(&value);
    IntDeinit(&mod);
    IntDeinit(&root);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// ---------------------------------------------------------------------------
// value > modulus on a p==3 prime: int_mod reduction must run first so the
// computed root is for (value mod p). value = 23, p = 7 -> 23 mod 7 = 2,
// sqrt is 3/4. Confirms the reduction at 2441 is wired through to the root.
// ---------------------------------------------------------------------------
bool test_m1_modsqrt_value_reduced_before_root(void) {
    WriteFmt("Testing IntModSqrt reduces value first (23 mod 7)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value = IntFrom(23, &alloc.base);
    Int mod   = IntFrom(7, &alloc.base);
    Int root  = IntFrom(99, &alloc.base);
    Int check = IntInit(&alloc.base);

    bool result = IntModSqrt(&root, &value, &mod);
    IntSquareMod(&check, &root, &mod);

    // root^2 must equal the *reduced* value 2, not 23.
    result = result && (IntCompare(&check, 2) == 0);
    result = result && (IntCompare(&root, 7) < 0);

    IntDeinit(&value);
    IntDeinit(&mod);
    IntDeinit(&root);
    IntDeinit(&check);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

///
/// Positive correctness guard for int_mul: a large multiplication whose
/// product exercises the shift-and-add accumulation loop (multiple set
/// bits in b, carries spanning many words). Guards the loop body's
/// arithmetic broadly so any value-changing damage is caught.
///
static bool test_m10_mul_large_product(void) {
    WriteFmt("Testing int_mul large product correctness\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    /* 123456789 * 987654321 = 121932631112635269 */
    Int a       = IntFrom((u64)123456789u, &alloc.base);
    Int b       = IntFrom((u64)987654321u, &alloc.base);
    Int product = IntInit(&alloc.base);

    bool ok = IntMul(&product, &a, &b);

    bool result = ok && (IntToU64(&product) == (u64)121932631112635269ull);

    IntDeinit(&a);
    IntDeinit(&b);
    IntDeinit(&product);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

///
/// Zero-case guard: a*0 and 0*b both yield a zero Int whose value reads
/// back as 0 and whose bit length is 0. Exercises the early
/// `IntIsZero(a) || IntIsZero(b)` branch.
///
static bool test_m10_mul_zero_result(void) {
    WriteFmt("Testing int_mul zero operand\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int a       = IntFrom((u64)999u, &alloc.base);
    Int zero    = IntInit(&alloc.base);
    Int product = IntInit(&alloc.base);

    bool ok = IntMul(&product, &a, &zero);

    bool result = ok && IntIsZero(&product);
    result      = result && (IntToU64(&product) == 0u);
    result      = result && (IntBitLength(&product) == 0u);

    IntDeinit(&a);
    IntDeinit(&zero);
    IntDeinit(&product);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

///
/// Deadend: int_mul must validate its `result` pointer before using it
/// (the allocator handle is fetched from `result` via IntAllocator). A
/// NULL result must trip ValidateInt and abort. Removing that validator
/// lets the function dereference NULL.
///
static bool test_m10_mul_null_result_deadend(void) {
    WriteFmt("Testing int_mul NULL result validation\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int a = IntFrom((u64)2u, &alloc.base);
    Int b = IntFrom((u64)3u, &alloc.base);

    IntMul(NULL, &a, &b);

    DefaultAllocatorDeinit(&alloc);
    return false;
}

bool test_m11_pow_u64_contract(void) {
    WriteFmt("Testing int_pow_u64 base^exponent contract\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int base   = IntFrom(7, &alloc.base);
    Int result = IntInit(&alloc.base);

    // exponent 0 -> 1 (loop body never runs)
    IntPow(&result, &base, 0u);
    bool ok = IntToU64(&result) == 1;

    // exponent 1 -> base (one odd bit, no squaring)
    IntPow(&result, &base, 1u);
    ok = ok && (IntToU64(&result) == 7);

    // exponent 2 -> 49 (squaring path)
    IntPow(&result, &base, 2u);
    ok = ok && (IntToU64(&result) == 49);

    // exponent 5 -> 16807 (mixed odd/even bits)
    IntPow(&result, &base, 5u);
    ok = ok && (IntToU64(&result) == 16807);

    IntDeinit(&base);
    IntDeinit(&result);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Kills the cxx_remove_void_call mutant on ValidateInt(result) at the top of
// IntGCD. Real code aborts cleanly via LOG_FATAL when result is NULL. With the
// validation removed, the first use of result is IntInit(IntAllocator(result))
// inside the loop (b is non-zero so the loop body runs), which dereferences
// NULL via (result)->bits and crashes instead of aborting cleanly -- a
// different, distinguishable outcome.
bool test_m13_gcd_null_result(void) {
    WriteFmt("Testing IntGCD NULL result handling\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int a = IntFrom(12, &alloc.base);
    Int b = IntFrom(8, &alloc.base);

    IntGCD(NULL, &a, &b);

    IntDeinit(&a);
    IntDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// Kills the cxx_remove_void_call mutant on ValidateInt(a) at the top of IntGCD.
// Real code aborts cleanly via LOG_FATAL when a is NULL. With the validation
// removed, ValidateInt(b) still passes (b is valid), and the first use of a is
// IntInit(IntAllocator(a)) at line 1639, which is a raw (a)->bits dereference
// of NULL -- a crash, distinguishable from the clean LOG_FATAL abort. Unlike
// IntModAdd's a/b (re-validated by int_div_mod downstream), IntGCD touches a
// through the unvalidated IntAllocator macro first, so this is uniquely
// caller-observable.
bool test_m13_gcd_null_a(void) {
    WriteFmt("Testing IntGCD NULL a handling\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int b      = IntFrom(8, &alloc.base);
    Int result = IntInit(&alloc.base);

    IntGCD(&result, NULL, &b);

    IntDeinit(&b);
    IntDeinit(&result);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// Kills the cxx_remove_void_call mutant on ValidateInt(b) at the top of IntGCD.
// Real code aborts cleanly via LOG_FATAL when b is NULL. With the validation
// removed, the first use of b is IntInit(IntAllocator(b)) at line 1640, a raw
// (b)->bits dereference of NULL -- a crash, distinguishable from the clean
// LOG_FATAL abort.
bool test_m13_gcd_null_b(void) {
    WriteFmt("Testing IntGCD NULL b handling\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int a      = IntFrom(12, &alloc.base);
    Int result = IntInit(&alloc.base);

    IntGCD(&result, &a, NULL);

    IntDeinit(&a);
    IntDeinit(&result);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// Kills the cxx_remove_void_call mutant on ValidateInt(result) at the top of
// IntModAdd. Real code aborts cleanly via LOG_FATAL when result is NULL. With
// the validation removed, IntIsZero(modulus) passes (modulus is non-zero) and
// the first use of result is IntInit(IntAllocator(result)) at line 2030, a raw
// (result)->bits dereference of NULL -- a crash, distinguishable from the clean
// abort. (a/b/modulus validations are equivalent: int_div_mod re-validates a/b
// and IntIsZero re-validates modulus, all aborting identically.)
bool test_m13_modadd_null_result(void) {
    WriteFmt("Testing IntModAdd NULL result handling\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int a = IntFrom(7, &alloc.base);
    Int b = IntFrom(3, &alloc.base);
    Int m = IntFrom(11, &alloc.base);

    IntModAdd(NULL, &a, &b, &m);

    IntDeinit(&a);
    IntDeinit(&b);
    IntDeinit(&m);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

///
/// Deadend: IntModMul must validate `result` before touching it.
///
/// Real code aborts at `ValidateInt(result)` (Int.c:2104) when `result` is
/// NULL. Removing that void call (cxx_remove_void_call) lets the NULL pointer
/// flow into `IntInit(IntAllocator(result))`, which dereferences `&NULL->bits`
/// and crashes instead of producing the controlled LOG_FATAL the harness
/// observes. The two behaviours differ, so this kills the mutant.
///
static bool test_m14_mod_mul_null_result_deadend(void) {
    WriteFmt("Testing IntModMul NULL result validation\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int a   = IntFrom(3, &alloc.base);
    Int b   = IntFrom(5, &alloc.base);
    Int mod = IntFrom(7, &alloc.base);

    IntModMul(NULL, &a, &b, &mod);

    DefaultAllocatorDeinit(&alloc);
    return false;
}

// Kills the cxx_assign_const mutant on `borrow = true;` (Int.c:1159) inside the
// `diff < 0` branch of int_sub. The borrow bit must propagate across bit
// positions for multi-bit subtraction to be correct. For 4 - 1:
//   bits of 4 = 100, bits of 1 = 001
//   i=0: 0-1     = -1 -> diff 1, borrow set
//   i=1: 0-0-1   = -1 -> diff 1, borrow set   (depends on borrow being true)
//   i=2: 1-0-1   =  0 ->         borrow clear
//   result bits 1,1,0 = 011 = 3 (correct).
// If `borrow = true` is replaced by a const (false), the borrow never
// propagates: i=1 yields diff 0 and i=2 yields diff 1, giving 101 = 5. So the
// caller-observable difference diverges (3 vs 5).
bool test_m15_sub_borrow_propagates(void) {
    WriteFmt("Testing IntSub borrow propagation across bits\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int a            = IntFrom(4, &alloc.base);
    Int b            = IntFrom(1, &alloc.base);
    Int result_value = IntInit(&alloc.base);

    bool ok = IntSub(&result_value, &a, &b);
    ok      = ok && (IntToU64(&result_value) == 3);

    IntDeinit(&a);
    IntDeinit(&b);
    IntDeinit(&result_value);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Kills the cxx_remove_void_call mutant on ValidateInt(result) at the top of
// int_sub. With a >= b the function proceeds past the int_compare guard and
// dereferences result through INT_BITS(result) (BitVecResize). Real code aborts
// cleanly via LOG_FATAL at the validation; with it removed, the NULL result
// reaches INT_BITS(NULL) and crashes -- a different, killing outcome. (The a/b
// validations are redundant: int_compare(a, b) on the next line re-runs
// ValidateInt on both, so only the result validation is uniquely observable.)
bool test_m15_sub_null_result(void) {
    WriteFmt("Testing IntSub NULL result handling\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int a = IntFrom(9, &alloc.base);
    Int b = IntFrom(2, &alloc.base);

    int_sub(NULL, &a, &b);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// Kills the cxx_remove_void_call mutant on ValidateInt(quotient) at the top of
// int_div_u64_rem (Int.c:1558). divisor is non-zero so the divisor==0 guard
// passes, then quotient is used in IntInit(IntAllocator(quotient)) which
// dereferences quotient. Real code aborts cleanly via LOG_FATAL at the
// validation; with it removed the NULL quotient reaches IntAllocator(NULL) and
// crashes.
bool test_m15_div_u64_rem_null_quotient(void) {
    WriteFmt("Testing int_div_u64_rem NULL quotient handling\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int dividend = IntFrom(100, &alloc.base);

    int_div_u64_rem(NULL, &dividend, 7);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// Kills the cxx_remove_void_call mutant on ValidateInt(dividend) at the top of
// int_div_u64_rem (Int.c:1559). divisor is non-zero so the guard passes, then
// dividend is used in IntInit(IntAllocator(dividend)) which dereferences
// dividend. Real code aborts cleanly via LOG_FATAL at the validation; with it
// removed the NULL dividend reaches IntAllocator(NULL) and crashes.
bool test_m15_div_u64_rem_null_dividend(void) {
    WriteFmt("Testing int_div_u64_rem NULL dividend handling\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int quotient = IntInit(&alloc.base);

    int_div_u64_rem(&quotient, NULL, 7);
    IntDeinit(&quotient);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// IntLCM(result, a, b): when either operand is zero the result must be set
// to zero via `int_replace(result, &zero)` (Int.c:1674). The caller owns
// `result` and passes it pre-populated; the function does NOT re-init it,
// it only overwrites through int_replace. If that int_replace call is
// removed, `result` keeps its prior value. We pre-load result with 99 and
// assert IntLCM(result, 0, 5) drives it to 0 -- this fails (stays 99) when
// the int_replace void-call is stripped.
bool test_m16_lcm_zero_operand_replaces_result(void) {
    WriteFmt("Testing IntLCM zero-operand zeroes result\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int a            = IntFrom(0, &alloc.base);
    Int b            = IntFrom(5, &alloc.base);
    Int result_value = IntFrom(99, &alloc.base);

    bool result = IntLCM(&result_value, &a, &b);
    result      = result && IntIsZero(&result_value);
    result      = result && (IntCompare(&result_value, 0) == 0);

    IntDeinit(&a);
    IntDeinit(&b);
    IntDeinit(&result_value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Kills the cxx_le_to_lt mutant on the loop bound `degree <= max_degree` at
// IntIsPerfectPower (Int.c:1922). max_degree is floor(log2(value)) =
// IntBitLength(value) - 1. For value = 32 = 2^5, max_degree = 6 - 1 = 5, and 32
// is a perfect d-th power only for d that divides 5, i.e. d == 5 (d == 1 is
// trivial). It is NOT a perfect square (sqrt 5.65), cube (3.17), or 4th power
// (2.37). Real code's loop runs degree = 2..5 inclusive and finds the exact
// 5th root (2^5 == 32) -> returns true. With `<=` weakened to `<`, the loop
// stops at degree == 4 and never tests degree 5, so every checked degree is
// inexact -> returns false. The assertion below (expects true) therefore passes
// on real code and fails under the mutant.
bool test_m18_perfect_power_max_degree_only(void) {
    WriteFmt("Testing IntIsPerfectPower hits exponent == max_degree (32 = 2^5)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    // 32 = 2^5: only perfect-power exponent in [2, max_degree] is the boundary
    // value max_degree itself, so it distinguishes `<=` from `<`.
    Int boundary = IntFrom(32, &alloc.base);
    // 64 = 2^6 = 8^2 = 4^3 = 2^6: a perfect square found at degree 2, used as a
    // sanity anchor so the test also exercises the common early-hit path.
    Int square_anchor = IntFrom(64, &alloc.base);

    bool result = IntIsPerfectPower(&boundary);
    result      = result && IntIsPerfectPower(&square_anchor);

    IntDeinit(&boundary);
    IntDeinit(&square_anchor);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Kills the cxx_remove_void_call mutant on ValidateInt(result) at the top of
// int_div_exact (Int.c:1451). Real code aborts cleanly via LOG_FATAL when
// result is NULL: ValidateInt(NULL) -> ValidateBitVec(NULL) -> LOG_FATAL. With
// the validation removed, divisor is non-zero so IntIsZero(divisor) is false,
// and the first use of result is IntInit(IntAllocator(result)) at line 1460 --
// IntAllocator expands to BitVecAllocator(&(result)->bits) = (&NULL->bits)->
// allocator, a raw near-NULL dereference that SIGSEGVs. A crash is not the
// clean Abort() the deadend harness expects, so the mutant fails this deadend
// test, distinguishing it from real code. (ValidateInt(dividend) and
// ValidateInt(divisor) are redundant: a NULL dividend/divisor aborts identically
// downstream via int_div_mod / IntIsZero, so they are not tested here.)
bool test_m18_div_exact_null_result(void) {
    WriteFmt("Testing int_div_exact NULL result handling\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int dividend = IntFrom(12, &alloc.base);
    Int divisor  = IntFrom(4, &alloc.base);

    IntDivExact(NULL, &dividend, &divisor);

    IntDeinit(&dividend);
    IntDeinit(&divisor);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// int_add accumulates bit-by-bit with a propagating carry. Adding a value to
// itself across a full carry chain (all-ones magnitude + 1) forces the carry
// to ripple through every bit and produce one new high bit. The exact binary
// string guards the per-bit sum/carry loop.
bool test_m19_add_carry_chain(void) {
    WriteFmt("Testing int_add carry propagation\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int ones = IntFromBinary("1111111", &alloc.base); // 127
    Int one  = IntFrom(1u, &alloc.base);
    Int sum  = IntInit(&alloc.base);

    IntAdd(&sum, &ones, &one);
    Str bits = IntToBinary(&sum);

    bool result = (IntToU64(&sum) == 128u);
    result      = result && (ZstrCompare(StrBegin(&bits), "10000000") == 0);

    StrDeinit(&bits);
    IntDeinit(&ones);
    IntDeinit(&one);
    IntDeinit(&sum);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Zero-value path: root and remainder must both be reset to 0, overwriting any
// prior contents. Pre-seeds non-zero destinations so a missing int_replace
// (lines 1712/1713) leaves the stale value and fails the assertion.
static bool test_m2_root_rem_zero_value_resets_outputs(void) {
    WriteFmt("Testing IntRootRem zero-value resets both outputs\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value     = IntFrom(0, &alloc.base);
    Int root      = IntFrom(99, &alloc.base);
    Int remainder = IntFrom(77, &alloc.base);

    bool ok     = IntRootRem(&root, &remainder, &value, 3);
    bool result = ok;
    result      = result && (IntCompare(&root, 0) == 0);
    result      = result && (IntCompare(&remainder, 0) == 0);

    IntDeinit(&value);
    IntDeinit(&root);
    IntDeinit(&remainder);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Perfect square via degree-2: root=100, rem=0. The bit-length / high_shift
// computation (1730 init_const & replace_scalar, 1731 init_const) must yield an
// upper bound >= 100; if it collapses to 0/1 the search returns a far-too-small
// root. Also exercises the loop midpoint shift (1766).
static bool test_m2_root_rem_perfect_square_large(void) {
    WriteFmt("Testing IntRootRem 10000^(1/2) == 100 exact\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value     = IntFrom(10000, &alloc.base);
    Int root      = IntInit(&alloc.base);
    Int remainder = IntInit(&alloc.base);

    bool ok     = IntRootRem(&root, &remainder, &value, 2);
    bool result = ok;
    result      = result && (IntToU64(&root) == 100);
    result      = result && (IntToU64(&remainder) == 0);

    IntDeinit(&value);
    IntDeinit(&root);
    IntDeinit(&remainder);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Perfect cube: 1000^(1/3) == 10, remainder 0. Independent degree/vector to
// further constrain the high_shift bit-length math and the binary search.
static bool test_m2_root_rem_perfect_cube(void) {
    WriteFmt("Testing IntRootRem 1000^(1/3) == 10 exact\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value     = IntFrom(1000, &alloc.base);
    Int root      = IntInit(&alloc.base);
    Int remainder = IntInit(&alloc.base);

    bool ok     = IntRootRem(&root, &remainder, &value, 3);
    bool result = ok;
    result      = result && (IntToU64(&root) == 10);
    result      = result && (IntToU64(&remainder) == 0);

    IntDeinit(&value);
    IntDeinit(&root);
    IntDeinit(&remainder);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Non-perfect cube: 1001 -> root 10, remainder 1. Pins the invariant
// root^k <= value < (root+1)^k AND remainder == value - root^k, so any off-by
// the midpoint/compare path produces a wrong root or remainder.
static bool test_m2_root_rem_nonperfect_cube_remainder(void) {
    WriteFmt("Testing IntRootRem 1001^(1/3) == 10 rem 1\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value     = IntFrom(1001, &alloc.base);
    Int root      = IntInit(&alloc.base);
    Int remainder = IntInit(&alloc.base);

    bool ok     = IntRootRem(&root, &remainder, &value, 3);
    bool result = ok;
    result      = result && (IntToU64(&root) == 10);
    result      = result && (IntToU64(&remainder) == 1);

    IntDeinit(&value);
    IntDeinit(&root);
    IntDeinit(&remainder);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Just-below a perfect power: 999 -> root 9, remainder 999-729=270. Forces the
// search to settle one below 10 and validates the remainder subtraction path.
static bool test_m2_root_rem_just_below_perfect_cube(void) {
    WriteFmt("Testing IntRootRem 999^(1/3) == 9 rem 270\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value     = IntFrom(999, &alloc.base);
    Int root      = IntInit(&alloc.base);
    Int remainder = IntInit(&alloc.base);

    bool ok     = IntRootRem(&root, &remainder, &value, 3);
    bool result = ok;
    result      = result && (IntToU64(&root) == 9);
    result      = result && (IntToU64(&remainder) == 270);

    IntDeinit(&value);
    IntDeinit(&root);
    IntDeinit(&remainder);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Larger fourth root to push the binary search across several iterations:
// 1000000^(1/4): 31^4=923521, 32^4=1048576 -> root 31, rem 1000000-923521=76479.
static bool test_m2_root_rem_fourth_root_large(void) {
    WriteFmt("Testing IntRootRem 1000000^(1/4) == 31 rem 76479\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value     = IntFrom(1000000, &alloc.base);
    Int root      = IntInit(&alloc.base);
    Int remainder = IntInit(&alloc.base);

    bool ok     = IntRootRem(&root, &remainder, &value, 4);
    bool result = ok;
    result      = result && (IntToU64(&root) == 31);
    result      = result && (IntToU64(&remainder) == 76479);

    IntDeinit(&value);
    IntDeinit(&root);
    IntDeinit(&remainder);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// int_pow (Int-exponent dispatch).
//
// Kills:
//  - line 1289 cxx_replace_scalar_call on the int_pow_u64(...) tail call:
//    forcing the returned bool to a constant either drops the true return or
//    skips the actual power computation, so the observable result/return
//    diverge from base**exponent.
//  - line 1284 cxx_replace_scalar_call on !IntFitsU64(exponent) in its
//    "forced false (fits)" direction is also pinned here: with a small
//    exponent the real code must take the int_pow_u64 path and return true.
bool test_m22_pow_int_exponent_value(void) {
    WriteFmt("Testing IntPow with an Int exponent (small, exact value)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int base  = IntFrom(7, &alloc.base);
    Int exp   = IntFrom(5, &alloc.base);
    Int power = IntInit(&alloc.base);

    // 7**5 == 16807.
    bool ok     = IntPow(&power, &base, &exp);
    bool result = ok;
    result      = result && (IntToU64(&power) == 16807u);

    IntDeinit(&base);
    IntDeinit(&exp);
    IntDeinit(&power);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// int_pow line 1284 cxx_replace_scalar_call on !IntFitsU64(exponent),
// "forced true (fits)" direction: an exponent of 2**64 does NOT fit u64, so
// the real code logs and returns false leaving the result untouched. Forcing
// IntFitsU64 to report "fits" would instead attempt the power (truncating the
// exponent through IntToU64) and not return the clean false.
bool test_m22_pow_int_exponent_too_large(void) {
    WriteFmt("Testing IntPow rejects an exponent that overflows u64\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int base = IntFrom(2, &alloc.base);
    // 2**64 == 18446744073709551616 -> bit length 65, does not fit u64.
    Int exp      = IntFromStr("18446744073709551616", &alloc.base);
    Int power    = IntFrom(123, &alloc.base);
    Int sentinel = IntFrom(123, &alloc.base);

    bool rejected = !IntPow(&power, &base, &exp);
    // Result must be left untouched on rejection.
    bool result = rejected && (IntCompare(&power, &sentinel) == 0);

    IntDeinit(&base);
    IntDeinit(&exp);
    IntDeinit(&power);
    IntDeinit(&sentinel);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// int_div_i64 with a positive divisor.
//
// Kills:
//  - line 1495 cxx_replace_scalar_call on int_try_from_i64_with_allocator:
//    forcing it to report failure would make int_div_i64 return false for a
//    perfectly valid positive divisor.
//  - line 1500 col 15 cxx_replace_scalar_call and col 10 cxx_init_const on
//    `bool ok = int_div(...)`: dropping/short-circuiting the int_div call
//    leaves the quotient uncomputed or forces the wrong return value.
bool test_m22_div_i64_positive_divisor(void) {
    WriteFmt("Testing int_div_i64 with a positive divisor\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int dividend = IntFrom(1000, &alloc.base);
    Int quotient = IntInit(&alloc.base);

    // 1000 / 7 == 142 (floor).
    bool ok     = IntDiv(&quotient, &dividend, (i64)7);
    bool result = ok;
    result      = result && (IntToU64(&quotient) == 142u);

    IntDeinit(&dividend);
    IntDeinit(&quotient);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// int_div_i64 with a negative divisor must fail: int_try_from_i64_with_allocator
// rejects negatives, so int_div_i64 returns false. This anchors the "forced
// true" direction of the line 1495 cxx_replace_scalar_call mutant (which would
// wrongly skip the failure branch and attempt a division).
bool test_m22_div_i64_negative_divisor_fails(void) {
    WriteFmt("Testing int_div_i64 rejects a negative divisor\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int dividend = IntFrom(1000, &alloc.base);
    Int quotient = IntFrom(55, &alloc.base);

    bool failed = !IntDiv(&quotient, &dividend, (i64)-7);
    bool result = failed;

    IntDeinit(&dividend);
    IntDeinit(&quotient);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// int_div_exact_i64 with a positive divisor that divides exactly.
//
// Kills:
//  - line 1521 cxx_replace_scalar_call on int_try_from_i64_with_allocator.
//  - line 1526 col 15 cxx_replace_scalar_call and col 10 cxx_init_const on
//    `bool ok = int_div_exact(...)`.
bool test_m22_div_exact_i64_positive_divisor(void) {
    WriteFmt("Testing int_div_exact_i64 with an exact positive divisor\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int dividend = IntFrom(1001, &alloc.base);
    Int quotient = IntInit(&alloc.base);

    // 1001 / 7 == 143 exactly.
    bool ok     = IntDivExact(&quotient, &dividend, (i64)7);
    bool result = ok;
    result      = result && (IntToU64(&quotient) == 143u);

    IntDeinit(&dividend);
    IntDeinit(&quotient);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// int_div_exact_i64 with a negative divisor must fail at the i64 conversion,
// anchoring the "forced true" direction of the line 1521 mutant.
bool test_m22_div_exact_i64_negative_divisor_fails(void) {
    WriteFmt("Testing int_div_exact_i64 rejects a negative divisor\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int dividend = IntFrom(1001, &alloc.base);
    Int quotient = IntFrom(55, &alloc.base);

    bool failed = !IntDivExact(&quotient, &dividend, (i64)-7);
    bool result = failed;

    IntDeinit(&dividend);
    IntDeinit(&quotient);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// ---------------------------------------------------------------------------
// int_try_from_u64 / int_u64_bits.
// IntFrom(5u) must round-trip to value 5 with significant bit length 3 (101b).
//
// Kills:
//   L51:9  cxx_init_const  -- `u64 bits = int_u64_bits(value)` forced const 0
//                             makes the `bits == 0` guard return a zero Int.
//   L51:16 cxx_replace_scalar_call -- int_u64_bits(value) -> 0, same as above.
//   L96:13 cxx_post_inc_to_post_dec -- `bits++` -> `bits--` underflows u64 to a
//                             huge bit count, BitVecTryFromInteger fails, the
//                             value falls back to a zero Int.
// All three drive IntFrom(5u) to 0 (or a truncated value), so asserting the
// exact value 5 and bit length 3 distinguishes the real implementation.
// ---------------------------------------------------------------------------
bool test_m24_from_u64_roundtrip(void) {
    WriteFmt("Testing IntFrom(5u) round-trip and bit length\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value = IntFrom(5u, &alloc.base);

    bool fail = (IntCompare(&value, 5u) != 0);
    fail      = fail || (IntBitLength(&value) != 3);

    IntDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return !fail;
}

// IntFrom(1u) -> value 1, bit length 1. A second vector that still pins the
// int_u64_bits loop count (any miscount of the single set bit changes the
// stored value or trips the allocation guard).
bool test_m24_from_u64_value_one(void) {
    WriteFmt("Testing IntFrom(1u) round-trip\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value = IntFrom(1u, &alloc.base);

    bool fail = (IntCompare(&value, 1u) != 0);
    fail      = fail || (IntBitLength(&value) != 1);

    IntDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return !fail;
}

///
/// int_mul_i64 with a positive signed factor must compute value * factor.
///
/// Kills `factor < 0` -> `factor >= 0` (line 1268): under that mutation a
/// positive factor would trip the LOG_FATAL "negative scalar" abort instead
/// of multiplying. Also kills `cxx_replace_scalar_call` on the
/// `return int_mul_u64(...)` (line 1272): if the call were replaced by a
/// constant, `result` would never be updated and the assertion would fail.
///
bool test_m25_mul_i64_positive(void) {
    WriteFmt("Testing int_mul_i64 with positive scalar\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value        = IntFrom(7, &alloc.base);
    Int result_value = IntFrom(99, &alloc.base);

    /* Signed literal forces int_mul_i64 dispatch. */
    bool ok     = IntMul(&result_value, &value, (i64)13);
    bool result = ok && (IntToU64(&result_value) == 91);

    IntDeinit(&value);
    IntDeinit(&result_value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

///
/// int_mul_i64 with a zero signed factor must yield zero (no abort).
///
/// Kills `factor < 0` -> `factor <= 0` (line 1268): under that mutation a
/// factor of 0 would trip the negative-scalar LOG_FATAL abort, whereas the
/// real code multiplies by zero and produces 0.
///
bool test_m25_mul_i64_zero(void) {
    WriteFmt("Testing int_mul_i64 with zero scalar\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value        = IntFrom(123, &alloc.base);
    Int result_value = IntFrom(99, &alloc.base);

    bool ok     = IntMul(&result_value, &value, (i64)0);
    bool result = ok && (IntToU64(&result_value) == 0);

    IntDeinit(&value);
    IntDeinit(&result_value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

///
/// int_pow_i64 with a positive signed exponent must compute base^exponent.
///
/// Kills `exponent < 0` -> `exponent >= 0` (line 1342): a positive exponent
/// would trip the negative-exponent LOG_FATAL abort under that mutation.
/// Also kills `cxx_replace_scalar_call` on `return int_pow_u64(...)`
/// (line 1346): replacing the call by a constant leaves `result` uncomputed.
///
bool test_m25_pow_i64_positive(void) {
    WriteFmt("Testing int_pow_i64 with positive exponent\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int base         = IntFrom(3, &alloc.base);
    Int result_value = IntFrom(99, &alloc.base);

    bool ok     = IntPow(&result_value, &base, (i64)4);
    bool result = ok && (IntToU64(&result_value) == 81);

    IntDeinit(&base);
    IntDeinit(&result_value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

///
/// int_pow_i64 with a zero signed exponent must yield 1 (no abort).
///
/// Kills `exponent < 0` -> `exponent <= 0` (line 1342): an exponent of 0
/// would trip the negative-exponent LOG_FATAL abort under that mutation,
/// whereas the real code returns base^0 == 1.
///
bool test_m25_pow_i64_zero(void) {
    WriteFmt("Testing int_pow_i64 with zero exponent\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int base         = IntFrom(7, &alloc.base);
    Int result_value = IntFrom(99, &alloc.base);

    bool ok     = IntPow(&result_value, &base, (i64)0);
    bool result = ok && (IntToU64(&result_value) == 1);

    IntDeinit(&base);
    IntDeinit(&result_value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

///
/// int_div_u64 must compute the quotient via int_div.
///
/// Kills `cxx_init_const` on `bool ok = int_div(...)` (line 1487): replacing
/// the initializer with a constant skips the actual division so `result`
/// would never be updated and the quotient assertion would fail.
///
bool test_m25_div_u64_value(void) {
    WriteFmt("Testing int_div_u64 quotient\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int dividend     = IntFrom(100, &alloc.base);
    Int result_value = IntFrom(99, &alloc.base);

    /* Unsigned literal forces int_div_u64 dispatch. */
    bool ok     = IntDiv(&result_value, &dividend, 7u);
    bool result = ok && (IntToU64(&result_value) == 14);

    IntDeinit(&dividend);
    IntDeinit(&result_value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

///
/// int_div_exact_u64: 123456789 / 3 = 41152263 exactly.
/// Kills line 1513 cxx_init_const (`bool ok = int_div_exact(...)` forced to a
/// constant): a successful exact division must return true and compute the
/// quotient.
///
bool test_m26_div_exact_u64_returns_true_and_quotient(void) {
    WriteFmt("Testing int_div_exact_u64 success contract\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int dividend     = IntFrom(123456789, &alloc.base);
    Int result_value = IntInit(&alloc.base);

    bool ok = int_div_exact_u64(&result_value, &dividend, 3u);

    bool result = ok;
    result      = result && (IntToU64(&result_value) == 41152263u);

    IntDeinit(&dividend);
    IntDeinit(&result_value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

///
/// int_div_mod_u64: 12345678901234567890 / 97 = 127275040218913071 rem 3.
/// Kills line 1539 cxx_init_const (`bool ok = int_div_mod(...)` forced const):
/// success must return true with correct quotient AND remainder.
///
bool test_m26_div_mod_u64_returns_and_computes(void) {
    WriteFmt("Testing int_div_mod_u64 success contract\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int dividend  = IntFromStr("12345678901234567890", &alloc.base);
    Int quotient  = IntInit(&alloc.base);
    Int remainder = IntInit(&alloc.base);
    Str qtext     = StrInit(&alloc.base);

    bool ok = int_div_mod_u64(&quotient, &remainder, &dividend, 97u);
    qtext   = IntToStr(&quotient);

    bool result = ok;
    result      = result && (ZstrCompare(StrBegin(&qtext), "127275040218913071") == 0);
    result      = result && (IntToU64(&remainder) == 3u);

    StrDeinit(&qtext);
    IntDeinit(&dividend);
    IntDeinit(&quotient);
    IntDeinit(&remainder);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

///
/// int_div_mod_i64: 12345678901234567890 / 13 = 949667607787274453 rem 1.
/// Kills line 1552 cxx_init_const (`bool ok = int_div_mod(...)` forced const):
/// success must return true with correct quotient AND remainder.
///
bool test_m26_div_mod_i64_returns_and_computes(void) {
    WriteFmt("Testing int_div_mod_i64 success contract\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int dividend  = IntFromStr("12345678901234567890", &alloc.base);
    Int quotient  = IntInit(&alloc.base);
    Int remainder = IntInit(&alloc.base);
    Str qtext     = StrInit(&alloc.base);

    bool ok = int_div_mod_i64(&quotient, &remainder, &dividend, (i64)13);
    qtext   = IntToStr(&quotient);

    bool result = ok;
    result      = result && (ZstrCompare(StrBegin(&qtext), "949667607787274453") == 0);
    result      = result && (IntToU64(&remainder) == 1u);

    StrDeinit(&qtext);
    IntDeinit(&dividend);
    IntDeinit(&quotient);
    IntDeinit(&remainder);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

///
/// int_mod_i64_into: (12345678901234567890 mod 13) == 1, written into result.
/// Kills line 1614 cxx_init_const (`bool ok = int_div_mod_i64(...)` forced
/// const) and line 1614 col15 cxx_replace_scalar_call (the int_div_mod_i64
/// call replaced by a scalar constant leaves `result` uncomputed). Asserts
/// both the return value and the remainder value land in `result`.
///
bool test_m26_mod_i64_into_value_and_return(void) {
    WriteFmt("Testing int_mod_i64_into success contract\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int dividend     = IntFromStr("12345678901234567890", &alloc.base);
    Int result_value = IntInit(&alloc.base);

    bool ok = int_mod_i64_into(&result_value, &dividend, (i64)13);

    bool result = ok;
    result      = result && (IntToU64(&result_value) == 1u);

    IntDeinit(&dividend);
    IntDeinit(&result_value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

///
/// int_pow_i64_mod with a positive exponent: 7^13 mod 1000000007 == 889009735.
/// Kills line 2291 cxx_lt_to_ge (`exponent < 0` -> `exponent >= 0` would
/// LOG_FATAL on this non-negative exponent, aborting the test) and line 2295
/// cxx_replace_scalar_call (the int_pow_u64_mod call replaced by a constant
/// leaves `result` uncomputed / wrong). Real code returns true and computes
/// the correct reduced power.
///
bool test_m26_pow_i64_mod_positive_exponent(void) {
    WriteFmt("Testing int_pow_i64_mod positive exponent\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int base         = IntFrom(7, &alloc.base);
    Int modulus      = IntFrom(1000000007, &alloc.base);
    Int result_value = IntInit(&alloc.base);

    bool ok = int_pow_i64_mod(&result_value, &base, (i64)13, &modulus);

    bool result = ok;
    result      = result && (IntToU64(&result_value) == 889009735u);

    IntDeinit(&base);
    IntDeinit(&modulus);
    IntDeinit(&result_value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

///
/// int_pow_i64_mod with exponent == 0: 7^0 mod 1000000007 == 1.
/// Kills line 2291 cxx_lt_to_le (`exponent < 0` -> `exponent <= 0` would
/// LOG_FATAL when exponent == 0, aborting the test). Real code returns true
/// with result 1.
///
bool test_m26_pow_i64_mod_zero_exponent(void) {
    WriteFmt("Testing int_pow_i64_mod zero exponent\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int base         = IntFrom(7, &alloc.base);
    Int modulus      = IntFrom(1000000007, &alloc.base);
    Int result_value = IntInit(&alloc.base);

    bool ok = int_pow_i64_mod(&result_value, &base, (i64)0, &modulus);

    bool result = ok;
    result      = result && (IntToU64(&result_value) == 1u);

    IntDeinit(&base);
    IntDeinit(&modulus);
    IntDeinit(&result_value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

///
/// IntRoot: floor(4096^(1/4)) == 8 exactly. Guards the success path of
/// IntRoot computing the correct integer root.
///
bool test_m26_root_value(void) {
    WriteFmt("Testing IntRoot success contract\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value        = IntFrom(4096, &alloc.base);
    Int result_value = IntInit(&alloc.base);

    bool ok = IntRoot(&result_value, &value, 4);

    bool result = ok;
    result      = result && (IntToU64(&result_value) == 8u);

    IntDeinit(&value);
    IntDeinit(&result_value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Kills cxx_gt_to_ge on int_is_odd's length guard
//   line 224:  return BitVecLen(INT_BITS(value)) > 0 && BitVecGet(INT_BITS(value), 0);
// For a zero Int the backing BitVec has length 0.
//   - real code: 0 > 0 is false, short-circuits, returns false (zero is even).
//   - mutant (`>` -> `>=`): 0 >= 0 is true, so it evaluates
//     BitVecGet(bits, 0) on a length-0 BitVec, which trips a LOG_FATAL
//     (idx 0 >= length 0) and aborts.
// Real code returns false cleanly; the mutant crashes -> distinguishable.
bool test_m27_is_odd_zero_no_abort(void) {
    WriteFmt("Testing IntIsOdd(0) returns false without aborting\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int zero = IntInit(&alloc.base);

    bool result = (IntIsOdd(&zero) == false);

    IntDeinit(&zero);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

///
/// int_mod_u64_into: `bool ok = int_div_mod_u64(...)` computes the
/// remainder into `result`. A cxx_init_const mutant replaces that
/// initializer with a constant, skipping the call so `result` stays the
/// zero-initialised Int. 100 mod 7 == 2, so a non-zero remainder pins the
/// call having actually run.
///
static bool test_m28_mod_u64_into_computes_remainder(void) {
    WriteFmt("Testing int_mod_u64_into remainder (init-const guard)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int dividend = IntFrom((u64)100u, &alloc.base);
    Int result   = IntInit(&alloc.base);

    bool ok      = IntMod(&result, &dividend, (unsigned long)7u);
    bool correct = ok && (IntToU64(&result) == (u64)2u);

    IntDeinit(&result);
    IntDeinit(&dividend);
    DefaultAllocatorDeinit(&alloc);
    return correct;
}

///
/// Deadend: IntTrailingZeroCount validates its argument before the scan
/// loop. Same lever as above -- a zeroed (non-NULL) Int makes the mutant
/// return 0 without crashing, so the absent ValidateInt abort is observed.
///
static bool test_m28_trailing_zero_invalid_deadend(void) {
    WriteFmt("Testing IntTrailingZeroCount validation on invalid Int\n");

    Int invalid = {0};

    (void)IntTrailingZeroCount(&invalid);

    return false;
}

///
/// IntJacobiWithError writes `*error = !ok` (Int.c:2013). cxx_assign_const
/// forces that store to a fixed boolean constant, breaking one of the two
/// directions. This test exercises both: a valid odd-modulus call ((2/7) == 1)
/// must leave error == false, and an even-modulus call (IntTryJacobi fails)
/// must leave error == true. A constant store cannot satisfy both polarities,
/// so this kills the mutant regardless of which constant is substituted.
///
bool test_m29_jacobi_error_flag_success_and_failure(void) {
    WriteFmt("Testing IntJacobiWithError error flag both directions\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int a      = IntFrom(2, &alloc.base);
    Int n_odd  = IntFrom(7, &alloc.base);
    Int n_even = IntFrom(8, &alloc.base);

    bool err_ok  = true;
    bool err_bad = false;

    int sym = IntJacobiWithError(&a, &n_odd, &err_ok);
    (void)IntJacobiWithError(&a, &n_even, &err_bad);

    bool result = (sym == 1);
    result      = result && (err_ok == false);
    result      = result && (err_bad == true);

    IntDeinit(&a);
    IntDeinit(&n_odd);
    IntDeinit(&n_even);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

/*
 * IntModInv(result, value, modulus) computes the modular inverse using the
 * extended Euclidean algorithm. On success it sets a local flag `ok = true`
 * and returns it. The `cxx_assign_const` mutant at the `ok = true` site flips
 * the assigned constant, so a successful inversion would wrongly return false.
 *
 * Caller-observable invariant: when gcd(value, modulus) == 1, IntModInv must
 * return true, 0 <= result < modulus, and (value * result) % modulus == 1.
 */
bool test_m3_mod_inv_success_sets_ok(void) {
    WriteFmt("Testing IntModInv returns true on a valid inverse\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value  = IntFrom(3, &alloc.base);
    Int mod    = IntFrom(11, &alloc.base);
    Int result = IntInit(&alloc.base);
    Int check  = IntInit(&alloc.base);

    bool ok = IntModInv(&result, &value, &mod);

    /* Real code: ok == true. Mutant flipping `ok = true`: ok == false. */
    bool pass = ok;
    /* 3 * 4 = 12 == 1 (mod 11), so the inverse is 4. */
    pass = pass && (IntToU64(&result) == 4);

    IntModMul(&check, &value, &result, &mod);
    pass = pass && (IntToU64(&check) == 1);

    IntDeinit(&value);
    IntDeinit(&mod);
    IntDeinit(&result);
    IntDeinit(&check);
    DefaultAllocatorDeinit(&alloc);
    return pass;
}

/*
 * Same `ok = true` flag, exercised through the branch where the extended
 * Euclidean coefficient t comes out negative and must be folded back into
 * [0, modulus). inv(5) mod 11 = 9 (5 * 9 = 45 = 44 + 1). Still must return
 * true with a residue product of 1.
 */
bool test_m3_mod_inv_negative_t_branch(void) {
    WriteFmt("Testing IntModInv negative-coefficient normalization returns true\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value  = IntFrom(5, &alloc.base);
    Int mod    = IntFrom(11, &alloc.base);
    Int result = IntInit(&alloc.base);
    Int check  = IntInit(&alloc.base);

    bool ok = IntModInv(&result, &value, &mod);

    bool pass = ok;
    pass      = pass && (IntToU64(&result) == 9);

    IntModMul(&check, &value, &result, &mod);
    pass = pass && (IntToU64(&check) == 1);

    IntDeinit(&value);
    IntDeinit(&mod);
    IntDeinit(&result);
    IntDeinit(&check);
    DefaultAllocatorDeinit(&alloc);
    return pass;
}

// 1487 / 1500: int_div_u64 / int_div_i64 `bool ok = int_div(...)`. Under
// the faithful model the call still runs but ok becomes 42 (truthy), so
// the function returns true even when int_div genuinely fails. A zero
// divisor makes int_div_mod return false (logged, no abort); the return
// value is the observable that distinguishes real (false) from mutant
// (42). The quotient is left unchanged on failure.
bool test_fe_1487_div_u64_zero_returns_false(void) {
    WriteFmt("Testing IntDiv u64 zero-divisor return value\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int dividend = IntFrom(10, &alloc.base);
    Int quotient = IntFrom(99, &alloc.base);

    bool ok     = IntDiv(&quotient, &dividend, 0u);
    bool result = (ok == false);
    result      = result && (IntCompare(&quotient, 99) == 0);

    IntDeinit(&dividend);
    IntDeinit(&quotient);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_fe_1500_div_i64_zero_returns_false(void) {
    WriteFmt("Testing IntDiv i64 zero-divisor return value\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int dividend = IntFrom(10, &alloc.base);
    Int quotient = IntFrom(99, &alloc.base);

    bool ok     = IntDiv(&quotient, &dividend, (i64)0);
    bool result = (ok == false);
    result      = result && (IntCompare(&quotient, 99) == 0);

    IntDeinit(&dividend);
    IntDeinit(&quotient);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 1513 / 1526: int_div_exact_u64 / int_div_exact_i64 `bool ok =
// int_div_exact(...)`. int_div_exact returns false on a non-exact
// division; the faithful mutant turns that into 42 (truthy). 127 is not
// divisible by 10, so real code returns false, mutant returns true.
bool test_fe_1513_div_exact_u64_inexact_false(void) {
    WriteFmt("Testing IntDivExact u64 inexact return value\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int dividend = IntFrom(127, &alloc.base);
    Int quotient = IntInit(&alloc.base);

    bool ok     = IntDivExact(&quotient, &dividend, 10u);
    bool result = (ok == false);

    IntDeinit(&dividend);
    IntDeinit(&quotient);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_fe_1526_div_exact_i64_inexact_false(void) {
    WriteFmt("Testing IntDivExact i64 inexact return value\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int dividend = IntFrom(127, &alloc.base);
    Int quotient = IntInit(&alloc.base);

    bool ok     = IntDivExact(&quotient, &dividend, (i64)10);
    bool result = (ok == false);

    IntDeinit(&dividend);
    IntDeinit(&quotient);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 1539 / 1552: int_div_mod_u64 / int_div_mod_i64 `bool ok =
// int_div_mod(...)`. Zero divisor makes int_div_mod return false; the
// faithful mutant yields 42 (truthy). Observe the return value.
bool test_fe_1539_div_mod_u64_zero_returns_false(void) {
    WriteFmt("Testing IntDivMod u64 zero-divisor return value\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int dividend  = IntFrom(127, &alloc.base);
    Int quotient  = IntInit(&alloc.base);
    Int remainder = IntInit(&alloc.base);

    bool ok     = IntDivMod(&quotient, &remainder, &dividend, 0u);
    bool result = (ok == false);

    IntDeinit(&dividend);
    IntDeinit(&quotient);
    IntDeinit(&remainder);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_fe_1552_div_mod_i64_zero_returns_false(void) {
    WriteFmt("Testing IntDivMod i64 zero-divisor return value\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int dividend  = IntFrom(127, &alloc.base);
    Int quotient  = IntInit(&alloc.base);
    Int remainder = IntInit(&alloc.base);

    bool ok     = IntDivMod(&quotient, &remainder, &dividend, (i64)0);
    bool result = (ok == false);

    IntDeinit(&dividend);
    IntDeinit(&quotient);
    IntDeinit(&remainder);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 1606 / 1614: int_mod_u64_into / int_mod_i64_into `bool ok =
// int_div_mod_*(...)`. Zero divisor makes the underlying div_mod return
// false; the faithful mutant yields 42 (truthy). Observe the return value.
bool test_fe_1606_mod_u64_zero_returns_false(void) {
    WriteFmt("Testing IntMod u64 zero-divisor return value\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int dividend     = IntFrom(127, &alloc.base);
    Int result_value = IntInit(&alloc.base);

    bool ok     = IntMod(&result_value, &dividend, 0u);
    bool result = (ok == false);

    IntDeinit(&dividend);
    IntDeinit(&result_value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_fe_1614_mod_i64_zero_returns_false(void) {
    WriteFmt("Testing IntMod i64 zero-divisor return value\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int dividend     = IntFrom(127, &alloc.base);
    Int result_value = IntInit(&alloc.base);

    bool ok     = IntMod(&result_value, &dividend, (i64)0);
    bool result = (ok == false);

    IntDeinit(&dividend);
    IntDeinit(&result_value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 1730: IntRootRem `u64 bits = IntBitLength(value)`. The faithful mutant
// freezes bits at 42, so the binary-search upper bound 2^(bits/degree)
// collapses below the true root for a value whose bit length exceeds 42.
// 2^50 is a perfect square with sqrt 2^25 = 33554432; the real bound
// 2^(51/2)=2^25 reaches it, the mutant bound 2^(42/2)=2^21 cannot, so the
// mutant returns a wrong (too small) root.
bool test_fe_1730_root_rem_large_value(void) {
    WriteFmt("Testing IntRootRem bit-length derived bound (large value)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value = IntFrom(1, &alloc.base);
    IntShiftLeft(&value, 50); // 2^50

    Int root      = IntInit(&alloc.base);
    Int remainder = IntInit(&alloc.base);

    bool ok     = IntSqrtRem(&root, &remainder, &value);
    bool result = ok && (IntToU64(&root) == 33554432u); // 2^25
    result      = result && (IntToU64(&remainder) == 0);

    IntDeinit(&value);
    IntDeinit(&root);
    IntDeinit(&remainder);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 1986: IntTryJacobi reciprocity flip `result = -result`. The faithful
// cxx_assign_const mutation `result = 42` overwrites the running symbol
// with 42, so the final *out becomes 42 instead of the true Jacobi value.
// (3/7) reduces through the both-3-mod-4 reciprocity flip to -1.
bool test_fe_1986_jacobi_reciprocity_sign(void) {
    WriteFmt("Testing IntJacobi reciprocity sign flip\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int a = IntFrom(3, &alloc.base);
    Int n = IntFrom(7, &alloc.base);

    int  symbol = 99;
    bool ok     = IntTryJacobi(&symbol, &a, &n);
    bool result = ok && (symbol == -1);

    IntDeinit(&a);
    IntDeinit(&n);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 2596: IntModSqrt Tonelli-Shanks inner loop `for (i = 1; i < m; i++)`.
// The faithful swap `i >= m` is false at entry (i=1) whenever m>=2, so the
// loop that finds the least i with t^(2^i)==1 never runs; i stays 1 and
// the subsequent reduction step diverges, yielding a root that does not
// square back. p=17 has p-1=2^4*1 (m=4), so the inner loop genuinely
// iterates. sqrt(2) mod 17 = 6 (6^2 = 36 = 2 mod 17).
bool test_fe_2596_mod_sqrt_tonelli_inner(void) {
    WriteFmt("Testing IntModSqrt Tonelli-Shanks inner loop\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value = IntFrom(2, &alloc.base);
    Int mod   = IntFrom(17, &alloc.base);
    Int root  = IntInit(&alloc.base);
    Int check = IntInit(&alloc.base);

    bool ok = IntModSqrt(&root, &value, &mod);
    IntSquareMod(&check, &root, &mod);
    bool result = ok && (IntCompare(&check, 2) == 0);

    IntDeinit(&value);
    IntDeinit(&mod);
    IntDeinit(&root);
    IntDeinit(&check);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_m4_one_not_prime(void) {
    WriteFmt("Testing IntIsProbablePrime(1) == false\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value = IntFrom(1, &alloc.base);

    bool result = !IntIsProbablePrime(&value);

    IntDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_m4_two_is_prime(void) {
    WriteFmt("Testing IntIsProbablePrime(2) == true\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value = IntFrom(2, &alloc.base);

    bool result = IntIsProbablePrime(&value);

    IntDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_m4_four_not_prime(void) {
    WriteFmt("Testing IntIsProbablePrime(4) == false\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value = IntFrom(4, &alloc.base);

    bool result = !IntIsProbablePrime(&value);

    IntDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_m4_three_is_prime(void) {
    WriteFmt("Testing IntIsProbablePrime(3) == true\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value = IntFrom(3, &alloc.base);

    bool result = IntIsProbablePrime(&value);

    IntDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_m4_prime_97(void) {
    WriteFmt("Testing IntIsProbablePrime(97) == true (deep witness loop)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value = IntFrom(97, &alloc.base);

    bool result = IntIsProbablePrime(&value);

    IntDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_m4_composite_561(void) {
    WriteFmt("Testing IntIsProbablePrime(561) == false (Carmichael)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value = IntFrom(561, &alloc.base);

    bool result = !IntIsProbablePrime(&value);

    IntDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_m4_composite_1763(void) {
    WriteFmt("Testing IntIsProbablePrime(1763 = 41*43) == false (reaches MR)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value = IntFrom(1763, &alloc.base);

    bool result = !IntIsProbablePrime(&value);

    IntDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_m4_spsp_1373653_composite(void) {
    WriteFmt("Testing IntIsProbablePrime(1373653) == false (SPSP base 2,3)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value = IntFromStr("1373653", &alloc.base);

    bool result = !IntIsProbablePrime(&value);

    IntDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_m4_large_prime(void) {
    WriteFmt("Testing IntIsProbablePrime(1000000007) == true\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value = IntFromStr("1000000007", &alloc.base);

    bool result = IntIsProbablePrime(&value);

    IntDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_m4_error_flag_cleared(void) {
    WriteFmt("Testing IntIsProbablePrime clears the error flag\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int  value = IntFrom(97, &alloc.base);
    bool error = true;

    bool prime  = IntIsProbablePrime(&value, &error);
    bool result = prime && !error;

    IntDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

///
/// Correctness over a multi-bit dividend. Exercises the full
/// shift/subtract loop. Kills the success-path return mutant
/// (`ok = true` at the tail) because it asserts the call returns true
/// AND yields the exact quotient and remainder.
///
static bool test_m5_div_mod_large_correct(void) {
    WriteFmt("Testing int_div_mod large-value correctness\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int dividend  = IntFromStr("12345678901234567890123456789", &alloc.base);
    Int divisor   = IntFromStr("987654321987654321", &alloc.base);
    Int quotient  = IntInit(&alloc.base);
    Int remainder = IntInit(&alloc.base);

    bool ok = int_div_mod(&quotient, &remainder, &dividend, &divisor);

    Str qtext = IntToStr(&quotient);
    Str rtext = IntToStr(&remainder);

    bool result = ok;
    result      = result && (ZstrCompare(StrBegin(&qtext), "12499999874") == 0);
    result      = result && (ZstrCompare(StrBegin(&rtext), "833333448067901235") == 0);

    StrDeinit(&qtext);
    StrDeinit(&rtext);
    IntDeinit(&dividend);
    IntDeinit(&divisor);
    IntDeinit(&quotient);
    IntDeinit(&remainder);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

///
/// Smaller verifiable case: 1000000007 / 97 = 10309278 rem 41.
/// A second, independent kill for the success-path return mutant and a
/// guard on the shift/subtract bit-setting loop producing the exact
/// quotient bits.
///
static bool test_m5_div_mod_known_quot_rem(void) {
    WriteFmt("Testing int_div_mod 1000000007 / 97\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int dividend  = IntFromStr("1000000007", &alloc.base);
    Int divisor   = IntFromStr("97", &alloc.base);
    Int quotient  = IntInit(&alloc.base);
    Int remainder = IntInit(&alloc.base);

    bool ok = int_div_mod(&quotient, &remainder, &dividend, &divisor);

    bool result = ok;
    result      = result && (IntToU64(&quotient) == 10309278u);
    result      = result && (IntToU64(&remainder) == 41u);

    IntDeinit(&dividend);
    IntDeinit(&divisor);
    IntDeinit(&quotient);
    IntDeinit(&remainder);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

///
/// Exact division: 11975308533 / 97 = 123456789 rem 0. Confirms the
/// remainder is fully reduced to zero by the subtract loop and the
/// success return path.
///
static bool test_m5_div_mod_exact_zero_remainder(void) {
    WriteFmt("Testing int_div_mod exact division remainder == 0\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int dividend  = IntFromStr("11975308533", &alloc.base);
    Int divisor   = IntFromStr("97", &alloc.base);
    Int quotient  = IntInit(&alloc.base);
    Int remainder = IntInit(&alloc.base);

    bool ok = int_div_mod(&quotient, &remainder, &dividend, &divisor);

    bool result = ok;
    result      = result && (IntToU64(&quotient) == 123456789u);
    result      = result && IntIsZero(&remainder);

    IntDeinit(&dividend);
    IntDeinit(&divisor);
    IntDeinit(&quotient);
    IntDeinit(&remainder);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

///
/// dividend < divisor takes the else branch: quotient must be zero and
/// the remainder must equal the dividend unchanged.
///
static bool test_m5_div_mod_dividend_smaller(void) {
    WriteFmt("Testing int_div_mod dividend < divisor\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int dividend  = IntFromStr("42", &alloc.base);
    Int divisor   = IntFromStr("1000", &alloc.base);
    Int quotient  = IntInit(&alloc.base);
    Int remainder = IntInit(&alloc.base);

    bool ok = int_div_mod(&quotient, &remainder, &dividend, &divisor);

    bool result = ok;
    result      = result && IntIsZero(&quotient);
    result      = result && (IntToU64(&remainder) == 42u);

    IntDeinit(&dividend);
    IntDeinit(&divisor);
    IntDeinit(&quotient);
    IntDeinit(&remainder);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

///
/// Division by zero returns false and leaves the destinations usable.
/// Guards the early IntIsZero(divisor) bailout.
///
static bool test_m5_div_mod_by_zero_returns_false(void) {
    WriteFmt("Testing int_div_mod division by zero\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int dividend  = IntFromStr("12345", &alloc.base);
    Int divisor   = IntInit(&alloc.base); /* zero */
    Int quotient  = IntInit(&alloc.base);
    Int remainder = IntInit(&alloc.base);

    bool ok = int_div_mod(&quotient, &remainder, &dividend, &divisor);

    bool result = (ok == false);

    IntDeinit(&dividend);
    IntDeinit(&divisor);
    IntDeinit(&quotient);
    IntDeinit(&remainder);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

///
/// Deadend: a NULL quotient must be rejected by ValidateInt(quotient)
/// at the top of int_div_mod (clean LOG_FATAL abort). Without that
/// validation the NULL is dereferenced later via IntAllocator(quotient).
///
static bool test_m5_div_mod_null_quotient_aborts(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    Int dividend  = IntFromStr("100", &alloc.base);
    Int divisor   = IntFromStr("7", &alloc.base);
    Int remainder = IntInit(&alloc.base);

    /* Must abort inside int_div_mod via ValidateInt(quotient). */
    int_div_mod(NULL, &remainder, &dividend, &divisor);

    IntDeinit(&dividend);
    IntDeinit(&divisor);
    IntDeinit(&remainder);
    DefaultAllocatorDeinit(&alloc);
    return true;
}

///
/// Deadend: a NULL remainder must be rejected by ValidateInt(remainder)
/// at the top of int_div_mod (clean LOG_FATAL abort). Without it the
/// NULL is dereferenced later via IntInit(IntAllocator(remainder)).
///
static bool test_m5_div_mod_null_remainder_aborts(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    Int dividend = IntFromStr("100", &alloc.base);
    Int divisor  = IntFromStr("7", &alloc.base);
    Int quotient = IntInit(&alloc.base);

    /* Must abort inside int_div_mod via ValidateInt(remainder). */
    int_div_mod(&quotient, NULL, &dividend, &divisor);

    IntDeinit(&dividend);
    IntDeinit(&divisor);
    IntDeinit(&quotient);
    DefaultAllocatorDeinit(&alloc);
    return true;
}

bool test_m6_pow_mod_acc_reduction_mod_one(void) {
    WriteFmt("Testing int_pow_mod acc=1 mod 1 reduction\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int base         = IntFrom(7, &alloc.base);
    Int exp          = IntFrom(0, &alloc.base);
    Int mod          = IntFrom(1, &alloc.base);
    Int result_value = IntFrom(99, &alloc.base);

    bool ok = int_pow_mod(&result_value, &base, &exp, &mod);
    ok      = ok && (IntCompare(&result_value, 0) == 0);

    IntDeinit(&base);
    IntDeinit(&exp);
    IntDeinit(&mod);
    IntDeinit(&result_value);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

bool test_m6_pow_mod_mod_one_nonzero_exp(void) {
    WriteFmt("Testing int_pow_mod x^e mod 1 == 0\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int base         = IntFrom(123456, &alloc.base);
    Int exp          = IntFrom(11, &alloc.base);
    Int mod          = IntFrom(1, &alloc.base);
    Int result_value = IntFrom(42, &alloc.base);

    bool ok = int_pow_mod(&result_value, &base, &exp, &mod);
    ok      = ok && (IntCompare(&result_value, 0) == 0);

    IntDeinit(&base);
    IntDeinit(&exp);
    IntDeinit(&mod);
    IntDeinit(&result_value);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

bool test_m6_pow_mod_known_vector(void) {
    WriteFmt("Testing int_pow_mod 3^13 mod 497\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int base         = IntFrom(3, &alloc.base);
    Int exp          = IntFrom(13, &alloc.base);
    Int mod          = IntFrom(497, &alloc.base);
    Int result_value = IntInit(&alloc.base);

    bool ok = int_pow_mod(&result_value, &base, &exp, &mod);
    ok      = ok && (IntToU64(&result_value) == 444);

    IntDeinit(&base);
    IntDeinit(&exp);
    IntDeinit(&mod);
    IntDeinit(&result_value);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

bool test_m6_pow_mod_no_internal_leak(void) {
    WriteFmt("Testing int_pow_mod frees internal temporaries\n");

    DebugAllocator dbg = DebugAllocatorInit();

    Int base         = IntFrom(123456789u, &dbg.base);
    Int exp          = IntFrom(987654321u, &dbg.base); // many set bits + bits
    Int mod          = IntFrom(1000000007u, &dbg.base);
    Int result_value = IntInit(&dbg.base);

    bool ok = int_pow_mod(&result_value, &base, &exp, &mod);

    IntDeinit(&base);
    IntDeinit(&exp);
    IntDeinit(&mod);
    IntDeinit(&result_value);

    // After releasing every Int the test created, nothing the function
    // allocated internally should remain outstanding.
    ok = ok && (DebugAllocatorLiveCount(&dbg) == 0);

    DebugAllocatorDeinit(&dbg);
    return ok;
}

///
/// value <= 1 path: IntNextPrime(0) must set result to 2 and return true.
/// Kills the scalar-call replacement of int_try_from_u64 (line 2875) and the
/// removal of int_replace(result, &two) (line 2879): if int_replace is dropped
/// the result never becomes 2, and if the conversion is forced to fail the call
/// returns false instead of true.
///
bool test_m7_next_prime_of_zero_is_two(void) {
    WriteFmt("Testing IntNextPrime(0) == 2\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value = IntFrom(0, &alloc.base);
    Int next  = IntInit(&alloc.base);

    bool ok = IntNextPrime(&next, &value);

    bool result = ok && (IntToU64(&next) == 2);

    IntDeinit(&value);
    IntDeinit(&next);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

///
/// value > 1, candidate odd: IntNextPrime(10) -> 11.
/// Kills:
///   - line 2872 int_compare_u64(value,1) scalar-call replacement (would force
///     the value<=1 branch and return 2 instead of 11),
///   - line 2890 int_add_u64(candidate,+1) scalar-call replacement (would
///     return false instead of computing 11),
///   - line 2894 int_compare_u64(candidate,2) scalar-call replacement (would
///     enter the candidate<=2 branch and return 2 instead of 11).
///
bool test_m7_next_prime_of_ten_is_eleven(void) {
    WriteFmt("Testing IntNextPrime(10) == 11\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value = IntFrom(10, &alloc.base);
    Int next  = IntInit(&alloc.base);

    bool ok = IntNextPrime(&next, &value);

    bool result = ok && (IntToU64(&next) == 11);

    IntDeinit(&value);
    IntDeinit(&next);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

///
/// value > 1, candidate even: IntNextPrime(9) -> candidate 10 is even, must be
/// bumped to 11. Kills line 2907 int_add_u64(candidate,+1) scalar-call
/// replacement: forcing that add to fail returns false instead of 11.
///
bool test_m7_next_prime_of_nine_is_eleven(void) {
    WriteFmt("Testing IntNextPrime(9) == 11\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value = IntFrom(9, &alloc.base);
    Int next  = IntInit(&alloc.base);

    bool ok = IntNextPrime(&next, &value);

    bool result = ok && (IntToU64(&next) == 11);

    IntDeinit(&value);
    IntDeinit(&next);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

///
/// Deadend: passing a NULL result must trip ValidateInt(result) (line 2869)
/// and abort. If that validator call is removed, the NULL pointer reaches
/// IntInit(IntAllocator(result)) and dereferences NULL (a crash, not a clean
/// abort), so the deadend harness no longer observes the expected abort.
///
bool test_m7_next_prime_null_result(void) {
    WriteFmt("Testing IntNextPrime(NULL result) aborts\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value = IntFrom(10, &alloc.base);

    (void)IntNextPrime(NULL, &value);

    IntDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return true;
}

// Kills the cxx_replace_scalar_call mutant on int_sub(result, &ar, &br) at the
// IntGE(ar, br) branch of IntModSub. With a >= b (mod m) the result must be the
// real difference (a - b) mod m. If the int_sub call is replaced by a scalar,
// `result` is never written (stays 0) and IntModSub takes the error path
// returning false, so both the return value and the magnitude diverge.
bool test_m8_modsub_ge_branch_subtracts(void) {
    WriteFmt("Testing IntModSub a>=b branch performs real subtraction\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int a            = IntFrom(9, &alloc.base);
    Int b            = IntFrom(5, &alloc.base);
    Int m            = IntFrom(13, &alloc.base);
    Int result_value = IntInit(&alloc.base);

    bool ok = IntModSub(&result_value, &a, &b, &m);
    ok      = ok && (IntToU64(&result_value) == 4);

    IntDeinit(&a);
    IntDeinit(&b);
    IntDeinit(&m);
    IntDeinit(&result_value);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Kills the cxx_remove_void_call mutant on ValidateInt(result) at the top of
// IntModSub. Real code aborts cleanly via LOG_FATAL when result is NULL. With
// the validation removed, the first use of result is IntAllocator(result),
// which dereferences NULL and crashes instead of aborting cleanly -- a
// different, distinguishable outcome. (a/b/modulus validations are equivalent:
// downstream IntIsZero/int_mod re-validate and abort identically, so only the
// result validation is uniquely caller-observable here.)
bool test_m8_modsub_null_result(void) {
    WriteFmt("Testing IntModSub NULL result handling\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int a = IntFrom(7, &alloc.base);
    Int b = IntFrom(3, &alloc.base);
    Int m = IntFrom(11, &alloc.base);

    IntModSub(NULL, &a, &b, &m);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// Pins base**exp mod m to an independently computed value. A correct call must
// return true with the exact reduced result. The cxx_replace_scalar_call mutant
// on the int_mod() at line 2185 flips the call's return so the function takes
// the failure branch and returns false -> this assertion fails under mutation.
// 3^13 mod 17 = 12 (3^8=16, 3^4=13, 3^1=3; 16*13*3 mod 17 = 12).
bool test_m9_pow_u64_mod_known_value(void) {
    WriteFmt("Testing int_pow_u64_mod known value 3^13 mod 17\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int base         = IntFrom(3, &alloc.base);
    Int mod          = IntFrom(17, &alloc.base);
    Int result_value = IntInit(&alloc.base);

    bool ok = IntPowMod(&result_value, &base, 13u, &mod);

    bool result = ok && (IntToU64(&result_value) == 12);

    IntDeinit(&base);
    IntDeinit(&mod);
    IntDeinit(&result_value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Exponent 0: the while loop body never runs, acc stays 1 mod m. Pins that the
// initial acc = (1 mod m) reduction path produces 1 for m > 1.
bool test_m9_pow_u64_mod_exponent_zero(void) {
    WriteFmt("Testing int_pow_u64_mod exponent zero -> 1\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int base         = IntFrom(123456789, &alloc.base);
    Int mod          = IntFrom(1000000007, &alloc.base);
    Int result_value = IntInit(&alloc.base);

    bool ok = IntPowMod(&result_value, &base, 0u, &mod);

    bool result = ok && (IntToU64(&result_value) == 1);

    IntDeinit(&base);
    IntDeinit(&mod);
    IntDeinit(&result_value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Larger prime modulus with a non-trivial base/exponent so the reduction inside
// each squaring genuinely wraps. 2^64 mod 1000000007 computed independently.
// 2^30 = 1073741824 mod p = 73741817; squaring chain yields 2^64 mod p.
// 2^64 = 18446744073709551616; 18446744073709551616 mod 1000000007 = 582344008.
bool test_m9_pow_u64_mod_large_modulus(void) {
    WriteFmt("Testing int_pow_u64_mod 2^64 mod 1000000007\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int base         = IntFrom(2, &alloc.base);
    Int mod          = IntFrom(1000000007, &alloc.base);
    Int result_value = IntInit(&alloc.base);

    bool ok = IntPowMod(&result_value, &base, 64u, &mod);

    bool result = ok && (IntToU64(&result_value) == 582344008);

    IntDeinit(&base);
    IntDeinit(&mod);
    IntDeinit(&result_value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// ValidateInt(result) at line 2170: NULL result aborts before any work.
bool test_m9_pow_u64_mod_null_result(void) {
    WriteFmt("Testing int_pow_u64_mod NULL result aborts\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int base = IntFrom(2, &alloc.base);
    Int mod  = IntFrom(7, &alloc.base);

    IntPowMod((Int *)NULL, &base, 5u, &mod);

    DefaultAllocatorDeinit(&alloc);
    return false;
}

// ValidateInt(base) at line 2171: NULL base aborts before any work.
bool test_m9_pow_u64_mod_null_base(void) {
    WriteFmt("Testing int_pow_u64_mod NULL base aborts\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int result_value = IntInit(&alloc.base);
    Int mod          = IntFrom(7, &alloc.base);

    IntPowMod(&result_value, (Int *)NULL, 5u, &mod);

    DefaultAllocatorDeinit(&alloc);
    return false;
}

// ValidateInt(modulus) at line 2172: NULL modulus aborts before any work.
bool test_m9_pow_u64_mod_null_modulus(void) {
    WriteFmt("Testing int_pow_u64_mod NULL modulus aborts\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int result_value = IntInit(&alloc.base);
    Int base         = IntFrom(2, &alloc.base);

    IntPowMod(&result_value, &base, 5u, (Int *)NULL);

    DefaultAllocatorDeinit(&alloc);
    return false;
}

bool test_mul_nonzero_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    Int x = IntFrom(123456u, a);
    Int y = IntFrom(7891011u, a);
    Int r = IntInit(a);

    bool ok = IntMul(&r, &x, &y);
    ok      = ok && IntToU64(&r) == 123456ull * 7891011ull;

    IntDeinit(&x);
    IntDeinit(&y);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_mul_zero_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    Int x = IntFrom(0u, a);
    Int y = IntFrom(7891011u, a);
    Int r = IntFrom(42u, a); // r starts non-empty so the internal deinit matters

    bool ok = IntMul(&r, &x, &y);
    ok      = ok && IntIsZero(&r);

    IntDeinit(&x);
    IntDeinit(&y);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_add_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    Int x = IntFrom(0xFFFFFFFFull, a);
    Int y = IntFrom(0xFFFFFFFFull, a);
    Int r = IntFrom(7u, a);

    bool ok = IntAdd(&r, &x, &y);
    ok      = ok && IntToU64(&r) == 0xFFFFFFFFull + 0xFFFFFFFFull;

    IntDeinit(&x);
    IntDeinit(&y);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_sub_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    Int x = IntFrom(1000000u, a);
    Int y = IntFrom(999983u, a);
    Int r = IntFrom(7u, a);

    bool ok = IntSub(&r, &x, &y);
    ok      = ok && IntToU64(&r) == 17u;

    IntDeinit(&x);
    IntDeinit(&y);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_add_u64_in_place_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    // IntAdd with a u64 operand drives int_add_u64 -> int_add_u64_in_place,
    // which frees lhs/rhs temporaries on success (lines 276/277).
    Int x = IntFrom(500u, a);
    Int r = IntFrom(7u, a);

    bool ok = IntAdd(&r, &x, 250u);
    ok      = ok && IntToU64(&r) == 750u;

    IntDeinit(&x);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_mul_u64_in_place_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    // IntMul with a u64 operand drives int_mul_u64 -> int_mul_u64_in_place,
    // which frees lhs/rhs temporaries on success (lines 251/252).
    Int x = IntFrom(500u, a);
    Int r = IntFrom(7u, a);

    bool ok = IntMul(&r, &x, 13u);
    ok      = ok && IntToU64(&r) == 6500u;

    IntDeinit(&x);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_sub_u64_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    Int x = IntFrom(1000u, a);
    Int r = IntFrom(7u, a);

    bool ok = IntSub(&r, &x, 250u); // int_sub_u64 frees rhs on success (1187)
    ok      = ok && IntToU64(&r) == 750u;

    IntDeinit(&x);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_div_mod_ge_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    // dividend >= divisor drives the long-division branch (lines 1378..1411,
    // 1420..1423 replace/deinit).
    Int dvd = IntFrom(1000003u, a);
    Int dvs = IntFrom(101u, a);
    Int q   = IntFrom(7u, a);
    Int r   = IntFrom(9u, a);

    bool ok = IntDivMod(&q, &r, &dvd, &dvs);
    ok      = ok && IntToU64(&q) == 1000003u / 101u && IntToU64(&r) == 1000003u % 101u;

    IntDeinit(&dvd);
    IntDeinit(&dvs);
    IntDeinit(&q);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_div_mod_lt_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    // dividend < divisor drives the else branch (line 1413: deinit+reinit q).
    Int dvd = IntFrom(50u, a);
    Int dvs = IntFrom(101u, a);
    Int q   = IntFrom(7u, a);
    Int r   = IntFrom(9u, a);

    bool ok = IntDivMod(&q, &r, &dvd, &dvs);
    ok      = ok && IntIsZero(&q) && IntToU64(&r) == 50u;

    IntDeinit(&dvd);
    IntDeinit(&dvs);
    IntDeinit(&q);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_div_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    Int dvd = IntFrom(1000003u, a);
    Int dvs = IntFrom(101u, a);
    Int r   = IntFrom(7u, a);

    bool ok = IntDiv(&r, &dvd, &dvs); // int_div frees remainder on success (1445)
    ok      = ok && IntToU64(&r) == 1000003u / 101u;

    IntDeinit(&dvd);
    IntDeinit(&dvs);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_div_exact_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    Int dvd = IntFrom(1000u, a);
    Int dvs = IntFrom(8u, a);
    Int r   = IntFrom(7u, a);

    bool ok = IntDivExact(&r, &dvd, &dvs); // frees remainder on success (1474)
    ok      = ok && IntToU64(&r) == 125u;

    IntDeinit(&dvd);
    IntDeinit(&dvs);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_div_scalar_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    // int_div_u64 / int_div_exact_u64 / int_div_i64 etc. free divisor_value
    // on success (1488, 1514, 1501, 1527).
    Int dvd = IntFrom(1000u, a);
    Int r1  = IntFrom(7u, a);
    Int r2  = IntFrom(7u, a);
    Int r3  = IntFrom(7u, a);

    bool ok = IntDiv(&r1, &dvd, 8u);
    ok      = ok && IntDivExact(&r2, &dvd, 8u);
    ok      = ok && IntDiv(&r3, &dvd, (i64)8);
    ok      = ok && IntToU64(&r1) == 125u && IntToU64(&r2) == 125u && IntToU64(&r3) == 125u;

    IntDeinit(&dvd);
    IntDeinit(&r1);
    IntDeinit(&r2);
    IntDeinit(&r3);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_div_mod_scalar_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    // int_div_mod_u64 / int_div_mod_i64 free divisor_value on success
    // (1540, 1553); int_mod_u64_into / int_mod_i64_into free quotient (1607/1615).
    Int dvd = IntFrom(1003u, a);
    Int q   = IntFrom(7u, a);
    Int r   = IntFrom(7u, a);
    Int m1  = IntFrom(7u, a);
    Int m2  = IntFrom(7u, a);

    bool ok = IntDivMod(&q, &r, &dvd, 100u);
    ok      = ok && IntMod(&m1, &dvd, 100u);
    ok      = ok && IntMod(&m2, &dvd, (i64)100);
    ok      = ok && IntToU64(&q) == 10u && IntToU64(&r) == 3u;
    ok      = ok && IntToU64(&m1) == 3u && IntToU64(&m2) == 3u;

    IntDeinit(&dvd);
    IntDeinit(&q);
    IntDeinit(&r);
    IntDeinit(&m1);
    IntDeinit(&m2);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_mod_int_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    Int dvd = IntFrom(1003u, a);
    Int dvs = IntFrom(100u, a);
    Int r   = IntFrom(7u, a);

    bool ok = IntMod(&r, &dvd, &dvs); // int_mod frees quotient on success (1598)
    ok      = ok && IntToU64(&r) == 3u;

    IntDeinit(&dvd);
    IntDeinit(&dvs);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_pow_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    // exponent 13 = 0b1101 hits both the multiply-acc branch (1317) and the
    // square-current branch (1331), and final current/acc cleanup (1336).
    Int base = IntFrom(3u, a);
    Int r    = IntFrom(7u, a);

    bool ok = IntPow(&r, &base, 13u);
    ok      = ok && IntToU64(&r) == 1594323u; // 3^13

    IntDeinit(&base);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_gcd_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    // Euclid loop frees x each iteration (1657), final y cleanup (1663).
    Int x = IntFrom(123456u, a);
    Int y = IntFrom(7890u, a);
    Int r = IntFrom(7u, a);

    bool ok = IntGCD(&r, &x, &y);
    ok      = ok && IntToU64(&r) == 6u;

    IntDeinit(&x);
    IntDeinit(&y);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_lcm_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    // Success path frees gcd + quotient (1689/1690).
    Int x = IntFrom(21u, a);
    Int y = IntFrom(6u, a);
    Int r = IntFrom(7u, a);

    bool ok = IntLCM(&r, &x, &y);
    ok      = ok && IntToU64(&r) == 42u;

    IntDeinit(&x);
    IntDeinit(&y);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_root_rem_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    // Non-trivial cube root drives the bisection loop: low/high/mid/mid_pow
    // deinits (1813,1819,1832,1837,1838), the final block (1855..1858).
    Int v    = IntFrom(1000000u, a);
    Int root = IntFrom(7u, a);
    Int rem  = IntFrom(9u, a);

    bool ok = IntRootRem(&root, &rem, &v, 3); // 100^3 = 1e6
    ok      = ok && IntToU64(&root) == 100u && IntIsZero(&rem);

    IntDeinit(&v);
    IntDeinit(&root);
    IntDeinit(&rem);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_root_rem_inexact_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    // Inexact root exercises the cmp>0 branch (1819/1822/1832 high update).
    Int v    = IntFrom(1000u, a);
    Int root = IntFrom(7u, a);
    Int rem  = IntFrom(9u, a);

    bool ok = IntRootRem(&root, &rem, &v, 3); // cbrt(1000)=10 exact actually
    ok      = ok && IntToU64(&root) == 10u && IntIsZero(&rem);

    // and a genuinely inexact one
    Int v2 = IntFrom(999u, a);
    ok     = ok && IntRootRem(&root, &rem, &v2, 3);
    ok     = ok && IntToU64(&root) == 9u && IntToU64(&rem) == 999u - 729u;

    IntDeinit(&v);
    IntDeinit(&v2);
    IntDeinit(&root);
    IntDeinit(&rem);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_root_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    Int v = IntFrom(1000000u, a);
    Int r = IntFrom(7u, a);

    bool ok = IntRoot(&r, &v, 3); // frees remainder on success (1877)
    ok      = ok && IntToU64(&r) == 100u;

    IntDeinit(&v);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_is_perfect_square_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    // Frees root + remainder on success (1904/1905).
    Int sq  = IntFrom(123201u, a); // 351^2
    Int nsq = IntFrom(123202u, a);

    bool ok = IntIsPerfectSquare(&sq) && !IntIsPerfectSquare(&nsq);

    IntDeinit(&sq);
    IntDeinit(&nsq);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_is_perfect_power_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    // Loop frees root + remainder each degree (1934/1935).
    Int pw  = IntFrom(7776u, a); // 6^5
    Int npw = IntFrom(7777u, a);

    bool ok = IntIsPerfectPower(&pw) && !IntIsPerfectPower(&npw);

    IntDeinit(&pw);
    IntDeinit(&npw);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_jacobi_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    // Drives the main loop (swap, even-stripping 1972, inner deinits) and both
    // tail branches: result-1 path (1998) and nn!=1 -> 0 path requires gcd!=1.
    Int x  = IntFrom(1001u, a);
    Int n  = IntFrom(9907u, a); // prime
    int jr = 0;

    bool ok = IntTryJacobi(&jr, &x, &n);

    // a case where gcd(a,n) != 1 -> the *out=0 branch (frees nn at 1998)
    Int x2  = IntFrom(15u, a);
    Int n2  = IntFrom(9u, a); // odd, gcd(15,9)=3
    int jr2 = 0;
    ok      = ok && IntTryJacobi(&jr2, &x2, &n2) && jr2 == 0;

    IntDeinit(&x);
    IntDeinit(&n);
    IntDeinit(&x2);
    IntDeinit(&n2);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_mod_add_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    Int x = IntFrom(12345u, a);
    Int y = IntFrom(67890u, a);
    Int m = IntFrom(1009u, a);
    Int r = IntFrom(7u, a);

    bool ok = IntModAdd(&r, &x, &y, &m); // frees ar/br/sum on success (2042-2044)
    ok      = ok && IntToU64(&r) == (12345u % 1009u + 67890u % 1009u) % 1009u;

    IntDeinit(&x);
    IntDeinit(&y);
    IntDeinit(&m);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_mod_sub_ge_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    // ar >= br branch (2098/2099 frees ar/br).
    Int x = IntFrom(900u, a);
    Int y = IntFrom(100u, a);
    Int m = IntFrom(1009u, a);
    Int r = IntFrom(7u, a);

    bool ok = IntModSub(&r, &x, &y, &m);
    ok      = ok && IntToU64(&r) == 800u;

    IntDeinit(&x);
    IntDeinit(&y);
    IntDeinit(&m);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_mod_sub_lt_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    // ar < br branch: nonzero diff -> modulus-diff (2095 frees diff).
    Int x = IntFrom(100u, a);
    Int y = IntFrom(900u, a);
    Int m = IntFrom(1009u, a);
    Int r = IntFrom(7u, a);

    bool ok = IntModSub(&r, &x, &y, &m);
    ok      = ok && IntToU64(&r) == 1009u - 800u;

    IntDeinit(&x);
    IntDeinit(&y);
    IntDeinit(&m);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_mod_sub_zero_diff_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    // ar < br but diff is a multiple of modulus -> diff==0 inner branch
    // (still frees diff at 2095). x=0,y=m gives ar=0,br=0 actually; use
    // x=0, y=1009*2 reduced -> br=0; to make br>ar with diff==0 mod m we need
    // ar<br after reduction with (br-ar)%m==0 -> impossible unless equal.
    // Instead exercise the ar<br with diff==0 by x=0 mod m, y=0 mod m won't.
    // Use the generic ar<br path already covered; here cover equal-after-mod.
    Int x = IntFrom(5u, a);
    Int y = IntFrom(5u, a);
    Int m = IntFrom(1009u, a);
    Int r = IntFrom(7u, a);

    bool ok = IntModSub(&r, &x, &y, &m);
    ok      = ok && IntIsZero(&r);

    IntDeinit(&x);
    IntDeinit(&y);
    IntDeinit(&m);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_mod_mul_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    Int x = IntFrom(12345u, a);
    Int y = IntFrom(67890u, a);
    Int m = IntFrom(1009u, a);
    Int r = IntFrom(7u, a);

    bool ok = IntModMul(&r, &x, &y, &m); // frees ar/br/prod (2126-2128)
    ok      = ok && IntToU64(&r) == (12345ull * 67890ull) % 1009ull;

    IntDeinit(&x);
    IntDeinit(&y);
    IntDeinit(&m);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_square_mod_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    Int x = IntFrom(12345u, a);
    Int m = IntFrom(1009u, a);
    Int r = IntFrom(7u, a);

    bool ok = IntSquareMod(&r, &x, &m);
    ok      = ok && IntToU64(&r) == (12345ull * 12345ull) % 1009ull;

    IntDeinit(&x);
    IntDeinit(&m);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_mod_div_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    // IntModDiv frees inverse on success (2160).
    Int x = IntFrom(42u, a);
    Int y = IntFrom(5u, a);
    Int m = IntFrom(1009u, a);
    Int r = IntFrom(7u, a);

    bool ok = IntModDiv(&r, &x, &y, &m);

    IntDeinit(&x);
    IntDeinit(&y);
    IntDeinit(&m);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_pow_u64_mod_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    // int_pow_u64_mod: multiply-acc (2201) and square (2215), final cleanup
    // (2220 frees base_mod).
    Int base = IntFrom(7u, a);
    Int m    = IntFrom(1000000007u, a);
    Int r    = IntFrom(9u, a);

    bool ok = IntPowMod(&r, &base, 13u, &m);
    ok      = ok && IntToU64(&r) == 96889010407ull % 1000000007ull; // 7^13 mod m

    IntDeinit(&base);
    IntDeinit(&m);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_pow_mod_integer_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    // int_pow_mod (Int exponent) main loop + final base_mod/exp cleanup.
    Int base = IntFrom(7u, a);
    Int exp  = IntFrom(13u, a);
    Int m    = IntFrom(1000000007u, a);
    Int r    = IntFrom(9u, a);

    bool ok = IntPowMod(&r, &base, &exp, &m);
    ok      = ok && IntToU64(&r) == 96889010407ull % 1000000007ull;

    IntDeinit(&base);
    IntDeinit(&exp);
    IntDeinit(&m);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_mod_inv_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    Int v = IntFrom(17u, a);
    Int m = IntFrom(3120u, a); // gcd(17,3120)=1
    Int r = IntFrom(7u, a);

    bool ok = IntModInv(&r, &v, &m);
    ok      = ok && (IntToU64(&r) * 17u) % 3120u == 1u;

    IntDeinit(&v);
    IntDeinit(&m);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_mod_inv_negative_t_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    // A case where the Bezout coefficient t is negative, driving the
    // t.negative && !zero branch (modulus - mag_mod, frees positive/mag_mod
    // and mag_mod at 2414).
    Int v = IntFrom(3u, a);
    Int m = IntFrom(7u, a); // inverse of 3 mod 7 is 5
    Int r = IntFrom(9u, a);

    bool ok = IntModInv(&r, &v, &m);
    ok      = ok && IntToU64(&r) == 5u;

    IntDeinit(&v);
    IntDeinit(&m);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_mod_inv_no_solution_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    // gcd != 1 -> r != one, the if(IntEQ(&r,&one)) block is skipped and ok stays
    // false; the function still frees reduced/r/new_r/one/t/new_t at the tail
    // (2419-2424). All those carry allocations.
    Int v = IntFrom(6u, a);
    Int m = IntFrom(9u, a);           // gcd(6,9)=3, no inverse
    Int r = IntFrom(7u, a);

    bool ok = !IntModInv(&r, &v, &m); // returns false

    IntDeinit(&v);
    IntDeinit(&m);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_mod_sqrt_p3mod4_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    // modulus 7 (7 % 4 == 3) drives the fast path: frees exponent + a (2488/2489).
    Int v = IntFrom(2u, a); // 2 is QR mod 7 (3^2=2)
    Int m = IntFrom(7u, a);
    Int r = IntFrom(9u, a);

    bool ok = IntModSqrt(&r, &v, &m);
    u64  rv = IntToU64(&r);
    ok      = ok && (rv * rv) % 7u == 2u;

    IntDeinit(&v);
    IntDeinit(&m);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_mod_sqrt_tonelli_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    // modulus 17 (17 % 4 == 1) drives the Tonelli-Shanks block: q/z/c/t/r,
    // the witness-search loop, the t-loop with t_power/b/b_sq, and the success
    // int_replace path (2724-2735). 2 is a QR mod 17 (6^2=36=2).
    Int v = IntFrom(2u, a);
    Int m = IntFrom(17u, a);
    Int r = IntFrom(9u, a);

    bool ok = IntModSqrt(&r, &v, &m);
    u64  rv = IntToU64(&r);
    ok      = ok && (rv * rv) % 17u == 2u;

    IntDeinit(&v);
    IntDeinit(&m);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_mod_sqrt_tonelli_larger_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    // modulus 41 (41 % 4 == 1, 41-1 = 40 = 8*5 so m=3 levels) drives the inner
    // b-squaring loop (2662) and the c/t/b reassignments more deeply.
    Int v = IntFrom(10u, a); // 10 is QR mod 41 (16^2=256=256-246=10)
    Int m = IntFrom(41u, a);
    Int r = IntFrom(9u, a);

    bool ok = IntModSqrt(&r, &v, &m);
    u64  rv = IntToU64(&r);
    ok      = ok && (rv * rv) % 41u == 10u;

    IntDeinit(&v);
    IntDeinit(&m);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_mod_sqrt_zero_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    // value % modulus == 0 -> zero branch frees a (2449).
    Int v = IntFrom(0u, a);
    Int m = IntFrom(17u, a);
    Int r = IntFrom(9u, a);

    bool ok = IntModSqrt(&r, &v, &m) && IntIsZero(&r);

    IntDeinit(&v);
    IntDeinit(&m);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_mod_sqrt_mod2_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    // modulus 2 path: int_replace(result, &a) (no extra deinit, but reaches
    // the compare-u64==2 branch); a is moved into result.
    Int v = IntFrom(1u, a);
    Int m = IntFrom(2u, a);
    Int r = IntFrom(9u, a);

    bool ok = IntModSqrt(&r, &v, &m) && IntToU64(&r) == 1u;

    IntDeinit(&v);
    IntDeinit(&m);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_mod_sqrt_no_solution_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    // jacobi != 1 -> non-residue, frees a and returns false (2473's deinit at
    // the early non-residue return). 3 is a non-residue mod 7.
    Int v = IntFrom(3u, a);
    Int m = IntFrom(7u, a);
    Int r = IntFrom(9u, a);

    bool ok = !IntModSqrt(&r, &v, &m);

    IntDeinit(&v);
    IntDeinit(&m);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_mod_sqrt_composite_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    // composite (non-prime) odd modulus -> !prime branch frees a (2465).
    Int v = IntFrom(2u, a);
    Int m = IntFrom(9u, a); // odd composite
    Int r = IntFrom(9u, a);

    bool ok = !IntModSqrt(&r, &v, &m);

    IntDeinit(&v);
    IntDeinit(&m);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_mod_sqrt_even_modulus_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    // even modulus (>2) -> IntIsEven branch frees a (2465).
    Int v = IntFrom(3u, a);
    Int m = IntFrom(8u, a);
    Int r = IntFrom(9u, a);

    bool ok = !IntModSqrt(&r, &v, &m);

    IntDeinit(&v);
    IntDeinit(&m);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_is_probable_prime_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    // Large-ish prime drives the full Miller-Rabin loop: base/x deinits in the
    // base>=value continue (2807/2808), the x==1||n-1 continue (2820/2821), the
    // witness inner loop (2839), per-iteration base/x (2853/2854), and the d /
    // n_minus_one tail (2860/2861).
    Int prime     = IntFrom(1000003u, a); // prime
    Int composite = IntFrom(1000005u, a); // composite

    bool ok = IntIsProbablePrime(&prime) && !IntIsProbablePrime(&composite);

    IntDeinit(&prime);
    IntDeinit(&composite);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_is_probable_prime_witness_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    // A strong-pseudoprime-ish composite that forces the witness inner loop
    // (squaring x repeatedly) before failing. 2047 = 23*89 passes base 2 (it's
    // a 2-SPRP) but fails on another base -> exercises x squaring (2839).
    Int n = IntFrom(2047u, a);

    bool ok = !IntIsProbablePrime(&n);

    IntDeinit(&n);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_next_prime_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    // Drives candidate-stepping loop (frees candidate on success at 2924, and
    // the +2 stepping at 2918). Starting from an even-ish composite.
    Int v = IntFrom(1000000u, a);
    Int r = IntFrom(7u, a);

    bool ok = IntNextPrime(&r, &v);
    ok      = ok && IntToU64(&r) == 1000003u;

    IntDeinit(&v);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_next_prime_small_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    // value <= 1 -> two path (2879 int_replace) and the candidate<=2 path
    // (2902/2903 frees candidate then replaces with two).
    Int v0 = IntFrom(0u, a);
    Int r0 = IntFrom(7u, a);
    Int v1 = IntFrom(2u, a);
    Int r1 = IntFrom(7u, a);

    bool ok = IntNextPrime(&r0, &v0) && IntToU64(&r0) == 2u;
    ok      = ok && IntNextPrime(&r1, &v1) && IntToU64(&r1) == 3u;

    IntDeinit(&v0);
    IntDeinit(&r0);
    IntDeinit(&v1);
    IntDeinit(&r1);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_mul_nonzero_nonempty_result_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    Int x = IntFrom(123456u, a);
    Int y = IntFrom(7891011u, a);
    Int r = IntFrom(0xDEADBEEFu, a); // pre-populated: holds a live buffer

    bool ok = IntMul(&r, &x, &y);
    ok      = ok && IntToU64(&r) == 123456ull * 7891011ull;

    IntDeinit(&x);
    IntDeinit(&y);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_div_exact_i64_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    Int dvd = IntFrom(1000u, a);
    Int r   = IntFrom(7u, a);

    bool ok = IntDivExact(&r, &dvd, (i64)8);
    ok      = ok && IntToU64(&r) == 125u;

    IntDeinit(&dvd);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_mod_sqrt_tonelli_inner_loop_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    // moduli with p % 4 == 1 (forces the Tonelli-Shanks block) and a high power
    // of two dividing p-1: 17 (16=2^4), 97 (96=2^5*3), 193 (192=2^6*3),
    // 257 (256=2^8). Sample a spread of residues across [1, p-1] per modulus --
    // enough to drive the inner squaring loop and every cleanup branch (the leak
    // oracle), not an exhaustive correctness sweep.
    u64       moduli[] = {17u, 97u, 193u, 257u};
    const u64 samples  = 12u;
    bool      ok       = true;

    for (u64 mi = 0; mi < sizeof(moduli) / sizeof(moduli[0]) && ok; mi++) {
        u64 p = moduli[mi];
        for (u64 si = 0; si < samples && ok; si++) {
            u64 vv = 1u + (si * (p - 2u)) / (samples - 1u);
            Int v  = IntFrom(vv, a);
            Int m  = IntFrom(p, a);
            Int r  = IntFrom(9u, a);

            bool found = IntModSqrt(&r, &v, &m);
            if (found) {
                u64 rv = IntToU64(&r);
                ok     = ok && (rv * rv) % p == vv;
            }

            IntDeinit(&v);
            IntDeinit(&m);
            IntDeinit(&r);
        }
    }

    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_blind_mod_inv_correct(void) {
    DebugAllocator dbg   = DebugAllocatorInitWith(blind_cfg());
    Allocator     *alloc = ALLOCATOR_OF(&dbg);

    Int value   = IntFrom(7u, alloc);
    Int modulus = IntFrom(40u, alloc); // 7 * 23 = 161 = 1 (mod 40)
    Int inverse = IntFrom(0u, alloc);

    bool ok = IntModInv(&inverse, &value, &modulus);
    ok      = ok && IntCompare(&inverse, 23u) == 0;

    IntDeinit(&value);
    IntDeinit(&modulus);
    IntDeinit(&inverse);

    ok = ok && DebugAllocatorLiveCount(&dbg) == 0;
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_blind_mod_sqrt_tonelli(void) {
    DebugAllocator dbg   = DebugAllocatorInitWith(blind_cfg());
    Allocator     *alloc = ALLOCATOR_OF(&dbg);

    Int value   = IntFrom(9u, alloc);
    Int modulus = IntFrom(41u, alloc); // 41-1 = 8*5 -> several Tonelli rounds
    Int root    = IntFrom(0u, alloc);
    Int check   = IntFrom(0u, alloc);

    bool ok = IntModSqrt(&root, &value, &modulus);
    ok      = ok && IntSquareMod(&check, &root, &modulus);
    ok      = ok && IntCompare(&check, 9u) == 0;

    IntDeinit(&value);
    IntDeinit(&modulus);
    IntDeinit(&root);
    IntDeinit(&check);

    ok = ok && DebugAllocatorLiveCount(&dbg) == 0;
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_blind_mod_sqrt_fast_branch(void) {
    DebugAllocator dbg   = DebugAllocatorInitWith(blind_cfg());
    Allocator     *alloc = ALLOCATOR_OF(&dbg);

    Int value   = IntFrom(2u, alloc);
    Int modulus = IntFrom(7u, alloc); // 7 % 4 == 3
    Int root    = IntFrom(0u, alloc);
    Int check   = IntFrom(0u, alloc);

    bool ok = IntModSqrt(&root, &value, &modulus);
    ok      = ok && IntSquareMod(&check, &root, &modulus);
    ok      = ok && IntCompare(&check, 2u) == 0;

    IntDeinit(&value);
    IntDeinit(&modulus);
    IntDeinit(&root);
    IntDeinit(&check);

    ok = ok && DebugAllocatorLiveCount(&dbg) == 0;
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_blind_mod_sub_branches(void) {
    DebugAllocator dbg   = DebugAllocatorInitWith(blind_cfg());
    Allocator     *alloc = ALLOCATOR_OF(&dbg);

    Int m = IntFrom(7u, alloc);
    Int r = IntFrom(0u, alloc);

    // ar >= br branch: 5 - 2 = 3 (mod 7)
    Int  a1 = IntFrom(5u, alloc), b1 = IntFrom(2u, alloc);
    bool ok = IntModSub(&r, &a1, &b1, &m) && IntCompare(&r, 3u) == 0;
    // ar < br branch: 2 - 5 = -3 = 4 (mod 7)
    Int a2 = IntFrom(2u, alloc), b2 = IntFrom(5u, alloc);
    ok = ok && IntModSub(&r, &a2, &b2, &m) && IntCompare(&r, 4u) == 0;
    // equal branch: 5 - 12 = 0 (mod 7)
    Int a3 = IntFrom(5u, alloc), b3 = IntFrom(12u, alloc);
    ok = ok && IntModSub(&r, &a3, &b3, &m) && IntIsZero(&r);

    IntDeinit(&a1);
    IntDeinit(&b1);
    IntDeinit(&a2);
    IntDeinit(&b2);
    IntDeinit(&a3);
    IntDeinit(&b3);
    IntDeinit(&m);
    IntDeinit(&r);

    ok = ok && DebugAllocatorLiveCount(&dbg) == 0;
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_blind_next_prime(void) {
    DebugAllocator dbg   = DebugAllocatorInitWith(blind_cfg());
    Allocator     *alloc = ALLOCATOR_OF(&dbg);

    Int value = IntFrom(100u, alloc);
    Int next  = IntFrom(0u, alloc);

    bool ok = IntNextPrime(&next, &value);
    ok      = ok && IntCompare(&next, 101u) == 0;

    IntDeinit(&value);
    IntDeinit(&next);

    ok = ok && DebugAllocatorLiveCount(&dbg) == 0;
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_blind_carmichael_is_composite(void) {
    DebugAllocator dbg   = DebugAllocatorInitWith(blind_cfg());
    Allocator     *alloc = ALLOCATOR_OF(&dbg);

    Int  value = IntFrom(561u, alloc);
    bool error = false;
    bool prime = IntIsProbablePrime(&value, &error);
    bool ok    = (error == false) && (prime == false);

    IntDeinit(&value);

    ok = ok && DebugAllocatorLiveCount(&dbg) == 0;
    DebugAllocatorDeinit(&dbg);
    return ok;
}

int main(void) {
    WriteFmt("[INFO] Starting Int.Math tests\n\n");

    TestFunction tests[] = {
        test_int_shift_left_grows,
        test_int_shift_right_shrinks,
        test_int_add,
        test_int_add_generic,
        test_int_sub,
        test_int_sub_generic,
        test_int_sub_underflow_preserves_result,
        test_int_mul,
        test_int_mul_scalar,
        test_int_mul_zero,
        test_int_square,
        test_int_pow_generic,
        test_int_div_mod,
        test_int_div,
        test_int_div_exact,
        test_int_div_exact_failure_preserves_result,
        test_int_div_mod_scalar,
        test_int_mod,
        test_int_mod_scalar,
        test_int_gcd,
        test_int_lcm,
        test_int_root,
        test_int_root_rem,
        test_int_sqrt,
        test_int_sqrt_rem,
        test_int_is_perfect_square,
        test_int_is_perfect_power,
        test_int_jacobi,
        test_int_square_mod,
        test_int_mod_add,
        test_int_mod_sub,
        test_int_mod_mul,
        test_int_mod_div,
        test_int_pow_mod_scalar,
        test_int_pow_mod_integer_exponent,
        test_int_mod_inv,
        test_int_mod_sqrt,
        test_int_mod_sqrt_no_solution,
        test_int_is_probable_prime,
        test_int_next_prime,
        test_int_mod_inv_no_solution,
        test_int_mod_div_no_solution,
        test_int_div_by_zero,
        test_int_root_zero_degree,
        test_int_div_scalar_zero_divisor,
        test_int_mod_scalar_zero_modulus,
        test_int_mod_div_zero_modulus,
        test_int_jacobi_even_denominator,
        test_int_pow_mod_integer_zero_modulus,
        test_m1_modsqrt_p3mod4_residue,
        test_m1_modsqrt_p3mod4_nonresidue_preserves_result,
        test_m1_modsqrt_p1mod4_tonelli_deep,
        test_m1_modsqrt_p1mod4_tonelli_jloop,
        test_m1_modsqrt_p1mod4_tonelli_multi_outer,
        test_m1_modsqrt_p1mod4_tonelli_deepest,
        test_m1_modsqrt_p1mod4_tonelli_shallow,
        test_m1_modsqrt_p1mod4_nonresidue_preserves_result,
        test_m1_modsqrt_zero_value_sets_result_zero,
        test_m1_modsqrt_multiple_of_modulus_sets_zero,
        test_m1_modsqrt_modulus_two_sets_result,
        test_m1_modsqrt_even_modulus_fails,
        test_m1_modsqrt_composite_modulus_fails,
        test_m1_modsqrt_zero_modulus_fails_preserves_result,
        test_m1_modsqrt_value_reduced_before_root,
        test_m10_mul_large_product,
        test_m10_mul_zero_result,
        test_m11_pow_u64_contract,
        test_m15_sub_borrow_propagates,
        test_m16_lcm_zero_operand_replaces_result,
        test_m18_perfect_power_max_degree_only,
        test_m19_add_carry_chain,
        test_m2_root_rem_zero_value_resets_outputs,
        test_m2_root_rem_perfect_square_large,
        test_m2_root_rem_perfect_cube,
        test_m2_root_rem_nonperfect_cube_remainder,
        test_m2_root_rem_just_below_perfect_cube,
        test_m2_root_rem_fourth_root_large,
        test_m22_pow_int_exponent_value,
        test_m22_pow_int_exponent_too_large,
        test_m22_div_i64_positive_divisor,
        test_m22_div_i64_negative_divisor_fails,
        test_m22_div_exact_i64_positive_divisor,
        test_m22_div_exact_i64_negative_divisor_fails,
        test_m24_from_u64_roundtrip,
        test_m24_from_u64_value_one,
        test_m25_mul_i64_positive,
        test_m25_mul_i64_zero,
        test_m25_pow_i64_positive,
        test_m25_pow_i64_zero,
        test_m25_div_u64_value,
        test_m26_div_exact_u64_returns_true_and_quotient,
        test_m26_div_mod_u64_returns_and_computes,
        test_m26_div_mod_i64_returns_and_computes,
        test_m26_mod_i64_into_value_and_return,
        test_m26_pow_i64_mod_positive_exponent,
        test_m26_pow_i64_mod_zero_exponent,
        test_m26_root_value,
        test_m27_is_odd_zero_no_abort,
        test_m28_mod_u64_into_computes_remainder,
        test_m29_jacobi_error_flag_success_and_failure,
        test_m3_mod_inv_success_sets_ok,
        test_m3_mod_inv_negative_t_branch,
        test_fe_1487_div_u64_zero_returns_false,
        test_fe_1500_div_i64_zero_returns_false,
        test_fe_1513_div_exact_u64_inexact_false,
        test_fe_1526_div_exact_i64_inexact_false,
        test_fe_1539_div_mod_u64_zero_returns_false,
        test_fe_1552_div_mod_i64_zero_returns_false,
        test_fe_1606_mod_u64_zero_returns_false,
        test_fe_1614_mod_i64_zero_returns_false,
        test_fe_1730_root_rem_large_value,
        test_fe_1986_jacobi_reciprocity_sign,
        test_fe_2596_mod_sqrt_tonelli_inner,
        test_m4_one_not_prime,
        test_m4_two_is_prime,
        test_m4_four_not_prime,
        test_m4_three_is_prime,
        test_m4_prime_97,
        test_m4_composite_561,
        test_m4_composite_1763,
        test_m4_spsp_1373653_composite,
        test_m4_large_prime,
        test_m4_error_flag_cleared,
        test_m5_div_mod_large_correct,
        test_m5_div_mod_known_quot_rem,
        test_m5_div_mod_exact_zero_remainder,
        test_m5_div_mod_dividend_smaller,
        test_m5_div_mod_by_zero_returns_false,
        test_m6_pow_mod_acc_reduction_mod_one,
        test_m6_pow_mod_mod_one_nonzero_exp,
        test_m6_pow_mod_known_vector,
        test_m6_pow_mod_no_internal_leak,
        test_m7_next_prime_of_zero_is_two,
        test_m7_next_prime_of_ten_is_eleven,
        test_m7_next_prime_of_nine_is_eleven,
        test_m8_modsub_ge_branch_subtracts,
        test_m9_pow_u64_mod_known_value,
        test_m9_pow_u64_mod_exponent_zero,
        test_m9_pow_u64_mod_large_modulus,
        test_mul_nonzero_no_leak,
        test_mul_zero_no_leak,
        test_add_no_leak,
        test_sub_no_leak,
        test_add_u64_in_place_no_leak,
        test_mul_u64_in_place_no_leak,
        test_sub_u64_no_leak,
        test_div_mod_ge_no_leak,
        test_div_mod_lt_no_leak,
        test_div_no_leak,
        test_div_exact_no_leak,
        test_div_scalar_no_leak,
        test_div_mod_scalar_no_leak,
        test_mod_int_no_leak,
        test_pow_no_leak,
        test_gcd_no_leak,
        test_lcm_no_leak,
        test_root_rem_no_leak,
        test_root_rem_inexact_no_leak,
        test_root_no_leak,
        test_is_perfect_square_no_leak,
        test_is_perfect_power_no_leak,
        test_jacobi_no_leak,
        test_mod_add_no_leak,
        test_mod_sub_ge_no_leak,
        test_mod_sub_lt_no_leak,
        test_mod_sub_zero_diff_no_leak,
        test_mod_mul_no_leak,
        test_square_mod_no_leak,
        test_mod_div_no_leak,
        test_pow_u64_mod_no_leak,
        test_pow_mod_integer_no_leak,
        test_mod_inv_no_leak,
        test_mod_inv_negative_t_no_leak,
        test_mod_inv_no_solution_no_leak,
        test_mod_sqrt_p3mod4_no_leak,
        test_mod_sqrt_tonelli_no_leak,
        test_mod_sqrt_tonelli_larger_no_leak,
        test_mod_sqrt_zero_no_leak,
        test_mod_sqrt_mod2_no_leak,
        test_mod_sqrt_no_solution_no_leak,
        test_mod_sqrt_composite_no_leak,
        test_mod_sqrt_even_modulus_no_leak,
        test_is_probable_prime_no_leak,
        test_is_probable_prime_witness_no_leak,
        test_next_prime_no_leak,
        test_next_prime_small_no_leak,
        test_mul_nonzero_nonempty_result_no_leak,
        test_div_exact_i64_no_leak,
        test_mod_sqrt_tonelli_inner_loop_no_leak,
        test_blind_mod_inv_correct,
        test_blind_mod_sqrt_tonelli,
        test_blind_mod_sqrt_fast_branch,
        test_blind_mod_sub_branches,
        test_blind_next_prime,
        test_blind_carmichael_is_composite,
    };

    TestFunction deadend_tests[] = {
        test_int_add_null_result,
        test_int_shift_left_null,
        test_int_pow_mod_scalar_zero_modulus,
        test_m10_mul_null_result_deadend,
        test_m13_gcd_null_result,
        test_m13_gcd_null_a,
        test_m13_gcd_null_b,
        test_m13_modadd_null_result,
        test_m14_mod_mul_null_result_deadend,
        test_m15_sub_null_result,
        test_m15_div_u64_rem_null_quotient,
        test_m15_div_u64_rem_null_dividend,
        test_m18_div_exact_null_result,
        test_m28_trailing_zero_invalid_deadend,
        test_m5_div_mod_null_quotient_aborts,
        test_m5_div_mod_null_remainder_aborts,
        test_m7_next_prime_null_result,
        test_m8_modsub_null_result,
        test_m9_pow_u64_mod_null_result,
        test_m9_pow_u64_mod_null_base,
        test_m9_pow_u64_mod_null_modulus,
    };

    int total_tests         = sizeof(tests) / sizeof(tests[0]);
    int total_deadend_tests = sizeof(deadend_tests) / sizeof(deadend_tests[0]);

    return run_test_suite(tests, total_tests, deadend_tests, total_deadend_tests, "Int.Math");
}
