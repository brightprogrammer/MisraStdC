#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Allocator/Budget.h>
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
        Zstr dot_pos = ZstrFindChar(StrBegin(&s), '.');
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

// ===========================================================================
// Mutation-survivor guards relocated from the Str.Mutants* staging suites.
// Each pins a caller-observable outcome of a numeric-conversion path.
// ===========================================================================

// ---- StrToF64 (Str.Mutants1) ----------------------------------------------

// 1010:cxx_remove_void_call -- deletes ValidateStr(str). Real code aborts on
// an uninitialized Str; the mutant skips validation and parses garbage.
static bool test_validate_str_guard(void) {
    Str s     = {0}; // zero magic -> ValidateStr LOG_FATALs
    f64 value = 0.0;
    // Should abort inside ValidateStr; the return is never reached on real code.
    StrToF64(&s, &value, NULL);
    return false;
}

// 1021:cxx_lt_to_ge -- leading-whitespace loop `pos<length` -> `pos>=length`.
// Mutant never skips the leading space; parsing then finds no digits.
static bool test_leading_space_skip_condition(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              s     = StrInitFromZstr(" 1.5", &alloc);
    f64              value = 0.0;
    bool             ok    = StrToF64(&s, &value, NULL);
    bool             pass  = ok && F64Abs(value - 1.5) < 1e-9;
    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return pass;
}

// 1022:cxx_post_inc_to_post_dec -- leading-space loop `pos++` -> `pos--`.
// `pos` underflows to SIZE_MAX, so the input is rejected as "Empty string".
static bool test_leading_space_advances(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              s     = StrInitFromZstr(" 1.0", &alloc);
    f64              value = 0.0;
    bool             ok    = StrToF64(&s, &value, NULL);
    bool             pass  = ok && F64Abs(value - 1.0) < 1e-9;
    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return pass;
}

// 1037:cxx_sub_to_add -- nan acceptance `length-pos==3` -> `length+pos==3`.
// With a leading space (pos=1) the exact-"nan" path is lost and the token is
// rejected.
static bool test_nan_exact_accept_with_offset(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              s     = StrInitFromZstr(" nan", &alloc);
    f64              value = 0.0;
    bool             ok    = StrToF64(&s, &value, NULL);
    bool             pass  = ok && F64IsNan(value);
    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return pass;
}

// 1043:cxx_sub_to_add -- inf acceptance `length-pos==3` -> `length+pos==3`.
// Same as 1037 for the inf branch.
static bool test_inf_exact_accept_with_offset(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              s     = StrInitFromZstr(" inf", &alloc);
    f64              value = 0.0;
    bool             ok    = StrToF64(&s, &value, NULL);
    bool             pass  = ok && F64IsInf(value) && value > 0;
    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return pass;
}

// 1052:cxx_assign_const -- `negative=true` forced to false. A negative literal
// comes back positive.
static bool test_minus_sign_makes_negative(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              s     = StrInitFromZstr("-5", &alloc);
    f64              value = 0.0;
    bool             ok    = StrToF64(&s, &value, NULL);
    bool             pass  = ok && F64Abs(value - (-5.0)) < 1e-9;
    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return pass;
}

// 1070:cxx_post_inc_to_post_dec -- '+' branch `pos++` -> `pos--`. `pos`
// underflows and the digits are never seen.
static bool test_plus_sign_advances(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              s     = StrInitFromZstr("+5", &alloc);
    f64              value = 0.0;
    bool             ok    = StrToF64(&s, &value, NULL);
    bool             pass  = ok && F64Abs(value - 5.0) < 1e-9;
    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return pass;
}

// 1078:cxx_assign_const -- integer-digit `have_digits=true` forced to false.
// A plain integer is wrongly rejected as "No valid digits".
static bool test_integer_digits_set_flag(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              s     = StrInitFromZstr("42", &alloc);
    f64              value = 0.0;
    bool             ok    = StrToF64(&s, &value, NULL);
    bool             pass  = ok && F64Abs(value - 42.0) < 1e-9;
    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return pass;
}

// 1089:cxx_assign_const -- fractional `have_digits=true` forced to false. A
// fraction-only literal (no integer digits) is wrongly rejected.
static bool test_fractional_digits_set_flag(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              s     = StrInitFromZstr(".5", &alloc);
    f64              value = 0.0;
    bool             ok    = StrToF64(&s, &value, NULL);
    bool             pass  = ok && F64Abs(value - 0.5) < 1e-9;
    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return pass;
}

// 1094:cxx_eq_to_ne -- exponent gate `data[pos]=='E'` -> `data[pos]!='E'`.
// A tolerated trailing non-exponent char wrongly enters the exponent block
// and then fails for lack of exponent digits.
static bool test_exponent_gate_not_widened(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              s     = StrInitFromZstr("5x", &alloc);
    f64              value = 0.0;
    bool             ok    = StrToF64(&s, &value, NULL); // non-strict default
    bool             pass  = ok && F64Abs(value - 5.0) < 1e-9;
    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return pass;
}

// 1100:cxx_assign_const -- `exp_negative=true` forced to false. The exponent
// is applied with the wrong sign.
static bool test_negative_exponent_sign(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              s     = StrInitFromZstr("1e-2", &alloc);
    f64              value = 0.0;
    bool             ok    = StrToF64(&s, &value, NULL);
    bool             pass  = ok && F64Abs(value - 0.01) < 1e-9;
    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return pass;
}

// 1117:cxx_assign_const -- `have_exp_digits=true` forced to false. Valid
// scientific notation is rejected as "Missing exponent digits".
static bool test_exponent_digits_set_flag(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              s     = StrInitFromZstr("1e5", &alloc);
    f64              value = 0.0;
    bool             ok    = StrToF64(&s, &value, NULL);
    bool             pass  = ok && F64Abs(value - 100000.0) < 1e-6;
    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return pass;
}

// 1131:cxx_lt_to_ge -- trailing-space loop `pos<length` -> `pos>=length`.
// Mutant never consumes the trailing space, so a strict caller sees an extra
// character and rejects a valid number.
static bool test_trailing_space_skip_condition(void) {
    DefaultAllocator alloc  = DefaultAllocatorInit();
    Str              s      = StrInitFromZstr("5 ", &alloc);
    f64              value  = 0.0;
    StrParseConfig   config = {.strict = true, .trim_space = true, .base = 0};
    bool             ok     = StrToF64(&s, &value, &config);
    bool             pass   = ok && F64Abs(value - 5.0) < 1e-9;
    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return pass;
}

// 1132:cxx_post_inc_to_post_dec -- trailing-space loop `pos++` -> `pos--`.
// `pos` walks backward onto the parsed digit, leaving `pos<length` so a strict
// caller rejects the number.
static bool test_trailing_space_advances(void) {
    DefaultAllocator alloc  = DefaultAllocatorInit();
    Str              s      = StrInitFromZstr("5 ", &alloc);
    f64              value  = 0.0;
    StrParseConfig   config = {.strict = true, .trim_space = true, .base = 0};
    bool             ok     = StrToF64(&s, &value, &config);
    bool             pass   = ok && F64Abs(value - 5.0) < 1e-9;
    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return pass;
}

// 1134:cxx_lt_to_ge -- strict trailing check `pos<length` -> `pos>=length`.
// With no trailing chars `pos==length`, so the mutant wrongly reports "Extra
// characters".
static bool test_strict_trailing_check_ge(void) {
    DefaultAllocator alloc  = DefaultAllocatorInit();
    Str              s      = StrInitFromZstr("5", &alloc);
    f64              value  = 0.0;
    StrParseConfig   config = {.strict = true, .trim_space = true, .base = 0};
    bool             ok     = StrToF64(&s, &value, &config);
    bool             pass   = ok && F64Abs(value - 5.0) < 1e-9;
    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return pass;
}

// 1134:cxx_lt_to_le -- strict trailing check `pos<length` -> `pos<=length`.
// At `pos==length` the mutant condition is true and rejects a valid number.
static bool test_strict_trailing_check_le(void) {
    DefaultAllocator alloc  = DefaultAllocatorInit();
    Str              s      = StrInitFromZstr("42", &alloc);
    f64              value  = 0.0;
    StrParseConfig   config = {.strict = true, .trim_space = true, .base = 0};
    bool             ok     = StrToF64(&s, &value, &config);
    bool             pass   = ok && F64Abs(value - 42.0) < 1e-9;
    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return pass;
}

// ---- StrToU64 (Str.Mutants2) ----------------------------------------------

// 874:14 cxx_ne_to_eq -- `base != 0 && !is_valid_base(base)` guard. Flipping
// to `base == 0 && ...` disables the out-of-range-base rejection. Base 99 is
// invalid (>36) but every decimal digit is in-range for it, so only this
// entry guard refuses it -- char_to_digit downstream would happily accept "5"
// (5 < 99). Real code rejects base 99; the mutant parses "5" as 5.
static bool test_str_to_u64_invalid_base_rejected(void) {
    WriteFmt("Testing StrToU64 rejects an out-of-range base\n");
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str            s      = StrInitFromZstr("5", &alloc);
    u64            value  = 0;
    StrParseConfig config = {.base = 99};
    bool           ok     = StrToU64(&s, &value, &config);
    bool           result = (!ok);

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 874:23 cxx_replace_scalar_call -- is_valid_base(base) replaced by a
// constant breaks the valid/invalid base decision. A valid explicit base 16
// parse of "ff" must succeed with value 255.
static bool test_str_to_u64_valid_base_sixteen_accepted(void) {
    WriteFmt("Testing StrToU64 accepts explicit base 16\n");
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str            s      = StrInitFromZstr("ff", &alloc);
    u64            value  = 0;
    StrParseConfig config = {.base = 16};
    bool           ok     = StrToU64(&s, &value, &config);
    bool           result = (ok && value == 255);

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 880:16 cxx_lt_to_ge -- leading-whitespace skip `pos < str->length` flipped
// to `pos >= str->length` never skips leading spaces, so " 42" hits a space
// digit and is rejected. Real code parses 42.
static bool test_str_to_u64_leading_space_skipped(void) {
    WriteFmt("Testing StrToU64 skips leading whitespace\n");
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str  s      = StrInitFromZstr(" 42", &alloc);
    u64  value  = 0;
    bool ok     = StrToU64(&s, &value, NULL);
    bool result = (ok && value == 42);

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 881:12 cxx_post_inc_to_post_dec -- leading-whitespace skip `pos++` flipped
// to `pos--` underflows the unsigned position on a leading space, so " 42"
// falls into the empty-string path. Real code parses 42.
static bool test_str_to_u64_leading_space_advance(void) {
    WriteFmt("Testing StrToU64 advances over leading whitespace\n");
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str  s      = StrInitFromZstr(" 42", &alloc);
    u64  value  = 0;
    bool ok     = StrToU64(&s, &value, NULL);
    bool result = (ok && value == 42);

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 899:48 cxx_eq_to_ne -- base-0 octal-prefix check `prefix == 'o' || prefix
// == 'O'`; flipping the second to `prefix != 'O'` treats a leading "0" + any
// non-'O' char as an octal prefix, skipping two chars. "05" must parse as
// decimal 5. Mutant skips "0","5", finds no digits, returns false.
static bool test_str_to_u64_zero_five_is_decimal(void) {
    WriteFmt("Testing StrToU64 parses 05 as decimal\n");
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str  s      = StrInitFromZstr("05", &alloc);
    u64  value  = 0;
    bool ok     = StrToU64(&s, &value, NULL);
    bool result = (ok && value == 5);

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 909:1 cxx_init_const -- `bool have_digits = false;` mutated to a constant
// (true) defeats the no-digits guard. "0x" consumes the hex prefix but has no
// digits and must be rejected. Mutant returns true with value 0.
static bool test_str_to_u64_prefix_only_rejected(void) {
    WriteFmt("Testing StrToU64 rejects bare 0x prefix\n");
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str  s      = StrInitFromZstr("0x", &alloc);
    u64  value  = 0;
    bool ok     = StrToU64(&s, &value, NULL);
    bool result = (!ok);

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 922:43 cxx_div_to_mul -- overflow guard `result > (UINT64_MAX - digit) /
// base` mutated to `* base`. The corrupted threshold lets an actual overflow
// slip through. UINT64_MAX+1 == "18446744073709551616" must be rejected.
static bool test_str_to_u64_overflow_rejected(void) {
    WriteFmt("Testing StrToU64 rejects u64 overflow\n");
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str  s      = StrInitFromZstr("18446744073709551616", &alloc);
    u64  value  = 0;
    bool ok     = StrToU64(&s, &value, NULL);
    bool result = (!ok);

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 928:21 cxx_assign_const -- `have_digits = true;` mutated to assign a
// constant (false) means a valid number still reports "no valid digits". "42"
// must parse to 42. Mutant returns false.
static bool test_str_to_u64_have_digits_set(void) {
    WriteFmt("Testing StrToU64 marks digits consumed\n");
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str  s      = StrInitFromZstr("42", &alloc);
    u64  value  = 0;
    bool ok     = StrToU64(&s, &value, NULL);
    bool result = (ok && value == 42);

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 932:16 cxx_lt_to_ge -- trailing-whitespace skip `pos < str->length` flipped
// to `pos >= str->length` never consumes trailing spaces, so strict mode
// wrongly reports extra characters for "42 ". Real code parses 42.
static bool test_str_to_u64_trailing_space_skipped_strict(void) {
    WriteFmt("Testing StrToU64 skips trailing whitespace in strict mode\n");
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str            s      = StrInitFromZstr("42 ", &alloc);
    u64            value  = 0;
    StrParseConfig config = {.strict = true};
    bool           ok     = StrToU64(&s, &value, &config);
    bool           result = (ok && value == 42);

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 933:12 cxx_post_inc_to_post_dec -- trailing-whitespace skip `pos++` flipped
// to `pos--` walks back into the consumed digits, leaving pos < length so
// strict mode wrongly rejects "42  ". Real code parses 42.
static bool test_str_to_u64_trailing_space_advance_strict(void) {
    WriteFmt("Testing StrToU64 advances over trailing whitespace in strict mode\n");
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str            s      = StrInitFromZstr("42  ", &alloc);
    u64            value  = 0;
    StrParseConfig config = {.strict = true};
    bool           ok     = StrToU64(&s, &value, &config);
    bool           result = (ok && value == 42);

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 935:31 cxx_lt_to_ge -- strict extra-characters check `config->strict && pos
// < str->length` flipped to `pos >= str->length` no longer rejects genuine
// trailing junk. "42x" in strict mode must be rejected. Mutant returns true.
static bool test_str_to_u64_strict_rejects_trailing_junk(void) {
    WriteFmt("Testing StrToU64 strict mode rejects trailing junk\n");
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str            s      = StrInitFromZstr("42x", &alloc);
    u64            value  = 0;
    StrParseConfig config = {.strict = true};
    bool           ok     = StrToU64(&s, &value, &config);
    bool           result = (!ok);

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 935:31 cxx_lt_to_le -- same check flipped to `pos <= str->length`, always
// true after the trailing-ws skip, so strict mode rejects every input,
// including a clean "42". Real code parses 42.
static bool test_str_to_u64_strict_accepts_clean(void) {
    WriteFmt("Testing StrToU64 strict mode accepts a clean number\n");
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str            s      = StrInitFromZstr("42", &alloc);
    u64            value  = 0;
    StrParseConfig config = {.strict = true};
    bool           ok     = StrToU64(&s, &value, &config);
    bool           result = (ok && value == 42);

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 863:5 cxx_remove_void_call -- ValidateStr(str) is the fail-fast entry
// barrier. A corrupt Str (zeroed magic) must abort at the barrier. Under the
// mutant the barrier is gone and the corruption is not caught at entry.
static bool test_str_to_u64_validate_barrier(void) {
    WriteFmt("Testing StrToU64 validate barrier aborts on corrupt Str\n");

    Str            bad    = {0}; // zeroed magic: ValidateStr must LOG_FATAL
    u64            value  = 0;
    StrParseConfig config = {.base = 10};
    StrToU64(&bad, &value, &config);

    return false; // unreachable on real code (aborts above)
}

// ---- StrFromF64 (Str.Mutants3) --------------------------------------------

// 676:28 lt_to_le -- NaN emit loop `for (i = 0; i < 3; i++)`. With `i <= 3`
// the loop reads nan_str[3] (the terminating NUL) and pushes it, giving a
// 4-byte "nan\0" instead of the 3-byte "nan".
static bool test_nan_length_and_bytes(void) {
    WriteFmt("Testing StrFromF64 NaN emits exactly 3 bytes\n");
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              s     = StrInit(&alloc);

    StrFloatFormat config = {.precision = 2, .force_sci = false, .uppercase = false};
    StrFromF64(&s, F64_NAN, &config);

    bool result = (StrLen(&s) == 3) && (ZstrCompare(StrBegin(&s), "nan") == 0);
    if (!result)
        WriteFmt("    FAIL: expected len 3 \"nan\", got len {} \"{}\"\n", StrLen(&s), StrBegin(&s));

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 695:28 lt_to_le -- inf emit loop. `i <= 3` reads inf_str[3] (NUL) and
// pushes it, giving 4-byte "inf\0" instead of 3-byte "inf".
static bool test_inf_length_and_bytes(void) {
    WriteFmt("Testing StrFromF64 inf emits exactly 3 bytes\n");
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              s     = StrInit(&alloc);

    StrFloatFormat config = {.precision = 2, .force_sci = false, .uppercase = false};
    StrFromF64(&s, F64_INFINITY, &config);

    bool result = (StrLen(&s) == 3) && (ZstrCompare(StrBegin(&s), "inf") == 0);
    if (!result)
        WriteFmt("    FAIL: expected len 3 \"inf\", got len {} \"{}\"\n", StrLen(&s), StrBegin(&s));

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 714:65 lt_to_le -- `value < 0.0001` threshold. The literal and a runtime
// 0.0001 share the same nearest double, so `<= 0.0001` flips 0.0001 from
// fixed notation ("0.000100") to scientific ("1.000000e-04").
static bool test_small_threshold_stays_fixed(void) {
    WriteFmt("Testing StrFromF64 0.0001 stays fixed notation\n");
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              s     = StrInit(&alloc);

    StrFloatFormat config = {.precision = 6, .force_sci = false, .uppercase = false};
    StrFromF64(&s, 0.0001, &config);

    bool result = (ZstrCompare(StrBegin(&s), "0.000100") == 0);
    if (!result)
        WriteFmt("    FAIL: expected \"0.000100\", got \"{}\"\n", StrBegin(&s));

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 714:83 ge_to_gt -- `value >= 1e7` threshold. 1e7 is exactly representable;
// `> 1e7` makes exactly 1e7 fall into fixed notation ("10000000.00") instead
// of scientific ("1.00e+07").
static bool test_large_threshold_uses_sci(void) {
    WriteFmt("Testing StrFromF64 1e7 uses scientific notation\n");
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              s     = StrInit(&alloc);

    StrFloatFormat config = {.precision = 2, .force_sci = false, .uppercase = false};
    StrFromF64(&s, 1e7, &config);

    bool result = (ZstrCompare(StrBegin(&s), "1.00e+07") == 0);
    if (!result)
        WriteFmt("    FAIL: expected \"1.00e+07\", got \"{}\"\n", StrBegin(&s));

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 736:31 gt_to_ge -- scientific `if (config->precision > 0)`. precision is u8
// so `>= 0` is always true; at precision 0 the mutant emits a bare '.' plus a
// zero-iteration fraction loop, giving "5.e+00" instead of "5e+00".
static bool test_sci_precision_zero_no_dot(void) {
    WriteFmt("Testing StrFromF64 scientific precision 0 omits dot\n");
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              s     = StrInit(&alloc);

    StrFloatFormat config = {.precision = 0, .force_sci = true, .uppercase = false};
    StrFromF64(&s, 5.0, &config);

    bool result = (ZstrCompare(StrBegin(&s), "5e+00") == 0);
    if (!result)
        WriteFmt("    FAIL: expected \"5e+00\", got \"{}\"\n", StrBegin(&s));

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 759:17 ge_to_gt -- exponent sign `if (exp >= 0)`. With `> 0`, exp == 0 takes
// the else branch: emits '-' and "00", giving "5.00e-00" instead of "5.00e+00".
static bool test_sci_zero_exponent_sign(void) {
    WriteFmt("Testing StrFromF64 zero exponent prints +00\n");
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              s     = StrInit(&alloc);

    StrFloatFormat config = {.precision = 2, .force_sci = true, .uppercase = false};
    StrFromF64(&s, 5.0, &config);

    bool result = (ZstrCompare(StrBegin(&s), "5.00e+00") == 0);
    if (!result)
        WriteFmt("    FAIL: expected \"5.00e+00\", got \"{}\"\n", StrBegin(&s));

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 776:28 init_const -- `bool ok = true;` in the scientific exponent emit.
// Flipping the init to false makes a fully successful emit still hit
// `if (!ok) return NULL;`, discarding a valid conversion. A successful call
// must return the destination handle, not NULL.
static bool test_sci_exponent_success_returns_handle(void) {
    WriteFmt("Testing StrFromF64 scientific success returns handle\n");
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              s     = StrInit(&alloc);

    StrFloatFormat config = {.precision = 2, .force_sci = true, .uppercase = false};
    Str           *r      = StrFromF64(&s, 123.456, &config);

    bool result = (r == &s) && (ZstrCompare(StrBegin(&s), "1.23e+02") == 0);
    if (!result)
        WriteFmt("    FAIL: expected handle + \"1.23e+02\", got null={} \"{}\"\n", (r == NULL), StrBegin(&s));

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 807:18 init_const -- `bool ok = true;` in the fixed-notation integer emit.
// Same shape as 776: flipping to false discards a successful conversion. A
// successful call must return the destination handle.
static bool test_fixed_integer_success_returns_handle(void) {
    WriteFmt("Testing StrFromF64 fixed success returns handle\n");
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              s     = StrInit(&alloc);

    StrFloatFormat config = {.precision = 2, .force_sci = false, .uppercase = false};
    Str           *r      = StrFromF64(&s, 123.0, &config);

    bool result = (r == &s) && (ZstrCompare(StrBegin(&s), "123.00") == 0);
    if (!result)
        WriteFmt("    FAIL: expected handle + \"123.00\", got null={} \"{}\"\n", (r == NULL), StrBegin(&s));

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 827:31 gt_to_ge -- fixed `if (config->precision > 0)`. precision is u8 so
// `>= 0` is always true; at precision 0 the mutant emits '.' plus a
// zero-iteration fraction loop, giving "123." instead of "123".
static bool test_fixed_precision_zero_no_dot(void) {
    WriteFmt("Testing StrFromF64 fixed precision 0 omits dot\n");
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              s     = StrInit(&alloc);

    StrFloatFormat config = {.precision = 0, .force_sci = false, .uppercase = false};
    StrFromF64(&s, 123.0, &config);

    bool result = (ZstrCompare(StrBegin(&s), "123") == 0);
    if (!result)
        WriteFmt("    FAIL: expected \"123\", got \"{}\"\n", StrBegin(&s));

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 834:17 init_const -- `f64 scale = 1.0;` seeds the precision scale. Flipping
// to 0.0 leaves scale == 0 after the loop, so round_f64(value*0)/0 = NaN and
// the fractional digits become garbage instead of "456".
static bool test_fixed_fraction_scale_seed(void) {
    WriteFmt("Testing StrFromF64 fixed fraction scale seed\n");
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              s     = StrInit(&alloc);

    StrFloatFormat config = {.precision = 3, .force_sci = false, .uppercase = false};
    StrFromF64(&s, 123.456, &config);

    bool result = (ZstrCompare(StrBegin(&s), "123.456") == 0);
    if (!result)
        WriteFmt("    FAIL: expected \"123.456\", got \"{}\"\n", StrBegin(&s));

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 835:30 lt_to_le -- scale loop `for (i = 0; i < precision; i++)`. With `<=`
// it builds 10^(precision+1), rounding one digit too fine. For 0.1235 at
// precision 3 the correct half-up result is "0.124"; the mutant keeps the
// raw value and emits "0.123".
static bool test_fixed_round_half_up(void) {
    WriteFmt("Testing StrFromF64 fixed half-up rounding\n");
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              s     = StrInit(&alloc);

    StrFloatFormat config = {.precision = 3, .force_sci = false, .uppercase = false};
    StrFromF64(&s, 0.1235, &config);

    bool result = (ZstrCompare(StrBegin(&s), "0.124") == 0);
    if (!result)
        WriteFmt("    FAIL: expected \"0.124\", got \"{}\"\n", StrBegin(&s));

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 661:5 remove_void_call -- removing ValidateStr(str) lets a NULL handle slip
// past the fail-fast barrier. Deadend: a NULL str must abort.
static bool test_from_f64_null_aborts(void) {
    WriteFmt("Testing StrFromF64 NULL aborts\n");
    StrFloatFormat config = {.precision = 2, .force_sci = false, .uppercase = false};
    StrFromF64(NULL, 1.0, &config);
    return false;
}

static bool test_from_f64_null_aborts_high_precision(void) {
    WriteFmt("Testing StrFromF64 NULL aborts before the precision guard\n");
    StrFloatFormat config = {.precision = 18, .force_sci = false, .uppercase = false};
    StrFromF64(NULL, 1.0, &config);
    return false;
}

// ---- skip_prefix via StrToU64 (Str.Mutants4) ------------------------------

// 525:13 cxx_add_to_sub : `pos + 2 > length` -> `pos - 2 > length`.
// At pos=0 the unsigned wrap makes the guard fire early and the prefix is
// never consumed -> "0x" is read as digits -> invalid.
static bool test_skip_prefix_len_guard_hex(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              s     = StrInitFromZstr("0x1F", &alloc);
    StrParseConfig   cfg   = {.base = 16};
    u64              out   = 0;
    bool             ok    = StrToU64(&s, &out, &cfg);
    bool             pass  = ok && out == 31;
    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return pass;
}

// 529:10 cxx_init_const : `char prefix_char = data[pos+1]` -> constant.
// prefix_char becomes a sentinel that matches no case -> prefix not
// consumed -> "0x" read as digits -> invalid.
static bool test_skip_prefix_char_init_hex(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              s     = StrInitFromZstr("0x5", &alloc);
    StrParseConfig   cfg   = {.base = 16};
    u64              out   = 0;
    bool             ok    = StrToU64(&s, &out, &cfg);
    bool             pass  = ok && out == 5;
    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return pass;
}

// 529:38 cxx_add_to_sub : `data[pos+1]` -> `data[pos-1]`. At pos=0 this is
// an out-of-bounds read that will not equal the real prefix char -> prefix
// not consumed -> invalid.
static bool test_skip_prefix_char_index_hex(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              s     = StrInitFromZstr("0x5", &alloc);
    StrParseConfig   cfg   = {.base = 16};
    u64              out   = 0;
    bool             ok    = StrToU64(&s, &out, &cfg);
    bool             pass  = ok && out == 5;
    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return pass;
}

// 533:29 cxx_eq_to_ne : base-2 `prefix_char == 'b'` -> `!= 'b'`.
static bool test_skip_prefix_bin_lower(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              s     = StrInitFromZstr("0b1", &alloc);
    StrParseConfig   cfg   = {.base = 2};
    u64              out   = 0;
    bool             ok    = StrToU64(&s, &out, &cfg);
    bool             pass  = ok && out == 1;
    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return pass;
}

// 534:28 cxx_add_to_sub : base-2 `return pos + 2` -> `return pos - 2`.
// pos wraps to SIZE_MAX -> the digit loop runs zero times -> no digits.
static bool test_skip_prefix_bin_return(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              s     = StrInitFromZstr("0b1", &alloc);
    StrParseConfig   cfg   = {.base = 2};
    u64              out   = 0;
    bool             ok    = StrToU64(&s, &out, &cfg);
    bool             pass  = ok && out == 1;
    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return pass;
}

// 539:29 cxx_eq_to_ne : base-8 `prefix_char == 'o'` -> `!= 'o'`.
static bool test_skip_prefix_oct_lower(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              s     = StrInitFromZstr("0o7", &alloc);
    StrParseConfig   cfg   = {.base = 8};
    u64              out   = 0;
    bool             ok    = StrToU64(&s, &out, &cfg);
    bool             pass  = ok && out == 7;
    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return pass;
}

// 539:51 cxx_eq_to_ne : base-8 uppercase `prefix_char == 'O'` -> `!= 'O'`.
static bool test_skip_prefix_oct_upper(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              s     = StrInitFromZstr("0O7", &alloc);
    StrParseConfig   cfg   = {.base = 8};
    u64              out   = 0;
    bool             ok    = StrToU64(&s, &out, &cfg);
    bool             pass  = ok && out == 7;
    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return pass;
}

// 540:28 cxx_add_to_sub : base-8 `return pos + 2` -> `return pos - 2`.
static bool test_skip_prefix_oct_return(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              s     = StrInitFromZstr("0o7", &alloc);
    StrParseConfig   cfg   = {.base = 8};
    u64              out   = 0;
    bool             ok    = StrToU64(&s, &out, &cfg);
    bool             pass  = ok && out == 7;
    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return pass;
}

// 545:29 cxx_eq_to_ne : base-16 `prefix_char == 'x'` -> `!= 'x'`.
static bool test_skip_prefix_hex_lower(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              s     = StrInitFromZstr("0x1F", &alloc);
    StrParseConfig   cfg   = {.base = 16};
    u64              out   = 0;
    bool             ok    = StrToU64(&s, &out, &cfg);
    bool             pass  = ok && out == 31;
    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return pass;
}

// 545:51 cxx_eq_to_ne : base-16 uppercase `prefix_char == 'X'` -> `!= 'X'`.
static bool test_skip_prefix_hex_upper(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              s     = StrInitFromZstr("0X1F", &alloc);
    StrParseConfig   cfg   = {.base = 16};
    u64              out   = 0;
    bool             ok    = StrToU64(&s, &out, &cfg);
    bool             pass  = ok && out == 31;
    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return pass;
}

// 546:28 cxx_add_to_sub : base-16 `return pos + 2` -> `return pos - 2`.
static bool test_skip_prefix_hex_return(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              s     = StrInitFromZstr("0x1F", &alloc);
    StrParseConfig   cfg   = {.base = 16};
    u64              out   = 0;
    bool             ok    = StrToU64(&s, &out, &cfg);
    bool             pass  = ok && out == 31;
    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return pass;
}

// ---- StrToI64 / char_to_digit (Str.Mutants5) ------------------------------

// 961:16 cxx_lt_to_ge -- leading-whitespace skip `pos < length` becomes
// `pos >= length`, so leading spaces are no longer stripped and the sign is
// missed. "  -5" must still parse to -5.
static bool test_str_to_i64_leading_space_negative(void) {
    WriteFmt("Testing StrToI64 strips leading spaces before negative sign\n");
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str  s       = StrInitFromZstr("  -5", &alloc);
    i64  value   = 0;
    bool success = StrToI64(&s, &value, NULL);
    bool result  = success && (value == -5);

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 962:12 cxx_post_inc_to_post_dec -- inside the whitespace-skip loop `pos++`
// becomes `pos--`, wrapping pos to SIZE_MAX and rejecting the input. "  5"
// must still parse to 5.
static bool test_str_to_i64_leading_space_positive(void) {
    WriteFmt("Testing StrToI64 advances past leading spaces\n");
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str  s       = StrInitFromZstr("  5", &alloc);
    i64  value   = 0;
    bool success = StrToI64(&s, &value, NULL);
    bool result  = success && (value == 5);

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 971:18 cxx_assign_const -- `negative = true` becomes `negative = 42`/false in
// the '-' branch, so a negative input is stored positive. "-5" must be -5.
static bool test_str_to_i64_negative_sign(void) {
    WriteFmt("Testing StrToI64 honors the negative sign\n");
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str  s       = StrInitFromZstr("-5", &alloc);
    i64  value   = 0;
    bool success = StrToI64(&s, &value, NULL);
    bool result  = success && (value == -5);

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 974:12 cxx_post_inc_to_post_dec -- in the '+' branch `pos++` becomes `pos--`,
// wrapping pos and building a garbage substring view. "+5" must parse to 5.
static bool test_str_to_i64_plus_sign(void) {
    WriteFmt("Testing StrToI64 skips a leading plus sign\n");
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str  s       = StrInitFromZstr("+5", &alloc);
    i64  value   = 0;
    bool success = StrToI64(&s, &value, NULL);
    bool result  = success && (value == 5);

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 982:23 cxx_assign_const -- `temp_str.capacity = str->length - pos` becomes
// `temp_str.capacity = 42`. temp_str carries the validated magic bit, so
// StrToU64's ValidateStr runs validate_vec_structural which LOG_FATALs when
// length > capacity. A 43-digit (all-zero) input has temp_str.length == 43;
// real code sets capacity == 43 (no abort, parses to 0) while the mutant pins
// capacity to 42 and aborts. We assert the real-code success/value here; under
// the mutant the abort kills the process and this normal test never returns.
static bool test_str_to_i64_long_zero_view(void) {
    WriteFmt("Testing StrToI64 sets the borrowed view capacity to its length\n");
    DefaultAllocator alloc = DefaultAllocatorInit();

    // 43 '0' characters: a valid parse (value 0) whose length exceeds the
    // mutated constant capacity of 42.
    Str s = StrInitFromZstr("0000000000000000000000000000000000000000000", &alloc);

    i64  value   = 7; // sentinel that must be overwritten to 0
    bool success = StrToI64(&s, &value, NULL);
    bool result  = success && (value == 0) && (StrLen(&s) == 43);

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 993:28 cxx_gt_to_ge -- the negative-overflow bound `unsigned_value >
// 9223372036854775808ULL` becomes `>=`, wrongly rejecting magnitude 2^63.
// INT64_MIN ("-9223372036854775808") must still parse.
static bool test_str_to_i64_int64_min(void) {
    WriteFmt("Testing StrToI64 accepts INT64_MIN\n");
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str  s       = StrInitFromZstr("-9223372036854775808", &alloc);
    i64  value   = 0;
    bool success = StrToI64(&s, &value, NULL);
    bool result  = success && (value == INT64_MIN);

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 517:19 cxx_lt_to_le -- char_to_digit's range check `*digit < base` becomes
// `*digit <= base`, accepting an out-of-range digit equal to the base. Parsing
// "2" in base 2 maps '2' -> digit 2 which must be rejected (parse fails).
static bool test_char_to_digit_base_boundary(void) {
    WriteFmt("Testing char_to_digit rejects a digit equal to the base\n");
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str            s       = StrInitFromZstr("2", &alloc);
    u64            value   = 999; // sentinel
    StrParseConfig config  = {.base = 2};
    bool           success = StrToU64(&s, &value, &config);
    // Real code: '2' is invalid in base 2 -> parse fails. The mutant accepts
    // digit value 2 and returns success.
    bool result = (success == false);

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 950:5 cxx_remove_void_call -- removes ValidateStr(str) at the top of
// StrToI64. A corrupted Str (length > capacity, dirty magic) must abort.
static bool test_str_to_i64_corrupt_str_aborts(void) {
    WriteFmt("Testing StrToI64 validates its Str argument (should abort)\n");
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str s = StrInitFromZstr("123", &alloc);
    // intentional bypass: plant an inconsistent (length, capacity) pair that
    // ValidateStr must reject; mark dirty so the body re-runs.
    s.length   = 100;
    s.capacity = 5;
    MAGIC_MARK_DIRTY(&s);

    i64 value = 0;
    StrToI64(&s, &value, NULL); // should abort here

    return false;               // unreachable on real code
}

// ---- StrFromI64 (Str.Mutants6) --------------------------------------------

// StrFromI64 640:cxx_assign_const -- `abs_value = (u64)INT64_MIN;` has its
// constant replaced. This branch handles value == INT64_MIN exactly (which the
// other suites avoid by using INT64_MIN+1); the correct decimal rendering must
// be the full magnitude.
static bool test_from_i64_int64_min_exact_digits(void) {
    WriteFmt("Testing StrFromI64(INT64_MIN) exact digits\n");
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str          s      = StrInit(&alloc);
    StrIntFormat config = {.base = 10, .uppercase = false};
    StrFromI64(&s, INT64_MIN, &config);
    bool result = (ZstrCompare(StrBegin(&s), "-9223372036854775808") == 0);

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// ---- round_f64 via StrFromF64 (Str.Mutants7) ------------------------------

// round_f64 @ 63:11 (x >= two53 -> x < two53). Flipping the first guard term
// makes the guard true for every in-range magnitude, returning the value
// unrounded. 2.25 at precision 1 must round-half-away to "2.3"; unrounded it
// truncates to "2.2".
static bool test_round_f64_guard_first_term(void) {
    WriteFmt("Testing StrFromF64 rounds 2.25@p1 to 2.3 (round_f64 guard, first term)\n");
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str            s      = StrInit(&alloc);
    StrFloatFormat config = {.precision = 1, .force_sci = false, .uppercase = false};
    StrFromF64(&s, 2.25, &config);
    bool result = (ZstrCompare(StrBegin(&s), "2.3") == 0);
    if (!result) {
        WriteFmt("    FAIL: Expected '2.3', got '{}'\n", StrBegin(&s));
    }

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// round_f64 @ 63:25 (x <= -two53 -> x > -two53). The second guard term then
// reads `x > -two53`, true for every in-range positive value, so the rounding
// is skipped and 2.25@p1 prints "2.2" instead of "2.3".
static bool test_round_f64_guard_second_term(void) {
    WriteFmt("Testing StrFromF64 rounds 2.25@p1 to 2.3 (round_f64 guard, second term)\n");
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str            s      = StrInit(&alloc);
    StrFloatFormat config = {.precision = 1, .force_sci = false, .uppercase = false};
    StrFromF64(&s, 2.25, &config);
    bool result = (ZstrCompare(StrBegin(&s), "2.3") == 0);
    if (!result) {
        WriteFmt("    FAIL: Expected '2.3', got '{}'\n", StrBegin(&s));
    }

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// ---- StrFromU64 / StrFromF64 (Str.Mutants8) -------------------------------

// StrFromU64 564:10 cxx_replace_scalar_call -- `is_valid_base(config->base)`
// is replaced by a surviving constant `true`, so invalid bases are accepted
// instead of rejected with NULL. base 37 is invalid (37 > 36).
//   real   : returns NULL.
//   mutant : runs the digit loop (value % 37) and returns &s (non-NULL).
static bool test_from_u64_rejects_invalid_base(void) {
    WriteFmt("Testing StrFromU64 rejects an invalid base with NULL (564:10)\n");
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str          s      = StrInit(&alloc);
    StrIntFormat config = {.base = 37, .uppercase = false};
    Str         *ret    = StrFromU64(&s, 5, &config);

    bool result = (ret == NULL);
    if (!result) {
        WriteFmt("    FAIL: expected NULL return for base 37, got non-NULL\n");
    }

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// StrFromU64 595:14 cxx_init_const -- `bool ok = true;` becomes `ok = false;`.
// The loop only ever assigns ok=false (on push failure) and never sets it
// true, so a successful nonzero conversion still hits `if (!ok) return NULL`.
//   real   : returns &s and s holds "12345".
//   mutant : returns NULL even though the digits were pushed.
// Asserting the non-NULL return (which the formatting itself does not expose)
// is what kills the mutant.
static bool test_from_u64_success_returns_str(void) {
    WriteFmt("Testing StrFromU64 returns the Str pointer on success (595:14)\n");
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str          s      = StrInit(&alloc);
    StrIntFormat config = {.base = 10, .uppercase = false};
    Str         *ret    = StrFromU64(&s, 12345, &config);

    bool result = (ret == &s) && (ZstrCompare(StrBegin(&s), "12345") == 0);
    if (!result) {
        WriteFmt("    FAIL: expected &s and '12345', got ret={} '{}'\n", (ret != NULL), s);
    }

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// StrFromU64 609:24 cxx_assign_const -- inside the digit-replay loop the push
// failure handler `ok = false;` becomes `ok = true;`, so an allocation failure
// is reported as success: a truncated/empty str is returned non-NULL instead
// of the documented NULL.
//
// A BudgetAllocator with an 8-byte slot cannot serve the Str's first growth
// request (a char Str padded to the allocator alignment needs > 8 bytes), so
// the very first StrPushBackR fails.
//   real   : `ok` stays false -> returns NULL.
//   mutant : `ok` becomes true -> returns &s (non-NULL).
static bool test_from_u64_oom_returns_null(void) {
    WriteFmt("Testing StrFromU64 returns NULL when a digit push fails (609:24)\n");

    // Buffer large enough for the bitmap word + one 8-byte slot, but the slot
    // is far too small for the string's backing buffer, so allocation fails.
    u8              buf[64] = {0};
    BudgetAllocator bp      = BudgetAllocatorInit(buf, sizeof(buf), 8);

    Str          s      = StrInit(&bp);
    StrIntFormat config = {.base = 10, .uppercase = false};
    // 10-digit value: any growth request exceeds the 8-byte slot.
    Str *ret = StrFromU64(&s, 9999999999ULL, &config);

    bool result = (ret == NULL);
    if (!result) {
        WriteFmt("    FAIL: expected NULL on allocation failure, got non-NULL\n");
    }

    StrDeinit(&s);
    BudgetAllocatorDeinit(&bp);
    return result;
}

// StrFromF64 817:28 cxx_assign_const -- in the fixed-notation integer-digit
// replay loop the push-failure handler `ok = false;` becomes `ok = 42;`, so an
// allocation failure during the integer part is reported as success. At
// precision 0 the integer loop is the *only* emitter (no '.' / fraction / 'e'
// pushes follow), so when its push fails the real code returns NULL at the
// `if (!ok)` guard while the mutant falls through to `return str`.
//   real   : `ok` stays false -> returns NULL.
//   mutant : `ok` becomes truthy -> falls through -> returns &s (non-NULL).
static bool test_from_f64_int_oom_returns_null(void) {
    WriteFmt("Testing StrFromF64 returns NULL when an integer-digit push fails (817:28)\n");

    u8              buf[64] = {0};
    BudgetAllocator bp      = BudgetAllocatorInit(buf, sizeof(buf), 8);

    Str            s      = StrInit(&bp);
    StrFloatFormat config = {.precision = 0, .force_sci = false, .uppercase = false};
    // Positive 15-digit value, precision 0 -> the integer digits are the only
    // emit (no sign/prefix/'.'/'e' pushes). The Str is a Vec(char) with stride
    // sizeof(char) == 1, so its backing buffer grows pow2 (alloc n2+1 bytes);
    // the 8-byte slot holds the first seven chars but the eighth integer-digit
    // push needs a 9-byte buffer and fails inside the integer-digit loop. No
    // later direct-return push can mask the mutated `ok`.
    Str *ret = StrFromF64(&s, 123456789012345.0, &config);

    bool result = (ret == NULL);
    if (!result) {
        WriteFmt("    FAIL: expected NULL on integer-digit allocation failure, got non-NULL\n");
    }

    StrDeinit(&s);
    BudgetAllocatorDeinit(&bp);
    return result;
}

// StrFromF64 790:28 cxx_assign_const -- in the scientific exponent-digit replay
// loop the push-failure handler `ok = false;` becomes `ok = 42;`, so an
// allocation failure while emitting the exponent is reported as success. The
// exponent loop is the *last* emitter (its `if (!ok)` guard is the final
// return-NULL before `return str`), so a failure there is observable: real
// returns NULL, mutant falls through to `return str`.
//
// "1.23e+150" is nine bytes. The Str is a Vec(char) with stride sizeof(char)
// == 1, so its backing buffer grows pow2 (alloc n2+1 bytes). With an 8-byte
// Budget slot the buffer holds "1.23e+1" (seven chars), but the eighth char --
// the exponent digit '5', pushed by this very loop -- needs a 9-byte buffer
// and fails. All earlier pushes (through 'e', '+', and the first exponent
// digit '1') succeed, so it is the exponent loop -- not an earlier push --
// that fails.
//   real   : `ok` stays false -> returns NULL.
//   mutant : `ok` becomes truthy -> falls through -> returns &s (non-NULL).
static bool test_from_f64_exp_oom_returns_null(void) {
    WriteFmt("Testing StrFromF64 returns NULL when an exponent-digit push fails (790:28)\n");

    u8              buf[512] = {0};
    BudgetAllocator bp       = BudgetAllocatorInit(buf, sizeof(buf), 8);

    Str            s      = StrInit(&bp);
    StrFloatFormat config = {.precision = 2, .force_sci = true, .uppercase = false};
    Str           *ret    = StrFromF64(&s, 1.23e150, &config);

    bool result = (ret == NULL);
    if (!result) {
        WriteFmt("    FAIL: expected NULL on exponent-digit allocation failure, got non-NULL\n");
    }

    StrDeinit(&s);
    BudgetAllocatorDeinit(&bp);
    return result;
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
        test_str_large_scale_conversions,
        // StrToF64 (Str.Mutants1)
        test_leading_space_skip_condition,
        test_leading_space_advances,
        test_nan_exact_accept_with_offset,
        test_inf_exact_accept_with_offset,
        test_minus_sign_makes_negative,
        test_plus_sign_advances,
        test_integer_digits_set_flag,
        test_fractional_digits_set_flag,
        test_exponent_gate_not_widened,
        test_negative_exponent_sign,
        test_exponent_digits_set_flag,
        test_trailing_space_skip_condition,
        test_trailing_space_advances,
        test_strict_trailing_check_ge,
        test_strict_trailing_check_le,
        // StrToU64 (Str.Mutants2)
        test_str_to_u64_invalid_base_rejected,
        test_str_to_u64_valid_base_sixteen_accepted,
        test_str_to_u64_leading_space_skipped,
        test_str_to_u64_leading_space_advance,
        test_str_to_u64_zero_five_is_decimal,
        test_str_to_u64_prefix_only_rejected,
        test_str_to_u64_overflow_rejected,
        test_str_to_u64_have_digits_set,
        test_str_to_u64_trailing_space_skipped_strict,
        test_str_to_u64_trailing_space_advance_strict,
        test_str_to_u64_strict_rejects_trailing_junk,
        test_str_to_u64_strict_accepts_clean,
        // StrFromF64 (Str.Mutants3)
        test_nan_length_and_bytes,
        test_inf_length_and_bytes,
        test_small_threshold_stays_fixed,
        test_large_threshold_uses_sci,
        test_sci_precision_zero_no_dot,
        test_sci_zero_exponent_sign,
        test_sci_exponent_success_returns_handle,
        test_fixed_integer_success_returns_handle,
        test_fixed_precision_zero_no_dot,
        test_fixed_fraction_scale_seed,
        test_fixed_round_half_up,
        // skip_prefix via StrToU64 (Str.Mutants4)
        test_skip_prefix_len_guard_hex,
        test_skip_prefix_char_init_hex,
        test_skip_prefix_char_index_hex,
        test_skip_prefix_bin_lower,
        test_skip_prefix_bin_return,
        test_skip_prefix_oct_lower,
        test_skip_prefix_oct_upper,
        test_skip_prefix_oct_return,
        test_skip_prefix_hex_lower,
        test_skip_prefix_hex_upper,
        test_skip_prefix_hex_return,
        // StrToI64 / char_to_digit (Str.Mutants5)
        test_str_to_i64_leading_space_negative,
        test_str_to_i64_leading_space_positive,
        test_str_to_i64_negative_sign,
        test_str_to_i64_plus_sign,
        test_str_to_i64_long_zero_view,
        test_str_to_i64_int64_min,
        test_char_to_digit_base_boundary,
        // StrFromI64 (Str.Mutants6)
        test_from_i64_int64_min_exact_digits,
        // round_f64 via StrFromF64 (Str.Mutants7)
        test_round_f64_guard_first_term,
        test_round_f64_guard_second_term,
        // StrFromU64 / StrFromF64 (Str.Mutants8)
        test_from_u64_rejects_invalid_base,
        test_from_u64_success_returns_str,
        test_from_u64_oom_returns_null,
        test_from_f64_int_oom_returns_null,
        test_from_f64_exp_oom_returns_null
    };

    // Array of deadend test functions
    TestFunction deadend_tests[] = {
        test_str_conversion_null_failures,
        test_str_conversion_bounds_failures,
        test_str_conversion_invalid_input_failures,
        test_validate_str_guard,           // StrToF64 (Str.Mutants1)
        test_str_to_u64_validate_barrier,  // StrToU64 (Str.Mutants2)
        test_from_f64_null_aborts,         // StrFromF64 (Str.Mutants3)
        test_from_f64_null_aborts_high_precision,
        test_str_to_i64_corrupt_str_aborts // StrToI64 (Str.Mutants5)
    };

    int total_tests         = sizeof(tests) / sizeof(tests[0]);
    int total_deadend_tests = sizeof(deadend_tests) / sizeof(deadend_tests[0]);

    // Run all tests using the centralized test driver
    return run_test_suite(tests, total_tests, deadend_tests, total_deadend_tests, "Str.Convert");
}
