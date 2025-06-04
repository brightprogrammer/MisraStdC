#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Log.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <Misra/Types.h>

// Include test utilities
#include "../Util/TestRunner.h"

// Function prototypes
bool test_str_from_u64(void);
bool test_str_from_i64(void);
bool test_str_from_f64(void);
bool test_str_to_u64(void);
bool test_str_to_i64(void);
bool test_str_to_f64(void);

// Test StrFromU64 function
bool test_str_from_u64(void) {
    printf("Testing StrFromU64\n");

    Str s = StrInit();

    // Test decimal conversion
    StrFromU64(&s, 12345, 10, false);
    bool result = (ZstrCompare(s.data, "12345") == 0);

    // Test hexadecimal conversion (lowercase)
    StrClear(&s);
    StrFromU64(&s, 0xABCD, 16, false);
    result = result && (ZstrCompare(s.data, "0xabcd") == 0);

    // Test hexadecimal conversion (uppercase)
    StrClear(&s);
    StrFromU64(&s, 0xABCD, 16, true);
    result = result && (ZstrCompare(s.data, "0xABCD") == 0);

    // Test binary conversion
    StrClear(&s);
    StrFromU64(&s, 42, 2, false);
    result = result && (ZstrCompare(s.data, "0b101010") == 0);

    // Test octal conversion
    StrClear(&s);
    StrFromU64(&s, 42, 8, false);
    result = result && (ZstrCompare(s.data, "0o52") == 0);

    // Test zero
    StrClear(&s);
    StrFromU64(&s, 0, 10, false);
    result = result && (ZstrCompare(s.data, "0") == 0);

    StrDeinit(&s);
    return result;
}

// Test StrFromI64 function
bool test_str_from_i64(void) {
    printf("Testing StrFromI64\n");

    Str s = StrInit();

    // Test positive decimal conversion
    StrFromI64(&s, 12345, 10, false);
    bool result = (ZstrCompare(s.data, "12345") == 0);

    // Test negative decimal conversion
    StrClear(&s);
    StrFromI64(&s, -12345, 10, false);
    result = result && (ZstrCompare(s.data, "-12345") == 0);

    // Test hexadecimal conversion of negative number (check only for prefix, not exact value)
    StrClear(&s);
    StrFromI64(&s, -0xABCD, 16, false);
    result = result && (ZstrCompareN(s.data, "0x", 2) == 0);

    // Test zero
    StrClear(&s);
    StrFromI64(&s, 0, 10, false);
    result = result && (ZstrCompare(s.data, "0") == 0);

    // Test binary conversion
    StrClear(&s);
    StrFromI64(&s, 42, 2, false);
    result = result && (ZstrCompare(s.data, "0b101010") == 0);

    StrDeinit(&s);
    return result;
}

// Test StrFromF64 function
bool test_str_from_f64(void) {
    printf("Testing StrFromF64\n");

    Str s = StrInit();

    // Test integer conversion
    StrFromF64(&s, 123.0, 2, false, false);
    bool result = (ZstrCompare(s.data, "123.00") == 0);

    // Test fractional conversion
    StrClear(&s);
    StrFromF64(&s, 123.456, 3, false, false);
    result = result && (ZstrCompare(s.data, "123.456") == 0);

    // Test negative number
    StrClear(&s);
    StrFromF64(&s, -123.456, 3, false, false);
    result = result && (ZstrCompare(s.data, "-123.456") == 0);

    // Test scientific notation (forced)
    StrClear(&s);
    StrFromF64(&s, 123.456, 3, true, false);
    result = result && (ZstrCompare(s.data, "1.235e+02") == 0);

    // Test scientific notation (uppercase)
    StrClear(&s);
    StrFromF64(&s, 123.456, 3, true, true);
    result = result && (ZstrCompare(s.data, "1.235E+02") == 0);

    // Test very small number (auto scientific notation)
    StrClear(&s);
    StrFromF64(&s, 0.0000123, 3, false, false);
    result = result && (ZstrCompare(s.data, "1.230e-05") == 0);

    // Test very large number (auto scientific notation)
    StrClear(&s);
    StrFromF64(&s, 1234567890123.0, 2, false, false);
    result = result && (ZstrCompare(s.data, "1.23e+12") == 0);

    // Test zero
    StrClear(&s);
    StrFromF64(&s, 0.0, 2, false, false);
    result = result && (ZstrCompare(s.data, "0.00") == 0);

    // Test infinity
    StrClear(&s);
    StrFromF64(&s, INFINITY, 2, false, false);
    result = result && (ZstrCompare(s.data, "inf") == 0);

    // Test negative infinity
    StrClear(&s);
    StrFromF64(&s, -INFINITY, 2, false, false);
    result = result && (ZstrCompare(s.data, "-inf") == 0);

    // Test NaN
    StrClear(&s);
    StrFromF64(&s, NAN, 2, false, false);
    result = result && (ZstrCompare(s.data, "nan") == 0);

    StrDeinit(&s);
    return result;
}

// Test StrToU64 function
bool test_str_to_u64(void) {
    printf("Testing StrToU64\n");

    // Test decimal conversion
    Str  s       = StrInitFromZstr("12345");
    u64  value   = 0;
    bool success = StrToU64(&s, &value, 10);
    bool result  = (success && value == 12345);

    // Test hexadecimal conversion with explicit base
    StrDeinit(&s);
    s       = StrInitFromZstr("ABCD"); // No 0x prefix when base is explicitly 16
    success = StrToU64(&s, &value, 16);
    result  = result && (success && value == 0xABCD);

    // Test hexadecimal conversion (auto-detect base with 0)
    StrDeinit(&s);
    s       = StrInitFromZstr("0xABCD");
    success = StrToU64(&s, &value, 0);
    result  = result && (success && value == 0xABCD);

    // Test binary conversion
    StrDeinit(&s);
    s       = StrInitFromZstr("0b101010");
    success = StrToU64(&s, &value, 0);
    result  = result && (success && value == 42);

    // Test octal conversion
    StrDeinit(&s);
    s       = StrInitFromZstr("0o52");
    success = StrToU64(&s, &value, 0);
    result  = result && (success && value == 42);

    // Test zero
    StrDeinit(&s);
    s       = StrInitFromZstr("0");
    success = StrToU64(&s, &value, 10);
    result  = result && (success && value == 0);

    // Test invalid input
    StrDeinit(&s);
    s       = StrInitFromZstr("not a number");
    success = StrToU64(&s, &value, 10);
    result  = result && (!success);

    // Test negative number (should fail for unsigned)
    StrDeinit(&s);
    s       = StrInitFromZstr("-123");
    success = StrToU64(&s, &value, 10);
    result  = result && (!success);

    StrDeinit(&s);
    return result;
}

// Test StrToI64 function
bool test_str_to_i64(void) {
    printf("Testing StrToI64\n");

    // Test positive decimal conversion
    Str  s       = StrInitFromZstr("12345");
    i64  value   = 0;
    bool success = StrToI64(&s, &value, 10);
    bool result  = (success && value == 12345);

    // Test negative decimal conversion
    StrDeinit(&s);
    s       = StrInitFromZstr("-12345");
    success = StrToI64(&s, &value, 10);
    result  = result && (success && value == -12345);

    // Test hexadecimal conversion
    StrDeinit(&s);
    s       = StrInitFromZstr("0xABCD");
    success = StrToI64(&s, &value, 0);
    result  = result && (success && value == 0xABCD);

    // Test binary conversion
    StrDeinit(&s);
    s       = StrInitFromZstr("0b101010");
    success = StrToI64(&s, &value, 0);
    result  = result && (success && value == 42);

    // Test zero
    StrDeinit(&s);
    s       = StrInitFromZstr("0");
    success = StrToI64(&s, &value, 10);
    result  = result && (success && value == 0);

    // Test invalid input
    StrDeinit(&s);
    s       = StrInitFromZstr("not a number");
    success = StrToI64(&s, &value, 10);
    result  = result && (!success);

    StrDeinit(&s);
    return result;
}

// Test StrToF64 function
bool test_str_to_f64(void) {
    printf("Testing StrToF64\n");

    // Test integer conversion
    Str  s       = StrInitFromZstr("123");
    f64  value   = 0.0;
    bool success = StrToF64(&s, &value);
    bool result  = (success && fabs(value - 123.0) < 0.0001);

    // Test fractional conversion
    StrDeinit(&s);
    s       = StrInitFromZstr("123.456");
    success = StrToF64(&s, &value);
    result  = result && (success && fabs(value - 123.456) < 0.0001);

    // Test negative number
    StrDeinit(&s);
    s       = StrInitFromZstr("-123.456");
    success = StrToF64(&s, &value);
    result  = result && (success && fabs(value - (-123.456)) < 0.0001);

    // Test scientific notation
    StrDeinit(&s);
    s       = StrInitFromZstr("1.23e2");
    success = StrToF64(&s, &value);
    result  = result && (success && fabs(value - 123.0) < 0.0001);

    // Test zero
    StrDeinit(&s);
    s       = StrInitFromZstr("0");
    success = StrToF64(&s, &value);
    result  = result && (success && fabs(value) < 0.0001);

    // Test infinity
    StrDeinit(&s);
    s       = StrInitFromZstr("inf");
    success = StrToF64(&s, &value);
    result  = result && (success && isinf(value) && value > 0);

    // Test negative infinity
    StrDeinit(&s);
    s       = StrInitFromZstr("-inf");
    success = StrToF64(&s, &value);
    result  = result && (success && isinf(value) && value < 0);

    // Test NaN
    StrDeinit(&s);
    s       = StrInitFromZstr("nan");
    success = StrToF64(&s, &value);
    result  = result && (success && isnan(value));

    // Test invalid input
    StrDeinit(&s);
    s       = StrInitFromZstr("not a number");
    success = StrToF64(&s, &value);
    result  = result && (!success);

    StrDeinit(&s);
    return result;
}

// Main function that runs all tests
int main(void) {
    printf("[INFO] Starting Str.Convert tests\n\n");

    // Array of test functions
    TestFunction tests[] =
        {test_str_from_u64, test_str_from_i64, test_str_from_f64, test_str_to_u64, test_str_to_i64, test_str_to_f64};

    int total_tests = sizeof(tests) / sizeof(tests[0]);

    // Run all tests using the centralized test driver
    return run_test_suite(tests, total_tests, NULL, 0, "Str.Convert");
}
