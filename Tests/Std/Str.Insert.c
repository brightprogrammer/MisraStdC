#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Log.h>
#include <stdio.h>
#include <string.h>
#include <Misra/Types.h>

// Include test utilities
#include "../Util/TestRunner.h"

// Function prototypes
bool test_str_insert_char_at(void);
bool test_str_insert_cstr(void);
bool test_str_insert_zstr(void);
bool test_str_insert(void);
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

// Test StrInsertCharAt function
bool test_str_insert_char_at(void) {
    WriteFmt("Testing StrInsertCharAt\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    Str s = StrInitFromZstr("Hello", &alloc);

    // Insert a character in the middle
    StrInsertCharAt(&s, '!', 2);

    // Check that the character was inserted correctly
    bool result = (ZstrCompare(s.data, "He!llo") == 0);

    // Insert a character at the beginning
    StrInsertCharAt(&s, '?', 0);

    // Check that the character was inserted correctly
    result = result && (ZstrCompare(s.data, "?He!llo") == 0);

    // Insert a character at the end
    StrInsertCharAt(&s, '.', s.length);

    // Check that the character was inserted correctly
    result = result && (ZstrCompare(s.data, "?He!llo.") == 0);

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test StrInsertCstr function
bool test_str_insert_cstr(void) {
    WriteFmt("Testing StrInsertCstr\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    Str s = StrInitFromZstr("Hello", &alloc);

    // Insert a string in the middle
    StrInsertCstr(&s, " World", 2, 6);

    // Check that the string was inserted correctly
    bool result = (ZstrCompare(s.data, "He Worldllo") == 0);

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test StrInsertZstr function
bool test_str_insert_zstr(void) {
    WriteFmt("Testing StrInsertZstr\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    Str s = StrInitFromZstr("Hello", &alloc);

    // Insert a string in the middle
    StrInsertZstr(&s, " World", 2);

    // Check that the string was inserted correctly
    bool result = (ZstrCompare(s.data, "He Worldllo") == 0);

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test StrInsert function
bool test_str_insert(void) {
    WriteFmt("Testing StrInsert\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    Str s1 = StrInitFromZstr("Hello", &alloc);
    Str s2 = StrInitFromZstr(" World", &alloc);

    // Insert s2 into s1 in the middle
    StrInsert(&s1, &s2, 2);

    // Check that the string was inserted correctly
    bool result = (ZstrCompare(s1.data, "He Worldllo") == 0);

    StrDeinit(&s1);
    StrDeinit(&s2);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test StrPushCstr function
bool test_str_push_cstr(void) {
    WriteFmt("Testing StrPushCstr\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    Str s = StrInitFromZstr("Hello", &alloc);

    // Push a string at position 2
    StrPushCstr(&s, " World", 6, 2);

    // Check that the string was inserted correctly
    bool result = (ZstrCompare(s.data, "He Worldllo") == 0);

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test StrPushZstr function
bool test_str_push_zstr(void) {
    WriteFmt("Testing StrPushZstr\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    Str s = StrInitFromZstr("Hello", &alloc);

    // Push a string at position 2
    StrPushZstr(&s, " World", 2);

    // Check that the string was inserted correctly
    bool result = (ZstrCompare(s.data, "He Worldllo") == 0);

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test StrPushBackCstr function
bool test_str_push_back_cstr(void) {
    WriteFmt("Testing StrPushBackCstr\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    Str s = StrInitFromZstr("Hello", &alloc);

    // Push a string at the back
    StrPushBackCstr(&s, " World", 6);

    // Check that the string was inserted correctly
    bool result = (ZstrCompare(s.data, "Hello World") == 0);

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test StrPushBackZstr function
bool test_str_push_back_zstr(void) {
    WriteFmt("Testing StrPushBackZstr\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    Str s = StrInitFromZstr("Hello", &alloc);

    // Push a string at the back
    StrPushBackZstr(&s, " World");

    // Check that the string was inserted correctly
    bool result = (ZstrCompare(s.data, "Hello World") == 0);

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test StrPushFrontCstr function
bool test_str_push_front_cstr(void) {
    WriteFmt("Testing StrPushFrontCstr\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    Str s = StrInitFromZstr("World", &alloc);

    // Push a string at the front
    StrPushFrontCstr(&s, "Hello ", 6);

    // Check that the string was inserted correctly
    bool result = (ZstrCompare(s.data, "Hello World") == 0);

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test StrPushFrontZstr function
bool test_str_push_front_zstr(void) {
    WriteFmt("Testing StrPushFrontZstr\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    Str s = StrInitFromZstr("World", &alloc);

    // Push a string at the front
    StrPushFrontZstr(&s, "Hello ");

    // Check that the string was inserted correctly
    bool result = (ZstrCompare(s.data, "Hello World") == 0);

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
    StrPushBack(&s, ' ');
    StrPushBack(&s, 'W');
    StrPushBack(&s, 'o');
    StrPushBack(&s, 'r');
    StrPushBack(&s, 'l');
    StrPushBack(&s, 'd');

    // Check that the characters were inserted correctly
    bool result = (ZstrCompare(s.data, "Hello World") == 0);

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
    StrPushFront(&s, ' ');
    StrPushFront(&s, 'o');
    StrPushFront(&s, 'l');
    StrPushFront(&s, 'l');
    StrPushFront(&s, 'e');
    StrPushFront(&s, 'H');

    // Check that the characters were inserted correctly
    bool result = (ZstrCompare(s.data, "Hello World") == 0);

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
    bool result = (ZstrCompare(s1.data, "Hello World") == 0);

    // Check that s2 was reset - data should be NULL, length should be 0
    result = result && (s2.length == 0 && s2.data == NULL);

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
    bool result = (ZstrCompare(s1.data, "Hello World") == 0);

    // Check that s2 was not reset
    result = result && (s2.length == 6 && ZstrCompare(s2.data, " World") == 0);

    StrDeinit(&s1);
    StrDeinit(&s2);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test StrMerge function (alias for StrMergeR)
bool test_str_merge(void) {
    WriteFmt("Testing StrMerge\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    Str s1 = StrInitFromZstr("Hello", &alloc);
    Str s2 = StrInitFromZstr(" World", &alloc);

    // Merge s2 into s1
    StrMerge(&s1, &s2);

    // Check that the strings were merged correctly
    bool result = (ZstrCompare(s1.data, "Hello World") == 0);

    // Check that s2 was not reset (since StrMerge is an alias for StrMergeR)
    result = result && (s2.length == 6 && ZstrCompare(s2.data, " World") == 0);

    StrDeinit(&s1);
    StrDeinit(&s2);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test StrAppendFmt as the in-tree replacement for the now-removed
// StrAppendf printf-style formatter.
bool test_str_write_fmt_append(void) {
    WriteFmt("Testing StrAppendFmt append\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    Str s = StrInitFromZstr("Hello", &alloc);

    // Append formatted suffix.
    StrAppendFmt(&s, " {} {}", (const char *)"World", (u32)2023);

    // Check that the string was appended correctly
    bool result = (ZstrCompare(s.data, "Hello World 2023") == 0);

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
        test_str_insert,
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
