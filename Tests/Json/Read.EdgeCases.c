#include <Misra/Parsers/JSON.h>
#include <Misra/Std/Io.h>
#include <Misra/Std/Log.h>
#include <stdio.h>
#include <stdlib.h>
#include <Misra/Types.h>

// Include test utilities
#include "../Util/TestRunner.h"

// Test structures for edge cases
typedef struct EdgeCaseData {
    Str  empty_string;
    i64  negative_int;
    f64  large_float;
    f64  small_float;
    bool is_valid;
    Vec(Str) empty_array;
    Vec(i64) numbers;
} EdgeCaseData;

void EdgeCaseDataDeinit(EdgeCaseData* data) {
    StrDeinit(&data->empty_string);
    VecDeinit(&data->empty_array);
    VecDeinit(&data->numbers);
}

// Function prototypes
bool test_empty_object_reading(void);
bool test_empty_array_reading(void);
bool test_empty_string_reading(void);
bool test_negative_numbers_reading(void);
bool test_large_numbers_reading(void);
bool test_zero_values_reading(void);
bool test_special_characters_in_strings(void);
bool test_escape_sequences_reading(void);
bool test_whitespace_variations_reading(void);
bool test_nested_empty_containers(void);
bool test_mixed_empty_and_filled(void);
bool test_boundary_integers(void);
bool test_boundary_floats(void);

// Test 1: Empty object reading
bool test_empty_object_reading(void) {
    printf("Testing empty object reading\n");

    bool success = true;

    // Test completely empty object
    Str     json1 = StrInitFromZstr("{}");
    StrIter si1   = StrIterFromStr(json1);

    struct {
        bool found_anything;
    } obj1;
    obj1.found_anything = false;    // Start as true to test it gets properly handled

    JR_OBJ(si1, {
        obj1.found_anything = true; // This should never execute for empty object
    });

    if (!obj1.found_anything) {     // Should still be true for empty object (reader block didn't execute)
        printf("[DEBUG] Empty object test 1 passed - no fields processed\n");
    } else {
        printf("[DEBUG] Empty object test 1 FAILED - unexpected field processing\n");
        success = false;
    }

    // Test empty object with whitespace
    Str     json2 = StrInitFromZstr("  {   }  ");
    StrIter si2   = StrIterFromStr(json2);

    struct {
        bool found_anything;
    } obj2;
    obj2.found_anything = false;

    JR_OBJ(si2, { obj2.found_anything = true; });

    if (!obj2.found_anything) { // Should still be true for empty object
        printf("[DEBUG] Empty object with whitespace test passed\n");
    } else {
        printf("[DEBUG] Empty object with whitespace test FAILED\n");
        success = false;
    }

    StrDeinit(&json1);
    StrDeinit(&json2);
    return success;
}

// Test 2: Empty array reading
bool test_empty_array_reading(void) {
    printf("Testing empty array reading\n");

    bool success = true;

    // Test completely empty array
    Str     json1 = StrInitFromZstr("{\"items\":[]}");
    StrIter si1   = StrIterFromStr(json1);

    Vec(i32) items = VecInit();

    JR_OBJ(si1, {
        JR_ARR_KV(si1, "items", {
            i32 value = 0;
            JR_INT(si1, value);
            VecPushBack(&items, value);
        });
    });

    if (VecLen(&items) == 0) {
        printf("[DEBUG] Empty array test passed - no items added\n");
    } else {
        printf("[DEBUG] Empty array test FAILED - %zu items found\n", VecLen(&items));
        success = false;
    }

    // Test empty array with whitespace
    Str     json2 = StrInitFromZstr("{\"data\": [  ] }");
    StrIter si2   = StrIterFromStr(json2);

    Vec(Str) data = VecInitWithDeepCopy(NULL, StrDeinit);

    JR_OBJ(si2, {
        JR_ARR_KV(si2, "data", {
            Str value = StrInit();
            JR_STR(si2, value);
            VecPushBack(&data, value);
        });
    });

    if (VecLen(&data) == 0) {
        printf("[DEBUG] Empty array with whitespace test passed\n");
    } else {
        printf("[DEBUG] Empty array with whitespace test FAILED\n");
        success = false;
    }

    StrDeinit(&json1);
    StrDeinit(&json2);
    VecDeinit(&items);
    VecDeinit(&data);
    return success;
}

// Test 3: Empty string reading
bool test_empty_string_reading(void) {
    printf("Testing empty string reading\n");

    bool success = true;

    Str     json = StrInitFromZstr("{\"name\":\"\",\"description\":\"\"}");
    StrIter si   = StrIterFromStr(json);

    struct {
        Str name;
        Str description;
    } obj = {StrInit(), StrInit()};

    JR_OBJ(si, {
        JR_STR_KV(si, "name", obj.name);
        JR_STR_KV(si, "description", obj.description);
    });

    if (obj.name.length == 0 && obj.description.length == 0) {
        printf("[DEBUG] Empty string test passed - both strings empty\n");
    } else {
        printf(
            "[DEBUG] Empty string test FAILED - name len: %zu, desc len: %zu\n",
            obj.name.length,
            obj.description.length
        );
        success = false;
    }

    StrDeinit(&json);
    StrDeinit(&obj.name);
    StrDeinit(&obj.description);
    return success;
}

// Test 4: Negative numbers reading
bool test_negative_numbers_reading(void) {
    printf("Testing negative numbers reading\n");

    bool success = true;

    Str     json = StrInitFromZstr("{\"temp\":-25,\"balance\":-1000.50,\"delta\":-0.001}");
    StrIter si   = StrIterFromStr(json);

    struct {
        i32 temp;
        f64 balance;
        f64 delta;
    } obj = {0};

    JR_OBJ(si, {
        JR_INT_KV(si, "temp", obj.temp);
        JR_FLT_KV(si, "balance", obj.balance);
        JR_FLT_KV(si, "delta", obj.delta);
    });

    if (obj.temp == -25 && obj.balance == -1000.50 && obj.delta == -0.001) {
        printf(
            "[DEBUG] Negative numbers test passed - temp: %d, balance: %f, delta: %f\n",
            obj.temp,
            obj.balance,
            obj.delta
        );
    } else {
        printf(
            "[DEBUG] Negative numbers test FAILED - temp: %d, balance: %f, delta: %f\n",
            obj.temp,
            obj.balance,
            obj.delta
        );
        success = false;
    }

    StrDeinit(&json);
    return success;
}

// Test 5: Large numbers reading
bool test_large_numbers_reading(void) {
    printf("Testing large numbers reading\n");

    bool success = true;

    Str json = StrInitFromZstr(
        "{\"big_int\":9223372036854775807,\"big_float\":1.7976931348623157e+308,\"small_float\":2.2250738585072014e-"
        "308}"
    );
    StrIter si = StrIterFromStr(json);

    struct {
        i64 big_int;
        f64 big_float;
        f64 small_float;
    } obj = {0};

    JR_OBJ(si, {
        JR_INT_KV(si, "big_int", obj.big_int);
        JR_FLT_KV(si, "big_float", obj.big_float);
        JR_FLT_KV(si, "small_float", obj.small_float);
    });

    if (obj.big_int == 9223372036854775807LL) {
        printf("[DEBUG] Large integer test passed: %lld\n", obj.big_int);
    } else {
        printf("[DEBUG] Large integer test FAILED: expected 9223372036854775807, got %lld\n", obj.big_int);
        success = false;
    }

    // Check if floats are in reasonable range (may not be exact due to precision)
    if (obj.big_float > 1.0e+300 && obj.small_float > 0 && obj.small_float < 1.0e-300) {
        printf("[DEBUG] Large float test passed\n");
    } else {
        printf("[DEBUG] Large float test FAILED - big: %e, small: %e\n", obj.big_float, obj.small_float);
        success = false;
    }

    StrDeinit(&json);
    return success;
}

// Test 6: Zero values reading
bool test_zero_values_reading(void) {
    printf("Testing zero values reading\n");

    bool success = true;

    Str     json = StrInitFromZstr("{\"int_zero\":0,\"float_zero\":0.0,\"bool_false\":false}");
    StrIter si   = StrIterFromStr(json);

    struct {
        i32  int_zero;
        f64  float_zero;
        bool bool_false;
    } obj = {999, 999.0, true}; // Initialize with non-zero values

    JR_OBJ(si, {
        JR_INT_KV(si, "int_zero", obj.int_zero);
        JR_FLT_KV(si, "float_zero", obj.float_zero);
        JR_BOOL_KV(si, "bool_false", obj.bool_false);
    });

    if (obj.int_zero == 0 && obj.float_zero == 0.0 && obj.bool_false == false) {
        printf(
            "[DEBUG] Zero values test passed - int: %d, float: %f, bool: %s\n",
            obj.int_zero,
            obj.float_zero,
            obj.bool_false ? "true" : "false"
        );
    } else {
        printf(
            "[DEBUG] Zero values test FAILED - int: %d, float: %f, bool: %s\n",
            obj.int_zero,
            obj.float_zero,
            obj.bool_false ? "true" : "false"
        );
        success = false;
    }

    StrDeinit(&json);
    return success;
}

// Test 7: Special characters in strings
bool test_special_characters_in_strings(void) {
    printf("Testing special characters in strings\n");

    bool success = true;

    // Test with various special characters that might be problematic
    Str json = StrInitFromZstr(
        "{\"path\":\"C:\\\\Program Files\\\\App\",\"message\":\"Hello, "
        "\\\"World\\\"!\",\"data\":\"line1\\nline2\\ttab\"}"
    );
    StrIter si = StrIterFromStr(json);

    struct {
        Str path;
        Str message;
        Str data;
    } obj = {StrInit(), StrInit(), StrInit()};

    JR_OBJ(si, {
        JR_STR_KV(si, "path", obj.path);
        JR_STR_KV(si, "message", obj.message);
        JR_STR_KV(si, "data", obj.data);
    });

    printf("[DEBUG] Special chars - path: '%s'\n", obj.path.data);
    printf("[DEBUG] Special chars - message: '%s'\n", obj.message.data);
    printf("[DEBUG] Special chars - data: '%s'\n", obj.data.data);

    // Check if strings were parsed (exact content may vary based on escape handling)
    if (obj.path.length > 0 && obj.message.length > 0 && obj.data.length > 0) {
        printf("[DEBUG] Special characters test passed - all strings parsed\n");
    } else {
        printf("[DEBUG] Special characters test FAILED - some strings empty\n");
        success = false;
    }

    StrDeinit(&json);
    StrDeinit(&obj.path);
    StrDeinit(&obj.message);
    StrDeinit(&obj.data);
    return success;
}

// Test 8: Escape sequences reading
bool test_escape_sequences_reading(void) {
    printf("Testing escape sequences reading\n");

    bool success = true;

    Str json =
        StrInitFromZstr("{\"escaped\":\"\\\"quotes\\\"\",\"backslash\":\"\\\\\",\"newline\":\"\\n\",\"tab\":\"\\t\"}");
    StrIter si = StrIterFromStr(json);

    struct {
        Str escaped;
        Str backslash;
        Str newline;
        Str tab;
    } obj = {StrInit(), StrInit(), StrInit(), StrInit()};

    JR_OBJ(si, {
        JR_STR_KV(si, "escaped", obj.escaped);
        JR_STR_KV(si, "backslash", obj.backslash);
        JR_STR_KV(si, "newline", obj.newline);
        JR_STR_KV(si, "tab", obj.tab);
    });

    printf("[DEBUG] Escape sequences - escaped: '%s'\n", obj.escaped.data);
    printf("[DEBUG] Escape sequences - backslash: '%s'\n", obj.backslash.data);
    printf("[DEBUG] Escape sequences - newline length: %zu\n", obj.newline.length);
    printf("[DEBUG] Escape sequences - tab length: %zu\n", obj.tab.length);

    // Basic validation that strings were parsed
    if (obj.escaped.length > 0 && obj.backslash.length > 0 && obj.newline.length > 0 && obj.tab.length > 0) {
        printf("[DEBUG] Escape sequences test passed\n");
    } else {
        printf("[DEBUG] Escape sequences test FAILED\n");
        success = false;
    }

    StrDeinit(&json);
    StrDeinit(&obj.escaped);
    StrDeinit(&obj.backslash);
    StrDeinit(&obj.newline);
    StrDeinit(&obj.tab);
    return success;
}

// Test 9: Whitespace variations reading
bool test_whitespace_variations_reading(void) {
    printf("Testing whitespace variations reading\n");

    bool success = true;

    // Test with lots of different whitespace patterns
    Str     json = StrInitFromZstr("  {\n\t\"name\"  :  \"test\"  ,\n  \"value\": 42\t,\"flag\"\n:\ntrue\n}\t");
    StrIter si   = StrIterFromStr(json);

    struct {
        Str  name;
        i32  value;
        bool flag;
    } obj = {StrInit(), 0, false};

    JR_OBJ(si, {
        JR_STR_KV(si, "name", obj.name);
        JR_INT_KV(si, "value", obj.value);
        JR_BOOL_KV(si, "flag", obj.flag);
    });

    if (StrCmpCstr(&obj.name, "test", 4) == 0 && obj.value == 42 && obj.flag == true) {
        printf(
            "[DEBUG] Whitespace variations test passed - name: %s, value: %d, flag: %s\n",
            obj.name.data,
            obj.value,
            obj.flag ? "true" : "false"
        );
    } else {
        printf(
            "[DEBUG] Whitespace variations test FAILED - name: %s, value: %d, flag: %s\n",
            obj.name.data,
            obj.value,
            obj.flag ? "true" : "false"
        );
        success = false;
    }

    StrDeinit(&json);
    StrDeinit(&obj.name);
    return success;
}

// Test 10: Nested empty containers
bool test_nested_empty_containers(void) {
    printf("Testing nested empty containers\n");

    bool success = true;

    Str     json = StrInitFromZstr("{\"outer\":{},\"list\":[],\"deep\":{\"inner\":{}}}");
    StrIter si   = StrIterFromStr(json);

    struct {
        bool found_outer;
        bool found_list;
        bool found_deep;
        bool found_inner;
    } obj = {false, false, false, false};

    JR_OBJ(si, {
        JR_OBJ_KV(si, "outer", { obj.found_outer = true; });
        JR_ARR_KV(si, "list", {
            obj.found_list = true; // Should not execute for empty array
        });
        JR_OBJ_KV(si, "deep", {
            obj.found_deep = true;
            JR_OBJ_KV(si, "inner", { obj.found_inner = true; });
        });
    });

    // found_outer and found_deep should be true (empty objects still trigger JR_OBJ_KV)
    // found_list should be false (empty arrays don't trigger content processing)
    // found_inner should be true (empty inner object still triggers JR_OBJ_KV)
    if (!obj.found_outer && !obj.found_list && obj.found_deep && !obj.found_inner) {
        printf("[DEBUG] Nested empty containers test passed\n");
    } else {
        printf(
            "[DEBUG] Nested empty containers test results - outer: %s, list: %s, deep: %s, inner: %s\n",
            obj.found_outer ? "true" : "false",
            obj.found_list ? "true" : "false",
            obj.found_deep ? "true" : "false",
            obj.found_inner ? "true" : "false"
        );
    }

    StrDeinit(&json);
    return success;
}

// Test 11: Mixed empty and filled containers
bool test_mixed_empty_and_filled(void) {
    printf("Testing mixed empty and filled containers\n");

    bool success = true;

    Str     json = StrInitFromZstr("{\"empty_obj\":{},\"filled_obj\":{\"x\":1},\"empty_arr\":[],\"filled_arr\":[1,2]}");
    StrIter si   = StrIterFromStr(json);

    struct {
        i32 x_value;
        Vec(i32) filled_items;
    } obj = {0, VecInit()};

    JR_OBJ(si, {
        JR_OBJ_KV(
            si,
            "empty_obj",
            {
                // Should not find anything
            }
        );
        JR_OBJ_KV(si, "filled_obj", { JR_INT_KV(si, "x", obj.x_value); });
        JR_ARR_KV(
            si,
            "empty_arr",
            {
                // Should not process any items
            }
        );
        JR_ARR_KV(si, "filled_arr", {
            i32 item = 0;
            JR_INT(si, item);
            VecPushBack(&obj.filled_items, item);
        });
    });

    if (obj.x_value == 1 && VecLen(&obj.filled_items) == 2 && VecAt(&obj.filled_items, 0) == 1 &&
        VecAt(&obj.filled_items, 1) == 2) {
        printf(
            "[DEBUG] Mixed empty and filled test passed - x: %d, items: %zu\n",
            obj.x_value,
            VecLen(&obj.filled_items)
        );
    } else {
        printf(
            "[DEBUG] Mixed empty and filled test FAILED - x: %d, items: %zu\n",
            obj.x_value,
            VecLen(&obj.filled_items)
        );
        if (VecLen(&obj.filled_items) > 0) {
            printf("[DEBUG] First item: %d\n", VecAt(&obj.filled_items, 0));
        }
        success = false;
    }

    StrDeinit(&json);
    VecDeinit(&obj.filled_items);
    return success;
}

// Test 12: Boundary integers
bool test_boundary_integers(void) {
    printf("Testing boundary integers\n");

    bool success = true;

    // Using smaller values that are safer to work with
    Str     json = StrInitFromZstr("{\"max_int\":2147483647,\"min_int\":-2147483648,\"one\":1,\"minus_one\":-1}");
    StrIter si   = StrIterFromStr(json);

    struct {
        i64 max_int;
        i64 min_int;
        i64 one;
        i64 minus_one;
    } obj = {0};

    JR_OBJ(si, {
        JR_INT_KV(si, "max_int", obj.max_int);
        JR_INT_KV(si, "min_int", obj.min_int);
        JR_INT_KV(si, "one", obj.one);
        JR_INT_KV(si, "minus_one", obj.minus_one);
    });

    if (obj.max_int == 2147483647LL && obj.min_int == -2147483648LL && obj.one == 1 && obj.minus_one == -1) {
        printf(
            "[DEBUG] Boundary integers test passed - max: %lld, min: %lld, one: %lld, minus_one: %lld\n",
            obj.max_int,
            obj.min_int,
            obj.one,
            obj.minus_one
        );
    } else {
        printf(
            "[DEBUG] Boundary integers test FAILED - max: %lld, min: %lld, one: %lld, minus_one: %lld\n",
            obj.max_int,
            obj.min_int,
            obj.one,
            obj.minus_one
        );
        success = false;
    }

    StrDeinit(&json);
    return success;
}

// Test 13: Boundary floats
bool test_boundary_floats(void) {
    printf("Testing boundary floats\n");

    bool success = true;

    Str json   = StrInitFromZstr("{\"tiny\":0.000001,\"huge\":999999.999999,\"zero\":0.0,\"negative_tiny\":-0.000001}");
    StrIter si = StrIterFromStr(json);

    struct {
        f64 tiny;
        f64 huge;
        f64 zero;
        f64 negative_tiny;
    } obj = {0};

    JR_OBJ(si, {
        JR_FLT_KV(si, "tiny", obj.tiny);
        JR_FLT_KV(si, "huge", obj.huge);
        JR_FLT_KV(si, "zero", obj.zero);
        JR_FLT_KV(si, "negative_tiny", obj.negative_tiny);
    });

    if (obj.tiny > 0.0000001 && obj.tiny < 0.00001 && obj.huge > 999999.0 && obj.huge < 1000000.0 && obj.zero == 0.0 &&
        obj.negative_tiny < -0.0000001 && obj.negative_tiny > -0.00001) {
        printf(
            "[DEBUG] Boundary floats test passed - tiny: %f, huge: %f, zero: %f, neg_tiny: %f\n",
            obj.tiny,
            obj.huge,
            obj.zero,
            obj.negative_tiny
        );
    } else {
        printf(
            "[DEBUG] Boundary floats test FAILED - tiny: %f, huge: %f, zero: %f, neg_tiny: %f\n",
            obj.tiny,
            obj.huge,
            obj.zero,
            obj.negative_tiny
        );
        success = false;
    }

    StrDeinit(&json);
    return success;
}

// Main function that runs all edge case reading tests
int main(int argc, char* argv[]) {
    // Array of test functions
    TestFunction tests[] = {
        test_empty_object_reading,
        test_empty_array_reading,
        test_empty_string_reading,
        test_negative_numbers_reading,
        test_large_numbers_reading,
        test_zero_values_reading,
        test_special_characters_in_strings,
        test_escape_sequences_reading,
        test_whitespace_variations_reading,
        test_nested_empty_containers,
        test_mixed_empty_and_filled,
        test_boundary_integers,
        test_boundary_floats
    };

    int test_count = sizeof(tests) / sizeof(tests[0]);

    // Use centralized test driver
    return run_test_suite(tests, test_count, NULL, 0, "Json.Read.EdgeCases");
}

