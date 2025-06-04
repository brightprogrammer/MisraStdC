#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Log.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <Misra/Types.h>

// Include test utilities
#include "../Util/TestRunner.h"

// Function prototypes
bool test_str_first(void);
bool test_str_last(void);
bool test_str_begin(void);
bool test_str_end(void);
bool test_str_char_at(void);
bool test_str_char_ptr_at(void);

// Test StrFirst function
bool test_str_first(void) {
    printf("Testing StrFirst\n");

    Str s = StrInitFromZstr("Hello");

    // Get the first character
    char first = StrFirst(&s);

    // Check that the character is correct
    bool result = (first == 'H');

    StrDeinit(&s);
    return result;
}

// Test StrLast function
bool test_str_last(void) {
    printf("Testing StrLast\n");

    Str s = StrInitFromZstr("Hello");

    // Get the last character
    char last = StrLast(&s);

    // Check that the character is correct
    bool result = (last == 'o');

    StrDeinit(&s);
    return result;
}

// Test StrBegin function
bool test_str_begin(void) {
    printf("Testing StrBegin\n");

    Str s = StrInitFromZstr("Hello");

    // Get a pointer to the first character using StrBegin
    char* begin = StrBegin(&s);

    // Check that the pointer is correct
    bool result = (begin == s.data && *begin == 'H');

    StrDeinit(&s);
    return result;
}

// Test StrEnd function
bool test_str_end(void) {
    printf("Testing StrEnd\n");

    Str s = StrInitFromZstr("Hello");

    // Get a pointer to one past the last character using StrEnd
    char* end = StrEnd(&s);

    // Check that the pointer is correct
    bool result = (end == s.data + s.length && *end == '\0');

    StrDeinit(&s);
    return result;
}

// Test StrCharAt function
bool test_str_char_at(void) {
    printf("Testing StrCharAt\n");

    Str s = StrInitFromZstr("Hello");

    // Access characters at different indices
    // Now using the fixed StrCharAt macro
    char c0 = StrCharAt(&s, 0);
    char c1 = StrCharAt(&s, 1);
    char c2 = StrCharAt(&s, 2);
    char c3 = StrCharAt(&s, 3);
    char c4 = StrCharAt(&s, 4);

    // Check that the characters are correct
    bool result = (c0 == 'H' && c1 == 'e' && c2 == 'l' && c3 == 'l' && c4 == 'o');

    StrDeinit(&s);
    return result;
}

// Test StrCharPtrAt function
bool test_str_char_ptr_at(void) {
    printf("Testing StrCharPtrAt\n");

    Str s = StrInitFromZstr("Hello");

    // Access character pointers at different indices
    // Now using the fixed StrCharPtrAt macro
    char* p0 = StrCharPtrAt(&s, 0);
    char* p1 = StrCharPtrAt(&s, 1);
    char* p2 = StrCharPtrAt(&s, 2);
    char* p3 = StrCharPtrAt(&s, 3);
    char* p4 = StrCharPtrAt(&s, 4);

    // Check that the pointers are correct
    bool result = (*p0 == 'H' && *p1 == 'e' && *p2 == 'l' && *p3 == 'l' && *p4 == 'o');

    // Also check that the pointers are at the expected positions
    result = result && (p0 == s.data && p1 == s.data + 1 && p2 == s.data + 2 && p3 == s.data + 3 && p4 == s.data + 4);

    StrDeinit(&s);
    return result;
}

// Main function that runs all tests
int main(void) {
    printf("[INFO] Starting Str.Access tests\n\n");

    // Array of test functions
    TestFunction tests[] = {
        test_str_first, 
        test_str_last, 
        test_str_begin, 
        test_str_end, 
        test_str_char_at, 
        test_str_char_ptr_at
    };

    int total_tests = sizeof(tests) / sizeof(tests[0]);

    // Run all tests using the centralized test driver
    return run_test_suite(tests, total_tests, NULL, 0, "Str.Access");
}
