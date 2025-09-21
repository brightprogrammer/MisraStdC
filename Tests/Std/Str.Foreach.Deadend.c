#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Log.h>
#include <stdio.h>
#include <Misra/Types.h> // For LVAL macro

// Include test utilities
#include "../Util/TestRunner.h"

// Deadend test prototypes (tests that should crash due to out-of-bounds access)
bool test_str_foreach_out_of_bounds_access(void);
bool test_str_foreach_idx_out_of_bounds_access(void);
bool test_str_foreach_idx_basic_out_of_bounds_access(void);
bool test_str_foreach_reverse_idx_out_of_bounds_access(void);
bool test_str_foreach_ptr_idx_out_of_bounds_access(void);
bool test_str_foreach_reverse_ptr_idx_out_of_bounds_access(void);
bool test_str_foreach_ptr_in_range_idx_out_of_bounds_access(void);

// Deadend test: Make idx go out of bounds in StrForeachInRangeIdx by shrinking string during iteration
bool test_str_foreach_out_of_bounds_access(void) {
    WriteFmt("Testing StrForeachInRangeIdx where idx goes out of bounds (should crash)\n");

    Str s = StrInitFromZstr("Hello World!"); // 12 characters

    // Use StrForeachInRangeIdx which captures the 'end' parameter at the start
    // Even if we shrink the string, the loop will continue until idx reaches the fixed end
    size original_length = s.length; // Capture this as 12
    StrForeachInRangeIdx(&s, chr, idx, 0, original_length) {
        WriteFmt("Accessing idx {} (s.length={}): '{c}'\n", idx, s.length, chr);

        // When we reach idx=4, drastically shrink the string to length 3
        // But StrForeachInRangeIdx will continue until idx reaches original_length (12)
        if (idx == 4) {
            StrResize(&s, 3); // Shrink to only 3 characters
            WriteFmt(
                "String resized to length {}, but range iteration will continue to idx {}...\n",
                s.length,
                original_length
            );
        }

        // When idx >= 3 (after resize), StrForeachInRangeIdx will detect:
        // if ((idx) >= (v)->length) LOG_FATAL(...)
        // This should cause a fatal error when idx >= s.length
    }

    // Should never reach here if idx goes out of bounds
    StrDeinit(&s);
    return false;
}

// Deadend test: Make idx go out of bounds in StrForeachInRangeIdx by deleting characters
bool test_str_foreach_idx_out_of_bounds_access(void) {
    WriteFmt("Testing StrForeachInRangeIdx with character deletion where idx goes out of bounds (should crash)\n");

    Str s = StrInitFromZstr("Programming"); // 11 characters

    // Use StrForeachInRangeIdx with a fixed range that will become invalid
    // when we delete characters during iteration
    size original_length = s.length; // Capture this as 11
    StrForeachInRangeIdx(&s, chr, idx, 0, original_length) {
        WriteFmt("Accessing idx {} (s.length={}): '{c}'\n", idx, s.length, chr);

        // When we reach idx=3, delete several characters from the beginning
        // This will make the higher indices invalid
        if (idx == 3) {
            StrDeleteRange(&s, 0, 6); // Remove first 6 characters
            WriteFmt(
                "Deleted first 6 characters, new length={}, but range iteration will continue to idx {}...\n",
                s.length,
                original_length
            );
        }

        // When idx >= 5 (after deletion), StrForeachInRangeIdx will detect:
        // if ((idx) >= (v)->length) LOG_FATAL(...)
        // This should cause a fatal error when idx >= s.length
    }

    // Should never reach here if bounds checking triggers
    StrDeinit(&s);
    return false;
}

// Deadend test: Make idx go out of bounds in StrForeachReverseIdx by modifying string during iteration
bool test_str_foreach_reverse_idx_out_of_bounds_access(void) {
    WriteFmt("Testing StrForeachReverseIdx where idx goes out of bounds (should crash)\n");

    Str s = StrInitFromZstr("Beautiful Weather"); // 17 characters

    // StrForeachReverseIdx (VecForeachReverseIdx) has explicit bounds checking: if ((idx) >= (v)->length) LOG_FATAL(...)
    StrForeachReverseIdx(&s, chr, idx) {
        WriteFmt("Accessing idx {} (s.length={}): '{c}'\n", idx, s.length, chr);

        // When we reach idx=10, drastically shrink the string
        // This will make subsequent iterations invalid since idx will still decrement
        // but the string length is now smaller
        if (idx == 10) {
            StrResize(&s, 4); // Shrink to only 4 characters
            WriteFmt("String resized to length {} during reverse iteration...\n", s.length);
        }

        // When idx >= s.length, the bounds check will trigger:
        // if ((idx) >= (v)->length) LOG_FATAL(...)
    }

    // Should never reach here if bounds checking triggers
    StrDeinit(&s);
    return false;
}

// Deadend test: Make idx go out of bounds in StrForeachPtrIdx by modifying string during iteration
bool test_str_foreach_ptr_idx_out_of_bounds_access(void) {
    WriteFmt("Testing StrForeachPtrIdx where idx goes out of bounds (should crash)\n");

    Str s = StrInitFromZstr("Programming Test"); // 16 characters

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
        // if ((idx) >= (v)->length) LOG_FATAL(...)
    }

    // Should never reach here if bounds checking triggers
    StrDeinit(&s);
    return false;
}

// Deadend test: Make idx go out of bounds in StrForeachReversePtrIdx by modifying string during iteration
bool test_str_foreach_reverse_ptr_idx_out_of_bounds_access(void) {
    WriteFmt("Testing StrForeachReversePtrIdx where idx goes out of bounds (should crash)\n");

    Str s = StrInitFromZstr("Excellent Example"); // 17 characters

    // StrForeachReversePtrIdx (VecForeachPtrReverseIdx) has explicit bounds checking: if ((idx) >= (v)->length) LOG_FATAL(...)
    StrForeachReversePtrIdx(&s, chr_ptr, idx) {
        WriteFmt("Accessing idx {} (s.length={}): '{c}'\n", idx, s.length, *chr_ptr);

        // When we reach idx=12, shrink the string significantly
        if (idx == 12) {
            StrResize(&s, 5); // Shrink to only 5 characters
            WriteFmt("String resized to length {} during reverse ptr iteration...\n", s.length);
        }

        // When idx >= s.length, the bounds check will trigger:
        // if ((idx) >= (v)->length) LOG_FATAL(...)
    }

    // Should never reach here if bounds checking triggers
    StrDeinit(&s);
    return false;
}

// Deadend test: Make idx go out of bounds in StrForeachPtrInRangeIdx by modifying string during iteration
bool test_str_foreach_ptr_in_range_idx_out_of_bounds_access(void) {
    WriteFmt("Testing StrForeachPtrInRangeIdx where idx goes out of bounds (should crash)\n");

    Str s = StrInitFromZstr("Comprehensive Testing Framework"); // 32 characters

    // Use StrForeachPtrInRangeIdx with a fixed range that becomes invalid when we modify the string
    size original_length = s.length; // Capture this as 32
    StrForeachPtrInRangeIdx(&s, chr_ptr, idx, 0, original_length) {
        WriteFmt("Accessing idx {} (s.length={}): '{c}'\n", idx, s.length, *chr_ptr);

        // When we reach idx=8, delete several characters
        if (idx == 8) {
            StrDeleteRange(&s, 0, 20); // Remove first 20 characters
            WriteFmt(
                "Deleted first 20 characters, new length={}, but range ptr iteration continues to idx {}...\n",
                s.length,
                original_length
            );
        }

        // When idx >= s.length, the bounds check will trigger:
        // if ((idx) >= (v)->length) LOG_FATAL(...)
    }

    // Should never reach here if bounds checking triggers
    StrDeinit(&s);
    return false;
}

// Deadend test: Make idx go out of bounds in basic StrForeachIdx by modifying string during iteration
bool test_str_foreach_idx_basic_out_of_bounds_access(void) {
    WriteFmt("Testing basic StrForeachIdx where idx goes out of bounds (should crash)\n");

    Str s = StrInitFromZstr("Testing Basic"); // 13 characters

    // Basic StrForeachIdx (VecForeachIdx) now has explicit bounds checking: if ((idx) >= (v)->length) LOG_FATAL(...)
    StrForeachIdx(&s, chr, idx) {
        WriteFmt("Accessing idx {} (s.length={}): '{c}'\n", idx, s.length, chr);

        // When we reach idx=3, drastically shrink the string
        // This will make subsequent iterations invalid
        if (idx == 3) {
            StrResize(&s, 2); // Shrink to only 2 characters
            WriteFmt("String resized to length {}, but basic foreach iteration continues...\n", s.length);
        }

        // When idx >= s.length, the bounds check will trigger:
        // if ((idx) >= (v)->length) LOG_FATAL(...)
    }

    // Should never reach here if bounds checking triggers
    StrDeinit(&s);
    return false;
}

// Main function that runs all deadend tests
int main(void) {
    WriteFmt("[INFO] Starting Str.Foreach.Deadend tests\n\n");

    // Array of deadend test functions (tests that should crash)
    TestFunction deadend_tests[] = {
        test_str_foreach_out_of_bounds_access,
        test_str_foreach_idx_out_of_bounds_access,
        test_str_foreach_idx_basic_out_of_bounds_access,
        test_str_foreach_reverse_idx_out_of_bounds_access,
        test_str_foreach_ptr_idx_out_of_bounds_access,
        test_str_foreach_reverse_ptr_idx_out_of_bounds_access,
        test_str_foreach_ptr_in_range_idx_out_of_bounds_access
    };

    int deadend_count = sizeof(deadend_tests) / sizeof(deadend_tests[0]);

    // Run all deadend tests using the centralized test driver
    return run_test_suite(NULL, 0, deadend_tests, deadend_count, "Str.Foreach.Deadend");
}
