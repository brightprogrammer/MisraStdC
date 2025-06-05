#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Log.h>
#include <stdio.h>
#include <string.h>
#include <Misra/Types.h>

// Include test utilities
#include "../Util/TestRunner.h"

// Function prototypes
bool test_str_init(void);
bool test_str_init_from_cstr(void);
bool test_str_init_from_zstr(void);
bool test_str_init_from_str(void);
bool test_str_dup(void);
bool test_str_printf(void);
bool test_str_init_stack(void);
bool test_str_init_copy(void);
bool test_str_deinit(void);

// Test StrInit function
bool test_str_init(void) {
    printf("Testing StrInit\n");

    Str s = StrInit();

    // Validate the string
    ValidateStr(&s);

    // Check that it's initialized correctly
    // A newly initialized string may have NULL data if capacity is 0
    bool result = (s.length == 0);

    StrDeinit(&s);
    return result;
}

// Test StrInitFromCstr function
bool test_str_init_from_cstr(void) {
    printf("Testing StrInitFromCstr\n");

    const char* test_str = "Hello, World!";
    size_t      len      = 5; // Just "Hello"
    Str         s        = StrInitFromCstr(test_str, len);

    // Validate the string
    ValidateStr(&s);

    // Check that it's initialized correctly
    bool result = (s.length == len && ZstrCompareN(s.data, test_str, len) == 0 && s.data[len] == '\0');

    StrDeinit(&s);
    return result;
}

// Test StrInitFromZstr function
bool test_str_init_from_zstr(void) {
    printf("Testing StrInitFromZstr\n");

    const char* test_str = "Hello, World!";
    Str         s        = StrInitFromZstr(test_str);

    // Validate the string
    ValidateStr(&s);

    // Check that it's initialized correctly
    bool result = (s.length == strlen(test_str) && ZstrCompare(s.data, test_str) == 0);

    StrDeinit(&s);
    return result;
}

// Test StrInitFromStr function
bool test_str_init_from_str(void) {
    printf("Testing StrInitFromStr\n");

    Str src = StrInitFromZstr("Hello, World!");
    Str dst = StrInitFromStr(&src);

    // Validate both strings
    ValidateStr(&src);
    ValidateStr(&dst);

    // Check that dst is initialized correctly
    bool result = (dst.length == src.length && ZstrCompare(dst.data, src.data) == 0);

    StrDeinit(&src);
    StrDeinit(&dst);
    return result;
}

// Test StrDup function (alias for StrInitFromStr)
bool test_str_dup(void) {
    printf("Testing StrDup\n");

    Str src = StrInitFromZstr("Hello, World!");
    Str dst = StrDup(&src);

    // Validate both strings
    ValidateStr(&src);
    ValidateStr(&dst);

    // Check that dst is initialized correctly
    bool result = (dst.length == src.length && ZstrCompare(dst.data, src.data) == 0);

    StrDeinit(&src);
    StrDeinit(&dst);
    return result;
}

// Test StrPrintf function
bool test_str_printf(void) {
    printf("Testing StrPrintf\n");

    Str s = StrInit();
    StrPrintf(&s, "Hello, %s!", "World");

    // Validate the string
    ValidateStr(&s);

    // Check that it's initialized correctly
    bool result = (ZstrCompare(s.data, "Hello, World!") == 0);

    StrDeinit(&s);
    return result;
}

// Test StrInitStack macro
bool test_str_init_stack(void) {
    printf("Testing StrInitStack\n");

    bool result = true;

    // Test with the actual StrInitStack macro
    Str stack_str;
    StrInitStack(stack_str, 20, {
        // Inside the scope where the stack string is valid
        StrPushBackZstr(&stack_str, "Hello, Stack!");

        // Validate the string
        ValidateStr(&stack_str);

        // Check that it works correctly
        if (ZstrCompare(stack_str.data, "Hello, Stack!") != 0) {
            result = false;
        }

        // Check capacity is as expected
        if (stack_str.capacity != 20) {
            result = false;
        }
    });

    // After the scope, stack_str should be zeroed out
    if (stack_str.data != NULL || stack_str.length != 0 || stack_str.capacity != 0) {
        result = false;
    }

    return result;
}

// Test StrInitCopy function
bool test_str_init_copy(void) {
    printf("Testing StrInitCopy\n");

    Str src = StrInitFromZstr("Hello, World!");
    Str dst = StrInit();

    // Copy src to dst
    bool success = StrInitCopy(&dst, &src);

    // Validate both strings
    ValidateStr(&src);
    ValidateStr(&dst);

    // Check that the copy was successful
    bool result = (success && dst.length == src.length && ZstrCompare(dst.data, src.data) == 0);

    StrDeinit(&src);
    StrDeinit(&dst);
    return result;
}

// Test StrDeinit function
bool test_str_deinit(void) {
    printf("Testing StrDeinit\n");

    Str s = StrInitFromZstr("Hello, World!");

    // Validate the string before deinit
    ValidateStr(&s);

    // Deinit the string
    StrDeinit(&s);

    // Check that the string is deinited correctly
    // Note: We can't really check much here, as the memory is freed
    // The best we can do is make sure we don't crash

    return true;
}

// Main function that runs all tests
int main(void) {
    printf("[INFO] Starting Str.Init tests\n\n");

    // Array of test functions
    TestFunction tests[] = {
        test_str_init,
        test_str_init_from_cstr,
        test_str_init_from_zstr,
        test_str_init_from_str,
        test_str_dup,
        test_str_printf,
        test_str_init_stack,
        test_str_init_copy,
        test_str_deinit
    };

    int total_tests = sizeof(tests) / sizeof(tests[0]);

    // Run all tests using the centralized test driver
    return run_test_suite(tests, total_tests, NULL, 0, "Str.Init");
}
