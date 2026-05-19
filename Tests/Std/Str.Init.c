#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Allocator/Default.h>
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
bool test_str_z_alias(void);
bool test_str_init_from_str(void);
bool test_str_dup(void);
bool test_str_printf(void);
bool test_str_init_stack(void);
bool test_str_init_copy(void);
bool test_str_clone_inherits_allocator_config(void);
bool test_str_deinit(void);

// Test StrInit function
bool test_str_init(void) {
    WriteFmt("Testing StrInit\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Str s = StrInit(&alloc);

    // Validate the string
    ValidateStr(&s);

    // Check that it's initialized correctly
    // A newly initialized string may have NULL data if capacity is 0
    bool result = (s.length == 0);

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test StrInitFromCstr function
bool test_str_init_from_cstr(void) {
    WriteFmt("Testing StrInitFromCstr\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    const char *test_str = "Hello, World!";
    size_t      len      = 5; // Just "Hello"
    Str         s        = StrInitFromCstr(test_str, len, &alloc);

    // Validate the string
    ValidateStr(&s);

    // Check that it's initialized correctly
    bool result = (s.length == len && ZstrCompareN(s.data, test_str, len) == 0 && s.data[len] == '\0');

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test StrInitFromZstr function
bool test_str_init_from_zstr(void) {
    WriteFmt("Testing StrInitFromZstr\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    const char *test_str = "Hello, World!";
    Str         s        = StrInitFromZstr(test_str, &alloc);

    // Validate the string
    ValidateStr(&s);

    // Check that it's initialized correctly
    bool result = (s.length == ZstrLen(test_str) && ZstrCompare(s.data, test_str) == 0);

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test StrZ alias
bool test_str_z_alias(void) {
    WriteFmt("Testing StrZ\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    const char *test_str = "Alias Test";
    Str         s        = StrZ(test_str, &alloc);

    ValidateStr(&s);

    bool result = (s.length == ZstrLen(test_str) && ZstrCompare(s.data, test_str) == 0);

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test StrInitFromStr function
bool test_str_init_from_str(void) {
    WriteFmt("Testing StrInitFromStr\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Str src = StrInitFromZstr("Hello, World!", &alloc);
    Str dst = StrInitFromStr(&src, &alloc);

    // Validate both strings
    ValidateStr(&src);
    ValidateStr(&dst);

    // Check that dst is initialized correctly
    bool result = (dst.length == src.length && ZstrCompare(dst.data, src.data) == 0);

    StrDeinit(&src);
    StrDeinit(&dst);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test StrDup function (alias for StrInitFromStr)
bool test_str_dup(void) {
    WriteFmt("Testing StrDup\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Str src = StrInitFromZstr("Hello, World!", &alloc);
    Str dst = StrDup(&src, &alloc);

    // Validate both strings
    ValidateStr(&src);
    ValidateStr(&dst);

    // Check that dst is initialized correctly
    bool result = (dst.length == src.length && ZstrCompare(dst.data, src.data) == 0);

    StrDeinit(&src);
    StrDeinit(&dst);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test StrAppendFmt function
bool test_str_WriteFmt(void) {
    WriteFmt("Testing StrAppendFmt\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Str s = StrInit(&alloc);
    StrAppendFmt(&s, "Hello, {}!", &"World"[0]);

    // Validate the string
    ValidateStr(&s);

    // Check that it's initialized correctly
    bool result = (ZstrCompare(s.data, "Hello, World!") == 0);

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test StrInitStack macro
bool test_str_init_stack(void) {
    WriteFmt("Testing StrInitStack\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    bool result = true;

    // Test with the actual StrInitStack macro
    Str stack_str;
    StrInitStack(stack_str, &alloc, 20, {
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

    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test StrInitCopy function
bool test_str_init_copy(void) {
    WriteFmt("Testing StrInitCopy\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Str src = StrInitFromZstr("Hello, World!", &alloc);
    Str dst = StrInit(&alloc);

    // Copy src to dst
    bool success = StrInitCopy(&dst, &src);

    // Validate both strings
    ValidateStr(&src);
    ValidateStr(&dst);

    // Check that the copy was successful
    bool result = (success && dst.length == src.length && ZstrCompare(dst.data, src.data) == 0);

    StrDeinit(&src);
    StrDeinit(&dst);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test that Str clones inherit the source allocator pointer
bool test_str_clone_inherits_allocator_config(void) {
    WriteFmt("Testing Str clone allocator inheritance\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Str src = StrInitFromCstr("Hello, World!", ZstrLen("Hello, World!"), &alloc);

    Str dup = StrInitFromStr(&src, &alloc);
    Str dst = StrInit(&alloc);
    bool copied = StrInitCopy(&dst, &src);

    ValidateStr(&src);
    ValidateStr(&dup);
    ValidateStr(&dst);

    bool dup_allocator_matches = (dup.allocator == src.allocator);
    bool dst_allocator_matches = copied && (dst.allocator == src.allocator);

    bool result =
        copied &&
        dup.length == src.length &&
        dst.length == src.length &&
        ZstrCompare(dup.data, src.data) == 0 &&
        ZstrCompare(dst.data, src.data) == 0 &&
        dup_allocator_matches &&
        dst_allocator_matches;

    StrDeinit(&src);
    StrDeinit(&dup);
    StrDeinit(&dst);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test StrDeinit function
bool test_str_deinit(void) {
    WriteFmt("Testing StrDeinit\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Str s = StrInitFromZstr("Hello, World!", &alloc);

    // Validate the string before deinit
    ValidateStr(&s);

    // Deinit the string
    StrDeinit(&s);

    // Check that the string is deinited correctly
    // Note: We can't really check much here, as the memory is freed
    // The best we can do is make sure we don't crash

    DefaultAllocatorDeinit(&alloc);
    return true;
}

// Main function that runs all tests
int main(void) {
    WriteFmt("[INFO] Starting Str.Init tests\n\n");

    // Array of test functions
    TestFunction tests[] = {
        test_str_init,
        test_str_init_from_cstr,
        test_str_init_from_zstr,
        test_str_z_alias,
        test_str_init_from_str,
        test_str_dup,
        test_str_WriteFmt,
        test_str_init_stack,
        test_str_init_copy,
        test_str_clone_inherits_allocator_config,
        test_str_deinit
    };

    int total_tests = sizeof(tests) / sizeof(tests[0]);

    // Run all tests using the centralized test driver
    return run_test_suite(tests, total_tests, NULL, 0, "Str.Init");
}
