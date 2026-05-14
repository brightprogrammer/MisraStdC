#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Container/Int.h>
#include <Misra/Std/Log.h>
#include <Misra/Types.h>
#include <string.h>

#include "../Util/TestRunner.h"

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

    Int a      = IntFrom(255, &alloc.base);
    Int b      = IntFrom(1, &alloc.base);
    Int result_value = IntInit(&alloc.base);
    Str text   = StrInit(&alloc.base);

    IntAdd(&result_value, &a, &b);
    text = IntToBinary(&result_value);

    bool result = IntToU64(&result_value) == 256;
    result      = result && (ZstrCompare(text.data, "100000000") == 0);

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
    result = result && (ZstrCompare(text.data, "123456789012345678901234567900") == 0);

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

    Int a      = IntFrom(256, &alloc.base);
    Int b      = IntFrom(1, &alloc.base);
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
    result = result && (ZstrCompare(text.data, "12345678901234567800") == 0);

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

    Int a      = IntFrom(3, &alloc.base);
    Int b      = IntFrom(5, &alloc.base);
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

    Int a      = IntFrom(21, &alloc.base);
    Int b      = IntFrom(6, &alloc.base);
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

    Int value = IntFromStr("12345678901234567890", &alloc.base);
    Int result_value = IntInit(&alloc.base);
    Str text = StrInit(&alloc.base);

    IntMul(&result_value, &value, 25u);
    text = IntToStr(&result_value);

    bool result = ZstrCompare(text.data, "308641972530864197250") == 0;

    IntDeinit(&value);
    IntDeinit(&result_value);
    StrDeinit(&text);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_mul_zero(void) {
    WriteFmt("Testing IntMul with zero\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int a      = IntFrom(0, &alloc.base);
    Int b      = IntFrom(12345, &alloc.base);
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

    Int value = IntFrom(12345, &alloc.base);
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

    Int base = IntFrom(7, &alloc.base);
    Int exponent = IntFrom(20, &alloc.base);
    Int result_value = IntInit(&alloc.base);
    Str text = StrInit(&alloc.base);

    IntPow(&result_value, &base, 20u);
    text   = IntToStr(&result_value);
    bool result = ZstrCompare(text.data, "79792266297612001") == 0;

    StrDeinit(&text);
    IntPow(&result_value, &base, &exponent);
    text   = IntToStr(&result_value);
    result = result && (ZstrCompare(text.data, "79792266297612001") == 0);

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

    bool result = ZstrCompare(qtext.data, "127275040218913071") == 0;
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

    Int dividend = IntFrom(126, &alloc.base);
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

    Int dividend = IntFromStr("12345678901234567890", &alloc.base);
    Int result_value = IntInit(&alloc.base);
    Str text = StrInit(&alloc.base);

    bool result = IntDivExact(&result_value, &dividend, 90u);
    text = IntToStr(&result_value);
    result = result && (ZstrCompare(text.data, "137174210013717421") == 0);

    IntDeinit(&dividend);
    IntDeinit(&result_value);
    StrDeinit(&text);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_div_exact_failure_preserves_result(void) {
    WriteFmt("Testing IntDivExact failure handling\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int dividend = IntFrom(10, &alloc.base);
    Int divisor = IntFrom(3, &alloc.base);
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

    Int dividend = IntFromStr("12345678901234567890", &alloc.base);
    Int quotient = IntInit(&alloc.base);
    Int remainder = IntInit(&alloc.base);
    Str text = StrInit(&alloc.base);

    IntDivMod(&quotient, &remainder, &dividend, 97);
    text = IntToStr(&quotient);

    bool result = ZstrCompare(text.data, "127275040218913071") == 0;
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

    Int dividend = IntFrom(126, &alloc.base);
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

    Int value = IntFromStr("12345678901234567890", &alloc.base);
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

    Int a = IntFrom(48, &alloc.base);
    Int b = IntFrom(18, &alloc.base);
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

    Int a = IntFrom(21, &alloc.base);
    Int b = IntFrom(6, &alloc.base);
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

    Int value = IntFrom(4096, &alloc.base);
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

    Int value = IntFrom(200, &alloc.base);
    Int root = IntInit(&alloc.base);
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

    Int value = IntFrom(200, &alloc.base);
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

    Int value = IntFrom(200, &alloc.base);
    Int root = IntInit(&alloc.base);
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

    Int square = IntFrom(144, &alloc.base);
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

    Int power = IntFrom(81, &alloc.base);
    Int non_power = IntFrom(82, &alloc.base);
    Int one = IntFrom(1, &alloc.base);

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

    Int value = IntFrom(12345, &alloc.base);
    Int mod = IntFrom(97, &alloc.base);
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

    Int a = IntFrom(100, &alloc.base);
    Int b = IntFrom(250, &alloc.base);
    Int m = IntFrom(13, &alloc.base);
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

    Int a = IntFrom(5, &alloc.base);
    Int b = IntFrom(9, &alloc.base);
    Int m = IntFrom(13, &alloc.base);
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

    Int a = IntFrom(123, &alloc.base);
    Int b = IntFrom(456, &alloc.base);
    Int m = IntFrom(97, &alloc.base);
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

    Int a = IntFrom(10, &alloc.base);
    Int b = IntFrom(3, &alloc.base);
    Int m = IntFrom(13, &alloc.base);
    Int result_value = IntInit(&alloc.base);
    Int check = IntInit(&alloc.base);

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

    Int base = IntFrom(7, &alloc.base);
    Int mod = IntFrom(13, &alloc.base);
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

    Int base = IntFrom(4, &alloc.base);
    Int exp = IntFrom(13, &alloc.base);
    Int mod = IntFrom(497, &alloc.base);
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

    Int value = IntFrom(3, &alloc.base);
    Int mod = IntFrom(11, &alloc.base);
    Int result_value = IntInit(&alloc.base);
    Int check = IntInit(&alloc.base);

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
    Int mod = IntFrom(13, &alloc.base);
    Int root = IntInit(&alloc.base);
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
    Int mod = IntFrom(7, &alloc.base);
    Int root = IntFrom(99, &alloc.base);

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

    Int prime = IntFromStr("1000000007", &alloc.base);
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
    Int next = IntInit(&alloc.base);
    Str text = StrInit(&alloc.base);

    bool ok = IntNextPrime(&next, &value);
    text = IntToStr(&next);

    bool result = ok && ZstrCompare(text.data, "1000000007") == 0;

    IntDeinit(&value);
    IntDeinit(&next);
    StrDeinit(&text);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_mod_inv_no_solution(void) {
    WriteFmt("Testing IntModInv no-solution case\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value = IntFrom(6, &alloc.base);
    Int mod = IntFrom(15, &alloc.base);
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

    Int a = IntFrom(1, &alloc.base);
    Int b = IntFrom(6, &alloc.base);
    Int m = IntFrom(15, &alloc.base);
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

    Int value     = IntFrom(16, &alloc.base);
    Int root      = IntFrom(99, &alloc.base);
    Int remainder = IntFrom(77, &alloc.base);
    bool result   = !IntRootRem(&root, &remainder, &value, 0);

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

    Int base = IntFrom(2, &alloc.base);
    Int mod = IntInit(&alloc.base);
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
    };

    TestFunction deadend_tests[] = {
        test_int_add_null_result,
        test_int_shift_left_null,
        test_int_pow_mod_scalar_zero_modulus,
    };

    int total_tests         = sizeof(tests) / sizeof(tests[0]);
    int total_deadend_tests = sizeof(deadend_tests) / sizeof(deadend_tests[0]);

    return run_test_suite(tests, total_tests, deadend_tests, total_deadend_tests, "Int.Math");
}
