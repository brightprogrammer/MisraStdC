#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Memory.h>
#include <stdio.h>
#include <Misra/Types.h>

// Include test utilities
#include "../Util/TestRunner.h"

// Function prototypes
bool test_str_pop_back(void);
bool test_str_pop_front(void);
bool test_str_remove(void);
bool test_str_remove_range(void);
bool test_str_delete_last_char(void);
bool test_str_delete(void);
bool test_str_delete_range(void);

// Test StrPopBack function
bool test_str_pop_back(void) {
    WriteFmt("Testing StrPopBack\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    Str s = StrInitFromZstr("Hello", &alloc);

    // Pop a character from the back
    char c;
    StrPopBack(&s, &c);

    // Check that the character was popped correctly
    bool result = (c == 'o' && ZstrCompare(s.data, "Hell") == 0);

    // Pop another character without storing it - avoid passing NULL directly
    char ignored;
    StrPopBack(&s, &ignored);

    // Check that the character was removed
    result = result && (ZstrCompare(s.data, "Hel") == 0);

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test StrPopFront function
bool test_str_pop_front(void) {
    WriteFmt("Testing StrPopFront\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    Str s = StrInitFromZstr("Hello", &alloc);

    // Pop a character from the front
    char c;
    StrPopFront(&s, &c);

    // Check that the character was popped correctly
    bool result = (c == 'H' && ZstrCompare(s.data, "ello") == 0);

    // Pop another character without storing it - avoid passing NULL directly
    char ignored;
    StrPopFront(&s, &ignored);

    // Check that the character was removed
    result = result && (ZstrCompare(s.data, "llo") == 0);

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test StrRemove function
bool test_str_remove(void) {
    WriteFmt("Testing StrRemove\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    Str s = StrInitFromZstr("Hello", &alloc);

    // Remove a character from the middle
    char c;
    StrRemove(&s, &c, 2);

    // Check that the character was removed correctly
    bool result = (c == 'l' && ZstrCompare(s.data, "Helo") == 0);

    // Remove another character without storing it - avoid passing NULL directly
    char ignored;
    StrRemove(&s, &ignored, 1);

    // Check that the character was removed
    result = result && (ZstrCompare(s.data, "Hlo") == 0);

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test StrRemoveRange function
bool test_str_remove_range(void) {
    WriteFmt("Testing StrRemoveRange\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    Str s = StrInitFromZstr("Hello World", &alloc);

    // Create a buffer to store the removed characters
    char buffer[6];
    MemSet(buffer, 0, sizeof(buffer));

    // Remove a range of characters
    StrRemoveRange(&s, buffer, 5, 5);

    // Check that the characters were removed correctly
    bool result = (ZstrCompare(buffer, " Worl") == 0 && ZstrCompare(s.data, "Hellod") == 0);

    // Remove another range without storing it - use a temporary buffer instead of NULL
    char ignored[2];
    StrRemoveRange(&s, ignored, 4, 2);

    // Check that the characters were removed
    result = result && (ZstrCompare(s.data, "Hell") == 0);

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test StrDeleteLastChar function
bool test_str_delete_last_char(void) {
    WriteFmt("Testing StrDeleteLastChar\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    Str s = StrInitFromZstr("Hello", &alloc);

    // Delete the last character
    StrDeleteLastChar(&s);

    // Check that the character was deleted
    bool result = (ZstrCompare(s.data, "Hell") == 0);

    // Delete another character
    StrDeleteLastChar(&s);

    // Check that the character was deleted
    result = result && (ZstrCompare(s.data, "Hel") == 0);

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test StrDelete function
bool test_str_delete(void) {
    WriteFmt("Testing StrDelete\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    Str s = StrInitFromZstr("Hello", &alloc);

    // Delete a character from the middle
    StrDelete(&s, 2);

    // Check that the character was deleted
    bool result = (ZstrCompare(s.data, "Helo") == 0);

    // Delete another character
    StrDelete(&s, 1);

    // Check that the character was deleted
    result = result && (ZstrCompare(s.data, "Hlo") == 0);

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test StrDeleteRange function
bool test_str_delete_range(void) {
    WriteFmt("Testing StrDeleteRange\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    Str s = StrInitFromZstr("Hello World", &alloc);

    // Delete a range of characters
    StrDeleteRange(&s, 5, 6);

    // Check that the characters were deleted
    bool result = (ZstrCompare(s.data, "Hello") == 0);

    // Delete another range
    StrDeleteRange(&s, 2, 2);

    // Check that the characters were deleted
    result = result && (ZstrCompare(s.data, "Heo") == 0);

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Main function that runs all tests
int main(void) {
    WriteFmt("[INFO] Starting Str.Remove tests\n\n");

    // Array of test functions
    TestFunction tests[] = {
        test_str_pop_back,
        test_str_pop_front,
        test_str_remove,
        test_str_remove_range,
        test_str_delete_last_char,
        test_str_delete,
        test_str_delete_range
    };

    int total_tests = sizeof(tests) / sizeof(tests[0]);

    // Run all tests using the centralized test driver
    return run_test_suite(tests, total_tests, NULL, 0, "Str.Remove");
}
