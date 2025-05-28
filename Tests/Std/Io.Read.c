#include <Misra/Std/Io.h>
#include <Misra/Std/Log.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <math.h>  // For fabs()

// Define epsilon for float comparisons
#define FLOAT_EPSILON 1e-6
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
    success = success && (StrCmp(&s, &expected) == 0);
    StrDeinit(&expected);
    StrClear(&s);
    
    // Test quoted string reading
    StrReadFmt("\"Hello, World!\"", "{}", FMT(s));
    
    expected = StrInitFromZstr("Hello, World!");
    success = success && (StrCmp(&s, &expected) == 0);
    StrDeinit(&expected);
    
    StrDeinit(&s);
    
    return success;
}

// Test reading multiple arguments
bool test_multiple_arguments_reading(void) {
    printf("Testing multiple arguments reading\n");
    
    bool success = true;
    
    i32 num = 0;
    Str name = StrInit();
    StrReadFmt("Count: 42, Name: Alice", "Count: {}, Name: {}", FMT(num), FMT(name));
    
    success = success && (num == 42);
    
    Str expected = StrInitFromZstr("Alice");
    success = success && (StrCmp(&name, &expected) == 0);
    StrDeinit(&expected);
    StrClear(&name);
    
    // Test with different order
    f64 val = 0.0;
    StrReadFmt("Value: 3.14, Name: Bob", "Value: {}, Name: {}", FMT(val), FMT(name));
    
    success = success && double_equals(val, 3.14);
    
    expected = StrInitFromZstr("Bob");
    success = success && (StrCmp(&name, &expected) == 0);
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

// Main function that runs all tests
int main(void) {
    printf("[INFO] Starting format reader tests\n\n");
    
    // Array of test functions
    bool (*tests[])(void) = {
        test_integer_decimal_reading,
        test_integer_hex_reading,
        test_integer_binary_reading,
        test_integer_octal_reading,
        test_float_basic_reading,
        test_float_scientific_reading,
        test_string_reading,
        test_multiple_arguments_reading,
        test_error_handling_reading
    };
    
    int total_tests = sizeof(tests) / sizeof(tests[0]);
    int passed = 0;
    int failed = 0;
    
    // Run all tests and accumulate results
    for (int i = 0; i < total_tests; i++) {
        printf("[TEST %d/%d] ", i + 1, total_tests);
        bool result = tests[i]();
        if (result) {
            printf("[PASS]\n\n");
            passed++;
        } else {
            printf("[FAIL]\n\n");
            failed++;
        }
    }
    
    // Print summary
    printf("[SUMMARY] Total: %d, Passed: %d, Failed: %d\n", total_tests, passed, failed);
    
    // Return non-zero exit code if any test failed
    return failed > 0 ? 1 : 0;
} 
