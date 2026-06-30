#include <Misra/Parsers/JSON.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Io.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Memory.h>
#include <Misra/Types.h>

// Include test utilities
#include "../../Util/JsonReaderAllocAware.h"
#include "../../Util/TestRunner.h"




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
bool test_scalar_readers_reject_malformed(void);
bool test_scalar_readers_value_and_advance(void);
bool test_truncated_unicode_escape_rejected(void);
bool test_unknown_keys_of_every_type_skipped(void);
bool test_malformed_object_rejected(void);
bool test_negative_number_exact_values(void);
bool test_js_float_reads_integer_valued_number(void);
bool test_js_float_reads_zero_valued_integer(void);
bool test_js_null_clears_flag_on_non_null(void);
bool test_js_null_sets_flag_on_null(void);

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

    if (StrLen(&obj.name) == 0 && StrLen(&obj.description) == 0) {
        WriteFmt("[DEBUG] Empty string test passed - both strings empty\n");
    } else {
        WriteFmt(
            "[DEBUG] Empty string test FAILED - name len: {}, desc len: {}\n",
            StrLen(&obj.name),
            StrLen(&obj.description)
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

    // Decoded content is caller-observable -- assert exact bytes, not just
    // non-empty. Each \\ collapses to one '\', each \" to one '"', \n to LF,
    // \t to HT.
    //
    // path:    "C:\\Program Files\\App"  -> C:\Program Files\App
    // message: "Hello, \"World\"!"       -> Hello, "World"!
    // data:    "line1\nline2\ttab"       -> line1<LF>line2<HT>tab
    Zstr path_want = "C:\\Program Files\\App";
    Zstr msg_want  = "Hello, \"World\"!";
    Zstr data_want = "line1\nline2\ttab";
    if (StrLen(&obj.path) != ZstrLen(path_want) || MemCompare(StrBegin(&obj.path), path_want, StrLen(&obj.path)) != 0) {
        WriteFmtLn("[DEBUG] Special characters FAILED: path content wrong");
        success = false;
    }
    if (StrLen(&obj.message) != ZstrLen(msg_want) ||
        MemCompare(StrBegin(&obj.message), msg_want, StrLen(&obj.message)) != 0) {
        WriteFmtLn("[DEBUG] Special characters FAILED: message content wrong");
        success = false;
    }
    if (StrLen(&obj.data) != ZstrLen(data_want) || MemCompare(StrBegin(&obj.data), data_want, StrLen(&obj.data)) != 0) {
        WriteFmtLn("[DEBUG] Special characters FAILED: data content wrong");
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

    // One key per escape the parser claims to support: \" \\ \/ \b \f \n \r \t.
    // In a C source literal each backslash is doubled, so e.g. the JSON token
    // `\n` is spelled "\\n" here.
    Str json = StrInitFromZstr(
        "{"
        "\"quote\":\"\\\"\","
        "\"backslash\":\"\\\\\","
        "\"slash\":\"\\/\","
        "\"backspace\":\"\\b\","
        "\"formfeed\":\"\\f\","
        "\"newline\":\"\\n\","
        "\"carriage\":\"\\r\","
        "\"tab\":\"\\t\""
        "}",
        &alloc
    );
    StrIter si = StrIterFromStr(json);

    struct {
        Str quote;
        Str backslash;
        Str slash;
        Str backspace;
        Str formfeed;
        Str newline;
        Str carriage;
        Str tab;
    } obj = {
        StrInit(&alloc),
        StrInit(&alloc),
        StrInit(&alloc),
        StrInit(&alloc),
        StrInit(&alloc),
        StrInit(&alloc),
        StrInit(&alloc),
        StrInit(&alloc)
    };

    JR_OBJ(si, {
        JR_STR_KV(si, "quote", obj.quote);
        JR_STR_KV(si, "backslash", obj.backslash);
        JR_STR_KV(si, "slash", obj.slash);
        JR_STR_KV(si, "backspace", obj.backspace);
        JR_STR_KV(si, "formfeed", obj.formfeed);
        JR_STR_KV(si, "newline", obj.newline);
        JR_STR_KV(si, "carriage", obj.carriage);
        JR_STR_KV(si, "tab", obj.tab);
    });

    // Decoded content is caller-observable: assert exact length AND exact
    // byte for every escape. A length-only check lets a parser that decodes
    // \n -> literal 'n' or \" -> '\'' pass silently. mull's operators cannot
    // express char-literal swaps, so these byte assertions are the only
    // durable guard for escape semantics -- write them with no hedging.
    // \uXXXX is intentionally absent: the parser rejects it (see
    // test_truncated_unicode_escape_rejected).
    struct {
        const Str *got;
        char       want;
        Zstr       name;
    } expect[] = {
        {    &obj.quote,  '"',     "quote"},
        {&obj.backslash, '\\', "backslash"},
        {    &obj.slash,  '/',     "slash"},
        {&obj.backspace, '\b', "backspace"},
        { &obj.formfeed, '\f',  "formfeed"},
        {  &obj.newline, '\n',   "newline"},
        { &obj.carriage, '\r',  "carriage"},
        {      &obj.tab, '\t',       "tab"},
    };
    for (u64 i = 0; i < sizeof(expect) / sizeof(expect[0]); i++) {
        if (StrLen(expect[i].got) != 1 || *StrBegin(expect[i].got) != expect[i].want) {
            WriteFmtLn(
                "[DEBUG] Escape sequences FAILED: {} decoded wrong (len={})",
                expect[i].name,
                StrLen(expect[i].got)
            );
            success = false;
        }
    }

    StrDeinit(&json);
    StrDeinit(&obj.quote);
    StrDeinit(&obj.backslash);
    StrDeinit(&obj.slash);
    StrDeinit(&obj.backspace);
    StrDeinit(&obj.formfeed);
    StrDeinit(&obj.newline);
    StrDeinit(&obj.carriage);
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

    if (StrCmp(&obj.name, "test", 4) == 0 && obj.value == 42 && obj.flag == true) {
        WriteFmt(
            "[DEBUG] Whitespace variations test passed - name: {}, value: {}, flag: {}\n",
            StrBegin(&obj.name),
            obj.value,
            obj.flag ? "true" : "false"
        );
    } else {
        WriteFmt(
            "[DEBUG] Whitespace variations test FAILED - name: {}, value: {}, flag: {}\n",
            StrBegin(&obj.name),
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

// Test 14: Scalar readers reject malformed tokens (FAILURE contract:
// "returns original StrIter on error" -- the iterator must NOT advance,
// so a caller can detect the failure and fall back).
bool test_scalar_readers_reject_malformed(void) {
    WriteFmtLn("Testing scalar readers reject malformed tokens");

    DefaultAllocator alloc   = DefaultAllocatorInit();
    bool             success = true;

    // JReadBool on a token that starts like a bool but isn't.
    {
        Str     j   = StrInitFromZstr("tru3", &alloc);
        StrIter si  = StrIterFromStr(j);
        bool    b   = true;
        StrIter out = JReadBool(si, &b);
        if (StrIterIndex(&out) != StrIterIndex(&si)) {
            WriteFmtLn("[DEBUG] JReadBool advanced on malformed 'tru3'");
            success = false;
        }
        StrDeinit(&j);
    }
    {
        Str     j   = StrInitFromZstr("fXlse", &alloc);
        StrIter si  = StrIterFromStr(j);
        bool    b   = true;
        StrIter out = JReadBool(si, &b);
        if (StrIterIndex(&out) != StrIterIndex(&si)) {
            WriteFmtLn("[DEBUG] JReadBool advanced on malformed 'fXlse'");
            success = false;
        }
        StrDeinit(&j);
    }
    // Input too short to spell a bool -- must fail (the >= length guard).
    {
        Str     j   = StrInitFromZstr("tr", &alloc);
        StrIter si  = StrIterFromStr(j);
        bool    b   = true;
        StrIter out = JReadBool(si, &b);
        if (StrIterIndex(&out) != StrIterIndex(&si)) {
            WriteFmtLn("[DEBUG] JReadBool advanced on too-short 'tr'");
            success = false;
        }
        StrDeinit(&j);
    }
    // JReadNull on a 'n...' token that isn't "null".
    {
        Str     j       = StrInitFromZstr("nuXX", &alloc);
        StrIter si      = StrIterFromStr(j);
        bool    is_null = true;
        StrIter out     = JReadNull(si, &is_null);
        if (StrIterIndex(&out) != StrIterIndex(&si)) {
            WriteFmtLn("[DEBUG] JReadNull advanced on malformed 'nuXX'");
            success = false;
        }
        StrDeinit(&j);
    }

    return success;
}

// Test 15: Scalar readers parse valid tokens to the right value AND
// consume exactly the token (SUCCESS contract: advance past the token).
bool test_scalar_readers_value_and_advance(void) {
    WriteFmtLn("Testing scalar readers parse + advance");

    DefaultAllocator alloc   = DefaultAllocatorInit();
    bool             success = true;

    {
        Str     j   = StrInitFromZstr("true rest", &alloc);
        StrIter si  = StrIterFromStr(j);
        bool    b   = false;
        StrIter out = JReadBool(si, &b);
        // value correct and exactly 4 bytes consumed
        if (!(b == true && StrIterIndex(&out) == 4)) {
            WriteFmtLn("[DEBUG] JReadBool 'true' value/advance wrong: b={}, idx={}", b, StrIterIndex(&out));
            success = false;
        }
        StrDeinit(&j);
    }
    {
        Str     j   = StrInitFromZstr("false rest", &alloc);
        StrIter si  = StrIterFromStr(j);
        bool    b   = true;
        StrIter out = JReadBool(si, &b);
        if (!(b == false && StrIterIndex(&out) == 5)) {
            WriteFmtLn("[DEBUG] JReadBool 'false' value/advance wrong: b={}, idx={}", b, StrIterIndex(&out));
            success = false;
        }
        StrDeinit(&j);
    }
    {
        Str     j       = StrInitFromZstr("null rest", &alloc);
        StrIter si      = StrIterFromStr(j);
        bool    is_null = false;
        StrIter out     = JReadNull(si, &is_null);
        if (!(is_null == true && StrIterIndex(&out) == 4)) {
            WriteFmtLn("[DEBUG] JReadNull 'null' value/advance wrong: n={}, idx={}", is_null, StrIterIndex(&out));
            success = false;
        }
        StrDeinit(&j);
    }

    // Exact-length tokens (no trailing bytes): the readers must still
    // accept these. A token that exactly fills the remaining input is the
    // boundary case for the minimum-length guard.
    {
        Str     j   = StrInitFromZstr("true", &alloc);
        StrIter si  = StrIterFromStr(j);
        bool    b   = false;
        StrIter out = JReadBool(si, &b);
        if (!(b == true && StrIterIndex(&out) == 4)) {
            WriteFmtLn("[DEBUG] JReadBool exact 'true' wrong: b={}, idx={}", b, StrIterIndex(&out));
            success = false;
        }
        StrDeinit(&j);
    }
    {
        Str     j   = StrInitFromZstr("false", &alloc);
        StrIter si  = StrIterFromStr(j);
        bool    b   = true;
        StrIter out = JReadBool(si, &b);
        if (!(b == false && StrIterIndex(&out) == 5)) {
            WriteFmtLn("[DEBUG] JReadBool exact 'false' wrong: b={}, idx={}", b, StrIterIndex(&out));
            success = false;
        }
        StrDeinit(&j);
    }
    {
        Str     j       = StrInitFromZstr("null", &alloc);
        StrIter si      = StrIterFromStr(j);
        bool    is_null = false;
        StrIter out     = JReadNull(si, &is_null);
        if (!(is_null == true && StrIterIndex(&out) == 4)) {
            WriteFmtLn("[DEBUG] JReadNull exact 'null' wrong: n={}, idx={}", is_null, StrIterIndex(&out));
            success = false;
        }
        StrDeinit(&j);
    }

    return success;
}

// Test 16: \uXXXX escapes are unsupported and must be rejected -- both the
// truncated form (over-read guard) AND a complete \uXXXX (no silent-drop /
// wrong-answer). Reader FAILURE contract: returns the original iterator with
// the output cleared. A string without \u still parses.
bool test_truncated_unicode_escape_rejected(void) {
    WriteFmtLn("Testing \\u escape rejection (truncated and complete)");

    DefaultAllocator alloc   = DefaultAllocatorInit();
    bool             success = true;

    // "\u" with no following hex digits, then closing quote/EOF.
    Zstr cases[] = {"\"\\u\"", "\"\\u12\"", "\"ab\\u\""};
    for (u64 i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        Str     j   = StrInitFromZstr(cases[i], &alloc);
        StrIter si  = StrIterFromStr(j);
        Str     out = StrInit(&alloc);
        StrIter r   = JReadString(si, &out);
        // Truncated escape -> failure -> iterator unchanged.
        if (StrIterIndex(&r) != StrIterIndex(&si)) {
            WriteFmtLn("[DEBUG] JReadString advanced on truncated escape case {}", i);
            success = false;
        }
        StrDeinit(&out);
        StrDeinit(&j);
    }

    // A well-formed \uXXXX escape is unsupported and must be REJECTED, not
    // silently consumed: the parser has no decoder, so accepting it would
    // hand the caller a string whose content differs from the document with
    // a success status -- a wrong answer in a fail-fast library. Contract:
    // reject -> iterator rewinds to start, output cleared.
    {
        Str     j   = StrInitFromZstr("\"a\\u00e9b\"", &alloc);
        StrIter si  = StrIterFromStr(j);
        Str     out = StrInit(&alloc);
        StrIter r   = JReadString(si, &out);
        if (StrIterIndex(&r) != StrIterIndex(&si)) {
            WriteFmtLn("[DEBUG] JReadString accepted a complete \\u escape (should reject): idx={}", StrIterIndex(&r));
            success = false;
        }
        if (StrLen(&out) != 0) {
            WriteFmtLn("[DEBUG] JReadString left partial output on rejected \\u escape: len={}", StrLen(&out));
            success = false;
        }
        StrDeinit(&out);
        StrDeinit(&j);
    }

    // A string with no \u escape still parses normally -- the rejection above
    // is specific to \u, not a blanket failure.
    {
        Str     j   = StrInitFromZstr("\"plain\"", &alloc);
        StrIter si  = StrIterFromStr(j);
        Str     out = StrInit(&alloc);
        StrIter r   = JReadString(si, &out);
        if (StrIterIndex(&r) == StrIterIndex(&si) || StrIterIndex(&r) != StrIterLength(&r)) {
            WriteFmtLn("[DEBUG] JReadString failed on a plain string: idx={}", StrIterIndex(&r));
            success = false;
        }
        if (StrLen(&out) != 5 || MemCompare(StrBegin(&out), "plain", 5) != 0) {
            WriteFmtLn("[DEBUG] JReadString decoded plain string wrong: len={}", StrLen(&out));
            success = false;
        }
        StrDeinit(&out);
        StrDeinit(&j);
    }

    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Test 17: Unknown keys (every value shape) are skipped, and a later
// recognized key still parses correctly. This exercises the JSkipValue /
// JSkipObject / JSkipArray dispatch the reader relies on.
bool test_unknown_keys_of_every_type_skipped(void) {
    WriteFmtLn("Testing unknown keys of every value type are skipped");

    DefaultAllocator alloc   = DefaultAllocatorInit();
    bool             success = true;

    Str json = StrInitFromZstr(
        "{"
        "\"u_str\":\"ignore\","
        "\"u_int\":-42,"
        "\"u_zero\":0,"
        "\"u_flt\":3.5,"
        "\"u_bool\":true,"
        "\"u_null\":null,"
        "\"u_obj\":{\"a\":1,\"b\":[2,3]},"
        "\"u_arr\":[1,{\"x\":9},\"s\"],"
        "\"wanted\":7"
        "}",
        &alloc
    );
    StrIter si     = StrIterFromStr(json);
    i64     wanted = 0;

    JR_OBJ(si, { JR_INT_KV(si, "wanted", wanted); });

    // The recognized value past all the skipped ones must come through,
    // and the iterator must finish at the closing brace (whole object
    // consumed).
    if (wanted != 7) {
        WriteFmtLn("[DEBUG] wanted parsed wrong after skips: {}", wanted);
        success = false;
    }
    if (StrIterIndex(&si) != StrIterLength(&si)) {
        WriteFmtLn(
            "[DEBUG] iterator did not consume whole object: idx={}, len={}",
            StrIterIndex(&si),
            StrIterLength(&si)
        );
        success = false;
    }

    StrDeinit(&json);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Test 18: Structurally broken objects are rejected -- the reader rewinds
// the iterator to the start (FAILURE contract) instead of silently
// accepting a partial parse.
bool test_malformed_object_rejected(void) {
    WriteFmtLn("Testing malformed objects are rejected (iterator rewinds)");

    DefaultAllocator alloc   = DefaultAllocatorInit();
    bool             success = true;

    // missing ':' separator, missing closing brace, missing comma.
    Zstr cases[] = {
        "{\"a\" 1}",        // no colon
        "{\"a\":1",         // truncated, no closing brace
        "{\"a\":1 \"b\":2}" // missing comma between pairs
    };

    for (u64 i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        Str     json = StrInitFromZstr(cases[i], &alloc);
        StrIter si   = StrIterFromStr(json);
        i64     a    = 0;
        i64     b    = 0;

        JR_OBJ(si, {
            JR_INT_KV(si, "a", a);
            JR_INT_KV(si, "b", b);
        });
        (void)a;
        (void)b;

        // On structural failure the macro restores si to its start.
        if (StrIterIndex(&si) != 0) {
            WriteFmtLn("[DEBUG] malformed object case {} did not rewind: idx={}", i, StrIterIndex(&si));
            success = false;
        }

        StrDeinit(&json);
    }

    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Test 19: Negative numbers round-trip to their exact value. This pins
// the sign handling in JReadNumber (the negate step) as caller-observable.
bool test_negative_number_exact_values(void) {
    WriteFmtLn("Testing negative number exact values");

    DefaultAllocator alloc   = DefaultAllocatorInit();
    bool             success = true;

    {
        Str     j   = StrInitFromZstr("-12345", &alloc);
        StrIter si  = StrIterFromStr(j);
        i64     v   = 0;
        StrIter out = JReadInteger(si, &v);
        if (!(StrIterIndex(&out) != StrIterIndex(&si) && v == -12345)) {
            WriteFmtLn("[DEBUG] JReadInteger '-12345' -> {}", v);
            success = false;
        }
        StrDeinit(&j);
    }
    {
        // Positive control: a sign-flip mutation would make this negative.
        Str     j   = StrInitFromZstr("12345", &alloc);
        StrIter si  = StrIterFromStr(j);
        i64     v   = 0;
        StrIter out = JReadInteger(si, &v);
        if (!(StrIterIndex(&out) != StrIterIndex(&si) && v == 12345)) {
            WriteFmtLn("[DEBUG] JReadInteger '12345' -> {}", v);
            success = false;
        }
        StrDeinit(&j);
    }
    {
        Str     j   = StrInitFromZstr("-2.5", &alloc);
        StrIter si  = StrIterFromStr(j);
        f64     v   = 0.0;
        StrIter out = JReadFloat(si, &v);
        if (!(StrIterIndex(&out) != StrIterIndex(&si) && v == -2.5)) {
            WriteFmtLn("[DEBUG] JReadFloat '-2.5' -> {}", v);
            success = false;
        }
        StrDeinit(&j);
    }

    DefaultAllocatorDeinit(&alloc);
    return success;
}

// JReadFloat on an INTEGER-valued JSON number must promote num.i to the
// exact f64 value. The non-float branch does `*val = (f64)num.i;`; a
// value-substitution mutant turning that into `*val = 42` would hand back
// 42.0 for any integer. Parse "7" and pin *val == 7.0 (and advance).
bool test_js_float_reads_integer_valued_number(void) {
    WriteFmtLn("Testing JReadFloat promotes an integer-valued number exactly");

    DefaultAllocator alloc   = DefaultAllocatorInit();
    bool             success = true;

    Str     j   = StrInitFromZstr("7", &alloc);
    StrIter si  = StrIterFromStr(j);
    f64     val = 0.0;
    StrIter out = JReadFloat(si, &val);

    // Must advance (token consumed) and yield exactly 7.0 -- not 42.0.
    if (StrIterIndex(&out) == StrIterIndex(&si)) {
        WriteFmtLn("[DEBUG] JReadFloat did not advance on integer '7'");
        success = false;
    }
    if (val != 7.0) {
        WriteFmtLn("[DEBUG] JReadFloat integer-promotion wrong: expected 7.0, got {}", val);
        success = false;
    }

    StrDeinit(&j);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Second integer-promotion witness with a different value, so the mutant
// cannot coincide with the literal. 13 -> 13.0, never 42.0.
bool test_js_float_reads_zero_valued_integer(void) {
    WriteFmtLn("Testing JReadFloat promotes a second integer value exactly");

    DefaultAllocator alloc   = DefaultAllocatorInit();
    bool             success = true;

    Str     j   = StrInitFromZstr("13", &alloc);
    StrIter si  = StrIterFromStr(j);
    f64     val = 0.0;
    StrIter out = JReadFloat(si, &val);

    if (StrIterIndex(&out) == StrIterIndex(&si)) {
        WriteFmtLn("[DEBUG] JReadFloat did not advance on integer '13'");
        success = false;
    }
    if (val != 13.0) {
        WriteFmtLn("[DEBUG] JReadFloat integer-promotion wrong: expected 13.0, got {}", val);
        success = false;
    }

    StrDeinit(&j);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// JReadNull writes `*is_null = false;` unconditionally before the match,
// so a NON-null token leaves *is_null == false. A value-substitution
// mutant turning that into `*is_null = 42` (truthy) would report a
// non-null value as null. Feed "null2" (a 'n...' token that is NOT
// exactly "null"): the reader fails (iterator rewinds) but *is_null must
// remain false -- assert it is NOT truthy.
bool test_js_null_clears_flag_on_non_null(void) {
    WriteFmtLn("Testing JReadNull clears is_null on a non-null token");

    DefaultAllocator alloc   = DefaultAllocatorInit();
    bool             success = true;

    // 'n' lead-in but not "null": ZstrCompareN fails -> *is_null stays the
    // value the unconditional clear set it to, which must be false.
    Str     j       = StrInitFromZstr("nuII", &alloc);
    StrIter si      = StrIterFromStr(j);
    bool    is_null = true; // poison: must be overwritten to false
    StrIter out     = JReadNull(si, &is_null);

    // Malformed -> iterator rewinds (no advance).
    if (StrIterIndex(&out) != StrIterIndex(&si)) {
        WriteFmtLn("[DEBUG] JReadNull advanced on malformed 'nuII'");
        success = false;
    }
    // The unconditional clear must have run: a truthy mutant fails here.
    if (is_null != false) {
        WriteFmtLn("[DEBUG] JReadNull left is_null truthy on non-null input: {}", is_null);
        success = false;
    }

    StrDeinit(&j);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Positive control: a real "null" token sets *is_null = true and advances
// four bytes. Pins the success branch alongside the clear above so a
// mutant cannot satisfy one test by sabotaging the other.
bool test_js_null_sets_flag_on_null(void) {
    WriteFmtLn("Testing JReadNull sets is_null on a real null token");

    DefaultAllocator alloc   = DefaultAllocatorInit();
    bool             success = true;

    Str     j       = StrInitFromZstr("null", &alloc);
    StrIter si      = StrIterFromStr(j);
    bool    is_null = false;
    StrIter out     = JReadNull(si, &is_null);

    if (!(is_null == true && StrIterIndex(&out) == 4)) {
        WriteFmtLn("[DEBUG] JReadNull 'null' value/advance wrong: n={}, idx={}", is_null, StrIterIndex(&out));
        success = false;
    }

    StrDeinit(&j);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Main function that runs all edge case reading tests
int main(void) {
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
        test_boundary_floats,
        test_scalar_readers_reject_malformed,
        test_scalar_readers_value_and_advance,
        test_truncated_unicode_escape_rejected,
        test_unknown_keys_of_every_type_skipped,
        test_malformed_object_rejected,
        test_negative_number_exact_values,
        test_js_float_reads_integer_valued_number,
        test_js_float_reads_zero_valued_integer,
        test_js_null_clears_flag_on_non_null,
        test_js_null_sets_flag_on_null
    };

    int test_count = sizeof(tests) / sizeof(tests[0]);

    // Use centralized test driver
    return run_test_suite(tests, test_count, NULL, 0, "Json.Read.EdgeCases");
}
