#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Log.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

// Function prototypes
bool test_str_try_reduce_space(void);
bool test_str_swap_char_at(void);
bool test_str_resize(void);
bool test_str_reserve(void);
bool test_str_clear(void);
bool test_str_reverse(void);

// Test StrTryReduceSpace function
bool test_str_try_reduce_space(void) {
    printf("Testing StrTryReduceSpace\n");
    
    Str s = StrInit();
    
    // Reserve more space than needed
    StrReserve(&s, 100);
    
    // Add some data
    StrPushBackZstr(&s, "Hello");
    
    // Original capacity should be at least 100
    bool result = (s.capacity >= 100);
    
    // Try to reduce space
    StrTryReduceSpace(&s);
    
    // Capacity should now be closer to the actual length
    result = result && (s.capacity < 100) && (s.capacity >= s.length);
    
    StrDeinit(&s);
    return result;
}

// Test StrSwapCharAt function
bool test_str_swap_char_at(void) {
    printf("Testing StrSwapCharAt\n");
    
    Str s = StrInitFromZstr("Hello");
    
    // Swap 'H' and 'o'
    StrSwapCharAt(&s, 0, 4);
    
    // Check that the characters were swapped
    bool result = (s.data[0] == 'o' && s.data[4] == 'H');
    
    // Swap 'e' and 'l'
    StrSwapCharAt(&s, 1, 2);
    
    // Check that the characters were swapped
    result = result && (s.data[1] == 'l' && s.data[2] == 'e');
    
    StrDeinit(&s);
    return result;
}

// Test StrResize function
bool test_str_resize(void) {
    printf("Testing StrResize\n");
    
    Str s = StrInitFromZstr("Hello");
    
    // Initial length should be 5
    bool result = (s.length == 5);
    
    // Resize to a smaller length
    StrResize(&s, 3);
    
    // Length should now be 3 and content should be "Hel"
    result = result && (s.length == 3) && (ZstrCompareN(s.data, "Hel", 3) == 0);
    
    // Resize to a larger length
    StrResize(&s, 8);
    
    // Length should now be 8, and the first 3 characters should still be "Hel"
    // The rest will be filled with zeros
    result = result && (s.length == 8) && (ZstrCompareN(s.data, "Hel", 3) == 0);
    
    StrDeinit(&s);
    return result;
}

// Test StrReserve function
bool test_str_reserve(void) {
    printf("Testing StrReserve\n");
    
    Str s = StrInit();
    
    // Reserve more space
    StrReserve(&s, 100);
    
    // Capacity should now be at least 100
    bool result = (s.capacity >= 100);
    
    // Length should still be 0
    result = result && (s.length == 0);
    
    // Reserve less space (should be a no-op)
    StrReserve(&s, 50);
    
    // Capacity should still be at least 100
    result = result && (s.capacity >= 100);
    
    StrDeinit(&s);
    return result;
}

// Test StrClear function
bool test_str_clear(void) {
    printf("Testing StrClear\n");
    
    Str s = StrInitFromZstr("Hello, World!");
    
    // Initial length should be 13
    bool result = (s.length == 13);
    
    // Clear the string
    StrClear(&s);
    
    // Length should now be 0, but capacity should remain
    result = result && (s.length == 0) && (s.capacity >= 13);
    
    // Data pointer should still be valid
    result = result && (s.data != NULL);
    
    StrDeinit(&s);
    return result;
}

// Test StrReverse function
bool test_str_reverse(void) {
    printf("Testing StrReverse\n");
    
    Str s = StrInitFromZstr("Hello");
    
    // Reverse the string
    StrReverse(&s);
    
    // Check that the string was reversed
    bool result = (ZstrCompare(s.data, "olleH") == 0);
    
    // Test with an even-length string
    StrDeinit(&s);
    s = StrInitFromZstr("abcd");
    
    // Reverse the string
    StrReverse(&s);
    
    // Check that the string was reversed
    result = result && (ZstrCompare(s.data, "dcba") == 0);
    
    // Test with a single-character string
    StrDeinit(&s);
    s = StrInitFromZstr("a");
    
    // Reverse the string
    StrReverse(&s);
    
    // Check that the string is unchanged
    result = result && (ZstrCompare(s.data, "a") == 0);
    
    // Test with an empty string
    StrDeinit(&s);
    s = StrInit();
    
    // Reverse the string
    StrReverse(&s);
    
    // Check that the string is still empty
    result = result && (s.length == 0);
    
    StrDeinit(&s);
    return result;
}

// Main function that runs all tests
int main(void) {
    printf("[INFO] Starting Str.Memory tests\n\n");
    
    // Array of test functions
    bool (*tests[])(void) = {
        test_str_try_reduce_space,
        test_str_swap_char_at,
        test_str_resize,
        test_str_reserve,
        test_str_clear,
        test_str_reverse
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

