#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Log.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

// Function prototypes
bool test_str_init(void);
bool test_str_init_from_zstr(void);
bool test_str_init_from_cstr(void);
bool test_str_init_copy(void);
bool test_str_push_back(void);
bool test_str_push_back_zstr(void);
bool test_str_cmp(void);

// Test string initialization
bool test_str_init(void) {
    printf("Testing StrInit\n");
    
    Str s = StrInit();
    
    // Use ValidateStr to check if the string is valid
    bool result = true;
    ValidateStr(&s);  // This will assert if the string is invalid
    
    // Also check that the string is empty
    result = (s.length == 0);
    
    StrDeinit(&s);
    return result;
}

// Test string initialization from C string
bool test_str_init_from_zstr(void) {
    printf("Testing StrInitFromZstr\n");
    
    const char* test_str = "Hello, World!";
    Str s = StrInitFromZstr(test_str);
    
    ValidateStr(&s);  // Validate the string
    
    bool result = (s.length == strlen(test_str) && 
                  ZstrCompare(s.data, test_str) == 0);
    
    StrDeinit(&s);
    return result;
}

// Test string initialization from C string with length
bool test_str_init_from_cstr(void) {
    printf("Testing StrInitFromCstr\n");
    
    const char* test_str = "Hello, World!";
    size_t len = 5; // Just "Hello"
    Str s = StrInitFromCstr(test_str, len);
    
    ValidateStr(&s);  // Validate the string
    
    bool result = (s.length == len && 
                  ZstrCompareN(s.data, test_str, len) == 0 &&
                  s.data[len] == '\0');
    
    StrDeinit(&s);
    return result;
}

// Test string copy initialization
bool test_str_init_copy(void) {
    printf("Testing StrInitCopy\n");
    
    Str src = StrInitFromZstr("Hello, World!");
    ValidateStr(&src);  // Validate source string
    
    Str dst = StrInit();
    bool success = StrInitCopy(&dst, &src);
    
    ValidateStr(&dst);  // Validate destination string
    
    bool result = (success && 
                  dst.length == src.length && 
                  ZstrCompare(dst.data, src.data) == 0);
    
    StrDeinit(&src);
    StrDeinit(&dst);
    return result;
}

// Test string push back
bool test_str_push_back(void) {
    printf("Testing StrPushBack\n");
    
    Str s = StrInit();
    ValidateStr(&s);  // Validate initial string
    
    StrPushBack(&s, 'H');
    StrPushBack(&s, 'e');
    StrPushBack(&s, 'l');
    StrPushBack(&s, 'l');
    StrPushBack(&s, 'o');
    
    ValidateStr(&s);  // Validate after modifications
    
    bool result = (s.length == 5 && 
                  ZstrCompare(s.data, "Hello") == 0);
    
    StrDeinit(&s);
    return result;
}

// Test string push back C string
bool test_str_push_back_zstr(void) {
    printf("Testing StrPushBackZstr\n");
    
    Str s = StrInit();
    ValidateStr(&s);  // Validate initial string
    
    StrPushBackZstr(&s, "Hello");
    ValidateStr(&s);  // Validate after first append
    
    StrPushBackZstr(&s, ", ");
    StrPushBackZstr(&s, "World!");
    
    ValidateStr(&s);  // Validate after all modifications
    
    bool result = (s.length == 13 && 
                  ZstrCompare(s.data, "Hello, World!") == 0);
    
    StrDeinit(&s);
    return result;
}

// Test string comparison
bool test_str_cmp(void) {
    printf("Testing StrCmp\n");
    
    Str s1 = StrInitFromZstr("Hello");
    Str s2 = StrInitFromZstr("Hello");
    Str s3 = StrInitFromZstr("World");
    
    ValidateStr(&s1);  // Validate all strings
    ValidateStr(&s2);
    ValidateStr(&s3);
    
    bool result = (StrCmp(&s1, &s2) == 0 && 
                  StrCmp(&s1, &s3) < 0 &&
                  StrCmp(&s3, &s1) > 0);
    
    StrDeinit(&s1);
    StrDeinit(&s2);
    StrDeinit(&s3);
    return result;
}

// Main function that runs all tests
int main(void) {
    printf("[INFO] Starting Str tests\n\n");
    
    // Array of test functions
    bool (*tests[])(void) = {
        test_str_init,
        test_str_init_from_zstr,
        test_str_init_from_cstr,
        test_str_init_copy,
        test_str_push_back,
        test_str_push_back_zstr,
        test_str_cmp
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
