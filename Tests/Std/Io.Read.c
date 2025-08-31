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
    WriteFmt("Testing integer decimal reading\n");

    bool success = true;

    // Test signed integers
    i8 i8_val = 0;
    StrReadFmt("-42", "{}", i8_val);
    success = success && (i8_val == -42);

    i16 i16_val = 0;
    StrReadFmt("-1234", "{}", i16_val);
    success = success && (i16_val == -1234);

    i32 i32_val = 0;
    StrReadFmt("-123456", "{}", i32_val);
    success = success && (i32_val == -123456);

    i64 i64_val = 0;
    StrReadFmt("-1234567890", "{}", i64_val);
    success = success && (i64_val == -1234567890LL);

    // Test unsigned integers
    u8 u8_val = 0;
    StrReadFmt("42", "{}", u8_val);
    success = success && (u8_val == 42);

    u16 u16_val = 0;
    StrReadFmt("1234", "{}", u16_val);
    success = success && (u16_val == 1234);

    u32 u32_val = 0;
    StrReadFmt("123456", "{}", u32_val);
    success = success && (u32_val == 123456);

    u64 u64_val = 0;
    StrReadFmt("1234567890", "{}", u64_val);
    success = success && (u64_val == 1234567890ULL);

    // Test edge cases
    i8_val = 0;
    StrReadFmt("127", "{}", i8_val);
    success = success && (i8_val == 127);

    i8_val = 0;
    StrReadFmt("-128", "{}", i8_val);
    success = success && (i8_val == -128);

    u8_val = 0;
    StrReadFmt("255", "{}", u8_val);
    success = success && (u8_val == 255);

    u8_val = 0;
    StrReadFmt("0", "{}", u8_val);
    success = success && (u8_val == 0);

    // Test leading zeros
    i32_val = 0;
    StrReadFmt("000042", "{}", i32_val);
    success = success && (i32_val == 42);

    i32_val = 0;
    StrReadFmt("-000042", "{}", i32_val);
    success = success && (i32_val == -42);

    // Test whitespace handling
    i32_val = 0;
    StrReadFmt("   42", "{}", i32_val);
    success = success && (i32_val == 42);

    i32_val = 0;
    StrReadFmt("42   ", "{}", i32_val);
    success = success && (i32_val == 42);

    i32_val = 0;
    StrReadFmt("  42  ", "{}", i32_val);
    success = success && (i32_val == 42);

    return success;
}

// Test hexadecimal integer reading
bool test_integer_hex_reading(void) {
    WriteFmt("Testing integer hexadecimal reading\n");

    bool success = true;

    u32 val = 0;
    StrReadFmt("0xdeadbeef", "{}", val);
    success = success && (val == 0xdeadbeef);

    val = 0;
    StrReadFmt("0xDEADBEEF", "{}", val);
    success = success && (val == 0xDEADBEEF);

    // Test hex edge cases
    val = 0;
    StrReadFmt("0x0", "{}", val);
    success = success && (val == 0);

    val = 0;
    StrReadFmt("0xf", "{}", val);
    success = success && (val == 0xf);

    val = 0;
    StrReadFmt("0xaBcDeF", "{}", val);
    success = success && (val == 0xabcdef);

    return success;
}

// Test binary integer reading
bool test_integer_binary_reading(void) {
    WriteFmt("Testing integer binary reading\n");

    bool success = true;

    i8 val = 0;
    StrReadFmt("0b101010", "{}", val);
    success = success && (val == 42);

    // Test binary edge cases
    val = 0;
    StrReadFmt("0b0", "{}", val);
    success = success && (val == 0);

    val = 0;
    StrReadFmt("0b1", "{}", val);
    success = success && (val == 1);

    return success;
}

// Test octal integer reading
bool test_integer_octal_reading(void) {
    WriteFmt("Testing integer octal reading\n");

    bool success = true;

    i32 val = 0;
    StrReadFmt("0o755", "{}", val);
    success = success && (val == 0755);

    val = 0;
    StrReadFmt("755", "{}", val);
    success = success && (val == 755);

    // Test octal edge cases
    val = 0;
    StrReadFmt("0o0", "{}", val);
    success = success && (val == 0);

    val = 0;
    StrReadFmt("0o7", "{}", val);
    success = success && (val == 7);

    return success;
}

// Test basic float reading
bool test_float_basic_reading(void) {
    WriteFmt("Testing basic float reading\n");

    bool success = true;

    // Test basic float values
    f32 f32_val = 0.0f;
    StrReadFmt("3.14159", "{}", f32_val);
    success = success && float_equals(f32_val, 3.14159f);

    f64 f64_val = 0.0;
    StrReadFmt("3.14159265359", "{}", f64_val);
    success = success && double_equals(f64_val, 3.14159265359);

    // Test float edge cases
    f64_val = 1.0;
    StrReadFmt("0.0", "{}", f64_val);
    success = success && double_equals(f64_val, 0.0);

    f64_val = 1.0;
    StrReadFmt("-0.0", "{}", f64_val);
    // Special case for -0.0 which compares equal to 0.0 but has different bit pattern
    // We'll just check if it's close to zero
    success = success && double_equals(f64_val, 0.0);

    f64_val = 0.0;
    StrReadFmt("42.0", "{}", f64_val);
    success = success && double_equals(f64_val, 42.0);

    f64_val = 0.0;
    StrReadFmt("0.42", "{}", f64_val);
    success = success && double_equals(f64_val, 0.42);

    return success;
}

// Test scientific notation reading
bool test_float_scientific_reading(void) {
    WriteFmt("Testing scientific notation reading\n");

    bool success = true;

    f64 val = 0.0;
    StrReadFmt("1.23e4", "{}", val);
    success = success && double_equals(val, 12300.0);

    val = 0.0;
    StrReadFmt("1.23E4", "{}", val);
    success = success && double_equals(val, 12300.0);

    val = 0.0;
    StrReadFmt("1.23e+4", "{}", val);
    success = success && double_equals(val, 12300.0);

    val = 0.0;
    StrReadFmt("1.23e-4", "{}", val);
    success = success && double_equals(val, 0.000123);

    // Test scientific notation edge cases
    val = 0.0;
    StrReadFmt("1.0e0", "{}", val);
    success = success && double_equals(val, 1.0);

    val = 0.0;
    StrReadFmt("1.0E-0", "{}", val);
    success = success && double_equals(val, 1.0);

    val = 0.0;
    StrReadFmt("1.0e+0", "{}", val);
    success = success && double_equals(val, 1.0);

    return success;
}

// Test string reading
bool test_string_reading(void) {
    WriteFmt("Testing string reading\n");

    bool success = true;

    // Test basic string reading
    Str s = StrInit();
    StrReadFmt("Hello", "{}", s);

    Str expected = StrInitFromZstr("Hello");
    success      = success && (StrCmp(&s, &expected) == 0);
    StrDeinit(&expected);
    StrClear(&s);

    // Test quoted string reading
    StrReadFmt("\"Hello, World!\"", "{}", s);

    expected = StrInitFromZstr("Hello, World!");
    success  = success && (StrCmp(&s, &expected) == 0);
    StrDeinit(&expected);

    StrDeinit(&s);

    return success;
}

// Test reading multiple arguments
bool test_multiple_arguments_reading(void) {
    WriteFmt("Testing multiple arguments reading\n");

    bool success = true;

    i32 num  = 0;
    Str name = StrInit();
    StrReadFmt("Count: 42, Name: Alice", "Count: {}, Name: {}", num, name);

    success = success && (num == 42);

    Str expected = StrInitFromZstr("Alice");
    success      = success && (StrCmp(&name, &expected) == 0);
    StrDeinit(&expected);
    StrClear(&name);

    // Test with different order
    f64 val = 0.0;
    StrReadFmt("Value: 3.14, Name: Bob", "Value: {}, Name: {}", val, name);

    success = success && double_equals(val, 3.14);

    expected = StrInitFromZstr("Bob");
    success  = success && (StrCmp(&name, &expected) == 0);
    StrDeinit(&expected);

    StrDeinit(&name);

    return success;
}

// Test error handling
bool test_error_handling_reading(void) {
    WriteFmt("Testing error handling for reading\n");

    // For error handling tests, we'll just verify that the variables don't change
    // when invalid input is provided

    bool success = true;

    // Test mismatched format
    i32 num = 42;
    StrReadFmt("Count: forty-two", "Count: {}", num);
    // The value should remain unchanged since the parsing should fail
    success = success && (num == 42);

    // Test invalid integer
    num = 42;
    StrReadFmt("Count: abc", "Count: {}", num);
    success = success && (num == 42);

    // Test overflow
    i8 small = 42;
    StrReadFmt("Value: 1000", "Value: {}", small);
    success = success && (small == 42);

    return success;
}

// Test character ordinal reading with :c format specifier
bool test_character_ordinal_reading(void) {
    WriteFmt("Testing character ordinal reading with :c format specifier\n");

    bool success = true;

    // Test reading single character into u8
    u8 u8_val = 0;
    StrReadFmt("A", "{c}", u8_val);
    WriteFmt("u8_val = {}, expected = {}, pass = {}\n", u8_val, 'A', (u8_val == 'A') ? "true" : "false");
    success = success && (u8_val == 'A');

    u8_val = 0;
    StrReadFmt("z", "{c}", u8_val);
    WriteFmt("u8_val = {}, expected = {}, pass = {}\n", u8_val, 'z', (u8_val == 'z') ? "true" : "false");
    success = success && (u8_val == 'z');

    // Test reading single character into signed integers
    i8 i8_val = 0;
    StrReadFmt("B", "{c}", i8_val);
    WriteFmt("i8_val = {}, expected = {}, pass = {}\n", i8_val, 'B', (i8_val == 'B') ? "true" : "false");
    success = success && (i8_val == 'B');

    i16 i16_val = 0;
    StrReadFmt("C", "{c}", i16_val);
    WriteFmt("i16_val = {}, expected = {}, pass = {}\n", i16_val, 'C', (i16_val == 'C') ? "true" : "false");
    success = success && (i16_val == 'C');

    i32 i32_val = 0;
    StrReadFmt("D", "{c}", i32_val);
    WriteFmt("i32_val = {}, expected = {}, pass = {}\n", i32_val, 'D', (i32_val == 'D') ? "true" : "false");
    success = success && (i32_val == 'D');

    i64 i64_val = 0;
    StrReadFmt("E", "{c}", i64_val);
    WriteFmt("i64_val = {}, expected = {}, pass = {}\n", i64_val, 'E', (i64_val == 'E') ? "true" : "false");
    success = success && (i64_val == 'E');

    // Test reading single character into unsigned integers
    u16 u16_val = 0;
    StrReadFmt("F", "{c}", u16_val);
    WriteFmt("u16_val = {}, expected = {}, pass = {}\n", u16_val, 'F', (u16_val == 'F') ? "true" : "false");
    success = success && (u16_val == 'F');

    u32 u32_val = 0;
    StrReadFmt("G", "{c}", u32_val);
    WriteFmt("u32_val = {}, expected = {}, pass = {}\n", u32_val, 'G', (u32_val == 'G') ? "true" : "false");
    success = success && (u32_val == 'G');

    u64 u64_val = 0;
    StrReadFmt("H", "{c}", u64_val);
    WriteFmt("u64_val = {}, expected = {}, pass = {}\n", u64_val, 'H', (u64_val == 'H') ? "true" : "false");
    success = success && (u64_val == 'H');

    // Test reading multiple characters into larger integer types
    // For u16, read 2 characters
    u16_val = 0;
    StrReadFmt("AB", "{c}", u16_val);
    bool u16_multi_pass = (ZstrCompareN((const char*)&u16_val, "AB", 2) == 0);
    WriteFmt("u16_val multi-char test: comparing memory with 'AB', pass = {}\n", u16_multi_pass ? "true" : "false");
    WriteFmt(
        "DEBUG: u16_val bytes: [{}, {}], expected 'AB' bytes: [{}, {}]\n",
        (int)((u8*)&u16_val)[0],
        (int)((u8*)&u16_val)[1],
        (int)'A',
        (int)'B'
    );
    success = success && u16_multi_pass;

    // For i16, read 2 characters
    i16_val = 0;
    StrReadFmt("CD", "{c}", i16_val);
    bool i16_multi_pass = (ZstrCompareN((const char*)&i16_val, "CD", 2) == 0);
    WriteFmt("i16_val multi-char test: comparing memory with 'CD', pass = {}\n", i16_multi_pass ? "true" : "false");
    success = success && i16_multi_pass;

    // For u32, read up to 4 characters
    u32_val = 0;
    StrReadFmt("EFGH", "{c}", u32_val);
    bool u32_multi_pass = (ZstrCompareN((const char*)&u32_val, "EFGH", 4) == 0);
    WriteFmt("u32_val multi-char test: comparing memory with 'EFGH', pass = {}\n", u32_multi_pass ? "true" : "false");
    success = success && u32_multi_pass;

    // For i32, read up to 4 characters
    i32_val = 0;
    StrReadFmt("IJKL", "{c}", i32_val);
    bool i32_multi_pass = (ZstrCompareN((const char*)&i32_val, "IJKL", 4) == 0);
    WriteFmt("i32_val multi-char test: comparing memory with 'IJKL', pass = {}\n", i32_multi_pass ? "true" : "false");
    success = success && i32_multi_pass;

    // For u64, read up to 8 characters
    u64_val = 0;
    StrReadFmt("MNOPQRST", "{c}", u64_val);
    bool u64_multi_pass = (ZstrCompareN((const char*)&u64_val, "MNOPQRST", 8) == 0);
    WriteFmt(
        "u64_val multi-char test: comparing memory with 'MNOPQRST', pass = {}\n",
        u64_multi_pass ? "true" : "false"
    );
    success = success && u64_multi_pass;

    // For i64, read up to 8 characters
    i64_val = 0;
    StrReadFmt("UVWXYZab", "{c}", i64_val);
    bool i64_multi_pass = (ZstrCompareN((const char*)&i64_val, "UVWXYZab", 8) == 0);
    WriteFmt(
        "i64_val multi-char test: comparing memory with 'UVWXYZab', pass = {}\n",
        i64_multi_pass ? "true" : "false"
    );
    success = success && i64_multi_pass;

    // Test reading characters into float types (should interpret as character ordinals)
    f32 f32_val = 0.0f;
    StrReadFmt("A", "{c}", f32_val);
    bool f32_pass = (f32_val == (f32)'A');
    WriteFmt("f32_val = {}, expected = {}, pass = {}\n", f32_val, (f32)'A', f32_pass ? "true" : "false");
    success = success && f32_pass;

    f64 f64_val = 0.0;
    StrReadFmt("B", "{c}", f64_val);
    bool f64_pass = (f64_val == (f64)'B');
    WriteFmt("f64_val = {}, expected = {}, pass = {}\n", f64_val, (f64)'B', f64_pass ? "true" : "false");
    success = success && f64_pass;

    // Test with high ASCII characters
    u8_val = 0;
    StrReadFmt("~", "{c}", u8_val);
    bool tilde_pass = (u8_val == '~');
    WriteFmt("u8_val = {}, expected = {} (~), pass = {}\n", u8_val, '~', tilde_pass ? "true" : "false");
    success = success && tilde_pass;

    // Test partial reads for larger types with fewer characters
    u32_val = 0;
    StrReadFmt("XY", "{c}", u32_val);
    bool xy_pass = (ZstrCompareN((const char*)&u32_val, "XY", 2) == 0);
    WriteFmt("u32_val partial test: comparing memory with 'XY', pass = {}\n", xy_pass ? "true" : "false");
    success = success && xy_pass;

    u64_val = 0;
    StrReadFmt("abc", "{c}", u64_val);
    bool abc_pass = (ZstrCompareN((const char*)&u64_val, "abc", 3) == 0);
    WriteFmt("u64_val partial test: comparing memory with 'abc', pass = {}\n", abc_pass ? "true" : "false");
    success = success && abc_pass;

    // Test that :c has no effect on string types (should work like regular string reading)
    Str str_val = StrInit();
    StrReadFmt("Hello", "{c}", str_val);

    Str  expected = StrInitFromZstr("Hello");
    bool str_pass = (StrCmp(&str_val, &expected) == 0);
    WriteFmt("str_val test: comparing with 'Hello', pass = {}\n", str_pass ? "true" : "false");
    success = success && str_pass;
    StrDeinit(&expected);
    StrDeinit(&str_val);

    // Test :c with quoted strings (should work like regular string reading)
    str_val = StrInit();
    StrReadFmt("\"World\"", "{c}", str_val);

    expected             = StrInitFromZstr("World");
    bool quoted_str_pass = (StrCmp(&str_val, &expected) == 0);
    WriteFmt("quoted str_val test: comparing with 'World', pass = {}\n", quoted_str_pass ? "true" : "false");
    success = success && quoted_str_pass;
    StrDeinit(&expected);
    StrDeinit(&str_val);

    WriteFmt("Overall success: {}\n", success ? "true" : "false");
    return success;
}

// Test string case conversion with :a and :A format specifiers
bool test_string_case_conversion_reading(void) {
    WriteFmt("Testing string case conversion with :a and :A format specifiers\n");

    bool success = true;

    // Test 1: :a (lowercase) conversion
    {
        Str         result = StrInit();
        const char* input  = "Hello World";

        StrReadFmt(input, "{a}", result);

        WriteFmt("Test 1 - :a (lowercase)\n");
        WriteFmt("Input: '{}', Output: '", input);
        for (size_t i = 0; i < result.length; i++) {
            WriteFmt("{c}", result.data[i]);
        }
        WriteFmt("'\n");

        // Should read "hello" (stops at first space)
        Str  expected   = StrInitFromZstr("hello");
        bool test1_pass = (StrCmp(&result, &expected) == 0);
        WriteFmt("Expected: 'hello', Pass: {}\n\n", test1_pass ? "true" : "false");
        success = success && test1_pass;

        StrDeinit(&expected);
        StrDeinit(&result);
    }

    // Test 2: :A (uppercase) conversion
    {
        Str         result = StrInit();
        const char* input  = "hello world";

        StrReadFmt(input, "{A}", result);

        WriteFmt("Test 2 - :A (uppercase)\n");
        WriteFmt("Input: '{}', Output: '", input);
        for (size_t i = 0; i < result.length; i++) {
            WriteFmt("{c}", result.data[i]);
        }
        WriteFmt("'\n");

        // Should read "HELLO" (stops at first space)
        Str  expected   = StrInitFromZstr("HELLO");
        bool test2_pass = (StrCmp(&result, &expected) == 0);
        WriteFmt("Expected: 'HELLO', Pass: {}\n\n", test2_pass ? "true" : "false");
        success = success && test2_pass;

        StrDeinit(&expected);
        StrDeinit(&result);
    }

    // Test 3: :a with quoted string
    {
        Str         result = StrInit();
        const char* input  = "\"MiXeD CaSe\"";

        StrReadFmt(input, "{a}", result);

        WriteFmt("Test 3 - :a with quoted string\n");
        WriteFmt("Input: '{}', Output: '", input);
        for (size_t i = 0; i < result.length; i++) {
            WriteFmt("{c}", result.data[i]);
        }
        WriteFmt("'\n");

        // Should read "mixed case" (converts the entire quoted string)
        Str  expected   = StrInitFromZstr("mixed case");
        bool test3_pass = (StrCmp(&result, &expected) == 0);
        WriteFmt("Expected: 'mixed case', Pass: {}\n\n", test3_pass ? "true" : "false");
        success = success && test3_pass;

        StrDeinit(&expected);
        StrDeinit(&result);
    }

    // Test 4: :A with quoted string containing special characters
    {
        Str         result = StrInit();
        const char* input  = "\"abc123XYZ\"";

        StrReadFmt(input, "{A}", result);

        WriteFmt("Test 4 - :A with mixed alphanumeric\n");
        WriteFmt("Input: '{}', Output: '", input);
        for (size_t i = 0; i < result.length; i++) {
            WriteFmt("{c}", result.data[i]);
        }
        WriteFmt("'\n");

        // Should read "ABC123XYZ" (only letters are converted, numbers unchanged)
        Str  expected   = StrInitFromZstr("ABC123XYZ");
        bool test4_pass = (StrCmp(&result, &expected) == 0);
        WriteFmt("Expected: 'ABC123XYZ', Pass: {}\n\n", test4_pass ? "true" : "false");
        success = success && test4_pass;

        StrDeinit(&expected);
        StrDeinit(&result);
    }

    // Test 5: Regular :c format (no case conversion) for comparison
    {
        Str         result = StrInit();
        const char* input  = "Hello World";

        StrReadFmt(input, "{c}", result);

        WriteFmt("Test 5 - :c (no case conversion)\n");
        WriteFmt("Input: '{}', Output: '", input);
        for (size_t i = 0; i < result.length; i++) {
            WriteFmt("{c}", result.data[i]);
        }
        WriteFmt("'\n");

        // Should read "Hello" (stops at first space, no case conversion)
        Str  expected   = StrInitFromZstr("Hello");
        bool test5_pass = (StrCmp(&result, &expected) == 0);
        WriteFmt("Expected: 'Hello', Pass: {}\n\n", test5_pass ? "true" : "false");
        success = success && test5_pass;

        StrDeinit(&expected);
        StrDeinit(&result);
    }

    WriteFmt("Overall case conversion success: {}\n", success ? "true" : "false");
    return success;
}

// Test BitVec reading
bool test_bitvec_reading(void) {
    WriteFmt("Testing BitVec reading\n");

    bool success = true;

    // Test 1: Reading binary string
    BitVec bv1 = BitVecInit();
    StrReadFmt("10110", "{}", bv1);
    Str result1 = BitVecToStr(&bv1);
    success     = success && (ZstrCompare(result1.data, "10110") == 0);
    WriteFmt(
        "Test 1 - Binary: {}, Success: {}\n",
        result1,
        (ZstrCompare(result1.data, "10110") == 0) ? "true" : "false"
    );
    StrDeinit(&result1);
    BitVecDeinit(&bv1);

    // Test 2: Reading hex format
    BitVec bv2 = BitVecInit();
    StrReadFmt("0xDEAD", "{}", bv2);
    u64 value2 = BitVecToInteger(&bv2);
    success    = success && (value2 == 0xDEAD);
    WriteFmt("Test 2 - Hex: {}, Success: {}\n", value2, (value2 == 0xDEAD) ? "true" : "false");
    BitVecDeinit(&bv2);

    // Test 3: Reading octal format
    BitVec bv3 = BitVecInit();
    StrReadFmt("0o755", "{}", bv3);
    u64 value3 = BitVecToInteger(&bv3);
    success    = success && (value3 == 0755);
    WriteFmt("Test 3 - Octal: {}, Success: {}\n", value3, (value3 == 0755) ? "true" : "false");
    BitVecDeinit(&bv3);

    // Test 4: Reading with whitespace
    BitVec bv4 = BitVecInit();
    StrReadFmt("   1101", "{}", bv4);
    Str result4 = BitVecToStr(&bv4);
    success     = success && (ZstrCompare(result4.data, "1101") == 0);
    WriteFmt(
        "Test 4 - Whitespace: {}, Success: {}\n",
        result4,
        (ZstrCompare(result4.data, "1101") == 0) ? "true" : "false"
    );
    StrDeinit(&result4);
    BitVecDeinit(&bv4);

    // Test 5: Reading zero values
    BitVec bv5 = BitVecInit();
    StrReadFmt("0", "{}", bv5);
    Str result5 = BitVecToStr(&bv5);
    success     = success && (ZstrCompare(result5.data, "0") == 0);
    WriteFmt("Test 5 - Zero: {}, Success: {}\n", result5, (ZstrCompare(result5.data, "0") == 0) ? "true" : "false");
    StrDeinit(&result5);
    BitVecDeinit(&bv5);

    WriteFmt("Overall BitVec reading success: {}\n", success ? "true" : "false");
    return success;
}

// Main function that runs all tests
int main(void) {
    WriteFmt("[INFO] Starting format reader tests\n\n");

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
