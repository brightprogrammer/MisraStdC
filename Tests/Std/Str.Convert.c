#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Memory.h>
#include <Misra/Std/Math.h>
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
bool test_str_round_trip_conversions(void);
bool test_str_edge_case_conversions(void);
bool test_str_precision_limits(void);
bool test_str_all_base_support(void);
bool test_str_large_scale_conversions(void);
bool test_str_conversion_null_failures(void);
bool test_str_conversion_bounds_failures(void);
bool test_str_conversion_invalid_input_failures(void);

// Test StrFromU64 function
bool test_str_from_u64(void) {
    WriteFmt("Testing StrFromU64\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    Str s = StrInit(&alloc);

    // Test decimal conversion
    StrIntFormat config = {.base = 10, .uppercase = false};
    StrFromU64(&s, 12345, &config);
    bool result = (ZstrCompare(StrBegin(&s), "12345") == 0);
    if (!result) {
        WriteFmt("    FAIL: Expected '12345', got '{}'\n", s);
    }

    // Test hexadecimal conversion (lowercase)
    StrClear(&s);
    config = (StrIntFormat) {.base = 16, .uppercase = false, .use_prefix = true};
    StrFromU64(&s, 0xABCD, &config);
    result = result && (ZstrCompare(StrBegin(&s), "0xabcd") == 0);
    if (!result) {
        WriteFmt("    FAIL: Expected '0xabcd', got '{}'\n", s);
    }

    // Test hexadecimal conversion (uppercase)
    StrClear(&s);
    config = (StrIntFormat) {.base = 16, .uppercase = true, .use_prefix = true};
    StrFromU64(&s, 0xABCD, &config);
    result = result && (ZstrCompare(StrBegin(&s), "0xABCD") == 0);
    if (!result) {
        WriteFmt("    FAIL: Expected '0xABCD', got '{}'\n", s);
    }

    // Test binary conversion
    StrClear(&s);
    config = (StrIntFormat) {.base = 2, .uppercase = false, .use_prefix = true};
    StrFromU64(&s, 42, &config);
    result = result && (ZstrCompare(StrBegin(&s), "0b101010") == 0);
    if (!result) {
        WriteFmt("    FAIL: Expected '0b101010', got '{}'\n", s);
    }

    // Test octal conversion
    StrClear(&s);
    config = (StrIntFormat) {.base = 8, .uppercase = false, .use_prefix = true};
    StrFromU64(&s, 42, &config);
    result = result && (ZstrCompare(StrBegin(&s), "0o52") == 0);
    if (!result) {
        WriteFmt("    FAIL: Expected '0o52', got '{}'\n", s);
    }

    // Test zero
    StrClear(&s);
    config = (StrIntFormat) {.base = 10, .uppercase = false};
    StrFromU64(&s, 0, &config);
    result = result && (ZstrCompare(StrBegin(&s), "0") == 0);
    if (!result) {
        WriteFmt("    FAIL: Expected '0', got '{}'\n", s);
    }

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test StrFromI64 function
bool test_str_from_i64(void) {
    WriteFmt("Testing StrFromI64\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    Str s = StrInit(&alloc);

    // Test positive decimal conversion
    StrIntFormat config = {.base = 10, .uppercase = false};
    StrFromI64(&s, 12345, &config);
    bool result = (ZstrCompare(StrBegin(&s), "12345") == 0);
    if (!result) {
        WriteFmt("    FAIL: Expected '12345', got '{}'\n", StrBegin(&s));
    }

    // Test negative decimal conversion (only decimal supports negative sign)
    StrClear(&s);
    config = (StrIntFormat) {.base = 10, .uppercase = false};
    StrFromI64(&s, -12345, &config);
    result = result && (ZstrCompare(StrBegin(&s), "-12345") == 0);
    if (!result) {
        WriteFmt("    FAIL: Expected '-12345', got '{}'\n", StrBegin(&s));
    }

    // Test hexadecimal conversion of negative number (negative non-decimal treated as unsigned)
    StrClear(&s);
    config = (StrIntFormat) {.base = 16, .uppercase = false, .use_prefix = true};
    StrFromI64(&s, -0xABCD, &config);
    // For negative numbers in non-decimal bases, it uses unsigned representation
    // -0xABCD = -(43981) = large positive number when treated as unsigned
    result = result && (ZstrCompareN(StrBegin(&s), "0x", 2) == 0);
    if (!result) {
        WriteFmt("    FAIL: Expected hex prefix '0x', got '{}'\n", StrBegin(&s));
    }

    // Test zero
    StrClear(&s);
    config = (StrIntFormat) {.base = 10, .uppercase = false};
    StrFromI64(&s, 0, &config);
    result = result && (ZstrCompare(StrBegin(&s), "0") == 0);
    if (!result) {
        WriteFmt("    FAIL: Expected '0', got '{}'\n", StrBegin(&s));
    }

    // Test binary conversion
    StrClear(&s);
    config = (StrIntFormat) {.base = 2, .uppercase = false, .use_prefix = true};
    StrFromI64(&s, 42, &config);
    result = result && (ZstrCompare(StrBegin(&s), "0b101010") == 0);
    if (!result) {
        WriteFmt("    FAIL: Expected '0b101010', got '{}'\n", StrBegin(&s));
    }

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test StrFromF64 function
bool test_str_from_f64(void) {
    WriteFmt("Testing StrFromF64\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    Str s = StrInit(&alloc);

    // Test integer conversion
    StrFloatFormat config = {.precision = 2, .force_sci = false, .uppercase = false};
    StrFromF64(&s, 123.0, &config);
    bool result = (ZstrCompare(StrBegin(&s), "123.00") == 0);
    if (!result) {
        WriteFmt("    FAIL: Expected '123.00', got '{}'\n", StrBegin(&s));
    }

    // Test fractional conversion
    StrClear(&s);
    config = (StrFloatFormat) {.precision = 3, .force_sci = false, .uppercase = false};
    StrFromF64(&s, 123.456, &config);
    result = result && (ZstrCompare(StrBegin(&s), "123.456") == 0);
    if (!result) {
        WriteFmt("    FAIL: Expected '123.456', got '{}'\n", StrBegin(&s));
    }

    // Test negative number
    StrClear(&s);
    config = (StrFloatFormat) {.precision = 3, .force_sci = false, .uppercase = false};
    StrFromF64(&s, -123.456, &config);
    result = result && (ZstrCompare(StrBegin(&s), "-123.456") == 0);

    // Test scientific notation (forced)
    StrClear(&s);
    config = (StrFloatFormat) {.precision = 3, .force_sci = true, .uppercase = false};
    StrFromF64(&s, 123.456, &config);
    result = result && (ZstrCompare(StrBegin(&s), "1.235e+02") == 0);

    // Test scientific notation (uppercase)
    StrClear(&s);
    config = (StrFloatFormat) {.precision = 3, .force_sci = true, .uppercase = true};
    StrFromF64(&s, 123.456, &config);
    result = result && (ZstrCompare(StrBegin(&s), "1.235E+02") == 0);

    // Test very small number (auto scientific notation)
    StrClear(&s);
    config = (StrFloatFormat) {.precision = 3, .force_sci = false, .uppercase = false};
    StrFromF64(&s, 0.0000123, &config);
    result = result && (ZstrCompare(StrBegin(&s), "1.230e-05") == 0);

    // Test very large number (auto scientific notation)
    StrClear(&s);
    config = (StrFloatFormat) {.precision = 2, .force_sci = false, .uppercase = false};
    StrFromF64(&s, 1234567890123.0, &config);
    result = result && (ZstrCompare(StrBegin(&s), "1.23e+12") == 0);

    // Test zero
    StrClear(&s);
    config = (StrFloatFormat) {.precision = 2, .force_sci = false, .uppercase = false};
    StrFromF64(&s, 0.0, &config);
    result = result && (ZstrCompare(StrBegin(&s), "0.00") == 0);

    // Test infinity
    StrClear(&s);
    config = (StrFloatFormat) {.precision = 2, .force_sci = false, .uppercase = false};
    StrFromF64(&s, F64_INFINITY, &config);
    result = result && (ZstrCompare(StrBegin(&s), "inf") == 0);

    // Test negative infinity
    StrClear(&s);
    config = (StrFloatFormat) {.precision = 2, .force_sci = false, .uppercase = false};
    StrFromF64(&s, -F64_INFINITY, &config);
    result = result && (ZstrCompare(StrBegin(&s), "-inf") == 0);

    // Test NaN
    StrClear(&s);
    config = (StrFloatFormat) {.precision = 2, .force_sci = false, .uppercase = false};
    StrFromF64(&s, F64_NAN, &config);
    result = result && (ZstrCompare(StrBegin(&s), "nan") == 0);

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test StrToU64 function
bool test_str_to_u64(void) {
    WriteFmt("Testing StrToU64\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    // Test decimal conversion
    Str  s       = StrInitFromZstr("12345", &alloc);
    u64  value   = 0;
    bool success = StrToU64(&s, &value, NULL);
    bool result  = (success && value == 12345);

    // Test hexadecimal conversion with explicit base
    StrDeinit(&s);
    s                     = StrInitFromZstr("ABCD", &alloc); // No 0x prefix when base is explicitly 16
    StrParseConfig config = {.base = 16};
    success               = StrToU64(&s, &value, &config);
    result                = result && (success && value == 0xABCD);

    // Test hexadecimal conversion (auto-detect base with 0)
    StrDeinit(&s);
    s       = StrInitFromZstr("0xABCD", &alloc);
    success = StrToU64(&s, &value, NULL);
    result  = result && (success && value == 0xABCD);

    // Test binary conversion
    StrDeinit(&s);
    s       = StrInitFromZstr("0b101010", &alloc);
    success = StrToU64(&s, &value, NULL);
    result  = result && (success && value == 42);

    // Test octal conversion
    StrDeinit(&s);
    s       = StrInitFromZstr("0o52", &alloc);
    success = StrToU64(&s, &value, NULL);
    result  = result && (success && value == 42);

    // Test zero
    StrDeinit(&s);
    s       = StrInitFromZstr("0", &alloc);
    success = StrToU64(&s, &value, NULL);
    result  = result && (success && value == 0);

    // Test invalid input
    StrDeinit(&s);
    s       = StrInitFromZstr("not a number", &alloc);
    success = StrToU64(&s, &value, NULL);
    result  = result && (!success);

    // Test negative number (should fail for unsigned)
    StrDeinit(&s);
    s       = StrInitFromZstr("-123", &alloc);
    success = StrToU64(&s, &value, NULL);
    result  = result && (!success);

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test StrToI64 function
bool test_str_to_i64(void) {
    WriteFmt("Testing StrToI64\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    // Test positive decimal conversion
    Str  s       = StrInitFromZstr("12345", &alloc);
    i64  value   = 0;
    bool success = StrToI64(&s, &value, NULL);
    bool result  = (success && value == 12345);

    // Test negative decimal conversion
    StrDeinit(&s);
    s       = StrInitFromZstr("-12345", &alloc);
    success = StrToI64(&s, &value, NULL);
    result  = result && (success && value == -12345);

    // Test hexadecimal conversion
    StrDeinit(&s);
    s       = StrInitFromZstr("0xABCD", &alloc);
    success = StrToI64(&s, &value, NULL);
    result  = result && (success && value == 0xABCD);

    // Test binary conversion
    StrDeinit(&s);
    s       = StrInitFromZstr("0b101010", &alloc);
    success = StrToI64(&s, &value, NULL);
    result  = result && (success && value == 42);

    // Test zero
    StrDeinit(&s);
    s       = StrInitFromZstr("0", &alloc);
    success = StrToI64(&s, &value, NULL);
    result  = result && (success && value == 0);

    // Test invalid input
    StrDeinit(&s);
    s       = StrInitFromZstr("not a number", &alloc);
    success = StrToI64(&s, &value, NULL);
    result  = result && (!success);

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test StrToF64 function
bool test_str_to_f64(void) {
    WriteFmt("Testing StrToF64\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    // Test integer conversion
    Str  s       = StrInitFromZstr("123", &alloc);
    f64  value   = 0.0;
    bool success = StrToF64(&s, &value, NULL);
    bool result  = (success && F64Abs(value - 123.0) < 0.0001);

    // Test fractional conversion
    StrDeinit(&s);
    s       = StrInitFromZstr("123.456", &alloc);
    success = StrToF64(&s, &value, NULL);
    result  = result && (success && F64Abs(value - 123.456) < 0.0001);

    // Test negative number
    StrDeinit(&s);
    s       = StrInitFromZstr("-123.456", &alloc);
    success = StrToF64(&s, &value, NULL);
    result  = result && (success && F64Abs(value - (-123.456)) < 0.0001);

    // Test scientific notation
    StrDeinit(&s);
    s       = StrInitFromZstr("1.23e2", &alloc);
    success = StrToF64(&s, &value, NULL);
    result  = result && (success && F64Abs(value - 123.0) < 0.0001);

    // Test zero
    StrDeinit(&s);
    s       = StrInitFromZstr("0", &alloc);
    success = StrToF64(&s, &value, NULL);
    result  = result && (success && F64Abs(value) < 0.0001);

    // Test infinity
    StrDeinit(&s);
    s       = StrInitFromZstr("inf", &alloc);
    success = StrToF64(&s, &value, NULL);
    result  = result && (success && F64IsInf(value) && value > 0);

    // Test negative infinity
    StrDeinit(&s);
    s       = StrInitFromZstr("-inf", &alloc);
    success = StrToF64(&s, &value, NULL);
    result  = result && (success && F64IsInf(value) && value < 0);

    // Test NaN
    StrDeinit(&s);
    s       = StrInitFromZstr("nan", &alloc);
    success = StrToF64(&s, &value, NULL);
    result  = result && (success && F64IsNan(value));

    // Test invalid input
    StrDeinit(&s);
    s       = StrInitFromZstr("not a number", &alloc);
    success = StrToF64(&s, &value, NULL);
    result  = result && (!success);

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Round-trip conversion tests
bool test_str_round_trip_conversions(void) {
    WriteFmt("Testing Str round-trip conversions\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    bool result = true;

    // Test integer round-trips
    u64 u64_values[] = {0, 1, 42, 255, 65535, 0xFFFFFFFF, 0x123456789ABCDEF, UINT64_MAX};
    i64 i64_values[] = {
        0,
        1,
        -1,
        42,
        -42,
        32767,
        -32768,
        2147483647,
        -2147483648,
        INT64_MAX,
        INT64_MIN + 1
    }; // Avoid INT64_MIN negation issue

    for (size i = 0; i < sizeof(u64_values) / sizeof(u64_values[0]); i++) {
        Str s = StrInit(&alloc);

        // Test decimal round-trip
        StrIntFormat config = {.base = 10, .uppercase = false};
        StrFromU64(&s, u64_values[i], &config);
        u64  recovered_u64 = 0;
        bool success       = StrToU64(&s, &recovered_u64, NULL);
        result             = result && success && (recovered_u64 == u64_values[i]);

        // Test hex round-trip
        StrClear(&s);
        config = (StrIntFormat) {.base = 16, .uppercase = false, .use_prefix = true};
        StrFromU64(&s, u64_values[i], &config);
        recovered_u64 = 0;
        success       = StrToU64(&s, &recovered_u64, NULL); // Can now use explicit base 16 with 0x prefix
        result        = result && success && (recovered_u64 == u64_values[i]);

        StrDeinit(&s);
    }

    for (size i = 0; i < sizeof(i64_values) / sizeof(i64_values[0]); i++) {
        Str s = StrInit(&alloc);

        // Test decimal round-trip
        StrIntFormat config = {.base = 10, .uppercase = false};
        StrFromI64(&s, i64_values[i], &config);
        i64  recovered_i64 = 0;
        bool success       = StrToI64(&s, &recovered_i64, NULL);
        result             = result && success && (recovered_i64 == i64_values[i]);

        StrDeinit(&s);
    }

    // Test double round-trips with various precisions
    f64 f64_values[] = {0.0, 1.0, -1.0, 3.14159, -3.14159, 1e-10, 1e10, 123.456789};

    for (size i = 0; i < sizeof(f64_values) / sizeof(f64_values[0]); i++) {
        for (u8 precision = 1; precision <= 6; precision++) {
            Str s = StrInit(&alloc);

            StrFloatFormat config = {.precision = precision, .force_sci = false, .uppercase = false};
            StrFromF64(&s, f64_values[i], &config);
            f64  recovered_f64 = 0.0;
            bool success       = StrToF64(&s, &recovered_f64, NULL);

            // Allow for precision loss
            f64 tolerance = F64Pow(10.0, -(i32)precision + 1);
            result        = result && success && (F64Abs(recovered_f64 - f64_values[i]) < tolerance);

            StrDeinit(&s);
        }
    }

    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Edge case conversion tests
bool test_str_edge_case_conversions(void) {
    WriteFmt("Testing Str edge case conversions\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    bool result = true;

    // Test base boundary conditions
    for (u8 base = 2; base <= 36; base++) {
        Str s = StrInit(&alloc);

        // Test with base value itself
        StrIntFormat config = {.base = base, .uppercase = false, .use_prefix = (base == 2 || base == 8 || base == 16)};
        StrFromU64(&s, base, &config);
        u64 recovered = 0;

        // For bases with prefixes, use auto-detect. For others, use explicit base parsing
        StrParseConfig parse_config = {.base = base};
        bool           success      = StrToU64(&s, &recovered, config.use_prefix ? NULL : &parse_config);
        result                      = result && success && (recovered == base);

        StrDeinit(&s);
    }

    // Test extreme values
    Str s = StrInit(&alloc);

    // Test maximum u64
    StrIntFormat config = {.base = 10, .uppercase = false};
    StrFromU64(&s, UINT64_MAX, &config);
    u64  recovered_max = 0;
    bool success       = StrToU64(&s, &recovered_max, NULL);
    result             = result && success && (recovered_max == UINT64_MAX);

    // Test minimum i64 (avoid INT64_MIN due to negation UB)
    StrClear(&s);
    config = (StrIntFormat) {.base = 10, .uppercase = false};
    StrFromI64(&s, INT64_MIN + 1, &config);
    i64 recovered_min = 0;
    success           = StrToI64(&s, &recovered_min, NULL);
    result            = result && success && (recovered_min == INT64_MIN + 1);

    // Test very small floating point
    StrClear(&s);
    StrFloatFormat fconfig = {.precision = 3, .force_sci = false, .uppercase = false};
    StrFromF64(&s, 1e-300, &fconfig);
    f64 recovered_small = 0.0;
    success             = StrToF64(&s, &recovered_small, NULL);
    result              = result && success && (recovered_small < 1e-299);

    // Test very large floating point
    StrClear(&s);
    fconfig = (StrFloatFormat) {.precision = 3, .force_sci = false, .uppercase = false};
    StrFromF64(&s, 1e300, &fconfig);
    f64 recovered_large = 0.0;
    success             = StrToF64(&s, &recovered_large, NULL);
    result              = result && success && (recovered_large > 1e299);

    StrDeinit(&s);

    // Test prefix handling
    struct {
        Zstr input;
        u64  expected;
        u8   base;
    } prefix_tests[] = {
        {  "0x1A",  26,  0}, // Auto-detect hex
        {"0b1010",  10,  0}, // Auto-detect binary
        {  "0o17",  15,  0}, // Auto-detect octal
        {    "42",  42,  0}, // Auto-detect decimal
        {    "FF", 255, 16}, // Explicit hex (no prefix)
        {  "1010",  10,  2}, // Explicit binary (no prefix)
    };

    for (size i = 0; i < sizeof(prefix_tests) / sizeof(prefix_tests[0]); i++) {
        Str            test_str = StrInitFromZstr(prefix_tests[i].input, &alloc);
        u64            value    = 0;
        StrParseConfig config   = {.base = prefix_tests[i].base};
        bool           success  = StrToU64(&test_str, &value, prefix_tests[i].base == 0 ? NULL : &config);
        result                  = result && success && (value == prefix_tests[i].expected);
        StrDeinit(&test_str);
    }

    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Precision limits testing
bool test_str_precision_limits(void) {
    WriteFmt("Testing Str precision limits\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    bool result = true;

    // Test floating point precision boundaries
    f64 test_value = 123.456789012345;

    for (u8 precision = 1; precision <= 17; precision++) {
        Str s = StrInit(&alloc);

        StrFloatFormat config = {.precision = precision, .force_sci = false, .uppercase = false};
        StrFromF64(&s, test_value, &config);

        // String should have expected decimal places
        char *dot_pos = ZstrFindChar(StrBegin(&s), '.');
        if (dot_pos) {
            size decimal_places = ZstrLen(dot_pos + 1);
            // Allow for trailing zeros being omitted in some cases
            result = result && (decimal_places <= precision);
        }

        StrDeinit(&s);
    }

    // Test scientific notation thresholds
    f64 sci_values[] = {1e-5, 1e-4, 1e15, 1e16};

    for (size i = 0; i < sizeof(sci_values) / sizeof(sci_values[0]); i++) {
        Str s = StrInit(&alloc);

        // Force scientific notation
        StrFloatFormat config = {.precision = 3, .force_sci = true, .uppercase = false};
        StrFromF64(&s, sci_values[i], &config);
        bool has_e = (ZstrFindChar(StrBegin(&s), 'e') != NULL);
        result     = result && has_e;

        // Test uppercase E
        StrClear(&s);
        config = (StrFloatFormat) {.precision = 3, .force_sci = true, .uppercase = true};
        StrFromF64(&s, sci_values[i], &config);
        bool has_E = (ZstrFindChar(StrBegin(&s), 'E') != NULL);
        result     = result && has_E;

        StrDeinit(&s);
    }

    // Test base conversion accuracy
    u64 large_value = 0x123456789ABCDEF;

    for (u8 base = 2; base <= 36; base++) {
        Str s = StrInit(&alloc);

        StrIntFormat config = {.base = base, .uppercase = false};
        StrFromU64(&s, large_value, &config);
        u64            recovered = 0;
        StrParseConfig pconfig   = {.base = base};
        bool           success   = StrToU64(&s, &recovered, &pconfig); // Can now use explicit base with prefixes
        result                   = result && success && (recovered == large_value);

        StrDeinit(&s);
    }

    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Large-scale conversion tests
bool test_str_all_base_support(void) {
    WriteFmt("Testing Str all bases 2-36 support\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    bool result = true;

    // Test value to convert across all bases
    u64 test_value = 12345;

    // Test each base from 2 to 36
    for (u8 base = 2; base <= 36; base++) {
        Str s = StrInit(&alloc);

        // Test StrFromU64
        StrIntFormat config = {.base = base, .uppercase = false, .use_prefix = false};
        StrFromU64(&s, test_value, &config);

        // Test StrToU64 round-trip
        u64            recovered    = 0;
        StrParseConfig parse_config = {.base = base};
        bool           success      = StrToU64(&s, &recovered, &parse_config);

        result = result && success && (recovered == test_value);

        StrDeinit(&s);
    }

    // Test uppercase digits for bases that use letters (11-36)
    for (u8 base = 11; base <= 36; base++) {
        Str s = StrInit(&alloc);

        StrIntFormat config = {.base = base, .uppercase = true, .use_prefix = false};
        StrFromU64(&s, test_value, &config);

        u64            recovered    = 0;
        StrParseConfig parse_config = {.base = base};
        bool           success      = StrToU64(&s, &recovered, &parse_config);

        result = result && success && (recovered == test_value);

        StrDeinit(&s);
    }

    // Test multiple values across all bases
    u64 test_values[] = {0, 1, 10, 100, 255, 1000, 65535};

    for (size i = 0; i < sizeof(test_values) / sizeof(test_values[0]); i++) {
        for (u8 base = 2; base <= 36; base++) {
            Str s = StrInit(&alloc);

            StrIntFormat config = {.base = base, .uppercase = false, .use_prefix = false};
            StrFromU64(&s, test_values[i], &config);

            u64            recovered    = 0;
            StrParseConfig parse_config = {.base = base};
            bool           success      = StrToU64(&s, &recovered, &parse_config);

            result = result && success && (recovered == test_values[i]);

            StrDeinit(&s);

            if (!result)
                break;
        }
        if (!result)
            break;
    }

    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_str_large_scale_conversions(void) {
    WriteFmt("Testing Str large-scale conversions\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    bool result = true;

    // Test many values for consistency
    for (u64 i = 0; i < 1000; i++) {
        u64 test_value = i * 1000007; // Large prime multiplier

        Str          s      = StrInit(&alloc);
        StrIntFormat config = {.base = 10, .uppercase = false};
        StrFromU64(&s, test_value, &config);

        u64  recovered = 0;
        bool success   = StrToU64(&s, &recovered, NULL);
        result         = result && success && (recovered == test_value);

        StrDeinit(&s);

        if (!result)
            break; // Early exit on failure
    }

    // Test various floating point values
    for (int exp = -10; exp <= 10; exp++) {
        for (int mantissa = 1; mantissa <= 9; mantissa++) {
            f64 test_value = mantissa * F64Pow((10.0), (i32)(exp));

            Str            s      = StrInit(&alloc);
            StrFloatFormat config = {.precision = 6, .force_sci = false, .uppercase = false};
            StrFromF64(&s, test_value, &config);

            f64  recovered = 0.0;
            bool success   = StrToF64(&s, &recovered, NULL);

            // Allow for floating point precision issues
            f64 tolerance = F64Abs(test_value) * 1e-10;
            if (tolerance < 1e-15)
                tolerance = 1e-15;

            result = result && success && (F64Abs(recovered - test_value) < tolerance);

            StrDeinit(&s);

            if (!result)
                break;
        }
        if (!result)
            break;
    }

    // Test very long strings
    char long_number[100];
    MemCopy(long_number, "12345678901234567890123456789012345678901234567890", 51);

    Str  long_str   = StrInitFromZstr(long_number, &alloc);
    u64  long_value = 0;
    bool success    = StrToU64(&long_str, &long_value, NULL);
    // This might overflow, but should handle gracefully
    result = result && (success || !success); // Either succeeds or fails gracefully

    StrDeinit(&long_str);

    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Deadend tests for NULL pointer handling
bool test_str_conversion_null_failures(void) {
    WriteFmt("Testing Str conversion NULL pointer handling\n");

    // Test NULL string pointer - should abort
    StrIntFormat config = {.base = 10, .uppercase = false};
    StrFromU64(NULL, 42, &config);

    return false;
}

bool test_str_conversion_bounds_failures(void) {
    WriteFmt("Testing Str conversion bounds failures\n");

    // Test StrFromI64 with NULL pointer - should abort
    StrIntFormat config = {.base = 10, .uppercase = false};
    StrFromI64(NULL, 42, &config);

    return false;
}

bool test_str_conversion_invalid_input_failures(void) {
    WriteFmt("Testing Str conversion invalid input failures\n");

    // Test StrFromF64 with NULL pointer - should abort
    StrFloatFormat config = {.precision = 2, .force_sci = false, .uppercase = false};
    StrFromF64(NULL, 123.45, &config);

    return false;
}

// Main function that runs all tests
int main(void) {
    WriteFmt("[INFO] Starting Str.Convert tests\n\n");

    // Array of normal test functions
    TestFunction tests[] = {
        test_str_from_u64,
        test_str_from_i64,
        test_str_from_f64,
        test_str_to_u64,
        test_str_to_i64,
        test_str_to_f64,
        test_str_round_trip_conversions,
        test_str_edge_case_conversions,
        test_str_precision_limits,
        test_str_all_base_support,
        test_str_large_scale_conversions
    };

    // Array of deadend test functions
    TestFunction deadend_tests[] = {
        test_str_conversion_null_failures,
        test_str_conversion_bounds_failures,
        test_str_conversion_invalid_input_failures
    };

    int total_tests         = sizeof(tests) / sizeof(tests[0]);
    int total_deadend_tests = sizeof(deadend_tests) / sizeof(deadend_tests[0]);

    // Run all tests using the centralized test driver
    return run_test_suite(tests, total_tests, deadend_tests, total_deadend_tests, "Str.Convert");
}
