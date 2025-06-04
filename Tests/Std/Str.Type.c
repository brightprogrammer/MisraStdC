#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Log.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

// Include test utilities for deadend testing
#include "../Util/TestRunner.h"

// Function prototypes
bool test_str_type(void);
bool test_strs_type(void);
bool test_validate_str(void);
bool test_validate_strs(void);

// Deadend test prototypes (tests that should crash/abort)
bool test_validate_invalid_str(void);
bool test_validate_invalid_strs(void);

// Test Str type definition
bool test_str_type(void) {
    printf("Testing Str type definition\n");

    // Create a Str object
    Str s = StrInit();

    // Check that it behaves like a Vec of chars
    StrPushBack(&s, 'H');
    StrPushBack(&s, 'e');
    StrPushBack(&s, 'l');
    StrPushBack(&s, 'l');
    StrPushBack(&s, 'o');

    bool result = (s.length == 5 && ZstrCompare(s.data, "Hello") == 0);

    StrDeinit(&s);
    return result;
}

// Test Strs type definition
bool test_strs_type(void) {
    printf("Testing Strs type definition\n");

    // Create a Strs object (vector of strings)
    Strs sv = VecInitWithDeepCopy(NULL, StrDeinit);

    // Add some strings
    Str s1 = StrInitFromZstr("Hello");
    Str s2 = StrInitFromZstr("World");

    VecPushBack(&sv, s1);
    VecPushBack(&sv, s2);

    // Check that it behaves like a Vec of Str objects
    bool result = (sv.length == 2);

    // Check the content of the strings
    if (result) {
        Str* str1 = &VecAt(&sv, 0);
        Str* str2 = &VecAt(&sv, 1);

        result = result && (ZstrCompare(str1->data, "Hello") == 0);
        result = result && (ZstrCompare(str2->data, "World") == 0);
    }

    VecDeinit(&sv); // This should call StrDeinit on each element
    return result;
}

// Test ValidateStr macro
bool test_validate_str(void) {
    printf("Testing ValidateStr macro\n");

    // Create a valid Str
    Str s = StrInit();

    // This should not crash
    ValidateStr(&s);

    // Note: We can't really test invalid strings here as ValidateStr
    // will abort the program if the string is invalid. In a real test
    // framework, we would use something like a death test for this.

    bool result = true; // If we got here, the validation didn't crash

    StrDeinit(&s);
    return result;
}

// Test ValidateStrs macro
bool test_validate_strs(void) {
    printf("Testing ValidateStrs macro\n");

    // Create a valid Strs
    Strs sv = VecInit();

    // This should not crash
    ValidateStrs(&sv);

    // Note: We can't really test invalid Strs objects here as ValidateStrs
    // will abort the program if the object is invalid.

    bool result = true; // If we got here, the validation didn't crash

    VecDeinit(&sv);
    return result;
}

// Deadend test: Test ValidateStr with invalid string (should crash/abort)
bool test_validate_invalid_str(void) {
    printf("Testing ValidateStr with invalid string (should abort)\n");

    // Create an invalid Str by corrupting its fields
    Str s = StrInit();

    // Corrupt the string to make it invalid
    s.length   = 100; // Set length much larger than actual capacity
    s.capacity = 5;   // Small capacity
    // s.data remains valid but length/capacity are inconsistent

    // This should abort the program
    ValidateStr(&s);

    // Should never reach here
    return false;
}

// Deadend test: Test ValidateStrs with invalid Strs (should crash/abort)
bool test_validate_invalid_strs(void) {
    printf("Testing ValidateStrs with invalid Strs (should abort)\n");

    // Create an invalid Strs by corrupting its fields
    Strs sv = VecInit();

    // Corrupt the vector to make it invalid
    sv.length   = 50; // Set length much larger than actual capacity
    sv.capacity = 2;  // Small capacity
    // sv.data remains valid but length/capacity are inconsistent

    // This should abort the program
    ValidateStrs(&sv);

    // Should never reach here
    return false;
}

// Main function that runs all tests
int main(void) {
    printf("[INFO] Starting Str.Type tests\n\n");

    // Array of normal test functions
    TestFunction tests[] = {test_str_type, test_strs_type, test_validate_str, test_validate_strs};

    // Array of deadend test functions (tests that should crash/abort)
    TestFunction deadend_tests[] = {test_validate_invalid_str, test_validate_invalid_strs};

    int total_tests   = sizeof(tests) / sizeof(tests[0]);
    int deadend_count = sizeof(deadend_tests) / sizeof(deadend_tests[0]);

    // Run all tests using the centralized test driver
    return run_test_suite(tests, total_tests, deadend_tests, deadend_count, "Str.Type");
}
