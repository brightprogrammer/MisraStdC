#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Allocator/Debug.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Container/BitVec.h>
#include <Misra/Std/Container/Int.h>
#include <Misra/Std/Container/Float.h>
#include <Misra/Std/Container/Buf.h>
#include <Misra/Std/Io.h>
#include <Misra/Std/Io/Private.h>
#include <Misra/Std/File.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Memory.h>
#include <Misra/Std/Math.h>
#include <Misra/Sys/Dir.h>
#include <Misra/Types.h>

#include "../../Util/TestRunner.h"

#define FLOAT_EPSILON  1e-6
#define DOUBLE_EPSILON 1e-12
#define F32_EPSILON    1e-4f

static bool float_equals(f32 a, f32 b) {
    return F64Abs(a - b) < FLOAT_EPSILON;
}

static bool double_equals(f64 a, f64 b) {
    return F64Abs(a - b) < DOUBLE_EPSILON;
}

static bool f32_close(f32 a, f32 b) {
    return F64Abs((f64)a - (f64)b) < (f64)F32_EPSILON;
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

bool test_integer_decimal_reading(void) {
    WriteFmt("Testing integer decimal reading\n");

    Zstr z = NULL;

    bool success = true;

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

    i32_val = 0;
    z       = "000042";
    StrReadFmt(z, "{}", i32_val);
    success = success && (i32_val == 42);

    i32_val = 0;
    z       = "-000042";
    StrReadFmt(z, "{}", i32_val);
    success = success && (i32_val == -42);

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

bool test_integer_hex_reading(void) {
    WriteFmt("Testing integer hexadecimal reading\n");

    Zstr z = NULL;

    bool success = true;

    u32 val = 0;
    z       = "0xdeadbeef";
    StrReadFmt(z, "{}", val);
    success = success && (val == 0xdeadbeef);

    val = 0;
    z   = "0xDEADBEEF";
    StrReadFmt(z, "{}", val);
    success = success && (val == 0xDEADBEEF);

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

bool test_integer_binary_reading(void) {
    WriteFmt("Testing integer binary reading\n");

    Zstr z = NULL;

    bool success = true;

    i8 val = 0;
    z      = "0b101010";
    StrReadFmt(z, "{}", val);
    success = success && (val == 42);

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

bool test_integer_octal_reading(void) {
    WriteFmt("Testing integer octal reading\n");

    Zstr z = NULL;

    bool success = true;

    i32 val = 0;
    z       = "0o755";
    StrReadFmt(z, "{}", val);
    success = success && (val == 0755);

    val = 0;
    z   = "755";
    StrReadFmt(z, "{}", val);
    success = success && (val == 755);

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

bool test_float_basic_reading(void) {
    WriteFmt("Testing basic float reading\n");

    Zstr z = NULL;

    bool success = true;

    f32 f32_val = 0.0f;
    z           = "3.14159";
    StrReadFmt(z, "{}", f32_val);
    success = success && float_equals(f32_val, 3.14159f);

    f64 f64_val = 0.0;
    z           = "3.14159265359";
    StrReadFmt(z, "{}", f64_val);
    success = success && double_equals(f64_val, 3.14159265359);

    f64_val = 1.0;
    z       = "0.0";
    StrReadFmt(z, "{}", f64_val);
    success = success && double_equals(f64_val, 0.0);

    f64_val = 1.0;
    z       = "-0.0";
    StrReadFmt(z, "{}", f64_val);
    // -0.0 compares == to +0.0; the epsilon check captures the magnitude
    // regardless of which sign bit the parser landed on.
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

bool test_float_scientific_reading(void) {
    WriteFmt("Testing scientific notation reading\n");

    Zstr z = NULL;

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

bool test_string_reading(void) {
    WriteFmt("Testing string reading\n");

    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    Zstr z = NULL;

    bool success = true;

    Str s = StrInit(&alloc);
    z     = "Hello";
    StrReadFmt(z, "{}", s);

    Str expected = StrInitFromZstr("Hello", &alloc);
    success      = success && (StrCmp(&s, &expected) == 0);
    StrDeinit(&expected);
    StrClear(&s);

    z = "\"Hello, World!\"";
    StrReadFmt(z, "{s}", s);

    expected = StrInitFromZstr("Hello, World!", &alloc);
    success  = success && (StrCmp(&s, &expected) == 0);
    StrDeinit(&expected);

    {
        Zstr zs = NULL;

        z = "Allocator-backed";
        StrReadFmt(z, "{s}", ZstrIO(zs, alloc_base));
        success = success && (ZstrCompare(zs, "Allocator-backed") == 0);

        z = "\"Allocator-backed replacement\"";
        StrReadFmt(z, "{s}", ZstrIO(zs, alloc_base));
        success = success && (ZstrCompare(zs, "Allocator-backed replacement") == 0);
        zstr_deinit(&zs, alloc_base);
    }

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);

    return success;
}

bool test_multiple_arguments_reading(void) {
    WriteFmt("Testing multiple arguments reading\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Zstr z = NULL;

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

// Each case here feeds malformed input; the contract is that the
// destination variable is left at its pre-read value on parse failure.
bool test_error_handling_reading(void) {
    WriteFmt("Testing error handling for reading\n");

    Zstr z = NULL;

    bool success = true;

    i32 num = 42;
    z       = "Count: forty-two";
    StrReadFmt(z, "Count: {}", num);
    success = success && (num == 42);

    num = 42;
    z   = "Count: abc";
    StrReadFmt(z, "Count: {}", num);
    success = success && (num == 42);

    i8 small = 42;
    z        = "Value: 1000";
    StrReadFmt(z, "Value: {}", small);
    success = success && (small == 42);

    return success;
}

bool test_character_ordinal_reading(void) {
    WriteFmt("Testing character ordinal reading with :c format specifier\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Zstr z = NULL;

    bool success = true;

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

    u16_val = 0;
    z       = "AB";
    StrReadFmt(z, "{c}", u16_val);
    bool u16_multi_pass = (ZstrCompareN((Zstr)&u16_val, "AB", 2) == 0);
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
    bool i16_multi_pass = (ZstrCompareN((Zstr)&i16_val, "CD", 2) == 0);
    WriteFmt("i16_val multi-char test: comparing memory with 'CD', pass = {}\n", i16_multi_pass ? "true" : "false");
    success = success && i16_multi_pass;

    u32_val = 0;
    z       = "EFGH";
    StrReadFmt(z, "{c}", u32_val);
    bool u32_multi_pass = (ZstrCompareN((Zstr)&u32_val, "EFGH", 4) == 0);
    WriteFmt("u32_val multi-char test: comparing memory with 'EFGH', pass = {}\n", u32_multi_pass ? "true" : "false");
    success = success && u32_multi_pass;

    i32_val = 0;
    z       = "IJKL";
    StrReadFmt(z, "{c}", i32_val);
    bool i32_multi_pass = (ZstrCompareN((Zstr)&i32_val, "IJKL", 4) == 0);
    WriteFmt("i32_val multi-char test: comparing memory with 'IJKL', pass = {}\n", i32_multi_pass ? "true" : "false");
    success = success && i32_multi_pass;

    u64_val = 0;
    z       = "MNOPQRST";
    StrReadFmt(z, "{c}", u64_val);
    bool u64_multi_pass = (ZstrCompareN((Zstr)&u64_val, "MNOPQRST", 8) == 0);
    WriteFmt(
        "u64_val multi-char test: comparing memory with 'MNOPQRST', pass = {}\n",
        u64_multi_pass ? "true" : "false"
    );
    success = success && u64_multi_pass;

    i64_val = 0;
    z       = "UVWXYZab";
    StrReadFmt(z, "{c}", i64_val);
    bool i64_multi_pass = (ZstrCompareN((Zstr)&i64_val, "UVWXYZab", 8) == 0);
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
    bool xy_pass = (ZstrCompareN((Zstr)&u32_val, "XY", 2) == 0);
    WriteFmt("u32_val partial test: comparing memory with 'XY', pass = {}\n", xy_pass ? "true" : "false");
    success = success && xy_pass;

    u64_val = 0;
    z       = "abc";
    StrReadFmt(z, "{c}", u64_val);
    bool abc_pass = (ZstrCompareN((Zstr)&u64_val, "abc", 3) == 0);
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

bool test_string_case_conversion_reading(void) {
    WriteFmt("Testing string case conversion with :a and :A format specifiers\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Zstr z = NULL;

    bool success = true;

    // Test 1: :a (lowercase) conversion
    {
        Str  result = StrInit(&alloc);
        Zstr in     = "Hello World";

        z = in;
        StrReadFmt(z, "{a}", result);

        WriteFmt("Test 1 - :a (lowercase)\n");
        WriteFmt("Input: '{}', Output: '", in);
        for (size i = 0; i < StrLen(&result); i++) {
            WriteFmt("{c}", StrBegin(&result)[i]);
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
        Str  result = StrInit(&alloc);
        Zstr in     = "Hello World";

        z = in;
        StrReadFmt(z, "{as}", result);

        WriteFmt("Test 1.1 - :as (lowercase string single word)\n");
        WriteFmt("Input: '{}', Output: '", in);
        for (size i = 0; i < StrLen(&result); i++) {
            WriteFmt("{c}", StrBegin(&result)[i]);
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
        Str  result = StrInit(&alloc);
        Zstr in     = "hello world";

        z = in;
        StrReadFmt(z, "{A}", result);

        WriteFmt("Test 2 - :A (uppercase)\n");
        WriteFmt("Input: '{}', Output: '", in);
        for (size i = 0; i < StrLen(&result); i++) {
            WriteFmt("{c}", StrBegin(&result)[i]);
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
        Str  result1 = StrInit(&alloc);
        Str  result2 = StrInit(&alloc);
        Zstr in      = "hello world";

        z = in;
        StrReadFmt(z, "{A} {A}", result1, result2);

        WriteFmt("Test 2 - :A (uppercase with split format)\n");
        WriteFmt("Input: '{}', Output: '{} {}'", in, result1, result2);

        bool test2_pass  = (StrCmp(&result1, "HELLO") == 0);
        test2_pass      &= (StrCmp(&result2, "WORLD") == 0);
        WriteFmt("Expected: 'HELLO WORLD', Pass: {}\n\n", test2_pass ? "true" : "false");
        success = success && test2_pass;

        StrDeinit(&result1);
        StrDeinit(&result2);
    }

    // Test 2.2: :A (uppercase) conversion
    {
        Str  result1 = StrInit(&alloc);
        Str  result2 = StrInit(&alloc);
        Zstr in      = "hello world mighty misra";

        z = in;
        StrReadFmt(z, "{As}{A}", result1, result2);
        // result1 must consume first word only
        // result2 must consume the space after hello and then everything after it

        WriteFmt("Test 2 - :A (uppercase with split format)\n");
        WriteFmt("Input: '{}', Output: '{}{}'", in, result1, result2);

        bool test2_pass  = (StrCmp(&result1, "HELLO") == 0);
        test2_pass      &= (StrCmp(&result2, " WORLD MIGHTY MISRA") == 0); // notice the extra space
        WriteFmt("Expected: 'HELLO WORLD MIGHTY MISRA', Pass: {}\n\n", test2_pass ? "true" : "false");
        success = success && test2_pass;

        StrDeinit(&result1);
        StrDeinit(&result2);
    }

    // Test 3: :a with quoted string
    {
        Str  result = StrInit(&alloc);
        Zstr in     = "\"MiXeD CaSe\"";

        z = in;
        StrReadFmt(z, "{as}", result);

        WriteFmt("Test 3 - :a with quoted string\n");
        WriteFmt("Input: '{}', Output: '", in);
        for (size i = 0; i < StrLen(&result); i++) {
            WriteFmt("{c}", StrBegin(&result)[i]);
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
        Str  result = StrInit(&alloc);
        Zstr in     = "\"abc123XYZ\"";

        z = in;
        StrReadFmt(z, "{As}", result);

        WriteFmt("Test 4 - :A with mixed alphanumeric\n");
        WriteFmt("Input: '{}', Output: '", in);
        for (size i = 0; i < StrLen(&result); i++) {
            WriteFmt("{c}", StrBegin(&result)[i]);
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
        Str  result = StrInit(&alloc);
        Zstr in     = "Hello World";

        z = in;
        StrReadFmt(z, "{c}", result);

        WriteFmt("Test 5 - :c (no case conversion)\n");
        WriteFmt("Input: '{}', Output: '", in);
        for (size i = 0; i < StrLen(&result); i++) {
            WriteFmt("{c}", StrBegin(&result)[i]);
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

bool test_bitvec_reading(void) {
    WriteFmt("Testing BitVec reading\n");

    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    Zstr z = NULL;

    bool success = true;

    BitVec bv1 = BitVecInit(alloc_base);
    z          = "10110";
    StrReadFmt(z, "{}", bv1);
    Str result1 = BitVecToStr(&bv1);
    success     = success && (ZstrCompare(StrBegin(&result1), "10110") == 0);
    WriteFmt(
        "Test 1 - Binary: {}, Success: {}\n",
        result1,
        (ZstrCompare(StrBegin(&result1), "10110") == 0) ? "true" : "false"
    );
    StrDeinit(&result1);
    BitVecDeinit(&bv1);

    BitVec bv2 = BitVecInit(alloc_base);
    z          = "0xDEAD";
    StrReadFmt(z, "{}", bv2);
    u64 value2 = BitVecToInteger(&bv2);
    success    = success && (value2 == 0xDEAD);
    WriteFmt("Test 2 - Hex: {}, Success: {}\n", value2, (value2 == 0xDEAD) ? "true" : "false");
    BitVecDeinit(&bv2);

    BitVec bv3 = BitVecInit(alloc_base);
    z          = "0o755";
    StrReadFmt(z, "{}", bv3);
    u64 value3 = BitVecToInteger(&bv3);
    success    = success && (value3 == 0755);
    WriteFmt("Test 3 - Octal: {}, Success: {}\n", value3, (value3 == 0755) ? "true" : "false");
    BitVecDeinit(&bv3);

    BitVec bv4 = BitVecInit(alloc_base);
    z          = "   1101";
    StrReadFmt(z, "{}", bv4);
    Str result4 = BitVecToStr(&bv4);
    success     = success && (ZstrCompare(StrBegin(&result4), "1101") == 0);
    WriteFmt(
        "Test 4 - Whitespace: {}, Success: {}\n",
        result4,
        (ZstrCompare(StrBegin(&result4), "1101") == 0) ? "true" : "false"
    );
    StrDeinit(&result4);
    BitVecDeinit(&bv4);

    BitVec bv5 = BitVecInit(alloc_base);
    z          = "0";
    StrReadFmt(z, "{}", bv5);
    Str result5 = BitVecToStr(&bv5);
    success     = success && (ZstrCompare(StrBegin(&result5), "0") == 0);
    WriteFmt(
        "Test 5 - Zero: {}, Success: {}\n",
        result5,
        (ZstrCompare(StrBegin(&result5), "0") == 0) ? "true" : "false"
    );
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

    Zstr z       = NULL;
    bool success = true;

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
    success  = success && (ZstrCompare(StrBegin(&dec_text), "123456789012345678901234567890") == 0);

    z = "deadbeefcafebabe1234";
    StrReadFmt(z, "{x}", hex);
    hex_text = IntToHexStr(&hex);
    success  = success && (ZstrCompare(StrBegin(&hex_text), "deadbeefcafebabe1234") == 0);

    z = "10100011";
    StrReadFmt(z, "{b}", bin);
    bin_text = IntToBinary(&bin);
    success  = success && (ZstrCompare(StrBegin(&bin_text), "10100011") == 0);

    z = "755";
    StrReadFmt(z, "{o}", oct);
    oct_text = IntToOctStr(&oct);
    success  = success && (ZstrCompare(StrBegin(&oct_text), "755") == 0);

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

    Zstr z       = NULL;
    bool success = true;

    Float dec = FloatInit(alloc_base);
    Float sci = FloatInit(alloc_base);
    Float neg = FloatInit(alloc_base);

    Str dec_text = StrInit(&alloc);
    Str sci_text = StrInit(&alloc);
    Str neg_text = StrInit(&alloc);

    z = "1234567890.012345";
    StrReadFmt(z, "{}", dec);
    dec_text = FloatToStr(&dec);
    success  = success && (ZstrCompare(StrBegin(&dec_text), "1234567890.012345") == 0);

    z = "1.234567e+04";
    StrReadFmt(z, "{e}", sci);
    sci_text = FloatToStr(&sci);
    success  = success && (ZstrCompare(StrBegin(&sci_text), "12345.67") == 0);

    z = "-0.00125";
    StrReadFmt(z, "{}", neg);
    neg_text = FloatToStr(&neg);
    success  = success && (ZstrCompare(StrBegin(&neg_text), "-0.00125") == 0);

    StrDeinit(&dec_text);
    StrDeinit(&sci_text);
    StrDeinit(&neg_text);
    FloatDeinit(&dec);
    FloatDeinit(&sci);
    FloatDeinit(&neg);
    DefaultAllocatorDeinit(&alloc);

    return success;
}

// ===========================================================================
// Mutation-hardening guards moved from Io.Mutants* staging files.
// ===========================================================================

// --- from Io.Mutants1.c (str_read_fmt brace/escape grammar) ---

// Escaped "{{" must consume exactly one '{' from the input and return the
// pointer just past it. Kills: 570 ge_to_gt/ge_to_lt (rem_p>=2 gate on the
// "{{" branch), 571 ne_to_eq (*in != '{'), 577 sub_assign (rem_p -= 2).
static bool test_m1_escaped_open_brace(void) {
    Zstr in  = "{rest";
    Zstr out = str_read_fmt(
        in,
        "{{",
        (TypeSpecificIO[]) {
            {NULL, NULL, NULL}
    },
        0
    );
    // Real: matches the literal '{', returns pointer to "rest".
    return out == in + 1;
}

// A bare "{{" against input "{" alone: real consumes the single brace and
// returns input+1 (end of string). A mutant that skips the escaped-brace
// branch treats '{' as a placeholder opener and fails (unmatched / NULL).
// Reinforces 570/571/577.
static bool test_m1_escaped_open_brace_exact(void) {
    Zstr in  = "{";
    Zstr out = str_read_fmt(
        in,
        "{{",
        (TypeSpecificIO[]) {
            {NULL, NULL, NULL}
    },
        0
    );
    return out == in + 1;
}

// Escaped "}}" must consume exactly one '}'. Kills: 578 ge_to_gt/ge_to_lt
// (rem_p>=2), 578 eq_to_ne (p[1] == '}'), 579 ne_to_eq (*in != '}'),
// 585 sub_assign (rem_p -= 2).
static bool test_m1_escaped_close_brace(void) {
    Zstr in  = "}rest";
    Zstr out = str_read_fmt(
        in,
        "}}",
        (TypeSpecificIO[]) {
            {NULL, NULL, NULL}
    },
        0
    );
    return out == in + 1;
}

static bool test_m1_escaped_close_brace_exact(void) {
    Zstr in  = "}";
    Zstr out = str_read_fmt(
        in,
        "}}",
        (TypeSpecificIO[]) {
            {NULL, NULL, NULL}
    },
        0
    );
    return out == in + 1;
}

// --- from Io.Mutants11.c (ZstrProcessEscape) ---

// --- Line 2176: `if (*s != '\\')` cxx_ne_to_eq ---
// A valid escape starts with '\\'. Real code skips the error branch and
// decodes; the mutant `*s == '\\'` enters the error branch and returns 0.
static bool test_m11_simple_escape_decodes(void) {
    Zstr p = "\\n";
    char c = ZstrProcessEscape(&p);
    return c == '\n';
}

// --- Lines 2186/2189/2192/2195/2198/2201/2204/2207/2210/2213/2216:
//     result = '<X>'; each cxx_assign_const replaces the RHS char with 42. ---
// Assert the exact decoded byte for every single-char escape so a 42 swap
// on any one case mismatches.
static bool test_m11_escape_n(void) {
    Zstr p = "\\n";
    return ZstrProcessEscape(&p) == '\n';
}
static bool test_m11_escape_r(void) {
    Zstr p = "\\r";
    return ZstrProcessEscape(&p) == '\r';
}
static bool test_m11_escape_t(void) {
    Zstr p = "\\t";
    return ZstrProcessEscape(&p) == '\t';
}
static bool test_m11_escape_b(void) {
    Zstr p = "\\b";
    return ZstrProcessEscape(&p) == '\b';
}
static bool test_m11_escape_f(void) {
    Zstr p = "\\f";
    return ZstrProcessEscape(&p) == '\f';
}
static bool test_m11_escape_v(void) {
    Zstr p = "\\v";
    return ZstrProcessEscape(&p) == '\v';
}
static bool test_m11_escape_a(void) {
    Zstr p = "\\a";
    return ZstrProcessEscape(&p) == '\a';
}
static bool test_m11_escape_backslash(void) {
    Zstr p = "\\\\";
    return ZstrProcessEscape(&p) == '\\';
}
static bool test_m11_escape_dquote(void) {
    Zstr p = "\\\"";
    return ZstrProcessEscape(&p) == '"';
}
static bool test_m11_escape_squote(void) {
    Zstr p = "\\'";
    return ZstrProcessEscape(&p) == '\'';
}
// case '0' -> result = '\0'; cxx_assign_const(42) at 2216 makes it '*'.
static bool test_m11_escape_nul(void) {
    Zstr p = "\\0";
    return ZstrProcessEscape(&p) == '\0';
}

// --- from Io.Mutants14.c (_read_u8) ---

// ---------------------------------------------------------------------------
// 2665:14 cxx_replace_scalar_call -- the per-char `!is_valid_number_char(...)`
// guard that terminates the digit scan. Forcing the call truthy (-> `if(!42)`
// == `if(false)`) makes the scan NEVER break, so it swallows the trailing
// invalid byte too. Input "42!" then collects "42!", which fails downstream
// validation -> no advance, value untouched. Live code breaks at '!', parses
// "42" -> 42 and advances 2 bytes.
// ---------------------------------------------------------------------------
static bool test_m14_scan_stops_at_invalid_char(void) {
    Zstr z = "42!";
    u8   v = 0;
    StrReadFmt(z, "{}", v);
    // Live: v == 42 and z advanced past "42" (now points at "!").
    return v == 42 && z[0] == '!';
}

// --- from Io.Mutants18.c (read_chars_internal) ---

// 1581:24 (current[0]=='\\' -> !=), 1581:46 (current[1]=='x' -> !=),
// 1587:28 (current[2]!='\0' -> ==), 1587:50 (current[3]!='\0' -> ==),
// 1588:25 (hex_val=... -> 42), 1588:27 (hex_byte(..) call -> 42),
// 1590:25 (hex_val>=0 -> <0), 1591:32 (char_to_store=(u8)hex_val -> 42).
// Real: "\x41" decodes to 0x41 ('A') and consumes all 4 input bytes.
// Every listed mutant either skips the hex branch (stores '\' = 0x5C) or
// substitutes 42 ('*') for the decoded byte -- all differ from 0x41.
static bool test_m18_hex_escape_decode(void) {
    Zstr z   = "\\x41"; // four bytes: '\', 'x', '4', '1'
    Zstr beg = z;
    u8   v   = 0;
    StrReadFmt(z, "{c}", v);
    bool ok = (v == 0x41);     // 'A'; mutants give 0x5C ('\') or 42 ('*')
    ok      = ok && (*z == 0); // all 4 bytes consumed; mutants leave "x41"
    (void)beg;
    return ok;
}

// 1590:25 (hex_val>=0 -> >0, ge_to_gt). Only distinguishable at hex_val==0.
// Real: "\x00" decodes to byte 0x00. The >0 mutant treats 0 as a failed
// decode and salvages the literal '\' (0x5C) instead.
static bool test_m18_hex_escape_zero_byte(void) {
    Zstr z = "\\x00";      // '\', 'x', '0', '0'
    u8   v = 0xAA;         // poison so a no-write is detectable too
    StrReadFmt(z, "{c}", v);
    bool ok = (v == 0x00); // mutant stores 0x5C ('\')
    ok      = ok && (*z == 0);
    return ok;
}

// 1586:17 (i32 hex_val = -1 -> 42). Reached only when the two-hex-digit
// guard at 1587 is false, i.e. a truncated "\x" at end-of-input. Real:
// hex_val stays -1, salvage path stores literal '\' (0x5C). Mutant:
// hex_val starts at 42, passes hex_val>=0, stores (u8)42 ('*').
// Also pins 1598:31 (char_to_store=(u8)*current -> 42): real stores 0x5C,
// the 42-mutant stores '*'.
static bool test_m18_truncated_hex_escape(void) {
    Zstr z = "\\x";              // '\', 'x', then NUL
    u8   v = 0;
    StrReadFmt(z, "{c}", v);
    bool ok = (v == 0x5C);       // '\' salvaged; mutants store 42 ('*')
    ok      = ok && (*z == 'x'); // only the '\' consumed
    return ok;
}

// --- from Io.Mutants21.c (_read_r32) ---

// ---------------------------------------------------------------------------
// Big-endian path, line 3602:
//   *v = ((u32)p[0] << 24) | ((u32)p[1] << 16) | ((u32)p[2] << 8) | p[3];
// Input bytes {0x12,0x34,0x56,0x78} read big-endian -> 0x12345678.
// Kills:
//   3602:16 assign_const  (*v <- 42 != 0x12345678),
//   3602:29 <<24 -> >>24  (0x12>>24 == 0 -> value loses high byte),
//   3602:49 <<16 -> >>16  (0x34>>16 == 0 -> value loses byte),
//   3602:69 <<8  -> >>8   (0x56>>8  == 0 -> value loses byte),
//   3602:36 | -> &        ((0x12000000 & 0x340000) == 0 -> != 0x12345678),
//   3602:56 | -> &        ((... & 0x5600) == 0 -> != 0x12345678),
//   3602:75 | -> &        ((... & 0x78) == 0 -> != 0x12345678).
// Every listed mutant yields a u32 other than 0x12345678; one exact compare
// rejects them all.
// ---------------------------------------------------------------------------
static bool test_m21_read_r32_big_endian_exact(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Buf              b     = BufInit(&alloc);
    bool             ok    = true;

    const u8 bytes[4] = {0x12, 0x34, 0x56, 0x78};
    ok                = ok && BufPushBytes(&b, bytes, 4);

    BufIter it    = BufIterFromBuf(&b);
    u32     v32   = 0;
    bool    rd_ok = BufReadFmt(&it, "{>4r}", v32);
    ok            = ok && rd_ok && (v32 == 0x12345678u);

    BufDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// Little-endian path, line 3605:
//   *v = ((u32)p[3] << 24) | ((u32)p[2] << 16) | ((u32)p[1] << 8) | p[0];
// Input bytes {0x78,0x56,0x34,0x12} read little-endian -> 0x12345678
// (p[3]=0x12<<24, p[2]=0x34<<16, p[1]=0x56<<8, p[0]=0x78).
// Kills:
//   3605:16 assign_const  (*v <- 42 != 0x12345678),
//   3605:29 <<24 -> >>24  (p[3]=0x12>>24 == 0 -> loses high byte),
//   3605:49 <<16 -> >>16  (p[2]=0x34>>16 == 0 -> loses byte),
//   3605:69 <<8  -> >>8   (p[1]=0x56>>8  == 0 -> loses byte),
//   3605:36 | -> &        (disjoint byte lanes -> & == 0 -> != 0x12345678),
//   3605:56 | -> &        (same),
//   3605:75 | -> &        (same).
// One exact compare rejects them all.
// ---------------------------------------------------------------------------
static bool test_m21_read_r32_little_endian_exact(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Buf              b     = BufInit(&alloc);
    bool             ok    = true;

    const u8 bytes[4] = {0x78, 0x56, 0x34, 0x12};
    ok                = ok && BufPushBytes(&b, bytes, 4);

    BufIter it    = BufIterFromBuf(&b);
    u32     v32   = 0;
    bool    rd_ok = BufReadFmt(&it, "{<4r}", v32);
    ok            = ok && rd_ok && (v32 == 0x12345678u);

    BufDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// Cross-check: the SAME big-endian input bytes read little-endian must give the
// byte-reversed value, and vice-versa. This pins the per-byte lane positions so
// a shift/or mutant cannot be masked by symmetry between the two branches.
// {0x12,0x34,0x56,0x78} read little-endian -> 0x78563412.
// Reinforces the 3605 shift mutants from the opposite byte pattern.
// ---------------------------------------------------------------------------
static bool test_m21_read_r32_little_endian_reversed(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Buf              b     = BufInit(&alloc);
    bool             ok    = true;

    const u8 bytes[4] = {0x12, 0x34, 0x56, 0x78};
    ok                = ok && BufPushBytes(&b, bytes, 4);

    BufIter it    = BufIterFromBuf(&b);
    u32     v32   = 0;
    bool    rd_ok = BufReadFmt(&it, "{<4r}", v32);
    ok            = ok && rd_ok && (v32 == 0x78563412u);

    BufDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// Native-endian resolution, lines 3594-3595:
//   if (fmt_info->endian == ENDIAN_NATIVE) {                 // 3594
//       fmt_info->endian = IS_LITTLE_ENDIAN() ? ENDIAN_LITTLE : ENDIAN_BIG; // 3595
//   }
// A `{^4r}` directive selects ENDIAN_NATIVE, which MUST be resolved to the
// host's concrete endianness before the switch; otherwise the switch hits its
// `default:` arm -> LOG_FATAL (process abort).
//
// Kills:
//   3594:26 == -> !=  (native spec no longer resolves -> default -> abort),
//   3595:26 assign_const (endian <- 42 -> not BIG/LITTLE -> default -> abort).
//
// A native-endian raw round-trip (write `^4r`, read `^4r`) succeeds and yields
// the original value on real code; under either mutant the READ aborts. The
// round-trip is host-endianness agnostic by construction.
// ---------------------------------------------------------------------------
static bool test_m21_read_r32_native_resolves_roundtrip(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Buf              b     = BufInit(&alloc);
    bool             ok    = true;

    ok = ok && BufWriteFmt(&b, "{^4r}", (u32)0xAABBCCDDu);
    ok = ok && (BufLength(&b) == 4);

    BufIter it    = BufIterFromBuf(&b);
    u32     v32   = 0;
    bool    rd_ok = BufReadFmt(&it, "{^4r}", v32);
    ok            = ok && rd_ok && (v32 == 0xAABBCCDDu);

    BufDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// --- from Io.Mutants22.c (_read_r64) ---

// ---------------------------------------------------------------------------
// Line 3632/3633: the ENDIAN_LITTLE reassembly
//   *v = ((u64)p[7] << 56) | ((u64)p[6] << 48) | ((u64)p[5] << 40)
//      | ((u64)p[4] << 32) | ((u64)p[3] << 24) | ((u64)p[2] << 16)
//      | ((u64)p[1] << 8)  | (u64)p[0];
// The LE write (_write_r64) lays the bytes out p[0]=LSB ... p[7]=MSB, so a
// {<8r} write-then-read of 0x0102030405060708 (all eight bytes distinct and
// non-zero) round-trips to exactly that value on real code.
//
// Kills every survivor on those two lines:
//   3632:29/49/69/89/109 + 3633:29/49 cxx_lshift_to_rshift -- turning any
//     `byte << k` into `byte >> k` zeroes/shrinks that byte's contribution
//     (e.g. p[7]>>56 == 0), so the reconstructed value loses bits 8..63 of
//     the corresponding byte and no longer equals 0x0102030405060708.
//   3632:36/56/76/96/116 + 3633:36/55 cxx_or_to_and -- replacing a `|` with
//     `&` ANDs a high partial sum against a single low byte/term, collapsing
//     the result toward 0 (e.g. ...&p[0] keeps only bits set in 0x08), again
//     != 0x0102030405060708.
// A single exact-value assertion rejects all ten mutant reconstructions.
// ---------------------------------------------------------------------------
static bool test_m22_little_endian_read_roundtrip_u64(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Buf              b     = BufInit(&alloc);
    bool             ok    = true;

    u64 expected = 0x0102030405060708ULL;

    ok = ok && BufWriteFmt(&b, "{<8r}", expected);
    ok = ok && (BufLength(&b) == 8);

    BufIter it    = BufIterFromBuf(&b);
    u64     v64   = 0;
    bool    rd_ok = BufReadFmt(&it, "{<8r}", v64);
    ok            = ok && rd_ok && (v64 == expected);

    BufDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// A second little-endian value whose low byte differs from the high byte and
// whose middle bytes carry weight, to defend the LE reassembly against any
// "happened to match" coincidence and to pin the byte ORDER (vs the big-endian
// arm). 0x1122334455667788 written LE, read LE, must round-trip exactly; a
// big-endian misread would reconstruct 0x8877665544332211.
// ---------------------------------------------------------------------------
static bool test_m22_little_endian_read_distinct_bytes(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Buf              b     = BufInit(&alloc);
    bool             ok    = true;

    u64 expected = 0x1122334455667788ULL;

    ok = ok && BufWriteFmt(&b, "{<8r}", expected);
    ok = ok && (BufLength(&b) == 8);

    BufIter it    = BufIterFromBuf(&b);
    u64     v64   = 0;
    bool    rd_ok = BufReadFmt(&it, "{<8r}", v64);
    ok            = ok && rd_ok && (v64 == expected);

    BufDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// Line 3621: native-endian resolution in _read_r64
//   if (fmt_info->endian == ENDIAN_NATIVE)
//       fmt_info->endian = IS_LITTLE_ENDIAN() ? ENDIAN_LITTLE : ENDIAN_BIG;
// The `^` prefix selects ENDIAN_NATIVE on the raw `r` path. Mutating the
// assigned value to 42 (cxx_assign_const at 3621:26) leaves endian neither
// LITTLE, BIG, nor NATIVE, so the switch below falls into the default arm ->
// LOG_FATAL (process abort). A native-endian {^8r} raw u64 round-trip
// succeeds and round-trips on real code; under the mutant the read aborts.
// ---------------------------------------------------------------------------
static bool test_m22_native_endian_read_roundtrip_u64(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Buf              b     = BufInit(&alloc);
    bool             ok    = true;

    u64 expected = 0x0102030405060708ULL;

    ok = ok && BufWriteFmt(&b, "{^8r}", expected);
    ok = ok && (BufLength(&b) == 8);

    BufIter it    = BufIterFromBuf(&b);
    u64     v64   = 0;
    bool    rd_ok = BufReadFmt(&it, "{^8r}", v64);
    ok            = ok && rd_ok && (v64 == expected);

    BufDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// --- from Io.Mutants3.c (str_append_fmt) ---

// ---------------------------------------------------------------------------
// `}}` escape index arithmetic: `i + 1` (line 456). The cxx_add_to_sub
// mutant turns the lookahead into `i - 1`, which underflows at i == 0 so
// the escape branch is skipped and "}}" is treated as an unmatched brace
// (returns false). Real code emits a single '}'.
// ---------------------------------------------------------------------------
static bool test_m3_double_close_brace_escape(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              out   = StrInit(&alloc);

    bool ok = StrAppendFmt(&out, "}}");
    ok      = ok && (ZstrCompare(StrBegin(&out), "}") == 0);

    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// --- from Io.Mutants31.c (ZstrHexDigitValue) ---

// Lowercase 'a'..'f' map to 10..15. Kills 1246:19 add_to_sub
// (10 + (c-'a') -> 10 - (c-'a')): 'b' real=11 mutant=9, 'f' real=15 mutant=5.
static bool test_m31_hexdigit_lowercase(void) {
    return ZstrHexDigitValue('a') == 10 && ZstrHexDigitValue('b') == 11 && ZstrHexDigitValue('f') == 15;
}

// Uppercase 'A' must return 10. Kills 1248:11 ge_to_gt / ge_to_lt
// (c >= 'A'): with '>' or '<', 'A' falls through to the -1 sentinel.
// Also kills 1248:23 le_to_gt (c <= 'F' -> c > 'F'): 'A' then yields -1.
// Also kills 1249:24 sub_to_add ((c-'A') -> (c+'A')): 'A' would give 140.
static bool test_m31_hexdigit_upper_A(void) {
    return ZstrHexDigitValue('A') == 10;
}

// Uppercase 'F' must return 15. Kills 1248:23 le_to_lt (c <= 'F' -> c < 'F'):
// 'F' would fall through to -1. Kills 1249:19 add_to_sub
// (10 + (c-'A') -> 10 - (c-'A')): 'F' real=15 mutant=5.
static bool test_m31_hexdigit_upper_F(void) {
    return ZstrHexDigitValue('F') == 15;
}

// Sanity: digits and an invalid char, to keep the contract honest.
static bool test_m31_hexdigit_misc(void) {
    return ZstrHexDigitValue('0') == 0 && ZstrHexDigitValue('9') == 9 && ZstrHexDigitValue('g') == -1 &&
           ZstrHexDigitValue('@') == -1;
}

// --- from Io.Mutants32.c (_read_ZstrAlloc / _read_r16) ---

// Io.c:2884:34 cxx_assign_const AND Io.c:2884:41 cxx_replace_scalar_call
// -- `default_fmt.max_read_len = (u32)ZstrLen(i);` taken when a
// non-NULL fmt arrives with max_read_len == 0. Both mutants pin the
// cap to 42, truncating a >42-char field. Real reads the full 50
// chars; mutant stops at 42. Caller observes the truncated string.
static bool test_m32_zstralloc_zero_maxlen_reads_full(void) {
    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    Zstr input = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmn"; // 50 chars, no spaces/backslashes

    char     *out = NULL;
    ZstrIOArg arg = {.value = (void *)&out, .allocator = alloc_base};
    FmtInfo   fmt = {0}; // max_read_len == 0 -> hits the !max_read_len branch

    Zstr next = _read_ZstrAlloc(input, &fmt, &arg);
    bool ok   = (next != NULL) && out && (ZstrCompare(out, input) == 0);

    if (out) {
        zstr_deinit(&out, alloc_base);
    }
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Io.c:2880:62 cxx_replace_scalar_call -- the NULL-fmt default
// `.max_read_len = (u32)ZstrLen(i)`. Reached only when fmt_info is
// NULL. The mutant pins the cap to 42, truncating the 50-char field;
// real code reads the whole string.
static bool test_m32_zstralloc_null_fmt_reads_full(void) {
    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    Zstr input = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmn"; // 50 chars

    char     *out = NULL;
    ZstrIOArg arg = {.value = (void *)&out, .allocator = alloc_base};

    Zstr next = _read_ZstrAlloc(input, NULL, &arg);
    bool ok   = (next != NULL) && out && (ZstrCompare(out, input) == 0);

    if (out) {
        zstr_deinit(&out, alloc_base);
    }
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Io.c:3576:16 cxx_assign_const (`*v = 42`),
// Io.c:3576:29 cxx_lshift_to_rshift (`p[0] << 8` -> `>> 8`),
// Io.c:3576:35 cxx_or_to_and (`... | p[1]` -> `... & p[1]`),
// Io.c:3566:26 cxx_eq_to_ne (`endian == ENDIAN_NATIVE`).
// `{>2r}` reads big-endian: bytes 0x12,0x34 -> 0x1234. All four
// mutants change the result:
//   *v=42 -> 42; >>8 -> 0x0034; &p[1] -> 0x1200&0x34 == 0;
//   eq->ne re-resolves the explicit BIG endian to host order
//   (little -> 0x3412 on this platform).
static bool test_m32_read_r16_big_endian(void) {
    Zstr input = "\x12\x34";       // big-endian 0x1234
    Zstr start = input;
    u16  v     = 0;
    StrReadFmt(input, "{>2r}", v); // advances `input` on success
    return (input != start) && (v == 0x1234);
}

// Io.c:3567:26 cxx_assign_const -- the NATIVE-resolution assignment
// `fmt_info->endian = IS_LITTLE_ENDIAN() ? LITTLE : BIG`. Reached only
// when endian == ENDIAN_NATIVE (`{^2r}`). Real resolves to a concrete
// host order and reads a valid value; the `= 42` mutant stores an
// invalid endian -> the switch hits its default -> LOG_FATAL. Real
// code succeeds and yields the host-order interpretation of 0x12,0x34.
static bool test_m32_read_r16_native_resolves(void) {
    Zstr input    = "\x12\x34";
    Zstr start    = input;
    u16  v        = 0;
    u16  expected = IS_LITTLE_ENDIAN() ? (u16)0x3412 : (u16)0x1234;
    StrReadFmt(input, "{^2r}", v); // real: resolves NATIVE; mutant: LOG_FATAL
    return (input != start) && (v == expected);
}

// --- from Io.Mutants33.c ---

// `{c}` on an integer renders each non-printable byte as `\xNN`. Byte 0x1A has
// low nibble 10 (kills line 1192 `low < 10` -> `<=`, which would emit ':' for a
// value of 10) and byte 0xE0 has high nibble 14 (kills line 1191 `'a' + (hiw -
// 10)` -> `'a' - (hiw - 10)`, which would emit ']').
static bool test_m33_char_hex_escape_nibbles(void) {
    DefaultAllocator alloc  = DefaultAllocatorInit();
    Str              output = StrInit(&alloc);

    u16 v = (u16)0x1AE0; // big-endian bytes: 0x1A, 0xE0

    StrAppendFmt(&output, "{c}", v);

    bool ok = (ZstrCompare(StrBegin(&output), "\\x1a\\xe0") == 0);

    StrDeinit(&output);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Line 2353 (cxx_eq_to_ne): `c == '+'` -> `c != '+'`. With `!=`, a leading '+'
// (which is NOT '+' under the negated test combined with the '-' clause)
// becomes rejected, so "+5" no longer parses as a number. Real code accepts a
// leading '+'.
static bool test_m33_read_leading_plus(void) {
    i32  v = 0;
    Zstr z = "+5";
    StrReadFmt(z, "{}", v);
    return v == 5;
}

// Line 3557 (cxx_assign_const): `*v = (u8)*i` replaced by `*v = 42` makes every
// raw 1-byte read yield 42 regardless of input. Round-trip a non-42 byte.
static bool test_m33_read_r8_value(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Buf              b     = BufInit(&alloc);

    bool ok = BufAppendFmt(&b, "{<1r}", (u8)0xAB);

    BufIter it  = BufIterFromBuf(&b);
    u8      out = 0;
    ok          = ok && BufReadFmt(&it, "{<1r}", out) && (out == 0xAB);

    BufDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// --- from Io.Mutants34.c (_read_f32 validity gate) ---

// ---------------------------------------------------------------------------
// 3434:10 _read_f32 -- validity gate `if (!is_valid_numeric_string(&temp,true))`
//   mutated to the literal value 42 (call still runs; `!42` is false so the
//   rejection branch is skipped -> input treated as always valid).
// Input "1f": the char-scan accepts it (hex letters pass is_valid_number_char),
// but is_valid_numeric_string rejects it (a bare 'f' with no 0x prefix).
// Real => reject, *v untouched (stays at its sentinel). Mutant => skips the
// gate and feeds "1f" to StrToF64, producing a different *v / advancing past it.
// We pin the rejection contract: the destination keeps its pre-read value.
// ---------------------------------------------------------------------------
static bool test_if_3434_f32_invalid_numeric_rejected(void) {
    bool ok = true;

    f32  v = 7.5f;
    Zstr z = "1f";
    StrReadFmt(z, "{}", v);
    // Real rejects "1f" -> v stays 7.5. Mutant bypasses the gate.
    ok = ok && (v == 7.5f);

    return ok;
}

// --- from Io.Mutants4.c (f_read_fmt / FReadFmt) ---

// ---------------------------------------------------------------------------
// 1121 (end_pos init / FileSeek(END) call) and 1131:gt_to_le and
// 1134:lt_to_ge.
// Place the parse target BEYOND byte 42 of the file. Real code computes
// file_len from the true end (well past 42) and reads the whole region,
// so the number is found. If end_pos is forced to 42, file_len caps the
// read at 42 bytes (all padding) and the number is never reached ->
// value stays at init. Also, `if (file_len > 0)` swapped to `<= 0`
// (1131 gt_to_le) skips the read for a non-empty file, and `if (got<0)`
// swapped to `>= 0` (1134 lt_to_ge) takes the early-return error path on
// a good read -- both leave the value unset. Asserting the value parsed
// kills all three.
// ---------------------------------------------------------------------------
static bool test_m4_end_pos_sizes_full_read(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              path  = StrInit(&alloc);

    // 50 leading spaces, then the number well past byte index 42.
    char content[64];
    for (u64 i = 0; i < 50; ++i)
        content[i] = ' ';
    content[50] = '9';
    content[51] = '1';

    File f  = m4_make_temp(&alloc, &path, content, 52);
    bool ok = FileIsOpen(&f);

    i32 v = -1;
    FReadFmt(&f, "{}", v);
    ok = ok && (v == 91);

    m4_cleanup(&f, &path);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// 1131:gt_to_ge corner / 1112-style emptiness: a zero-length file must
// leave the destination untouched and the position at 0 (no parse, no
// advance). Guards that the empty-buffer `if (StrLen(&buffer))` gate
// and the file_len gate stay coherent on the empty path.
// ---------------------------------------------------------------------------
static bool test_m4_empty_file_no_parse(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              path  = StrInit(&alloc);
    File             f     = m4_make_temp(&alloc, &path, "", 0);
    bool             ok    = FileIsOpen(&f);

    i32 v = 1234;
    FReadFmt(&f, "{}", v);
    ok = ok && (v == 1234);         // untouched
    ok = ok && (FileTell(&f) == 0); // no advance

    m4_cleanup(&f, &path);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// --- from Io.Mutants5.c (_read_f32) ---

// Companion: a non-empty char-flag read overwrites the low byte, proving
// the {c} path actually populates `temp` (and locks its value).
static bool test_m5_char_flag_reads_ordinal(void) {
    f32  v = 0.0f;
    Zstr z = "A";
    StrReadFmt(z, "{c}", v); // 'A' == 0x41 == 65
    return f32_close(v, 65.0f);
}

// 3422:14 cxx_replace_scalar_call -- `!is_valid_number_char(...)` break.
// Forcing the call value truthy (mutant) makes `!42` == false, so the
// scanner never breaks on the invalid 'g' and swallows the whole "12g3"
// token (which then fails validation, leaving `*v` untouched). Real code
// breaks at 'g', parses the "12" prefix into 12.0.
static bool test_m5_invalid_char_breaks_scan(void) {
    f32  v = 0.0f;
    Zstr z = "12g3";
    StrReadFmt(z, "{}", v);
    return f32_close(v, 12.0f);
}

// --- from Io.Mutants7.c (_read_Str escape branches) ---

// Decoded escape value must equal the real escaped character, and reading
// must continue past the escape to the following bytes.
//
// Kills (unquoted branch):
//   2309:22 cxx_init_const       -> c forced to 42 ('*') instead of 'A'
//   2309:29 cxx_replace_scalar   -> ZstrProcessEscape() value/side-effect lost
//   2316:24 cxx_ge_to_lt         -> r collapses to 0 after escape, "BC" dropped
static bool test_m7_unquoted_escape_value_and_continue(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str  s = StrInit(&alloc);
    Zstr z = "\\x41BC"; // backslash, x, 4, 1, B, C  ->  'A', 'B', 'C'
    StrReadFmt(z, "{}", s);

    Str  expected = StrInitFromZstr("ABC", &alloc);
    bool ok       = (StrLen(&s) == 3) && (StrCmp(&s, &expected) == 0);

    StrDeinit(&expected);
    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A VALID escape (decoded value != 0) must NOT take the failure path. The
// `if (c == 0)` guard distinguishes a valid escape from a decode failure.
//
// Kills (unquoted branch):
//   2310:23 cxx_eq_to_ne -> valid escape misread as failure -> Str emptied
static bool test_m7_unquoted_escape_valid_not_failure(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str  s = StrInit(&alloc);
    Zstr z = "\\x41"; // -> 'A'
    StrReadFmt(z, "{}", s);

    Str  expected = StrInitFromZstr("A", &alloc);
    bool ok       = (StrLen(&s) == 1) && (StrCmp(&s, &expected) == 0);

    StrDeinit(&expected);
    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// force-case branch on a decoded escape char: `{A}` must uppercase the
// decoded 'a' to 'A' (not replace it with a constant).
//
// Kills (unquoted branch):
//   2319:23 cxx_assign_const -> c forced to 42 ('*') instead of TO_UPPER('a')
static bool test_m7_unquoted_escape_force_case(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str  s = StrInit(&alloc);
    Zstr z = "\\x61";                             // -> 'a'
    StrReadFmt(z, "{A}", s);

    Str  expected = StrInitFromZstr("A", &alloc); // uppercased
    bool ok       = (StrLen(&s) == 1) && (StrCmp(&s, &expected) == 0);

    StrDeinit(&expected);
    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Decoded escape value inside a quoted field must equal the real escaped
// character and reading continues to the closing quote.
//
// Kills (quoted branch):
//   2270:22 cxx_init_const       -> c forced to 42 ('*') instead of 'A'
//   2270:29 cxx_replace_scalar   -> ZstrProcessEscape() value/side-effect lost
//   2280:24 cxx_ge_to_lt         -> r collapses to 0 after escape, "BC" dropped
static bool test_m7_quoted_escape_value_and_continue(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str s = StrInit(&alloc);
    // "\x41BC" inside double quotes -> decoded "ABC"
    Zstr z = "\"\\x41BC\"";
    StrReadFmt(z, "{s}", s);

    Str  expected = StrInitFromZstr("ABC", &alloc);
    bool ok       = (StrLen(&s) == 3) && (StrCmp(&s, &expected) == 0);

    StrDeinit(&expected);
    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A VALID escape inside a quoted field must not take the failure path.
//
// Kills (quoted branch):
//   2271:23 cxx_eq_to_ne -> valid escape misread as failure -> Str emptied
static bool test_m7_quoted_escape_valid_not_failure(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str  s = StrInit(&alloc);
    Zstr z = "\"\\x41\""; // quoted -> 'A'
    StrReadFmt(z, "{s}", s);

    Str  expected = StrInitFromZstr("A", &alloc);
    bool ok       = (StrLen(&s) == 1) && (StrCmp(&s, &expected) == 0);

    StrDeinit(&expected);
    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// force-case branch on a decoded escape char inside a quoted field.
//
// Kills (quoted branch):
//   2283:23 cxx_assign_const -> c forced to 42 ('*') instead of TO_UPPER('a')
static bool test_m7_quoted_escape_force_case(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str  s = StrInit(&alloc);
    Zstr z = "\"\\x61\"";                         // quoted -> 'a'
    StrReadFmt(z, "{As}", s);

    Str  expected = StrInitFromZstr("A", &alloc); // uppercased
    bool ok       = (StrLen(&s) == 1) && (StrCmp(&s, &expected) == 0);

    StrDeinit(&expected);
    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// --- from Io.Mutants9.c (write_char_internal escape nibbles) ---

// Both nibbles in the digit branch (0..9), high nibble NON-zero so the
// `'0' + hiw` add (L1229 col39) is distinguishable from `'0' - hiw`.
// 0x12 -> hi=1 ('1'), lo=2 ('2').  Pins:
//   L1228 init-const (hiw=42 -> wrong c1), rshift->lshift
//         ((0x12<<4)&0xf == 0 -> c1='0'), and->or (1|0xf=15 -> c1='f')
//   L1227 init-const (low=42 -> wrong c2), and->or (0x12|0xf=31 -> c2='f')
//   L1229/1230 col28 `<`->`>=` (real hiw=1<10 digit; mutated forces the
//         letter branch 'a'+(1-10) -> garbage), col39 add->sub.
static bool test_m9_escape_both_digit_nibbles(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              out   = StrInit(&alloc);
    bool             ok    = true;

    Zstr b = "\x12";
    StrAppendFmt(&out, "{c}", b);
    ok = ok && (ZstrCompare(StrBegin(&out), "\\x12") == 0);

    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// High-nibble digit branch with a different non-zero value, low-nibble zero.
// 0x90 -> hi=9 ('9'), lo=0 ('0').  Reinforces L1229 col39 `'0'+hiw`
// (add->sub gives '0'-9, a control char != '9') and the rshift mutation
// ((0x90<<4)&0xf == 0 -> c1='0' != '9').
static bool test_m9_escape_high_digit_nonzero(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              out   = StrInit(&alloc);
    bool             ok    = true;

    Zstr b = "\x90";
    StrAppendFmt(&out, "{c}", b);
    ok = ok && (ZstrCompare(StrBegin(&out), "\\x90") == 0);

    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Both nibbles in the lowercase LETTER branch at the `<10` boundary.
// 0xab -> hi=10 ('a'), lo=11 ('b'), spec {c} so is_caps=false.  Pins:
//   L1229/1230 col28 `<`->`<=` : real (10<10)==false picks the letter
//        branch ('a'); mutated (10<=10)==true picks '0'+10 == ':' != 'a'.
//   L1229 col87 / L1230 col87 sub->add: `hiw-10` -> `hiw+10` == 20,
//        'a'+20 == 'u' != 'a'.
//   is_caps selection: a caps mutation here would emit 'A'/'B'.
static bool test_m9_escape_letter_boundary_lowercase(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              out   = StrInit(&alloc);
    bool             ok    = true;

    Zstr b = "\xab";
    StrAppendFmt(&out, "{c}", b);
    ok = ok && (ZstrCompare(StrBegin(&out), "\\xab") == 0);

    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Both nibbles in the lowercase LETTER branch ABOVE the boundary so the
// `nibble - 10` offset is non-zero and the leading-letter add matters.
// 0xcd -> hi=12 ('a'+2='c'), lo=13 ('a'+3='d'), spec {c}.  Pins:
//   L1229 col80 `'a'+(hiw-10)` add->sub: 'a'-2 == 0x5f ('_') != 'c'.
//   L1230 col80 add->sub on the low nibble: 'a'-3 != 'd'.
//   col87 sub->add reinforced (hiw+10=22 -> 'a'+22='w' != 'c').
static bool test_m9_escape_letter_above_boundary_lowercase(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              out   = StrInit(&alloc);
    bool             ok    = true;

    Zstr b = "\xcd";
    StrAppendFmt(&out, "{c}", b);
    ok = ok && (ZstrCompare(StrBegin(&out), "\\xcd") == 0);

    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Uppercase LETTER branch at the `<10` boundary via the caps spec {A}.
// 0xAB with {A} -> is_caps=true -> hi=10 ('A'), lo=11 ('B').  Pins:
//   L1229 col68 / L1230 col68 sub->add on the caps branch:
//        `hiw-10` -> `hiw+10` == 20, 'A'+20 == 'U' != 'A'.
//   Confirms is_caps actually selects the 'A'-based branch (col61/80
//        differ in case): a caps mutation would emit lowercase.
static bool test_m9_escape_letter_boundary_uppercase(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              out   = StrInit(&alloc);
    bool             ok    = true;

    Zstr b = "\xab";
    StrAppendFmt(&out, "{A}", b);
    ok = ok && (ZstrCompare(StrBegin(&out), "\\xAB") == 0);

    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Uppercase LETTER branch above the boundary so `nibble - 10` is non-zero.
// 0xCD with {A} -> hi=12 ('A'+2='C'), lo=13 ('A'+3='D').  Pins:
//   L1229 col61 `'A'+(hiw-10)` add->sub: 'A'-2 == 0x3f ('?') != 'C'.
//   L1230 col61 add->sub on the low nibble: 'A'-3 != 'D'.
static bool test_m9_escape_letter_above_boundary_uppercase(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              out   = StrInit(&alloc);
    bool             ok    = true;

    Zstr b = "\xcd";
    StrAppendFmt(&out, "{A}", b);
    ok = ok && (ZstrCompare(StrBegin(&out), "\\xCD") == 0);

    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Mixed nibbles: high in the digit branch, low in the letter branch (and
// vice-versa) to ensure the two ternaries are independently exercised in a
// single byte and that the low-nibble `>>`/`&` isolation is correct.
// 0x1b -> hi=1 ('1'), lo=11 ('b') with {c};
// 0xb1 -> hi=11 ('b'), lo=1 ('1') with {c}.
static bool test_m9_escape_mixed_nibbles(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              out   = StrInit(&alloc);
    bool             ok    = true;

    Zstr lo_letter = "\x1b";
    StrAppendFmt(&out, "{c}", lo_letter);
    ok = ok && (ZstrCompare(StrBegin(&out), "\\x1b") == 0);
    StrClear(&out);

    Zstr hi_letter = "\xb1";
    StrAppendFmt(&out, "{c}", hi_letter);
    ok = ok && (ZstrCompare(StrBegin(&out), "\\xb1") == 0);

    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// DateTime parses from ISO 8601 through the IO reader. Lives in the Io
// suite so mutation testing of Io.c's _read_DateTime is covered: the Z /
// +HH:MM / -HH:MM offset forms (incl. half-hour minutes), the
// nanosecond branch, and the field comparisons.
static bool test_datetime_iso_read(void) {
    bool ok = true;
    {
        Zstr     in = "2021-01-01T00:00:00Z";
        DateTime d  = {0};
        StrReadFmt(in, "{}", d);
        ok = ok && d.year == 2021 && d.month == 1 && d.day == 1 && d.hour == 0 && d.minute == 0 && d.second == 0 &&
             d.nanosecond == 0 && d.utc_offset_seconds == 0 && d.weekday == 5 /* Friday, derived on parse */;
    }
    {
        Zstr     in = "2021-01-01T05:30:00+05:30";
        DateTime d  = {0};
        StrReadFmt(in, "{}", d);
        ok = ok && d.year == 2021 && d.hour == 5 && d.minute == 30 && d.utc_offset_seconds == 19800;
    }
    {
        Zstr     in = "2020-12-31T14:30:00-09:30";
        DateTime d  = {0};
        StrReadFmt(in, "{}", d);
        ok = ok && d.year == 2020 && d.month == 12 && d.day == 31 && d.hour == 14 && d.minute == 30 &&
             d.utc_offset_seconds == -34200;
    }
    {
        Zstr     in = "2021-01-01T00:00:00.123456789Z";
        DateTime d  = {0};
        StrReadFmt(in, "{}", d);
        ok = ok && d.nanosecond == 123456789 && d.utc_offset_seconds == 0;
    }
    return ok;
}

// A format spec of exactly 32 chars must be REJECTED (spec_len >= 32). The
// read fails, so the input cursor stays put and the destination is untouched.
static bool test_read_spec_too_long_rejected(void) {
    u64  v     = 0;
    Zstr p     = "ff";
    Zstr start = p;
    StrReadFmt(p, "{xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx}", v); // exactly 32 'x'
    return (p == start) && (v == 0);
}

static bool test_read_spec_len_32_boundary_rejected(void) {
    u64  v     = 999;
    Zstr p     = "5";
    Zstr start = p;
    StrReadFmt(p, "{11111111111111111111111111111111}", v); // valid 32-char width spec
    return (p == start) && (v == 999);
}

// Unquoted-string read budget binds at the interior literal anchor '#': two
// `\n` escapes exhaust the budget, so the field is exactly 2 bytes and 'rest'
// is not consumed (pins the escape-budget decrement against over-reading).
static bool test_read_escape_budget_anchor(void) {
    DefaultAllocator a   = DefaultAllocatorInit();
    Str              out = StrInit(ALLOCATOR_OF(&a));
    Zstr             p   = "\\n\\n#rest";
    StrReadFmt(p, "{}#", out);
    bool ok = (StrLen(&out) == 2) && (StrBegin(&out)[0] == '\n') && (StrBegin(&out)[1] == '\n');
    StrDeinit(&out);
    DefaultAllocatorDeinit(&a);
    return ok;
}

// _read_u8 hex form: "0xff" parses to 255, but a BARE prefix "0x" (no digits)
// must be REJECTED (pins the `StrLen==2 && [0]=='0' && [1]=='x'` bare-prefix
// guard: the mutant that disables it would accept "0x" and read 0).
static bool test_read_u8_hex(void) {
    u8 v = 0;
    {
        Zstr p = "0xff";
        StrReadFmt(p, "{}", v);
        if (v != 255)
            return false;
    }
    // Every bare prefix (no digits) must be rejected -- covers whichever
    // prefix-letter comparison the mutant flips.
    Zstr bare[] = {"0x", "0X", "0b", "0B", "0o", "0O"};
    for (u32 k = 0; k < sizeof(bare) / sizeof(bare[0]); ++k) {
        Zstr p     = bare[k];
        Zstr start = p;
        u8   w     = 7;
        StrReadFmt(p, "{}", w);
        if (p != start || w != 7) // real: read fails, cursor + dest untouched
            return false;
    }
    return true;
}

// max_read_len defaults to the remaining input length; an unanchored string
// field longer than the mutant's constant (42) must read in FULL (the
// `max_read_len = rem_in` survivor would cap it at 42).
static bool test_read_long_string_no_cap(void) {
    DefaultAllocator a   = DefaultAllocatorInit();
    Str              out = StrInit(ALLOCATOR_OF(&a));
    // Quoted string of 50 chars: max_read_len (= rem_in) must allow the full
    // read; the `max_read_len = rem_in` survivor would cap it at 42.
    Zstr p = "\"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\""; // 50 'a' quoted
    StrReadFmt(p, "{s}", out);
    bool ok = (StrLen(&out) == 50);
    StrDeinit(&out);
    DefaultAllocatorDeinit(&a);
    return ok;
}

// --- helpers/macros relocated from Io.Blind.c + Io.Leak.c ---
#define READ1(in, fmt, IO) str_read_fmt((in), (fmt), (TypeSpecificIO[]) {(IO)}, 1)
#define SENTINEL           (-987654.0)

#define LEAK_CFG                                                                                                       \
    ((DebugAllocatorConfig) {.capture_traces = false, .detect_overflow = false, .track_freed_history = false})


// ===========================================================================
// Mutation-hardening read / scratch-leak tests relocated from Io.Blind.c.
// ===========================================================================
static bool f64_is(f64 v, f64 e) {
    return F64Abs(v - e) < 1e-9;
}

// Read a Float and compare its canonical form. A wrong token boundary leaves
// the pre-read sentinel.
static bool read_float_is(Zstr in, Zstr expect) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *ab    = ALLOCATOR_OF(&alloc);
    Float            f     = FloatInit(ab);
    (void)FloatTryFromStr(&f, "99"); // sentinel
    StrReadFmt(in, "{}", f);
    Str  t  = FloatToStr(&f);
    bool ok = (ZstrCompare(StrBegin(&t), expect) == 0);
    StrDeinit(&t);
    FloatDeinit(&f);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// 592:26 gt_to_ge `rem_p > 0` (spec scan loop). 605:26 ge_to_gt `spec_len>=32`
// (over-long spec rejected). A normal spec "{}" with a value reads fine.
static bool test_read_spec_scan(void) {
    i32  v   = 0;
    Zstr in  = "42";
    Zstr out = READ1(in, "{}", TO_TYPE_SPECIFIC_IO(i32, &v));
    return out == in + 2 && v == 42;
}

// 605:26 ge_to_gt: a 32-char spec body is rejected. Build "{" + 32 'q' + "}".
static bool test_read_overlong_spec_rejected(void) {
    i32 v = 7;
    // 32 chars inside braces -> spec_len == 32 -> rejected.
    Zstr out = READ1("x", "{qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq}", TO_TYPE_SPECIFIC_IO(i32, &v));
    return out == NULL && v == 7;
}

// 618:21 init_const spec_ok=false, 622:32 assign_const data[spec_len]='\0',
// 624 parse gate: an invalid spec char 'q' must fail the parse and leave the
// destination untouched (covered in Read suite already, repeat for spec_ok).
static bool test_read_invalid_spec_leaves_dest(void) {
    i32  v   = 77;
    Zstr out = READ1("5", "{q}", TO_TYPE_SPECIFIC_IO(i32, &v));
    return out == NULL && v == 77;
}

// 630:35 assign_const `fmt_info.max_read_len = rem_in`: a {s} string read at
// end of format uses max_read_len = remaining input. A quoted string of 40
// chars must be read in full (not truncated to a constant 42... but 42 > 40
// so we need > 42). Use 50 chars: forcing max_read_len to a constant 42 caps
// the read at 42, observable as a short string.
static bool test_read_max_read_len_set(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Zstr             in    = "\"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\""; // 50 A's quoted
    Str              s     = StrInit(&alloc);
    Zstr             out   = READ1(in, "{s}", TO_TYPE_SPECIFIC_IO(Str, &s));
    bool             ok    = (out != NULL) && (StrLen(&s) == 50);
    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// 682:21 init_const x=0, 692:23 init_const var_width=0, 712:36 gt swaps, plus
// the raw read store. {<4r} into a u32 from 4 LE bytes -> the value. var_width
// 0->42 mis-selects the store; x 0->42 corrupts before pickup.
static bool test_read_raw_u32_le(void) {
    u32  v   = 0;
    Zstr in  = "\xEF\xBE\xAD\xDE";
    Zstr out = READ1(in, "{<4r}", TO_TYPE_SPECIFIC_IO(u32, &v));
    return out == in + 4 && v == 0xDEADBEEFu;
}

// 686:37 sub_to_add / 686:28 sub_assign (rem_in -= next-in in the RAW branch):
// after a raw read followed by a literal, rem_in must stay correct so the
// literal matches. {<1r} then literal 'Z': input "\x05Z".
static bool test_read_raw_then_literal(void) {
    u8   v   = 0;
    Zstr in  = "\x05Z";
    Zstr out = READ1(in, "{<1r}Z", TO_TYPE_SPECIFIC_IO(u8, &v));
    return out == in + 2 && v == 0x05;
}

// 748:22 init_const c=p[space_len], 750:49 add_to_sub `p[space_len + 1]`,
// 770:37/28 sub (rem_in -= next-in in the NON-raw branch), 792:19 post_dec
// (rem_in-- on a literal match). A {} field bounded by a trailing literal,
// then more input: "{}-end" on "12-end". The literal-run scan (748/750) caps
// the field at '-'; rem_in bookkeeping (770/792) keeps the tail in sync.
static bool test_read_field_bounded_by_literal(void) {
    i32  v   = 0;
    Zstr in  = "12-end";
    Zstr out = READ1(in, "{}-end", TO_TYPE_SPECIFIC_IO(i32, &v));
    return out == in + 6 && v == 12;
}

// 750:49 add_to_sub specifically: the literal run's escape check reads
// `p[space_len + 1]`. Use a non-numeric separator '!' so the field stops
// cleanly, then an escaped '{{' in the literal run. "{}!{{y" on "5!{y": field
// "5", literal run "!{{y" matches "!{y" -> consumes all 4 bytes. A +1 -> -1
// swap mis-reads the escape look-ahead and breaks the literal match.
static bool test_read_field_literal_with_escape(void) {
    i32  v   = 0;
    Zstr in  = "5!{y";
    Zstr out = READ1(in, "{}!{{y", TO_TYPE_SPECIFIC_IO(i32, &v));
    return out == in + 4 && v == 5;
}

// 820:13 / 829:17 init_const (fc/sc = 0), 854:23 ge_to_gt (arg_index>=argc),
// 864:14 init_const (x=0), 901:15 init_const (var_width=0): a clean buf read
// round-trip. 909:92 eq_to_ne (read_fn == _read_f64 in the u64/i64/f64 OR):
// covered by f64 below.
static bool test_buf_read_u16(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Buf              b     = BufInit(&alloc);
    bool             ok    = BufWriteFmt(&b, "{<2r}", (u16)0xBEEF);
    BufIter          it    = BufIterFromBuf(&b);
    u16              v     = 0;
    ok                     = ok && BufReadFmt(&it, "{<2r}", v) && (v == 0xBEEF);
    BufDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// 909:92 eq_to_ne (the `read_fn == _read_f64` clause): an f64 raw read must
// resolve var_width 8. A != swap drops f64 into the unsupported-type LOG_FATAL.
static bool test_buf_read_f64(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Buf              b     = BufInit(&alloc);
    union {
        f64 f;
        u64 u;
    } got      = {0};
    bool    ok = BufWriteFmt(&b, "{<8r}", (u64)0x0102030405060708ULL);
    BufIter it = BufIterFromBuf(&b);
    ok         = ok && BufReadFmt(&it, "{<8r}", got.f) && (got.u == 0x0102030405060708ULL);
    BufDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// 1511/1525 saw_digit, 1533 allow_sign after digit: a plain integer token.
static bool test_tokenlen_plain_digit(void) {
    return read_float_is("7 x", "7");
}

// 1539 saw_decimal: a decimal token "3.5".
static bool test_tokenlen_decimal(void) {
    return read_float_is("3.5 x", "3.5");
}

// 1515/1533 allow_sign (leading sign): "-2.5".
static bool test_tokenlen_signed(void) {
    return read_float_is("-2.5 x", "-2.5");
}

// 1546/1547/1548 exponent introducer (saw_exponent/need_exp_digit/allow_sign):
// "1.5e2" -> 150; "1.5e+2" sign after exponent.
static bool test_tokenlen_exponent(void) {
    return read_float_is("1.5e2 x", "150") && read_float_is("1.5e+2 x", "150") && read_float_is("2.5e-1 y", "0.25");
}

// 1514 need_exp_digit: "1.5e" with no exponent digit must FAIL (token_len 0),
// leaving the sentinel "99".
static bool test_tokenlen_incomplete_exponent(void) {
    // "1.5e" then space: float_fmt_token_length requires an exp digit, returns
    // 0 -> _read_Float fails -> sentinel kept.
    return read_float_is("1.5e ", "99");
}

// 2395:17 eq_to_ne `len == 3` (inf/nan short-circuit gate): "inf" (len 3) is a
// valid float; a !=3 swap rejects it.
static bool test_vns_inf_len3(void) {
    f64  v = SENTINEL;
    Zstr z = "inf";
    StrReadFmt(z, "{}", v);
    return F64IsInf(v) && v > 0;
}

// 2396 (4 eq) / 2397 (2 eq): each char of "inf" / "nan" checked both cases.
// "INF" upper, "nan" lower, "NaN" mixed all parse.
static bool test_vns_inf_nan_cases(void) {
    f64  a = SENTINEL, b = SENTINEL, c = SENTINEL;
    Zstr za = "INF", zb = "nan", zc = "NaN";
    StrReadFmt(za, "{}", a);
    StrReadFmt(zb, "{}", b);
    StrReadFmt(zc, "{}", c);
    return F64IsInf(a) && F64IsNan(b) && F64IsNan(c);
}

// 2400/2401 nan letters; 2406/2407/2408 the "-inf" (len 4) branch.
static bool test_vns_neg_inf_len4(void) {
    f64  a = SENTINEL, b = SENTINEL;
    Zstr za = "-inf", zb = "-INF";
    StrReadFmt(za, "{}", a);
    StrReadFmt(zb, "{}", b);
    return F64IsInf(a) && a < 0 && F64IsInf(b) && b < 0;
}

// 2406:33 eq_to_ne `data[0] == '-'`: a len-4 token that is NOT -inf, e.g.
// "1234", must NOT be hijacked by the -inf branch -> parses as 1234.
static bool test_vns_len4_not_neginf(void) {
    f64  v = SENTINEL;
    Zstr z = "1234";
    StrReadFmt(z, "{}", v);
    return f64_is(v, 1234.0);
}

// 2420/2422/2424 assign_const (is_hex/is_bin/is_oct true): a "0x1f" hex float
// slice must be ACCEPTED (StrToF64 parses leading 0 -> 0.0). 2418-detection
// then digit-walk. Also the upper-case 'X'/'B'/'O' variants (2419/2421/2423
// are killed by Write suite, but reinforce). Here pin is_hex assignment: "0x"
// alone (bare) is rejected; "0x1f" accepted (value 0.0 since StrToF64 of
// "0x1f" reads only the 0).
static bool test_vns_hex_slice_accepted(void) {
    f64  v = SENTINEL;
    Zstr z = "0x1f";
    StrReadFmt(z, "{}", v);
    // accepted -> StrToF64("0x1f") parses leading 0 -> 0.0.
    return f64_is(v, 0.0);
}

static bool test_vns_bin_slice_accepted(void) {
    f64  v = SENTINEL;
    Zstr z = "0b11";
    StrReadFmt(z, "{}", v);
    return f64_is(v, 0.0);
}

static bool test_vns_oct_slice_accepted(void) {
    f64  v = SENTINEL;
    Zstr z = "0o7";
    StrReadFmt(z, "{}", v);
    return f64_is(v, 0.0);
}

// 2436:48 eq_to_ne `i == 0 || i == 1` (prefix-skip). 2472 (binary digit set),
// 2476 (octal digit range): a binary slice "0b101" must accept 0/1 digits; an
// octal "0o17" must accept 0-7. The slice acceptance is observable as 0.0.
static bool test_vns_bin_digits(void) {
    f64  v = SENTINEL;
    Zstr z = "0b101";
    StrReadFmt(z, "{}", v);
    return f64_is(v, 0.0);
}

static bool test_vns_oct_digits(void) {
    f64  v = SENTINEL;
    Zstr z = "0o17";
    StrReadFmt(z, "{}", v);
    return f64_is(v, 0.0);
}

// 2476:38 le_to_lt / le_to_gt and 2476:26 ge variants: octal digit '7' is the
// upper boundary, '0' the lower. "0o70" must be accepted (both bounds).
static bool test_vns_oct_boundary_digits(void) {
    f64  v = SENTINEL;
    Zstr z = "0o70";
    StrReadFmt(z, "{}", v);
    return f64_is(v, 0.0);
}

// 2443:54 gt_to_ge `i > 0` (sign-after-exponent position guard): a normal
// "1e+5" has '+' at i==2 after 'e' -> accepted. The guard `i > 0` differs from
// `i >= 0` only at i==0; a leading sign at i==0 is handled by the i==0 clause,
// so to expose col-54 we need has_exp true and i>0; "1e+5" exercises it.
static bool test_vns_exp_sign(void) {
    f64  v = SENTINEL;
    Zstr z = "1e+5";
    StrReadFmt(z, "{}", v);
    return f64_is(v, 100000.0);
}

// 2452:25 has_decimal, 2460:21 has_exp assign_const, 2486 trailing-exp guard
// (1.2e is incomplete). 2486:39 sub `data[len-1]`, 2486:18 init last_char.
// A trailing 'e' makes the token invalid -> rejected.
static bool test_vns_double_decimal_rejected(void) {
    f64  v = SENTINEL;
    Zstr z = "1.2.3";
    StrReadFmt(z, "{}", v);
    // The f64 scanner stops the token at the 2nd '.', so the slice is "1.2" and
    // parses to 1.2, leaving ".3" unconsumed.  has_decimal must gate so a
    // double-dot inside one slice is rejected; here the token boundary handles
    // it. Pin the parsed prefix 1.2 (mutant on has_decimal assign would let the
    // 2nd dot through and corrupt).
    return f64_is(v, 1.2);
}

// 2486 trailing exponent guard: "12e" alone (whole input) -> incomplete -> the
// f64 reader's own exponent scan rewinds, slice "12", value 12.
static bool test_vns_trailing_exp(void) {
    f64  v = SENTINEL;
    Zstr z = "12e";
    StrReadFmt(z, "{}", v);
    return f64_is(v, 12.0);
}

// 2523:14 init_const temp=0 ({c} on f64 reads 8 bytes into temp): {c} read of
// 8 chars "ABCDEFGH" -> the f64 reinterpret. temp 0->42 corrupts the low byte.
static bool test_read_f64_char(void) {
    f64  v   = 0;
    Zstr in  = "ABCDEFGH";
    Zstr out = READ1(in, "{c}", TO_TYPE_SPECIFIC_IO(f64, &v));
    union {
        f64 f;
        u64 u;
    } got;
    got.f = v;
    // read_chars_internal stores bytes in order; the low 8 bytes are the chars.
    u64 expect = ((u64)'H' << 56) | ((u64)'G' << 48) | ((u64)'F' << 40) | ((u64)'E' << 32) | ((u64)'D' << 24) |
                 ((u64)'C' << 16) | ((u64)'B' << 8) | (u64)'A';
    // value is (f64)temp where temp holds the 8 bytes little-end first.
    (void)got;
    (void)expect;
    return out == in + 8 && v == (f64)expect;
}

// 2531:13 init_const c=0, 2586:57 gt_to_ge `StrIterIndex(&si) > StrIterIndex
// (&saved)` (exponent only after a digit): "1e5" -> 100000; a leading "e5"
// must NOT be treated as exponent.
static bool test_read_f64_exponent(void) {
    f64  v   = 0;
    Zstr in  = "1e5";
    Zstr out = READ1(in, "{}", TO_TYPE_SPECIFIC_IO(f64, &v));
    return out == in + 3 && f64_is(v, 100000.0);
}

// 2546:10 init_const c1=0 (peek-ahead for the leading-'-' inf clause): "-inf".
static bool test_read_f64_neg_inf(void) {
    f64  v   = 0;
    Zstr in  = "-inf";
    Zstr out = READ1(in, "{}", TO_TYPE_SPECIFIC_IO(f64, &v));
    return out == in + 4 && F64IsInf(v) && v < 0;
}

// 2580:25 has_decimal assign_const in _read_f64: "1.5.6" -> token "1.5", v=1.5,
// ".6" unconsumed. A mutant forcing has_decimal=42(true) at start would stop at
// the FIRST scan iteration. Pin v=1.5 and the cursor at the 2nd '.'.
static bool test_read_f64_double_dot(void) {
    f64  v   = 0;
    Zstr in  = "1.5.6";
    Zstr out = READ1(in, "{}", TO_TYPE_SPECIFIC_IO(f64, &v));
    return out == in + 3 && f64_is(v, 1.5) && (*out == '.');
}

// 2644:13 init_const c=0 in _read_u8: a plain u8 parse.
static bool test_read_u8(void) {
    u8   v   = 0;
    Zstr in  = "200";
    Zstr out = READ1(in, "{}", TO_TYPE_SPECIFIC_IO(u8, &v));
    return out == in + 3 && v == 200;
}

// 2680:50 eq_to_ne `StrLen(&temp) == 2` (bare-prefix guard) in _read_u8: a
// real "0xff" (len 4) must parse to 255 (guard does not fire). A != swap fires
// the guard for len 4 and rejects.
static bool test_blind_read_u8_hex(void) {
    u8   v   = 0;
    Zstr in  = "0xff";
    Zstr out = READ1(in, "{}", TO_TYPE_SPECIFIC_IO(u8, &v));
    return out == in + 4 && v == 255;
}

// 2689:10 replace_scalar_call `is_valid_numeric_string(&temp, false)` in
// _read_u8: input "1f" (lenient scan accepts 'f') must be REJECTED by the
// validator (trailing non-decimal) -> v untouched. Forcing the call truthy
// (42) bypasses rejection and parses "1" -> v=1.
static bool test_read_u8_invalid_rejected(void) {
    u8   v   = 7;
    Zstr in  = "1f";
    Zstr out = READ1(in, "{}", TO_TYPE_SPECIFIC_IO(u8, &v));
    // _read_u8 returns `start` (no advance) -> str_read_fmt sees next==in ->
    // returns NULL; v untouched.
    return out == NULL && v == 7;
}

// Bare prefix "0x" (len 2) IS rejected by the 2680 guard -> read does not
// advance -> str_read_fmt returns NULL; v untouched.
static bool test_read_u8_bare_prefix(void) {
    u8   v   = 7;
    Zstr in  = "0x";
    Zstr out = READ1(in, "{}", TO_TYPE_SPECIFIC_IO(u8, &v));
    return out == NULL && v == 7;
}

// ===========================================================================
// _read_Zstr (2889, 2899, 2908) -- _read_ZstrAlloc allocates the result and
// frees the previous through arg->allocator (a DebugAllocator -> observable).
// 2889/2899/2908 StrDeinit(&temp) removals leak the scratch temp.
// ===========================================================================
static bool test_read_zstr_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInit();
    char          *s   = NULL;
    Zstr           in  = "hello world";
    Zstr           out = str_read_fmt(in, "{}", (TypeSpecificIO[]) {ZstrIO(s, &dbg.base)}, 1);
    // Unquoted, non-{s} read takes the whole input (whitespace only ends an
    // is_string field), so s is the full "hello world".
    bool ok = (out != NULL) && s && (ZstrCompare(s, "hello world") == 0);
    if (s)
        AllocatorFree(&dbg.base, s);
    ok = ok && (DebugAllocatorLiveCount(&dbg) == 0);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// 3061:13 init c=0, 3074/3075 init c0/c1=0 (prefix peek), 3112:21 lt_to_le
// `bit_len < 4` (hex min width), value/width round-trip. 3104/3116 scratch
// dtor leak.
static bool test_read_bitvec_hex(void) {
    DebugAllocator dbg = DebugAllocatorInit();
    BitVec         bv  = BitVecInit(&dbg.base);
    Zstr           z   = "0x1";
    StrReadFmt(z, "{}", bv);
    bool ok = (BitVecToInteger(&bv) == 1) && (BitVecLen(&bv) == 4); // min-width clamp
    BitVecDeinit(&bv);
    ok = ok && (DebugAllocatorLiveCount(&dbg) == 0);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// hex with more bits so the clamp does NOT apply (3112 lt distinguishes).
static bool test_read_bitvec_hex_wide(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    BitVec           bv    = BitVecInit(&alloc.base);
    Zstr             z     = "0xDEAD";
    StrReadFmt(z, "{}", bv);
    bool ok = (BitVecToInteger(&bv) == 0xDEAD) && (BitVecLen(&bv) == 16);
    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// 3145/3154 scratch leak, 3150:21 lt_to_le `bit_len < 3` (octal min width).
static bool test_read_bitvec_octal(void) {
    DebugAllocator dbg = DebugAllocatorInit();
    BitVec         bv  = BitVecInit(&dbg.base);
    Zstr           z   = "0o1";
    StrReadFmt(z, "{}", bv);
    bool ok = (BitVecToInteger(&bv) == 1) && (BitVecLen(&bv) == 3);
    BitVecDeinit(&bv);
    ok = ok && (DebugAllocatorLiveCount(&dbg) == 0);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// 3178 scratch leak for the binary path.
static bool test_read_bitvec_binary(void) {
    DebugAllocator dbg = DebugAllocatorInit();
    BitVec         bv  = BitVecInit(&dbg.base);
    Zstr           z   = "10110";
    StrReadFmt(z, "{}", bv);
    bool ok = (BitVecLen(&bv) == 5) && (BitVecToInteger(&bv) == 13);
    BitVecDeinit(&bv);
    ok = ok && (DebugAllocatorLiveCount(&dbg) == 0);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// 3197:13 init c=0, 3213:10 init d0=0 (leading '+'), 3255:10 init ok, 3258
// scratch leak, value round-trip.
static bool test_read_int_plain(void) {
    DebugAllocator dbg = DebugAllocatorInit();
    Int            v   = IntInit(&dbg.base);
    Zstr           z   = "12345";
    StrReadFmt(z, "{}", v);
    Str  t  = IntToStr(&v);
    bool ok = (ZstrCompare(StrBegin(&t), "12345") == 0);
    StrDeinit(&t);
    IntDeinit(&v);
    ok = ok && (DebugAllocatorLiveCount(&dbg) == 0);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// 3213/3215 leading '+' skip (digits_saved). "+99" -> 99.
static bool test_read_int_leading_plus(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Int              v     = IntInit(&alloc.base);
    Zstr             z     = "+99";
    StrReadFmt(z, "{}", v);
    Str  t  = IntToStr(&v);
    bool ok = (ZstrCompare(StrBegin(&t), "99") == 0);
    StrDeinit(&t);
    IntDeinit(&v);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// 3220:10 / 3221:10 init p0/p1 (prefix peek), 3225 hex-prefix rejection: an
// Int hex read "{x}" of "ff" parses 255 but "0xff" is rejected (no 0x prefix
// allowed for Int). Pin the plain-hex accept.
static bool test_read_int_hex_plain(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Int              v     = IntInit(&alloc.base);
    Zstr             z     = "ff";
    StrReadFmt(z, "{x}", v);
    Str  t  = IntToStr(&v);
    bool ok = (ZstrCompare(StrBegin(&t), "255") == 0);
    StrDeinit(&t);
    IntDeinit(&v);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// 3247:10 init trailing=0 (digit-separator rejection): "12_3" must be rejected
// at the '_' -> the Int reader returns `start`, value untouched. Use a fresh
// Int with a sentinel value.
static bool test_read_int_underscore_rejected(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Int              v     = IntFrom(777, &alloc.base);
    Zstr             z     = "12_3";
    Zstr             out   = str_read_fmt(z, "{}", (TypeSpecificIO[]) {TO_TYPE_SPECIFIC_IO(Int, &v)}, 1);
    // Real: '_' rejected -> returns start (==z) -> str_read_fmt sees next==in ->
    // NULL; v unchanged (still 777).
    Str  t  = IntToStr(&v);
    bool ok = (out == NULL) && (ZstrCompare(StrBegin(&t), "777") == 0);
    StrDeinit(&t);
    IntDeinit(&v);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ===========================================================================
// _read_Float (3272, 3286, 3287, 3291, 3294, 3302, 3303, 3312, 3313, 3317,
// 3320, 3321). Dest Float written through FloatAllocator -> scratch leaks +
// value observable.
// ===========================================================================
static bool test_read_float_value(void) {
    DebugAllocator dbg = DebugAllocatorInit();
    Float          f   = FloatInit(&dbg.base);
    Zstr           z   = "3.14159";
    StrReadFmt(z, "{}", f);
    Str  t  = FloatToStr(&f);
    bool ok = (ZstrCompare(StrBegin(&t), "3.14159") == 0);
    StrDeinit(&t);
    FloatDeinit(&f);
    ok = ok && (DebugAllocatorLiveCount(&dbg) == 0);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// 3272:11 init token_len, 3310 token_len==0 reject: a non-float input leaves
// the sentinel and frees scratch.
static bool test_read_float_reject_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInit();
    Float          f   = FloatInit(&dbg.base);
    (void)FloatTryFromStr(&f, "42");
    Zstr z   = "xyz"; // no float token
    Zstr out = str_read_fmt(z, "{}", (TypeSpecificIO[]) {TO_TYPE_SPECIFIC_IO(Float, &f)}, 1);
    Str  t   = FloatToStr(&f);
    bool ok  = (out == NULL) && (ZstrCompare(StrBegin(&t), "42") == 0);
    StrDeinit(&t);
    FloatDeinit(&f);
    ok = ok && (DebugAllocatorLiveCount(&dbg) == 0);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// 3350:13 init c=0, 3398:25 has_decimal assign in _read_f32: a decimal token.
static bool test_read_f32_decimal(void) {
    f32  v   = 0;
    Zstr in  = "2.5";
    Zstr out = READ1(in, "{}", TO_TYPE_SPECIFIC_IO(f32, &v));
    return out == in + 3 && F64Abs((f64)v - 2.5) < 1e-4;
}

// 3365:10 init c1=0 (peek for leading-'-' inf): "-inf" as f32.
static bool test_read_f32_neg_inf(void) {
    f32  v   = 0;
    Zstr in  = "-inf";
    Zstr out = READ1(in, "{}", TO_TYPE_SPECIFIC_IO(f32, &v));
    return out == in + 4 && F64IsInf((f64)v) && v < 0;
}

// Write "42 hello" to a temp file, read an i32 then a literal+string. Pins
// 1128 file_len = end-cur (sub), 1131 file_len>0 (gt_to_ge), 1134 got<0
// (lt_to_le), 1147 rollback FileSeek.
static bool test_fread_seekable(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              path  = StrInit(&alloc);
    File             f     = FileOpenTemp(&path, &alloc);
    bool             ok    = FileIsOpen(&f);
    if (ok) {
        FileWrite(&f, "42", 2);
        FileSeek(&f, 0, FILE_SEEK_SET);
        i32 v = 0;
        FReadFmt(&f, "{}", v);
        ok = (v == 42);
        FileClose(&f);
        FileRemove(&path);
    }
    StrDeinit(&path);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// 1147 rollback: a parse that FAILS must rewind the file position. Write "xx"
// (not a number) then attempt an i32 read; the cursor must be unchanged so a
// following raw byte read still sees 'x'.
static bool test_fread_rollback(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              path  = StrInit(&alloc);
    File             f     = FileOpenTemp(&path, &alloc);
    bool             ok    = FileIsOpen(&f);
    if (ok) {
        FileWrite(&f, "xx", 2);
        FileSeek(&f, 0, FILE_SEEK_SET);
        i32 v = 123;
        FReadFmt(&f, "{}", v); // fails -> rollback
        // position must still be 0: read the first raw byte.
        char c   = 0;
        i64  got = FileRead(&f, &c, 1);
        ok       = (v == 123) && (got == 1) && (c == 'x');
        FileClose(&f);
        FileRemove(&path);
    }
    StrDeinit(&path);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ===========================================================================
// Read-side leak-guard tests + helpers relocated from Io.Leak.c.
// ===========================================================================
// 2408 / 2421 / 2466: quoted-string read whose budget saturates before the
// closing quote -> unterminated-quote branch frees the destination Str.
static bool leak_quoted_unterminated(Zstr input, Zstr fmt) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Str            s   = StrInit(ALLOCATOR_OF(&dbg));
    Zstr           p   = input;
    StrReadFmt(p, fmt, s); // reader frees s on the unterminated/error branch
    bool ok = (DebugAllocatorLiveCount(&dbg) == 0) && (DebugAllocatorLiveBytes(&dbg) == 0);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// 3232 / 3273: BitVec hex/oct read overflow error path frees the internal
// scratch (allocated through the destination BitVec's allocator).
static bool leak_bitvec_overflow(Zstr input) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    BitVec         bv  = BitVecInit(ALLOCATOR_OF(&dbg));
    Zstr           p   = input;
    StrReadFmt(p, "{}", bv);
    BitVecDeinit(&bv);
    bool ok = (DebugAllocatorLiveCount(&dbg) == 0) && (DebugAllocatorLiveBytes(&dbg) == 0);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_leak_read_quoted_budget_escape_freed(void) {
    // open-quote + 24 \n escapes + 'E' + close-quote, fmt "{s}E".
    return leak_quoted_unterminated(
        "\"\\n\\n\\n\\n\\n\\n\\n\\n\\n\\n\\n\\n\\n\\n\\n\\n\\n\\n\\n\\n\\n\\n\\n\\nE\"",
        "{s}E"
    );
}

bool test_leak_read_quoted_budget_plain_freed(void) {
    return leak_quoted_unterminated("\"aaaaaaaaaaaaaaaaaaaaaaaa#\"", "{s}#");
}

bool test_leak_read_quoted_unterminated_freed(void) {
    return leak_quoted_unterminated("\"aaaaaaaaaaaaaaaaaaaaaaaa", "{s}");
}

// 2439: unquoted invalid-escape error path frees the destination Str.
bool test_leak_read_unquoted_bad_escape_freed(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Str            s   = StrInit(ALLOCATOR_OF(&dbg));
    Zstr           p   = "aaaaaaaaaaaaaaaaaaaaaaaa\\q"; // 24 'a' + invalid \q
    StrReadFmt(p, "{}", s);
    bool ok = (DebugAllocatorLiveCount(&dbg) == 0) && (DebugAllocatorLiveBytes(&dbg) == 0);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_leak_read_bitvec_hex_overflow_freed(void) {
    return leak_bitvec_overflow("0xFFFFFFFFFFFFFFFFF"); // 17 hex digits > 64 bits
}

bool test_leak_read_bitvec_oct_overflow_freed(void) {
    return leak_bitvec_overflow("0o7777777777777777777777"); // 22 oct digits > 64 bits
}

// 3448: Float read whose token passes length-scan but FloatTryFromStr fails on
// exponent overflow -> fail branch frees the internal `temp` scratch.
bool test_leak_read_float_exp_overflow_freed(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Float          fv  = FloatInit(ALLOCATOR_OF(&dbg));
    Zstr           p   = "1e99999999999999999999999999999999999999999999999999";
    StrReadFmt(p, "{}", fv);
    FloatDeinit(&fv);
    bool ok = (DebugAllocatorLiveCount(&dbg) == 0) && (DebugAllocatorLiveBytes(&dbg) == 0);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// ---------------------------------------------------------------------------
// READ side, typed-container reader exception to the "all reads use internal
// DefaultAllocator scratch" rule documented in the header above. _read_Int /
// _read_Float allocate their parse scratch -- and overwrite `*value` -- through
// the DESTINATION container's OWN allocator (IntAllocator(value) /
// FloatAllocator(value)), which the caller supplies. So a removed internal
// *Deinit on the read success path IS observable when the destination is backed
// by a DebugAllocator, unlike the scalar readers (_read_u8 / _read_f64 /
// f_read_fmt) which truly use an uninjectable DefaultAllocatorInit() scratch.
//
// _read_Int success path: `IntDeinit(value)` then `*value = parsed`
// (Io.c:3262). The destination Int is pre-seeded with a heap-backed value, so
// dropping that IntDeinit leaks the PRIOR significand limbs -> live count != 0
// after the final IntDeinit.
// ---------------------------------------------------------------------------
bool test_leak_read_int_prior_value_freed(void) {
    DebugAllocator dbg  = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    // Seed the destination with a heap-backed Int (multi-limb) so the old
    // storage is visible in dbg's live count before the reassignment.
    Int value = IntFromStr("123456789012345678901234567890", adbg);

    Zstr input = "42";
    Zstr start = input;
    StrReadFmt(input, "{}", value);
    bool ok = (input != start); // pointer advanced => read succeeded

    IntDeinit(&value);
    ok = ok && (DebugAllocatorLiveCount(&dbg) == 0);
    ok = ok && (DebugAllocatorLiveBytes(&dbg) == 0);

    DebugAllocatorDeinit(&dbg);
    return ok;
}

// ---------------------------------------------------------------------------
// _read_Float success path: `FloatDeinit(value)` then `*value = parsed`
// (Io.c:3325). The destination Float is pre-seeded with a heap-backed
// significand, so dropping that FloatDeinit leaks the PRIOR significand
// storage -> live count != 0 after the final FloatDeinit.
// ---------------------------------------------------------------------------
bool test_leak_read_float_prior_value_freed(void) {
    DebugAllocator dbg  = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    // Seed with a heap-backed Float (wide significand) so the old significand
    // storage is visible in dbg's live count before the reassignment.
    Float value = FloatFromStr("987654321098765432109876543210.5", adbg);

    Zstr input = "2.5";
    Zstr start = input;
    StrReadFmt(input, "{}", value);
    bool ok = (input != start); // pointer advanced => read succeeded

    FloatDeinit(&value);
    ok = ok && (DebugAllocatorLiveCount(&dbg) == 0);
    ok = ok && (DebugAllocatorLiveBytes(&dbg) == 0);

    DebugAllocatorDeinit(&dbg);
    return ok;
}


int main(void) {
    WriteFmt("[INFO] Starting format reader tests\n\n");

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
        test_float_reading,
        test_datetime_iso_read,
        test_read_spec_too_long_rejected,
        test_read_spec_len_32_boundary_rejected,
        test_read_escape_budget_anchor,
        test_read_u8_hex,
        test_read_long_string_no_cap,
        test_m1_escaped_open_brace,
        test_m1_escaped_open_brace_exact,
        test_m1_escaped_close_brace,
        test_m1_escaped_close_brace_exact,
        test_m11_simple_escape_decodes,
        test_m11_escape_n,
        test_m11_escape_r,
        test_m11_escape_t,
        test_m11_escape_b,
        test_m11_escape_f,
        test_m11_escape_v,
        test_m11_escape_a,
        test_m11_escape_backslash,
        test_m11_escape_dquote,
        test_m11_escape_squote,
        test_m11_escape_nul,
        test_m14_scan_stops_at_invalid_char,
        test_m18_hex_escape_decode,
        test_m18_hex_escape_zero_byte,
        test_m18_truncated_hex_escape,
        test_m21_read_r32_big_endian_exact,
        test_m21_read_r32_little_endian_exact,
        test_m21_read_r32_little_endian_reversed,
        test_m21_read_r32_native_resolves_roundtrip,
        test_m22_little_endian_read_roundtrip_u64,
        test_m22_little_endian_read_distinct_bytes,
        test_m22_native_endian_read_roundtrip_u64,
        test_m3_double_close_brace_escape,
        test_m31_hexdigit_lowercase,
        test_m31_hexdigit_upper_A,
        test_m31_hexdigit_upper_F,
        test_m31_hexdigit_misc,
        test_m32_zstralloc_zero_maxlen_reads_full,
        test_m32_zstralloc_null_fmt_reads_full,
        test_m32_read_r16_big_endian,
        test_m32_read_r16_native_resolves,
        test_m33_char_hex_escape_nibbles,
        test_m33_read_leading_plus,
        test_m33_read_r8_value,
        test_if_3434_f32_invalid_numeric_rejected,
        test_m4_end_pos_sizes_full_read,
        test_m4_empty_file_no_parse,
        test_m5_char_flag_reads_ordinal,
        test_m5_invalid_char_breaks_scan,
        test_m7_unquoted_escape_value_and_continue,
        test_m7_unquoted_escape_valid_not_failure,
        test_m7_unquoted_escape_force_case,
        test_m7_quoted_escape_value_and_continue,
        test_m7_quoted_escape_valid_not_failure,
        test_m7_quoted_escape_force_case,
        test_m9_escape_both_digit_nibbles,
        test_m9_escape_high_digit_nonzero,
        test_m9_escape_letter_boundary_lowercase,
        test_m9_escape_letter_above_boundary_lowercase,
        test_m9_escape_letter_boundary_uppercase,
        test_m9_escape_letter_above_boundary_uppercase,
        test_m9_escape_mixed_nibbles,
        test_read_spec_scan,
        test_read_overlong_spec_rejected,
        test_read_invalid_spec_leaves_dest,
        test_read_max_read_len_set,
        test_read_raw_u32_le,
        test_read_raw_then_literal,
        test_read_field_bounded_by_literal,
        test_read_field_literal_with_escape,
        test_buf_read_u16,
        test_buf_read_f64,
        test_tokenlen_plain_digit,
        test_tokenlen_decimal,
        test_tokenlen_signed,
        test_tokenlen_exponent,
        test_tokenlen_incomplete_exponent,
        test_vns_inf_len3,
        test_vns_inf_nan_cases,
        test_vns_neg_inf_len4,
        test_vns_len4_not_neginf,
        test_vns_hex_slice_accepted,
        test_vns_bin_slice_accepted,
        test_vns_oct_slice_accepted,
        test_vns_bin_digits,
        test_vns_oct_digits,
        test_vns_oct_boundary_digits,
        test_vns_exp_sign,
        test_vns_double_decimal_rejected,
        test_vns_trailing_exp,
        test_read_f64_char,
        test_read_f64_exponent,
        test_read_f64_neg_inf,
        test_read_f64_double_dot,
        test_read_u8,
        test_blind_read_u8_hex,
        test_read_u8_invalid_rejected,
        test_read_u8_bare_prefix,
        test_read_zstr_no_leak,
        test_read_bitvec_hex,
        test_read_bitvec_hex_wide,
        test_read_bitvec_octal,
        test_read_bitvec_binary,
        test_read_int_plain,
        test_read_int_leading_plus,
        test_read_int_hex_plain,
        test_read_int_underscore_rejected,
        test_read_float_value,
        test_read_float_reject_no_leak,
        test_read_f32_decimal,
        test_read_f32_neg_inf,
        test_fread_seekable,
        test_fread_rollback,
        test_leak_read_quoted_budget_escape_freed,
        test_leak_read_quoted_budget_plain_freed,
        test_leak_read_quoted_unterminated_freed,
        test_leak_read_unquoted_bad_escape_freed,
        test_leak_read_bitvec_hex_overflow_freed,
        test_leak_read_bitvec_oct_overflow_freed,
        test_leak_read_float_exp_overflow_freed,
        test_leak_read_int_prior_value_freed,
        test_leak_read_float_prior_value_freed,
    };

    int total_tests = sizeof(tests) / sizeof(tests[0]);

    return run_test_suite(tests, total_tests, NULL, 0, "Io.Read");
}
