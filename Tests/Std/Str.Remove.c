#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Log.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

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
    printf("Testing StrPopBack\n");
    
    Str s = StrInitFromZstr("Hello");
    
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
    return result;
}

// Test StrPopFront function
bool test_str_pop_front(void) {
    printf("Testing StrPopFront\n");
    
    Str s = StrInitFromZstr("Hello");
    
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
    return result;
}

// Test StrRemove function
bool test_str_remove(void) {
    printf("Testing StrRemove\n");
    
    Str s = StrInitFromZstr("Hello");
    
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
    return result;
}

// Test StrRemoveRange function
bool test_str_remove_range(void) {
    printf("Testing StrRemoveRange\n");
    
    Str s = StrInitFromZstr("Hello World");
    
    // Create a buffer to store the removed characters
    char buffer[6];
    memset(buffer, 0, sizeof(buffer));
    
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
    return result;
}

// Test StrDeleteLastChar function
bool test_str_delete_last_char(void) {
    printf("Testing StrDeleteLastChar\n");
    
    Str s = StrInitFromZstr("Hello");
    
    // Delete the last character
    StrDeleteLastChar(&s);
    
    // Check that the character was deleted
    bool result = (ZstrCompare(s.data, "Hell") == 0);
    
    // Delete another character
    StrDeleteLastChar(&s);
    
    // Check that the character was deleted
    result = result && (ZstrCompare(s.data, "Hel") == 0);
    
    StrDeinit(&s);
    return result;
}

// Test StrDelete function
bool test_str_delete(void) {
    printf("Testing StrDelete\n");
    
    Str s = StrInitFromZstr("Hello");
    
    // Delete a character from the middle
    StrDelete(&s, 2);
    
    // Check that the character was deleted
    bool result = (ZstrCompare(s.data, "Helo") == 0);
    
    // Delete another character
    StrDelete(&s, 1);
    
    // Check that the character was deleted
    result = result && (ZstrCompare(s.data, "Hlo") == 0);
    
    StrDeinit(&s);
    return result;
}

// Test StrDeleteRange function
bool test_str_delete_range(void) {
    printf("Testing StrDeleteRange\n");
    
    Str s = StrInitFromZstr("Hello World");
    
    // Delete a range of characters
    StrDeleteRange(&s, 5, 6);
    
    // Check that the characters were deleted
    bool result = (ZstrCompare(s.data, "Hello") == 0);
    
    // Delete another range
    StrDeleteRange(&s, 2, 2);
    
    // Check that the characters were deleted
    result = result && (ZstrCompare(s.data, "Heo") == 0);
    
    StrDeinit(&s);
    return result;
}

// Main function that runs all tests
int main(void) {
    printf("[INFO] Starting Str.Remove tests\n\n");
    
    // Array of test functions
    bool (*tests[])(void) = {
        test_str_pop_back,
        test_str_pop_front,
        test_str_remove,
        test_str_remove_range,
        test_str_delete_last_char,
        test_str_delete,
        test_str_delete_range
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
