#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Container/BitVec.h>
#include <Misra/Std/Container/Int.h>
#include <Misra/Std/Container/Float.h>
#include <Misra/Std/Io.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Math.h>
#include <Misra/Types.h>

// Include test utilities
#include "../Util/TestRunner.h"

// Function prototypes
bool test_basic_formatting(void);
bool test_string_formatting(void);
bool test_integer_decimal_formatting(void);
bool test_integer_hex_formatting(void);
bool test_integer_binary_formatting(void);
bool test_integer_octal_formatting(void);
bool test_float_basic_formatting(void);
bool test_float_precision_formatting(void);
bool test_float_special_values(void);
bool test_width_alignment_formatting(void);
bool test_multiple_arguments(void);
bool test_error_handling(void);
bool test_char_formatting(void);
bool test_bitvec_formatting(void);
bool test_int_formatting(void);
bool test_float_formatting(void);

// Test basic formatting features
bool test_basic_formatting(void) {
    WriteFmt("Testing basic formatting\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Str  output  = StrInit(&alloc);
    bool success = true;

    // Test empty format string
    StrAppendFmt(&output, "");
    success = success && (output.length == 0);
    StrClear(&output);

    // Test literal text
    StrAppendFmt(&output, "Hello, world!");
    success = success && (ZstrCompare(output.data, "Hello, world!") == 0);
    StrClear(&output);

    // Test escaped braces
    StrAppendFmt(&output, "{{Hello}}");
    success = success && (ZstrCompare(output.data, "{Hello}") == 0);
    StrClear(&output);

    // Test double escaped braces
    StrAppendFmt(&output, "{{{{");
    success = success && (ZstrCompare(output.data, "{{") == 0);

    StrDeinit(&output);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Test string formatting
bool test_string_formatting(void) {
    WriteFmt("Testing string formatting\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Str  output  = StrInit(&alloc);
    bool success = true;

    // Test basic string
    const char *str = "Hello";
    StrAppendFmt(&output, "{}", str);
    success = success && (ZstrCompare(output.data, "Hello") == 0);
    StrClear(&output);

    // Test empty string
    const char *empty = "";
    StrAppendFmt(&output, "{}", empty);
    success = success && (output.length == 0);
    StrClear(&output);

    // Test string with width and alignment
    StrAppendFmt(&output, "{>10}", str);
    success = success && (ZstrCompare(output.data, "     Hello") == 0);
    StrClear(&output);

    StrAppendFmt(&output, "{<10}", str);
    success = success && (ZstrCompare(output.data, "Hello     ") == 0);
    StrClear(&output);

    StrAppendFmt(&output, "{^10}", str);
    success = success && (ZstrCompare(output.data, "  Hello   ") == 0);
    StrClear(&output);

    // Test Str object
    Str s = StrInitFromZstr("World", &alloc);
    StrAppendFmt(&output, "{}", s);
    success = success && (ZstrCompare(output.data, "World") == 0);
    StrDeinit(&s);

    StrDeinit(&output);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Test decimal integer formatting
bool test_integer_decimal_formatting(void) {
    WriteFmt("Testing integer decimal formatting\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Str  output  = StrInit(&alloc);
    bool success = true;

    // Test signed integers
    i8 i8_val = -42;
    StrAppendFmt(&output, "{}", i8_val);
    success = success && (ZstrCompare(output.data, "-42") == 0);
    StrClear(&output);

    i16 i16_val = -1234;
    StrAppendFmt(&output, "{}", i16_val);
    success = success && (ZstrCompare(output.data, "-1234") == 0);
    StrClear(&output);

    i32 i32_val = -123456;
    StrAppendFmt(&output, "{}", i32_val);
    success = success && (ZstrCompare(output.data, "-123456") == 0);
    StrClear(&output);

    i64 i64_val = -1234567890LL;
    StrAppendFmt(&output, "{}", i64_val);
    success = success && (ZstrCompare(output.data, "-1234567890") == 0);
    StrClear(&output);

    // Test unsigned integers
    u8 u8_val = 42;
    StrAppendFmt(&output, "{}", u8_val);
    success = success && (ZstrCompare(output.data, "42") == 0);
    StrClear(&output);

    u16 u16_val = 1234;
    StrAppendFmt(&output, "{}", u16_val);
    success = success && (ZstrCompare(output.data, "1234") == 0);
    StrClear(&output);

    u32 u32_val = 123456;
    StrAppendFmt(&output, "{}", u32_val);
    success = success && (ZstrCompare(output.data, "123456") == 0);
    StrClear(&output);

    u64 u64_val = 1234567890ULL;
    StrAppendFmt(&output, "{}", u64_val);
    success = success && (ZstrCompare(output.data, "1234567890") == 0);
    StrClear(&output);

    // Test edge cases
    i8 i8_max = 127;
    StrAppendFmt(&output, "{}", i8_max);
    success = success && (ZstrCompare(output.data, "127") == 0);
    StrClear(&output);

    i8 i8_min = -128;
    StrAppendFmt(&output, "{}", i8_min);
    success = success && (ZstrCompare(output.data, "-128") == 0);
    StrClear(&output);

    u8 u8_max = 255;
    StrAppendFmt(&output, "{}", u8_max);
    success = success && (ZstrCompare(output.data, "255") == 0);
    StrClear(&output);

    u8 u8_min = 0;
    StrAppendFmt(&output, "{}", u8_min);
    success = success && (ZstrCompare(output.data, "0") == 0);

    StrDeinit(&output);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Test hexadecimal formatting
bool test_integer_hex_formatting(void) {
    WriteFmt("Testing integer hexadecimal formatting\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Str  output  = StrInit(&alloc);
    bool success = true;

    u32 val = 0xDEADBEEF;
    StrAppendFmt(&output, "{x}", val);
    success = success && (ZstrCompare(output.data, "0xdeadbeef") == 0);
    StrClear(&output);

    StrAppendFmt(&output, "{X}", val);
    success = success && (ZstrCompare(output.data, "0xDEADBEEF") == 0);

    StrDeinit(&output);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Test binary formatting
bool test_integer_binary_formatting(void) {
    WriteFmt("Testing integer binary formatting\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Str  output  = StrInit(&alloc);
    bool success = true;

    u8 val = 0xA5; // 10100101 in binary
    StrAppendFmt(&output, "{b}", val);
    success = success && (ZstrCompare(output.data, "0b10100101") == 0);

    StrDeinit(&output);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Test octal formatting
bool test_integer_octal_formatting(void) {
    WriteFmt("Testing integer octal formatting\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Str  output  = StrInit(&alloc);
    bool success = true;

    u16 val = 0777;
    StrAppendFmt(&output, "{o}", val);
    success = success && (ZstrCompare(output.data, "0o777") == 0);

    StrDeinit(&output);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Test basic floating point formatting
bool test_float_basic_formatting(void) {
    WriteFmt("Testing basic floating point formatting\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Str  output  = StrInit(&alloc);
    bool success = true;

    f32 f32_val = 3.14159f;
    StrAppendFmt(&output, "{}", f32_val);
    success = success && (ZstrCompare(output.data, "3.141590") == 0);
    StrClear(&output);

    f64 f64_val = 2.71828;
    StrAppendFmt(&output, "{}", f64_val);
    success = success && (ZstrCompare(output.data, "2.718280") == 0);

    StrDeinit(&output);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Test floating point precision
bool test_float_precision_formatting(void) {
    WriteFmt("Testing floating point precision formatting\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Str  output  = StrInit(&alloc);
    bool success = true;

    f64 val = 3.14159265359;

    // Test different precisions
    StrAppendFmt(&output, "{.2}", val);
    success = success && (ZstrCompare(output.data, "3.14") == 0);
    StrClear(&output);

    StrAppendFmt(&output, "{.0}", val);
    success = success && (ZstrCompare(output.data, "3") == 0);
    StrClear(&output);

    StrAppendFmt(&output, "{.10}", val);
    success = success && (ZstrCompare(output.data, "3.1415926536") == 0);

    StrDeinit(&output);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Test special floating point values
bool test_float_special_values(void) {
    WriteFmt("Testing special floating point values\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Str  output  = StrInit(&alloc);
    bool success = true;

    // Test infinity
    f64 pos_inf = F64_INFINITY;
    StrAppendFmt(&output, "{}", pos_inf);
    success = success && (ZstrCompare(output.data, "inf") == 0);
    StrClear(&output);

    f64 neg_inf = -F64_INFINITY;
    StrAppendFmt(&output, "{}", neg_inf);
    success = success && (ZstrCompare(output.data, "-inf") == 0);
    StrClear(&output);

    // Test NaN
    f64 nan_val = F64_NAN;
    StrAppendFmt(&output, "{}", nan_val);
    success = success && (ZstrCompare(output.data, "nan") == 0);

    StrDeinit(&output);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Test width and alignment formatting
bool test_width_alignment_formatting(void) {
    WriteFmt("Testing width and alignment formatting\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Str  output  = StrInit(&alloc);
    bool success = true;

    // Test with integers
    i32 val = 42;
    StrAppendFmt(&output, "{5}", val);
    success = success && (ZstrCompare(output.data, "   42") == 0);
    StrClear(&output);

    StrAppendFmt(&output, "{<5}", val);
    success = success && (ZstrCompare(output.data, "42   ") == 0);
    StrClear(&output);

    StrAppendFmt(&output, "{^5}", val);
    success = success && (ZstrCompare(output.data, " 42  ") == 0);
    StrClear(&output);

    // Test with strings
    const char *str = "abc";
    StrAppendFmt(&output, "{5}", str);
    success = success && (ZstrCompare(output.data, "  abc") == 0);
    StrClear(&output);

    StrAppendFmt(&output, "{<5}", str);
    success = success && (ZstrCompare(output.data, "abc  ") == 0);
    StrClear(&output);

    StrAppendFmt(&output, "{^5}", str);
    success = success && (ZstrCompare(output.data, " abc ") == 0);

    StrDeinit(&output);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Test multiple arguments
bool test_multiple_arguments(void) {
    WriteFmt("Testing multiple arguments\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Str  output  = StrInit(&alloc);
    bool success = true;

    const char *hello = "Hello";
    i32         num   = 42;
    f64         pi    = 3.14;

    StrAppendFmt(&output, "{} {} {}", hello, num, pi);
    success = success && (ZstrCompare(output.data, "Hello 42 3.140000") == 0);
    StrClear(&output);

    // Instead of using positional arguments, we'll just reorder the arguments themselves
    StrAppendFmt(&output, "{} {} {}", pi, hello, num);
    success = success && (ZstrCompare(output.data, "3.140000 Hello 42") == 0);

    StrDeinit(&output);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Test character formatting specifiers
bool test_char_formatting(void) {
    WriteFmt("Testing character formatting specifiers\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Str  output  = StrInit(&alloc);
    bool success = true;

    // Test mixed case string with :c (preserve case)
    const char *mixed_case = "MiXeD CaSe";
    StrAppendFmt(&output, "{c}", mixed_case);
    success = success && (ZstrCompare(output.data, "MiXeD CaSe") == 0);
    StrClear(&output);

    // Test mixed case string with :a (lowercase)
    StrAppendFmt(&output, "{a}", mixed_case);
    success = success && (ZstrCompare(output.data, "mixed case") == 0);
    StrClear(&output);

    // Test mixed case string with :A (uppercase)
    StrAppendFmt(&output, "{A}", mixed_case);
    success = success && (ZstrCompare(output.data, "MIXED CASE") == 0);
    StrClear(&output);

    // Test with Str object
    Str s = StrInitFromZstr("MiXeD CaSe", &alloc);

    // Test with :c (preserve case)
    StrAppendFmt(&output, "{c}", s);
    success = success && (ZstrCompare(output.data, "MiXeD CaSe") == 0);
    StrClear(&output);

    // Test with :a (lowercase)
    StrAppendFmt(&output, "{a}", s);
    success = success && (ZstrCompare(output.data, "mixed case") == 0);
    StrClear(&output);

    // Test with :A (uppercase)
    StrAppendFmt(&output, "{A}", s);
    success = success && (ZstrCompare(output.data, "MIXED CASE") == 0);
    StrClear(&output);

    // Test with character values (u8)
    u8 upper_char = 'M';
    u8 lower_char = 'm';

    // Test uppercase char with :c (preserve case)
    StrAppendFmt(&output, "{c}", upper_char);
    success = success && (ZstrCompare(output.data, "M") == 0);
    StrClear(&output);

    // Test uppercase char with :a (lowercase)
    StrAppendFmt(&output, "{a}", upper_char);
    success = success && (ZstrCompare(output.data, "m") == 0);
    StrClear(&output);

    // Test lowercase char with :A (uppercase)
    StrAppendFmt(&output, "{A}", lower_char);
    success = success && (ZstrCompare(output.data, "M") == 0);
    StrClear(&output);

    // Test with u16 (containing ASCII values)
    u16 u16_value = ('A' << 8) | 'B'; // AB in big-endian

    // Test u16 with :c (preserve case)
    StrAppendFmt(&output, "{c}", u16_value);
    success = success && (output.length == 2 && output.data[0] == 'A' && output.data[1] == 'B');
    StrClear(&output);

    // Test u16 with :a (lowercase)
    StrAppendFmt(&output, "{a}", u16_value);
    success = success && (output.length == 2 && output.data[0] == 'a' && output.data[1] == 'b');
    StrClear(&output);

    // Test u16 with :A (uppercase)
    StrAppendFmt(&output, "{A}", u16_value);
    success = success && (output.length == 2 && output.data[0] == 'A' && output.data[1] == 'B');
    StrClear(&output);

    // Test with i16 (containing ASCII values)
    i16 i16_value = ('C' << 8) | 'd'; // Cd in big-endian

    // Test i16 with :c (preserve case)
    StrAppendFmt(&output, "{c}", i16_value);
    success = success && (output.length == 2 && output.data[0] == 'C' && output.data[1] == 'd');
    StrClear(&output);

    // Test i16 with :a (lowercase)
    StrAppendFmt(&output, "{a}", i16_value);
    success = success && (output.length == 2 && output.data[0] == 'c' && output.data[1] == 'd');
    StrClear(&output);

    // Test i16 with :A (uppercase)
    StrAppendFmt(&output, "{A}", i16_value);
    success = success && (output.length == 2 && output.data[0] == 'C' && output.data[1] == 'D');
    StrClear(&output);

    // Test with u32 (containing ASCII values)
    u32 u32_value = ('E' << 24) | ('f' << 16) | ('G' << 8) | 'h'; // EfGh in big-endian

    // Test u32 with :c (preserve case)
    StrAppendFmt(&output, "{c}", u32_value);
    success = success && (output.length == 4 && output.data[0] == 'E' && output.data[1] == 'f' &&
                          output.data[2] == 'G' && output.data[3] == 'h');
    StrClear(&output);

    // Test u32 with :a (lowercase)
    StrAppendFmt(&output, "{a}", u32_value);
    success = success && (output.length == 4 && output.data[0] == 'e' && output.data[1] == 'f' &&
                          output.data[2] == 'g' && output.data[3] == 'h');
    StrClear(&output);

    // Test u32 with :A (uppercase)
    StrAppendFmt(&output, "{A}", u32_value);
    success = success && (output.length == 4 && output.data[0] == 'E' && output.data[1] == 'F' &&
                          output.data[2] == 'G' && output.data[3] == 'H');
    StrClear(&output);

    // Test with i32 (containing ASCII values)
    i32 i32_value = ('I' << 24) | ('j' << 16) | ('K' << 8) | 'l'; // IjKl in big-endian

    // Test i32 with :c (preserve case)
    StrAppendFmt(&output, "{c}", i32_value);
    success = success && (output.length == 4 && output.data[0] == 'I' && output.data[1] == 'j' &&
                          output.data[2] == 'K' && output.data[3] == 'l');
    StrClear(&output);

    // Test i32 with :a (lowercase)
    StrAppendFmt(&output, "{a}", i32_value);
    success = success && (output.length == 4 && output.data[0] == 'i' && output.data[1] == 'j' &&
                          output.data[2] == 'k' && output.data[3] == 'l');
    StrClear(&output);

    // Test i32 with :A (uppercase)
    StrAppendFmt(&output, "{A}", i32_value);
    success = success && (output.length == 4 && output.data[0] == 'I' && output.data[1] == 'J' &&
                          output.data[2] == 'K' && output.data[3] == 'L');
    StrClear(&output);

    // Test with u64 (containing ASCII values)
    u64 u64_value = ((u64)'M' << 56) | ((u64)'n' << 48) | ((u64)'O' << 40) | ((u64)'p' << 32) | ('Q' << 24) |
                    ('r' << 16) | ('S' << 8) | 't'; // MnOpQrSt in big-endian

    // Test u64 with :c (preserve case)
    StrAppendFmt(&output, "{c}", u64_value);
    success = success && (output.length == 8 && output.data[0] == 'M' && output.data[1] == 'n' &&
                          output.data[2] == 'O' && output.data[3] == 'p' && output.data[4] == 'Q' &&
                          output.data[5] == 'r' && output.data[6] == 'S' && output.data[7] == 't');
    StrClear(&output);

    // Test u64 with :a (lowercase)
    StrAppendFmt(&output, "{a}", u64_value);
    success = success && (output.length == 8 && output.data[0] == 'm' && output.data[1] == 'n' &&
                          output.data[2] == 'o' && output.data[3] == 'p' && output.data[4] == 'q' &&
                          output.data[5] == 'r' && output.data[6] == 's' && output.data[7] == 't');
    StrClear(&output);

    // Test u64 with :A (uppercase)
    StrAppendFmt(&output, "{A}", u64_value);
    success = success && (output.length == 8 && output.data[0] == 'M' && output.data[1] == 'N' &&
                          output.data[2] == 'O' && output.data[3] == 'P' && output.data[4] == 'Q' &&
                          output.data[5] == 'R' && output.data[6] == 'S' && output.data[7] == 'T');
    StrClear(&output);

    // Test with i64 (containing ASCII values)
    i64 i64_value = ((i64)'U' << 56) | ((i64)'v' << 48) | ((i64)'W' << 40) | ((i64)'x' << 32) | ('Y' << 24) |
                    ('z' << 16) | ('1' << 8) | '2'; // UvWxYz12 in big-endian

    // Test i64 with :c (preserve case)
    StrAppendFmt(&output, "{c}", i64_value);
    success = success && (output.length == 8 && output.data[0] == 'U' && output.data[1] == 'v' &&
                          output.data[2] == 'W' && output.data[3] == 'x' && output.data[4] == 'Y' &&
                          output.data[5] == 'z' && output.data[6] == '1' && output.data[7] == '2');
    StrClear(&output);

    // Test i64 with :a (lowercase)
    StrAppendFmt(&output, "{a}", i64_value);
    success = success && (output.length == 8 && output.data[0] == 'u' && output.data[1] == 'v' &&
                          output.data[2] == 'w' && output.data[3] == 'x' && output.data[4] == 'y' &&
                          output.data[5] == 'z' && output.data[6] == '1' && output.data[7] == '2');
    StrClear(&output);

    // Test i64 with :A (uppercase)
    StrAppendFmt(&output, "{A}", i64_value);
    success = success && (output.length == 8 && output.data[0] == 'U' && output.data[1] == 'V' &&
                          output.data[2] == 'W' && output.data[3] == 'X' && output.data[4] == 'Y' &&
                          output.data[5] == 'Z' && output.data[6] == '1' && output.data[7] == '2');

    StrDeinit(&output);
    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Test BitVec formatting
bool test_bitvec_formatting(void) {
    WriteFmt("Testing BitVec formatting\n");

    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    Str  output  = StrInit(&alloc);
    bool success = true;

    // Test 1: Basic binary formatting
    BitVec bv1 = BitVecFromStr("10110", alloc_base);
    StrAppendFmt(&output, "{}", bv1);
    success = success && (ZstrCompare(output.data, "10110") == 0);
    StrClear(&output);

    // Test 2: Empty BitVec
    BitVec bv_empty = BitVecInit(alloc_base);
    StrAppendFmt(&output, "{}", bv_empty);
    success = success && (output.length == 0);
    StrClear(&output);

    // Test 3: Hex formatting
    BitVec bv2 = BitVecFromInteger(0xABCD, 16, alloc_base);
    StrAppendFmt(&output, "{x}", bv2);
    success = success && (ZstrCompare(output.data, "0xabcd") == 0);
    StrClear(&output);

    // Test 4: Uppercase hex formatting
    StrAppendFmt(&output, "{X}", bv2);
    success = success && (ZstrCompare(output.data, "0xABCD") == 0);
    StrClear(&output);

    // Test 5: Octal formatting
    BitVec bv3 = BitVecFromInteger(0755, 10, alloc_base);
    StrAppendFmt(&output, "{o}", bv3);
    success = success && (ZstrCompare(output.data, "0o755") == 0);
    StrClear(&output);

    // Test 6: Width and alignment
    StrAppendFmt(&output, "{>10}", bv1);
    success = success && (ZstrCompare(output.data, "     10110") == 0);
    StrClear(&output);

    StrAppendFmt(&output, "{<10}", bv1);
    success = success && (ZstrCompare(output.data, "10110     ") == 0);
    StrClear(&output);

    StrAppendFmt(&output, "{^10}", bv1);
    success = success && (ZstrCompare(output.data, "  10110   ") == 0);
    StrClear(&output);

    // Test 7: Zero value
    BitVec bv_zero = BitVecFromInteger(0, 1, alloc_base);
    StrAppendFmt(&output, "{x}", bv_zero);
    success = success && (ZstrCompare(output.data, "0x0") == 0);
    StrClear(&output);

    StrAppendFmt(&output, "{o}", bv_zero);
    success = success && (ZstrCompare(output.data, "0o0") == 0);
    StrClear(&output);

    // Cleanup
    BitVecDeinit(&bv1);
    BitVecDeinit(&bv_empty);
    BitVecDeinit(&bv2);
    BitVecDeinit(&bv3);
    BitVecDeinit(&bv_zero);
    StrDeinit(&output);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

bool test_int_formatting(void) {
    WriteFmt("Testing Int formatting\n");

    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    Str  output  = StrInit(&alloc);
    bool success = true;

    Int big_dec = IntFromStr("123456789012345678901234567890", alloc_base);
    Int hex_val = IntFromHexStr("deadbeefcafebabe1234", alloc_base);
    Int bin_val = IntFromBinary("10100011", alloc_base);
    Int oct_val = IntFrom(493, alloc_base);

    StrAppendFmt(&output, "{}", big_dec);
    success = success && (ZstrCompare(output.data, "123456789012345678901234567890") == 0);
    StrClear(&output);

    StrAppendFmt(&output, "{x}", hex_val);
    success = success && (ZstrCompare(output.data, "deadbeefcafebabe1234") == 0);
    StrClear(&output);

    StrAppendFmt(&output, "{X}", hex_val);
    success = success && (ZstrCompare(output.data, "DEADBEEFCAFEBABE1234") == 0);
    StrClear(&output);

    StrAppendFmt(&output, "{b}", bin_val);
    success = success && (ZstrCompare(output.data, "10100011") == 0);
    StrClear(&output);

    StrAppendFmt(&output, "{o}", oct_val);
    success = success && (ZstrCompare(output.data, "755") == 0);
    StrClear(&output);

    StrAppendFmt(&output, "{>34}", big_dec);
    success = success && (ZstrCompare(output.data, "    123456789012345678901234567890") == 0);

    IntDeinit(&big_dec);
    IntDeinit(&hex_val);
    IntDeinit(&bin_val);
    IntDeinit(&oct_val);
    StrDeinit(&output);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

bool test_float_formatting(void) {
    WriteFmt("Testing Float formatting\n");

    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    Str   output  = StrInit(&alloc);
    bool  success = true;
    Float exact   = FloatFromStr("1234567890.012345", alloc_base);
    Float sci     = FloatFromStr("12345.67", alloc_base);
    Float short_v = FloatFromStr("1.2", alloc_base);

    StrAppendFmt(&output, "{}", exact);
    success = success && (ZstrCompare(output.data, "1234567890.012345") == 0);
    StrClear(&output);

    StrAppendFmt(&output, "{e}", sci);
    success = success && (ZstrCompare(output.data, "1.234567e+04") == 0);
    StrClear(&output);

    StrAppendFmt(&output, "{E}", sci);
    success = success && (ZstrCompare(output.data, "1.234567E+04") == 0);
    StrClear(&output);

    StrAppendFmt(&output, "{.3}", short_v);
    success = success && (ZstrCompare(output.data, "1.200") == 0);
    StrClear(&output);

    StrAppendFmt(&output, "{>18}", sci);
    success = success && (ZstrCompare(output.data, "          12345.67") == 0);

    FloatDeinit(&exact);
    FloatDeinit(&sci);
    FloatDeinit(&short_v);
    StrDeinit(&output);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// // Test error handling
// bool test_error_handling(void) {
//     WriteFmt("Testing error handling\n");

//     // Since we can't directly test error cases without causing program termination,
//     // we'll just report success here. In a real-world scenario, we would need
//     // a more sophisticated approach to test error handling, such as:
//     // 1. Using a separate process that can fail
//     // 2. Capturing logs to verify error messages
//     // 3. Mocking the error handling functions

//     WriteFmt("Note: Error handling tests are skipped as they would cause program termination\n");
//     WriteFmt("In a real-world scenario, these would be tested with a more robust framework\n");

//     // All tests are considered passing since we can't properly test them
//     return true;
// }

// `StrWriteFmt` is the clear-then-append form: prior contents of the
// destination Str must be discarded before the formatted output lands.
bool test_str_write_fmt_clears(void);
bool test_str_write_fmt_clears(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              s     = StrInit(&alloc);
    StrAppendFmt(&s, "old prefix ");
    StrWriteFmt(&s, "fresh {}", LVAL(42));
    bool ok = (s.length == 8) && (s.data[0] == 'f') && (s.data[s.length - 1] == '2');
    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// `StrPatchFmt` overwrites existing bytes at an offset. Length stays
// constant on success; out-of-range writes fail and leave the Str
// untouched.
bool test_str_patch_fmt(void);
bool test_str_patch_fmt(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              s     = StrInit(&alloc);
    StrAppendFmt(&s, "AAAAAAAA");
    size before_length = s.length;
    bool ok            = StrPatchFmt(&s, 2, "{}", LVAL(1234));
    ok                 = ok && s.length == before_length;
    ok                 = ok && s.data[0] == 'A' && s.data[1] == 'A';
    ok                 = ok && s.data[2] == '1' && s.data[3] == '2' && s.data[4] == '3' && s.data[5] == '4';
    ok                 = ok && s.data[6] == 'A' && s.data[7] == 'A';

    // Patch that would extend past the end must fail.
    ok = ok && !StrPatchFmt(&s, 6, "{}", LVAL(9999));
    ok = ok && s.length == before_length;
    ok = ok && s.data[6] == 'A' && s.data[7] == 'A';

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Main function that runs all tests
int main(void) {
    WriteFmt("[INFO] Starting format writer tests\n\n");

    // Array of test functions
    TestFunction tests[] = {
        test_basic_formatting,
        test_string_formatting,
        test_integer_decimal_formatting,
        test_integer_hex_formatting,
        test_integer_binary_formatting,
        test_integer_octal_formatting,
        test_float_basic_formatting,
        test_float_precision_formatting,
        test_float_special_values,
        test_width_alignment_formatting,
        test_multiple_arguments,
        test_char_formatting,
        test_bitvec_formatting,
        test_int_formatting,
        test_float_formatting,
        test_str_write_fmt_clears,
        test_str_patch_fmt
        // test_error_handling
    };

    int total_tests = sizeof(tests) / sizeof(tests[0]);

    // Run all tests using the centralized test driver
    return run_test_suite(tests, total_tests, NULL, 0, "Io.Write");
}
