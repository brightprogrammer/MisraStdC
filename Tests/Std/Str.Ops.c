#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Log.h>

#include <stdio.h>
#include <string.h>
#include <Misra/Types.h>

// Include test utilities
#include "../Util/TestRunner.h"

// Function prototypes
bool test_str_cmp(void);
bool test_str_find(void);
bool test_str_starts_ends_with(void);
bool test_str_replace(void);
bool test_str_split(void);
bool test_str_strip(void);
bool test_str_contains_index(void);

// Test string comparison functions
bool test_str_cmp(void) {
    WriteFmt("Testing StrCmp and StrCmpCstr\n");

    Str s1 = StrInitFromZstr("Hello");
    Str s2 = StrInitFromZstr("Hello");
    Str s3 = StrInitFromZstr("World");
    Str s4 = StrInitFromZstr("Hello World");

    // Test StrCmp with equal strings
    int  cmp1   = StrCmp(&s1, &s2);
    bool result = (cmp1 == 0);

    // Test StrCmp with different strings (H < W in ASCII)
    int cmp2 = StrCmp(&s1, &s3);
    result   = result && (cmp2 < 0);

    // Test StrCmp with string prefix - ZstrCompare compares the entire strings
    int cmp3 = StrCmp(&s1, &s4);
    result   = result && (cmp3 < 0); // "Hello" comes before "Hello World" lexicographically

    // Test StrCmpCstr
    int cmp4 = StrCmpCstr(&s1, "Hello", 5);
    result   = result && (cmp4 == 0);

    int cmp5 = StrCmpCstr(&s1, "World", 5);
    result   = result && (cmp5 != 0);

    StrDeinit(&s1);
    StrDeinit(&s2);
    StrDeinit(&s3);
    StrDeinit(&s4);
    return result;
}

// Test string find functions
bool test_str_find(void) {
    WriteFmt("Testing StrFindStr, StrFindZstr, and StrFindCstr\n");

    Str haystack = StrInitFromZstr("Hello World");
    Str needle1  = StrInitFromZstr("World");
    Str needle2  = StrInitFromZstr("Hello");
    Str needle3  = StrInitFromZstr("NotFound");

    // Test StrFindStr with match at end
    const char *found1 = StrFindStr(&haystack, &needle1);
    bool        result = (found1 != NULL && ZstrCompare(found1, "World") == 0);

    // Test StrFindStr with match at beginning
    const char *found2 = StrFindStr(&haystack, &needle2);
    result             = result && (found2 != NULL && ZstrCompare(found2, "Hello World") == 0);

    // Test StrFindStr with no match
    const char *found3 = StrFindStr(&haystack, &needle3);
    result             = result && (found3 == NULL);

    // Test StrFindZstr
    const char *found4 = StrFindZstr(&haystack, "World");
    result             = result && (found4 != NULL && ZstrCompare(found4, "World") == 0);

    // Test StrFindCstr
    const char *found5 = StrFindCstr(&haystack, "Wor", 3);
    result             = result && (found5 != NULL && ZstrCompareN(found5, "World", 3) == 0);

    StrDeinit(&haystack);
    StrDeinit(&needle1);
    StrDeinit(&needle2);
    StrDeinit(&needle3);
    return result;
}

// Test string contains/index functions
bool test_str_contains_index(void) {
    WriteFmt("Testing StrContains and StrIndexOf variants\n");

    Str haystack = StrInitFromZstr("Hello World");
    Str needle   = StrInitFromZstr("World");

    bool result = StrContains(&haystack, &needle);
    result      = result && StrContainsZstr(&haystack, "Hello");
    result      = result && StrContainsCstr(&haystack, "lo Wo", 5);
    result      = result && (StrIndexOf(&haystack, &needle) == 6);
    result      = result && (StrIndexOfZstr(&haystack, "Hello") == 0);
    result      = result && (StrIndexOfCstr(&haystack, "World", 5) == 6);
    result      = result && !StrContainsZstr(&haystack, "missing");
    result      = result && (StrIndexOfZstr(&haystack, "missing") == SIZE_MAX);
    result      = result && StrContainsZstr(&haystack, "");
    result      = result && (StrIndexOfZstr(&haystack, "") == 0);

    StrDeinit(&haystack);
    StrDeinit(&needle);
    return result;
}

// Test string starts/ends with functions
bool test_str_starts_ends_with(void) {
    WriteFmt("Testing StrStartsWith and StrEndsWith variants\n");

    Str s      = StrInitFromZstr("Hello World");
    Str prefix = StrInitFromZstr("Hello");
    Str suffix = StrInitFromZstr("World");

    // Test StrStartsWith
    bool result = StrStartsWith(&s, &prefix);

    // Test StrEndsWith
    result = result && StrEndsWith(&s, &suffix);

    // Test StrStartsWithZstr
    result = result && StrStartsWithZstr(&s, "Hello");
    result = result && !StrStartsWithZstr(&s, "World");

    // Test StrEndsWithZstr
    result = result && StrEndsWithZstr(&s, "World");
    result = result && !StrEndsWithZstr(&s, "Hello");

    // Test StrStartsWithCstr
    result = result && StrStartsWithCstr(&s, "Hell", 4);
    result = result && !StrStartsWithCstr(&s, "Worl", 4);

    // Test StrEndsWithCstr
    result = result && StrEndsWithCstr(&s, "orld", 4);
    result = result && !StrEndsWithCstr(&s, "ello", 4);

    StrDeinit(&s);
    StrDeinit(&prefix);
    StrDeinit(&suffix);
    return result;
}

// Test string replace functions
bool test_str_replace(void) {
    WriteFmt("Testing StrReplace variants\n");

    // Test StrReplaceZstr
    Str s1 = StrInitFromZstr("Hello World");
    StrReplaceZstr(&s1, "World", "Universe", 1);
    bool result = (ZstrCompare(s1.data, "Hello Universe") == 0);

    // Test multiple replacements
    StrDeinit(&s1);
    s1 = StrInitFromZstr("Hello Hello Hello");
    StrReplaceZstr(&s1, "Hello", "Hi", 2);
    result = result && (ZstrCompare(s1.data, "Hi Hi Hello") == 0);

    // Test StrReplaceCstr - use the full "World" string instead of just "Wo"
    StrDeinit(&s1);
    s1 = StrInitFromZstr("Hello World");
    StrReplaceCstr(&s1, "World", 5, "Universe", 8, 1);
    result = result && (ZstrCompare(s1.data, "Hello Universe") == 0);

    // Test StrReplace
    StrDeinit(&s1);
    s1          = StrInitFromZstr("Hello World");
    Str find    = StrInitFromZstr("World");
    Str replace = StrInitFromZstr("Universe");
    StrReplace(&s1, &find, &replace, 1);
    result = result && (ZstrCompare(s1.data, "Hello Universe") == 0);

    StrDeinit(&s1);
    StrDeinit(&find);
    StrDeinit(&replace);
    return result;
}

// Test string split functions
bool test_str_split(void) {
    WriteFmt("Testing StrSplit and StrSplitToIters\n");

    // Test StrSplit
    Str  s     = StrInitFromZstr("Hello,World,Test");
    Strs split = StrSplit(&s, ",");

    bool result = (split.length == 3);
    if (split.length >= 3) {
        result = result && (ZstrCompare(split.data[0].data, "Hello") == 0);
        result = result && (ZstrCompare(split.data[1].data, "World") == 0);
        result = result && (ZstrCompare(split.data[2].data, "Test") == 0);
    }

    VecDeinit(&split);

    // Test StrSplitToIters
    StrIters iters = StrSplitToIters(&s, ",");
    result         = result && (iters.length == 3);

    if (iters.length >= 3) {
        // Check first iterator
        StrIter *iter1       = &iters.data[0];
        char     buffer1[10] = {0};
        memcpy(buffer1, iter1->data, iter1->length);
        result = result && (ZstrCompare(buffer1, "Hello") == 0);

        // Check second iterator
        StrIter *iter2       = &iters.data[1];
        char     buffer2[10] = {0};
        memcpy(buffer2, iter2->data, iter2->length);
        result = result && (ZstrCompare(buffer2, "World") == 0);

        // Check third iterator
        StrIter *iter3       = &iters.data[2];
        char     buffer3[10] = {0};
        memcpy(buffer3, iter3->data, iter3->length);
        result = result && (ZstrCompare(buffer3, "Test") == 0);
    }

    VecDeinit(&iters);
    StrDeinit(&s);
    return result;
}

// Test string strip functions
bool test_str_strip(void) {
    WriteFmt("Testing StrStrip variants\n");

    // Test StrLStrip
    Str  s1       = StrInitFromZstr("  Hello  ");
    Str  stripped = StrLStrip(&s1, NULL);
    bool result   = (ZstrCompare(stripped.data, "Hello  ") == 0);
    StrDeinit(&stripped);

    // Test StrRStrip
    stripped = StrRStrip(&s1, NULL);
    result   = result && (ZstrCompare(stripped.data, "  Hello") == 0);
    StrDeinit(&stripped);

    // Test StrStrip
    stripped = StrStrip(&s1, NULL);
    result   = result && (ZstrCompare(stripped.data, "Hello") == 0);
    StrDeinit(&stripped);

    // Test with custom strip characters
    StrDeinit(&s1);
    s1 = StrInitFromZstr("***Hello***");

    stripped = StrLStrip(&s1, "*");
    result   = result && (ZstrCompare(stripped.data, "Hello***") == 0);
    StrDeinit(&stripped);

    stripped = StrRStrip(&s1, "*");
    result   = result && (ZstrCompare(stripped.data, "***Hello") == 0);
    StrDeinit(&stripped);

    stripped = StrStrip(&s1, "*");
    result   = result && (ZstrCompare(stripped.data, "Hello") == 0);
    StrDeinit(&stripped);

    StrDeinit(&s1);
    return result;
}

// Main function that runs all tests
int main(void) {
    WriteFmt("[INFO] Starting Str.Ops tests\n\n");

    // Array of test functions
    TestFunction tests[] =
        {test_str_cmp,          test_str_find,        test_str_contains_index, test_str_starts_ends_with,
         test_str_replace,      test_str_split,       test_str_strip};

    int total_tests = sizeof(tests) / sizeof(tests[0]);

    // Run all tests using the centralized test driver
    return run_test_suite(tests, total_tests, NULL, 0, "Str.Ops");
}
