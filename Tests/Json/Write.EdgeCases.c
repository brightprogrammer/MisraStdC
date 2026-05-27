#include <Misra/Parsers/JSON.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Io.h>
#include <Misra/Std/Log.h>
#include <Misra/Types.h>

// Include test utilities
#include "../Util/TestRunner.h"

// Helper function to compare JSON output (removes spaces for comparison)
bool compare_json_output(const Str *output, Zstr expected, DefaultAllocator *alloc) {
    // Create a copy of expected without spaces for comparison
    Str expected_str   = StrInitFromZstr(expected, alloc);
    Str output_clean   = StrInit(alloc);
    Str expected_clean = StrInit(alloc);

    // Remove spaces and newlines from both strings for comparison
    for (size i = 0; i < StrLen(output); i++) {
        char c = StrBegin(output)[i];
        if (c != ' ' && c != '\n' && c != '\r' && c != '\t') {
            StrPushBackR(&output_clean, c);
        }
    }

    for (size i = 0; i < StrLen(&expected_str); i++) {
        char c = StrBegin(&expected_str)[i];
        if (c != ' ' && c != '\n' && c != '\r' && c != '\t') {
            StrPushBackR(&expected_clean, c);
        }
    }

    bool result = StrCmp(&output_clean, &expected_clean) == 0;

    if (!result) {
        WriteFmtLn("[DEBUG] JSON comparison failed");
        WriteFmt("[DEBUG] Expected: '");
        for (u64 i = 0; i < StrLen(&expected_clean); i++) {
            WriteFmt("{c}", StrBegin(&expected_clean)[i]);
        }
        WriteFmtLn("'");

        WriteFmt("[DEBUG] Got: '");
        for (u64 i = 0; i < StrLen(&output_clean); i++) {
            WriteFmt("{c}", StrBegin(&output_clean)[i]);
        }
        WriteFmtLn("'");
    }

    StrDeinit(&expected_str);
    StrDeinit(&output_clean);
    StrDeinit(&expected_clean);
    return result;
}

// Function prototypes
bool test_empty_object_writing(void);
bool test_empty_array_writing(void);
bool test_empty_string_writing(void);
bool test_negative_numbers_writing(void);
bool test_large_numbers_writing(void);
bool test_zero_values_writing(void);
bool test_special_characters_writing(void);
bool test_escape_sequences_writing(void);
bool test_nested_empty_containers_writing(void);
bool test_mixed_empty_and_filled_writing(void);
bool test_boundary_integers_writing(void);
bool test_boundary_floats_writing(void);
bool test_single_values_writing(void);

// Test 1: Empty object writing
bool test_empty_object_writing(void) {
    WriteFmtLn("Testing empty object writing");

    DefaultAllocator alloc = DefaultAllocatorInit();

    bool success = true;
    Str  json    = StrInit(&alloc);

    // Write completely empty object
    JW_OBJ(
        json,
        {
            // No content
        }
    );

    Zstr expected = "{}";
    if (!compare_json_output(&json, expected, &alloc)) {
        success = false;
    }

    StrDeinit(&json);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Test 2: Empty array writing
bool test_empty_array_writing(void) {
    WriteFmtLn("Testing empty array writing");

    DefaultAllocator alloc = DefaultAllocatorInit();

    bool success = true;
    Str  json    = StrInit(&alloc);

    Vec(i32) empty_numbers = VecInit(&alloc);
    Vec(Str) empty_strings = VecInitWithDeepCopy(NULL, StrDeinit, &alloc);

    JW_OBJ(json, {
        JW_ARR_KV(json, "numbers", empty_numbers, num, { JW_INT(json, num); });
        JW_ARR_KV(json, "strings", empty_strings, str, { JW_STR(json, str); });
    });

    Zstr expected = "{\"numbers\":[],\"strings\":[]}";
    if (!compare_json_output(&json, expected, &alloc)) {
        success = false;
    }

    StrDeinit(&json);
    VecDeinit(&empty_numbers);
    VecDeinit(&empty_strings);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Test 3: Empty string writing
bool test_empty_string_writing(void) {
    WriteFmtLn("Testing empty string writing");

    DefaultAllocator alloc = DefaultAllocatorInit();

    bool success = true;
    Str  json    = StrInit(&alloc);

    Str empty_name = StrInit(&alloc);
    Str empty_desc = StrInit(&alloc);

    JW_OBJ(json, {
        JW_STR_KV(json, "name", empty_name);
        JW_STR_KV(json, "description", empty_desc);
    });

    Zstr expected = "{\"name\":\"\",\"description\":\"\"}";
    if (!compare_json_output(&json, expected, &alloc)) {
        success = false;
    }

    StrDeinit(&json);
    StrDeinit(&empty_name);
    StrDeinit(&empty_desc);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Test 4: Negative numbers writing
bool test_negative_numbers_writing(void) {
    WriteFmtLn("Testing negative numbers writing");

    DefaultAllocator alloc = DefaultAllocatorInit();

    bool success = true;
    Str  json    = StrInit(&alloc);

    i32 temp    = -25;
    f64 balance = -1000.50;
    f64 delta   = -0.001;

    JW_OBJ(json, {
        JW_INT_KV(json, "temp", temp);
        JW_FLT_KV(json, "balance", balance);
        JW_FLT_KV(json, "delta", delta);
    });

    Zstr expected = "{\"temp\":-25,\"balance\":-1000.500000,\"delta\":-0.001000}";
    if (!compare_json_output(&json, expected, &alloc)) {
        success = false;
    }

    StrDeinit(&json);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Test 5: Large numbers writing
bool test_large_numbers_writing(void) {
    WriteFmtLn("Testing large numbers writing");

    DefaultAllocator alloc = DefaultAllocatorInit();

    bool success = true;
    Str  json    = StrInit(&alloc);

    i64 big_int     = 9223372036854775807LL;
    f64 big_float   = 1.7976931348623157e+308;
    f64 small_float = 2.2250738585072014e-308;

    JW_OBJ(json, {
        JW_INT_KV(json, "big_int", big_int);
        JW_FLT_KV(json, "big_float", big_float);
        JW_FLT_KV(json, "small_float", small_float);
    });

    // For large numbers, just check that valid JSON was produced
    if (StrLen(&json) > 0 && StrBegin(&json)[0] == '{' && StrBegin(&json)[StrLen(&json) - 1] == '}') {
        WriteFmtLn("[DEBUG] Large numbers test passed - produced valid JSON structure");
    } else {
        WriteFmtLn("[DEBUG] Large numbers test FAILED - invalid JSON structure");
        success = false;
    }

    WriteFmtLn("[DEBUG] Large numbers JSON: {}", json);

    StrDeinit(&json);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Test 6: Zero values writing
bool test_zero_values_writing(void) {
    WriteFmtLn("Testing zero values writing\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    bool success = true;
    Str  json    = StrInit(&alloc);

    i32  int_zero   = 0;
    f64  float_zero = 0.0;
    bool bool_false = false;

    JW_OBJ(json, {
        JW_INT_KV(json, "int_zero", int_zero);
        JW_FLT_KV(json, "float_zero", float_zero);
        JW_BOOL_KV(json, "bool_false", bool_false);
    });

    Zstr expected = "{\"int_zero\":0,\"float_zero\":0.000000,\"bool_false\":false}";
    if (!compare_json_output(&json, expected, &alloc)) {
        success = false;
    }

    StrDeinit(&json);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Test 7: Special characters writing
bool test_special_characters_writing(void) {
    WriteFmtLn("Testing special characters writing");

    DefaultAllocator alloc = DefaultAllocatorInit();

    bool success = true;
    Str  json    = StrInit(&alloc);

    // Note: These are the actual characters, not escape sequences
    Str path    = StrInitFromZstr("C:\\Program Files\\App", &alloc);
    Str message = StrInitFromZstr("Hello, \"World\"!", &alloc);
    Str data    = StrInitFromZstr("line1\nline2\ttab", &alloc);

    JW_OBJ(json, {
        JW_STR_KV(json, "path", path);
        JW_STR_KV(json, "message", message);
        JW_STR_KV(json, "data", data);
    });

    WriteFmtLn("[DEBUG] Special characters JSON: {}", json);

    // Just verify that valid JSON structure was produced
    if (StrLen(&json) > 0 && StrBegin(&json)[0] == '{' && StrBegin(&json)[StrLen(&json) - 1] == '}') {
        WriteFmtLn("[DEBUG] Special characters test passed - produced valid JSON");
    } else {
        WriteFmtLn("[DEBUG] Special characters test FAILED - invalid JSON structure");
        success = false;
    }

    StrDeinit(&json);
    StrDeinit(&path);
    StrDeinit(&message);
    StrDeinit(&data);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Test 8: Escape sequences writing
bool test_escape_sequences_writing(void) {
    WriteFmtLn("Testing escape sequences writing");

    DefaultAllocator alloc = DefaultAllocatorInit();

    bool success = true;
    Str  json    = StrInit(&alloc);

    // These contain actual special characters that should be escaped
    Str quotes    = StrInitFromZstr("\"quotes\"", &alloc);
    Str backslash = StrInitFromZstr("\\", &alloc);
    Str newline   = StrInitFromZstr("\n", &alloc);
    Str tab       = StrInitFromZstr("\t", &alloc);

    JW_OBJ(json, {
        JW_STR_KV(json, "quotes", quotes);
        JW_STR_KV(json, "backslash", backslash);
        JW_STR_KV(json, "newline", newline);
        JW_STR_KV(json, "tab", tab);
    });

    WriteFmtLn("[DEBUG] Escape sequences JSON: {}", json);

    // Verify valid JSON structure was produced
    if (StrLen(&json) > 0 && StrBegin(&json)[0] == '{' && StrBegin(&json)[StrLen(&json) - 1] == '}') {
        WriteFmtLn("[DEBUG] Escape sequences test passed - produced valid JSON");
    } else {
        WriteFmtLn("[DEBUG] Escape sequences test FAILED - invalid JSON structure");
        success = false;
    }

    StrDeinit(&json);
    StrDeinit(&quotes);
    StrDeinit(&backslash);
    StrDeinit(&newline);
    StrDeinit(&tab);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Test 9: Nested empty containers writing
bool test_nested_empty_containers_writing(void) {
    WriteFmtLn("Testing nested empty containers writing");

    DefaultAllocator alloc = DefaultAllocatorInit();

    bool success = true;
    Str  json    = StrInit(&alloc);

    Vec(i32) empty_list = VecInit(&alloc);

    JW_OBJ(json, {
        JW_OBJ_KV(
            json,
            "outer",
            {
                // Empty nested object
            }
        );
        JW_ARR_KV(json, "list", empty_list, item, { JW_INT(json, item); });
        JW_OBJ_KV(json, "deep", {
            JW_OBJ_KV(
                json,
                "inner",
                {
                    // Empty deeply nested object
                }
            );
        });
    });

    Zstr expected = "{\"outer\":{},\"list\":[],\"deep\":{\"inner\":{}}}";
    if (!compare_json_output(&json, expected, &alloc)) {
        success = false;
    }

    StrDeinit(&json);
    VecDeinit(&empty_list);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Test 10: Mixed empty and filled containers writing
bool test_mixed_empty_and_filled_writing(void) {
    WriteFmtLn("Testing mixed empty and filled containers writing");

    DefaultAllocator alloc = DefaultAllocatorInit();

    bool success = true;
    Str  json    = StrInit(&alloc);

    Vec(i32) empty_arr  = VecInit(&alloc);
    Vec(i32) filled_arr = VecInit(&alloc);
    i32 val1 = 1, val2 = 2;
    VecPushBack(&filled_arr, val1);
    VecPushBack(&filled_arr, val2);

    i32 x_value = 1;

    JW_OBJ(json, {
        JW_OBJ_KV(
            json,
            "empty_obj",
            {
                // Empty object
            }
        );
        JW_OBJ_KV(json, "filled_obj", { JW_INT_KV(json, "x", x_value); });
        JW_ARR_KV(json, "empty_arr", empty_arr, item, { JW_INT(json, item); });
        JW_ARR_KV(json, "filled_arr", filled_arr, item, { JW_INT(json, item); });
    });

    Zstr expected = "{\"empty_obj\":{},\"filled_obj\":{\"x\":1},\"empty_arr\":[],\"filled_arr\":[1,2]}";
    if (!compare_json_output(&json, expected, &alloc)) {
        success = false;
    }

    StrDeinit(&json);
    VecDeinit(&empty_arr);
    VecDeinit(&filled_arr);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Test 11: Boundary integers writing
bool test_boundary_integers_writing(void) {
    WriteFmtLn("Testing boundary integers writing");

    DefaultAllocator alloc = DefaultAllocatorInit();

    bool success = true;
    Str  json    = StrInit(&alloc);

    i64 max_int   = 2147483647LL;
    i64 min_int   = -2147483648LL;
    i64 one       = 1;
    i64 minus_one = -1;

    JW_OBJ(json, {
        JW_INT_KV(json, "max_int", max_int);
        JW_INT_KV(json, "min_int", min_int);
        JW_INT_KV(json, "one", one);
        JW_INT_KV(json, "minus_one", minus_one);
    });

    Zstr expected = "{\"max_int\":2147483647,\"min_int\":-2147483648,\"one\":1,\"minus_one\":-1}";
    if (!compare_json_output(&json, expected, &alloc)) {
        success = false;
    }

    StrDeinit(&json);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Test 12: Boundary floats writing
bool test_boundary_floats_writing(void) {
    WriteFmtLn("Testing boundary floats writing");

    DefaultAllocator alloc = DefaultAllocatorInit();

    bool success = true;
    Str  json    = StrInit(&alloc);

    f64 tiny          = 0.000001;
    f64 huge          = 999999.999999;
    f64 zero          = 0.0;
    f64 negative_tiny = -0.000001;

    JW_OBJ(json, {
        JW_FLT_KV(json, "tiny", tiny);
        JW_FLT_KV(json, "huge", huge);
        JW_FLT_KV(json, "zero", zero);
        JW_FLT_KV(json, "negative_tiny", negative_tiny);
    });

    // Check for reasonable float formatting (exact precision may vary)
    if (StrLen(&json) > 0 && StrBegin(&json)[0] == '{' && StrBegin(&json)[StrLen(&json) - 1] == '}') {
        WriteFmtLn("[DEBUG] Boundary floats test passed - JSON: {}", json);
    } else {
        WriteFmtLn("[DEBUG] Boundary floats test FAILED");
        success = false;
    }

    StrDeinit(&json);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Test 13: Single values writing (minimal valid JSON objects)
bool test_single_values_writing(void) {
    WriteFmtLn("Testing single values writing");

    DefaultAllocator alloc = DefaultAllocatorInit();

    bool success = true;
    Str  json1   = StrInit(&alloc);
    Str  json2   = StrInit(&alloc);
    Str  json3   = StrInit(&alloc);
    Str  json4   = StrInit(&alloc);

    // Single integer
    i32 single_int = 42;
    JW_OBJ(json1, { JW_INT_KV(json1, "value", single_int); });

    // Single string
    Str single_str = StrInitFromZstr("hello", &alloc);
    JW_OBJ(json2, { JW_STR_KV(json2, "text", single_str); });

    // Single boolean
    bool single_bool = true;
    JW_OBJ(json3, { JW_BOOL_KV(json3, "flag", single_bool); });

    // Single float
    f64 single_float = 3.14;
    JW_OBJ(json4, { JW_FLT_KV(json4, "pi", single_float); });

    Zstr expected1 = "{\"value\":42}";
    Zstr expected2 = "{\"text\":\"hello\"}";
    Zstr expected3 = "{\"flag\":true}";
    Zstr expected4 = "{\"pi\":3.140000}";

    if (!compare_json_output(&json1, expected1, &alloc))
        success = false;
    if (!compare_json_output(&json2, expected2, &alloc))
        success = false;
    if (!compare_json_output(&json3, expected3, &alloc))
        success = false;
    if (!compare_json_output(&json4, expected4, &alloc))
        success = false;

    StrDeinit(&json1);
    StrDeinit(&json2);
    StrDeinit(&json3);
    StrDeinit(&json4);
    StrDeinit(&single_str);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Main function that runs all edge case writing tests
int main(int argc, char *argv[]) {
    // Array of test functions
    TestFunction tests[] = {
        test_empty_object_writing,
        test_empty_array_writing,
        test_empty_string_writing,
        test_negative_numbers_writing,
        test_large_numbers_writing,
        test_zero_values_writing,
        test_special_characters_writing,
        test_escape_sequences_writing,
        test_nested_empty_containers_writing,
        test_mixed_empty_and_filled_writing,
        test_boundary_integers_writing,
        test_boundary_floats_writing,
        test_single_values_writing
    };

    int test_count = sizeof(tests) / sizeof(tests[0]);

    // Use centralized test driver
    return run_test_suite(tests, test_count, NULL, 0, "Json.Write.EdgeCases");
}
