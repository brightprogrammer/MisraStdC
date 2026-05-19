#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Io.h>
#include <Misra/Types.h> // For LVAL macro

// Include test utilities
#include "../Util/TestRunner.h"

// Function prototypes
bool test_str_foreach_idx(void);
bool test_str_foreach_reverse_idx(void);
bool test_str_foreach_ptr_idx(void);
bool test_str_foreach_reverse_ptr_idx(void);
bool test_str_foreach(void);
bool test_str_foreach_reverse(void);
bool test_str_foreach_ptr(void);
bool test_str_foreach_ptr_reverse(void);
bool test_str_foreach_in_range_idx(void);
bool test_str_foreach_in_range(void);
bool test_str_foreach_ptr_in_range_idx(void);
bool test_str_foreach_ptr_in_range(void);

bool test_str_foreach_out_of_bounds_access(void);
bool test_str_foreach_idx_out_of_bounds_access(void);
bool test_str_foreach_idx_basic_out_of_bounds_access(void);
bool test_str_foreach_reverse_idx_out_of_bounds_access(void);
bool test_str_foreach_ptr_idx_out_of_bounds_access(void);
bool test_str_foreach_reverse_ptr_idx_out_of_bounds_access(void);
bool test_str_foreach_ptr_in_range_idx_out_of_bounds_access(void);

// Test StrForeachIdx macro
bool test_str_foreach_idx(void) {
    WriteFmt("Testing StrForeachIdx\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    Str s = StrInitFromZstr("Hello", &alloc);

    // Build a new string by iterating through each character with its index
    Str result = StrInit(&alloc);
    StrForeachIdx(&s, chr, idx) {
        StrAppendFmt(&result, "{c}{}", chr, idx);
    }

    // The result should be "H0e1l2l3o4"
    bool success = (ZstrCompare(result.data, "H0e1l2l3o4") == 0);

    StrDeinit(&s);
    StrDeinit(&result);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Test StrForeachReverseIdx macro
bool test_str_foreach_reverse_idx(void) {
    WriteFmt("Testing StrForeachReverseIdx\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    Str s = StrInitFromZstr("Hello", &alloc);

    // Build a new string by iterating through each character in reverse with its index
    Str result = StrInit(&alloc);

    StrForeachReverseIdx(&s, chr, idx) {
        // Append the character and its index to the result string
        StrAppendFmt(&result, "{c}{}", chr, idx);
    }

    bool success = (ZstrCompare(result.data, "o4l3l2e1H0") == 0);
    WriteFmt("  (Index 0 was processed)\n");

    StrDeinit(&s);
    StrDeinit(&result);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Test StrForeachPtrIdx macro
bool test_str_foreach_ptr_idx(void) {
    WriteFmt("Testing StrForeachPtrIdx\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    Str s = StrInitFromZstr("Hello", &alloc);

    // Build a new string by iterating through each character pointer with its index
    Str result = StrInit(&alloc);
    StrForeachPtrIdx(&s, chrptr, idx) {
        // Append the character (via pointer) and its index to the result string
        StrAppendFmt(&result, "{c}{}", *chrptr, idx);

        // Modify the original string by converting to uppercase
        if (*chrptr >= 'a' && *chrptr <= 'z') {
            *chrptr = *chrptr - 'a' + 'A';
        }
    }

    // The result should be "H0e1l2l3o4"
    bool success = (ZstrCompare(result.data, "H0e1l2l3o4") == 0);

    // The original string should now be "HELLO" (all uppercase)
    success = success && (ZstrCompare(s.data, "HELLO") == 0);

    StrDeinit(&s);
    StrDeinit(&result);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Test StrForeachReversePtrIdx macro
bool test_str_foreach_reverse_ptr_idx(void) {
    WriteFmt("Testing StrForeachReversePtrIdx\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    Str s = StrInitFromZstr("Hello", &alloc);

    // Build a new string by iterating through each character pointer in reverse with its index
    Str result = StrInit(&alloc);

    StrForeachReversePtrIdx(&s, chrptr, idx) {
        // Append the character (via pointer) and its index to the result string
        StrAppendFmt(&result, "{c}{}", *chrptr, idx);

        // Modify the original string by converting to uppercase
        if (*chrptr >= 'a' && *chrptr <= 'z') {
            *chrptr = *chrptr - 'a' + 'A';
        }
    }

    bool success = false;
    success      = (ZstrCompare(result.data, "o4l3l2e1H0") == 0);
    success      = success && (ZstrCompare(s.data, "HELLO") == 0); // All uppercase
    WriteFmt("  (Index 0 was processed)\n");

    StrDeinit(&s);
    StrDeinit(&result);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Test StrForeach macro
bool test_str_foreach(void) {
    WriteFmt("Testing StrForeach\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    Str s = StrInitFromZstr("Hello", &alloc);

    // Build a new string by iterating through each character
    Str result = StrInit(&alloc);
    StrForeach(&s, chr) {
        // Append the character to the result string
        StrPushBack(&result, chr);
    }

    // The result should be "Hello"
    bool success = (ZstrCompare(result.data, "Hello") == 0);

    StrDeinit(&s);
    StrDeinit(&result);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Test StrForeachReverse macro
bool test_str_foreach_reverse(void) {
    WriteFmt("Testing StrForeachReverse\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    Str s = StrInitFromZstr("Hello", &alloc);

    // Build a new string by iterating through each character in reverse
    Str  result     = StrInit(&alloc);
    size char_count = 0;

    StrForeachReverse(&s, chr) {
        // Append the character to the result string
        StrPushBack(&result, chr);
        char_count++;
    }

    // The expected result depends on whether all characters are processed
    bool success = false;
    if (char_count == s.length) {
        success = (ZstrCompare(result.data, "olleH") == 0);
        WriteFmt("  (All characters were processed)\n");
    } else {
        success = (ZstrCompare(result.data, "olle") == 0);
        WriteFmt("  (First character was NOT processed - bug in macro)\n");
    }

    StrDeinit(&s);
    StrDeinit(&result);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Test StrForeachPtr macro
bool test_str_foreach_ptr(void) {
    WriteFmt("Testing StrForeachPtr\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    Str s = StrInitFromZstr("Hello", &alloc);

    // Build a new string by iterating through each character pointer
    Str result = StrInit(&alloc);
    StrForeachPtr(&s, chrptr) {
        // Append the character (via pointer) to the result string
        StrPushBack(&result, *chrptr);

        // Modify the original string by converting to uppercase
        if (*chrptr >= 'a' && *chrptr <= 'z') {
            *chrptr = *chrptr - 'a' + 'A';
        }
    }

    // The result should be "Hello" (original values before modification)
    bool success = (ZstrCompare(result.data, "Hello") == 0);

    // The original string should now be "HELLO" (all uppercase)
    success = success && (ZstrCompare(s.data, "HELLO") == 0);

    StrDeinit(&s);
    StrDeinit(&result);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Test StrForeachPtrReverse macro
bool test_str_foreach_ptr_reverse(void) {
    WriteFmt("Testing StrForeachPtrReverse\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    Str s = StrInitFromZstr("Hello", &alloc);

    // Build a new string by iterating through each character pointer in reverse
    Str  result     = StrInit(&alloc);
    size char_count = 0;

    StrForeachPtrReverse(&s, chrptr) {
        // Append the character (via pointer) to the result string
        StrPushBack(&result, *chrptr);

        // Modify the original string by converting to uppercase
        if (*chrptr >= 'a' && *chrptr <= 'z') {
            *chrptr = *chrptr - 'a' + 'A';
        }

        char_count++;
    }

    // The expected result depends on whether all characters are processed
    bool success = false;
    if (char_count == s.length) {
        success = (ZstrCompare(result.data, "olleH") == 0);
        success = success && (ZstrCompare(s.data, "HELLO") == 0); // All uppercase
        WriteFmt("  (All characters were processed)\n");
    } else {
        success = (ZstrCompare(result.data, "olle") == 0);
        success = success && (ZstrCompare(s.data, "HELLo") == 0); // All uppercase except first char
        WriteFmt("  (First character was NOT processed - bug in macro)\n");
    }

    StrDeinit(&s);
    StrDeinit(&result);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Test StrForeachInRangeIdx macro
bool test_str_foreach_in_range_idx(void) {
    WriteFmt("Testing StrForeachInRangeIdx\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    Str s = StrInitFromZstr("Hello World", &alloc);

    // Build a new string by iterating through a range of characters with indices
    Str result = StrInit(&alloc);
    StrForeachInRangeIdx(&s, chr, idx, 6, 11) {
        // Append the character and its index to the result string
        StrAppendFmt(&result, "{c}{}", chr, idx);
    }

    // The result should be "W6o7r8l9d10" (characters from index 6-10 with their indices)
    bool success = (ZstrCompare(result.data, "W6o7r8l9d10") == 0);

    // Test with empty range
    Str empty_result = StrInit(&alloc);
    StrForeachInRangeIdx(&s, chr, idx, 3, 3) {
        // This block should not execute
        StrPushBack(&empty_result, chr);
    }

    // The empty_result should remain empty
    success = success && (empty_result.length == 0);

    StrDeinit(&s);
    StrDeinit(&result);
    StrDeinit(&empty_result);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Test StrForeachInRange macro
bool test_str_foreach_in_range(void) {
    WriteFmt("Testing StrForeachInRange\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    Str s = StrInitFromZstr("Hello World", &alloc);

    // Build a new string by iterating through a range of characters
    Str result = StrInit(&alloc);
    StrForeachInRange(&s, chr, 0, 5) {
        // Append the character to the result string
        StrPushBack(&result, chr);
    }

    // The result should be "Hello" (first 5 characters)
    bool success = (ZstrCompare(result.data, "Hello") == 0);

    // Test with range at the end of the string
    Str end_result = StrInit(&alloc);
    StrForeachInRange(&s, chr, 6, 11) {
        // Append the character to the result string
        StrPushBack(&end_result, chr);
    }

    // The end_result should be "World" (last 5 characters)
    success = success && (ZstrCompare(end_result.data, "World") == 0);

    StrDeinit(&s);
    StrDeinit(&result);
    StrDeinit(&end_result);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Test StrForeachPtrInRangeIdx macro
bool test_str_foreach_ptr_in_range_idx(void) {
    WriteFmt("Testing StrForeachPtrInRangeIdx\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    Str s = StrInitFromZstr("Hello World", &alloc);

    // Build a new string by iterating through a range of character pointers with indices
    Str result = StrInit(&alloc);
    StrForeachPtrInRangeIdx(&s, chrptr, idx, 6, 11) {
        // Append the character and its index to the result string
        StrAppendFmt(&result, "{c}{}", *chrptr, idx);

        // Modify the original string by converting to uppercase
        if (*chrptr >= 'a' && *chrptr <= 'z') {
            *chrptr = *chrptr - 'a' + 'A';
        }
    }

    // The result should be "W6o7r8l9d10" (characters from index 6-10 with their indices)
    bool success = (ZstrCompare(result.data, "W6o7r8l9d10") == 0);

    // The original string should now have "WORLD" in uppercase
    success = success && (ZstrCompare(s.data, "Hello WORLD") == 0);

    StrDeinit(&s);
    StrDeinit(&result);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Test StrForeachPtrInRange macro
bool test_str_foreach_ptr_in_range(void) {
    WriteFmt("Testing StrForeachPtrInRange\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    Str s = StrInitFromZstr("Hello World", &alloc);

    // Build a new string by iterating through a range of character pointers
    Str result = StrInit(&alloc);
    StrForeachPtrInRange(&s, chrptr, 0, 5) {
        // Append the character to the result string
        StrPushBack(&result, *chrptr);

        // Modify the original string by converting to uppercase
        if (*chrptr >= 'a' && *chrptr <= 'z') {
            *chrptr = *chrptr - 'a' + 'A';
        }
    }

    // The result should be "Hello" (first 5 characters)
    bool success = (ZstrCompare(result.data, "Hello") == 0);

    // The original string should now have "HELLO" in uppercase
    success = success && (ZstrCompare(s.data, "HELLO World") == 0);

    StrDeinit(&s);
    StrDeinit(&result);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Make idx go out of bounds in StrForeachInRangeIdx by shrinking string during iteration
bool test_str_foreach_out_of_bounds_access(void) {
    WriteFmt("Testing StrForeachInRangeIdx where idx goes out of bounds\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    Str s = StrInitFromZstr("Hello World!", &alloc); // 12 characters

    // Use StrForeachInRangeIdx which captures the 'end' parameter at the start
    // Even if we shrink the string, the loop will continue until idx reaches the fixed end
    size original_length = s.length; // Capture this as 12
    StrForeachInRangeIdx(&s, chr, idx, 0, original_length) {
        WriteFmt("Accessing idx {} (s.length={}): '{c}'\n", idx, s.length, chr);

        // When we reach idx=4, drastically shrink the string to length 3
        // But StrForeachInRangeIdx will continue until idx reaches original_length (12)
        if (idx == 4) {
            StrResize(&s, 3); // Shrink to only 3 characters
            WriteFmt("String resized to length {}, idx={}...\n", s.length, idx);
        }

        // When idx >= 3 (after resize), StrForeachInRangeIdx will detect:
        // loop will automatically terminate

        if (idx > 4) {
            LOG_ERROR("Should've terminated");
            StrDeinit(&s);
            DefaultAllocatorDeinit(&alloc);
            return false;
        }
    }

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return true;
}

// Make idx go out of bounds in StrForeachInRangeIdx by deleting characters
bool test_str_foreach_idx_out_of_bounds_access(void) {
    WriteFmt("Testing StrForeachInRangeIdx with character deletion where idx goes out of bounds\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    Str s = StrInitFromZstr("Programming", &alloc); // 11 characters

    // Use StrForeachInRangeIdx with a fixed range that will become invalid
    // when we delete characters during iteration
    size original_length = s.length; // Capture this as 11
    StrForeachInRangeIdx(&s, chr, idx, 0, original_length) {
        WriteFmt("Accessing idx {} (s.length={}): '{c}'\n", idx, s.length, chr);

        // When we reach idx=3, delete several characters from the beginning
        // This will make the higher indices invalid
        if (idx == 3) {
            StrDeleteRange(&s, 0, 6); // Remove first 6 characters
            WriteFmt("Deleted first 6 characters, new length={}, idx={}...\n", s.length, idx);
        }

        // When idx >= 5 (after deletion), StrForeachInRangeIdx will detect:
        // loop will automatically terminate

        if (idx >= 5) {
            LOG_ERROR("Should've terminated");
            StrDeinit(&s);
            DefaultAllocatorDeinit(&alloc);
            return false;
        }
    }

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return true;
}

// Make idx go out of bounds in StrForeachReverseIdx by modifying string during iteration
bool test_str_foreach_reverse_idx_out_of_bounds_access(void) {
    WriteFmt("Testing StrForeachReverseIdx where idx goes out of bounds\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    Str s = StrInitFromZstr("Beautiful Weather", &alloc); // 17 characters

    // StrForeachReverseIdx (VecForeachReverseIdx) has explicit bounds checking: if ((idx) >= (v)->length) LOG_FATAL(...)
    StrForeachReverseIdx(&s, chr, idx) {
        WriteFmt("Accessing idx {} (s.length={}): '{c}'\n", idx, s.length, chr);

        // When we reach idx=10, drastically shrink the string
        // This will make subsequent iterations invalid since idx will still decrement
        // but the string length is now smaller
        if (idx == 10) {
            StrResize(&s, 4); // Shrink to only 4 characters
            WriteFmt("String resized to length {} during reverse iteration... idx = {}\n", s.length, idx);
        }

        // When idx < 10, the bounds check will trigger:
        // loop will automatically terminate
        if (idx < 10) {
            LOG_ERROR("Should've terminated");
            StrDeinit(&s);
            DefaultAllocatorDeinit(&alloc);
            return false;
        }
    }

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return true;
}

// Make idx go out of bounds in StrForeachPtrIdx by modifying string during iteration
bool test_str_foreach_ptr_idx_out_of_bounds_access(void) {
    WriteFmt("Testing StrForeachPtrIdx where idx goes out of bounds\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    Str s = StrInitFromZstr("Programming Test", &alloc); // 16 characters

    // StrForeachPtrIdx (VecForeachPtrIdx) has explicit bounds checking: if ((idx) >= (v)->length) LOG_FATAL(...)
    StrForeachPtrIdx(&s, chr_ptr, idx) {
        WriteFmt("Accessing idx {} (s.length={}): '{c}'\n", idx, s.length, *chr_ptr);

        // When we reach idx=4, delete most characters from the string
        // This will make the current idx invalid after the body executes
        if (idx == 4) {
            StrResize(&s, 4); // Shrink to only 4 characters (valid indices: 0,1,2,3)
            WriteFmt("String resized to length {}, current idx={} is now out of bounds...\n", s.length, idx);
        }

        // When idx >= s.length, the bounds check will trigger:
        // loop will automatically terminate

        if (idx > 4) {
            LOG_ERROR("Should've terminated");
            StrDeinit(&s);
            DefaultAllocatorDeinit(&alloc);
            return false;
        }
    }

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return true;
}

// Make idx go out of bounds in StrForeachReversePtrIdx by modifying string during iteration
bool test_str_foreach_reverse_ptr_idx_out_of_bounds_access(void) {
    WriteFmt("Testing StrForeachReversePtrIdx where idx goes out of bounds\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    Str s = StrInitFromZstr("Excellent Example", &alloc); // 17 characters

    // StrForeachReversePtrIdx (VecForeachPtrReverseIdx) has explicit bounds checking: if ((idx) >= (v)->length) LOG_FATAL(...)
    StrForeachReversePtrIdx(&s, chr_ptr, idx) {
        WriteFmt("Accessing idx {} (s.length={}): '{c}'\n", idx, s.length, *chr_ptr);

        // When we reach idx=12, shrink the string significantly
        if (idx == 12) {
            StrResize(&s, 5); // Shrink to only 5 characters
            WriteFmt("String resized to length {} during reverse ptr iteration... idx = {}\n", s.length, idx);
        }

        // When idx < 12, the bounds check will trigger:
        // loop will terminate automatically

        if (idx < 12) {
            LOG_ERROR("Should've terminated");
            StrDeinit(&s);
            DefaultAllocatorDeinit(&alloc);
            return false;
        }
    }

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return true;
}

// Make idx go out of bounds in StrForeachPtrInRangeIdx by modifying string during iteration
bool test_str_foreach_ptr_in_range_idx_out_of_bounds_access(void) {
    WriteFmt("Testing StrForeachPtrInRangeIdx where idx goes out of bounds\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    Str s = StrInitFromZstr("Comprehensive Testing Framework", &alloc); // 31 characters

    // Use StrForeachPtrInRangeIdx with a fixed range that becomes invalid when we modify the string
    size original_length = s.length; // Capture this as 32
    StrForeachPtrInRangeIdx(&s, chr_ptr, idx, 0, original_length) {
        WriteFmt("Accessing idx {} (s.length={}): '{c}'\n", idx, s.length, *chr_ptr);

        // When we reach idx=8, delete several characters
        if (idx == 8) {
            StrDeleteRange(&s, 0, 20); // Remove first 20 characters
            WriteFmt("Deleted first 20 characters, new length={}, idx = {}...\n", s.length, idx);
        }

        // When idx >= s.length, the bounds check will trigger:
        // loop will terminate automatically

        if (idx >= s.length) {
            LOG_ERROR("Should've terminated");
            StrDeinit(&s);
            DefaultAllocatorDeinit(&alloc);
            return false;
        }
    }

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return true;
}

// Make idx go out of bounds in basic StrForeachIdx by modifying string during iteration
bool test_str_foreach_idx_basic_out_of_bounds_access(void) {
    WriteFmt("Testing basic StrForeachIdx where idx goes out of bounds\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    Str s = StrInitFromZstr("Testing Basic", &alloc); // 13 characters

    // Basic StrForeachIdx (VecForeachIdx) now has explicit bounds checking: if ((idx) >= (v)->length) LOG_FATAL(...)
    StrForeachIdx(&s, chr, idx) {
        WriteFmt("Accessing idx {} (s.length={}): '{c}'\n", idx, s.length, chr);

        // When we reach idx=3, drastically shrink the string
        // This will make subsequent iterations invalid
        if (idx == 3) {
            StrResize(&s, 2); // Shrink to only 2 characters
            WriteFmt("String resized to length {}, but basic foreach iteration continues... idx = {}\n", s.length, idx);
        }

        // When idx >= s.length, the bounds check will trigger:
        // loop will terminate automatically

        if (idx > 3) {
            LOG_ERROR("Should've terminated");
            StrDeinit(&s);
            DefaultAllocatorDeinit(&alloc);
            return false;
        }
    }

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return true;
}

// Main function that runs all tests
int main(void) {
    WriteFmt("[INFO] Starting Str.Foreach.Simple tests\n\n");

    // Array of normal test functions
    TestFunction tests[] = {
        test_str_foreach_idx,
        test_str_foreach_reverse_idx,
        test_str_foreach_ptr_idx,
        test_str_foreach_reverse_ptr_idx,
        test_str_foreach,
        test_str_foreach_reverse,
        test_str_foreach_ptr,
        test_str_foreach_ptr_reverse,
        test_str_foreach_in_range_idx,
        test_str_foreach_in_range,
        test_str_foreach_ptr_in_range_idx,
        test_str_foreach_ptr_in_range,

        test_str_foreach_out_of_bounds_access,
        test_str_foreach_idx_out_of_bounds_access,
        test_str_foreach_idx_basic_out_of_bounds_access,
        test_str_foreach_reverse_idx_out_of_bounds_access,
        test_str_foreach_ptr_idx_out_of_bounds_access,
        test_str_foreach_reverse_ptr_idx_out_of_bounds_access,
        test_str_foreach_ptr_in_range_idx_out_of_bounds_access
    };

    int total_tests = sizeof(tests) / sizeof(tests[0]);

    // Run all tests using the centralized test driver
    return run_test_suite(tests, total_tests, NULL, 0, "Str.Foreach.Simple");
}
