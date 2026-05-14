#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Container/BitVec.h>
#include <Misra/Std/Container/Int.h>
#include <Misra/Std/Container/Float.h>
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
bool test_int_reading(void);
bool test_float_reading(void);

// Test decimal integer reading
bool test_integer_decimal_reading(void) {
    WriteFmt("Testing integer decimal reading\n");

    const char *z = NULL;

    bool success = true;

    // Test signed integers
    i8 i8_val = 0;
    z         = "-42";
    StrReadFmt(z, "{}", i8_val);
    success = success && (i8_val == -42);

    i16 i16_val = 0;
    z           = "-1234";
    StrReadFmt(z, "{}", i16_val);
    success = success && (i16_val == -1234);

    i32 i32_val = 0;
    z           = "-123456";
    StrReadFmt(z, "{}", i32_val);
    success = success && (i32_val == -123456);

    i64 i64_val = 0;
    z           = "-1234567890";
    StrReadFmt(z, "{}", i64_val);
    success = success && (i64_val == -1234567890LL);

    // Test unsigned integers
    u8 u8_val = 0;
    z         = "42";
    StrReadFmt(z, "{}", u8_val);
    success = success && (u8_val == 42);

    u16 u16_val = 0;
    z           = "1234";
    StrReadFmt(z, "{}", u16_val);
    success = success && (u16_val == 1234);

    u32 u32_val = 0;
    z           = "123456";
    StrReadFmt(z, "{}", u32_val);
    success = success && (u32_val == 123456);

    u64 u64_val = 0;
    z           = "1234567890";
    StrReadFmt(z, "{}", u64_val);
    success = success && (u64_val == 1234567890ULL);

    // Test edge cases
    i8_val = 0;
    z      = "127";
    StrReadFmt(z, "{}", i8_val);
    success = success && (i8_val == 127);

    i8_val = 0;
    z      = "-128";
    StrReadFmt(z, "{}", i8_val);
    success = success && (i8_val == -128);

    u8_val = 0;
    z      = "255";
    StrReadFmt(z, "{}", u8_val);
    success = success && (u8_val == 255);

    u8_val = 0;
    z      = "0";
    StrReadFmt(z, "{}", u8_val);
    success = success && (u8_val == 0);

    // Test leading zeros
    i32_val = 0;
    z       = "000042";
    StrReadFmt(z, "{}", i32_val);
    success = success && (i32_val == 42);

    i32_val = 0;
    z       = "-000042";
    StrReadFmt(z, "{}", i32_val);
    success = success && (i32_val == -42);

    // Test whitespace handling
    i32_val = 0;
    z       = "   42";
    StrReadFmt(z, "{}", i32_val);
    success = success && (i32_val == 42);

    i32_val = 0;
    z       = "42   ";
    StrReadFmt(z, "{}", i32_val);
    success = success && (i32_val == 42);

    i32_val = 0;
    z       = "  42  ";
    StrReadFmt(z, "{}", i32_val);
    success = success && (i32_val == 42);

    return success;
}

// Test hexadecimal integer reading
bool test_integer_hex_reading(void) {
    WriteFmt("Testing integer hexadecimal reading\n");

    const char *z = NULL;

    bool success = true;

    u32 val = 0;
    z       = "0xdeadbeef";
    StrReadFmt(z, "{}", val);
    success = success && (val == 0xdeadbeef);

    val = 0;
    z   = "0xDEADBEEF";
    StrReadFmt(z, "{}", val);
    success = success && (val == 0xDEADBEEF);

    // Test hex edge cases
    val = 0;
    z   = "0x0";
    StrReadFmt(z, "{}", val);
    success = success && (val == 0);

    val = 0;
    z   = "0xf";
    StrReadFmt(z, "{}", val);
    success = success && (val == 0xf);

    val = 0;
    z   = "0xaBcDeF";
    StrReadFmt(z, "{}", val);
    success = success && (val == 0xabcdef);

    return success;
}

// Test binary integer reading
bool test_integer_binary_reading(void) {
    WriteFmt("Testing integer binary reading\n");

    const char *z = NULL;

    bool success = true;

    i8 val = 0;
    z      = "0b101010";
    StrReadFmt(z, "{}", val);
    success = success && (val == 42);

    // Test binary edge cases
    val = 0;
    z   = "0b0";
    StrReadFmt(z, "{}", val);
    success = success && (val == 0);

    val = 0;
    z   = "0b1";
    StrReadFmt(z, "{}", val);
    success = success && (val == 1);

    return success;
}

// Test octal integer reading
bool test_integer_octal_reading(void) {
    WriteFmt("Testing integer octal reading\n");

    const char *z = NULL;

    bool success = true;

    i32 val = 0;
    z       = "0o755";
    StrReadFmt(z, "{}", val);
    success = success && (val == 0755);

    val = 0;
    z   = "755";
    StrReadFmt(z, "{}", val);
    success = success && (val == 755);

    // Test octal edge cases
    val = 0;
    z   = "0o0";
    StrReadFmt(z, "{}", val);
    success = success && (val == 0);

    val = 0;
    z   = "0o7";
    StrReadFmt(z, "{}", val);
    success = success && (val == 7);

    return success;
}

// Test basic float reading
bool test_float_basic_reading(void) {
    WriteFmt("Testing basic float reading\n");

    const char *z = NULL;

    bool success = true;

    // Test basic float values
    f32 f32_val = 0.0f;
    z           = "3.14159";
    StrReadFmt(z, "{}", f32_val);
    success = success && float_equals(f32_val, 3.14159f);

    f64 f64_val = 0.0;
    z           = "3.14159265359";
    StrReadFmt(z, "{}", f64_val);
    success = success && double_equals(f64_val, 3.14159265359);

    // Test float edge cases
    f64_val = 1.0;
    z       = "0.0";
    StrReadFmt(z, "{}", f64_val);
    success = success && double_equals(f64_val, 0.0);

    f64_val = 1.0;
    z       = "-0.0";
    StrReadFmt(z, "{}", f64_val);
    // Special case for -0.0 which compares equal to 0.0 but has different bit pattern
    // We'll just check if it's close to zero
    success = success && double_equals(f64_val, 0.0);

    f64_val = 0.0;
    z       = "42.0";
    StrReadFmt(z, "{}", f64_val);
    success = success && double_equals(f64_val, 42.0);

    f64_val = 0.0;
    z       = "0.42";
    StrReadFmt(z, "{}", f64_val);
    success = success && double_equals(f64_val, 0.42);

    return success;
}

// Test scientific notation reading
bool test_float_scientific_reading(void) {
    WriteFmt("Testing scientific notation reading\n");

    const char *z = NULL;

    bool success = true;

    f64 val = 0.0;
    z       = "1.23e4";
    StrReadFmt(z, "{}", val);
    success = success && double_equals(val, 12300.0);

    val = 0.0;
    z   = "1.23E4";
    StrReadFmt(z, "{}", val);
    success = success && double_equals(val, 12300.0);

    val = 0.0;
    z   = "1.23e+4";
    StrReadFmt(z, "{}", val);
    success = success && double_equals(val, 12300.0);

    val = 0.0;
    z   = "1.23e-4";
    StrReadFmt(z, "{}", val);
    success = success && double_equals(val, 0.000123);

    // Test scientific notation edge cases
    val = 0.0;
    z   = "1.0e0";
    StrReadFmt(z, "{}", val);
    success = success && double_equals(val, 1.0);

    val = 0.0;
    z   = "1.0E-0";
    StrReadFmt(z, "{}", val);
    success = success && double_equals(val, 1.0);

    val = 0.0;
    z   = "1.0e+0";
    StrReadFmt(z, "{}", val);
    success = success && double_equals(val, 1.0);

    return success;
}

// Test string reading
bool test_string_reading(void) {
    WriteFmt("Testing string reading\n");

    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    const char *z = NULL;

    bool success = true;

    // Test basic string reading
    Str s = StrInit(&alloc);
    z     = "Hello";
    StrReadFmt(z, "{}", s);

    Str expected = StrInitFromZstr("Hello", &alloc);
    success      = success && (StrCmp(&s, &expected) == 0);
    StrDeinit(&expected);
    StrClear(&s);

    // Test quoted string reading
    z = "\"Hello, World!\"";
    StrReadFmt(z, "{s}", s);

    expected = StrInitFromZstr("Hello, World!", &alloc);
    success  = success && (StrCmp(&s, &expected) == 0);
    StrDeinit(&expected);

    {
        char *zs = NULL;

        z = "Allocator-backed";
        StrReadFmt(z, "{s}", ZstrIO(zs, alloc_base));
        success = success && (ZstrCompare(zs, "Allocator-backed") == 0);

        z = "\"Allocator-backed replacement\"";
        StrReadFmt(z, "{s}", ZstrIO(zs, alloc_base));
        success = success && (ZstrCompare(zs, "Allocator-backed replacement") == 0);
        ZstrDeinitAlloc(&zs, alloc_base);
    }

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);

    return success;
}

// Test reading multiple arguments
bool test_multiple_arguments_reading(void) {
    WriteFmt("Testing multiple arguments reading\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    const char *z = NULL;

    bool success = true;

    i32 num  = 0;
    Str name = StrInit(&alloc);
    z        = "Count: 42, Name: Alice";
    StrReadFmt(z, "Count: {}, Name: {}", num, name);

    success = success && (num == 42);

    Str expected = StrInitFromZstr("Alice", &alloc);
    success      = success && (StrCmp(&name, &expected) == 0);
    StrDeinit(&expected);
    StrClear(&name);

    // Test with different order
    f64 val = 0.0;
    z       = "Value: 3.14, Name: Bob";
    StrReadFmt(z, "Value: {}, Name: {}", val, name);

    success = success && double_equals(val, 3.14);

    expected = StrInitFromZstr("Bob", &alloc);
    success  = success && (StrCmp(&name, &expected) == 0);
    StrDeinit(&expected);

    StrDeinit(&name);
    DefaultAllocatorDeinit(&alloc);

    return success;
}

// Test error handling
bool test_error_handling_reading(void) {
    WriteFmt("Testing error handling for reading\n");

    const char *z = NULL;

    // For error handling tests, we'll just verify that the variables don't change
    // when invalid input is provided

    bool success = true;

    // Test mismatched format
    i32 num = 42;
    z       = "Count: forty-two";
    StrReadFmt(z, "Count: {}", num);
    success = success && (num == 42);

    // Test invalid integer
    num = 42;
    z   = "Count: abc";
    StrReadFmt(z, "Count: {}", num);
    success = success && (num == 42);

    // Test overflow
    i8 small = 42;
    z        = "Value: 1000";
    StrReadFmt(z, "Value: {}", small);
    success = success && (small == 42);

    return success;
}

// Test character ordinal reading with :c format specifier
bool test_character_ordinal_reading(void) {
    WriteFmt("Testing character ordinal reading with :c format specifier\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    const char *z = NULL;

    bool success = true;

    // Test reading single character into u8
    u8 u8_val = 0;
    z         = "A";
    StrReadFmt(z, "{c}", u8_val);
    WriteFmt("u8_val = {}, expected = {}, pass = {}\n", u8_val, 'A', (u8_val == 'A') ? "true" : "false");
    success = success && (u8_val == 'A');

    u8_val = 0;
    z      = "z";
    StrReadFmt(z, "{c}", u8_val);
    WriteFmt("u8_val = {}, expected = {}, pass = {}\n", u8_val, 'z', (u8_val == 'z') ? "true" : "false");
    success = success && (u8_val == 'z');

    // Test reading single character into signed integers
    i8 i8_val = 0;
    z         = "B";
    StrReadFmt(z, "{c}", i8_val);
    WriteFmt("i8_val = {}, expected = {}, pass = {}\n", i8_val, 'B', (i8_val == 'B') ? "true" : "false");
    success = success && (i8_val == 'B');

    i16 i16_val = 0;
    z           = "C";
    StrReadFmt(z, "{c}", i16_val);
    WriteFmt("i16_val = {}, expected = {}, pass = {}\n", i16_val, 'C', (i16_val == 'C') ? "true" : "false");
    success = success && (i16_val == 'C');

    i32 i32_val = 0;
    z           = "D";
    StrReadFmt(z, "{c}", i32_val);
    WriteFmt("i32_val = {}, expected = {}, pass = {}\n", i32_val, 'D', (i32_val == 'D') ? "true" : "false");
    success = success && (i32_val == 'D');

    i64 i64_val = 0;
    z           = "E";
    StrReadFmt(z, "{c}", i64_val);
    WriteFmt("i64_val = {}, expected = {}, pass = {}\n", i64_val, 'E', (i64_val == 'E') ? "true" : "false");
    success = success && (i64_val == 'E');

    // Test reading single character into unsigned integers
    u16 u16_val = 0;
    z           = "F";
    StrReadFmt(z, "{c}", u16_val);
    WriteFmt("u16_val = {}, expected = {}, pass = {}\n", u16_val, 'F', (u16_val == 'F') ? "true" : "false");
    success = success && (u16_val == 'F');

    u32 u32_val = 0;
    z           = "G";
    StrReadFmt(z, "{c}", u32_val);
    WriteFmt("u32_val = {}, expected = {}, pass = {}\n", u32_val, 'G', (u32_val == 'G') ? "true" : "false");
    success = success && (u32_val == 'G');

    u64 u64_val = 0;
    z           = "H";
    StrReadFmt(z, "{c}", u64_val);
    WriteFmt("u64_val = {}, expected = {}, pass = {}\n", u64_val, 'H', (u64_val == 'H') ? "true" : "false");
    success = success && (u64_val == 'H');

    // Test reading multiple characters into larger integer types
    u16_val = 0;
    z       = "AB";
    StrReadFmt(z, "{c}", u16_val);
    bool u16_multi_pass = (ZstrCompareN((const char *)&u16_val, "AB", 2) == 0);
    WriteFmt("u16_val multi-char test: comparing memory with 'AB', pass = {}\n", u16_multi_pass ? "true" : "false");
    WriteFmt(
        "DEBUG: u16_val bytes: [{}, {}], expected 'AB' bytes: [{}, {}]\n",
        (int)((u8 *)&u16_val)[0],
        (int)((u8 *)&u16_val)[1],
        (int)'A',
        (int)'B'
    );
    success = success && u16_multi_pass;

    i16_val = 0;
    z       = "CD";
    StrReadFmt(z, "{c}", i16_val);
    bool i16_multi_pass = (ZstrCompareN((const char *)&i16_val, "CD", 2) == 0);
    WriteFmt("i16_val multi-char test: comparing memory with 'CD', pass = {}\n", i16_multi_pass ? "true" : "false");
    success = success && i16_multi_pass;

    u32_val = 0;
    z       = "EFGH";
    StrReadFmt(z, "{c}", u32_val);
    bool u32_multi_pass = (ZstrCompareN((const char *)&u32_val, "EFGH", 4) == 0);
    WriteFmt("u32_val multi-char test: comparing memory with 'EFGH', pass = {}\n", u32_multi_pass ? "true" : "false");
    success = success && u32_multi_pass;

    i32_val = 0;
    z       = "IJKL";
    StrReadFmt(z, "{c}", i32_val);
    bool i32_multi_pass = (ZstrCompareN((const char *)&i32_val, "IJKL", 4) == 0);
    WriteFmt("i32_val multi-char test: comparing memory with 'IJKL', pass = {}\n", i32_multi_pass ? "true" : "false");
    success = success && i32_multi_pass;

    u64_val = 0;
    z       = "MNOPQRST";
    StrReadFmt(z, "{c}", u64_val);
    bool u64_multi_pass = (ZstrCompareN((const char *)&u64_val, "MNOPQRST", 8) == 0);
    WriteFmt(
        "u64_val multi-char test: comparing memory with 'MNOPQRST', pass = {}\n",
        u64_multi_pass ? "true" : "false"
    );
    success = success && u64_multi_pass;

    i64_val = 0;
    z       = "UVWXYZab";
    StrReadFmt(z, "{c}", i64_val);
    bool i64_multi_pass = (ZstrCompareN((const char *)&i64_val, "UVWXYZab", 8) == 0);
    WriteFmt(
        "i64_val multi-char test: comparing memory with 'UVWXYZab', pass = {}\n",
        i64_multi_pass ? "true" : "false"
    );
    success = success && i64_multi_pass;

    f32 f32_val = 0.0f;
    z           = "A";
    StrReadFmt(z, "{c}", f32_val);
    bool f32_pass = (f32_val == (f32)'A');
    WriteFmt("f32_val = {}, expected = {}, pass = {}\n", f32_val, (f32)'A', f32_pass ? "true" : "false");
    success = success && f32_pass;

    f64 f64_val = 0.0;
    z           = "B";
    StrReadFmt(z, "{c}", f64_val);
    bool f64_pass = (f64_val == (f64)'B');
    WriteFmt("f64_val = {}, expected = {}, pass = {}\n", f64_val, (f64)'B', f64_pass ? "true" : "false");
    success = success && f64_pass;

    u8_val = 0;
    z      = "~";
    StrReadFmt(z, "{c}", u8_val);
    bool tilde_pass = (u8_val == '~');
    WriteFmt("u8_val = {}, expected = {} (~), pass = {}\n", u8_val, '~', tilde_pass ? "true" : "false");
    success = success && tilde_pass;

    u32_val = 0;
    z       = "XY";
    StrReadFmt(z, "{c}", u32_val);
    bool xy_pass = (ZstrCompareN((const char *)&u32_val, "XY", 2) == 0);
    WriteFmt("u32_val partial test: comparing memory with 'XY', pass = {}\n", xy_pass ? "true" : "false");
    success = success && xy_pass;

    u64_val = 0;
    z       = "abc";
    StrReadFmt(z, "{c}", u64_val);
    bool abc_pass = (ZstrCompareN((const char *)&u64_val, "abc", 3) == 0);
    WriteFmt("u64_val partial test: comparing memory with 'abc', pass = {}\n", abc_pass ? "true" : "false");
    success = success && abc_pass;

    Str str_val = StrInit(&alloc);
    z           = "Hello";
    StrReadFmt(z, "{c}", str_val);
    Str  expected = StrInitFromZstr("Hello", &alloc);
    bool str_pass = (StrCmp(&str_val, &expected) == 0);
    WriteFmt("str_val test: comparing with 'Hello', pass = {}\n", str_pass ? "true" : "false");
    success = success && str_pass;
    StrDeinit(&expected);
    StrDeinit(&str_val);

    str_val = StrInit(&alloc);
    z       = "\"World\"";
    StrReadFmt(z, "{cs}", str_val);
    expected             = StrInitFromZstr("World", &alloc);
    bool quoted_str_pass = (StrCmp(&str_val, &expected) == 0);
    WriteFmt("quoted str_val test: comparing with 'World', pass = {}\n", quoted_str_pass ? "true" : "false");
    success = success && quoted_str_pass;
    StrDeinit(&expected);
    StrDeinit(&str_val);

    WriteFmt("Overall success: {}\n", success ? "true" : "false");
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Test string case conversion with :a and :A format specifiers
bool test_string_case_conversion_reading(void) {
    WriteFmt("Testing string case conversion with :a and :A format specifiers\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    const char *z = NULL;

    bool success = true;

    // Test 1: :a (lowercase) conversion
    {
        Str         result = StrInit(&alloc);
        const char *in     = "Hello World";

        z = in;
        StrReadFmt(z, "{a}", result);

        WriteFmt("Test 1 - :a (lowercase)\n");
        WriteFmt("Input: '{}', Output: '", in);
        for (size_t i = 0; i < result.length; i++) {
            WriteFmt("{c}", result.data[i]);
        }
        WriteFmt("'\n");

        // Should read "hello" (stops at first space)
        Str  expected   = StrInitFromZstr("hello world", &alloc);
        bool test1_pass = (StrCmp(&result, &expected) == 0);
        WriteFmt("Expected: 'hello', Pass: {}\n\n", test1_pass ? "true" : "false");
        success = success && test1_pass;

        StrDeinit(&expected);
        StrDeinit(&result);
    }

    // Test 1.1: :a (lowercase) conversion
    {
        Str         result = StrInit(&alloc);
        const char *in     = "Hello World";

        z = in;
        StrReadFmt(z, "{as}", result);

        WriteFmt("Test 1.1 - :as (lowercase string single word)\n");
        WriteFmt("Input: '{}', Output: '", in);
        for (size_t i = 0; i < result.length; i++) {
            WriteFmt("{c}", result.data[i]);
        }
        WriteFmt("'\n");

        // Should read "hello" (stops at first space)
        Str  expected   = StrInitFromZstr("hello", &alloc);
        bool test1_pass = (StrCmp(&result, &expected) == 0);
        WriteFmt("Expected: 'hello', Pass: {}\n\n", test1_pass ? "true" : "false");
        success = success && test1_pass;

        StrDeinit(&expected);
        StrDeinit(&result);
    }

    // Test 2: :A (uppercase) conversion
    {
        Str         result = StrInit(&alloc);
        const char *in     = "hello world";

        z = in;
        StrReadFmt(z, "{A}", result);

        WriteFmt("Test 2 - :A (uppercase)\n");
        WriteFmt("Input: '{}', Output: '", in);
        for (size_t i = 0; i < result.length; i++) {
            WriteFmt("{c}", result.data[i]);
        }
        WriteFmt("'\n");

        // Should read "HELLO" (stops at first space)
        Str  expected   = StrInitFromZstr("HELLO WORLD", &alloc);
        bool test2_pass = (StrCmp(&result, &expected) == 0);
        WriteFmt("Expected: 'HELLO', Pass: {}\n\n", test2_pass ? "true" : "false");
        success = success && test2_pass;

        StrDeinit(&expected);
        StrDeinit(&result);
    }

    // Test 2.1: :A (uppercase) conversion
    {
        Str         result1 = StrInit(&alloc);
        Str         result2 = StrInit(&alloc);
        const char *in      = "hello world";

        z = in;
        StrReadFmt(z, "{A} {A}", result1, result2);

        WriteFmt("Test 2 - :A (uppercase with split format)\n");
        WriteFmt("Input: '{}', Output: '{} {}'", in, result1, result2);

        bool test2_pass  = (StrCmpZstr(&result1, "HELLO") == 0);
        test2_pass      &= (StrCmpZstr(&result2, "WORLD") == 0);
        WriteFmt("Expected: 'HELLO WORLD', Pass: {}\n\n", test2_pass ? "true" : "false");
        success = success && test2_pass;

        StrDeinit(&result1);
        StrDeinit(&result2);
    }

    // Test 2.2: :A (uppercase) conversion
    {
        Str         result1 = StrInit(&alloc);
        Str         result2 = StrInit(&alloc);
        const char *in      = "hello world mighty misra";

        z = in;
        StrReadFmt(z, "{As}{A}", result1, result2);
        // result1 must consume first word only
        // result2 must consume the space after hello and then everything after it

        WriteFmt("Test 2 - :A (uppercase with split format)\n");
        WriteFmt("Input: '{}', Output: '{}{}'", in, result1, result2);

        bool test2_pass  = (StrCmpZstr(&result1, "HELLO") == 0);
        test2_pass      &= (StrCmpZstr(&result2, " WORLD MIGHTY MISRA") == 0); // notice the extra space
        WriteFmt("Expected: 'HELLO WORLD MIGHTY MISRA', Pass: {}\n\n", test2_pass ? "true" : "false");
        success = success && test2_pass;

        StrDeinit(&result1);
        StrDeinit(&result2);
    }

    // Test 3: :a with quoted string
    {
        Str         result = StrInit(&alloc);
        const char *in     = "\"MiXeD CaSe\"";

        z = in;
        StrReadFmt(z, "{as}", result);

        WriteFmt("Test 3 - :a with quoted string\n");
        WriteFmt("Input: '{}', Output: '", in);
        for (size_t i = 0; i < result.length; i++) {
            WriteFmt("{c}", result.data[i]);
        }
        WriteFmt("'\n");

        // Should read "mixed case" (converts the entire quoted string)
        Str  expected   = StrInitFromZstr("mixed case", &alloc);
        bool test3_pass = (StrCmp(&result, &expected) == 0);
        WriteFmt("Expected: 'mixed case', Pass: {}\n\n", test3_pass ? "true" : "false");
        success = success && test3_pass;

        StrDeinit(&expected);
        StrDeinit(&result);
    }

    // Test 4: :A with quoted string containing special characters
    {
        Str         result = StrInit(&alloc);
        const char *in     = "\"abc123XYZ\"";

        z = in;
        StrReadFmt(z, "{As}", result);

        WriteFmt("Test 4 - :A with mixed alphanumeric\n");
        WriteFmt("Input: '{}', Output: '", in);
        for (size_t i = 0; i < result.length; i++) {
            WriteFmt("{c}", result.data[i]);
        }
        WriteFmt("'\n");

        // Should read "ABC123XYZ" (only letters are converted, numbers unchanged)
        Str  expected   = StrInitFromZstr("ABC123XYZ", &alloc);
        bool test4_pass = (StrCmp(&result, &expected) == 0);
        WriteFmt("Expected: 'ABC123XYZ', Pass: {}\n\n", test4_pass ? "true" : "false");
        success = success && test4_pass;

        StrDeinit(&expected);
        StrDeinit(&result);
    }

    // Test 5: Regular :c format (no case conversion) for comparison
    {
        Str         result = StrInit(&alloc);
        const char *in     = "Hello World";

        z = in;
        StrReadFmt(z, "{c}", result);

        WriteFmt("Test 5 - :c (no case conversion)\n");
        WriteFmt("Input: '{}', Output: '", in);
        for (size_t i = 0; i < result.length; i++) {
            WriteFmt("{c}", result.data[i]);
        }
        WriteFmt("'\n");

        // Should read "Hello" (stops at first space, no case conversion)
        Str  expected   = StrInitFromZstr("Hello World", &alloc);
        bool test5_pass = (StrCmp(&result, &expected) == 0);
        WriteFmt("Expected: 'Hello World', Pass: {}\n\n", test5_pass ? "true" : "false");
        success = success && test5_pass;

        StrDeinit(&expected);
        StrDeinit(&result);
    }

    WriteFmt("Overall case conversion success: {}\n", success ? "true" : "false");
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Test BitVec reading
bool test_bitvec_reading(void) {
    WriteFmt("Testing BitVec reading\n");

    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    const char *z = NULL;

    bool success = true;

    // Test 1: Reading binary string
    BitVec bv1 = BitVecInit(alloc_base);
    z          = "10110";
    StrReadFmt(z, "{}", bv1);
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
    BitVec bv2 = BitVecInit(alloc_base);
    z          = "0xDEAD";
    StrReadFmt(z, "{}", bv2);
    u64 value2 = BitVecToInteger(&bv2);
    success    = success && (value2 == 0xDEAD);
    WriteFmt("Test 2 - Hex: {}, Success: {}\n", value2, (value2 == 0xDEAD) ? "true" : "false");
    BitVecDeinit(&bv2);

    // Test 3: Reading octal format
    BitVec bv3 = BitVecInit(alloc_base);
    z          = "0o755";
    StrReadFmt(z, "{}", bv3);
    u64 value3 = BitVecToInteger(&bv3);
    success    = success && (value3 == 0755);
    WriteFmt("Test 3 - Octal: {}, Success: {}\n", value3, (value3 == 0755) ? "true" : "false");
    BitVecDeinit(&bv3);

    // Test 4: Reading with whitespace
    BitVec bv4 = BitVecInit(alloc_base);
    z          = "   1101";
    StrReadFmt(z, "{}", bv4);
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
    BitVec bv5 = BitVecInit(alloc_base);
    z          = "0";
    StrReadFmt(z, "{}", bv5);
    Str result5 = BitVecToStr(&bv5);
    success     = success && (ZstrCompare(result5.data, "0") == 0);
    WriteFmt("Test 5 - Zero: {}, Success: {}\n", result5, (ZstrCompare(result5.data, "0") == 0) ? "true" : "false");
    StrDeinit(&result5);
    BitVecDeinit(&bv5);

    WriteFmt("Overall BitVec reading success: {}\n", success ? "true" : "false");
    DefaultAllocatorDeinit(&alloc);
    return success;
}

bool test_int_reading(void) {
    WriteFmt("Testing Int reading\n");

    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    const char *z = NULL;
    bool        success = true;

    Int dec = IntInit(alloc_base);
    Int hex = IntInit(alloc_base);
    Int bin = IntInit(alloc_base);
    Int oct = IntInit(alloc_base);

    Str dec_text = StrInit(&alloc);
    Str hex_text = StrInit(&alloc);
    Str bin_text = StrInit(&alloc);
    Str oct_text = StrInit(&alloc);

    z = "123456789012345678901234567890";
    StrReadFmt(z, "{}", dec);
    dec_text = IntToStr(&dec);
    success  = success && (ZstrCompare(dec_text.data, "123456789012345678901234567890") == 0);

    z = "deadbeefcafebabe1234";
    StrReadFmt(z, "{x}", hex);
    hex_text = IntToHexStr(&hex);
    success  = success && (ZstrCompare(hex_text.data, "deadbeefcafebabe1234") == 0);

    z = "10100011";
    StrReadFmt(z, "{b}", bin);
    bin_text = IntToBinary(&bin);
    success  = success && (ZstrCompare(bin_text.data, "10100011") == 0);

    z = "755";
    StrReadFmt(z, "{o}", oct);
    oct_text = IntToOctStr(&oct);
    success  = success && (ZstrCompare(oct_text.data, "755") == 0);

    StrDeinit(&dec_text);
    StrDeinit(&hex_text);
    StrDeinit(&bin_text);
    StrDeinit(&oct_text);
    IntDeinit(&dec);
    IntDeinit(&hex);
    IntDeinit(&bin);
    IntDeinit(&oct);
    DefaultAllocatorDeinit(&alloc);

    return success;
}

bool test_float_reading(void) {
    WriteFmt("Testing Float reading\n");

    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    const char *z = NULL;
    bool        success = true;

    Float dec = FloatInit(alloc_base);
    Float sci = FloatInit(alloc_base);
    Float neg = FloatInit(alloc_base);

    Str dec_text = StrInit(&alloc);
    Str sci_text = StrInit(&alloc);
    Str neg_text = StrInit(&alloc);

    z = "1234567890.012345";
    StrReadFmt(z, "{}", dec);
    dec_text = FloatToStr(&dec);
    success  = success && (ZstrCompare(dec_text.data, "1234567890.012345") == 0);

    z = "1.234567e+04";
    StrReadFmt(z, "{e}", sci);
    sci_text = FloatToStr(&sci);
    success  = success && (ZstrCompare(sci_text.data, "12345.67") == 0);

    z = "-0.00125";
    StrReadFmt(z, "{}", neg);
    neg_text = FloatToStr(&neg);
    success  = success && (ZstrCompare(neg_text.data, "-0.00125") == 0);

    StrDeinit(&dec_text);
    StrDeinit(&sci_text);
    StrDeinit(&neg_text);
    FloatDeinit(&dec);
    FloatDeinit(&sci);
    FloatDeinit(&neg);
    DefaultAllocatorDeinit(&alloc);

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
        test_bitvec_reading,
        test_int_reading,
        test_float_reading
    };

    int total_tests = sizeof(tests) / sizeof(tests[0]);

    // Run all tests using the centralized test driver
    return run_test_suite(tests, total_tests, NULL, 0, "Io.Read");
}
