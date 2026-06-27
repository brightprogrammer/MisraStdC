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
#include <Misra/Std/Allocator.h>
#include <Misra/Std/Container/Buf.h>
#include <Misra/Std/File.h>
#include <Misra/Std/Io/Private.h>
#include <Misra/Std/Memory.h>
#include <Misra/Sys/Dir.h>

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

bool test_buf_formatting(void) {
    WriteFmt("Testing Buf formatting\n");

    DefaultAllocator alloc  = DefaultAllocatorInit();
    Str              output = StrInit(&alloc);
    Buf              b      = BufInit(&alloc);

    BufPushBytes(&b, (const u8 *)"hello", 5);
    StrAppendFmt(&output, "{}", b); // Buf is a first-class {} argument now

    bool ok = ZstrCompare(StrBegin(&output), "hello") == 0;

    BufDeinit(&b);
    StrDeinit(&output);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A non-printable byte rendered with the `{c}` family must be escaped
// as `\xHH` (lowercase hex for `{c}`/`{a}`, uppercase for `{A}`) so the
// output is always ASCII-safe. The printable-byte path is covered by
// test_char_formatting; this pins the escape branch.
bool test_char_nonprintable_escape(void);
bool test_char_nonprintable_escape(void) {
    WriteFmt("Testing {c} escaping of non-printable bytes\n");

    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              out   = StrInit(&alloc);
    bool             ok    = true;

    // 0x1B (ESC) is non-printable -> "\x1b".
    u8 esc = 0x1B;
    StrAppendFmt(&out, "{c}", esc);
    ok = ok && (ZstrCompare(StrBegin(&out), "\\x1b") == 0);
    StrClear(&out);

    // 0x07 (BEL) -> "\x07".
    u8 bel = 0x07;
    StrAppendFmt(&out, "{c}", bel);
    ok = ok && (ZstrCompare(StrBegin(&out), "\\x07") == 0);
    StrClear(&out);

    // 0xFF with caps spec -> uppercase hex digits "\xFF".
    u8 hi = 0xFF;
    StrAppendFmt(&out, "{A}", hi);
    ok = ok && (ZstrCompare(StrBegin(&out), "\\xFF") == 0);
    StrClear(&out);

    // 0xAB lowercase by default for {c}.
    u8 ab = 0xAB;
    StrAppendFmt(&out, "{c}", ab);
    ok = ok && (ZstrCompare(StrBegin(&out), "\\xab") == 0);

    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Raw binary I/O via the Buf format family: encoding a value with `{<Nr}`
// (little-endian) vs `{>Nr}` (big-endian) lays the bytes down in the
// requested order, and decoding the same directive recovers the value.
// Byte order is caller-observable, so we assert the exact byte layout.
bool test_buf_raw_endianness_and_roundtrip(void);
bool test_buf_raw_endianness_and_roundtrip(void) {
    WriteFmt("Testing Buf raw {<Nr}/{>Nr} byte order + read-back round-trip\n");

    DefaultAllocator alloc = DefaultAllocatorInit();
    Buf              b     = BufInit(&alloc);
    bool             ok    = true;

    // Little-endian u32: 0xDEADBEEF -> EF BE AD DE.
    ok = ok && BufWriteFmt(&b, "{<4r}", (u32)0xDEADBEEF);
    ok = ok && (BufLength(&b) == 4);
    {
        const u8 *d = BufData(&b);
        ok          = ok && d[0] == 0xEF && d[1] == 0xBE && d[2] == 0xAD && d[3] == 0xDE;
    }

    // Big-endian u32: 0xDEADBEEF -> DE AD BE EF.
    ok = ok && BufWriteFmt(&b, "{>4r}", (u32)0xDEADBEEF);
    {
        const u8 *d = BufData(&b);
        ok          = ok && d[0] == 0xDE && d[1] == 0xAD && d[2] == 0xBE && d[3] == 0xEF;
    }

    // Round-trip a u16 and a u64 through both orders.
    ok = ok && BufWriteFmt(&b, "{<2r}{>8r}", (u16)0x1234, (u64)0x0102030405060708ULL);
    {
        BufIter it    = BufIterFromBuf(&b);
        u16     v16   = 0;
        u64     v64   = 0;
        bool    rd_ok = BufReadFmt(&it, "{<2r}{>8r}", v16, v64);
        ok            = ok && rd_ok && (v16 == 0x1234) && (v64 == 0x0102030405060708ULL);
    }

    BufDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Binary literal bytes: emitted verbatim on write, matched on read, a
// mismatch is a soft false (cursor rewound), and `{{` escapes a literal
// '{'.
bool test_buf_literal_magic_roundtrip(void);
bool test_buf_literal_magic_roundtrip(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Buf              b     = BufInit(&alloc);
    bool             ok    = true;

    // "TZif" literal magic + a version byte, emitted verbatim.
    ok = ok && BufWriteFmt(&b, "TZif{>1r}", (u8)'2');
    ok = ok && (BufLength(&b) == 5);
    {
        const u8 *d = BufData(&b);
        ok          = ok && d[0] == 'T' && d[1] == 'Z' && d[2] == 'i' && d[3] == 'f' && d[4] == '2';
    }

    // Read back: the literal matches and the version is extracted.
    {
        BufIter it  = BufIterFromBuf(&b);
        u8      ver = 0;
        ok          = ok && BufReadFmt(&it, "TZif{>1r}", ver) && (ver == '2');
    }

    // Wrong magic -> soft false, no abort, cursor rewound, dest untouched.
    {
        BufIter it  = BufIterFromBuf(&b);
        u8      ver = 0xAB;
        bool    rd  = BufReadFmt(&it, "TZig{>1r}", ver);
        ok          = ok && !rd && (ver == 0xAB) && (IterRemainingLength(&it) == 5);
    }

    BufDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// `{{` round-trips as a single literal '{' byte.
bool test_buf_literal_brace_escape(void);
bool test_buf_literal_brace_escape(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Buf              b     = BufInit(&alloc);
    bool             ok    = true;

    ok = ok && BufWriteFmt(&b, "{{{>1r}", (u8)0x41); // literal '{' then 'A'
    ok = ok && (BufLength(&b) == 2) && (BufData(&b)[0] == '{') && (BufData(&b)[1] == 0x41);

    BufIter it = BufIterFromBuf(&b);
    u8      v  = 0;
    ok         = ok && BufReadFmt(&it, "{{{>1r}", v) && (v == 0x41);

    BufDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A literal that lands on the LAST byte (cursor has exactly 1 byte left)
// pins the `remaining < 1` bounds check against an off-by-one.
bool test_buf_literal_trailing_byte(void);
bool test_buf_literal_trailing_byte(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Buf              b     = BufInit(&alloc);
    bool             ok    = true;

    // Bytes 'Q','Z': read the Q, then match the trailing 'Z' literal with
    // exactly one byte remaining.
    ok = ok && BufWriteFmt(&b, "{>1r}Z", (u8)'Q');
    ok = ok && (BufLength(&b) == 2);
    {
        BufIter it = BufIterFromBuf(&b);
        u8      q  = 0;
        ok         = ok && BufReadFmt(&it, "{>1r}Z", q) && (q == 'Q');
    }
    // Same, but the trailing literal is an escaped '{'.
    StrClear((Str *)&b);
    ok = ok && BufWriteFmt(&b, "{>1r}{{", (u8)'Q');
    {
        BufIter it = BufIterFromBuf(&b);
        u8      q  = 0;
        ok         = ok && BufReadFmt(&it, "{>1r}{{", q) && (q == 'Q');
    }

    BufDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// DateTime renders as ISO 8601 through the IO writer. Lives in the Io
// suite (not the DateTime suite) so mutation testing of Io.c's
// _write_DateTime is actually covered: offset sign, half-hour minutes,
// and the nanosecond branch.
bool test_datetime_iso_write(void);
bool test_datetime_iso_write(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              s     = StrInit(&alloc);
    bool             ok    = true;
    u64              base  = 1609459200ull * 1000000000ull; // 2021-01-01T00:00:00Z

    DateTime utc = DateTimeFromUnixNs(base, 0);
    StrClear(&s);
    StrAppendFmt(&s, "{}", utc);
    ok = ok && ZstrCompare(StrBegin(&s), "2021-01-01T00:00:00Z") == 0;

    DateTime ist = DateTimeFromUnixNs(base, 19800); // +05:30
    StrClear(&s);
    StrAppendFmt(&s, "{}", ist);
    ok = ok && ZstrCompare(StrBegin(&s), "2021-01-01T05:30:00+05:30") == 0;

    DateTime neg = DateTimeFromUnixNs(base, -34200); // -09:30
    StrClear(&s);
    StrAppendFmt(&s, "{}", neg);
    ok = ok && ZstrCompare(StrBegin(&s), "2020-12-31T14:30:00-09:30") == 0;

    DateTime frac = DateTimeFromUnixNs(base + 123456789ull, 0);
    StrClear(&s);
    StrAppendFmt(&s, "{}", frac);
    ok = ok && ZstrCompare(StrBegin(&s), "2021-01-01T00:00:00.123456789Z") == 0;

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}


// ---- helpers/macros relocated from Io.Mutants*.c staging files ----
#define F32_EPSILON        1e-4f
#define DOUBLE_EPSILON     1e-9
#define SENTINEL           (-987654.0)
#define READ1(in, fmt, IO) str_read_fmt((in), (fmt), (TypeSpecificIO[]) {(IO)}, 1)

static bool f32_close(f32 a, f32 b) {
    return F64Abs((f64)a - (f64)b) < (f64)F32_EPSILON;
}

static bool double_equals(f64 a, f64 b) {
    return F64Abs(a - b) < DOUBLE_EPSILON;
}

static bool is_sentinel(f64 v) {
    return F64Abs(v - SENTINEL) < 1e-9;
}

static bool is_zero(f64 v) {
    return F64Abs(v - 0.0) < 1e-12;
}

// Helper: decode one "\\xHL" escape and return the produced char (0 on fail).
static char decode_hex_escape(Zstr lit) {
    Zstr p = lit;
    return ZstrProcessEscape(&p);
}

// Write `content` (len bytes) to a fresh temp file, returning the open
// File (positioned at 0) and stashing the path in `*out_path` so the
// caller can FileRemove + StrDeinit it afterwards.
static File m4_make_temp(DefaultAllocator *alloc, Str *out_path, const char *content, u64 len) {
    File f = FileOpenTemp(out_path, alloc);
    if (!FileIsOpen(&f))
        return f;
    FileWrite(&f, content, len);
    FileSeek(&f, 0, FILE_SEEK_SET);
    return f;
}

static void m4_cleanup(File *f, Str *path) {
    FileClose(f);
    FileRemove(path);
    StrDeinit(path);
}

// Read a Float and report its canonical FloatToStr form. `sentinel` is the
// pre-read value; a failed read leaves the Float untouched, so a mutant that
// changes the token boundary into an unparseable token (or a zero-length
// token) leaves the sentinel in place instead of the expected value.
static bool read_float_equals(Zstr input, Zstr expected) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    Float f = FloatInit(&alloc.base);
    // Distinct sentinel so a no-op (failed) read is observable.
    (void)FloatTryFromStr(&f, "99");

    StrReadFmt(input, "{}", f);

    Str  text = FloatToStr(&f);
    bool ok   = (ZstrCompare(StrBegin(&text), expected) == 0);

    StrDeinit(&text);
    FloatDeinit(&f);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

static bool roundtrip_eq(Zstr path, Zstr expect) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    File             f     = FileOpen(path, "r");
    bool             ok    = false;
    if (FileIsOpen(&f)) {
        Str back = StrInit(&alloc);
        FileRead(&f, &back);
        ok = (ZstrCompare(StrBegin(&back), expect) == 0);
        StrDeinit(&back);
        FileClose(&f);
    }
    DefaultAllocatorDeinit(&alloc);
    return ok;
}


// ---- mutation-hardening tests relocated from Io.Mutants*.c ----

// ---------------------------------------------------------------------------
// arg_index bound (line 610). With argc=1 but a real second reader present in
// the array, the real code must STOP at the second placeholder (more
// placeholders than arguments -> NULL). A mutant that uses '>' would read the
// second slot and succeed (non-NULL). Build argv by hand so the slot beyond
// argc holds a genuine reader, which the StrReadFmt macro never does.
static bool test_m1_arg_index_bound(void) {
    i32 a = 0;
    i32 b = 999;

    TypeSpecificIO argv[2] = {
        TO_TYPE_SPECIFIC_IO(i32, &a),
        TO_TYPE_SPECIFIC_IO(i32, &b),
    };

    // argc = 1: only one argument is allowed even though two placeholders
    // appear in the format.
    Zstr out = str_read_fmt("3-4", "{}-{}", argv, 1);

    // Real: rejects the second placeholder -> NULL, and b stays untouched.
    return out == NULL && b == 999 && a == 3;
}

// ---------------------------------------------------------------------------
// parse_format_spec result must gate the read (line 624 assign_const).
// An invalid spec char ('q') makes parse fail; the destination must be left
// untouched. A mutant that forces spec_ok=true would proceed to read and
// overwrite the destination.
static bool test_m1_invalid_spec_leaves_dest(void) {
    i32  val = 77;
    Zstr out = READ1("5", "{q}", TO_TYPE_SPECIFIC_IO(i32, &val));
    // Real: parse fails -> NULL, val unchanged.
    return out == NULL && val == 77;
}

// ---------------------------------------------------------------------------
// Trailing scan / max_read_len reset (line 594 post_dec). A spec that runs the
// inner scan loop ('c') followed by a trailing literal must leave rem_p in
// sync so the literal matches and the whole input is consumed exactly. A
// mutant that increments rem_p in the scan loop desyncs the trailing-literal
// matcher.
static bool test_m1_char_spec_then_literal(void) {
    u8   ch  = 0;
    Zstr in  = "AZ";
    Zstr out = READ1(in, "{c}Z", TO_TYPE_SPECIFIC_IO(u8, &ch));
    // Real: reads 'A' as the {c} field, matches literal 'Z', consumes all.
    return out == in + 2 && ch == 'A';
}

// ---------------------------------------------------------------------------
// max_read_len = rem_in for an end-of-format placeholder (line 764). A quoted
// string longer than 42 bytes must be read in full; a mutant that caps at 42
// truncates it.
static bool test_m1_long_quoted_string_not_capped(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    // 50 'A's inside quotes.
    Zstr in = "\"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\"";

    Str  s   = StrInit(&alloc);
    Zstr out = READ1(in, "{s}", TO_TYPE_SPECIFIC_IO(Str, &s));

    bool ok = (out != NULL) && (StrLen(&s) == 50);

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

static bool test_m1_raw_u8(void) {
    u8   v   = 0;
    Zstr in  = "\x07";
    Zstr out = READ1(in, "{<1r}", TO_TYPE_SPECIFIC_IO(u8, &v));
    return out == in + 1 && v == 0x07;
}

static bool test_m1_raw_i8(void) {
    i8   v   = 0;
    Zstr in  = "\x07";
    Zstr out = READ1(in, "{<1r}", TO_TYPE_SPECIFIC_IO(i8, &v));
    return out == in + 1 && v == 0x07;
}

static bool test_m1_raw_u16_le(void) {
    u16  v   = 0;
    Zstr in  = "\x34\x12"; // LE -> 0x1234
    Zstr out = READ1(in, "{<2r}", TO_TYPE_SPECIFIC_IO(u16, &v));
    return out == in + 2 && v == 0x1234;
}

static bool test_m1_raw_i16_be(void) {
    i16  v   = 0;
    Zstr in  = "\x12\x34"; // BE -> 0x1234
    Zstr out = READ1(in, "{>2r}", TO_TYPE_SPECIFIC_IO(i16, &v));
    return out == in + 2 && v == 0x1234;
}

static bool test_m1_raw_u32_le(void) {
    u32  v   = 0;
    Zstr in  = "\xEF\xBE\xAD\xDE"; // LE -> 0xDEADBEEF
    Zstr out = READ1(in, "{<4r}", TO_TYPE_SPECIFIC_IO(u32, &v));
    return out == in + 4 && v == 0xDEADBEEFu;
}

static bool test_m1_raw_i32_be(void) {
    i32  v   = 0;
    Zstr in  = "\x12\x34\x56\x78"; // BE -> 0x12345678
    Zstr out = READ1(in, "{>4r}", TO_TYPE_SPECIFIC_IO(i32, &v));
    return out == in + 4 && v == 0x12345678;
}

static bool test_m1_raw_u64_le(void) {
    u64  v   = 0;
    Zstr in  = "\x08\x07\x06\x05\x04\x03\x02\x01"; // LE -> 0x0102030405060708
    Zstr out = READ1(in, "{<8r}", TO_TYPE_SPECIFIC_IO(u64, &v));
    return out == in + 8 && v == 0x0102030405060708ull;
}

static bool test_m1_raw_i64_be(void) {
    i64  v   = 0;
    Zstr in  = "\x01\x02\x03\x04\x05\x06\x07\x08"; // BE -> 0x0102030405060708
    Zstr out = READ1(in, "{>8r}", TO_TYPE_SPECIFIC_IO(i64, &v));
    return out == in + 8 && v == 0x0102030405060708ll;
}

// f32 raw read: pin 699 (==_read_f32 in the width-4 arm) and the case-4 store.
// Compare bit patterns to avoid float-equality ambiguity.
static bool test_m1_raw_f32(void) {
    union {
        f32 f;
        u32 u;
    } got = {0};

    Zstr in  = "\xEF\xBE\xAD\xDE"; // LE bytes -> bit pattern 0xDEADBEEF
    Zstr out = READ1(in, "{<4r}", TO_TYPE_SPECIFIC_IO(f32, &got.f));

    return out == in + 4 && got.u == 0xDEADBEEFu;
}

// f64 raw read: pin 702 (==_read_f64 in the width-8 arm) and the case-8 store.
static bool test_m1_raw_f64(void) {
    union {
        f64 f;
        u64 u;
    } got = {0};

    Zstr in  = "\x08\x07\x06\x05\x04\x03\x02\x01"; // LE -> 0x0102030405060708
    Zstr out = READ1(in, "{<8r}", TO_TYPE_SPECIFIC_IO(f64, &got.f));

    return out == in + 8 && got.u == 0x0102030405060708ull;
}

// Kills line 1637 (start_len init_const 42), 1703 (width>0 gt_to_ge/gt_to_le),
// 1704 (content_len init_const / sub_to_add), 1705 (StrPad scalar-call 42).
// Pre-populate the output so start_len is non-zero: a wrong start_len or a
// sub->add swap mis-computes content_len and therefore the padding.
static bool test_m10_str_width_pad_after_prefix(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str  output = StrInit(&alloc);
    bool ok     = true;

    Str s = StrInitFromZstr("xyz", &alloc);

    StrAppendFmt(&output, "AB");
    // width 6, right-aligned: "xyz" (content_len 3) gets 3 pad spaces.
    StrAppendFmt(&output, "{>6}", s);
    // start_len was 2 ("AB"), content_len = StrLen(o) - start_len = 3, pad to
    // width 6 => 3 spaces. StrPad with ALIGN_RIGHT pushes the pad to the FRONT
    // of the whole buffer, so the prefix "AB" rides along: "   ABxyz". A wrong
    // start_len / sub->add mis-computes content_len and changes the pad count.
    ok = ok && (ZstrCompare(StrBegin(&output), "   ABxyz") == 0);

    StrDeinit(&s);
    StrDeinit(&output);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Kills line 1703 gt_to_le specifically and 1705 StrPad: a width with a
// left-aligned field that is shorter than width must pad on the right.
static bool test_m10_str_width_pad_left_align(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str  output = StrInit(&alloc);
    bool ok     = true;

    Str s = StrInitFromZstr("hi", &alloc);
    StrAppendFmt(&output, "{<5}", s);
    ok = ok && (ZstrCompare(StrBegin(&output), "hi   ") == 0);

    StrDeinit(&s);
    StrDeinit(&output);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Kills line 1644 ne_to_eq (uppercase flag forced low): `{X}` of a Str whose
// byte renders with an a..f hex digit must be uppercase. Byte 0x0F is positive
// regardless of char signedness, so (u64)c is exactly 15 -> 'f'/'F'.
static bool test_m10_str_hex_uppercase(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str  output = StrInit(&alloc);
    bool ok     = true;

    Str s = StrInit(&alloc);
    StrPushBackR(&s, (char)0x0F); // single byte 0x0F -> "f"

    StrAppendFmt(&output, "{X}", s);
    ok = ok && (ZstrCompare(StrBegin(&output), "0x0F") == 0);

    StrDeinit(&s);
    StrDeinit(&output);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Kills line 1644 and_to_or (uppercase flag forced on): `{x}` of the same byte
// must be lowercase.
static bool test_m10_str_hex_lowercase(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str  output = StrInit(&alloc);
    bool ok     = true;

    Str s = StrInit(&alloc);
    StrPushBackR(&s, (char)0x0F);

    StrAppendFmt(&output, "{x}", s);
    ok = ok && (ZstrCompare(StrBegin(&output), "0x0f") == 0);

    StrDeinit(&s);
    StrDeinit(&output);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Kills line 1646 gt_to_ge / gt_to_le (the "separator before all but the
// first byte" rule): a two-byte Str hex must be "0x01 0x02" -- one space
// between, none before the first.
static bool test_m10_str_hex_multibyte_separator(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str  output = StrInit(&alloc);
    bool ok     = true;

    Str s = StrInit(&alloc);
    StrPushBackR(&s, (char)0x01);
    StrPushBackR(&s, (char)0x02);

    StrAppendFmt(&output, "{x}", s);
    // gt_to_ge => " 0x01 0x02" (leading space); gt_to_le => "0x010x02".
    ok = ok && (ZstrCompare(StrBegin(&output), "0x01 0x02") == 0);

    StrDeinit(&s);
    StrDeinit(&output);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Kills line 1656 eq_to_ne (zero-pad nibble only when hex string is 1 char):
// a single-hex-digit byte (0x05) must render "0x05", not "0x5".
static bool test_m10_str_hex_zero_pad_nibble(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str  output = StrInit(&alloc);
    bool ok     = true;

    Str s = StrInit(&alloc);
    StrPushBackR(&s, (char)0x05);

    StrAppendFmt(&output, "{x}", s);
    ok = ok && (ZstrCompare(StrBegin(&output), "0x05") == 0);

    StrDeinit(&s);
    StrDeinit(&output);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Kills line 1674 eq_to_ne and 1677 assign_const (the precision-truncation
// "else" branch): precision 3 of "hello" must yield exactly "hel".
static bool test_m10_str_precision_truncate(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str  output = StrInit(&alloc);
    bool ok     = true;

    Str s = StrInitFromZstr("hello", &alloc);
    StrAppendFmt(&output, "{.3}", s);
    ok = ok && (ZstrCompare(StrBegin(&output), "hel") == 0);

    StrDeinit(&s);
    StrDeinit(&output);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Kills line 1675 assign_const (precision 0 => render nothing): precision 0 of
// "hello" must yield the empty string.
static bool test_m10_str_precision_zero(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str  output = StrInit(&alloc);
    bool ok     = true;

    Str s = StrInitFromZstr("hello", &alloc);
    StrAppendFmt(&output, "{.0}", s);
    ok = ok && (StrLen(&output) == 0);

    StrDeinit(&s);
    StrDeinit(&output);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// --- Line 2222 col22/col38: `if (s[0] == '\0' || s[1] == '\0')`
//     two cxx_eq_to_ne. For a full hex escape both digits are non-NUL, so the
//     real guard is false; either swap to != makes the guard true and errors. ---
static bool test_m11_hex_two_digits_not_truncated(void) {
    Zstr p = "\\x41"; // 'A'
    char c = ZstrProcessEscape(&p);
    return c == 'A';
}

// --- Line 2226 col17 (cxx_init_const) and col27 (cxx_replace_scalar_call):
//     hex_val initialized from hex_byte(...). Both swaps force hex_val=42.
//     0x41 = 65 != 42, so the decoded byte mismatches. ---
static bool test_m11_hex_value_is_decoded_not_42(void) {
    Zstr p = "\\x41"; // must yield 65 ('A'), not 42 ('*')
    char c = ZstrProcessEscape(&p);
    return c == (char)65;
}

// Second hex vector with a different value to be doubly sure the call result
// is actually consumed (255 != 42).
static bool test_m11_hex_ff(void) {
    Zstr p = "\\xff";
    char c = ZstrProcessEscape(&p);
    return (unsigned char)c == 0xFFu;
}

// --- Line 2227 cxx_lt_to_ge: `if (hex_val < 0)` -> `hex_val >= 0`.
//     For a valid digit pair hex_val >= 0 is true, so the mutant takes the
//     error branch and returns 0; real returns the byte. ---
static bool test_m11_hex_valid_not_rejected(void) {
    Zstr p = "\\x41";
    return ZstrProcessEscape(&p) == 'A';
}

// --- Line 2227 cxx_lt_to_le: `hex_val < 0` -> `hex_val <= 0`. Only differs at
//     hex_val == 0 ("\x00"). Both paths *return* 0, so the kill is the pointer
//     side effect: on success *str advances past the escape; on the (mutant)
//     error path *str is left untouched. ---
static bool test_m11_hex_zero_advances_pointer(void) {
    char        buf[] = {'\\', 'x', '0', '0', 'Z', '\0'};
    const char *p     = buf;
    char        c     = ZstrProcessEscape(&p);
    // real: decodes NUL byte and advances p to the last consumed hex digit
    // (buf[3]); mutant (<=0) errors out and leaves p == buf.
    return c == 0 && p == &buf[3];
}

// --- Line 2231 cxx_assign_const: `result = (char)hex_val;` -> result = 42.
//     0x41 -> 65, distinct from 42. ---
static bool test_m11_hex_result_assigned_from_value(void) {
    Zstr p = "\\x41";
    return ZstrProcessEscape(&p) == (char)65;
}

// Sanity: a malformed hex escape ("\xZZ") really errors (returns 0). Guards
// the hex_byte<0 path so the lt mutants have a contrasting branch covered.
static bool test_m11_hex_invalid_returns_zero(void) {
    Zstr p = "\\xZZ";
    return ZstrProcessEscape(&p) == 0;
}

// "0x1" -> value 1, raw bit_len would be 1, code clamps to 4.
// Kills: 3111:13 cxx_init_const (bit_len init -> 42),
//        3113:21 cxx_assign_const (bit_len = 4 -> 42),
//        3111:45 cxx_replace_scalar_call (clz -> 42 => 64-42=22),
//        3111:43 cxx_sub_to_add (64 - clz -> 64 + clz, clamped to 64).
static bool test_m12_hex_min_width_is_four(void) {
    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    BitVec bv = BitVecInit(alloc_base);
    Zstr   z  = "0x1";
    StrReadFmt(z, "{}", bv);

    bool ok = (BitVecToInteger(&bv) == 1) && (BitVecLen(&bv) == 4);

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// "0xDEAD" -> value 0xDEAD, 16 significant bits, no clamp.
// Reinforces the scalar/sub mutations on the width computation:
//   clz->42 => 64-42=22, sub->add => clamp 64; real is 16.
static bool test_m12_hex_width_tracks_value(void) {
    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    BitVec bv = BitVecInit(alloc_base);
    Zstr   z  = "0xDEAD";
    StrReadFmt(z, "{}", bv);

    bool ok = (BitVecToInteger(&bv) == 0xDEAD) && (BitVecLen(&bv) == 16);

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Hex consumes exactly the digits: cursor must advance past "0xDEAD".
static bool test_m12_hex_cursor_advances(void) {
    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    BitVec bv = BitVecInit(alloc_base);
    Zstr   z  = "0xDEAD";
    StrReadFmt(z, "{}", bv);

    bool ok = (BitVecToInteger(&bv) == 0xDEAD) && (*z == '\0');

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// "0o1" -> value 1, raw bit_len 1, clamped to 3.
// Kills: 3149:13 cxx_init_const, 3151:21 cxx_assign_const (3 -> 42),
//        3149:45 cxx_replace_scalar_call (clz -> 42 => 22),
//        3149:43 cxx_sub_to_add (clamped 64).
static bool test_m12_octal_min_width_is_three(void) {
    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    BitVec bv = BitVecInit(alloc_base);
    Zstr   z  = "0o1";
    StrReadFmt(z, "{}", bv);

    bool ok = (BitVecToInteger(&bv) == 1) && (BitVecLen(&bv) == 3);

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// "0o10" -> octal 10 == decimal 8. The digit loop guard is `c >= '0'`;
// mutating it to `c > '0'` rejects the trailing '0' so only "1" is read,
// yielding value 1 instead of 8.
// Kills: 3127:42 cxx_ge_to_gt.
static bool test_m12_octal_accepts_zero_digit(void) {
    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    BitVec bv = BitVecInit(alloc_base);
    Zstr   z  = "0o10";
    StrReadFmt(z, "{}", bv);

    bool ok = (BitVecToInteger(&bv) == 8) && (*z == '\0');

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Larger octal so width exceeds both the 3-bit floor and any clamp boundary,
// pinning the width computation: 0o755 == 0x1ED == 493, 9 significant bits.
static bool test_m12_octal_width_tracks_value(void) {
    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    BitVec bv = BitVecInit(alloc_base);
    Zstr   z  = "0o755";
    StrReadFmt(z, "{}", bv);

    bool ok = (BitVecToInteger(&bv) == 0755) && (BitVecLen(&bv) == 9);

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Hex render of a multi-byte Zstr: each byte -> "0xNN", space-separated,
// no leading space. Kills the `i > 0` separator guard (gt_to_ge/gt_to_le),
// the `size i = 0` start index (init_const), and `i++` (post_inc_to_post_dec
// would loop forever / mis-walk).
static bool test_m13_zstr_hex_multibyte(void) {
    DefaultAllocator alloc  = DefaultAllocatorInit();
    Str              output = StrInit(&alloc);

    Zstr s = "AB"; // 0x41 0x42
    StrAppendFmt(&output, "{x}", s);
    bool ok = (ZstrCompare(StrBegin(&output), "0x41 0x42") == 0);

    StrDeinit(&output);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Single byte: exactly "0xNN" with no separator. If the `i > 0` guard were
// `i >= 0`, a stray separator would appear; one byte still proves no leading
// space and the start index.
static bool test_m13_zstr_hex_single_byte(void) {
    DefaultAllocator alloc  = DefaultAllocatorInit();
    Str              output = StrInit(&alloc);

    Zstr s = "Z"; // 0x5a
    StrAppendFmt(&output, "{x}", s);
    bool ok = (ZstrCompare(StrBegin(&output), "0x5a") == 0);

    StrDeinit(&output);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Lowercase hex letters: byte 0xab -> "0xab". Real code: CAPS flag clear,
// so a-f stay lowercase. Kills the CAPS truthiness mutations on the config
// (and_to_or / ne_to_eq at the uppercase computation) -- if CAPS were forced
// true here output would be "0xAB".
static bool test_m13_zstr_hex_lowercase_letters(void) {
    DefaultAllocator alloc  = DefaultAllocatorInit();
    Str              output = StrInit(&alloc);

    Zstr s = "\xab"; // single byte 0xab
    StrAppendFmt(&output, "{x}", s);
    bool ok = (ZstrCompare(StrBegin(&output), "0xab") == 0);

    StrDeinit(&output);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Uppercase hex via `{X}`: byte 0xab -> "0xAB". CAPS flag is set; if the
// uppercase predicate were forced the other way output would be "0xab".
// Together with the lowercase test this pins both polarities of the CAPS
// computation (and_to_or @1739:82 and ne_to_eq @1739:99).
static bool test_m13_zstr_hex_uppercase_letters(void) {
    DefaultAllocator alloc  = DefaultAllocatorInit();
    Str              output = StrInit(&alloc);

    Zstr s = "\xab";
    StrAppendFmt(&output, "{X}", s);
    bool ok = (ZstrCompare(StrBegin(&output), "0xAB") == 0);

    StrDeinit(&output);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Zero-pad of a single nibble: byte 0x05 -> "0x05" (StrLen(&hex)==1 -> push
// front '0'). Kills the `StrLen(&hex) == 1` guard (eq_to_ne): with `!= 1`
// the pad would be skipped giving "0x5".
static bool test_m13_zstr_hex_zero_pad_nibble(void) {
    DefaultAllocator alloc  = DefaultAllocatorInit();
    Str              output = StrInit(&alloc);

    Zstr s = "\x05"; // single byte 0x05
    StrAppendFmt(&output, "{x}", s);
    bool ok = (ZstrCompare(StrBegin(&output), "0x05") == 0);

    StrDeinit(&output);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Two-digit byte is NOT padded: byte 0xfe -> "0xfe" (StrLen(&hex)==2). With
// the eq->ne mutation a spurious '0' would be prepended -> "0x0fe".
static bool test_m13_zstr_hex_no_pad_two_digits(void) {
    DefaultAllocator alloc  = DefaultAllocatorInit();
    Str              output = StrInit(&alloc);

    Zstr s = "\xfe";
    StrAppendFmt(&output, "{x}", s);
    bool ok = (ZstrCompare(StrBegin(&output), "0xfe") == 0);

    StrDeinit(&output);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Precision truncation: `{.3}` on "Hello" -> "Hel". Kills the precision MIN2
// assignment (assign_const @1765) -- forcing len to a constant would change
// the truncated length.
static bool test_m13_zstr_precision_truncate(void) {
    DefaultAllocator alloc  = DefaultAllocatorInit();
    Str              output = StrInit(&alloc);

    Zstr s = "Hello";
    StrAppendFmt(&output, "{.3}", s);
    bool ok = (ZstrCompare(StrBegin(&output), "Hel") == 0);

    StrDeinit(&output);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Precision longer than the string is a no-op: `{.10}` on "Hi" -> "Hi".
// MIN2 must keep the real length; assign_const to precision (10) would
// over-read / change output.
static bool test_m13_zstr_precision_longer_noop(void) {
    DefaultAllocator alloc  = DefaultAllocatorInit();
    Str              output = StrInit(&alloc);

    Zstr s = "Hi";
    StrAppendFmt(&output, "{.10}", s);
    bool ok = (ZstrCompare(StrBegin(&output), "Hi") == 0);

    StrDeinit(&output);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Precision 0 emits nothing: `{.0}` on "Hello" -> "". Kills the
// `precision == 0` guard (eq_to_ne @1762) and the `len = 0` assignment
// (assign_const @1763): with `!= 0` the carve-out is taken for any nonzero
// precision instead, and forcing len away from 0 would emit bytes.
static bool test_m13_zstr_precision_zero_empty(void) {
    DefaultAllocator alloc  = DefaultAllocatorInit();
    Str              output = StrInit(&alloc);

    Zstr s = "Hello";
    StrAppendFmt(&output, "{.0}", s);
    bool ok = (StrLen(&output) == 0);

    StrDeinit(&output);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Precision 1 keeps exactly one byte: `{.1}` on "Hello" -> "H". With the
// `precision == 0` guard mutated to `!= 0`, precision 1 would wrongly take
// the len=0 branch and emit "".
static bool test_m13_zstr_precision_one(void) {
    DefaultAllocator alloc  = DefaultAllocatorInit();
    Str              output = StrInit(&alloc);

    Zstr s = "Hello";
    StrAppendFmt(&output, "{.1}", s);
    bool ok = (ZstrCompare(StrBegin(&output), "H") == 0);

    StrDeinit(&output);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Right-pad to width with a non-empty prefix already in the buffer. This
// pins content_len = StrLen(o) - start_len (sub_to_add @1792): start_len is
// captured AFTER "abc" is written, so a `+` would compute a larger content
// length and shrink the padding. "Hi" is content_len 2, width 10 => 8 pad
// spaces. StrPad ALIGN_RIGHT pushes the pad to the FRONT of the whole buffer,
// so the "abc" prefix rides along after the spaces: "        abcHi".
static bool test_m13_zstr_pad_after_prefix(void) {
    DefaultAllocator alloc  = DefaultAllocatorInit();
    Str              output = StrInit(&alloc);

    Zstr s = "Hi";
    StrAppendFmt(&output, "abc{>10}", s);
    bool ok = (ZstrCompare(StrBegin(&output), "        abcHi") == 0);

    StrDeinit(&output);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Left-pad after prefix, second distinguishing case for the content_len
// subtraction: "xy" + "Hi" + 8 spaces.
static bool test_m13_zstr_left_pad_after_prefix(void) {
    DefaultAllocator alloc  = DefaultAllocatorInit();
    Str              output = StrInit(&alloc);

    Zstr s = "Hi";
    StrAppendFmt(&output, "xy{<10}", s);
    bool ok = (ZstrCompare(StrBegin(&output), "xyHi        ") == 0);

    StrDeinit(&output);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Empty Zstr still pads to width: `{>5}` on "" -> 5 spaces. Exercises the
// skip-body / fall-through-to-padding path while width logic still runs.
static bool test_m13_zstr_empty_padded(void) {
    DefaultAllocator alloc  = DefaultAllocatorInit();
    Str              output = StrInit(&alloc);

    Zstr s = "";
    StrAppendFmt(&output, "{>5}", s);
    bool ok = (ZstrCompare(StrBegin(&output), "     ") == 0);

    StrDeinit(&output);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// 2665:56 cxx_eq_to_ne -- the `is_first_char` argument
// `StrIterIndex(&si) == StrIterIndex(&saved)`. Flipping to `!=` inverts
// first-char detection, so the 'x' of a "0x" prefix (a valid second char only
// when !is_first_char) is rejected: the scan breaks after the leading '0'.
// Input "0x1f": live collects "0x1f" -> 31; mutant collects only "0" -> 0.
// ---------------------------------------------------------------------------
static bool test_m14_hex_prefix_uses_first_char_flag(void) {
    Zstr z = "0x1f";
    u8   v = 0;
    StrReadFmt(z, "{}", v);
    return v == 31;
}

// ---------------------------------------------------------------------------
// 2673:36 cxx_sub_to_add -- slice length `StrIterIndex(&si) - StrIterIndex(&saved)`.
// With no leading whitespace `saved` is index 0 and `-`/`+` coincide, so a
// LEADING SPACE is required to expose the difference. Input " 5": after the
// whitespace skip `saved` == 1, end == 2; live pos = 2-1 = 1 (slice "5" -> 5),
// mutant pos = 2+1 = 3 (over-long slice spills past the byte, fails) -> 0.
// ---------------------------------------------------------------------------
static bool test_m14_slice_length_is_difference(void) {
    Zstr z = " 5";
    u8   v = 0;
    StrReadFmt(z, "{}", v);
    return v == 5;
}

// ---------------------------------------------------------------------------
// 2680:23 cxx_eq_to_ne -- the bare-prefix guard `StrLen(&temp) == 2`. Flipping
// to `!=` makes the guard fire for ANY length other than 2, so a real
// multi-char hex literal is wrongly early-rejected. Input "0xff": live parses
// 0xff -> 255; mutant (len 4 != 2, [0]=='0', [1]=='x') early-rejects -> 0.
// ---------------------------------------------------------------------------
static bool test_m14_prefix_guard_len_gate(void) {
    Zstr z = "0xff";
    u8   v = 0;
    StrReadFmt(z, "{}", v);
    return v == 255;
}

// ---------------------------------------------------------------------------
// 2681:29/58/87 + 2682:29/58/87 cxx_eq_to_ne -- the six prefix-letter checks
// `[1]=='x' | 'X' | 'b' | 'B' | 'o' | 'O'`. Input "05" has [1]=='5', so every
// real check is false and the bare-prefix guard does NOT fire (live parses
// 05 -> 5). Flipping ANY single `==`->`!=` makes that one comparison true
// ('5' != letter), so the OR fires and the guard early-rejects the valid
// number -> 0. One input distinguishes all six mutations individually.
// ---------------------------------------------------------------------------
static bool test_m14_prefix_guard_letters(void) {
    Zstr z = "05";
    u8   v = 0;
    StrReadFmt(z, "{}", v);
    return v == 5;
}

// Low nibble '0' is the lower boundary of the digit range. "\\x10" -> 0x10.
// L40 col11 >=->>  : '0' > '0' false -> '0' non-hex -> byte becomes 0.
// L40 col11 >=-><  : '0' < '0' false -> '0' non-hex -> byte becomes 0.
static bool test_m15_digit_low_boundary_zero(void) {
    bool ok = (decode_hex_escape("\\x10") == (char)0x10);
    return ok;
}

// Low nibble '9' is the upper boundary of the digit range. "\\x09" -> 0x09.
// L40 col23 <=->>  : '9' > '9' false -> '9' non-hex -> byte 0.
// L40 col23 <=-><  : '9' < '9' false -> '9' non-hex -> byte 0.
static bool test_m15_digit_high_boundary_nine(void) {
    bool ok = (decode_hex_escape("\\x09") == (char)0x09);
    return ok;
}

// A mid-range digit pins the subtraction in line 41. "\\x05" -> 0x05.
// L41 col18 sub->add : hex_nibble('5') = '5' + '0' = 0x65 (not 5), and
//   hex_nibble('0') for the high nibble = '0' + '0' = 0x60, so the decoded
//   byte is no longer 0x05.
static bool test_m15_digit_value_subtraction(void) {
    bool ok = (decode_hex_escape("\\x05") == (char)0x05);
    return ok;
}

// Low nibble 'a' is the lower boundary of a..f. High nibble '0' is a digit
// (unaffected by lines 42-45). "\\x0a" -> 0x0a.
// L42 col11 >=->>  : 'a' > 'a' false -> 'a' non-hex -> byte 0.
// L42 col11 >=-><  : 'a' < 'a' false -> 'a' non-hex -> byte 0.
// L43 col24 sub->add : 10 + ('a' + 'a') (huge), byte != 0x0a.
static bool test_m15_lower_low_boundary_a(void) {
    bool ok = (decode_hex_escape("\\x0a") == (char)0x0a);
    return ok;
}

// Low nibble 'f' is the upper boundary of a..f. "\\x0f" -> 0x0f.
// L42 col23 <=->>  : 'f' > 'f' false -> 'f' non-hex -> byte 0.
// L42 col23 <=-><  : 'f' < 'f' false -> 'f' non-hex -> byte 0.
// L43 col19 add->sub : 10 - ('f' - 'a') = 10 - 5 = 5, so byte 0x05 != 0x0f.
static bool test_m15_lower_high_boundary_f(void) {
    bool ok = (decode_hex_escape("\\x0f") == (char)0x0f);
    return ok;
}

// Low nibble 'A' is the lower boundary of A..F. "\\x0A" -> 0x0a.
// L44 col11 >=->>  : 'A' > 'A' false -> 'A' non-hex -> byte 0.
// L44 col11 >=-><  : 'A' < 'A' false -> 'A' non-hex -> byte 0.
// L45 col24 sub->add : 10 + ('A' + 'A') (huge), byte != 0x0a.
static bool test_m15_upper_low_boundary_A(void) {
    bool ok = (decode_hex_escape("\\x0A") == (char)0x0a);
    return ok;
}

// Low nibble 'F' is the upper boundary of A..F. "\\x0F" -> 0x0f.
// L44 col23 <=->>  : 'F' > 'F' false -> 'F' non-hex -> byte 0.
// L44 col23 <=-><  : 'F' < 'F' false -> 'F' non-hex -> byte 0.
// L45 col19 add->sub : 10 - ('F' - 'A') = 10 - 5 = 5, so byte 0x05 != 0x0f.
static bool test_m15_upper_high_boundary_F(void) {
    bool ok = (decode_hex_escape("\\x0F") == (char)0x0f);
    return ok;
}

// width-1 (u8) round-trip.
//   Kills 846:28 (width != 1 -> == 1): mutant makes the valid width-1 spec
//         hit the "raw width must be 1/2/4/8" LOG_FATAL.
//   Kills 870:22 (x = v -> x = 42) and 925:33 (*(u8*)data = (u8)x -> 42):
//         either makes the stored byte 42 instead of the real value.
//   Kills 904:23 (var_width = 1 -> 42): mutant makes width(1) != var_width
//         and aborts at the width-mismatch LOG_FATAL.
static bool test_m16_u8_roundtrip(void) {
    WriteFmt("m16: u8 width-1 round-trip\n");
    DefaultAllocator alloc = DefaultAllocatorInit();
    Buf              b     = BufInit(&alloc);
    bool             ok    = true;

    ok = ok && BufWriteFmt(&b, "{<1r}", (u8)0x05);
    ok = ok && (BufLength(&b) == 1);

    BufIter it = BufIterFromBuf(&b);
    u8      v8 = 0;
    bool    rd = BufReadFmt(&it, "{<1r}", v8);
    ok         = ok && rd && (v8 == 0x05);

    BufDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// width-4 (u32) round-trip.
//   Kills 846:74 (width != 4 -> == 4): mutant makes the valid width-4 spec
//         hit the "raw width must be 1/2/4/8" LOG_FATAL.
//   Kills 882:22 (x = v -> x = 42) and 931:34 (*(u32*)data = (u32)x -> 42):
//         either makes the stored word 42 instead of the real value.
//   Kills 908:23 (var_width = 4 -> 42): mutant makes width(4) != var_width
//         and aborts at the width-mismatch LOG_FATAL.
static bool test_m16_u32_roundtrip(void) {
    WriteFmt("m16: u32 width-4 round-trip\n");
    DefaultAllocator alloc = DefaultAllocatorInit();
    Buf              b     = BufInit(&alloc);
    bool             ok    = true;

    ok = ok && BufWriteFmt(&b, "{<4r}", (u32)0xDEADBEEF);
    ok = ok && (BufLength(&b) == 4);

    BufIter it  = BufIterFromBuf(&b);
    u32     v32 = 0;
    bool    rd  = BufReadFmt(&it, "{<4r}", v32);
    ok          = ok && rd && (v32 == 0xDEADBEEF);

    BufDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// width-8 (u64) round-trip.
//   Kills 909:60 (read_fn == _read_u64 -> !=): with an actual _read_u64
//         destination, the mutated first clause flips the whole OR-chain to
//         false, so the u64 case falls into the "unsupported variable type"
//         LOG_FATAL instead of resolving var_width = 8.
static bool test_m16_u64_roundtrip(void) {
    WriteFmt("m16: u64 width-8 round-trip\n");
    DefaultAllocator alloc = DefaultAllocatorInit();
    Buf              b     = BufInit(&alloc);
    bool             ok    = true;

    ok = ok && BufWriteFmt(&b, "{>8r}", (u64)0x0102030405060708ULL);
    ok = ok && (BufLength(&b) == 8);

    BufIter it  = BufIterFromBuf(&b);
    u64     v64 = 0;
    bool    rd  = BufReadFmt(&it, "{>8r}", v64);
    ok          = ok && rd && (v64 == 0x0102030405060708ULL);

    BufDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// width-8 (i64) round-trip.
//   Kills 909:92 (read_fn == _read_i64 -> !=): with an actual _read_i64
//         destination, the mutated middle clause flips the OR-chain to
//         false for the i64 case, dropping it into the "unsupported variable
//         type" LOG_FATAL instead of resolving var_width = 8.
static bool test_m16_i64_roundtrip(void) {
    WriteFmt("m16: i64 width-8 round-trip\n");
    DefaultAllocator alloc = DefaultAllocatorInit();
    Buf              b     = BufInit(&alloc);
    bool             ok    = true;

    ok = ok && BufWriteFmt(&b, "{<8r}", (i64)-1234567890123LL);
    ok = ok && (BufLength(&b) == 8);

    BufIter it  = BufIterFromBuf(&b);
    i64     v64 = 0;
    bool    rd  = BufReadFmt(&it, "{<8r}", v64);
    ok          = ok && rd && (v64 == -1234567890123LL);

    BufDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// Deadend: a well-formed but NON-raw spec ({2}, width 2, no flags) parses
// successfully, so on real code the `if (!(fmt_info.flags & FMT_FLAG_RAW))`
// guard fires its LOG_FATAL ("only raw specs allowed").
//
//   Kills 843:30 (flags & RAW -> flags | RAW): the mutated OR is always
//         nonzero, so !(...) is false and the non-raw spec is wrongly
//         accepted -- buf_read_fmt then resolves a matching u16 destination
//         and RETURNS without aborting. Real code aborts here; mutant does
//         not -> deadend test distinguishes them.
// ---------------------------------------------------------------------------
static bool test_m16_nonraw_spec_deadend(void) {
    WriteFmt("m16: non-raw spec {2} must abort\n");
    DefaultAllocator alloc = DefaultAllocatorInit();
    Buf              b     = BufInit(&alloc);

    // Lay down 2 bytes so the mutant's (wrongly accepted) width-2 read has
    // enough input and would otherwise succeed silently.
    BufWriteFmt(&b, "{<2r}", (u16)0x1234);

    BufIter it  = BufIterFromBuf(&b);
    u16     v16 = 0;
    BufReadFmt(&it, "{2}", v16); // real: LOG_FATAL (non-raw). mutant: returns.

    BufDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return true; // unreached on real code (aborts above)
}

// Kills line 1479 gt_to_ge (`frac_digits > 0`): a value whose significand is a
// single digit yields frac_digits == 0, so NO decimal point must appear.
// gt_to_ge makes `0 >= 0` true and emits a stray "." -> "1.e+00".
static bool test_m17_sci_single_digit_no_point(void) {
    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    Str   output = StrInit(&alloc);
    bool  ok     = true;
    Float v      = FloatFromStr("1.0", alloc_base);

    StrAppendFmt(&output, "{e}", v);
    ok = ok && (ZstrCompare(StrBegin(&output), "1e+00") == 0);

    FloatDeinit(&v);
    StrDeinit(&output);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Kills line 1449 gt_to_ge / gt_to_le (`has_precision && precision > 0` in the
// zero branch): with explicit precision 0 of 0.0, NO decimal point or zeros
// must be emitted -> "0e+00". A ge/le swap makes `0 ? 0` true and emits ".".
static bool test_m17_sci_zero_precision_zero_no_point(void) {
    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    Str   output = StrInit(&alloc);
    bool  ok     = true;
    Float v      = FloatFromStr("0.0", alloc_base);

    StrAppendFmt(&output, "{.0e}", v);
    ok = ok && (ZstrCompare(StrBegin(&output), "0e+00") == 0);

    FloatDeinit(&v);
    StrDeinit(&output);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Kills line 1453 cxx_init_const (loop `i = 0` -> 42), cxx_lt_to_ge,
// cxx_lt_to_le, and cxx_post_inc_to_post_dec for the zero-branch zero-fill
// loop: precision 3 of 0.0 must emit exactly three '0' digits after the point
// -> "0.000e+00". A bad init/condition emits the wrong number of zeros; a
// dec-instead-of-inc never terminates.
static bool test_m17_sci_zero_precision_three_zeros(void) {
    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    Str   output = StrInit(&alloc);
    bool  ok     = true;
    Float v      = FloatFromStr("0.0", alloc_base);

    StrAppendFmt(&output, "{.3e}", v);
    ok = ok && (ZstrCompare(StrBegin(&output), "0.000e+00") == 0);

    FloatDeinit(&v);
    StrDeinit(&output);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Kills line 1484 cxx_lt_to_le (`i + 1 < StrLen(&digits)` selecting a real
// digit vs a '0' pad): a 2-digit significand (12) with precision 2 must read
// digit[1] then pad one '0' -> "1.20e+01". An lt->le swap walks one index too
// far (digit[2] is the string terminator) and corrupts the fractional part.
static bool test_m17_sci_frac_digit_vs_pad_boundary(void) {
    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    Str   output = StrInit(&alloc);
    bool  ok     = true;
    Float v      = FloatFromStr("12.0", alloc_base);

    StrAppendFmt(&output, "{.2e}", v);
    ok = ok && (ZstrCompare(StrBegin(&output), "1.20e+01") == 0);

    FloatDeinit(&v);
    StrDeinit(&output);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Companion full round-trip on a real scientific value: anchors the exponent
// sign/width and the uppercase ('E') path so adjacent mutations in the shared
// code stay covered.
static bool test_m17_sci_value_and_uppercase(void) {
    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    Str   output = StrInit(&alloc);
    bool  ok     = true;
    Float v      = FloatFromStr("12345.67", alloc_base);

    StrAppendFmt(&output, "{e}", v);
    ok = ok && (ZstrCompare(StrBegin(&output), "1.234567e+04") == 0);
    StrClear(&output);

    StrAppendFmt(&output, "{E}", v);
    ok = ok && (ZstrCompare(StrBegin(&output), "1.234567E+04") == 0);

    FloatDeinit(&v);
    StrDeinit(&output);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// 1598:31 (char_to_store=(u8)*current -> 42) via bad (non-hex) nibbles.
// Real: hex_byte('Z','Z') == -1, so the salvage path stores literal '\'.
// The 42-mutant stores '*' instead.
static bool test_m18_bad_nibble_salvage(void) {
    Zstr z = "\\xZZ";            // '\', 'x', 'Z', 'Z'
    u8   v = 0;
    StrReadFmt(z, "{c}", v);
    bool ok = (v == 0x5C);       // '\'; mutant stores 42
    ok      = ok && (*z == 'x'); // only '\' consumed
    return ok;
}

// 1576:10 (is_caps init -> 42/true), 1576:52 (&& -> ||), 1576:69 (!=0 -> ==0),
// 1607:27 (char_to_store = is_caps ? ... : ... -> 42).
// `{a}` sets FORCE_CASE without CAPS, so is_caps must be FALSE -> lowercase.
// Real: 'A' -> 'a'. Any is_caps-forced-true mutant uppercases (keeps 'A');
// the 42-mutant on 1607 stores '*'.
static bool test_m18_force_lowercase(void) {
    Zstr z = "A";
    u8   v = 0;
    StrReadFmt(z, "{a}", v);
    return v == 'a'; // mutants give 'A' (0x41) or '*' (42)
}

// 1576:69 (!=0 -> ==0) in the other polarity, plus 1607:27.
// `{A}` sets CAPS+FORCE_CASE, so is_caps must be TRUE -> uppercase.
// Real: 'a' -> 'A'. The ==0 mutant makes is_caps FALSE -> lowercases to 'a';
// the 42-mutant on 1607 stores '*'.
static bool test_m18_force_uppercase(void) {
    Zstr z = "a";
    u8   v = 0;
    StrReadFmt(z, "{A}", v);
    return v == 'A'; // mutant gives 'a' (0x61) or '*' (42)
}

// 1578:23 (bytes_read < buffer_size -> <=). With a u8 target buffer_size==1,
// so real reads exactly one byte from "AB" and stops, leaving "B" unconsumed.
// The <= mutant overflows by one (reads 'A' then 'B'), consuming both bytes
// and leaving the cursor at end-of-input.
//
// The target is the first element of a 4-byte array so the mutant's one-byte
// overstore stays in-bounds; the kill is read off the cursor, not the store.
static bool test_m18_buffer_size_stop(void) {
    u8   buf[4] = {0, 0, 0, 0};
    Zstr z      = "AB";
    StrReadFmt(z, "{c}", buf[0]);
    bool ok = (buf[0] == 'A');
    ok      = ok && (*z == 'B'); // exactly one byte consumed; mutant -> 0
    return ok;
}

// ---------------------------------------------------------------------------
// 2548 col 76: `c1 == 'i'` -> `c1 != 'i'` in the leading-'-' inf clause.
//
// Input "-inf": first char '-', look-ahead c1 == 'i'. Real code:
//   (c == '-' && (c1 == 'i' || c1 == 'I')) -> true -> inf branch ->
//   StrToF64("-inf") -> -inf, four chars consumed.
// Mutated (c1 != 'i'): with c1 == 'i' the clause (false || false) is false,
// so the inf branch is skipped; the digit scan then collects just "-",
// is_valid_numeric_string("-") passes but StrToF64("-") fails -> no consume,
// v left untouched. So the mutant fails to parse a value real code accepts.
// ---------------------------------------------------------------------------
static bool test_m19_neg_inf_lower(void) {
    f64  v    = 123.0;
    Zstr z    = "-inf";
    Zstr orig = z;
    StrReadFmt(z, "{}", v);
    // Real: consumed (z advanced) and v is -inf. Mutant: not consumed.
    return (z != orig) && F64IsInf(v) && (v < 0.0);
}

// ---------------------------------------------------------------------------
// 2548 col 89: `c1 == 'I'` -> `c1 != 'I'` in the leading-'-' inf clause.
//
// Input "-Inf": c1 == 'I'. Real: (c1 == 'i' || c1 == 'I') -> (false || true)
// -> true -> inf branch -> StrToF64("-Inf") -> -inf.
// Mutated (c1 != 'I'): (false || false) -> false -> no inf branch -> digit
// scan collects "-", StrToF64("-") fails -> nothing consumed.
// "-Inf" (uppercase I) is required: with input "-inf" the col-89 mutant's
// surviving `c1 == 'i'` term would still be true, hiding the change.
// ---------------------------------------------------------------------------
static bool test_m19_neg_inf_upper(void) {
    f64  v    = 7.0;
    Zstr z    = "-Inf";
    Zstr orig = z;
    StrReadFmt(z, "{}", v);
    return (z != orig) && F64IsInf(v) && (v < 0.0);
}

// ---------------------------------------------------------------------------
// 2548 col 62: `c == '-'` -> `c != '-'` in the leading-'-' inf clause.
//
// Input "5inf": first char '5' (not '-'), look-ahead c1 == 'i'.
// Real (c == '-' is false): inf branch skipped, digit scan stops at the 'i'
// (not a valid number char) -> token "5" -> v = 5.0, exactly ONE char
// consumed, so the input pointer ends up at 'i'.
// Mutated (c != '-' is true, c1 == 'i'): inf branch entered, the whole
// non-space token "5inf" is scanned; StrToF64("5inf") is non-strict and
// parses the "5" prefix -> succeeds -> all four chars consumed, so the input
// pointer ends up at the terminating NUL.
// The parsed value (5.0) is identical either way; the kill is the consumed
// length, observable as the char the advanced pointer lands on.
// ---------------------------------------------------------------------------
static bool test_m19_dash_clause_not_minus(void) {
    f64  v = 0.0;
    Zstr z = "5inf";
    StrReadFmt(z, "{}", v);
    // Real: advanced by 1 -> *z == 'i'. Mutant: advanced by 4 -> *z == '\0'.
    return double_equals(v, 5.0) && (*z == 'i');
}

// ---------------------------------------------------------------------------
// 2560 col 13: `if (StrToF64(&temp, v, NULL))` return replaced by 42 (truthy).
//
// Input "info": first char 'i' enters the inf/nan branch; the scanned token
// is "info". Real StrToF64("info") rejects it ("inf" with a trailing non-space
// 'o'), so the branch falls back to the digit scan, which immediately stops on
// 'i' -> empty slice -> _read_f64 returns without consuming -> StrReadFmt fails
// (input not advanced) and v is untouched.
// Mutated (StrToF64 forced truthy): the inf branch "succeeds" and returns the
// token end, so the input pointer advances past "info".
// ---------------------------------------------------------------------------
static bool test_m19_inf_branch_strtof64_truthy(void) {
    f64  v    = 55.0;
    Zstr z    = "info";
    Zstr orig = z;
    StrReadFmt(z, "{}", v);
    // Real: not consumed (z unchanged). Mutant: consumed (z advanced).
    return (z == orig);
}

// ---------------------------------------------------------------------------
// 2612 col 36: `StrIterIndex(&si) - StrIterIndex(&saved)` -> `+`.
//
// `saved` is taken AFTER leading spaces are skipped, so with a leading space
// its index is non-zero and `sub` vs `add` give different slice lengths.
// Input " 3.14": after the space skip, saved index == 1, scan end index == 5.
//   Real: pos = 5 - 1 = 4 -> slice "3.14" -> v = 3.14.
//   Mutant: pos = 5 + 1 = 6 -> StrInitFromCstr reads two bytes past the slice
//   (the NUL and beyond) -> is_valid_numeric_string rejects it -> no value.
// (A no-leading-space input would have saved index 0, where sub and add agree;
// the leading space is what makes the operator observable.)
// ---------------------------------------------------------------------------
static bool test_m19_token_length_leading_space(void) {
    f64  v = -1.0;
    Zstr z = " 3.14";
    StrReadFmt(z, "{}", v);
    // Real: v == 3.14. Mutant: parse fails, v left at -1.0.
    return double_equals(v, 3.14);
}

// ---------------------------------------------------------------------------
// 2616 col 10: `if (!is_valid_numeric_string(&temp, true))` -> guard call
// replaced by 42 (truthy), so the negated test is always false and the
// rejection branch is skipped.
//
// Input "1f": the lenient per-char scan accepts 'f' (a hex letter), producing
// the slice "1f". is_valid_numeric_string("1f", true) REJECTS it (a stray
// non-numeric trailing char). Real code therefore rejects "1f" -> no consume,
// v untouched. Mutated: the guard is bypassed and StrToF64("1f") is reached;
// being non-strict it parses the "1" prefix -> v = 1.0 and consumes the slice.
// ---------------------------------------------------------------------------
static bool test_m19_numeric_string_guard(void) {
    f64  v    = 88.0;
    Zstr z    = "1f";
    Zstr orig = z;
    StrReadFmt(z, "{}", v);
    // Real: rejected, not consumed (z unchanged). Mutant: consumed.
    return (z == orig);
}

// ---------------------------------------------------------------------------
// Bare base prefix "0x" must be REJECTED by the float validator.
//
// Collector yields the slice "0x"; len==2, so the `len > 2` prefix guard
// (line 2418) is NOT entered in real code and is_hex stays false. The
// digit walk then hits 'x' at i==1, which no clause accepts -> reject ->
// the f64 keeps its sentinel.
//
// Kills, all of which make the slice wrongly pass:
//   * 2414/2415/2416 cxx_init_const  (is_hex/is_bin/is_oct start true ->
//     the (is_hex||is_bin||is_oct) skip at 2436 swallows both chars)
//   * 2418 cxx_gt_to_ge  (len>2 -> len>=2 enters prefix detect, sets is_hex)
//   * 2431:15 cxx_init_const  (loop index i starts at 42 -> body skipped)
//   * 2431:24 cxx_lt_to_ge  (i<len -> i>=len, loop never runs)
//   * 2431:32 cxx_post_inc_to_post_dec (i-- runs away, loop exits at once)
// Under any of these the slice is accepted, StrToF64("0x") yields 0.0, and
// the f64 is overwritten -> assertion below fires.
static bool test_m2_bare_prefix_0x_rejected(void) {
    WriteFmt("[m2] bare prefix 0x must be rejected by float validator\n");

    f64  v = SENTINEL;
    Zstr z = "0x";
    StrReadFmt(z, "{}", v);

    // Real code: rejected, v untouched. Mutant: accepted -> 0.0.
    return is_sentinel(v);
}

// Lowercase 'b' bare binary prefix, same reasoning as 0x. Adds redundant
// coverage for the init/loop mutants through a different slice so a stray
// surviving case is still caught.
static bool test_m2_bare_prefix_0b_rejected(void) {
    WriteFmt("[m2] bare prefix 0b must be rejected by float validator\n");

    f64  v = SENTINEL;
    Zstr z = "0b";
    StrReadFmt(z, "{}", v);

    return is_sentinel(v);
}

static bool test_m2_bare_prefix_0o_rejected(void) {
    WriteFmt("[m2] bare prefix 0o must be rejected by float validator\n");

    f64  v = SENTINEL;
    Zstr z = "0o";
    StrReadFmt(z, "{}", v);

    return is_sentinel(v);
}

// ---------------------------------------------------------------------------
// A real hex slice "0X1F" must be ACCEPTED (StrToF64 parses the leading
// '0' -> 0.0). This pins the prefix-detection equality checks.
//
//   * 2419:39 cxx_eq_to_ne on `data[1] == 'X'` : with input "0X1F" the
//     lowercase `data[1]=='x'` operand is false, so the 'X' operand alone
//     decides. Mutated to `!= 'X'` it reads false -> is_hex never set ->
//     the digit walk rejects 'X' at i==1 -> v stays sentinel.
//   * Real code sets is_hex, accepts, StrToF64 -> 0.0.
static bool test_m2_hex_prefix_upper_X_accepted(void) {
    WriteFmt("[m2] 0X1F hex slice must be accepted (-> 0.0)\n");

    f64  v = SENTINEL;
    Zstr z = "0X1F";
    StrReadFmt(z, "{}", v);

    // Real code: is_hex set via the 'X' branch, slice accepted, value 0.0.
    return is_zero(v);
}

// 2421:46 cxx_eq_to_ne on `data[1] == 'B'`. Input "0B11": the lowercase
// 'b' operand is false, so the 'B' operand alone enables is_bin. Mutated
// to `!= 'B'` -> is_bin never set -> 'B' rejected -> v stays sentinel.
static bool test_m2_bin_prefix_upper_B_accepted(void) {
    WriteFmt("[m2] 0B11 binary slice must be accepted (-> 0.0)\n");

    f64  v = SENTINEL;
    Zstr z = "0B11";
    StrReadFmt(z, "{}", v);

    return is_zero(v);
}

// 2423:46 cxx_eq_to_ne on `data[1] == 'O'`. Input "0O7": the lowercase
// 'o' operand is false, so the 'O' operand alone enables is_oct. Mutated
// to `!= 'O'` -> is_oct never set -> 'O' rejected -> v stays sentinel.
static bool test_m2_oct_prefix_upper_O_accepted(void) {
    WriteFmt("[m2] 0O7 octal slice must be accepted (-> 0.0)\n");

    f64  v = SENTINEL;
    Zstr z = "0O7";
    StrReadFmt(z, "{}", v);

    return is_zero(v);
}

// ---------------------------------------------------------------------------
// Anchor tests: ordinary well-formed floats still round-trip. These guard
// against an over-eager validator change (e.g. a mutant that rejects valid
// input) and keep the sentinel/zero machinery honest.
static bool test_m2_plain_float_roundtrips(void) {
    WriteFmt("[m2] plain float 12.5 still parses\n");

    f64  v = SENTINEL;
    Zstr z = "12.5";
    StrReadFmt(z, "{}", v);

    return F64Abs(v - 12.5) < 1e-9;
}

static bool test_m2_lowercase_hex_prefix_accepted(void) {
    WriteFmt("[m2] 0x1f hex slice accepted (-> 0.0), guards 'x' operand\n");

    f64  v = SENTINEL;
    Zstr z = "0x1f";
    StrReadFmt(z, "{}", v);

    // Real: is_hex via lowercase 'x', accepted, StrToF64 -> 0.0.
    return is_zero(v);
}

// Read a Float token immediately followed by a literal ';'. On real code the
// reader consumes exactly the token "3.5" and the cursor lands on ';', which
// the format literal then matches, advancing the input pointer to end-of-
// string. If token_len is forced to 42 the cursor jumps far past the NUL, the
// trailing ';' literal cannot match, str_read_fmt returns NULL, and the input
// pointer is left unmoved -- so a fully-consumed cursor distinguishes them.
bool test_m20_float_token_boundary(void) {
    WriteFmt("m20: Float token boundary cursor\n");

    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    bool  success = true;
    Float val     = FloatInit(alloc_base);

    Zstr z = "3.5;";
    StrReadFmt(z, "{};", val);

    // On success the macro advances `z` to the cursor returned by
    // str_read_fmt, which after matching the trailing ';' is end-of-string.
    success = success && (ZstrLen(z) == 0);

    Str text = FloatToStr(&val);
    success  = success && (ZstrCompare(StrBegin(&text), "3.5") == 0);
    StrDeinit(&text);

    FloatDeinit(&val);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Same boundary check without a trailing literal: the token "12.25" is the
// whole input, so the real cursor lands exactly on the NUL (ZstrLen == 0).
// token_len <- 42 would advance start by 42 (well past the 5-char token),
// leaving the reported consumed length wrong; we pin both the cursor and the
// value.
bool test_m20_float_token_boundary_no_trailer(void) {
    WriteFmt("m20: Float token boundary, whole input\n");

    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    bool  success = true;
    Float val     = FloatInit(alloc_base);

    Zstr z = "12.25";
    StrReadFmt(z, "{}", val);

    success = success && (ZstrLen(z) == 0);

    Str text = FloatToStr(&val);
    success  = success && (ZstrCompare(StrBegin(&text), "12.25") == 0);
    StrDeinit(&text);

    FloatDeinit(&val);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Scientific token followed by a literal '!'. token_length must span the full
// "1.5e2" mantissa+exponent (5 chars). A 42-length over-advance skips the '!'
// boundary and breaks the literal match.
bool test_m20_float_scientific_boundary(void) {
    WriteFmt("m20: Float scientific token boundary\n");

    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    bool  success = true;
    Float val     = FloatInit(alloc_base);

    Zstr z = "1.5e2!";
    StrReadFmt(z, "{e}!", val);

    success = success && (ZstrLen(z) == 0);

    // 1.5e2 == 150
    Str text = FloatToStr(&val);
    success  = success && (ZstrCompare(StrBegin(&text), "150") == 0);
    StrDeinit(&text);

    FloatDeinit(&val);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Plain value round-trip: pins that the parsed token maps to the exact
// expected value after a successful read.
bool test_m20_float_value_exact(void) {
    WriteFmt("m20: Float value exact\n");

    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    bool  success = true;
    Float val     = FloatInit(alloc_base);

    Zstr z = "-0.25";
    StrReadFmt(z, "{}", val);

    success = success && (ZstrLen(z) == 0);

    Str text = FloatToStr(&val);
    success  = success && (ZstrCompare(StrBegin(&text), "-0.25") == 0);
    StrDeinit(&text);

    FloatDeinit(&val);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// 0x3ff = 0b11_1111_1111 -> 10 significant bits, clz(value) == 54, so
// 64 - clz == 10. Pins the count initialiser, the >>32/>>48/>>62 branch
// conditions, the +=16/+=4 accumulators, and the <<=4/<<=2 shifts.
static bool test_m23_clz_hex_3ff_bit_len_10(void) {
    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    BitVec bv = BitVecInit(alloc_base);
    Zstr   z  = "0x3ff";
    StrReadFmt(z, "{}", bv);

    bool ok = (BitVecLen(&bv) == 10) && (BitVecToInteger(&bv) == 0x3ff);

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// 0xff = 8 significant bits, clz(value) == 56, 64 - clz == 8. The only
// value among the suite whose clz exercises the >>56 block both ways:
// it pins the `count += 8` accumulator and the `value <<= 8` narrowing
// shift (a `>>=` there would over-narrow and miscount).
static bool test_m23_clz_hex_ff_bit_len_8(void) {
    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    BitVec bv = BitVecInit(alloc_base);
    Zstr   z  = "0xff";
    StrReadFmt(z, "{}", bv);

    bool ok = (BitVecLen(&bv) == 8) && (BitVecToInteger(&bv) == 0xff);

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// 0x10000 = 17 significant bits, clz(value) == 47, 64 - clz == 17. Bit 16
// is the only set bit, so this is the value that exercises the >>48
// block-condition shift (a left-shift there flips the branch) and the
// final `count += 1` accumulation against an odd bit length.
static bool test_m23_clz_hex_10000_bit_len_17(void) {
    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    BitVec bv = BitVecInit(alloc_base);
    Zstr   z  = "0x10000";
    StrReadFmt(z, "{}", bv);

    bool ok = (BitVecLen(&bv) == 17) && (BitVecToInteger(&bv) == 0x10000);

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// Line 238: `if (content_len >= width) return true;` (early no-pad exit).
//
// `{05}` on 42 -> content_len 2 < width 5 -> pad 3 zeros -> "00042".
// Kills 238:21 ge->lt: the `>=` becomes `<`, so content_len(2) < width(5) is
// TRUE -> early-returns with NO padding -> "42" (real "00042").
// Also kills the family of mutants downstream that all collapse the fill:
//   241:10 init_const (pad_len=42 -> 42 zeros, output far longer),
//   241:26 sub->add  (pad_len = 5+2 = 7 -> "0000000" + "42"),
//   244:10 init_const (insert_pos=42, out-of-range insert -> not "00042"),
//   248:15 init_const (i starts 42 >= pad_len -> loop skipped -> "42"),
//   248:24 lt->ge     (0 >= 3 false -> loop skipped -> "42"),
//   248:24 lt->le     (one extra iteration -> "000042"),
//   248:36 inc->dec   (i-- -> i never reaches pad_len -> unbounded fill).
// A single exact "00042" / length-5 assertion rejects every one of those.
// ---------------------------------------------------------------------------
static bool test_m24_zero_pad_positive_exact(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              out   = StrInit(&alloc);
    bool             ok    = true;

    StrAppendFmt(&out, "{05}", (u32)42);
    ok = ok && (ZstrCompare(StrBegin(&out), "00042") == 0);
    ok = ok && (StrLen(&out) == 5);

    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// Line 238 boundary: content_len == width must NOT pad.
//
// `{05}` on 12345 -> content_len 5 == width 5 -> `>=` true -> return true,
// output exactly "12345" (no extra zero).
// Kills 238:21 ge->gt: the `>=` becomes `>`, so 5 > 5 is FALSE -> falls through
// and pads one extra zero -> "012345" (length 6).
// ---------------------------------------------------------------------------
static bool test_m24_zero_pad_width_equals_content_no_pad(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              out   = StrInit(&alloc);
    bool             ok    = true;

    StrAppendFmt(&out, "{05}", (u32)12345);
    ok = ok && (ZstrCompare(StrBegin(&out), "12345") == 0);
    ok = ok && (StrLen(&out) == 5);

    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// Line 245 sign detection (negative): the '0' fill goes AFTER the leading '-'.
//
// `{06}` on -42 -> content "-42" (content_len 3) < width 6 -> pad 3 zeros after
// the sign -> "-00042".
// Kills:
//   245:21 gt->le  (content_len > 0 -> content_len <= 0, false for len 3 -> sign
//                   branch never taken -> fill before '-' -> "000-42"),
//   246:20 +=->-=  (insert_pos = content_start - 1 -> insert before the field /
//                   underflow -> not "-00042"; aborts or corrupts output).
// Exact "-00042" rejects both.
// ---------------------------------------------------------------------------
static bool test_m24_zero_pad_negative_sign_kept_ahead(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              out   = StrInit(&alloc);
    bool             ok    = true;

    StrAppendFmt(&out, "{06}", (i32)-42);
    ok = ok && (ZstrCompare(StrBegin(&out), "-00042") == 0);
    ok = ok && (StrLen(&out) == 6);

    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// Line 245 sign detection (positive): with NO leading sign, the '0' fill goes
// at the very start of the content.
//
// `{05}` on 42 -> first char '4' is neither '-' nor '+' -> insert_pos stays at
// content_start -> "00042".
// Kills:
//   245:57 eq->ne (StrCharAt == '-' becomes != '-': '4' != '-' is TRUE -> sign
//                  branch taken -> insert_pos += 1 -> fill after first digit ->
//                  "40002"),
//   245:95 eq->ne (StrCharAt == '+' becomes != '+': second operand '4' != '+'
//                  is TRUE -> insert_pos += 1 -> "40002").
// "40002" (real "00042") is rejected by the exact assertion. Distinct from the
// first test by directly pinning the no-sign placement contract.
// ---------------------------------------------------------------------------
static bool test_m24_zero_pad_positive_no_sign_shift(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              out   = StrInit(&alloc);
    bool             ok    = true;

    StrAppendFmt(&out, "{05}", (i32)42);
    ok = ok && (ZstrCompare(StrBegin(&out), "00042") == 0);

    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// Line 245 plus-sign path: a forced-plus value keeps '+' ahead of the fill.
//
// Note: the default integer spec does not emit '+', but StrFromI64 of a
// negative renders '-'. To exercise the '+' literal independently we use a
// negative value rendered then re-checked: here we instead assert the
// width math for a longer negative so the second '||' operand ('+') is the one
// that actually decides placement on a '+'-less, '-'-less... not reachable for
// plain ints. We therefore pin the '-' branch with a wider field so the
// arithmetic (line 241) cannot accidentally match.
//
// `{08}` on -7 -> content "-7" (len 2) -> pad 6 zeros after '-' -> "-0000007".
// Reinforces 245:21/246:20/241:26 with a different width so a width+content
// (sub->add) confusion (2+8=10 zeros) is also length-distinguished.
// ---------------------------------------------------------------------------
static bool test_m24_zero_pad_negative_wide(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              out   = StrInit(&alloc);
    bool             ok    = true;

    StrAppendFmt(&out, "{08}", (i32)-7);
    ok = ok && (ZstrCompare(StrBegin(&out), "-0000007") == 0);
    ok = ok && (StrLen(&out) == 8);

    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// Line 248 loop boundary (extra-iteration mutant): exactly pad_len zeros.
//
// `{016x}` on 0xFF -> hex "ff" (content_len 2) zero-padded (prefix suppressed)
// to width 16 -> 14 zeros + "ff" -> "00000000000000ff" (length 16).
// Kills:
//   248:24 lt->le  (loop runs pad_len+1 = 15 times -> length 17),
//   248:15 init_const / 248:24 lt->ge (loop skipped -> "ff"),
//   241:* width math errors (wrong zero count).
// A wide hex field makes the exact length the discriminator.
// ---------------------------------------------------------------------------
static bool test_m24_zero_pad_hex_width16_exact(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              out   = StrInit(&alloc);
    bool             ok    = true;

    StrAppendFmt(&out, "{016x}", (u32)0xFF);
    ok = ok && (ZstrCompare(StrBegin(&out), "00000000000000ff") == 0);
    ok = ok && (StrLen(&out) == 16);

    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Integer-valued Float "12" -> canonical "12" has no '.'. With precision 3 the
// no-dot branch must append '.' then exactly three '0' digits -> "12.000".
//   line 1368 precision>0  : gt_to_le makes it false -> "12" (no dot/zeros).
//   line 1372 i=0          : init_const i<-42 -> loop body never runs -> "12.".
//   line 1372 i<precision  : lt_to_ge/lt_to_le -> wrong zero count.
//   line 1372 i++ -> i--    : decrement underflows u32, loop never terminates
//                            (runaway append until abort) -> test cannot return
//                            "12.000".
bool test_m25_nodot_precision_pads_zeros(void) {
    WriteFmt("m25: no-dot integer, precision pads trailing zeros\n");

    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    bool  success = true;
    Float val     = FloatFromStr("12", alloc_base);

    Str out;
    success = success && float_try_to_decimal_str(&out, &val, 3, true, alloc_base);
    success = success && (ZstrCompare(StrBegin(&out), "12.000") == 0);
    StrDeinit(&out);

    FloatDeinit(&val);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Integer-valued Float "12" with precision 0: no-dot branch must NOT emit a
// trailing '.', producing bare "12".
//   line 1368 precision>0 : gt_to_ge makes it true at precision==0 -> "12.".
bool test_m25_nodot_precision_zero_no_dot(void) {
    WriteFmt("m25: no-dot integer, precision 0 yields no dot\n");

    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    bool  success = true;
    Float val     = FloatFromStr("12", alloc_base);

    Str out;
    success = success && float_try_to_decimal_str(&out, &val, 0, true, alloc_base);
    success = success && (ZstrCompare(StrBegin(&out), "12") == 0);
    StrDeinit(&out);

    FloatDeinit(&val);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Fractional Float "1.5" (canonical has a '.') with precision 3: the with-dot
// branch copies "5" then pads two trailing zeros -> "1.500".
//   line 1397 i++ -> i-- : decrementing pad index underflows and never reaches
//                          `precision`, so the test cannot produce "1.500".
bool test_m25_dot_precision_pads_zeros(void) {
    WriteFmt("m25: with-dot fraction, precision pads trailing zeros\n");

    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    bool  success = true;
    Float val     = FloatFromStr("1.5", alloc_base);

    Str out;
    success = success && float_try_to_decimal_str(&out, &val, 3, true, alloc_base);
    success = success && (ZstrCompare(StrBegin(&out), "1.500") == 0);
    StrDeinit(&out);

    FloatDeinit(&val);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Fractional Float "1.5" with precision 0: with-dot branch must drop the
// fractional part entirely and emit just the integer prefix "1".
//   line 1390 precision>0 : gt_to_ge makes it true at precision==0 -> "1.".
bool test_m25_dot_precision_zero_truncates_dot(void) {
    WriteFmt("m25: with-dot fraction, precision 0 drops fraction\n");

    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    bool  success = true;
    Float val     = FloatFromStr("1.5", alloc_base);

    Str out;
    success = success && float_try_to_decimal_str(&out, &val, 0, true, alloc_base);
    success = success && (ZstrCompare(StrBegin(&out), "1") == 0);
    StrDeinit(&out);

    FloatDeinit(&val);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Fractional Float "1.25" with precision 1: with-dot branch keeps
// MIN2(frac=2, precision=1)=1 fraction digit, no padding loop -> "1.2".
// Pins the prefix/frac span computation and the MIN2 truncation.
bool test_m25_dot_precision_truncates_fraction(void) {
    WriteFmt("m25: with-dot fraction, precision truncates\n");

    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    bool  success = true;
    Float val     = FloatFromStr("1.25", alloc_base);

    Str out;
    success = success && float_try_to_decimal_str(&out, &val, 1, true, alloc_base);
    success = success && (ZstrCompare(StrBegin(&out), "1.2") == 0);
    StrDeinit(&out);

    FloatDeinit(&val);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Kills 2067:52 cxx_and_to_or on the precision ternary
// `fmt_info->flags & FMT_FLAG_HAS_PRECISION ? fmt_info->precision : 6`.
// With no precision spec, real code uses the default precision 6 ->
// "1.500000". An `&`->`|` makes the condition always non-zero, so it uses
// fmt_info->precision (0 by default) and renders "2" instead.
static bool test_m26_default_precision_six(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str  output = StrInit(&alloc);
    bool ok     = true;

    f64 v = 1.5;
    StrAppendFmt(&output, "{}", v);
    ok = ok && (ZstrCompare(StrBegin(&output), "1.500000") == 0);

    StrDeinit(&output);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Kills 2071:46 cxx_and_to_or and 2071:63 cxx_ne_to_eq on the `uppercase`
// config: `(fmt_info->flags & FMT_FLAG_CAPS) != 0`.
// Lowercase scientific `{e}` of 1.5 must emit a lowercase 'e' -> "1.500000e+00".
// An `&`->`|` forces uppercase always true; a `!=`->`==` inverts the test;
// either way the lowercase case would print 'E' instead of 'e'.
static bool test_m26_scientific_lowercase_e(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str  output = StrInit(&alloc);
    bool ok     = true;

    f64 v = 1.5;
    StrAppendFmt(&output, "{e}", v);
    ok = ok && (ZstrCompare(StrBegin(&output), "1.500000e+00") == 0);

    StrDeinit(&output);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Companion to the above for the inverse polarity (2071:63 cxx_ne_to_eq):
// uppercase scientific `{E}` of 1.5 must emit an uppercase 'E' -> "1.500000E+00".
// A `!=`->`==` swap would render lowercase 'e' here.
static bool test_m26_scientific_uppercase_e(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str  output = StrInit(&alloc);
    bool ok     = true;

    f64 v = 1.5;
    StrAppendFmt(&output, "{E}", v);
    ok = ok && (ZstrCompare(StrBegin(&output), "1.500000E+00") == 0);

    StrDeinit(&output);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Kills 2030:10 cxx_init_const (`size start_len = StrLen(o)` -> 42) and
// 2086:14 cxx_init_const (`content_len = StrLen(o) - start_len` -> 42).
// Formatting into an empty Str with width 12: real content_len is 8
// ("1.500000"), padded with 4 leading spaces -> "    1.500000" (len 12).
// If start_len is forced to 42, content_len = 8 - 42 underflows huge and no
// padding happens. If content_len is forced to 42 (>= width), no padding
// happens either. Both diverge from the padded result.
static bool test_m26_width_pad_from_empty(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str  output = StrInit(&alloc);
    bool ok     = true;

    f64 v = 1.5;
    StrAppendFmt(&output, "{12}", v);
    ok = ok && (ZstrCompare(StrBegin(&output), "    1.500000") == 0);
    ok = ok && (StrLen(&output) == 12);

    StrDeinit(&output);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Kills 2086:38 cxx_sub_to_add (`StrLen(o) - start_len` -> `+`).
// Pre-seed the output with a 3-char prefix so start_len != 0. Real content_len
// for the field is (3 + 8) - 3 = 8, padded to width 12 with 4 pad spaces.
// StrPad ALIGN_RIGHT pushes the pad to the FRONT of the whole buffer, so the
// "XYZ" prefix rides along after the spaces: "    XYZ1.500000" (len 15).
// A `-`->`+` makes content_len 11 + 3 = 14 >= 12, so no padding is emitted
// -> "XYZ1.500000".
static bool test_m26_width_pad_with_prefix(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str  output = StrInit(&alloc);
    bool ok     = true;

    StrPushBackMany(&output, "XYZ");

    f64 v = 1.5;
    StrAppendFmt(&output, "{12}", v);
    ok = ok && (ZstrCompare(StrBegin(&output), "    XYZ1.500000") == 0);

    StrDeinit(&output);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Kills 2085:25 cxx_gt_to_le (`fmt_info->width > 0` -> `width <= 0`).
// width is u32; `<= 0` is true only when width == 0, so with an explicit
// width 12 the mutated guard skips StrPad entirely and emits no padding.
// Real code pads -> "    1.500000"; mutated -> "1.500000".
static bool test_m26_width_guard_pads_when_set(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str  output = StrInit(&alloc);
    bool ok     = true;

    f64 v = 1.5;
    StrAppendFmt(&output, "{12}", v);
    ok = ok && (StrLen(&output) == 12);

    StrDeinit(&output);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Exponent value 0 (e.g. 1.0 -> "1e+00").
//   - 1284:31 cxx_lt_to_le: `exponent < 0 ? '-' : '+'` -> `<=` makes the
//     zero exponent take the '-' branch ("1e-00").
//   - 1297:29 cxx_post_inc_to_post_dec: in the magnitude==0 branch
//     `data[digit_count++]` becomes `digit_count--`, underflowing the
//     count and corrupting the emitted digits.
//   - 1297:33 cxx_assign_const: `data[digit_count++] = '0'` becomes `= 42`
//     ('*'), so the exponent digit is wrong.
static bool test_m27_exp_zero_yields_plus_zero(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str   out = StrInit(&alloc);
    Float v   = FloatFromStr("1.0", &alloc.base);

    StrAppendFmt(&out, "{e}", v);
    bool ok = (ZstrCompare(StrBegin(&out), "1e+00") == 0);

    FloatDeinit(&v);
    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Negative exponent (0.001 -> "1e-03"). The magnitude of a negative
// exponent is computed as `(u64)(-(exponent + 1)) + 1`.
//   - 1285:43 cxx_minus_to_noop: drops the unary minus -> huge u64.
//   - 1285:54 cxx_add_to_sub: `exponent + 1` -> `exponent - 1` (mag 5).
//   - 1285:60 cxx_add_to_sub: trailing `+ 1` -> `- 1` (mag 1, "e-01").
static bool test_m27_exp_negative_magnitude(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str   out = StrInit(&alloc);
    Float v   = FloatFromStr("0.001", &alloc.base);

    StrAppendFmt(&out, "{e}", v);
    bool ok = (ZstrCompare(StrBegin(&out), "1e-03") == 0);

    FloatDeinit(&v);
    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Two-digit exponent (1e12 -> "1e+12"). The pad-to-two-digits guard
// `if (digit_count < 2)` must NOT fire when there are already two digits.
//   - 1310:25 cxx_lt_to_le: `digit_count < 2` -> `<= 2` pads an extra '0'
//     ("1e+012").
static bool test_m27_exp_two_digits_no_extra_pad(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str   out = StrInit(&alloc);
    Float v   = FloatFromStr("1e12", &alloc.base);

    StrAppendFmt(&out, "{e}", v);
    bool ok = (ZstrCompare(StrBegin(&out), "1e+12") == 0);

    FloatDeinit(&v);
    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// After a decimal point a sign must terminate the token. For "1.+5" the
// real scanner stops after "1." (a valid float == 1), then the trailing
// "+5" is left unconsumed.
//   - 1540:25 cxx_assign_const: `allow_sign = false` -> `= 42` (truthy)
//     keeps signs allowed, so the token grows to "1.+5", which fails to
//     parse and leaves the Float at the sentinel (99).
static bool test_m27_token_sign_after_decimal_stops(void) {
    return read_float_equals("1.+5", "1");
}

// After a digit a sign must terminate the token (no interior sign). For
// "5+3" the real scanner stops after "5" (a valid float == 5).
//   - 1527:28 cxx_assign_const: `allow_sign = false` -> `= 42` (truthy)
//     keeps signs allowed after a digit, so the token grows to "5+3",
//     which fails to parse and leaves the Float at the sentinel (99).
static bool test_m27_token_sign_after_digit_stops(void) {
    return read_float_equals("5+3", "5");
}

// A non-special trailing character (here 'z') must terminate the token.
// For "5z" the real scanner stops after "5" (a valid float == 5).
//   - 1545:30 cxx_eq_to_ne: `ch == 'E'` -> `ch != 'E'` makes any non-'E'
//     character (e.g. 'z') satisfy the exponent introducer, demanding a
//     trailing exponent digit and forcing token_length to return 0, so the
//     Float read fails and the sentinel (99) survives.
static bool test_m27_token_trailing_letter_stops(void) {
    return read_float_equals("5z", "5");
}

static bool test_m28_fwrite_roundtrip_string(void) {
    Zstr path = "io_mutants28_str.txt"; // CWD-relative: portable (no /tmp on Windows)
    File f    = FileOpen(path, "w");
    if (!FileIsOpen(&f)) {
        return false;
    }

    Zstr s   = "World";
    bool ret = FWriteFmt(&f, "Hello {}!", s);
    FileClose(&f);

    // Real: write succeeds -> ret true, file holds the exact rendered line.
    // gt_to_le mutant would skip the write (empty file); ne_to_eq mutant would
    // return false on the successful write.
    return ret && roundtrip_eq(path, "Hello World!");
}

static bool test_m28_fwrite_roundtrip_int(void) {
    Zstr path = "io_mutants28_int.txt"; // CWD-relative: portable (no /tmp on Windows)
    File f    = FileOpen(path, "w");
    if (!FileIsOpen(&f)) {
        return false;
    }

    i32  n   = 12345;
    bool ret = FWriteFmt(&f, "n={}", n);
    FileClose(&f);

    return ret && roundtrip_eq(path, "n=12345");
}

// FWriteFmtLn appends the trailing newline; reading it back pins that the
// newline is part of the single written line and the write-count identity
// compare still holds (543:83) with the '\n' counted.
static bool test_m28_fwriteln_roundtrip(void) {
    Zstr path = "io_mutants28_ln.txt"; // CWD-relative: portable (no /tmp on Windows)
    File f    = FileOpen(path, "w");
    if (!FileIsOpen(&f)) {
        return false;
    }

    bool ret = FWriteFmtLn(&f, "line");
    FileClose(&f);

    return ret && roundtrip_eq(path, "line\n");
}

static bool test_m28_i64_hex_lower(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              out   = StrInit(&alloc);

    i64 v = 255; // 0xff
    StrAppendFmt(&out, "{x}", v);
    // Real: lowercase hex "0xff". base->42 mutant fails (not "0xff");
    // and_to_or mutant would force caps -> "0xFF".
    bool ok = (ZstrCompare(StrBegin(&out), "0xff") == 0);

    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

static bool test_m28_i64_hex_upper(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              out   = StrInit(&alloc);

    i64 v = 255;
    StrAppendFmt(&out, "{X}", v);
    // Real: uppercase hex "0xFF". ne_to_eq mutant inverts caps -> "0xff".
    bool ok = (ZstrCompare(StrBegin(&out), "0xFF") == 0);

    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

static bool test_m28_i64_binary(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              out   = StrInit(&alloc);

    i64 v = 5; // 0b101
    StrAppendFmt(&out, "{b}", v);
    bool ok = (ZstrCompare(StrBegin(&out), "0b101") == 0);

    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

static bool test_m28_i64_octal(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              out   = StrInit(&alloc);

    i64 v = 8; // 0o10
    StrAppendFmt(&out, "{o}", v);
    bool ok = (ZstrCompare(StrBegin(&out), "0o10") == 0);

    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Negative i64 in plain decimal (base stays 10) renders the sign correctly.
static bool test_m28_i64_decimal_negative(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              out   = StrInit(&alloc);

    i64 v = -42;
    StrAppendFmt(&out, "{}", v);
    bool ok = (ZstrCompare(StrBegin(&out), "-42") == 0);

    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

static bool test_m28_i64_width_after_prefix(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              out   = StrInit(&alloc);

    i64 v = 42;
    StrAppendFmt(&out, "X{5}", v);
    bool ok = (ZstrCompare(StrBegin(&out), "   X42") == 0);

    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Zero-pad width after a prefix exercises pad_numeric_zeros with start_len>0,
// reinforcing the content-length computation (1958) on the zero-pad branch.
static bool test_m28_i64_zeropad_after_prefix(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              out   = StrInit(&alloc);

    i64 v = 42;
    StrAppendFmt(&out, "X{05}", v);
    // start_len=1, content "42" len 2, width 5 -> three '0' fills: "X00042".
    bool ok = (ZstrCompare(StrBegin(&out), "X00042") == 0);

    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// `\x41` -> h=4,l=1 -> (4<<4)|1 = 0x41 = 'A'.
// Kills: 51/52 init-const+scalar-call (h/l forced to 42 changes the byte),
//        55 lshift->rshift (4>>4|1 = 1), 55 or->and (0x40&1 = 0).
static bool test_m29_hexbyte_printable_roundtrip(void) {
    u8   v = 0;
    Zstr z = "\\x41";
    StrReadFmt(z, "{c}", v);
    return v == 0x41;
}

// `\x01` -> h=0,l=1. Real: byte 0x01.
// Kills: 53 col11 `h < 0` -> `<=` (h==0 would wrongly read as invalid and
//        salvage the literal '\\' = 0x5C instead).
static bool test_m29_hexbyte_high_nibble_zero(void) {
    u8   v = 0;
    Zstr z = "\\x01";
    StrReadFmt(z, "{c}", v);
    return v == 0x01;
}

// `\x10` -> h=1,l=0. Real: byte 0x10.
// Kills: 53 col20 `l < 0` -> `<=` (l==0 would wrongly read as invalid).
static bool test_m29_hexbyte_low_nibble_zero(void) {
    u8   v = 0;
    Zstr z = "\\x10";
    StrReadFmt(z, "{c}", v);
    return v == 0x10;
}

// Mixed letter/digit nibbles further constrain hex_nibble's branches and the
// `h < 0 || l < 0` guard (53 lt->ge: a valid pair must NOT be rejected).
// `\x4f` -> 0x4f = 'O'.
static bool test_m29_hexbyte_letter_nibble(void) {
    u8   v = 0;
    Zstr z = "\\x4f";
    StrReadFmt(z, "{c}", v);
    return v == 0x4f;
}

// ---------------------------------------------------------------------------
// _write_Int(): `{c}` (FMT_FLAG_CHAR) path. Formatting an Int whose value is a
// single printable byte must emit exactly that character.
//   Int(0x41) -> byte_len 1 -> IntToBytesBE writes {0x41} -> "A".
// Kills:
//   2997 init-const / scalar-call on byte_len (42 -> "A" + 41 NUL escapes),
//   2999 eq->ne on `byte_len == 0` (early empty return),
//   3010 scalar-call on IntToBytesBE (buffer stays zero -> "\\x00"),
//   3011 scalar-call on write_char_internal (truthy -> body skipped, "" out).
// ---------------------------------------------------------------------------
static bool test_m29_write_int_char_single(void) {
    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    Str  output  = StrInit(&alloc);
    Int  a       = IntFrom(0x41, alloc_base); // 'A'
    bool success = true;

    StrAppendFmt(&output, "{c}", a);
    success = (ZstrCompare(StrBegin(&output), "A") == 0);

    IntDeinit(&a);
    StrDeinit(&output);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// A different single-byte value guards against the char path collapsing to a
// constant: Int(0x7a) -> 'z'.
static bool test_m29_write_int_char_single_alt(void) {
    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    Str  output  = StrInit(&alloc);
    Int  a       = IntFrom(0x7a, alloc_base); // 'z'
    bool success = true;

    StrAppendFmt(&output, "{c}", a);
    success = (ZstrCompare(StrBegin(&output), "z") == 0);

    IntDeinit(&a);
    StrDeinit(&output);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// ---------------------------------------------------------------------------
// _write_Int(): decimal padding path. content_len = StrLen(o) - start_len.
// With a NON-empty destination (start_len != 0) the subtraction is observable:
//   pre-fill "ab" (start_len 2), append "{>6}" of 7 -> render "7" (StrLen 3).
//   real:    content_len = 3 - 2 = 1 -> pad 5 spaces -> "     ab7".
//   sub->add: content_len = 3 + 2 = 5 -> pad 1 space  -> " ab7".
// Kills: 3040 sub->add. (start_len>0 also makes 3040 non-equivalent.)
// ---------------------------------------------------------------------------
static bool test_m29_write_int_width_content_len(void) {
    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    Str  output  = StrInit(&alloc);
    Int  seven   = IntFrom(7, alloc_base);
    bool success = true;

    StrAppendFmt(&output, "ab");
    StrAppendFmt(&output, "{>6}", seven);
    success = (ZstrCompare(StrBegin(&output), "     ab7") == 0);

    IntDeinit(&seven);
    StrDeinit(&output);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// ---------------------------------------------------------------------------
// Unclosed `{` boundary: `brace_end >= fmt_len` (line 320). With an arg
// supplied so the not-enough-args guard can't mask it, a trailing `{`
// must make str_append_fmt FAIL. The cxx_ge_to_gt mutant lets brace_end
// == fmt_len slip through and formats the arg instead, returning true.
// ---------------------------------------------------------------------------
static bool test_m3_unclosed_brace_fails(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              out   = StrInit(&alloc);

    i32  v  = 5;
    bool ok = !StrAppendFmt(&out, "{", v); // unclosed -> must return false

    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// Raw `{1r}` on a u8: pins the u8/i8 var_width arm (line 364/365), the
// `x = *(u8 *)data` extraction (line 393), the `u8 y = (u8)x` narrowing
// (line 416) and the `_write_u8` call (line 417). Value 200 != 42 so any
// "value-becomes-42" mutant and any dropped-call mutant diverge.
// ---------------------------------------------------------------------------
static bool test_m3_raw_u8(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              out   = StrInit(&alloc);

    u8   v  = 200;
    bool ok = StrAppendFmt(&out, "{1r}", v);
    ok      = ok && (ZstrCompare(StrBegin(&out), "200") == 0);

    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// Raw `{2r}` on a u16 value 258 (= 0x0102): pins the u16/i16 arm
// (line 366/367), the `x = *(u16 *)data` extraction (line 397) and the
// `u16 y`/`_write_u16` output (lines 423/424). Also kills the u8/i8 arm
// eq->ne mutants (line 364): under those, var_width collapses to 1 and a
// single low byte (0x02 = 2) is read instead of 258.
// ---------------------------------------------------------------------------
static bool test_m3_raw_u16(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              out   = StrInit(&alloc);

    u16  v  = 258;
    bool ok = StrAppendFmt(&out, "{2r}", v);
    ok      = ok && (ZstrCompare(StrBegin(&out), "258") == 0);

    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// Raw `{4r}` on a u32 value 65538 (= 0x00010002): pins the u32/i32/f32
// arm (line 368/369/370), the `x = *(u32 *)data` extraction (line 401)
// and `u32 y`/`_write_u32` output (lines 430/431). Also kills the u16/i16
// arm eq->ne mutants (line 366): those force var_width to 2, reading the
// low 16 bits (0x0002 = 2) instead of 65538.
// ---------------------------------------------------------------------------
static bool test_m3_raw_u32(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              out   = StrInit(&alloc);

    u32  v  = 65538;
    bool ok = StrAppendFmt(&out, "{4r}", v);
    ok      = ok && (ZstrCompare(StrBegin(&out), "65538") == 0);

    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// Raw `{8r}` on a u64 value 4294967298 (= 0x0000000100000002): pins the
// u64 compare (line 371:37), the var_width=8 assignment (line 373), the
// `x = *(u64 *)data` extraction (line 405) and the `_write_u64` output
// (line 437). Also kills the u32/i32/f32 arm eq->ne mutants (line 368):
// those force var_width to 4, reading the low 32 bits (0x00000002 = 2).
// ---------------------------------------------------------------------------
static bool test_m3_raw_u64(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              out   = StrInit(&alloc);

    u64  v  = 4294967298ULL;
    bool ok = StrAppendFmt(&out, "{8r}", v);
    ok      = ok && (ZstrCompare(StrBegin(&out), "4294967298") == 0);

    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// Raw `{8r}` on an i64: pins the i64 compare (line 371:71). The eq->ne
// mutant flips the matching `write_fn == _write_i64` to false, so the
// writer falls through to the unsupported-type else and str_append_fmt
// returns false. Real code formats the value and returns true.
// ---------------------------------------------------------------------------
static bool test_m3_raw_i64(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              out   = StrInit(&alloc);

    i64  v  = 4294967298LL;
    bool ok = StrAppendFmt(&out, "{8r}", v);
    ok      = ok && (ZstrCompare(StrBegin(&out), "4294967298") == 0);

    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// Raw `{8r}` on an f64: pins the f64 compare (line 372:37). The eq->ne
// mutant flips the matching `write_fn == _write_f64` to false, dropping
// the f64 arm to the unsupported-type else (returns false). Real code
// reinterprets the f64 bits as u64 and renders a non-empty decimal.
// ---------------------------------------------------------------------------
static bool test_m3_raw_f64(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              out   = StrInit(&alloc);

    f64  v  = 1.5;
    bool ok = StrAppendFmt(&out, "{8r}", v);
    ok      = ok && (StrLen(&out) > 0); // returns false under the mutant

    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Width-pad a u64 that is appended onto a Str that ALREADY has content. The
// non-zero `start_len` makes `StrLen(o) - start_len` (line 1858) distinguish:
//   - cxx_init_const on start_len (1824:10): start_len->42, content_len
//     underflows, padding count wrong.
//   - cxx_init_const on content_len (1858:14): content_len->42 (>= width) so
//     StrPad no-ops, padding dropped.
//   - cxx_sub_to_add on StrLen(o)-start_len (1858:38): with start_len != 0 the
//     '+' inflates content_len so StrPad no-ops, padding dropped.
//   - cxx_gt_to_le on width>0 (1857:25): width(8)<=0 is false, padding skipped.
static bool test_m30_u64_width_pad_with_prefix(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              out   = StrInit(&alloc);
    bool             ok    = true;

    // Pre-existing content so start_len != 0 at the point _write_u64 runs.
    StrAppendFmt(&out, "X:");
    u64 v = 255;
    // content_len = StrLen(o) - start_len = 3 ("255"), width 8 -> 5 pad spaces.
    // StrPad ALIGN_RIGHT prepends the pad to the FRONT of the whole buffer, so
    // the "X:" prefix rides along after the spaces: "     X:255" (len 10).
    StrAppendFmt(&out, "{8}", v);
    ok = ok && (ZstrCompare(StrBegin(&out), "     X:255") == 0);

    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Same levers without a prefix, exercising start_len == 0 and a clean
// right-pad. Redundantly pins 1824/1857(le)/1858 for the simple path.
static bool test_m30_u64_width_pad_no_prefix(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              out   = StrInit(&alloc);
    bool             ok    = true;

    u64 v = 7;
    StrAppendFmt(&out, "{5}", v); // "    7"
    ok = ok && (ZstrCompare(StrBegin(&out), "    7") == 0);
    StrClear(&out);

    // Left-align variant so a dropped pad is unambiguous.
    StrAppendFmt(&out, "{<5}", v); // "7    "
    ok = ok && (ZstrCompare(StrBegin(&out), "7    ") == 0);

    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Zero-pad path of _write_u64: `{05}` routes through pad_numeric_zeros via
// content_len = StrLen(o) - start_len, with a non-zero start_len. Pins the
// content_len arithmetic (1824/1858) on the zero-pad branch too.
static bool test_m30_u64_zero_pad_with_prefix(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              out   = StrInit(&alloc);
    bool             ok    = true;

    StrAppendFmt(&out, "p");       // start_len = 1
    u64 v = 42;
    StrAppendFmt(&out, "{05}", v); // zero-pad width 5 -> "00042"
    ok = ok && (ZstrCompare(StrBegin(&out), "p00042") == 0);

    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Patch that EXACTLY fills the remaining buffer must succeed.
//   StrLen(tmp)=4, BufLength=8, offset=4 -> remaining 4, 4 > 4 is false -> ok.
// Kills:
//   - cxx_gt_to_ge on StrLen > remaining (1075:53): 4 >= 4 -> error -> false.
//   - cxx_gt_to_le on StrLen > remaining (1075:53): 4 <= 4 -> error -> false.
//   - cxx_gt_to_le on offset > BufLength (1075:20): 4 <= 8 -> error -> false.
// Also asserts the exact bytes landed, pinning the MemCopy branch.
static bool test_m30_patch_exact_fit_succeeds(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Buf              b     = BufInit(&alloc);
    bool             ok    = true;

    const u8 init[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    BufPushBytes(&b, init, 8);

    // u32 LE 0x11223344 -> bytes 44 33 22 11 at [4,8).
    ok = ok && BufPatchFmt(&b, 4, "{<4r}", (u32)0x11223344);
    ok = ok && (BufLength(&b) == 8);
    {
        const u8 *d = BufData(&b);
        ok          = ok && d[4] == 0x44 && d[5] == 0x33 && d[6] == 0x22 && d[7] == 0x11;
        // Bytes before the patch are untouched.
        ok = ok && d[0] == 0 && d[3] == 0;
    }

    BufDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Zero-byte patch at offset == BufLength must succeed (writing nothing at the
// very end is legal). Kills cxx_gt_to_ge on offset > BufLength (1075:20):
//   8 >= 8 -> error -> false, while real `8 > 8` is false -> true.
static bool test_m30_patch_empty_at_end_succeeds(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Buf              b     = BufInit(&alloc);
    bool             ok    = true;

    const u8 init[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    BufPushBytes(&b, init, 8);

    // Empty format renders zero bytes; offset 8 == BufLength.
    ok = ok && BufPatchFmt(&b, 8, "");
    ok = ok && (BufLength(&b) == 8);
    {
        const u8 *d = BufData(&b);
        ok          = ok && d[0] == 1 && d[7] == 8; // untouched
    }

    BufDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Over-long patch must FAIL and leave the buffer unchanged.
//   StrLen(tmp)=6, BufLength=8, offset=4 -> remaining 4; 6 > 4 -> error.
// Kills:
//   - cxx_assign_const on ok = false (1082:16): mutant keeps ok truthy.
//   - cxx_sub_to_add on BufLength - offset (1075:70): 6 > 8+4=12 is false, so
//     the mutant skips the error and (real) returns true; we assert false.
//   - cxx_gt_to_le on offset > BufLength (1075:20) is also caught (4 <= 8).
static bool test_m30_patch_overlong_fails(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Buf              b     = BufInit(&alloc);
    bool             ok    = true;

    const u8 init[8] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA};
    BufPushBytes(&b, init, 8);

    // {<2r}{<4r} renders 2 + 4 = 6 bytes at offset 4 -> needs [4,10), past end.
    bool rc = BufPatchFmt(&b, 4, "{<2r}{<4r}", (u16)0xBEEF, (u32)0xDEADC0DE);
    ok      = ok && (rc == false);
    ok      = ok && (BufLength(&b) == 8);
    {
        const u8 *d = BufData(&b);
        // On overflow the buffer is documented to be left unchanged.
        ok = ok && d[4] == 0xAA && d[5] == 0xAA && d[6] == 0xAA && d[7] == 0xAA;
    }

    BufDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A 1-byte raw field. The width-identity guard requires fmt width (1) to equal
// the variable width derived from the writer. Kills 958:19 assign_const
// (var_width = 1 -> 42): the guard would then LOG_FATAL on a real u8 write.
// Also kills 978:16 init_const (u8 v = *data -> 42): the emitted byte would
// be 0x2A instead of the real value 0x7E.
static bool test_m31_raw_u8(void) {
    DefaultAllocator alloc    = DefaultAllocatorInit();
    Buf              b        = BufInit(&alloc);
    bool             ok       = BufAppendFmt(&b, "{<1r}", (u8)0x7E);
    const u8         expect[] = {0x7E};
    ok = ok && BufLength(&b) == sizeof(expect) && MemCompare(BufData(&b), expect, sizeof(expect)) == 0;
    BufDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// An 8-byte signed raw field. The width-8 dispatch arm is
//   write_fn == _write_u64 || write_fn == _write_i64 || write_fn == _write_f64
// Kills 963:59 eq_to_ne (the _write_i64 comparison): for an i64 writer that
// arm becomes false on all three terms, dropping into the unsupported-type
// LOG_FATAL. Real path emits the little-endian bytes.
static bool test_m31_raw_i64(void) {
    DefaultAllocator alloc    = DefaultAllocatorInit();
    Buf              b        = BufInit(&alloc);
    bool             ok       = BufAppendFmt(&b, "{<8r}", (i64)0x0102030405060708ll);
    const u8         expect[] = {0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01};
    ok = ok && BufLength(&b) == sizeof(expect) && MemCompare(BufData(&b), expect, sizeof(expect)) == 0;
    BufDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// An 8-byte f64 raw field exercises the last term of the width-8 arm.
// Kills 963:93 eq_to_ne (the _write_f64 comparison): for an f64 writer that
// comparison inverts, the arm goes false, and the real code LOG_FATALs.
static bool test_m31_raw_f64(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Buf              b     = BufInit(&alloc);
    union {
        f64 f;
        u64 u;
    } in              = {.u = 0x0102030405060708ull};
    bool     ok       = BufAppendFmt(&b, "{<8r}", in.f);
    const u8 expect[] = {0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01};
    ok                = ok && BufLength(&b) == sizeof(expect) && MemCompare(BufData(&b), expect, sizeof(expect)) == 0;
    BufDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Scientific form WITHOUT an explicit precision must use the default 6-digit
// mantissa: 12345.67 -> "1.234567e+04". The has_precision argument is
//   (fmt_info->flags & FMT_FLAG_HAS_PRECISION) != 0
// Kills 2136:34 and_to_or (& -> |): the flag mask becomes non-zero, forcing
// has_precision=true with precision 0 -> "1e+04". Kills 2136:60 ne_to_eq
// (!= 0 -> == 0): with no precision flag set, the test flips to true, again
// forcing precision 0.
static bool test_m31_float_sci_default_precision(void) {
    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);
    Str              out        = StrInit(&alloc);
    Float            v          = FloatFromStr("12345.67", alloc_base);

    StrAppendFmt(&out, "{e}", v);
    bool ok = ZstrCompare(StrBegin(&out), "1.234567e+04") == 0;

    FloatDeinit(&v);
    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Scientific form WITH an explicit precision of 2: 12345.67 -> "1.23e+04".
// Reinforces 2136:60 ne_to_eq: under (!= 0 -> == 0) the has-precision flag is
// set so the test flips false, dropping back to the 6-digit default
// "1.234567e+04", which differs from the expected "1.23e+04".
static bool test_m31_float_sci_explicit_precision(void) {
    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);
    Str              out        = StrInit(&alloc);
    Float            v          = FloatFromStr("12345.67", alloc_base);

    StrAppendFmt(&out, "{.2e}", v);
    bool ok = ZstrCompare(StrBegin(&out), "1.23e+04") == 0;

    FloatDeinit(&v);
    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Width padding measures ONLY the bytes just written, computed as
//   content_len = StrLen(o) - start_len
// where start_len was captured before the float was appended. With a
// pre-existing prefix in the Str, kills 2161:38 sub_to_add (- -> +): the
// mutant computes content_len = float_len + 2*prefix_len, which exceeds the
// width, so StrPad emits no padding. Real code pads to the field width.
//
// prefix "XX" (start_len=2), float "1.2" (3 chars), width 5, right align:
//   real  content_len=3 -> pad 2 -> "  XX1.2" (front-padded whole Str)
//   mutant content_len=7 -> 7>=5  -> no pad -> "XX1.2"
static bool test_m31_float_width_uses_just_written_len(void) {
    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);
    Str              out        = StrInit(&alloc);
    Float            v          = FloatFromStr("1.2", alloc_base);

    StrAppendFmt(&out, "XX");
    StrAppendFmt(&out, "{>5}", v);
    bool ok = ZstrCompare(StrBegin(&out), "  XX1.2") == 0 && StrLen(&out) == 7;

    FloatDeinit(&v);
    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Io.c:1028:28 cxx_ne_to_eq -- `fmt_info.width != 1 && ...`.
// A valid 1-byte raw directive `{<1r}` has width 1; the real `!=`
// chain evaluates false (no fatal) so the byte lands. The `==`
// mutant makes `width == 1` true and LOG_FATALs on the otherwise
// valid width-1 spec. Real code must succeed and write exactly 0xAB.
static bool test_m32_raw_width1_byte_writes(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Buf              b     = BufInit(&alloc);
    bool             ok    = BufWriteFmt(&b, "{<1r}", (u8)0xAB);
    ok                     = ok && (BufLength(&b) == 1) && (BufData(&b)[0] == 0xAB);
    BufDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Io.c:2976:38 cxx_sub_to_add -- `content_len = StrLen(o) - start_len`.
// start_len snapshots the buffer length before this field renders, so
// content_len must measure only the just-rendered field. With a "XX"
// prefix already in the buffer, rendering a 1-bit BitVec under `{>5}`
// makes the just-this-field length 1, so StrPad inserts 4 spaces
// (front-pad over the whole Str) -> "    XX1" (length 7). The `+`
// mutant computes content_len = StrLen + start_len = 5 >= width 5, so
// StrPad no-ops -> "XX1" (length 3). The padded output is
// caller-observable.
static bool test_m32_bitvec_width_uses_field_length(void) {
    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    Str    out = StrInit(&alloc);
    BitVec bv  = BitVecFromStr("1", alloc_base);

    StrAppendFmt(&out, "XX{>5}", bv);
    bool ok = (StrLen(&out) == 7) && (ZstrCompare(StrBegin(&out), "    XX1") == 0);

    BitVecDeinit(&bv);
    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Io.c:1025:30 cxx_and_to_or -- `!(fmt_info.flags & FMT_FLAG_RAW)`.
// A non-raw spec whose width happens to be a legal raw width (`{4}`,
// width 4, no FMT_FLAG_RAW) must abort: the real `&` yields 0 ->
// `!(0)` true -> LOG_FATAL("only raw allowed"). The `|` mutant keeps
// the bit set, skips the guard, and proceeds to render -- so the real
// code aborts here while the mutant does not. Deadend.
static bool test_m32_deadend_nonraw_legal_width(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Buf              b     = BufInit(&alloc);
    BufWriteFmt(&b, "{4}", (u32)0x11223344); // real: LOG_FATAL before return
    BufDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return true;
}

// Line 500 (cxx_init_const): `bool ok = str_append_fmt(...)` forced to a
// truthy literal makes a FAILED render (malformed fmt) look successful, so
// StrPatchFmt would proceed/return true instead of false. Real code returns
// false on a malformed fmt and leaves the target untouched.
static bool test_m33_patch_fmt_malformed_returns_false(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              o     = StrInitFromZstr("-----", &alloc);

    // "AB{" pushes "AB" into the scratch then hits an unclosed spec ->
    // str_append_fmt returns false.
    bool r = StrPatchFmt(&o, 0, "AB{");

    bool ok = (r == false) && (ZstrCompare(StrBegin(&o), "-----") == 0);

    StrDeinit(&o);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Line 502 col 20 (cxx_gt_to_ge): `offset > StrLen(o)` -> `>=`. Patching an
// empty render exactly at the end (offset == length) is a legal no-op; the
// `>=` mutant rejects it.
static bool test_m33_patch_fmt_empty_at_end(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              o     = StrInitFromZstr("hello", &alloc);

    bool r = StrPatchFmt(&o, StrLen(&o), ""); // offset == length, empty render

    bool ok = (r == true) && (ZstrCompare(StrBegin(&o), "hello") == 0);

    StrDeinit(&o);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Line 502 col 48 (cxx_gt_to_ge): `StrLen(&tmp) > StrLen(o) - offset` -> `>=`.
// A render that exactly fills the remaining space is legal; the `>=` mutant
// rejects the exact-fit case and refuses to copy.
static bool test_m33_patch_fmt_exact_fit(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              o     = StrInitFromZstr("-----", &alloc); // length 5

    bool r = StrPatchFmt(&o, 0, "12345");                      // renders exactly 5 chars

    bool ok = (r == true) && (ZstrCompare(StrBegin(&o), "12345") == 0);

    StrDeinit(&o);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Line 1052 (cxx_replace_scalar_call): `return render_binary_fmt(...)`
// replaced by 42 drops the render entirely, so no bytes are emitted. Assert
// the bytes actually landed.
static bool test_m33_buf_append_fmt_emits_bytes(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Buf              b     = BufInit(&alloc);

    bool r = BufAppendFmt(&b, "{>4r}", (u32)0xAABBCCDD);

    const u8 *d  = BufData(&b);
    bool      ok = r && (BufLength(&b) == 4) && d[0] == 0xAA && d[1] == 0xBB && d[2] == 0xCC && d[3] == 0xDD;

    BufDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Line 1259 (cxx_lt_to_le): `digit < radix` -> `<=`. Reading an octal Int from
// "78" must stop at '8' (digit 8 is not a valid base-8 digit) and parse "7".
// The `<=` mutant would also collect '8', then fail to parse "78" as octal,
// leaving the Int at its default 0.
static bool test_m33_int_octal_stops_at_radix_digit(void) {
    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    Int oct = IntInit(alloc_base);

    Zstr z = "78";
    StrReadFmt(z, "{o}", oct);

    Str  oct_text = IntToOctStr(&oct);
    bool ok       = (ZstrCompare(StrBegin(&oct_text), "7") == 0);

    StrDeinit(&oct_text);
    IntDeinit(&oct);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Line 1809 (cxx_replace_scalar_call): `return _write_Zstr(...)` replaced by 42
// drops the inner write, so nothing is emitted. Assert the string content
// actually rendered.
static bool test_m33_write_zstralloc_emits(void) {
    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);
    Str              output     = StrInit(&alloc);

    Zstr name = "hello";
    StrAppendFmt(&output, "{s}", ZstrIO(name, alloc_base));

    bool ok = (ZstrCompare(StrBegin(&output), "hello") == 0);

    StrDeinit(&output);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Line 2006 (cxx_replace_scalar_call): `return write_int_as_chars(o, ..., *v,
// 1)` for `{c}` on i8 replaced by 42 drops the write. Assert the char emits.
static bool test_m33_i8_char_emits(void) {
    DefaultAllocator alloc  = DefaultAllocatorInit();
    Str              output = StrInit(&alloc);

    i8 v = (i8)'Q';
    StrAppendFmt(&output, "{c}", v);

    bool ok = (StrLen(&output) == 1) && (StrBegin(&output)[0] == 'Q');

    StrDeinit(&output);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Line 2106 (cxx_replace_scalar_call): `return write_int_as_chars(o, ..., bits,
// 4)` for `{c}` on f32 replaced by 42 drops the write. The 4 raw bytes of the
// f32 bit pattern must be emitted (big-endian iteration order).
static bool test_m33_f32_char_emits_four_bytes(void) {
    DefaultAllocator alloc  = DefaultAllocatorInit();
    Str              output = StrInit(&alloc);

    // Choose a bit pattern whose 4 bytes are all printable: 0x42434445 -> "BCDE".
    f32 v;
    u32 bits = 0x42434445u;
    MemCopy(&v, &bits, sizeof(v));

    StrAppendFmt(&output, "{c}", v);

    bool ok = (StrLen(&output) == 4) && (ZstrCompare(StrBegin(&output), "BCDE") == 0);

    StrDeinit(&output);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Line 3472 (cxx_eq_to_ne): `fmt_info->endian == ENDIAN_NATIVE` -> `!=`. With an
// explicit big-endian request `{>2r}`, real code keeps BIG. The `!=` mutant
// treats BIG as "needs native resolution" and overwrites it with the host
// order (little-endian here), reversing the bytes.
static bool test_m33_write_r16_explicit_big(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Buf              b     = BufInit(&alloc);

    bool      ok = BufWriteFmt(&b, "{>2r}", (u16)0x1234);
    const u8 *d  = BufData(&b);
    ok           = ok && (BufLength(&b) == 2) && d[0] == 0x12 && d[1] == 0x34;

    BufDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

static bool test_m33_write_r16_native(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Buf              b     = BufInit(&alloc);

    bool      ok = BufWriteFmt(&b, "{^2r}", (u16)0x1234);
    const u8 *d  = BufData(&b);
    // Host is little-endian: low byte first.
    ok = ok && (BufLength(&b) == 2) && d[0] == 0x34 && d[1] == 0x12;

    BufDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

static bool test_m33_write_r32_native(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Buf              b     = BufInit(&alloc);

    bool      ok = BufWriteFmt(&b, "{^4r}", (u32)0x11223344);
    const u8 *d  = BufData(&b);
    ok           = ok && (BufLength(&b) == 4) && d[0] == 0x44 && d[1] == 0x33 && d[2] == 0x22 && d[3] == 0x11;

    BufDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

static bool test_m33_write_r64_native(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Buf              b     = BufInit(&alloc);

    bool      ok = BufWriteFmt(&b, "{^8r}", (u64)0x0102030405060708ULL);
    const u8 *d  = BufData(&b);
    ok = ok && (BufLength(&b) == 8) && d[0] == 0x08 && d[1] == 0x07 && d[2] == 0x06 && d[3] == 0x05 && d[4] == 0x04 &&
         d[5] == 0x03 && d[6] == 0x02 && d[7] == 0x01;

    BufDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// 1230:28 write_char_internal -- hex/base digit char selection
//   `low < 10 ? '0'+low : <letter>` mutated to `low >= 10`.
// Format a non-printable byte whose two nibbles straddle the boundary:
//   0x1A -> hi nibble 1 (<10, digit '1'), low nibble A (>=10, letter 'a').
// Real => "\x1a". Mutant inverts the low-nibble branch => the low digit
// is mis-rendered, so the exact string differs.
// ---------------------------------------------------------------------------
bool test_if_1230_hex_low_nibble_digit_selection(void);
bool test_if_1230_hex_low_nibble_digit_selection(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              out   = StrInit(&alloc);
    bool             ok    = true;

    // The `{c}` escape path for non-printable bytes runs through
    // write_char_internal only for Str / Zstr arguments (a bare u8 takes the
    // write_int_as_chars path instead), so render a Str holding the byte 0x1A.
    // 0x1A: hi nibble 0x01 (<10 -> '1'), low nibble 0x0A (>=10 -> 'a') => "\x1a".
    Str src = StrInit(&alloc);
    StrPushBackR(&src, 0x1A);
    StrAppendFmt(&out, "{c}", src);
    ok = ok && (ZstrCompare(StrBegin(&out), "\\x1a") == 0);
    StrClear(&out);
    StrClear(&src);

    // 0xA1: low nibble 0x01 (<10 -> '1') -- pins the low digit on the <10 side
    // of the boundary too => "\xa1".
    StrPushBackR(&src, 0xA1);
    StrAppendFmt(&out, "{c}", src);
    ok = ok && (ZstrCompare(StrBegin(&out), "\\xa1") == 0);

    StrDeinit(&src);
    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// 1397:75 float_try_to_decimal_str -- trailing-zero pad loop counter.
//   `for(i=MIN2(frac,precision); i<precision; i++)` mutated to `i--`.
// A Float with fewer fractional digits than the requested precision drives
// the loop (real pads trailing zeros). With `i--` the counter underflows
// and the loop never terminates -> hang/crash under the mutant; real yields
// the exact padded string.
// ---------------------------------------------------------------------------
bool test_if_1397_float_trailing_zero_padding(void);
bool test_if_1397_float_trailing_zero_padding(void) {
    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);
    Str              out        = StrInit(&alloc);
    bool             ok         = true;

    // "1.2" has 1 fractional digit; {.5} requests 5 -> 4 zeros are padded.
    Float f = FloatFromStr("1.2", alloc_base);
    StrAppendFmt(&out, "{.5}", f);
    ok = ok && (ZstrCompare(StrBegin(&out), "1.20000") == 0);
    StrClear(&out);

    Float g = FloatFromStr("3.14", alloc_base);
    StrAppendFmt(&out, "{.6}", g);
    ok = ok && (ZstrCompare(StrBegin(&out), "3.140000") == 0);

    FloatDeinit(&f);
    FloatDeinit(&g);
    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// 238:21 pad_numeric_zeros -- zero-pad boundary `content_len >= width` -> `>`.
// A numeric value whose rendered width EXACTLY equals the field width must
// receive no extra zero padding. At the boundary `>=` and `>` diverge: the
// mutant ( `>` ) treats content_len==width as "needs pad" and inserts an
// extra '0'. Use {0Nd}-style zero-fill: value 255, width 3 -> "255" (no pad).
// ---------------------------------------------------------------------------
bool test_if_238_zero_pad_exact_width_boundary(void);
bool test_if_238_zero_pad_exact_width_boundary(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              out   = StrInit(&alloc);
    bool             ok    = true;

    // 255 is 3 digits; zero-pad to width 3 -> exactly fits, no leading zeros.
    u32 v = 255;
    StrAppendFmt(&out, "{03}", v);
    ok = ok && (ZstrCompare(StrBegin(&out), "255") == 0);
    StrClear(&out);

    // Sanity: width one wider really does pad (guards against a no-op pad).
    StrAppendFmt(&out, "{04}", v);
    ok = ok && (ZstrCompare(StrBegin(&out), "0255") == 0);

    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// 257:21 StrPad -- space-pad boundary `content_len >= width` -> `>`.
// Same boundary, space padding. A value whose width exactly equals the field
// width must get no padding; the mutant ( `>` ) adds one space at the boundary.
// ---------------------------------------------------------------------------
bool test_if_257_space_pad_exact_width_boundary(void);
bool test_if_257_space_pad_exact_width_boundary(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              out   = StrInit(&alloc);
    bool             ok    = true;

    // "abc" is 3 chars; field width 3 -> no padding at all.
    Zstr s = "abc";
    StrAppendFmt(&out, "{3}", s);
    ok = ok && (ZstrCompare(StrBegin(&out), "abc") == 0);
    StrClear(&out);

    // Left/center at the exact boundary likewise add nothing.
    StrAppendFmt(&out, "{<3}", s);
    ok = ok && (ZstrCompare(StrBegin(&out), "abc") == 0);
    StrClear(&out);

    StrAppendFmt(&out, "{^3}", s);
    ok = ok && (ZstrCompare(StrBegin(&out), "abc") == 0);
    StrClear(&out);

    // Sanity: width 4 does pad (one space, right-aligned default).
    StrAppendFmt(&out, "{4}", s);
    ok = ok && (ZstrCompare(StrBegin(&out), " abc") == 0);

    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// 909:92 buf_read_fmt -- reader-fn dispatch identity `read_fn == _read_u64`
//   mutated to `!=`. Reading a u64 from a Buf lands on this 8-byte branch.
// Under the mutant the `_read_u64` arm of the width-8 OR-chain inverts, so a
// u64 read no longer resolves var_width=8 and the function LOG_FATALs
// (process abort) instead of decoding. Real round-trips the value exactly.
// ---------------------------------------------------------------------------
bool test_if_909_buf_read_u64_dispatch(void);
bool test_if_909_buf_read_u64_dispatch(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Buf              b     = BufInit(&alloc);
    bool             ok    = true;

    ok = ok && BufWriteFmt(&b, "{>8r}", (u64)0x0102030405060708ULL);

    BufIter it  = BufIterFromBuf(&b);
    u64     v64 = 0;
    ok          = ok && BufReadFmt(&it, "{>8r}", v64);
    ok          = ok && (v64 == 0x0102030405060708ULL);

    BufDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// 514:5 str_patch_fmt -- `StrDeinit(&tmp)` call removed.
// Pure scratch-buffer cleanup; dropping it leaks the temp Str but does not
// change any observable result. Documented as IGNORE (leak-only) in the
// report; this test simply pins that StrPatchFmt still behaves correctly so
// the surrounding logic stays guarded.
// ---------------------------------------------------------------------------
bool test_if_514_patch_fmt_behaviour(void);
bool test_if_514_patch_fmt_behaviour(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              s     = StrInit(&alloc);
    bool             ok    = true;

    StrAppendFmt(&s, "AAAAAAAA");
    size before = StrLen(&s);
    ok          = ok && StrPatchFmt(&s, 2, "{}", LVAL(1234));
    ok          = ok && (StrLen(&s) == before);
    ok          = ok && StrBegin(&s)[2] == '1' && StrBegin(&s)[5] == '4';

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// 1703:25 _write_Str -- padding gate `if (fmt_info->width > 0)` -> `>= 0`.
// width is unsigned; `>= 0` is always true, so the mutant additionally enters
// the block when width==0. Inside, StrPad(o, 0, ...) is a no-op (its own
// content_len >= 0 guard returns immediately), so the mutant produces
// identical output. Documented as IGNORE (equivalent). This test pins the
// width==0 / no-pad behaviour for the Str path.
// ---------------------------------------------------------------------------
bool test_if_1703_str_zero_width_no_pad(void);
bool test_if_1703_str_zero_width_no_pad(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              out   = StrInit(&alloc);
    bool             ok    = true;

    Str s = StrInitFromZstr("hello", &alloc);
    StrAppendFmt(&out, "{}", s); // width defaults to 0 -> no padding
    ok = ok && (ZstrCompare(StrBegin(&out), "hello") == 0);

    StrDeinit(&s);
    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// 2067:52 _write_f64 -- `flags & FMT_FLAG_HAS_PRECISION ? precision : 6`
//   mutated to `flags | ...`. The default fmt_info.precision is itself 6, so
// when the precision flag is unset the fallback constant (6) equals the struct
// field value; `&` and `|` pick the same number. Documented as IGNORE
// (equivalent). This test pins both the default-precision and explicit-
// precision f64 outputs.
// ---------------------------------------------------------------------------
bool test_if_2067_f64_default_and_explicit_precision(void);
bool test_if_2067_f64_default_and_explicit_precision(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              out   = StrInit(&alloc);
    bool             ok    = true;

    f64 v = 3.14;
    StrAppendFmt(&out, "{}", v); // default precision 6
    ok = ok && (ZstrCompare(StrBegin(&out), "3.140000") == 0);
    StrClear(&out);

    StrAppendFmt(&out, "{.2}", v); // explicit precision 2
    ok = ok && (ZstrCompare(StrBegin(&out), "3.14") == 0);

    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// 1109: branch condition `probe < 0 || fd==0 || fd==1 || fd==2`.
// A temp file is seekable (probe==0, fd>2) so the real code takes the
// SEEKABLE branch, which advances the position to exactly the consumed
// byte count. Any swap that flips this condition true (lt_to_ge/lt_to_le
// on `probe<0`, eq_to_ne on the fd checks) diverts to the non-seekable
// branch, which slurps to EOF and leaves the position AT EOF. We assert
// both the parsed value AND that FileTell == consumed (2), not EOF (10).
// ---------------------------------------------------------------------------
static bool test_m4_seekable_position_advance(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              path  = StrInit(&alloc);
    File             f     = m4_make_temp(&alloc, &path, "42 zzzzzzz", 10);
    bool             ok    = FileIsOpen(&f);

    i32 v = 0;
    FReadFmt(&f, "{}", v);
    ok = ok && (v == 42);
    // Seekable branch advances to exactly the consumed bytes ("42").
    ok = ok && (FileTell(&f) == 2);

    m4_cleanup(&f, &path);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// 1108 (probe init / FileSeek(CUR) call) and 1120 (cur_pos init).
// `cur_pos` is taken from the entry-position probe. If probe/cur_pos is
// forced to a constant 42 while the real start is 0, then
// file_len = end_pos - cur_pos goes negative on this small file, the
// read+parse is skipped entirely, and the destination stays at its init
// value. Asserting the value WAS parsed kills the constant injections.
// ---------------------------------------------------------------------------
static bool test_m4_cur_pos_from_probe(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              path  = StrInit(&alloc);
    File             f     = m4_make_temp(&alloc, &path, "7", 1);
    bool             ok    = FileIsOpen(&f);

    i32 v = -1;
    FReadFmt(&f, "{}", v);
    ok = ok && (v == 7);

    m4_cleanup(&f, &path);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// 1150 (consumed init / new_pos - StrBegin) and 1151:add_to_sub.
// After a successful parse the position is advanced to
// `cur_pos + consumed`. Starting from cur_pos==0 with a 2-digit number
// followed by trailing bytes, the real position lands at exactly 2.
//   * 1150 cxx_init_const (consumed=42)      -> seek to 42.
//   * 1150 cxx_sub_to_add (ptr+ptr)          -> seek to a garbage offset.
//   * 1151 cxx_add_to_sub (cur_pos-consumed) -> seek to -2.
// All three diverge from the expected FileTell()==2.
// ---------------------------------------------------------------------------
static bool test_m4_consumed_offset_exact(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              path  = StrInit(&alloc);
    File             f     = m4_make_temp(&alloc, &path, "53 and more text", 16);
    bool             ok    = FileIsOpen(&f);

    i32 v = 0;
    FReadFmt(&f, "{}", v);
    ok = ok && (v == 53);
    ok = ok && (FileTell(&f) == 2);

    m4_cleanup(&f, &path);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// 3342:14 cxx_init_const -- `u32 temp = 0` in the {c} char path.
// A leading-space input makes read_chars_internal consume ZERO bytes, so
// the init value survives into `*v = (f32)temp`. Real init 0 -> 0.0;
// the mutant's init 42 -> 42.0.
static bool test_m5_char_flag_empty_keeps_zero_init(void) {
    f32  v = -1.0f;
    Zstr z = " ";
    StrReadFmt(z, "{c}", v);
    return f32_close(v, 0.0f);
}

// 3367:12/24/36/48/62/76/89 cxx_eq_to_ne -- the inf/nan special-token
// detector. Each distinct literal makes exactly one equality term the sole
// reason the special branch is entered; flipping it to != drops into the
// digit path which rejects the token and leaves `*v` untouched.
static bool test_m5_special_lower_inf(void) { // c == 'i'
    f32  v = 0.0f;
    Zstr z = "inf";
    StrReadFmt(z, "{}", v);
    return v > 1e30f;
}

static bool test_m5_special_upper_inf(void) { // c == 'I'
    f32  v = 0.0f;
    Zstr z = "INF";
    StrReadFmt(z, "{}", v);
    return v > 1e30f;
}

static bool test_m5_special_lower_nan(void) { // c == 'n'
    f32  v = 0.0f;
    Zstr z = "nan";
    StrReadFmt(z, "{}", v);
    return v != v;                            // NaN is the only value not equal to itself
}

static bool test_m5_special_upper_nan(void) { // c == 'N'
    f32  v = 0.0f;
    Zstr z = "NAN";
    StrReadFmt(z, "{}", v);
    return v != v;
}

static bool test_m5_special_neg_inf_dash(void) { // c == '-'
    f32  v = 0.0f;
    Zstr z = "-inf";
    StrReadFmt(z, "{}", v);
    return v < -1e30f;
}

static bool test_m5_special_neg_inf_c1_lower(void) { // c1 == 'i'
    f32  v = 0.0f;
    Zstr z = "-inf";
    StrReadFmt(z, "{}", v);
    return v < -1e30f;
}

static bool test_m5_special_neg_inf_c1_upper(void) { // c1 == 'I'
    f32  v = 0.0f;
    Zstr z = "-INF";
    StrReadFmt(z, "{}", v);
    return v < -1e30f;
}

// 3381:16 cxx_assign_const + 3380 path -- `*v = (f32)val` for inf must
// yield +inf, not the literal 42.0 the mutant would assign.
static bool test_m5_special_value_is_not_42(void) {
    f32  v = 0.0f;
    Zstr z = "inf";
    StrReadFmt(z, "{}", v);
    return v > 1e30f; // 42.0 would fail this
}

// 3404:16 cxx_eq_to_ne -- exponent introducer `c == 'e'`. With the
// exponent branch disabled the trailing "+3" is rejected by the digit
// scanner (sign legal only at index 0), so the token "1e" fails and `*v`
// stays untouched. Real code parses 1e+3 == 1000.
static bool test_m5_exponent_lower_with_sign(void) {
    f32  v = 0.0f;
    Zstr z = "1e+3";
    StrReadFmt(z, "{}", v);
    return f32_close(v, 1000.0f);
}

// 3404:28 cxx_eq_to_ne -- same for the uppercase 'E'.
static bool test_m5_exponent_upper_with_sign(void) {
    f32  v = 0.0f;
    Zstr z = "1E+3";
    StrReadFmt(z, "{}", v);
    return f32_close(v, 1000.0f);
}

// 3404:57 cxx_gt_to_le -- `StrIterIndex(&si) > StrIterIndex(&saved)`.
// Mutating > to <= makes the guard false on a real (non-leading) 'e', so
// the exponent branch is skipped and "1e+3" again loses its "+3" tail.
static bool test_m5_exponent_guard_not_le(void) {
    f32  v = 0.0f;
    Zstr z = "2e-2";
    StrReadFmt(z, "{}", v);
    return f32_close(v, 0.02f);
}

// 3407:44 cxx_eq_to_ne -- exponent sign `c == '+'`. Dropping '+' handling
// leaves '+' unconsumed; the digit-presence check then fails the token.
static bool test_m5_exponent_plus_sign(void) {
    f32  v = 0.0f;
    Zstr z = "1e+3";
    StrReadFmt(z, "{}", v);
    return f32_close(v, 1000.0f);
}

// 3407:56 cxx_eq_to_ne -- exponent sign `c == '-'`.
static bool test_m5_exponent_minus_sign(void) {
    f32  v = 0.0f;
    Zstr z = "1e-3";
    StrReadFmt(z, "{}", v);
    return f32_close(v, 0.001f);
}

// 3422:56 cxx_eq_to_ne -- the is_first_char argument
// `StrIterIndex(&si) == StrIterIndex(&saved)`. Mutating to != reports the
// leading '+' as a non-first char, which is_valid_number_char rejects, so
// the token starts empty and parsing fails. Real code accepts the sign and
// parses "+1.5" == 1.5.
static bool test_m5_leading_sign_is_first_char(void) {
    f32  v = 0.0f;
    Zstr z = "+1.5";
    StrReadFmt(z, "{}", v);
    return f32_close(v, 1.5f);
}

// 3430:36 cxx_sub_to_add -- `pos = index(si) - index(saved)`. With a
// leading space, saved is non-zero, so the subtraction yields the true
// token length (3 for "3.5"). The mutant's `index + index` over-copies
// past the token, embedding stray bytes that fail validation and leave
// `*v` untouched. Real code parses 3.5.
static bool test_m5_token_length_uses_subtraction(void) {
    f32  v = 0.0f;
    Zstr z = "  3.5";
    StrReadFmt(z, "{}", v);
    return f32_close(v, 3.5f);
}

// Plain decimal read parses digits and stops at the first non-digit. Kills
// the int_fmt_digit_matches_radix() -> 42 scalar-call replacement (3238):
// real stops at 'z' and parses "9"; the mutant treats every char as a
// matching digit, swallows "9z", then IntTryFromStrRadix("9z",10) fails and
// the value is left at the sentinel.
static bool test_m6_decimal_stops_at_nondigit(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    Int  v = IntInit(ALLOCATOR_OF(&alloc));
    Zstr z = "9z";
    StrReadFmt(z, "{}", v);
    bool ok = (IntCompare(&v, 9) == 0);

    IntDeinit(&v);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A leading '+' is consumed; a non-'+' first char is NOT. Kills the
// d0 == '+' -> != swap (3215): on input "5" the real code leaves the digit
// in place and parses 5, while the mutant treats '5' as a sign, advances
// past it, finds no digits, and rejects -> sentinel survives.
static bool test_m6_no_plus_sign_keeps_first_digit(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    Int  v = IntInit(ALLOCATOR_OF(&alloc));
    Zstr z = "5";
    StrReadFmt(z, "{}", v);
    bool ok = (IntCompare(&v, 5) == 0);

    IntDeinit(&v);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A leading '+' IS accepted (digits_saved advances past it) so "+8" -> 8.
// Guards the '+'-consume branch (line 3216 reachability) and complements the
// no-sign test above.
static bool test_m6_plus_sign_accepted(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    Int  v = IntInit(ALLOCATOR_OF(&alloc));
    Zstr z = "+8";
    StrReadFmt(z, "{}", v);
    bool ok = (IntCompare(&v, 8) == 0);

    IntDeinit(&v);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A trailing '_' digit separator is rejected. Kills the trailing == '_'
// -> != swap (3248): real seeds the loop, sees '_' after "9", and rejects
// (sentinel 123 survives); the mutant accepts and overwrites with 9.
static bool test_m6_trailing_underscore_rejected(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    Int  v = IntFromStr("123", &alloc);
    Zstr z = "9_";
    StrReadFmt(z, "{}", v);
    bool ok = (IntCompare(&v, 123) == 0);

    IntDeinit(&v);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Hex reads must NOT accept a "0x" prefix (digits only). With sentinel 200,
// "0xFF" is rejected so 200 survives. Kills the 3225 chain for the lower-x
// arm: radix==16 / p0=='0' / p1=='x' each -> != turns the error branch off,
// letting the mutant parse "0" and overwrite the sentinel with 0.
static bool test_m6_hex_rejects_0x_lower(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    Int  v = IntFromStr("200", &alloc);
    Zstr z = "0xFF";
    StrReadFmt(z, "{x}", v);
    bool ok = (IntCompare(&v, 200) == 0);

    IntDeinit(&v);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Same prefix rejection for the upper-case "0X" form -- this is the only
// input that reaches the p1 == 'X' arm (the 'x' arm short-circuits || on
// lower-case input), so it kills the 3225 col-54 p1 == 'X' -> != swap.
static bool test_m6_hex_rejects_0x_upper(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    Int  v = IntFromStr("200", &alloc);
    Zstr z = "0XFF";
    StrReadFmt(z, "{x}", v);
    bool ok = (IntCompare(&v, 200) == 0);

    IntDeinit(&v);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Plain hex digits ARE accepted: "ff" -> 255. Confirms the prefix guard
// does not reject legitimate hex and complements the 0x-rejection tests.
static bool test_m6_hex_plain_accepted(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    Int  v = IntInit(ALLOCATOR_OF(&alloc));
    Zstr z = "ff";
    StrReadFmt(z, "{x}", v);
    bool ok = (IntCompare(&v, 255) == 0);

    IntDeinit(&v);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Binary reads must NOT accept a "0b" prefix. Sentinel 5 survives "0b1".
// Kills the 3229 chain (radix==2 / p0=='0' / p1=='b') for the lower form.
static bool test_m6_binary_rejects_0b_lower(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    Int  v = IntFromStr("5", &alloc);
    Zstr z = "0b1";
    StrReadFmt(z, "{b}", v);
    bool ok = (IntCompare(&v, 5) == 0);

    IntDeinit(&v);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Upper-case "0B" form reaches the p1 == 'B' arm; kills 3229 col-53.
static bool test_m6_binary_rejects_0b_upper(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    Int  v = IntFromStr("5", &alloc);
    Zstr z = "0B1";
    StrReadFmt(z, "{b}", v);
    bool ok = (IntCompare(&v, 5) == 0);

    IntDeinit(&v);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Plain binary digits ARE accepted: "101" -> 5.
static bool test_m6_binary_plain_accepted(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    Int  v = IntInit(ALLOCATOR_OF(&alloc));
    Zstr z = "101";
    StrReadFmt(z, "{b}", v);
    bool ok = (IntCompare(&v, 5) == 0);

    IntDeinit(&v);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Octal reads must NOT accept a "0o" prefix. Sentinel 9 survives "0o7".
// Kills the 3233 chain (radix==8 / p0=='0' / p1=='o') for the lower form.
static bool test_m6_octal_rejects_0o_lower(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    Int  v = IntFromStr("9", &alloc);
    Zstr z = "0o7";
    StrReadFmt(z, "{o}", v);
    bool ok = (IntCompare(&v, 9) == 0);

    IntDeinit(&v);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Upper-case "0O" form reaches the p1 == 'O' arm; kills 3233 col-53.
static bool test_m6_octal_rejects_0o_upper(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    Int  v = IntFromStr("9", &alloc);
    Zstr z = "0O7";
    StrReadFmt(z, "{o}", v);
    bool ok = (IntCompare(&v, 9) == 0);

    IntDeinit(&v);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Plain octal digits ARE accepted: "17" -> 15.
static bool test_m6_octal_plain_accepted(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    Int  v = IntInit(ALLOCATOR_OF(&alloc));
    Zstr z = "17";
    StrReadFmt(z, "{o}", v);
    bool ok = (IntCompare(&v, 15) == 0);

    IntDeinit(&v);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Leading whitespace is skipped before parsing: "   42" -> 42. Exercises the
// IS_SPACE skip loop (line 3199) feeding the `char c` reused at 3238.
static bool test_m6_leading_whitespace_skipped(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    Int  v = IntInit(ALLOCATOR_OF(&alloc));
    Zstr z = "   42";
    StrReadFmt(z, "{}", v);
    bool ok = (IntCompare(&v, 42) == 0);

    IntDeinit(&v);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// Line 132: `fi->align = ALIGN_CENTER;` for the `^` prefix. The align/endian
// union means `^` selects ENDIAN_NATIVE (== ALIGN_CENTER == 2) on the raw `r`
// path. Mutating the assigned constant to 42 makes endian != ENDIAN_NATIVE, so
// _write_r* never resolves native -> host and falls into the switch default ->
// LOG_FATAL (process abort). A native-endian raw round-trip succeeds on real
// code and aborts under the mutant.
// ---------------------------------------------------------------------------
static bool test_m8_caret_native_endian_raw_roundtrip(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Buf              b     = BufInit(&alloc);
    bool             ok    = true;

    ok = ok && BufWriteFmt(&b, "{^2r}", (u16)0x1234);
    ok = ok && (BufLength(&b) == 2);

    BufIter it    = BufIterFromBuf(&b);
    u16     v16   = 0;
    bool    rd_ok = BufReadFmt(&it, "{^2r}", v16);
    ok            = ok && rd_ok && (v16 == 0x1234);

    BufDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// Line 144 leading-'0' zero-pad gate:
//   if (pos + 1 < len && spec[pos] == '0' && spec[pos + 1] >= '0' && spec[pos + 1] <= '9')
// `{05}` on 7 -> real zero-pads to width 5 -> "00007".
// Kills: 144:13 add->sub (pos-1 underflow -> gate false),
//        144:17 lt->ge   (1>=2 false -> gate false),
//        144:60 ge->lt   (spec[1]<'0' false -> gate false),
//        144:79 add->sub (spec[-1]='{' <= '9' false -> gate false),
//        144:84 le->gt   (spec[1]>'9' false -> gate false),
//        145 |= -> &=     (flags &= ZERO_PAD -> 0, never sets ZERO_PAD),
//        146 ++ -> --     (pos underflows, width becomes 0).
// Every listed mutant drops the zero-pad flag (or the width), so the output is
// no longer "00007".
// ---------------------------------------------------------------------------
static bool test_m8_zero_pad_width5(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              out   = StrInit(&alloc);
    bool             ok    = true;

    StrAppendFmt(&out, "{05}", (u32)7);
    ok = ok && (ZstrCompare(StrBegin(&out), "00007") == 0);

    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// `{007}` on 3 -> real zero-pads to width 7 -> "0000003".
// Kills: 144:60 ge->gt (spec[1]=='0' > '0' false -> no zero pad -> "      3"),
//        150:32 ge->gt (width-loop guard spec[1]=='0' > '0' false -> width 0,
//                       zero-pad set but width 0 -> "3").
// A single exact-match on "0000003" rejects both mutant outputs.
// ---------------------------------------------------------------------------
static bool test_m8_zero_pad_width7_double_zero(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              out   = StrInit(&alloc);
    bool             ok    = true;

    StrAppendFmt(&out, "{007}", (u32)3);
    ok = ok && (ZstrCompare(StrBegin(&out), "0000003") == 0);

    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// `{09}` on 3 -> real zero-pads to width 9 -> "000000003".
// Kills: 144:84 le->lt (spec[1]=='9' < '9' false -> no zero pad -> "        3").
// ---------------------------------------------------------------------------
static bool test_m8_zero_pad_width9(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              out   = StrInit(&alloc);
    bool             ok    = true;

    StrAppendFmt(&out, "{09}", (u32)3);
    ok = ok && (ZstrCompare(StrBegin(&out), "000000003") == 0);

    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// Width parsing (no zero pad): `{9}` on 3 -> real width 9 -> 8 spaces + "3".
//   line 150: if (pos < len && spec[pos] >= '0' && spec[pos] <= '9')
//   line 151: while (pos < len && spec[pos] >= '0' && spec[pos] <= '9')
// Kills: 150:52 le->lt (outer guard spec[0]=='9' < '9' false -> width block
//                      skipped -> width 0 -> "3"),
//        151:59 le->lt (loop guard spec[0]=='9' < '9' false -> loop body never
//                      runs -> width stays 0 -> "3").
// ---------------------------------------------------------------------------
static bool test_m8_width9_space_pad(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              out   = StrInit(&alloc);
    bool             ok    = true;

    StrAppendFmt(&out, "{9}", (u32)3);
    ok = ok && (ZstrCompare(StrBegin(&out), "        3") == 0);
    ok = ok && (StrLen(&out) == 9);

    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// Precision on a string: `{.9s}` on "Hello" -> real precision 9, output
// MIN2(len=5, 9) = "Hello".
//   line 160: if (pos >= len || spec[pos] < '0' || spec[pos] > '9') return false;
//   line 165: while (pos < len && spec[pos] >= '0' && spec[pos] <= '9')
// Kills: 160:56 gt->ge (spec[pos]=='9' >= '9' true -> treats '.' digit as
//                      malformed -> StrAppendFmt returns false),
//        165:59 le->lt (precision-loop guard '9' < '9' false -> loop never runs
//                      -> precision 0 -> "render nothing" -> output "").
// Asserts both: the call succeeds AND the output is exactly "Hello".
// ---------------------------------------------------------------------------
static bool test_m8_precision9_string(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              out   = StrInit(&alloc);
    bool             ok    = true;

    Zstr s  = "Hello";
    bool rc = StrAppendFmt(&out, "{.9s}", s);
    ok      = ok && rc;
    ok      = ok && (ZstrCompare(StrBegin(&out), "Hello") == 0);

    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Deadend: a binary format with more raw fields than supplied args must abort
// (`arg_index >= argc` -> LOG_FATAL "too few arguments"). Two `{<1r}` fields,
// one arg: the second field trips the guard.
static bool test_deadend_bufread_too_few_args(void) {
    WriteFmt("bufread too-few-args must abort\n");
    DefaultAllocator alloc = DefaultAllocatorInit();
    Buf              b     = BufInit(&alloc);
    BufWriteFmt(&b, "{<1r}{<1r}", (u8)0x11, (u8)0x22); // 2 bytes available
    BufIter it = BufIterFromBuf(&b);
    u8      v8 = 0;
    BufReadFmt(&it, "{<1r}{<1r}", v8);                 // 2 fields, 1 arg -> abort
    BufDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return true;
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
        test_str_patch_fmt,
        test_buf_formatting,
        test_char_nonprintable_escape,
        test_buf_raw_endianness_and_roundtrip,
        test_buf_literal_magic_roundtrip,
        test_buf_literal_brace_escape,
        test_buf_literal_trailing_byte,
        test_datetime_iso_write,
        test_m1_arg_index_bound,
        test_m1_invalid_spec_leaves_dest,
        test_m1_char_spec_then_literal,
        test_m1_long_quoted_string_not_capped,
        test_m1_raw_u8,
        test_m1_raw_i8,
        test_m1_raw_u16_le,
        test_m1_raw_i16_be,
        test_m1_raw_u32_le,
        test_m1_raw_i32_be,
        test_m1_raw_u64_le,
        test_m1_raw_i64_be,
        test_m1_raw_f32,
        test_m1_raw_f64,
        test_m10_str_width_pad_after_prefix,
        test_m10_str_width_pad_left_align,
        test_m10_str_hex_uppercase,
        test_m10_str_hex_lowercase,
        test_m10_str_hex_multibyte_separator,
        test_m10_str_hex_zero_pad_nibble,
        test_m10_str_precision_truncate,
        test_m10_str_precision_zero,
        test_m11_hex_two_digits_not_truncated,
        test_m11_hex_value_is_decoded_not_42,
        test_m11_hex_ff,
        test_m11_hex_valid_not_rejected,
        test_m11_hex_zero_advances_pointer,
        test_m11_hex_result_assigned_from_value,
        test_m11_hex_invalid_returns_zero,
        test_m12_hex_min_width_is_four,
        test_m12_hex_width_tracks_value,
        test_m12_hex_cursor_advances,
        test_m12_octal_min_width_is_three,
        test_m12_octal_accepts_zero_digit,
        test_m12_octal_width_tracks_value,
        test_m13_zstr_hex_multibyte,
        test_m13_zstr_hex_single_byte,
        test_m13_zstr_hex_lowercase_letters,
        test_m13_zstr_hex_uppercase_letters,
        test_m13_zstr_hex_zero_pad_nibble,
        test_m13_zstr_hex_no_pad_two_digits,
        test_m13_zstr_precision_truncate,
        test_m13_zstr_precision_longer_noop,
        test_m13_zstr_precision_zero_empty,
        test_m13_zstr_precision_one,
        test_m13_zstr_pad_after_prefix,
        test_m13_zstr_left_pad_after_prefix,
        test_m13_zstr_empty_padded,
        test_m14_hex_prefix_uses_first_char_flag,
        test_m14_slice_length_is_difference,
        test_m14_prefix_guard_len_gate,
        test_m14_prefix_guard_letters,
        test_m15_digit_low_boundary_zero,
        test_m15_digit_high_boundary_nine,
        test_m15_digit_value_subtraction,
        test_m15_lower_low_boundary_a,
        test_m15_lower_high_boundary_f,
        test_m15_upper_low_boundary_A,
        test_m15_upper_high_boundary_F,
        test_m16_u8_roundtrip,
        test_m16_u32_roundtrip,
        test_m16_u64_roundtrip,
        test_m16_i64_roundtrip,
        test_m17_sci_single_digit_no_point,
        test_m17_sci_zero_precision_zero_no_point,
        test_m17_sci_zero_precision_three_zeros,
        test_m17_sci_frac_digit_vs_pad_boundary,
        test_m17_sci_value_and_uppercase,
        test_m18_bad_nibble_salvage,
        test_m18_force_lowercase,
        test_m18_force_uppercase,
        test_m18_buffer_size_stop,
        test_m19_neg_inf_lower,
        test_m19_neg_inf_upper,
        test_m19_dash_clause_not_minus,
        test_m19_inf_branch_strtof64_truthy,
        test_m19_token_length_leading_space,
        test_m19_numeric_string_guard,
        test_m2_bare_prefix_0x_rejected,
        test_m2_bare_prefix_0b_rejected,
        test_m2_bare_prefix_0o_rejected,
        test_m2_hex_prefix_upper_X_accepted,
        test_m2_bin_prefix_upper_B_accepted,
        test_m2_oct_prefix_upper_O_accepted,
        test_m2_plain_float_roundtrips,
        test_m2_lowercase_hex_prefix_accepted,
        test_m20_float_token_boundary,
        test_m20_float_token_boundary_no_trailer,
        test_m20_float_scientific_boundary,
        test_m20_float_value_exact,
        test_m23_clz_hex_3ff_bit_len_10,
        test_m23_clz_hex_ff_bit_len_8,
        test_m23_clz_hex_10000_bit_len_17,
        test_m24_zero_pad_positive_exact,
        test_m24_zero_pad_width_equals_content_no_pad,
        test_m24_zero_pad_negative_sign_kept_ahead,
        test_m24_zero_pad_positive_no_sign_shift,
        test_m24_zero_pad_negative_wide,
        test_m24_zero_pad_hex_width16_exact,
        test_m25_nodot_precision_pads_zeros,
        test_m25_nodot_precision_zero_no_dot,
        test_m25_dot_precision_pads_zeros,
        test_m25_dot_precision_zero_truncates_dot,
        test_m25_dot_precision_truncates_fraction,
        test_m26_default_precision_six,
        test_m26_scientific_lowercase_e,
        test_m26_scientific_uppercase_e,
        test_m26_width_pad_from_empty,
        test_m26_width_pad_with_prefix,
        test_m26_width_guard_pads_when_set,
        test_m27_exp_zero_yields_plus_zero,
        test_m27_exp_negative_magnitude,
        test_m27_exp_two_digits_no_extra_pad,
        test_m27_token_sign_after_decimal_stops,
        test_m27_token_sign_after_digit_stops,
        test_m27_token_trailing_letter_stops,
        test_m28_fwrite_roundtrip_string,
        test_m28_fwrite_roundtrip_int,
        test_m28_fwriteln_roundtrip,
        test_m28_i64_hex_lower,
        test_m28_i64_hex_upper,
        test_m28_i64_binary,
        test_m28_i64_octal,
        test_m28_i64_decimal_negative,
        test_m28_i64_width_after_prefix,
        test_m28_i64_zeropad_after_prefix,
        test_m29_hexbyte_printable_roundtrip,
        test_m29_hexbyte_high_nibble_zero,
        test_m29_hexbyte_low_nibble_zero,
        test_m29_hexbyte_letter_nibble,
        test_m29_write_int_char_single,
        test_m29_write_int_char_single_alt,
        test_m29_write_int_width_content_len,
        test_m3_unclosed_brace_fails,
        test_m3_raw_u8,
        test_m3_raw_u16,
        test_m3_raw_u32,
        test_m3_raw_u64,
        test_m3_raw_i64,
        test_m3_raw_f64,
        test_m30_u64_width_pad_with_prefix,
        test_m30_u64_width_pad_no_prefix,
        test_m30_u64_zero_pad_with_prefix,
        test_m30_patch_exact_fit_succeeds,
        test_m30_patch_empty_at_end_succeeds,
        test_m30_patch_overlong_fails,
        test_m31_raw_u8,
        test_m31_raw_i64,
        test_m31_raw_f64,
        test_m31_float_sci_default_precision,
        test_m31_float_sci_explicit_precision,
        test_m31_float_width_uses_just_written_len,
        test_m32_raw_width1_byte_writes,
        test_m32_bitvec_width_uses_field_length,
        test_m33_patch_fmt_malformed_returns_false,
        test_m33_patch_fmt_empty_at_end,
        test_m33_patch_fmt_exact_fit,
        test_m33_buf_append_fmt_emits_bytes,
        test_m33_int_octal_stops_at_radix_digit,
        test_m33_write_zstralloc_emits,
        test_m33_i8_char_emits,
        test_m33_f32_char_emits_four_bytes,
        test_m33_write_r16_explicit_big,
        test_m33_write_r16_native,
        test_m33_write_r32_native,
        test_m33_write_r64_native,
        test_if_1230_hex_low_nibble_digit_selection,
        test_if_1397_float_trailing_zero_padding,
        test_if_238_zero_pad_exact_width_boundary,
        test_if_257_space_pad_exact_width_boundary,
        test_if_909_buf_read_u64_dispatch,
        test_if_514_patch_fmt_behaviour,
        test_if_1703_str_zero_width_no_pad,
        test_if_2067_f64_default_and_explicit_precision,
        test_m4_seekable_position_advance,
        test_m4_cur_pos_from_probe,
        test_m4_consumed_offset_exact,
        test_m5_char_flag_empty_keeps_zero_init,
        test_m5_special_lower_inf,
        test_m5_special_upper_inf,
        test_m5_special_lower_nan,
        test_m5_special_upper_nan,
        test_m5_special_neg_inf_dash,
        test_m5_special_neg_inf_c1_lower,
        test_m5_special_neg_inf_c1_upper,
        test_m5_special_value_is_not_42,
        test_m5_exponent_lower_with_sign,
        test_m5_exponent_upper_with_sign,
        test_m5_exponent_guard_not_le,
        test_m5_exponent_plus_sign,
        test_m5_exponent_minus_sign,
        test_m5_leading_sign_is_first_char,
        test_m5_token_length_uses_subtraction,
        test_m6_decimal_stops_at_nondigit,
        test_m6_no_plus_sign_keeps_first_digit,
        test_m6_plus_sign_accepted,
        test_m6_trailing_underscore_rejected,
        test_m6_hex_rejects_0x_lower,
        test_m6_hex_rejects_0x_upper,
        test_m6_hex_plain_accepted,
        test_m6_binary_rejects_0b_lower,
        test_m6_binary_rejects_0b_upper,
        test_m6_binary_plain_accepted,
        test_m6_octal_rejects_0o_lower,
        test_m6_octal_rejects_0o_upper,
        test_m6_octal_plain_accepted,
        test_m6_leading_whitespace_skipped,
        test_m8_caret_native_endian_raw_roundtrip,
        test_m8_zero_pad_width5,
        test_m8_zero_pad_width7_double_zero,
        test_m8_zero_pad_width9,
        test_m8_width9_space_pad,
        test_m8_precision9_string,
    };

    int total_tests = sizeof(tests) / sizeof(tests[0]);

    TestFunction deadend_tests[] = {
        test_m16_nonraw_spec_deadend,
        test_m32_deadend_nonraw_legal_width,
        test_deadend_bufread_too_few_args,
    };

    return run_test_suite(
        tests,
        total_tests,
        deadend_tests,
        sizeof(deadend_tests) / sizeof(deadend_tests[0]),
        "Io.Write"
    );
}
