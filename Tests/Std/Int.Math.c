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

    Int value = IntFrom(3);

    IntShiftLeft(&value, 4);

    bool result = IntToU64(&value) == 48;
    result      = result && (IntBitLength(&value) == 6);

    IntDeinit(&value);
    return result;
}

bool test_int_shift_right_shrinks(void) {
    WriteFmt("Testing IntShiftRight\n");

    Int value = IntFromBinary("110000");

    IntShiftRight(&value, 4);

    bool result = IntToU64(&value) == 3;
    result      = result && (IntBitLength(&value) == 2);

    IntDeinit(&value);
    return result;
}

bool test_int_add(void) {
    WriteFmt("Testing IntAdd\n");

    Int a      = IntFrom(255);
    Int b      = IntFrom(1);
    Int result_value = IntInit();
    Str text   = StrInit();

    IntAdd(&result_value, &a, &b);
    text = IntToBinary(&result_value);

    bool result = IntToU64(&result_value) == 256;
    result      = result && (ZstrCompare(text.data, "100000000") == 0);

    StrDeinit(&text);
    IntDeinit(&a);
    IntDeinit(&b);
    IntDeinit(&result_value);
    return result;
}

bool test_int_add_generic(void) {
    WriteFmt("Testing IntAdd generic dispatch\n");

    Int base         = IntFrom(40);
    Int rhs          = IntFrom(2);
    Int result_value = IntInit();
    Int huge         = IntFromStr("123456789012345678901234567890");
    Str text         = StrInit();

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
    return result;
}

bool test_int_sub(void) {
    WriteFmt("Testing IntSub\n");

    Int a      = IntFrom(256);
    Int b      = IntFrom(1);
    Int result_value = IntInit();

    bool result = IntSub(&result_value, &a, &b);
    result      = result && (IntToU64(&result_value) == 255);

    IntDeinit(&a);
    IntDeinit(&b);
    IntDeinit(&result_value);
    return result;
}

bool test_int_sub_generic(void) {
    WriteFmt("Testing IntSub generic dispatch\n");

    Int base         = IntFrom(40);
    Int rhs          = IntFrom(2);
    Int result_value = IntInit();
    Int preserved    = IntFrom(99);
    Int huge         = IntFromStr("12345678901234567890");
    Str text         = StrInit();

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
    return result;
}

bool test_int_sub_underflow_preserves_result(void) {
    WriteFmt("Testing IntSub underflow handling\n");

    Int a      = IntFrom(3);
    Int b      = IntFrom(5);
    Int result_value = IntFrom(99);

    bool result = !IntSub(&result_value, &a, &b);
    result      = result && (IntToU64(&result_value) == 99);

    IntDeinit(&a);
    IntDeinit(&b);
    IntDeinit(&result_value);
    return result;
}

bool test_int_mul(void) {
    WriteFmt("Testing IntMul\n");

    Int a      = IntFrom(21);
    Int b      = IntFrom(6);
    Int result_value = IntInit();

    IntMul(&result_value, &a, &b);

    bool result = IntToU64(&result_value) == 126;

    IntDeinit(&a);
    IntDeinit(&b);
    IntDeinit(&result_value);
    return result;
}

bool test_int_mul_scalar(void) {
    WriteFmt("Testing IntMul generic dispatch\n");

    Int value = IntFromStr("12345678901234567890");
    Int result_value = IntInit();
    Str text = StrInit();

    IntMul(&result_value, &value, 25u);
    text = IntToStr(&result_value);

    bool result = ZstrCompare(text.data, "308641972530864197250") == 0;

    IntDeinit(&value);
    IntDeinit(&result_value);
    StrDeinit(&text);
    return result;
}

bool test_int_mul_zero(void) {
    WriteFmt("Testing IntMul with zero\n");

    Int a      = IntFrom(0);
    Int b      = IntFrom(12345);
    Int result_value = IntInit();

    IntMul(&result_value, &a, &b);

    bool result = IntIsZero(&result_value);
    result      = result && (IntToU64(&result_value) == 0);

    IntDeinit(&a);
    IntDeinit(&b);
    IntDeinit(&result_value);
    return result;
}

bool test_int_square(void) {
    WriteFmt("Testing IntSquare\n");

    Int value = IntFrom(12345);
    Int result_value = IntInit();

    IntSquare(&result_value, &value);

    bool result = IntToU64(&result_value) == 152399025;

    IntDeinit(&value);
    IntDeinit(&result_value);
    return result;
}

bool test_int_pow_generic(void) {
    WriteFmt("Testing IntPow generic dispatch\n");

    Int base = IntFrom(7);
    Int exponent = IntFrom(20);
    Int result_value = IntInit();
    Str text = StrInit();

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
    return result;
}

bool test_int_div_mod(void) {
    WriteFmt("Testing IntDivMod generic dispatch\n");

    Int dividend  = IntFromStr("12345678901234567890");
    Int quotient  = IntInit();
    Int remainder = IntInit();
    Str qtext     = StrInit();

    IntDivMod(&quotient, &remainder, &dividend, 97u);
    qtext = IntToStr(&quotient);

    bool result = ZstrCompare(qtext.data, "127275040218913071") == 0;
    result      = result && (IntToU64(&remainder) == 3);

    StrDeinit(&qtext);
    IntDeinit(&dividend);
    IntDeinit(&quotient);
    IntDeinit(&remainder);
    return result;
}

bool test_int_div(void) {
    WriteFmt("Testing IntDiv generic dispatch\n");

    Int dividend = IntFrom(126);
    Int result_value = IntInit();

    IntDiv(&result_value, &dividend, 10u);

    bool result = IntToU64(&result_value) == 12;

    IntDeinit(&dividend);
    IntDeinit(&result_value);
    return result;
}

bool test_int_div_exact(void) {
    WriteFmt("Testing IntDivExact generic dispatch\n");

    Int dividend = IntFromStr("12345678901234567890");
    Int result_value = IntInit();
    Str text = StrInit();

    bool result = IntDivExact(&result_value, &dividend, 90u);
    text = IntToStr(&result_value);
    result = result && (ZstrCompare(text.data, "137174210013717421") == 0);

    IntDeinit(&dividend);
    IntDeinit(&result_value);
    StrDeinit(&text);
    return result;
}

bool test_int_div_exact_failure_preserves_result(void) {
    WriteFmt("Testing IntDivExact failure handling\n");

    Int dividend = IntFrom(10);
    Int divisor = IntFrom(3);
    Int result_value = IntFrom(99);

    bool result = !IntDivExact(&result_value, &dividend, &divisor);
    result      = result && (IntToU64(&result_value) == 99);

    IntDeinit(&dividend);
    IntDeinit(&divisor);
    IntDeinit(&result_value);
    return result;
}

bool test_int_div_mod_scalar(void) {
    WriteFmt("Testing IntDivMod scalar-divisor dispatch\n");

    Int dividend = IntFromStr("12345678901234567890");
    Int quotient = IntInit();
    Int remainder = IntInit();
    Str text = StrInit();

    IntDivMod(&quotient, &remainder, &dividend, 97);
    text = IntToStr(&quotient);

    bool result = ZstrCompare(text.data, "127275040218913071") == 0;
    result      = result && (IntToU64(&remainder) == 3);

    IntDeinit(&dividend);
    IntDeinit(&quotient);
    IntDeinit(&remainder);
    StrDeinit(&text);
    return result;
}

bool test_int_mod(void) {
    WriteFmt("Testing IntMod generic dispatch\n");

    Int dividend = IntFrom(126);
    Int result_value = IntInit();

    IntMod(&result_value, &dividend, 10u);

    bool result = IntToU64(&result_value) == 6;

    IntDeinit(&dividend);
    IntDeinit(&result_value);
    return result;
}

bool test_int_mod_scalar(void) {
    WriteFmt("Testing IntMod scalar-divisor dispatch\n");

    Int value = IntFromStr("12345678901234567890");
    Int remainder = IntInit();

    IntMod(&remainder, &value, 97u);

    IntDeinit(&value);
    bool result = IntToU64(&remainder) == 3;
    IntDeinit(&remainder);
    return result;
}

bool test_int_gcd(void) {
    WriteFmt("Testing IntGCD\n");

    Int a = IntFrom(48);
    Int b = IntFrom(18);
    Int result_value = IntInit();

    IntGCD(&result_value, &a, &b);

    bool result = IntToU64(&result_value) == 6;

    IntDeinit(&a);
    IntDeinit(&b);
    IntDeinit(&result_value);
    return result;
}

bool test_int_lcm(void) {
    WriteFmt("Testing IntLCM\n");

    Int a = IntFrom(21);
    Int b = IntFrom(6);
    Int result_value = IntInit();

    IntLCM(&result_value, &a, &b);

    bool result = IntToU64(&result_value) == 42;

    IntDeinit(&a);
    IntDeinit(&b);
    IntDeinit(&result_value);
    return result;
}

bool test_int_root(void) {
    WriteFmt("Testing IntRoot\n");

    Int value = IntFrom(4096);
    Int result_value = IntInit();

    IntRoot(&result_value, &value, 4);

    bool result = IntToU64(&result_value) == 8;

    IntDeinit(&value);
    IntDeinit(&result_value);
    return result;
}

bool test_int_root_rem(void) {
    WriteFmt("Testing IntRootRem\n");

    Int value = IntFrom(200);
    Int root = IntInit();
    Int remainder = IntInit();

    IntRootRem(&root, &remainder, &value, 3);

    bool result = IntToU64(&root) == 5;
    result      = result && (IntToU64(&remainder) == 75);

    IntDeinit(&value);
    IntDeinit(&root);
    IntDeinit(&remainder);
    return result;
}

bool test_int_sqrt(void) {
    WriteFmt("Testing IntSqrt\n");

    Int value = IntFrom(200);
    Int result_value = IntInit();

    IntSqrt(&result_value, &value);

    bool result = IntToU64(&result_value) == 14;

    IntDeinit(&value);
    IntDeinit(&result_value);
    return result;
}

bool test_int_sqrt_rem(void) {
    WriteFmt("Testing IntSqrtRem\n");

    Int value = IntFrom(200);
    Int root = IntInit();
    Int remainder = IntInit();

    IntSqrtRem(&root, &remainder, &value);

    bool result = IntToU64(&root) == 14;
    result      = result && (IntToU64(&remainder) == 4);

    IntDeinit(&value);
    IntDeinit(&root);
    IntDeinit(&remainder);
    return result;
}

bool test_int_is_perfect_square(void) {
    WriteFmt("Testing IntIsPerfectSquare\n");

    Int square = IntFrom(144);
    Int non_square = IntFrom(145);

    bool result = IntIsPerfectSquare(&square);
    result      = result && !IntIsPerfectSquare(&non_square);

    IntDeinit(&square);
    IntDeinit(&non_square);
    return result;
}

bool test_int_is_perfect_power(void) {
    WriteFmt("Testing IntIsPerfectPower\n");

    Int power = IntFrom(81);
    Int non_power = IntFrom(82);
    Int one = IntFrom(1);

    bool result = IntIsPerfectPower(&power);
    result      = result && !IntIsPerfectPower(&non_power);
    result      = result && IntIsPerfectPower(&one);

    IntDeinit(&power);
    IntDeinit(&non_power);
    IntDeinit(&one);
    return result;
}

bool test_int_jacobi(void) {
    WriteFmt("Testing IntJacobi\n");

    Int a = IntFrom(5);
    Int p = IntFrom(7);
    Int b = IntFrom(9);
    Int n = IntFrom(21);

    bool result = IntJacobi(&a, &p) == -1;
    result      = result && (IntJacobi(&b, &n) == 0);

    IntDeinit(&a);
    IntDeinit(&p);
    IntDeinit(&b);
    IntDeinit(&n);
    return result;
}

bool test_int_square_mod(void) {
    WriteFmt("Testing IntSquareMod\n");

    Int value = IntFrom(12345);
    Int mod = IntFrom(97);
    Int result_value = IntInit();

    IntSquareMod(&result_value, &value, &mod);

    bool result = IntToU64(&result_value) == 94;

    IntDeinit(&value);
    IntDeinit(&mod);
    IntDeinit(&result_value);
    return result;
}

bool test_int_mod_add(void) {
    WriteFmt("Testing IntModAdd\n");

    Int a = IntFrom(100);
    Int b = IntFrom(250);
    Int m = IntFrom(13);
    Int result_value = IntInit();

    IntModAdd(&result_value, &a, &b, &m);

    bool result = IntToU64(&result_value) == 12;

    IntDeinit(&a);
    IntDeinit(&b);
    IntDeinit(&m);
    IntDeinit(&result_value);
    return result;
}

bool test_int_mod_sub(void) {
    WriteFmt("Testing IntModSub\n");

    Int a = IntFrom(5);
    Int b = IntFrom(9);
    Int m = IntFrom(13);
    Int result_value = IntInit();

    IntModSub(&result_value, &a, &b, &m);

    bool result = IntToU64(&result_value) == 9;

    IntDeinit(&a);
    IntDeinit(&b);
    IntDeinit(&m);
    IntDeinit(&result_value);
    return result;
}

bool test_int_mod_mul(void) {
    WriteFmt("Testing IntModMul\n");

    Int a = IntFrom(123);
    Int b = IntFrom(456);
    Int m = IntFrom(97);
    Int result_value = IntInit();

    IntModMul(&result_value, &a, &b, &m);

    bool result = IntToU64(&result_value) == 22;

    IntDeinit(&a);
    IntDeinit(&b);
    IntDeinit(&m);
    IntDeinit(&result_value);
    return result;
}

bool test_int_mod_div(void) {
    WriteFmt("Testing IntModDiv\n");

    Int a = IntFrom(10);
    Int b = IntFrom(3);
    Int m = IntFrom(13);
    Int result_value = IntInit();
    Int check = IntInit();

    bool result = IntModDiv(&result_value, &a, &b, &m);
    result      = result && (IntToU64(&result_value) == 12);

    IntModMul(&check, &result_value, &b, &m);
    result = result && (IntCompare(&check, 10) == 0);

    IntDeinit(&a);
    IntDeinit(&b);
    IntDeinit(&m);
    IntDeinit(&result_value);
    IntDeinit(&check);
    return result;
}

bool test_int_pow_mod_scalar(void) {
    WriteFmt("Testing IntPowMod scalar-exponent dispatch\n");

    Int base = IntFrom(7);
    Int mod = IntFrom(13);
    Int result_value = IntInit();

    IntPowMod(&result_value, &base, 20u, &mod);

    bool result = IntToU64(&result_value) == 3;

    IntDeinit(&base);
    IntDeinit(&mod);
    IntDeinit(&result_value);
    return result;
}

bool test_int_pow_mod_integer_exponent(void) {
    WriteFmt("Testing IntPowMod Int-exponent dispatch\n");

    Int base = IntFrom(4);
    Int exp = IntFrom(13);
    Int mod = IntFrom(497);
    Int result_value = IntInit();

    IntPowMod(&result_value, &base, &exp, &mod);

    bool result = IntToU64(&result_value) == 445;

    IntDeinit(&base);
    IntDeinit(&exp);
    IntDeinit(&mod);
    IntDeinit(&result_value);
    return result;
}

bool test_int_mod_inv(void) {
    WriteFmt("Testing IntModInv\n");

    Int value = IntFrom(3);
    Int mod = IntFrom(11);
    Int result_value = IntInit();
    Int check = IntInit();

    bool result = IntModInv(&result_value, &value, &mod);
    result      = result && (IntToU64(&result_value) == 4);

    IntModMul(&check, &value, &result_value, &mod);
    result = result && (IntToU64(&check) == 1);

    IntDeinit(&value);
    IntDeinit(&mod);
    IntDeinit(&result_value);
    IntDeinit(&check);
    return result;
}

bool test_int_mod_sqrt(void) {
    WriteFmt("Testing IntModSqrt\n");

    Int value = IntFrom(10);
    Int mod = IntFrom(13);
    Int root = IntInit();
    Int check = IntInit();

    bool result = IntModSqrt(&root, &value, &mod);
    IntSquareMod(&check, &root, &mod);
    result = result && (IntCompare(&check, 10) == 0);

    IntDeinit(&value);
    IntDeinit(&mod);
    IntDeinit(&root);
    IntDeinit(&check);
    return result;
}

bool test_int_mod_sqrt_no_solution(void) {
    WriteFmt("Testing IntModSqrt no-solution case\n");

    Int value = IntFrom(3);
    Int mod = IntFrom(7);
    Int root = IntFrom(99);

    bool result = !IntModSqrt(&root, &value, &mod);
    result      = result && (IntCompare(&root, 99) == 0);

    IntDeinit(&value);
    IntDeinit(&mod);
    IntDeinit(&root);
    return result;
}

bool test_int_is_probable_prime(void) {
    WriteFmt("Testing IntIsProbablePrime\n");

    Int prime = IntFromStr("1000000007");
    Int composite = IntFrom(561);

    bool result = IntIsProbablePrime(&prime);
    result      = result && !IntIsProbablePrime(&composite);

    IntDeinit(&prime);
    IntDeinit(&composite);
    return result;
}

bool test_int_next_prime(void) {
    WriteFmt("Testing IntNextPrime\n");

    Int value = IntFromStr("1000000000");
    Int next = IntInit();
    Str text = StrInit();

    bool ok = IntNextPrime(&next, &value);
    text = IntToStr(&next);

    bool result = ok && ZstrCompare(text.data, "1000000007") == 0;

    IntDeinit(&value);
    IntDeinit(&next);
    StrDeinit(&text);
    return result;
}

bool test_int_mod_inv_no_solution(void) {
    WriteFmt("Testing IntModInv no-solution case\n");

    Int value = IntFrom(6);
    Int mod = IntFrom(15);
    Int result_value = IntFrom(99);

    bool result = !IntModInv(&result_value, &value, &mod);
    result      = result && (IntToU64(&result_value) == 99);

    IntDeinit(&value);
    IntDeinit(&mod);
    IntDeinit(&result_value);
    return result;
}

bool test_int_mod_div_no_solution(void) {
    WriteFmt("Testing IntModDiv no-solution case\n");

    Int a = IntFrom(1);
    Int b = IntFrom(6);
    Int m = IntFrom(15);
    Int result_value = IntFrom(99);

    bool result = !IntModDiv(&result_value, &a, &b, &m);
    result      = result && (IntCompare(&result_value, 99) == 0);

    IntDeinit(&a);
    IntDeinit(&b);
    IntDeinit(&m);
    IntDeinit(&result_value);
    return result;
}

bool test_int_add_null_result(void) {
    WriteFmt("Testing IntAdd NULL result handling\n");

    Int a = IntFrom(1);
    Int b = IntFrom(2);

    IntAdd(NULL, &a, &b);
    return false;
}

bool test_int_shift_left_null(void) {
    WriteFmt("Testing IntShiftLeft NULL handling\n");

    IntShiftLeft(NULL, 1);
    return false;
}

bool test_int_div_by_zero(void) {
    WriteFmt("Testing Int division by zero handling\n");

    Int dividend  = IntFrom(1);
    Int divisor   = IntInit();
    Int quotient  = IntFrom(99);
    Int remainder = IntFrom(77);

    bool result = !IntDivMod(&quotient, &remainder, &dividend, &divisor);

    result = result && (IntCompare(&quotient, 99) == 0);
    result = result && (IntCompare(&remainder, 77) == 0);

    IntDeinit(&dividend);
    IntDeinit(&divisor);
    IntDeinit(&quotient);
    IntDeinit(&remainder);
    return result;
}

bool test_int_root_zero_degree(void) {
    WriteFmt("Testing IntRoot zero-degree handling\n");

    Int value     = IntFrom(16);
    Int root      = IntFrom(99);
    Int remainder = IntFrom(77);
    bool result   = !IntRootRem(&root, &remainder, &value, 0);

    result = result && (IntCompare(&root, 99) == 0);
    result = result && (IntCompare(&remainder, 77) == 0);

    IntDeinit(&value);
    IntDeinit(&root);
    IntDeinit(&remainder);
    return result;
}

bool test_int_div_scalar_zero_divisor(void) {
    WriteFmt("Testing IntDiv scalar zero-divisor handling\n");

    Int dividend = IntFrom(10);
    Int quotient = IntFrom(99);

    IntDiv(&quotient, &dividend, 0u);
    bool result = IntCompare(&quotient, 99) == 0;

    IntDeinit(&dividend);
    IntDeinit(&quotient);
    return result;
}

bool test_int_mod_scalar_zero_modulus(void) {
    WriteFmt("Testing IntMod scalar zero-modulus handling\n");

    Int value        = IntFrom(10);
    Int result_value = IntFrom(99);

    IntMod(&result_value, &value, 0u);
    bool result = IntCompare(&result_value, 99) == 0;

    IntDeinit(&value);
    IntDeinit(&result_value);
    return result;
}

bool test_int_mod_div_zero_modulus(void) {
    WriteFmt("Testing IntModDiv zero modulus handling\n");

    Int a            = IntFrom(10);
    Int b            = IntFrom(3);
    Int m            = IntInit();
    Int result_value = IntFrom(99);

    bool result = !IntModDiv(&result_value, &a, &b, &m);
    result      = result && (IntCompare(&result_value, 99) == 0);

    IntDeinit(&a);
    IntDeinit(&b);
    IntDeinit(&m);
    IntDeinit(&result_value);
    return result;
}

bool test_int_jacobi_even_denominator(void) {
    WriteFmt("Testing IntJacobi even denominator handling\n");

    Int  a      = IntFrom(3);
    Int  n      = IntFrom(8);
    int  symbol = 99;
    bool error  = false;
    bool result = !IntTryJacobi(&symbol, &a, &n);

    result = result && (IntJacobi(&a, &n, &error) == 0);
    result = result && (symbol == 99);
    result = result && error;

    IntDeinit(&a);
    IntDeinit(&n);
    return result;
}

bool test_int_pow_mod_scalar_zero_modulus(void) {
    WriteFmt("Testing IntPowMod scalar-exponent zero modulus handling\n");

    Int base = IntFrom(2);
    Int mod = IntInit();
    Int result_value = IntInit();

    IntPowMod(&result_value, &base, 8u, &mod);
    return false;
}

bool test_int_pow_mod_integer_zero_modulus(void) {
    WriteFmt("Testing IntPowMod Int-exponent zero modulus handling\n");

    Int base         = IntFrom(2);
    Int exp          = IntFrom(8);
    Int mod          = IntInit();
    Int result_value = IntFrom(99);

    bool result = !IntPowMod(&result_value, &base, &exp, &mod);
    result      = result && (IntCompare(&result_value, 99) == 0);

    IntDeinit(&base);
    IntDeinit(&exp);
    IntDeinit(&mod);
    IntDeinit(&result_value);
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
