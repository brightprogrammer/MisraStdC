#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Log.h>
#include <Misra/Types.h>

// Include test utilities
#include "../../Util/TestRunner.h"

// Function prototypes
bool test_str_insert_char_at(void);
bool test_str_insert_cstr(void);
bool test_str_insert_zstr(void);
bool test_str_push_cstr(void);
bool test_str_push_zstr(void);
bool test_str_push_back_cstr(void);
bool test_str_push_back_zstr(void);
bool test_str_push_front_cstr(void);
bool test_str_push_front_zstr(void);
bool test_str_push_back(void);
bool test_str_push_front(void);
bool test_str_merge_l(void);
bool test_str_merge_r(void);
bool test_str_merge(void);
bool test_str_write_fmt_append(void);

// Test StrInsertR function
bool test_str_insert_char_at(void) {
    WriteFmt("Testing StrInsertR\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    Str s = StrInitFromZstr("Hello", &alloc);

    // Insert a character in the middle
    StrInsertR(&s, '!', 2);

    // Check that the character was inserted correctly
    bool result = (ZstrCompare(StrBegin(&s), "He!llo") == 0);

    // Insert a character at the beginning
    StrInsertR(&s, '?', 0);

    // Check that the character was inserted correctly
    result = result && (ZstrCompare(StrBegin(&s), "?He!llo") == 0);

    // Insert a character at the end
    StrInsertR(&s, '.', StrLen(&s));

    // Check that the character was inserted correctly
    result = result && (ZstrCompare(StrBegin(&s), "?He!llo.") == 0);

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test StrInsertMany 4-arg (Cstr) form
bool test_str_insert_cstr(void) {
    WriteFmt("Testing StrInsertMany (Cstr form)\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    Str s = StrInitFromZstr("Hello", &alloc);

    // Insert a string in the middle: (cstr, cstr_len) adjacent, then idx
    StrInsertMany(&s, " World", 6, 2);

    // Check that the string was inserted correctly
    bool result = (ZstrCompare(StrBegin(&s), "He Worldllo") == 0);

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test StrInsertMany 3-arg Zstr form
bool test_str_insert_zstr(void) {
    WriteFmt("Testing StrInsertMany (Zstr form)\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    Str s = StrInitFromZstr("Hello", &alloc);

    // Insert a string in the middle
    Zstr w = " World";
    StrInsertMany(&s, w, 2);

    // Check that the string was inserted correctly
    bool result = (ZstrCompare(StrBegin(&s), "He Worldllo") == 0);

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test StrInsertMany function
bool test_str_push_cstr(void) {
    WriteFmt("Testing StrInsertMany\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    Str s = StrInitFromZstr("Hello", &alloc);

    // Push a string at position 2
    StrInsertMany(&s, " World", 6, 2);

    // Check that the string was inserted correctly
    bool result = (ZstrCompare(StrBegin(&s), "He Worldllo") == 0);

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test StrInsertMany function
bool test_str_push_zstr(void) {
    WriteFmt("Testing StrInsertMany\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    Str s = StrInitFromZstr("Hello", &alloc);

    // Push a string at position 2
    StrInsertMany(&s, " World", 2);

    // Check that the string was inserted correctly
    bool result = (ZstrCompare(StrBegin(&s), "He Worldllo") == 0);

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test StrPushBackMany function
bool test_str_push_back_cstr(void) {
    WriteFmt("Testing StrPushBackMany\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    Str s = StrInitFromZstr("Hello", &alloc);

    // Push a string at the back
    StrPushBackMany(&s, " World", 6);

    // Check that the string was inserted correctly
    bool result = (ZstrCompare(StrBegin(&s), "Hello World") == 0);

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test StrPushBackMany function
bool test_str_push_back_zstr(void) {
    WriteFmt("Testing StrPushBackMany\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    Str s = StrInitFromZstr("Hello", &alloc);

    // Push a string at the back
    StrPushBackMany(&s, " World");

    // Check that the string was inserted correctly
    bool result = (ZstrCompare(StrBegin(&s), "Hello World") == 0);

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test StrPushFrontMany function
bool test_str_push_front_cstr(void) {
    WriteFmt("Testing StrPushFrontMany\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    Str s = StrInitFromZstr("World", &alloc);

    // Push a string at the front
    StrPushFrontMany(&s, "Hello ", 6);

    // Check that the string was inserted correctly
    bool result = (ZstrCompare(StrBegin(&s), "Hello World") == 0);

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test StrPushFrontMany function
bool test_str_push_front_zstr(void) {
    WriteFmt("Testing StrPushFrontMany\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    Str s = StrInitFromZstr("World", &alloc);

    // Push a string at the front
    StrPushFrontMany(&s, "Hello ");

    // Check that the string was inserted correctly
    bool result = (ZstrCompare(StrBegin(&s), "Hello World") == 0);

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test StrPushBack function
bool test_str_push_back(void) {
    WriteFmt("Testing StrPushBack\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    Str s = StrInitFromZstr("Hello", &alloc);

    // Push characters at the back
    StrPushBackR(&s, ' ');
    StrPushBackR(&s, 'W');
    StrPushBackR(&s, 'o');
    StrPushBackR(&s, 'r');
    StrPushBackR(&s, 'l');
    StrPushBackR(&s, 'd');

    // Check that the characters were inserted correctly
    bool result = (ZstrCompare(StrBegin(&s), "Hello World") == 0);

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test StrPushFront function
bool test_str_push_front(void) {
    WriteFmt("Testing StrPushFront\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    Str s = StrInitFromZstr("World", &alloc);

    // Push characters at the front
    StrPushFrontR(&s, ' ');
    StrPushFrontR(&s, 'o');
    StrPushFrontR(&s, 'l');
    StrPushFrontR(&s, 'l');
    StrPushFrontR(&s, 'e');
    StrPushFrontR(&s, 'H');

    // Check that the characters were inserted correctly
    bool result = (ZstrCompare(StrBegin(&s), "Hello World") == 0);

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test StrMergeL function
bool test_str_merge_l(void) {
    WriteFmt("Testing StrMergeL\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    Str s1 = StrInitFromZstr("Hello", &alloc);
    Str s2 = StrInitFromZstr(" World", &alloc);

    // Merge s2 into s1 (L-value semantics)
    StrMergeL(&s1, &s2);

    // Validate both vectors
    ValidateVec(&s1);
    ValidateVec(&s2);

    // Check that the strings were merged correctly
    bool result = (ZstrCompare(StrBegin(&s1), "Hello World") == 0);

    // Check that s2 was reset - data should be NULL, length should be 0
    result = result && (StrLen(&s2) == 0 && StrBegin(&s2) == NULL);

    StrDeinit(&s1);
    StrDeinit(&s2);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test StrMergeR function
bool test_str_merge_r(void) {
    WriteFmt("Testing StrMergeR\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    Str s1 = StrInitFromZstr("Hello", &alloc);
    Str s2 = StrInitFromZstr(" World", &alloc);

    // Merge s2 into s1 (R-value semantics)
    StrMergeR(&s1, &s2);

    // Check that the strings were merged correctly
    bool result = (ZstrCompare(StrBegin(&s1), "Hello World") == 0);

    // Check that s2 was not reset
    result = result && (StrLen(&s2) == 6 && ZstrCompare(StrBegin(&s2), " World") == 0);

    StrDeinit(&s1);
    StrDeinit(&s2);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test StrMerge function (unsuffixed = L-form per convention; src zeroed).
bool test_str_merge(void) {
    WriteFmt("Testing StrMerge\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    Str s1 = StrInitFromZstr("Hello", &alloc);
    Str s2 = StrInitFromZstr(" World", &alloc);

    // Merge s2 into s1 (L-form; ownership of s2's storage transfers to s1)
    StrMerge(&s1, &s2);

    // Check that the strings were merged correctly
    bool result = (ZstrCompare(StrBegin(&s1), "Hello World") == 0);

    // s2 was zeroed on take per L-form contract
    result = result && (StrLen(&s2) == 0 && StrBegin(&s2) == NULL);

    StrDeinit(&s1);
    StrDeinit(&s2);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test StrAppendFmt as the in-tree replacement for the now-removed
// StrAppendf formatter.
bool test_str_write_fmt_append(void) {
    WriteFmt("Testing StrAppendFmt append\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    Str s = StrInitFromZstr("Hello", &alloc);

    // Append formatted suffix.
    StrAppendFmt(&s, " {} {}", (Zstr) "World", (u32)2023);

    // Check that the string was appended correctly
    bool result = (ZstrCompare(StrBegin(&s), "Hello World 2023") == 0);

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Main function that runs all tests
int main(void) {
    WriteFmt("[INFO] Starting Str.Insert tests\n\n");

    // Array of test functions
    TestFunction tests[] = {
        test_str_insert_char_at,
        test_str_insert_cstr,
        test_str_insert_zstr,
        test_str_push_cstr,
        test_str_push_zstr,
        test_str_push_back_cstr,
        test_str_push_back_zstr,
        test_str_push_front_cstr,
        test_str_push_front_zstr,
        test_str_push_back,
        test_str_push_front,
        test_str_merge_l,
        test_str_merge_r,
        test_str_merge,
        test_str_write_fmt_append
    };

    int total_tests = sizeof(tests) / sizeof(tests[0]);

    // Run all tests using the centralized test driver
    return run_test_suite(tests, total_tests, NULL, 0, "Str.Insert");
}
