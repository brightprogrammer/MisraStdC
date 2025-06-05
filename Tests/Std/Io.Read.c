#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Container/BitVec.h>
#include <Misra/Std/Io.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Memory.h>
#include <stdio.h>
#include <math.h> // For fabs()
#include <string.h>
#include <Misra/Types.h>

// Include test utilities
#include "../Util/TestRunner.h"

// Define epsilon for float comparisons
#define FLOAT_EPSILON  1e-6
#define DOUBLE_EPSILON 1e-12

// Helper function for comparing floats with epsilon
static bool float_equals(float a, float b) {
    return fabs(a - b) < FLOAT_EPSILON;
}

// Helper function for comparing doubles with epsilon
static bool double_equals(double a, double b) {
    return fabs(a - b) < DOUBLE_EPSILON;
}

// Function prototypes
bool test_integer_decimal_reading(void);
bool test_integer_hex_reading(void);
bool test_integer_binary_reading(void);
bool test_integer_octal_reading(void);
bool test_float_basic_reading(void);
bool test_float_scientific_reading(void);
bool test_string_reading(void);
bool test_multiple_arguments_reading(void);
bool test_error_handling_reading(void);
bool test_character_ordinal_reading(void);
bool test_string_case_conversion_reading(void);
bool test_bitvec_reading(void);

// Test decimal integer reading
bool test_integer_decimal_reading(void) {
    printf("Testing integer decimal reading\n");

    bool success = true;

    // Test signed integers
    i8 i8_val = 0;
    StrReadFmt("-42", "{}", FMT(i8_val));
    success = success && (i8_val == -42);

    i16 i16_val = 0;
    StrReadFmt("-1234", "{}", FMT(i16_val));
    success = success && (i16_val == -1234);

    i32 i32_val = 0;
    StrReadFmt("-123456", "{}", FMT(i32_val));
    success = success && (i32_val == -123456);

    i64 i64_val = 0;
    StrReadFmt("-1234567890", "{}", FMT(i64_val));
    success = success && (i64_val == -1234567890LL);

    // Test unsigned integers
    u8 u8_val = 0;
    StrReadFmt("42", "{}", FMT(u8_val));
    success = success && (u8_val == 42);

    u16 u16_val = 0;
    StrReadFmt("1234", "{}", FMT(u16_val));
    success = success && (u16_val == 1234);

    u32 u32_val = 0;
    StrReadFmt("123456", "{}", FMT(u32_val));
    success = success && (u32_val == 123456);

    u64 u64_val = 0;
    StrReadFmt("1234567890", "{}", FMT(u64_val));
    success = success && (u64_val == 1234567890ULL);

    // Test edge cases
    i8_val = 0;
    StrReadFmt("127", "{}", FMT(i8_val));
    success = success && (i8_val == 127);

    i8_val = 0;
    StrReadFmt("-128", "{}", FMT(i8_val));
    success = success && (i8_val == -128);

    u8_val = 0;
    StrReadFmt("255", "{}", FMT(u8_val));
    success = success && (u8_val == 255);

    u8_val = 0;
    StrReadFmt("0", "{}", FMT(u8_val));
    success = success && (u8_val == 0);

    // Test leading zeros
    i32_val = 0;
    StrReadFmt("000042", "{}", FMT(i32_val));
    success = success && (i32_val == 42);

    i32_val = 0;
    StrReadFmt("-000042", "{}", FMT(i32_val));
    success = success && (i32_val == -42);

    // Test whitespace handling
    i32_val = 0;
    StrReadFmt("   42", "{}", FMT(i32_val));
    success = success && (i32_val == 42);

    i32_val = 0;
    StrReadFmt("42   ", "{}", FMT(i32_val));
    success = success && (i32_val == 42);

    i32_val = 0;
    StrReadFmt("  42  ", "{}", FMT(i32_val));
    success = success && (i32_val == 42);

    return success;
}

// Test hexadecimal integer reading
bool test_integer_hex_reading(void) {
    printf("Testing integer hexadecimal reading\n");

    bool success = true;

    u32 val = 0;
    StrReadFmt("0xdeadbeef", "{}", FMT(val));
    success = success && (val == 0xdeadbeef);

    val = 0;
    StrReadFmt("0xDEADBEEF", "{}", FMT(val));
    success = success && (val == 0xDEADBEEF);

    // Test hex edge cases
    val = 0;
    StrReadFmt("0x0", "{}", FMT(val));
    success = success && (val == 0);

    val = 0;
    StrReadFmt("0xf", "{}", FMT(val));
    success = success && (val == 0xf);

    val = 0;
    StrReadFmt("0xaBcDeF", "{}", FMT(val));
    success = success && (val == 0xabcdef);

    return success;
}

// Test binary integer reading
bool test_integer_binary_reading(void) {
    printf("Testing integer binary reading\n");

    bool success = true;

    i8 val = 0;
    StrReadFmt("0b101010", "{}", FMT(val));
    success = success && (val == 42);

    // Test binary edge cases
    val = 0;
    StrReadFmt("0b0", "{}", FMT(val));
    success = success && (val == 0);

    val = 0;
    StrReadFmt("0b1", "{}", FMT(val));
    success = success && (val == 1);

    return success;
}

// Test octal integer reading
bool test_integer_octal_reading(void) {
    printf("Testing integer octal reading\n");

    bool success = true;

    i32 val = 0;
    StrReadFmt("0o755", "{}", FMT(val));
    success = success && (val == 0755);

    val = 0;
    StrReadFmt("755", "{}", FMT(val));
    success = success && (val == 755);

    // Test octal edge cases
    val = 0;
    StrReadFmt("0o0", "{}", FMT(val));
    success = success && (val == 0);

    val = 0;
    StrReadFmt("0o7", "{}", FMT(val));
    success = success && (val == 7);

    return success;
}

// Test basic float reading
bool test_float_basic_reading(void) {
    printf("Testing basic float reading\n");

    bool success = true;

    // Test basic float values
    f32 f32_val = 0.0f;
    StrReadFmt("3.14159", "{}", FMT(f32_val));
    success = success && float_equals(f32_val, 3.14159f);

    f64 f64_val = 0.0;
    StrReadFmt("3.14159265359", "{}", FMT(f64_val));
    success = success && double_equals(f64_val, 3.14159265359);

    // Test float edge cases
    f64_val = 1.0;
    StrReadFmt("0.0", "{}", FMT(f64_val));
    success = success && double_equals(f64_val, 0.0);

    f64_val = 1.0;
    StrReadFmt("-0.0", "{}", FMT(f64_val));
    // Special case for -0.0 which compares equal to 0.0 but has different bit pattern
    // We'll just check if it's close to zero
    success = success && double_equals(f64_val, 0.0);

    f64_val = 0.0;
    StrReadFmt("42.0", "{}", FMT(f64_val));
    success = success && double_equals(f64_val, 42.0);

    f64_val = 0.0;
    StrReadFmt("0.42", "{}", FMT(f64_val));
    success = success && double_equals(f64_val, 0.42);

    return success;
}

// Test scientific notation reading
bool test_float_scientific_reading(void) {
    printf("Testing scientific notation reading\n");

    bool success = true;

    f64 val = 0.0;
    StrReadFmt("1.23e4", "{}", FMT(val));
    success = success && double_equals(val, 12300.0);

    val = 0.0;
    StrReadFmt("1.23E4", "{}", FMT(val));
    success = success && double_equals(val, 12300.0);

    val = 0.0;
    StrReadFmt("1.23e+4", "{}", FMT(val));
    success = success && double_equals(val, 12300.0);

    val = 0.0;
    StrReadFmt("1.23e-4", "{}", FMT(val));
    success = success && double_equals(val, 0.000123);

    // Test scientific notation edge cases
    val = 0.0;
    StrReadFmt("1.0e0", "{}", FMT(val));
    success = success && double_equals(val, 1.0);

    val = 0.0;
    StrReadFmt("1.0E-0", "{}", FMT(val));
    success = success && double_equals(val, 1.0);

    val = 0.0;
    StrReadFmt("1.0e+0", "{}", FMT(val));
    success = success && double_equals(val, 1.0);

    return success;
}

// Test string reading
bool test_string_reading(void) {
    printf("Testing string reading\n");

    bool success = true;

    // Test basic string reading
    Str s = StrInit();
    StrReadFmt("Hello", "{}", FMT(s));

    Str expected = StrInitFromZstr("Hello");
    success      = success && (StrCmp(&s, &expected) == 0);
    StrDeinit(&expected);
    StrClear(&s);

    // Test quoted string reading
    StrReadFmt("\"Hello, World!\"", "{}", FMT(s));

    expected = StrInitFromZstr("Hello, World!");
    success  = success && (StrCmp(&s, &expected) == 0);
    StrDeinit(&expected);

    StrDeinit(&s);

    return success;
}

// Test reading multiple arguments
bool test_multiple_arguments_reading(void) {
    printf("Testing multiple arguments reading\n");

    bool success = true;

    i32 num  = 0;
    Str name = StrInit();
    StrReadFmt("Count: 42, Name: Alice", "Count: {}, Name: {}", FMT(num), FMT(name));

    success = success && (num == 42);

    Str expected = StrInitFromZstr("Alice");
    success      = success && (StrCmp(&name, &expected) == 0);
    StrDeinit(&expected);
    StrClear(&name);

    // Test with different order
    f64 val = 0.0;
    StrReadFmt("Value: 3.14, Name: Bob", "Value: {}, Name: {}", FMT(val), FMT(name));

    success = success && double_equals(val, 3.14);

    expected = StrInitFromZstr("Bob");
    success  = success && (StrCmp(&name, &expected) == 0);
    StrDeinit(&expected);

    StrDeinit(&name);

    return success;
}

// Test error handling
bool test_error_handling_reading(void) {
    printf("Testing error handling for reading\n");

    // For error handling tests, we'll just verify that the variables don't change
    // when invalid input is provided

    bool success = true;

    // Test mismatched format
    i32 num = 42;
    StrReadFmt("Count: forty-two", "Count: {}", FMT(num));
    // The value should remain unchanged since the parsing should fail
    success = success && (num == 42);

    // Test invalid integer
    num = 42;
    StrReadFmt("Count: abc", "Count: {}", FMT(num));
    success = success && (num == 42);

    // Test overflow
    i8 small = 42;
    StrReadFmt("Value: 1000", "Value: {}", FMT(small));
    success = success && (small == 42);

    return success;
}

// Test character ordinal reading with :c format specifier
bool test_character_ordinal_reading(void) {
    printf("Testing character ordinal reading with :c format specifier\n");

    bool success = true;

    // Test reading single character into u8
    u8 u8_val = 0;
    StrReadFmt("A", "{:c}", FMT(u8_val));
    printf("u8_val = %d, expected = %d, pass = %s\n", u8_val, 'A', (u8_val == 'A') ? "true" : "false");
    success = success && (u8_val == 'A');

    u8_val = 0;
    StrReadFmt("z", "{:c}", FMT(u8_val));
    printf("u8_val = %d, expected = %d, pass = %s\n", u8_val, 'z', (u8_val == 'z') ? "true" : "false");
    success = success && (u8_val == 'z');

    // Test reading single character into signed integers
    i8 i8_val = 0;
    StrReadFmt("B", "{:c}", FMT(i8_val));
    printf("i8_val = %d, expected = %d, pass = %s\n", i8_val, 'B', (i8_val == 'B') ? "true" : "false");
    success = success && (i8_val == 'B');

    i16 i16_val = 0;
    StrReadFmt("C", "{:c}", FMT(i16_val));
    printf("i16_val = %d, expected = %d, pass = %s\n", i16_val, 'C', (i16_val == 'C') ? "true" : "false");
    success = success && (i16_val == 'C');

    i32 i32_val = 0;
    StrReadFmt("D", "{:c}", FMT(i32_val));
    printf("i32_val = %d, expected = %d, pass = %s\n", i32_val, 'D', (i32_val == 'D') ? "true" : "false");
    success = success && (i32_val == 'D');

    i64 i64_val = 0;
    StrReadFmt("E", "{:c}", FMT(i64_val));
    printf("i64_val = %lld, expected = %d, pass = %s\n", i64_val, 'E', (i64_val == 'E') ? "true" : "false");
    success = success && (i64_val == 'E');

    // Test reading single character into unsigned integers
    u16 u16_val = 0;
    StrReadFmt("F", "{:c}", FMT(u16_val));
    printf("u16_val = %d, expected = %d, pass = %s\n", u16_val, 'F', (u16_val == 'F') ? "true" : "false");
    success = success && (u16_val == 'F');

    u32 u32_val = 0;
    StrReadFmt("G", "{:c}", FMT(u32_val));
    printf("u32_val = %d, expected = %d, pass = %s\n", u32_val, 'G', (u32_val == 'G') ? "true" : "false");
    success = success && (u32_val == 'G');

    u64 u64_val = 0;
    StrReadFmt("H", "{:c}", FMT(u64_val));
    printf("u64_val = %llu, expected = %d, pass = %s\n", u64_val, 'H', (u64_val == 'H') ? "true" : "false");
    success = success && (u64_val == 'H');

    // Test reading multiple characters into larger integer types
    // For u16, read 2 characters
    u16_val = 0;
    StrReadFmt("AB", "{:c}", FMT(u16_val));
    bool u16_multi_pass = (ZstrCompareN((const char*)&u16_val, "AB", 2) == 0);
    printf("u16_val multi-char test: comparing memory with 'AB', pass = %s\n", u16_multi_pass ? "true" : "false");
    printf(
        "DEBUG: u16_val bytes: [%d, %d], expected 'AB' bytes: [%d, %d]\n",
        (int)((u8*)&u16_val)[0],
        (int)((u8*)&u16_val)[1],
        (int)'A',
        (int)'B'
    );
    success = success && u16_multi_pass;

    // For i16, read 2 characters
    i16_val = 0;
    StrReadFmt("CD", "{:c}", FMT(i16_val));
    bool i16_multi_pass = (ZstrCompareN((const char*)&i16_val, "CD", 2) == 0);
    printf("i16_val multi-char test: comparing memory with 'CD', pass = %s\n", i16_multi_pass ? "true" : "false");
    success = success && i16_multi_pass;

    // For u32, read up to 4 characters
    u32_val = 0;
    StrReadFmt("EFGH", "{:c}", FMT(u32_val));
    bool u32_multi_pass = (ZstrCompareN((const char*)&u32_val, "EFGH", 4) == 0);
    printf("u32_val multi-char test: comparing memory with 'EFGH', pass = %s\n", u32_multi_pass ? "true" : "false");
    success = success && u32_multi_pass;

    // For i32, read up to 4 characters
    i32_val = 0;
    StrReadFmt("IJKL", "{:c}", FMT(i32_val));
    bool i32_multi_pass = (ZstrCompareN((const char*)&i32_val, "IJKL", 4) == 0);
    printf("i32_val multi-char test: comparing memory with 'IJKL', pass = %s\n", i32_multi_pass ? "true" : "false");
    success = success && i32_multi_pass;

    // For u64, read up to 8 characters
    u64_val = 0;
    StrReadFmt("MNOPQRST", "{:c}", FMT(u64_val));
    bool u64_multi_pass = (ZstrCompareN((const char*)&u64_val, "MNOPQRST", 8) == 0);
    printf("u64_val multi-char test: comparing memory with 'MNOPQRST', pass = %s\n", u64_multi_pass ? "true" : "false");
    success = success && u64_multi_pass;

    // For i64, read up to 8 characters
    i64_val = 0;
    StrReadFmt("UVWXYZab", "{:c}", FMT(i64_val));
    bool i64_multi_pass = (ZstrCompareN((const char*)&i64_val, "UVWXYZab", 8) == 0);
    printf("i64_val multi-char test: comparing memory with 'UVWXYZab', pass = %s\n", i64_multi_pass ? "true" : "false");
    success = success && i64_multi_pass;

    // Test reading characters into float types (should interpret as character ordinals)
    f32 f32_val = 0.0f;
    StrReadFmt("A", "{:c}", FMT(f32_val));
    bool f32_pass = (f32_val == (f32)'A');
    printf("f32_val = %f, expected = %f, pass = %s\n", f32_val, (f32)'A', f32_pass ? "true" : "false");
    success = success && f32_pass;

    f64 f64_val = 0.0;
    StrReadFmt("B", "{:c}", FMT(f64_val));
    bool f64_pass = (f64_val == (f64)'B');
    printf("f64_val = %f, expected = %f, pass = %s\n", f64_val, (f64)'B', f64_pass ? "true" : "false");
    success = success && f64_pass;

    // Test with high ASCII characters
    u8_val = 0;
    StrReadFmt("~", "{:c}", FMT(u8_val));
    bool tilde_pass = (u8_val == '~');
    printf("u8_val = %d, expected = %d (~), pass = %s\n", u8_val, '~', tilde_pass ? "true" : "false");
    success = success && tilde_pass;

    // Test partial reads for larger types with fewer characters
    u32_val = 0;
    StrReadFmt("XY", "{:c}", FMT(u32_val));
    bool xy_pass = (ZstrCompareN((const char*)&u32_val, "XY", 2) == 0);
    printf("u32_val partial test: comparing memory with 'XY', pass = %s\n", xy_pass ? "true" : "false");
    success = success && xy_pass;

    u64_val = 0;
    StrReadFmt("abc", "{:c}", FMT(u64_val));
    bool abc_pass = (ZstrCompareN((const char*)&u64_val, "abc", 3) == 0);
    printf("u64_val partial test: comparing memory with 'abc', pass = %s\n", abc_pass ? "true" : "false");
    success = success && abc_pass;

    // Test that :c has no effect on string types (should work like regular string reading)
    Str str_val = StrInit();
    StrReadFmt("Hello", "{:c}", FMT(str_val));

    Str  expected = StrInitFromZstr("Hello");
    bool str_pass = (StrCmp(&str_val, &expected) == 0);
    printf("str_val test: comparing with 'Hello', pass = %s\n", str_pass ? "true" : "false");
    success = success && str_pass;
    StrDeinit(&expected);
    StrDeinit(&str_val);

    // Test :c with quoted strings (should work like regular string reading)
    str_val = StrInit();
    StrReadFmt("\"World\"", "{:c}", FMT(str_val));

    expected             = StrInitFromZstr("World");
    bool quoted_str_pass = (StrCmp(&str_val, &expected) == 0);
    printf("quoted str_val test: comparing with 'World', pass = %s\n", quoted_str_pass ? "true" : "false");
    success = success && quoted_str_pass;
    StrDeinit(&expected);
    StrDeinit(&str_val);

    printf("Overall success: %s\n", success ? "true" : "false");
    return success;
}

// Test string case conversion with :a and :A format specifiers
bool test_string_case_conversion_reading(void) {
    printf("Testing string case conversion with :a and :A format specifiers\n");

    bool success = true;

    // Test 1: :a (lowercase) conversion
    {
        Str         result = StrInit();
        const char* input  = "Hello World";

        StrReadFmt(input, "{:a}", FMT(result));

        printf("Test 1 - :a (lowercase)\n");
        printf("Input: '%s', Output: '", input);
        for (size_t i = 0; i < result.length; i++) {
            printf("%c", result.data[i]);
        }
        printf("'\n");

        // Should read "hello" (stops at first space)
        Str  expected   = StrInitFromZstr("hello");
        bool test1_pass = (StrCmp(&result, &expected) == 0);
        printf("Expected: 'hello', Pass: %s\n\n", test1_pass ? "true" : "false");
        success = success && test1_pass;

        StrDeinit(&expected);
        StrDeinit(&result);
    }

    // Test 2: :A (uppercase) conversion
    {
        Str         result = StrInit();
        const char* input  = "hello world";

        StrReadFmt(input, "{:A}", FMT(result));

        printf("Test 2 - :A (uppercase)\n");
        printf("Input: '%s', Output: '", input);
        for (size_t i = 0; i < result.length; i++) {
            printf("%c", result.data[i]);
        }
        printf("'\n");

        // Should read "HELLO" (stops at first space)
        Str  expected   = StrInitFromZstr("HELLO");
        bool test2_pass = (StrCmp(&result, &expected) == 0);
        printf("Expected: 'HELLO', Pass: %s\n\n", test2_pass ? "true" : "false");
        success = success && test2_pass;

        StrDeinit(&expected);
        StrDeinit(&result);
    }

    // Test 3: :a with quoted string
    {
        Str         result = StrInit();
        const char* input  = "\"MiXeD CaSe\"";

        StrReadFmt(input, "{:a}", FMT(result));

        printf("Test 3 - :a with quoted string\n");
        printf("Input: '%s', Output: '", input);
        for (size_t i = 0; i < result.length; i++) {
            printf("%c", result.data[i]);
        }
        printf("'\n");

        // Should read "mixed case" (converts the entire quoted string)
        Str  expected   = StrInitFromZstr("mixed case");
        bool test3_pass = (StrCmp(&result, &expected) == 0);
        printf("Expected: 'mixed case', Pass: %s\n\n", test3_pass ? "true" : "false");
        success = success && test3_pass;

        StrDeinit(&expected);
        StrDeinit(&result);
    }

    // Test 4: :A with quoted string containing special characters
    {
        Str         result = StrInit();
        const char* input  = "\"abc123XYZ\"";

        StrReadFmt(input, "{:A}", FMT(result));

        printf("Test 4 - :A with mixed alphanumeric\n");
        printf("Input: '%s', Output: '", input);
        for (size_t i = 0; i < result.length; i++) {
            printf("%c", result.data[i]);
        }
        printf("'\n");

        // Should read "ABC123XYZ" (only letters are converted, numbers unchanged)
        Str  expected   = StrInitFromZstr("ABC123XYZ");
        bool test4_pass = (StrCmp(&result, &expected) == 0);
        printf("Expected: 'ABC123XYZ', Pass: %s\n\n", test4_pass ? "true" : "false");
        success = success && test4_pass;

        StrDeinit(&expected);
        StrDeinit(&result);
    }

    // Test 5: Regular :c format (no case conversion) for comparison
    {
        Str         result = StrInit();
        const char* input  = "Hello World";

        StrReadFmt(input, "{:c}", FMT(result));

        printf("Test 5 - :c (no case conversion)\n");
        printf("Input: '%s', Output: '", input);
        for (size_t i = 0; i < result.length; i++) {
            printf("%c", result.data[i]);
        }
        printf("'\n");

        // Should read "Hello" (stops at first space, no case conversion)
        Str  expected   = StrInitFromZstr("Hello");
        bool test5_pass = (StrCmp(&result, &expected) == 0);
        printf("Expected: 'Hello', Pass: %s\n\n", test5_pass ? "true" : "false");
        success = success && test5_pass;

        StrDeinit(&expected);
        StrDeinit(&result);
    }

    printf("Overall case conversion success: %s\n", success ? "true" : "false");
    return success;
}

// Test BitVec reading
bool test_bitvec_reading(void) {
    printf("Testing BitVec reading\n");

    bool success = true;

    // Test 1: Reading binary string
    BitVec bv1 = BitVecInit();
    StrReadFmt("10110", "{}", FMT(bv1));
    Str result1 = BitVecToStr(&bv1);
    success = success && (ZstrCompare(result1.data, "10110") == 0);
    printf("Test 1 - Binary: %.*s, Success: %s\n", (int)result1.length, result1.data, 
           (ZstrCompare(result1.data, "10110") == 0) ? "true" : "false");
    StrDeinit(&result1);
    BitVecDeinit(&bv1);

    // Test 2: Reading hex format
    BitVec bv2 = BitVecInit();
    StrReadFmt("0xDEAD", "{}", FMT(bv2));
    u64 value2 = BitVecToInteger(&bv2);
    success = success && (value2 == 0xDEAD);
    printf("Test 2 - Hex: 0x%llx, Success: %s\n", value2, (value2 == 0xDEAD) ? "true" : "false");
    BitVecDeinit(&bv2);

    // Test 3: Reading octal format
    BitVec bv3 = BitVecInit();
    StrReadFmt("0o755", "{}", FMT(bv3));
    u64 value3 = BitVecToInteger(&bv3);
    success = success && (value3 == 0755);
    printf("Test 3 - Octal: %llo, Success: %s\n", value3, (value3 == 0755) ? "true" : "false");
    BitVecDeinit(&bv3);

    // Test 4: Reading with whitespace
    BitVec bv4 = BitVecInit();
    StrReadFmt("   1101", "{}", FMT(bv4));
    Str result4 = BitVecToStr(&bv4);
    success = success && (ZstrCompare(result4.data, "1101") == 0);
    printf("Test 4 - Whitespace: %.*s, Success: %s\n", (int)result4.length, result4.data,
           (ZstrCompare(result4.data, "1101") == 0) ? "true" : "false");
    StrDeinit(&result4);
    BitVecDeinit(&bv4);

    // Test 5: Reading zero values
    BitVec bv5 = BitVecInit();
    StrReadFmt("0", "{}", FMT(bv5));
    Str result5 = BitVecToStr(&bv5);
    success = success && (ZstrCompare(result5.data, "0") == 0);
    printf("Test 5 - Zero: %.*s, Success: %s\n", (int)result5.length, result5.data,
           (ZstrCompare(result5.data, "0") == 0) ? "true" : "false");
    StrDeinit(&result5);
    BitVecDeinit(&bv5);

    printf("Overall BitVec reading success: %s\n", success ? "true" : "false");
    return success;
}

// Main function that runs all tests
int main(void) {
    printf("[INFO] Starting format reader tests\n\n");

    // Array of test functions
    TestFunction tests[] = {
        test_integer_decimal_reading,
        test_integer_hex_reading,
        test_integer_binary_reading,
        test_integer_octal_reading,
        test_float_basic_reading,
        test_float_scientific_reading,
        test_string_reading,
        test_multiple_arguments_reading,
        test_error_handling_reading,
        test_character_ordinal_reading,
        test_string_case_conversion_reading,
        test_bitvec_reading
    };

    int total_tests = sizeof(tests) / sizeof(tests[0]);

    // Run all tests using the centralized test driver
    return run_test_suite(tests, total_tests, NULL, 0, "Io.Read");
}
