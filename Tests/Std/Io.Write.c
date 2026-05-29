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

#include "../Util/TestRunner.h"

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
bool test_char_formatting(void);
bool test_bitvec_formatting(void);
bool test_int_formatting(void);
bool test_float_formatting(void);

bool test_basic_formatting(void) {
    WriteFmt("Testing basic formatting\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Str  output  = StrInit(&alloc);
    bool success = true;

    StrAppendFmt(&output, "");
    success = success && (StrLen(&output) == 0);
    StrClear(&output);

    StrAppendFmt(&output, "Hello, world!");
    success = success && (ZstrCompare(StrBegin(&output), "Hello, world!") == 0);
    StrClear(&output);

    StrAppendFmt(&output, "{{Hello}}");
    success = success && (ZstrCompare(StrBegin(&output), "{Hello}") == 0);
    StrClear(&output);

    StrAppendFmt(&output, "{{{{");
    success = success && (ZstrCompare(StrBegin(&output), "{{") == 0);

    StrDeinit(&output);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

bool test_string_formatting(void) {
    WriteFmt("Testing string formatting\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Str  output  = StrInit(&alloc);
    bool success = true;

    Zstr str = "Hello";
    StrAppendFmt(&output, "{}", str);
    success = success && (ZstrCompare(StrBegin(&output), "Hello") == 0);
    StrClear(&output);

    Zstr empty = "";
    StrAppendFmt(&output, "{}", empty);
    success = success && (StrLen(&output) == 0);
    StrClear(&output);

    StrAppendFmt(&output, "{>10}", str);
    success = success && (ZstrCompare(StrBegin(&output), "     Hello") == 0);
    StrClear(&output);

    StrAppendFmt(&output, "{<10}", str);
    success = success && (ZstrCompare(StrBegin(&output), "Hello     ") == 0);
    StrClear(&output);

    StrAppendFmt(&output, "{^10}", str);
    success = success && (ZstrCompare(StrBegin(&output), "  Hello   ") == 0);
    StrClear(&output);

    Str s = StrInitFromZstr("World", &alloc);
    StrAppendFmt(&output, "{}", s);
    success = success && (ZstrCompare(StrBegin(&output), "World") == 0);
    StrDeinit(&s);

    StrDeinit(&output);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

bool test_integer_decimal_formatting(void) {
    WriteFmt("Testing integer decimal formatting\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Str  output  = StrInit(&alloc);
    bool success = true;

    i8 i8_val = -42;
    StrAppendFmt(&output, "{}", i8_val);
    success = success && (ZstrCompare(StrBegin(&output), "-42") == 0);
    StrClear(&output);

    i16 i16_val = -1234;
    StrAppendFmt(&output, "{}", i16_val);
    success = success && (ZstrCompare(StrBegin(&output), "-1234") == 0);
    StrClear(&output);

    i32 i32_val = -123456;
    StrAppendFmt(&output, "{}", i32_val);
    success = success && (ZstrCompare(StrBegin(&output), "-123456") == 0);
    StrClear(&output);

    i64 i64_val = -1234567890LL;
    StrAppendFmt(&output, "{}", i64_val);
    success = success && (ZstrCompare(StrBegin(&output), "-1234567890") == 0);
    StrClear(&output);

    u8 u8_val = 42;
    StrAppendFmt(&output, "{}", u8_val);
    success = success && (ZstrCompare(StrBegin(&output), "42") == 0);
    StrClear(&output);

    u16 u16_val = 1234;
    StrAppendFmt(&output, "{}", u16_val);
    success = success && (ZstrCompare(StrBegin(&output), "1234") == 0);
    StrClear(&output);

    u32 u32_val = 123456;
    StrAppendFmt(&output, "{}", u32_val);
    success = success && (ZstrCompare(StrBegin(&output), "123456") == 0);
    StrClear(&output);

    u64 u64_val = 1234567890ULL;
    StrAppendFmt(&output, "{}", u64_val);
    success = success && (ZstrCompare(StrBegin(&output), "1234567890") == 0);
    StrClear(&output);

    i8 i8_max = 127;
    StrAppendFmt(&output, "{}", i8_max);
    success = success && (ZstrCompare(StrBegin(&output), "127") == 0);
    StrClear(&output);

    i8 i8_min = -128;
    StrAppendFmt(&output, "{}", i8_min);
    success = success && (ZstrCompare(StrBegin(&output), "-128") == 0);
    StrClear(&output);

    u8 u8_max = 255;
    StrAppendFmt(&output, "{}", u8_max);
    success = success && (ZstrCompare(StrBegin(&output), "255") == 0);
    StrClear(&output);

    u8 u8_min = 0;
    StrAppendFmt(&output, "{}", u8_min);
    success = success && (ZstrCompare(StrBegin(&output), "0") == 0);

    StrDeinit(&output);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

bool test_integer_hex_formatting(void) {
    WriteFmt("Testing integer hexadecimal formatting\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Str  output  = StrInit(&alloc);
    bool success = true;

    u32 val = 0xDEADBEEF;
    StrAppendFmt(&output, "{x}", val);
    success = success && (ZstrCompare(StrBegin(&output), "0xdeadbeef") == 0);
    StrClear(&output);

    StrAppendFmt(&output, "{X}", val);
    success = success && (ZstrCompare(StrBegin(&output), "0xDEADBEEF") == 0);

    StrDeinit(&output);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

bool test_integer_binary_formatting(void) {
    WriteFmt("Testing integer binary formatting\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Str  output  = StrInit(&alloc);
    bool success = true;

    u8 val = 0xA5; // 10100101 in binary
    StrAppendFmt(&output, "{b}", val);
    success = success && (ZstrCompare(StrBegin(&output), "0b10100101") == 0);

    StrDeinit(&output);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

bool test_integer_octal_formatting(void) {
    WriteFmt("Testing integer octal formatting\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Str  output  = StrInit(&alloc);
    bool success = true;

    u16 val = 0777;
    StrAppendFmt(&output, "{o}", val);
    success = success && (ZstrCompare(StrBegin(&output), "0o777") == 0);

    StrDeinit(&output);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

bool test_float_basic_formatting(void) {
    WriteFmt("Testing basic floating point formatting\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Str  output  = StrInit(&alloc);
    bool success = true;

    f32 f32_val = 3.14159f;
    StrAppendFmt(&output, "{}", f32_val);
    success = success && (ZstrCompare(StrBegin(&output), "3.141590") == 0);
    StrClear(&output);

    f64 f64_val = 2.71828;
    StrAppendFmt(&output, "{}", f64_val);
    success = success && (ZstrCompare(StrBegin(&output), "2.718280") == 0);

    StrDeinit(&output);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

bool test_float_precision_formatting(void) {
    WriteFmt("Testing floating point precision formatting\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Str  output  = StrInit(&alloc);
    bool success = true;

    f64 val = 3.14159265359;

    StrAppendFmt(&output, "{.2}", val);
    success = success && (ZstrCompare(StrBegin(&output), "3.14") == 0);
    StrClear(&output);

    StrAppendFmt(&output, "{.0}", val);
    success = success && (ZstrCompare(StrBegin(&output), "3") == 0);
    StrClear(&output);

    StrAppendFmt(&output, "{.10}", val);
    success = success && (ZstrCompare(StrBegin(&output), "3.1415926536") == 0);

    StrDeinit(&output);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

bool test_float_special_values(void) {
    WriteFmt("Testing special floating point values\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Str  output  = StrInit(&alloc);
    bool success = true;

    f64 pos_inf = F64_INFINITY;
    StrAppendFmt(&output, "{}", pos_inf);
    success = success && (ZstrCompare(StrBegin(&output), "inf") == 0);
    StrClear(&output);

    f64 neg_inf = -F64_INFINITY;
    StrAppendFmt(&output, "{}", neg_inf);
    success = success && (ZstrCompare(StrBegin(&output), "-inf") == 0);
    StrClear(&output);

    f64 nan_val = F64_NAN;
    StrAppendFmt(&output, "{}", nan_val);
    success = success && (ZstrCompare(StrBegin(&output), "nan") == 0);

    StrDeinit(&output);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

bool test_width_alignment_formatting(void) {
    WriteFmt("Testing width and alignment formatting\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Str  output  = StrInit(&alloc);
    bool success = true;

    i32 val = 42;
    StrAppendFmt(&output, "{5}", val);
    success = success && (ZstrCompare(StrBegin(&output), "   42") == 0);
    StrClear(&output);

    StrAppendFmt(&output, "{<5}", val);
    success = success && (ZstrCompare(StrBegin(&output), "42   ") == 0);
    StrClear(&output);

    StrAppendFmt(&output, "{^5}", val);
    success = success && (ZstrCompare(StrBegin(&output), " 42  ") == 0);
    StrClear(&output);

    Zstr str = "abc";
    StrAppendFmt(&output, "{5}", str);
    success = success && (ZstrCompare(StrBegin(&output), "  abc") == 0);
    StrClear(&output);

    StrAppendFmt(&output, "{<5}", str);
    success = success && (ZstrCompare(StrBegin(&output), "abc  ") == 0);
    StrClear(&output);

    StrAppendFmt(&output, "{^5}", str);
    success = success && (ZstrCompare(StrBegin(&output), " abc ") == 0);

    StrDeinit(&output);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

bool test_multiple_arguments(void) {
    WriteFmt("Testing multiple arguments\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Str  output  = StrInit(&alloc);
    bool success = true;

    Zstr hello = "Hello";
    i32  num   = 42;
    f64  pi    = 3.14;

    StrAppendFmt(&output, "{} {} {}", hello, num, pi);
    success = success && (ZstrCompare(StrBegin(&output), "Hello 42 3.140000") == 0);
    StrClear(&output);

    StrAppendFmt(&output, "{} {} {}", pi, hello, num);
    success = success && (ZstrCompare(StrBegin(&output), "3.140000 Hello 42") == 0);

    StrDeinit(&output);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

bool test_char_formatting(void) {
    WriteFmt("Testing character formatting specifiers\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Str  output  = StrInit(&alloc);
    bool success = true;

    Zstr mixed_case = "MiXeD CaSe";
    StrAppendFmt(&output, "{c}", mixed_case);
    success = success && (ZstrCompare(StrBegin(&output), "MiXeD CaSe") == 0);
    StrClear(&output);

    StrAppendFmt(&output, "{a}", mixed_case);
    success = success && (ZstrCompare(StrBegin(&output), "mixed case") == 0);
    StrClear(&output);

    StrAppendFmt(&output, "{A}", mixed_case);
    success = success && (ZstrCompare(StrBegin(&output), "MIXED CASE") == 0);
    StrClear(&output);

    Str s = StrInitFromZstr("MiXeD CaSe", &alloc);

    StrAppendFmt(&output, "{c}", s);
    success = success && (ZstrCompare(StrBegin(&output), "MiXeD CaSe") == 0);
    StrClear(&output);

    StrAppendFmt(&output, "{a}", s);
    success = success && (ZstrCompare(StrBegin(&output), "mixed case") == 0);
    StrClear(&output);

    StrAppendFmt(&output, "{A}", s);
    success = success && (ZstrCompare(StrBegin(&output), "MIXED CASE") == 0);
    StrClear(&output);

    u8 upper_char = 'M';
    u8 lower_char = 'm';

    StrAppendFmt(&output, "{c}", upper_char);
    success = success && (ZstrCompare(StrBegin(&output), "M") == 0);
    StrClear(&output);

    StrAppendFmt(&output, "{a}", upper_char);
    success = success && (ZstrCompare(StrBegin(&output), "m") == 0);
    StrClear(&output);

    StrAppendFmt(&output, "{A}", lower_char);
    success = success && (ZstrCompare(StrBegin(&output), "M") == 0);
    StrClear(&output);

    u16 u16_value = ('A' << 8) | 'B'; // big-endian "AB"

    StrAppendFmt(&output, "{c}", u16_value);
    success = success && (StrLen(&output) == 2 && StrBegin(&output)[0] == 'A' && StrBegin(&output)[1] == 'B');
    StrClear(&output);

    StrAppendFmt(&output, "{a}", u16_value);
    success = success && (StrLen(&output) == 2 && StrBegin(&output)[0] == 'a' && StrBegin(&output)[1] == 'b');
    StrClear(&output);

    StrAppendFmt(&output, "{A}", u16_value);
    success = success && (StrLen(&output) == 2 && StrBegin(&output)[0] == 'A' && StrBegin(&output)[1] == 'B');
    StrClear(&output);

    i16 i16_value = ('C' << 8) | 'd'; // big-endian "Cd"

    StrAppendFmt(&output, "{c}", i16_value);
    success = success && (StrLen(&output) == 2 && StrBegin(&output)[0] == 'C' && StrBegin(&output)[1] == 'd');
    StrClear(&output);

    StrAppendFmt(&output, "{a}", i16_value);
    success = success && (StrLen(&output) == 2 && StrBegin(&output)[0] == 'c' && StrBegin(&output)[1] == 'd');
    StrClear(&output);

    StrAppendFmt(&output, "{A}", i16_value);
    success = success && (StrLen(&output) == 2 && StrBegin(&output)[0] == 'C' && StrBegin(&output)[1] == 'D');
    StrClear(&output);

    u32 u32_value = ('E' << 24) | ('f' << 16) | ('G' << 8) | 'h'; // big-endian "EfGh"

    StrAppendFmt(&output, "{c}", u32_value);
    success = success && (StrLen(&output) == 4 && StrBegin(&output)[0] == 'E' && StrBegin(&output)[1] == 'f' &&
                          StrBegin(&output)[2] == 'G' && StrBegin(&output)[3] == 'h');
    StrClear(&output);

    StrAppendFmt(&output, "{a}", u32_value);
    success = success && (StrLen(&output) == 4 && StrBegin(&output)[0] == 'e' && StrBegin(&output)[1] == 'f' &&
                          StrBegin(&output)[2] == 'g' && StrBegin(&output)[3] == 'h');
    StrClear(&output);

    StrAppendFmt(&output, "{A}", u32_value);
    success = success && (StrLen(&output) == 4 && StrBegin(&output)[0] == 'E' && StrBegin(&output)[1] == 'F' &&
                          StrBegin(&output)[2] == 'G' && StrBegin(&output)[3] == 'H');
    StrClear(&output);

    i32 i32_value = ('I' << 24) | ('j' << 16) | ('K' << 8) | 'l'; // big-endian "IjKl"

    StrAppendFmt(&output, "{c}", i32_value);
    success = success && (StrLen(&output) == 4 && StrBegin(&output)[0] == 'I' && StrBegin(&output)[1] == 'j' &&
                          StrBegin(&output)[2] == 'K' && StrBegin(&output)[3] == 'l');
    StrClear(&output);

    StrAppendFmt(&output, "{a}", i32_value);
    success = success && (StrLen(&output) == 4 && StrBegin(&output)[0] == 'i' && StrBegin(&output)[1] == 'j' &&
                          StrBegin(&output)[2] == 'k' && StrBegin(&output)[3] == 'l');
    StrClear(&output);

    StrAppendFmt(&output, "{A}", i32_value);
    success = success && (StrLen(&output) == 4 && StrBegin(&output)[0] == 'I' && StrBegin(&output)[1] == 'J' &&
                          StrBegin(&output)[2] == 'K' && StrBegin(&output)[3] == 'L');
    StrClear(&output);

    u64 u64_value = ((u64)'M' << 56) | ((u64)'n' << 48) | ((u64)'O' << 40) | ((u64)'p' << 32) | ('Q' << 24) |
                    ('r' << 16) | ('S' << 8) | 't'; // big-endian "MnOpQrSt"

    StrAppendFmt(&output, "{c}", u64_value);
    success = success && (StrLen(&output) == 8 && StrBegin(&output)[0] == 'M' && StrBegin(&output)[1] == 'n' &&
                          StrBegin(&output)[2] == 'O' && StrBegin(&output)[3] == 'p' && StrBegin(&output)[4] == 'Q' &&
                          StrBegin(&output)[5] == 'r' && StrBegin(&output)[6] == 'S' && StrBegin(&output)[7] == 't');
    StrClear(&output);

    StrAppendFmt(&output, "{a}", u64_value);
    success = success && (StrLen(&output) == 8 && StrBegin(&output)[0] == 'm' && StrBegin(&output)[1] == 'n' &&
                          StrBegin(&output)[2] == 'o' && StrBegin(&output)[3] == 'p' && StrBegin(&output)[4] == 'q' &&
                          StrBegin(&output)[5] == 'r' && StrBegin(&output)[6] == 's' && StrBegin(&output)[7] == 't');
    StrClear(&output);

    StrAppendFmt(&output, "{A}", u64_value);
    success = success && (StrLen(&output) == 8 && StrBegin(&output)[0] == 'M' && StrBegin(&output)[1] == 'N' &&
                          StrBegin(&output)[2] == 'O' && StrBegin(&output)[3] == 'P' && StrBegin(&output)[4] == 'Q' &&
                          StrBegin(&output)[5] == 'R' && StrBegin(&output)[6] == 'S' && StrBegin(&output)[7] == 'T');
    StrClear(&output);

    i64 i64_value = ((i64)'U' << 56) | ((i64)'v' << 48) | ((i64)'W' << 40) | ((i64)'x' << 32) | ('Y' << 24) |
                    ('z' << 16) | ('1' << 8) | '2'; // big-endian "UvWxYz12"

    StrAppendFmt(&output, "{c}", i64_value);
    success = success && (StrLen(&output) == 8 && StrBegin(&output)[0] == 'U' && StrBegin(&output)[1] == 'v' &&
                          StrBegin(&output)[2] == 'W' && StrBegin(&output)[3] == 'x' && StrBegin(&output)[4] == 'Y' &&
                          StrBegin(&output)[5] == 'z' && StrBegin(&output)[6] == '1' && StrBegin(&output)[7] == '2');
    StrClear(&output);

    StrAppendFmt(&output, "{a}", i64_value);
    success = success && (StrLen(&output) == 8 && StrBegin(&output)[0] == 'u' && StrBegin(&output)[1] == 'v' &&
                          StrBegin(&output)[2] == 'w' && StrBegin(&output)[3] == 'x' && StrBegin(&output)[4] == 'y' &&
                          StrBegin(&output)[5] == 'z' && StrBegin(&output)[6] == '1' && StrBegin(&output)[7] == '2');
    StrClear(&output);

    StrAppendFmt(&output, "{A}", i64_value);
    success = success && (StrLen(&output) == 8 && StrBegin(&output)[0] == 'U' && StrBegin(&output)[1] == 'V' &&
                          StrBegin(&output)[2] == 'W' && StrBegin(&output)[3] == 'X' && StrBegin(&output)[4] == 'Y' &&
                          StrBegin(&output)[5] == 'Z' && StrBegin(&output)[6] == '1' && StrBegin(&output)[7] == '2');

    StrDeinit(&output);
    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

bool test_bitvec_formatting(void) {
    WriteFmt("Testing BitVec formatting\n");

    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    Str  output  = StrInit(&alloc);
    bool success = true;

    BitVec bv1 = BitVecFromStr("10110", alloc_base);
    StrAppendFmt(&output, "{}", bv1);
    success = success && (ZstrCompare(StrBegin(&output), "10110") == 0);
    StrClear(&output);

    BitVec bv_empty = BitVecInit(alloc_base);
    StrAppendFmt(&output, "{}", bv_empty);
    success = success && (StrLen(&output) == 0);
    StrClear(&output);

    BitVec bv2 = BitVecFromInteger(0xABCD, 16, alloc_base);
    StrAppendFmt(&output, "{x}", bv2);
    success = success && (ZstrCompare(StrBegin(&output), "0xabcd") == 0);
    StrClear(&output);

    StrAppendFmt(&output, "{X}", bv2);
    success = success && (ZstrCompare(StrBegin(&output), "0xABCD") == 0);
    StrClear(&output);

    BitVec bv3 = BitVecFromInteger(0755, 10, alloc_base);
    StrAppendFmt(&output, "{o}", bv3);
    success = success && (ZstrCompare(StrBegin(&output), "0o755") == 0);
    StrClear(&output);

    StrAppendFmt(&output, "{>10}", bv1);
    success = success && (ZstrCompare(StrBegin(&output), "     10110") == 0);
    StrClear(&output);

    StrAppendFmt(&output, "{<10}", bv1);
    success = success && (ZstrCompare(StrBegin(&output), "10110     ") == 0);
    StrClear(&output);

    StrAppendFmt(&output, "{^10}", bv1);
    success = success && (ZstrCompare(StrBegin(&output), "  10110   ") == 0);
    StrClear(&output);

    BitVec bv_zero = BitVecFromInteger(0, 1, alloc_base);
    StrAppendFmt(&output, "{x}", bv_zero);
    success = success && (ZstrCompare(StrBegin(&output), "0x0") == 0);
    StrClear(&output);

    StrAppendFmt(&output, "{o}", bv_zero);
    success = success && (ZstrCompare(StrBegin(&output), "0o0") == 0);
    StrClear(&output);

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
    success = success && (ZstrCompare(StrBegin(&output), "123456789012345678901234567890") == 0);
    StrClear(&output);

    StrAppendFmt(&output, "{x}", hex_val);
    success = success && (ZstrCompare(StrBegin(&output), "deadbeefcafebabe1234") == 0);
    StrClear(&output);

    StrAppendFmt(&output, "{X}", hex_val);
    success = success && (ZstrCompare(StrBegin(&output), "DEADBEEFCAFEBABE1234") == 0);
    StrClear(&output);

    StrAppendFmt(&output, "{b}", bin_val);
    success = success && (ZstrCompare(StrBegin(&output), "10100011") == 0);
    StrClear(&output);

    StrAppendFmt(&output, "{o}", oct_val);
    success = success && (ZstrCompare(StrBegin(&output), "755") == 0);
    StrClear(&output);

    StrAppendFmt(&output, "{>34}", big_dec);
    success = success && (ZstrCompare(StrBegin(&output), "    123456789012345678901234567890") == 0);

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
    success = success && (ZstrCompare(StrBegin(&output), "1234567890.012345") == 0);
    StrClear(&output);

    StrAppendFmt(&output, "{e}", sci);
    success = success && (ZstrCompare(StrBegin(&output), "1.234567e+04") == 0);
    StrClear(&output);

    StrAppendFmt(&output, "{E}", sci);
    success = success && (ZstrCompare(StrBegin(&output), "1.234567E+04") == 0);
    StrClear(&output);

    StrAppendFmt(&output, "{.3}", short_v);
    success = success && (ZstrCompare(StrBegin(&output), "1.200") == 0);
    StrClear(&output);

    StrAppendFmt(&output, "{>18}", sci);
    success = success && (ZstrCompare(StrBegin(&output), "          12345.67") == 0);

    FloatDeinit(&exact);
    FloatDeinit(&sci);
    FloatDeinit(&short_v);
    StrDeinit(&output);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// `StrWriteFmt` is the clear-then-append form: prior contents of the
// destination Str must be discarded before the formatted output lands.
bool test_str_write_fmt_clears(void);
bool test_str_write_fmt_clears(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              s     = StrInit(&alloc);
    StrAppendFmt(&s, "old prefix ");
    StrWriteFmt(&s, "fresh {}", LVAL(42));
    bool ok = (StrLen(&s) == 8) && (StrBegin(&s)[0] == 'f') && (StrBegin(&s)[StrLen(&s) - 1] == '2');
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
    size before_length = StrLen(&s);
    bool ok            = StrPatchFmt(&s, 2, "{}", LVAL(1234));
    ok                 = ok && StrLen(&s) == before_length;
    ok                 = ok && StrBegin(&s)[0] == 'A' && StrBegin(&s)[1] == 'A';
    ok = ok && StrBegin(&s)[2] == '1' && StrBegin(&s)[3] == '2' && StrBegin(&s)[4] == '3' && StrBegin(&s)[5] == '4';
    ok = ok && StrBegin(&s)[6] == 'A' && StrBegin(&s)[7] == 'A';

    // Patch that would extend past the end must fail.
    ok = ok && !StrPatchFmt(&s, 6, "{}", LVAL(9999));
    ok = ok && StrLen(&s) == before_length;
    ok = ok && StrBegin(&s)[6] == 'A' && StrBegin(&s)[7] == 'A';

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

int main(void) {
    WriteFmt("[INFO] Starting format writer tests\n\n");

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
    };

    int total_tests = sizeof(tests) / sizeof(tests[0]);

    return run_test_suite(tests, total_tests, NULL, 0, "Io.Write");
}
