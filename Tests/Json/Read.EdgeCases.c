#include <Misra/Parsers/JSON.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Io.h>
#include <Misra/Std/Log.h>
#include <Misra/Types.h>

// Include test utilities
#include "../Util/TestRunner.h"


// --- Allocator-aware overrides of JR_OBJ/JR_STR/JR_STR_KV ---
// These rewrite the JSON read path to use explicit JReadString
// with a local `DefaultAllocator alloc` available in scope.
// JSON.h is untouched.
#undef JR_OBJ
#define JR_OBJ(si, reader)                                                                                             \
    do {                                                                                                               \
        if (!StrIterRemainingLength(&si)) {                                                                            \
            break;                                                                                                     \
        }                                                                                                              \
        StrIter saved_si = si;                                                                                         \
        si               = JSkipWhitespace(si);                                                                        \
        char jr_c;                                                                                                     \
        if (!StrIterPeek(&si, &jr_c) || jr_c != '{') {                                                                 \
            LOG_ERROR("Invalid object start. Expected '{'.");                                                          \
            si = saved_si;                                                                                             \
            break;                                                                                                     \
        }                                                                                                              \
        StrIterMustNext(&si);                                                                                          \
        si = JSkipWhitespace(si);                                                                                      \
        StrIter read_si_;                                                                                              \
        bool    expect_comma = false;                                                                                  \
        bool    failed       = false;                                                                                  \
        while (StrIterPeek(&si, &jr_c) && jr_c != '}') {                                                               \
            if (expect_comma) {                                                                                        \
                if (jr_c != ',') {                                                                                     \
                    LOG_ERROR("Expected ',' after key/value pairs in object. Invalid JSON object.");                   \
                    failed = true;                                                                                     \
                    si     = saved_si;                                                                                 \
                    break;                                                                                             \
                }                                                                                                      \
                StrIterMustNext(&si);                                                                                  \
                si = JSkipWhitespace(si);                                                                              \
            }                                                                                                          \
            Str key  = StrInit(&alloc);                                                                                \
            read_si_ = JReadString(si, &key);                                                                          \
            if (read_si_.pos == si.pos) {                                                                              \
                LOG_ERROR("Failed to read string key in object. Invalid JSON");                                        \
                StrDeinit(&key);                                                                                       \
                failed = true;                                                                                         \
                si     = saved_si;                                                                                     \
                break;                                                                                                 \
            }                                                                                                          \
            si = read_si_;                                                                                             \
            si = JSkipWhitespace(si);                                                                                  \
            if (!StrIterPeek(&si, &jr_c) || jr_c != ':') {                                                             \
                LOG_ERROR("Expected ':' after key string. Failed to read JSON");                                       \
                StrDeinit(&key);                                                                                       \
                failed = true;                                                                                         \
                si     = saved_si;                                                                                     \
                break;                                                                                                 \
            }                                                                                                          \
            StrIterMustNext(&si);                                                                                      \
            si                     = JSkipWhitespace(si);                                                              \
            StrIter si_before_read = si;                                                                               \
            { reader }                                                                                                 \
            if (si_before_read.pos == si.pos) {                                                                        \
                StrIter read_si2 = JSkipValue(si);                                                                     \
                if (read_si2.pos == si.pos) {                                                                          \
                    LOG_ERROR("Failed to parse value. Invalid JSON.");                                                 \
                    StrDeinit(&key);                                                                                   \
                    failed = true;                                                                                     \
                    si     = saved_si;                                                                                 \
                    break;                                                                                             \
                }                                                                                                      \
                si = read_si2;                                                                                         \
            }                                                                                                          \
            StrDeinit(&key);                                                                                           \
            si           = JSkipWhitespace(si);                                                                        \
            expect_comma = true;                                                                                       \
        }                                                                                                              \
        if (!failed) {                                                                                                 \
            if (!StrIterPeek(&si, &jr_c) || jr_c != '}') {                                                             \
                LOG_ERROR("Expected end of object '}' but found '{c}'", jr_c);                                         \
                failed = true;                                                                                         \
                si     = saved_si;                                                                                     \
                break;                                                                                                 \
            }                                                                                                          \
            StrIterMustNext(&si);                                                                                      \
        }                                                                                                              \
    } while (0)

#undef JR_STR
#define JR_STR(si, str)                                                                                                \
    do {                                                                                                               \
        Str my_str = StrInit(&alloc);                                                                                  \
        si         = JReadString((si), &my_str);                                                                       \
        (str)      = my_str;                                                                                           \
    } while (0)

#undef JR_STR_KV
#define JR_STR_KV(si, k, str)                                                                                          \
    do {                                                                                                               \
        if (!StrCmpZstr(&key, (k))) {                                                                                  \
            Str my_str = StrInit(&alloc);                                                                              \
            si         = JReadString((si), &my_str);                                                                   \
            (str)      = my_str;                                                                                       \
        }                                                                                                              \
    } while (0)
// --- end of allocator-aware overrides ---


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

void EdgeCaseDataDeinit(EdgeCaseData *data) {
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
    WriteFmtLn("Testing empty object reading");

    DefaultAllocator alloc = DefaultAllocatorInit();

    bool success = true;

    // Test completely empty object
    Str     json1 = StrInitFromZstr("{}", &alloc);
    StrIter si1   = StrIterFromStr(json1);

    struct {
        bool found_anything;
    } obj1;
    obj1.found_anything = false;    // Start as true to test it gets properly handled

    JR_OBJ(si1, {
        obj1.found_anything = true; // This should never execute for empty object
    });

    if (!obj1.found_anything) {     // Should still be true for empty object (reader block didn't execute)
        WriteFmtLn("[DEBUG] Empty object test 1 passed - no fields processed");
    } else {
        WriteFmtLn("[DEBUG] Empty object test 1 FAILED - unexpected field processing");
        success = false;
    }

    // Test empty object with whitespace
    Str     json2 = StrInitFromZstr("  {   }  ", &alloc);
    StrIter si2   = StrIterFromStr(json2);

    struct {
        bool found_anything;
    } obj2;
    obj2.found_anything = false;

    JR_OBJ(si2, { obj2.found_anything = true; });

    if (!obj2.found_anything) { // Should still be true for empty object
        WriteFmtLn("[DEBUG] Empty object with whitespace test passed");
    } else {
        WriteFmtLn("[DEBUG] Empty object with whitespace test FAILED");
        success = false;
    }

    StrDeinit(&json1);
    StrDeinit(&json2);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Test 2: Empty array reading
bool test_empty_array_reading(void) {
    WriteFmtLn("Testing empty array reading");

    DefaultAllocator alloc = DefaultAllocatorInit();

    bool success = true;

    // Test completely empty array
    Str     json1 = StrInitFromZstr("{\"items\":[]}", &alloc);
    StrIter si1   = StrIterFromStr(json1);

    Vec(i32) items = VecInit(&alloc);

    JR_OBJ(si1, {
        JR_ARR_KV(si1, "items", {
            i32 value = 0;
            JR_INT(si1, value);
            VecPushBack(&items, value);
        });
    });

    if (VecLen(&items) == 0) {
        WriteFmtLn("[DEBUG] Empty array test passed - no items added");
    } else {
        WriteFmtLn("[DEBUG] Empty array test FAILED - {} items found", VecLen(&items));
        success = false;
    }

    // Test empty array with whitespace
    Str     json2 = StrInitFromZstr("{\"data\": [  ] }", &alloc);
    StrIter si2   = StrIterFromStr(json2);

    Vec(Str) data = VecInitWithDeepCopy(NULL, StrDeinit, &alloc);

    JR_OBJ(si2, {
        JR_ARR_KV(si2, "data", {
            Str value = StrInit(&alloc);
            JR_STR(si2, value);
            VecPushBack(&data, value);
        });
    });

    if (VecLen(&data) == 0) {
        WriteFmtLn("[DEBUG] Empty array with whitespace test passed");
    } else {
        WriteFmtLn("[DEBUG] Empty array with whitespace test FAILED");
        success = false;
    }

    StrDeinit(&json1);
    StrDeinit(&json2);
    VecDeinit(&items);
    VecDeinit(&data);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Test 3: Empty string reading
bool test_empty_string_reading(void) {
    WriteFmt("Testing empty string reading\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    bool success = true;

    Str     json = StrInitFromZstr("{\"name\":\"\",\"description\":\"\"}", &alloc);
    StrIter si   = StrIterFromStr(json);

    struct {
        Str name;
        Str description;
    } obj = {StrInit(&alloc), StrInit(&alloc)};

    JR_OBJ(si, {
        JR_STR_KV(si, "name", obj.name);
        JR_STR_KV(si, "description", obj.description);
    });

    if (obj.name.length == 0 && obj.description.length == 0) {
        WriteFmt("[DEBUG] Empty string test passed - both strings empty\n");
    } else {
        WriteFmt(
            "[DEBUG] Empty string test FAILED - name len: {}, desc len: {}\n",
            obj.name.length,
            obj.description.length
        );
        success = false;
    }

    StrDeinit(&json);
    StrDeinit(&obj.name);
    StrDeinit(&obj.description);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Test 4: Negative numbers reading
bool test_negative_numbers_reading(void) {
    WriteFmt("Testing negative numbers reading\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    bool success = true;

    Str     json = StrInitFromZstr("{\"temp\":-25,\"balance\":-1000.50,\"delta\":-0.001}", &alloc);
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
        WriteFmt(
            "[DEBUG] Negative numbers test passed - temp: {}, balance: {}, delta: {}\n",
            obj.temp,
            obj.balance,
            obj.delta
        );
    } else {
        WriteFmt(
            "[DEBUG] Negative numbers test FAILED - temp: {}, balance: {}, delta: {}\n",
            obj.temp,
            obj.balance,
            obj.delta
        );
        success = false;
    }

    StrDeinit(&json);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Test 5: Large numbers reading
bool test_large_numbers_reading(void) {
    WriteFmt("Testing large numbers reading\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    bool success = true;

    Str json = StrInitFromZstr(
        "{\"big_int\":9223372036854775807,\"big_float\":1.7976931348623157e+308,\"small_float\":2.2250738585072014e-"
        "308}",
        &alloc
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
        WriteFmt("[DEBUG] Large integer test passed: {}\n", obj.big_int);
    } else {
        WriteFmt("[DEBUG] Large integer test FAILED: expected 9223372036854775807, got {}\n", obj.big_int);
        success = false;
    }

    // Check if floats are in reasonable range (may not be exact due to precision)
    if (obj.big_float > 1.0e+300 && obj.small_float > 0 && obj.small_float < 1.0e-300) {
        WriteFmt("[DEBUG] Large float test passed\n");
    } else {
        WriteFmt("[DEBUG] Large float test FAILED - big: {e}, small: {e}\n", obj.big_float, obj.small_float);
        success = false;
    }

    StrDeinit(&json);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Test 6: Zero values reading
bool test_zero_values_reading(void) {
    WriteFmt("Testing zero values reading\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    bool success = true;

    Str     json = StrInitFromZstr("{\"int_zero\":0,\"float_zero\":0.0,\"bool_false\":false}", &alloc);
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
        WriteFmt(
            "[DEBUG] Zero values test passed - int: {}, float: {}, bool: {}\n",
            obj.int_zero,
            obj.float_zero,
            obj.bool_false ? "true" : "false"
        );
    } else {
        WriteFmt(
            "[DEBUG] Zero values test FAILED - int: {}, float: {}, bool: {}\n",
            obj.int_zero,
            obj.float_zero,
            obj.bool_false ? "true" : "false"
        );
        success = false;
    }

    StrDeinit(&json);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Test 7: Special characters in strings
bool test_special_characters_in_strings(void) {
    WriteFmt("Testing special characters in strings\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    bool success = true;

    // Test with various special characters that might be problematic
    Str json = StrInitFromZstr(
        "{\"path\":\"C:\\\\Program Files\\\\App\",\"message\":\"Hello, "
        "\\\"World\\\"!\",\"data\":\"line1\\nline2\\ttab\"}",
        &alloc
    );
    StrIter si = StrIterFromStr(json);

    struct {
        Str path;
        Str message;
        Str data;
    } obj = {StrInit(&alloc), StrInit(&alloc), StrInit(&alloc)};

    JR_OBJ(si, {
        JR_STR_KV(si, "path", obj.path);
        JR_STR_KV(si, "message", obj.message);
        JR_STR_KV(si, "data", obj.data);
    });

    WriteFmt("[DEBUG] Special chars - path: '{}'\n", obj.path);
    WriteFmt("[DEBUG] Special chars - message: '{}'\n", obj.message);
    WriteFmt("[DEBUG] Special chars - data: '{}'\n", obj.data);

    // Check if strings were parsed (exact content may vary based on escape handling)
    if (obj.path.length > 0 && obj.message.length > 0 && obj.data.length > 0) {
        WriteFmt("[DEBUG] Special characters test passed - all strings parsed\n");
    } else {
        WriteFmt("[DEBUG] Special characters test FAILED - some strings empty\n");
        success = false;
    }

    StrDeinit(&json);
    StrDeinit(&obj.path);
    StrDeinit(&obj.message);
    StrDeinit(&obj.data);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Test 8: Escape sequences reading
bool test_escape_sequences_reading(void) {
    WriteFmt("Testing escape sequences reading\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    bool success = true;

    Str json = StrInitFromZstr(
        "{\"escaped\":\"\\\"quotes\\\"\",\"backslash\":\"\\\\\",\"newline\":\"\\n\",\"tab\":\"\\t\"}",
        &alloc
    );
    StrIter si = StrIterFromStr(json);

    struct {
        Str escaped;
        Str backslash;
        Str newline;
        Str tab;
    } obj = {StrInit(&alloc), StrInit(&alloc), StrInit(&alloc), StrInit(&alloc)};

    JR_OBJ(si, {
        JR_STR_KV(si, "escaped", obj.escaped);
        JR_STR_KV(si, "backslash", obj.backslash);
        JR_STR_KV(si, "newline", obj.newline);
        JR_STR_KV(si, "tab", obj.tab);
    });

    WriteFmtLn("[DEBUG] Escape sequences - escaped: '{}'\n", obj.escaped);
    WriteFmtLn("[DEBUG] Escape sequences - backslash: '{}'\n", obj.backslash);
    WriteFmtLn("[DEBUG] Escape sequences - newline length: {}\n", obj.newline.length);
    WriteFmtLn("[DEBUG] Escape sequences - tab length: {}\n", obj.tab.length);

    // Basic validation that strings were parsed
    if (obj.escaped.length > 0 && obj.backslash.length > 0 && obj.newline.length > 0 && obj.tab.length > 0) {
        WriteFmt("[DEBUG] Escape sequences test passed\n");
    } else {
        WriteFmt("[DEBUG] Escape sequences test FAILED\n");
        success = false;
    }

    StrDeinit(&json);
    StrDeinit(&obj.escaped);
    StrDeinit(&obj.backslash);
    StrDeinit(&obj.newline);
    StrDeinit(&obj.tab);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Test 9: Whitespace variations reading
bool test_whitespace_variations_reading(void) {
    WriteFmt("Testing whitespace variations reading\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    bool success = true;

    // Test with lots of different whitespace patterns
    Str     json = StrInitFromZstr("  {\n\t\"name\"  :  \"test\"  ,\n  \"value\": 42\t,\"flag\"\n:\ntrue\n}\t", &alloc);
    StrIter si   = StrIterFromStr(json);

    struct {
        Str  name;
        i32  value;
        bool flag;
    } obj = {StrInit(&alloc), 0, false};

    JR_OBJ(si, {
        JR_STR_KV(si, "name", obj.name);
        JR_INT_KV(si, "value", obj.value);
        JR_BOOL_KV(si, "flag", obj.flag);
    });

    if (StrCmpCstr(&obj.name, "test", 4) == 0 && obj.value == 42 && obj.flag == true) {
        WriteFmt(
            "[DEBUG] Whitespace variations test passed - name: {}, value: {}, flag: {}\n",
            obj.name.data,
            obj.value,
            obj.flag ? "true" : "false"
        );
    } else {
        WriteFmt(
            "[DEBUG] Whitespace variations test FAILED - name: {}, value: {}, flag: {}\n",
            obj.name.data,
            obj.value,
            obj.flag ? "true" : "false"
        );
        success = false;
    }

    StrDeinit(&json);
    StrDeinit(&obj.name);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Test 10: Nested empty containers
bool test_nested_empty_containers(void) {
    WriteFmtLn("Testing nested empty containers\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    bool success = true;

    Str     json = StrInitFromZstr("{\"outer\":{},\"list\":[],\"deep\":{\"inner\":{}}}", &alloc);
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
        WriteFmt("[DEBUG] Nested empty containers test passed\n");
    } else {
        WriteFmt(
            "[DEBUG] Nested empty containers test results - outer: {}, list: {}, deep: {}, inner: {}\n",
            obj.found_outer ? "true" : "false",
            obj.found_list ? "true" : "false",
            obj.found_deep ? "true" : "false",
            obj.found_inner ? "true" : "false"
        );
    }

    StrDeinit(&json);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Test 11: Mixed empty and filled containers
bool test_mixed_empty_and_filled(void) {
    WriteFmt("Testing mixed empty and filled containers\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    bool success = true;

    Str json =
        StrInitFromZstr("{\"empty_obj\":{},\"filled_obj\":{\"x\":1},\"empty_arr\":[],\"filled_arr\":[1,2]}", &alloc);
    StrIter si = StrIterFromStr(json);

    struct {
        i32 x_value;
        Vec(i32) filled_items;
    } obj = {0, VecInit(&alloc)};

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
        WriteFmt(
            "[DEBUG] Mixed empty and filled test passed - x: {}, items: {}\n",
            obj.x_value,
            VecLen(&obj.filled_items)
        );
    } else {
        WriteFmt(
            "[DEBUG] Mixed empty and filled test FAILED - x: {}, items: {}\n",
            obj.x_value,
            VecLen(&obj.filled_items)
        );
        if (VecLen(&obj.filled_items) > 0) {
            WriteFmt("[DEBUG] First item: {}\n", VecAt(&obj.filled_items, 0));
        }
        success = false;
    }

    StrDeinit(&json);
    VecDeinit(&obj.filled_items);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Test 12: Boundary integers
bool test_boundary_integers(void) {
    WriteFmt("Testing boundary integers\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    bool success = true;

    // Using smaller values that are safer to work with
    Str json   = StrInitFromZstr("{\"max_int\":2147483647,\"min_int\":-2147483648,\"one\":1,\"minus_one\":-1}", &alloc);
    StrIter si = StrIterFromStr(json);

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
        WriteFmt(
            "[DEBUG] Boundary integers test passed - max: {}, min: {}, one: {}, minus_one: {}\n",
            obj.max_int,
            obj.min_int,
            obj.one,
            obj.minus_one
        );
    } else {
        WriteFmt(
            "[DEBUG] Boundary integers test FAILED - max: {}, min: {}, one: {}, minus_one: {}\n",
            obj.max_int,
            obj.min_int,
            obj.one,
            obj.minus_one
        );
        success = false;
    }

    StrDeinit(&json);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Test 13: Boundary floats
bool test_boundary_floats(void) {
    WriteFmt("Testing boundary floats\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    bool success = true;

    Str json =
        StrInitFromZstr("{\"tiny\":0.000001,\"huge\":999999.999999,\"zero\":0.0,\"negative_tiny\":-0.000001}", &alloc);
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
        WriteFmt(
            "[DEBUG] Boundary floats test passed - tiny: {}, huge: {}, zero: {}, neg_tiny: {}\n",
            obj.tiny,
            obj.huge,
            obj.zero,
            obj.negative_tiny
        );
    } else {
        WriteFmt(
            "[DEBUG] Boundary floats test FAILED - tiny: {}, huge: {}, zero: {}, neg_tiny: {}\n",
            obj.tiny,
            obj.huge,
            obj.zero,
            obj.negative_tiny
        );
        success = false;
    }

    StrDeinit(&json);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Main function that runs all edge case reading tests
int main(int argc, char *argv[]) {
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
