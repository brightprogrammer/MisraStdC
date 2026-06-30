#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Log.h>

// Include test utilities
#include "../../Util/TestRunner.h"

// Function prototypes
bool test_str_try_reduce_space(void);
bool test_str_swap_char_at(void);
bool test_str_resize(void);
bool test_str_reserve(void);
bool test_str_clear(void);
bool test_str_reverse(void);

// Test StrTryReduceSpace function
bool test_str_try_reduce_space(void) {
    WriteFmt("Testing StrTryReduceSpace\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    Str s = StrInit(&alloc);

    // Reserve more space than needed
    StrReserve(&s, 100);

    // Add some data
    StrPushBackMany(&s, "Hello");

    // Original capacity should be at least 100
    bool result = (StrCapacity(&s) >= 100);

    // Try to reduce space
    StrTryReduceSpace(&s);

    // Capacity should now be closer to the actual length
    result = result && (StrCapacity(&s) < 100) && (StrCapacity(&s) >= StrLen(&s));

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test StrSwapCharAt function
bool test_str_swap_char_at(void) {
    WriteFmt("Testing StrSwapCharAt\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    Str s = StrInitFromZstr("Hello", &alloc);

    // Swap 'H' and 'o'
    StrSwapCharAt(&s, 0, 4);

    // Check that the characters were swapped
    bool result = (StrBegin(&s)[0] == 'o' && StrBegin(&s)[4] == 'H');

    // Swap 'e' and 'l'
    StrSwapCharAt(&s, 1, 2);

    // Check that the characters were swapped
    result = result && (StrBegin(&s)[1] == 'l' && StrBegin(&s)[2] == 'e');

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test StrResize function
bool test_str_resize(void) {
    WriteFmt("Testing StrResize\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    Str s = StrInitFromZstr("Hello", &alloc);

    // Initial length should be 5
    bool result = (StrLen(&s) == 5);

    // Resize to a smaller length
    StrResize(&s, 3);

    // Length should now be 3 and content should be "Hel"
    result = result && (StrLen(&s) == 3) && (ZstrCompareN(StrBegin(&s), "Hel", 3) == 0);

    // Resize to a larger length
    StrResize(&s, 8);

    // Length should now be 8, and the first 3 characters should still be "Hel"
    // The rest will be filled with zeros
    result = result && (StrLen(&s) == 8) && (ZstrCompareN(StrBegin(&s), "Hel", 3) == 0);

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test StrReserve function
bool test_str_reserve(void) {
    WriteFmt("Testing StrReserve\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    Str s = StrInit(&alloc);

    // Reserve more space
    StrReserve(&s, 100);

    // Capacity should now be at least 100
    bool result = (StrCapacity(&s) >= 100);

    // Length should still be 0
    result = result && (StrLen(&s) == 0);

    // Reserve less space (should be a no-op)
    StrReserve(&s, 50);

    // Capacity should still be at least 100
    result = result && (StrCapacity(&s) >= 100);

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test StrClear function
bool test_str_clear(void) {
    WriteFmt("Testing StrClear\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    Str s = StrInitFromZstr("Hello, World!", &alloc);

    // Initial length should be 13
    bool result = (StrLen(&s) == 13);

    // Clear the string
    StrClear(&s);

    // Length should now be 0, but capacity should remain
    result = result && (StrLen(&s) == 0) && (StrCapacity(&s) >= 13);

    // Data pointer should still be valid
    result = result && (StrBegin(&s) != NULL);

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test StrReverse function
bool test_str_reverse(void) {
    WriteFmt("Testing StrReverse\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    Str s = StrInitFromZstr("Hello", &alloc);

    // Reverse the string
    StrReverse(&s);

    // Check that the string was reversed
    bool result = (ZstrCompare(StrBegin(&s), "olleH") == 0);

    // Test with an even-length string
    StrDeinit(&s);
    s = StrInitFromZstr("abcd", &alloc);

    // Reverse the string
    StrReverse(&s);

    // Check that the string was reversed
    result = result && (ZstrCompare(StrBegin(&s), "dcba") == 0);

    // Test with a single-character string
    StrDeinit(&s);
    s = StrInitFromZstr("a", &alloc);

    // Reverse the string
    StrReverse(&s);

    // Check that the string is unchanged
    result = result && (ZstrCompare(StrBegin(&s), "a") == 0);

    // Test with an empty string
    StrDeinit(&s);
    s = StrInit(&alloc);

    // Reverse the string
    StrReverse(&s);

    // Check that the string is still empty
    result = result && (StrLen(&s) == 0);

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Main function that runs all tests
int main(void) {
    WriteFmt("[INFO] Starting Str.Memory tests\n\n");

    // Array of test functions
    TestFunction tests[] = {
        test_str_try_reduce_space,
        test_str_swap_char_at,
        test_str_resize,
        test_str_reserve,
        test_str_clear,
        test_str_reverse
    };

    int total_tests = sizeof(tests) / sizeof(tests[0]);

    // Run all tests using the centralized test driver
    return run_test_suite(tests, total_tests, NULL, 0, "Str.Memory");
}
