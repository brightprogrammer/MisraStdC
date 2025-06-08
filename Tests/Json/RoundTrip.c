#include <Misra/Parsers/JSON.h>
#include <Misra/Std/Io.h>
#include <Misra/Std/Log.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <Misra/Types.h>

// Include test utilities
#include "../Util/TestRunner.h"

// Test structures for round-trip testing
typedef struct TestPerson {
    u64  id;
    Str  name;
    u32  age;
    bool is_active;
    f64  salary;
} TestPerson;

typedef struct TestConfig {
    bool debug_mode;
    u32  timeout;
    Str  log_level;
    Vec(Str) features;
} TestConfig;

typedef struct ComplexData {
    TestPerson user;
    TestConfig config;
    Vec(i32) numbers;
    Vec(bool) flags;
} ComplexData;

// Cleanup functions
void TestPersonDeinit(TestPerson* person) {
    StrDeinit(&person->name);
}

void TestConfigDeinit(TestConfig* config) {
    StrDeinit(&config->log_level);
    VecDeinit(&config->features);
}

void ComplexDataDeinit(ComplexData* data) {
    TestPersonDeinit(&data->user);
    TestConfigDeinit(&data->config);
    VecDeinit(&data->numbers);
    VecDeinit(&data->flags);
}

// Function prototypes
bool test_simple_roundtrip(void);
bool test_numeric_roundtrip(void);
bool test_boolean_roundtrip(void);
bool test_string_roundtrip(void);
bool test_array_roundtrip(void);
bool test_nested_object_roundtrip(void);
bool test_complex_data_roundtrip(void);
bool test_empty_containers_roundtrip(void);
bool test_edge_cases_roundtrip(void);

// Helper function to compare persons
bool compare_persons(const TestPerson* a, const TestPerson* b) {
    return a->id == b->id && StrCmp(&a->name, &b->name) == 0 && a->age == b->age && a->is_active == b->is_active &&
           a->salary == b->salary;
}

// Helper function to compare configs
bool compare_configs(const TestConfig* a, const TestConfig* b) {
    if (a->debug_mode != b->debug_mode || a->timeout != b->timeout || StrCmp(&a->log_level, &b->log_level) != 0 ||
        VecLen(&a->features) != VecLen(&b->features)) {
        return false;
    }

    for (size i = 0; i < VecLen(&a->features); i++) {
        if (StrCmp(&VecAt(&a->features, i), &VecAt(&b->features, i)) != 0) {
            return false;
        }
    }
    return true;
}

// Test 1: Simple value round-trip
bool test_simple_roundtrip(void) {
    printf("Testing simple value round-trip\n");

    bool success = true;

    // Original data
    struct {
        i32  count;
        f64  temperature;
        bool enabled;
        Str  message;
    } original = {42, 25.5, true, StrInitFromZstr("hello world")};

    // Write to JSON
    Str json = StrInit();
    JW_OBJ(json, {
        JW_INT_KV(json, "count", original.count);
        JW_FLT_KV(json, "temperature", original.temperature);
        JW_BOOL_KV(json, "enabled", original.enabled);
        JW_STR_KV(json, "message", original.message);
    });

    printf("[DEBUG] Generated JSON: %s\n", json.data);

    // Read back from JSON
    struct {
        i32  count;
        f64  temperature;
        bool enabled;
        Str  message;
    } parsed = {0, 0.0, false, StrInit()};

    StrIter si = StrIterFromStr(json);
    JR_OBJ(si, {
        JR_INT_KV(si, "count", parsed.count);
        JR_FLT_KV(si, "temperature", parsed.temperature);
        JR_BOOL_KV(si, "enabled", parsed.enabled);
        JR_STR_KV(si, "message", parsed.message);
    });

    // Compare values
    if (original.count == parsed.count && original.temperature == parsed.temperature &&
        original.enabled == parsed.enabled && StrCmp(&original.message, &parsed.message) == 0) {
        printf("[DEBUG] Simple round-trip test passed\n");
    } else {
        printf("[DEBUG] Simple round-trip test FAILED\n");
        printf(
            "[DEBUG] Original: count=%d, temp=%f, enabled=%s, msg='%s'\n",
            original.count,
            original.temperature,
            original.enabled ? "true" : "false",
            original.message.data
        );
        printf(
            "[DEBUG] Parsed: count=%d, temp=%f, enabled=%s, msg='%s'\n",
            parsed.count,
            parsed.temperature,
            parsed.enabled ? "true" : "false",
            parsed.message.data
        );
        success = false;
    }

    StrDeinit(&json);
    StrDeinit(&original.message);
    StrDeinit(&parsed.message);
    return success;
}

// Test 2: Numeric precision round-trip
bool test_numeric_roundtrip(void) {
    printf("Testing numeric precision round-trip\n");

    bool success = true;

    // Original data with various numeric edge cases
    struct {
        i64 big_int;
        i64 negative_int;
        i64 zero_int;
        f64 precise_float;
        f64 small_float;
        f64 negative_float;
    } original = {9223372036854775807LL, -2147483648LL, 0, 3.141592653589793, 0.000001, -999.999};

    // Write to JSON
    Str json = StrInit();
    JW_OBJ(json, {
        JW_INT_KV(json, "big_int", original.big_int);
        JW_INT_KV(json, "negative_int", original.negative_int);
        JW_INT_KV(json, "zero_int", original.zero_int);
        JW_FLT_KV(json, "precise_float", original.precise_float);
        JW_FLT_KV(json, "small_float", original.small_float);
        JW_FLT_KV(json, "negative_float", original.negative_float);
    });

    printf("[DEBUG] Numeric JSON: %s\n", json.data);

    // Read back from JSON
    struct {
        i64 big_int;
        i64 negative_int;
        i64 zero_int;
        f64 precise_float;
        f64 small_float;
        f64 negative_float;
    } parsed = {0};

    StrIter si = StrIterFromStr(json);
    JR_OBJ(si, {
        JR_INT_KV(si, "big_int", parsed.big_int);
        JR_INT_KV(si, "negative_int", parsed.negative_int);
        JR_INT_KV(si, "zero_int", parsed.zero_int);
        JR_FLT_KV(si, "precise_float", parsed.precise_float);
        JR_FLT_KV(si, "small_float", parsed.small_float);
        JR_FLT_KV(si, "negative_float", parsed.negative_float);
    });

    // Compare values (allowing small float precision differences)
    bool ints_match =
        (original.big_int == parsed.big_int && original.negative_int == parsed.negative_int &&
         original.zero_int == parsed.zero_int);

    bool floats_match =
        (fabs(original.precise_float - parsed.precise_float) < 0.000001 &&
         fabs(original.small_float - parsed.small_float) < 0.0000001 &&
         fabs(original.negative_float - parsed.negative_float) < 0.001);

    if (ints_match && floats_match) {
        printf("[DEBUG] Numeric round-trip test passed\n");
    } else {
        printf("[DEBUG] Numeric round-trip test FAILED\n");
        printf("[DEBUG] Integers match: %s\n", ints_match ? "true" : "false");
        printf("[DEBUG] Floats match: %s\n", floats_match ? "true" : "false");
        success = false;
    }

    StrDeinit(&json);
    return success;
}

// Test 3: Boolean round-trip
bool test_boolean_roundtrip(void) {
    printf("Testing boolean round-trip\n");

    bool success = true;

    // Original data
    struct {
        bool flag1;
        bool flag2;
        bool flag3;
        bool flag4;
    } original = {true, false, true, false};

    // Write to JSON
    Str json = StrInit();
    JW_OBJ(json, {
        JW_BOOL_KV(json, "flag1", original.flag1);
        JW_BOOL_KV(json, "flag2", original.flag2);
        JW_BOOL_KV(json, "flag3", original.flag3);
        JW_BOOL_KV(json, "flag4", original.flag4);
    });

    // Read back from JSON
    struct {
        bool flag1;
        bool flag2;
        bool flag3;
        bool flag4;
    } parsed = {false, true, false, true}; // Initialize with opposite values

    StrIter si = StrIterFromStr(json);
    JR_OBJ(si, {
        JR_BOOL_KV(si, "flag1", parsed.flag1);
        JR_BOOL_KV(si, "flag2", parsed.flag2);
        JR_BOOL_KV(si, "flag3", parsed.flag3);
        JR_BOOL_KV(si, "flag4", parsed.flag4);
    });

    // Compare values
    if (original.flag1 == parsed.flag1 && original.flag2 == parsed.flag2 && original.flag3 == parsed.flag3 &&
        original.flag4 == parsed.flag4) {
        printf("[DEBUG] Boolean round-trip test passed\n");
    } else {
        printf("[DEBUG] Boolean round-trip test FAILED\n");
        success = false;
    }

    StrDeinit(&json);
    return success;
}

// Test 4: String round-trip
bool test_string_roundtrip(void) {
    printf("Testing string round-trip\n");

    bool success = true;

    // Original data with various string types
    struct {
        Str empty;
        Str simple;
        Str with_spaces;
        Str with_special;
    } original = {
        StrInit(),
        StrInitFromZstr("hello"),
        StrInitFromZstr("hello world with spaces"),
        StrInitFromZstr("special: !@#$%^&*()")
    };

    // Write to JSON
    Str json = StrInit();
    JW_OBJ(json, {
        JW_STR_KV(json, "empty", original.empty);
        JW_STR_KV(json, "simple", original.simple);
        JW_STR_KV(json, "with_spaces", original.with_spaces);
        JW_STR_KV(json, "with_special", original.with_special);
    });

    // Read back from JSON
    struct {
        Str empty;
        Str simple;
        Str with_spaces;
        Str with_special;
    } parsed = {StrInit(), StrInit(), StrInit(), StrInit()};

    StrIter si = StrIterFromStr(json);
    JR_OBJ(si, {
        JR_STR_KV(si, "empty", parsed.empty);
        JR_STR_KV(si, "simple", parsed.simple);
        JR_STR_KV(si, "with_spaces", parsed.with_spaces);
        JR_STR_KV(si, "with_special", parsed.with_special);
    });

    // Compare values
    if (parsed.empty.length == original.empty.length && StrCmp(&original.simple, &parsed.simple) == 0 &&
        StrCmp(&original.with_spaces, &parsed.with_spaces) == 0 &&
        StrCmp(&original.with_special, &parsed.with_special) == 0) {
        printf("[DEBUG] String round-trip test passed\n");
    } else {
        printf("[DEBUG] String round-trip test FAILED\n");
        success = false;
    }

    // Cleanup
    StrDeinit(&json);
    StrDeinit(&original.empty);
    StrDeinit(&original.simple);
    StrDeinit(&original.with_spaces);
    StrDeinit(&original.with_special);
    StrDeinit(&parsed.empty);
    StrDeinit(&parsed.simple);
    StrDeinit(&parsed.with_spaces);
    StrDeinit(&parsed.with_special);
    return success;
}

// Test 5: Array round-trip
bool test_array_roundtrip(void) {
    printf("Testing array round-trip\n");

    bool success = true;

    // Original data
    Vec(i32) original_numbers = VecInit();
    Vec(Str) original_strings = VecInitWithDeepCopy(NULL, StrDeinit);

    // Populate arrays
    i32 nums[5] = {1, 2, 3, -5, 0};
    for (size i = 0; i < 5; i++) {
        VecPushBack(&original_numbers, nums[i]);
    }

    // Create strings and push them properly
    Str str1 = StrInitFromZstr("first");
    Str str2 = StrInitFromZstr("second");
    Str str3 = StrInitFromZstr("");
    Str str4 = StrInitFromZstr("last");

    VecPushBack(&original_strings, str1);
    VecPushBack(&original_strings, str2);
    VecPushBack(&original_strings, str3);
    VecPushBack(&original_strings, str4);

    // Write to JSON
    Str json = StrInit();
    JW_OBJ(json, {
        JW_ARR_KV(json, "numbers", original_numbers, num, { JW_INT(json, num); });
        JW_ARR_KV(json, "strings", original_strings, str, { JW_STR(json, str); });
    });

    // Read back from JSON
    Vec(i32) parsed_numbers = VecInit();
    Vec(Str) parsed_strings = VecInitWithDeepCopy(NULL, StrDeinit);

    StrIter si = StrIterFromStr(json);
    JR_OBJ(si, {
        JR_ARR_KV(si, "numbers", {
            i32 num = 0;
            JR_INT(si, num);
            VecPushBack(&parsed_numbers, num);
        });
        JR_ARR_KV(si, "strings", {
            Str str = StrInit();
            JR_STR(si, str);
            VecPushBack(&parsed_strings, str);
        });
    });

    // Compare arrays
    bool numbers_match = (VecLen(&original_numbers) == VecLen(&parsed_numbers));
    if (numbers_match) {
        for (size i = 0; i < VecLen(&original_numbers); i++) {
            if (VecAt(&original_numbers, i) != VecAt(&parsed_numbers, i)) {
                numbers_match = false;
                break;
            }
        }
    }

    bool strings_match = (VecLen(&original_strings) == VecLen(&parsed_strings));
    if (strings_match) {
        for (size i = 0; i < VecLen(&original_strings); i++) {
            if (VecAt(&original_strings, i).length != VecAt(&parsed_strings, i).length ||
                (VecAt(&original_strings, i).length &&
                 StrCmp(&VecAt(&original_strings, i), &VecAt(&parsed_strings, i)) != 0)) {
                strings_match = false;
                break;
            }
        }
    }

    if (numbers_match && strings_match) {
        printf("[DEBUG] Array round-trip test passed\n");
    } else {
        printf("[DEBUG] Array round-trip test FAILED\n");
        printf(
            "[DEBUG] Numbers match: %s (orig %zu, parsed %zu)\n",
            numbers_match ? "true" : "false",
            VecLen(&original_numbers),
            VecLen(&parsed_numbers)
        );
        printf(
            "[DEBUG] Strings match: %s (orig %zu, parsed %zu)\n",
            strings_match ? "true" : "false",
            VecLen(&original_strings),
            VecLen(&parsed_strings)
        );
        success = false;
    }

    // Cleanup
    StrDeinit(&json);
    VecDeinit(&original_numbers);
    VecDeinit(&original_strings);
    VecDeinit(&parsed_numbers);
    VecDeinit(&parsed_strings);
    return success;
}

// Test 6: Nested object round-trip
bool test_nested_object_roundtrip(void) {
    printf("Testing nested object round-trip\n");

    bool success = true;

    // Original data
    TestPerson original_person = {12345, StrInitFromZstr("John Doe"), 30, true, 75000.50};

    // Write to JSON
    Str json = StrInit();
    JW_OBJ(json, {
        JW_OBJ_KV(json, "user", {
            JW_INT_KV(json, "id", original_person.id);
            JW_STR_KV(json, "name", original_person.name);
            JW_INT_KV(json, "age", original_person.age);
            JW_BOOL_KV(json, "is_active", original_person.is_active);
            JW_FLT_KV(json, "salary", original_person.salary);
        });
    });

    // Read back from JSON
    TestPerson parsed_person = {0, StrInit(), 0, false, 0.0};

    StrIter si = StrIterFromStr(json);
    JR_OBJ(si, {
        JR_OBJ_KV(si, "user", {
            JR_INT_KV(si, "id", parsed_person.id);
            JR_STR_KV(si, "name", parsed_person.name);
            JR_INT_KV(si, "age", parsed_person.age);
            JR_BOOL_KV(si, "is_active", parsed_person.is_active);
            JR_FLT_KV(si, "salary", parsed_person.salary);
        });
    });

    // Compare
    if (compare_persons(&original_person, &parsed_person)) {
        printf("[DEBUG] Nested object round-trip test passed\n");
    } else {
        printf("[DEBUG] Nested object round-trip test FAILED\n");
        success = false;
    }

    // Cleanup
    StrDeinit(&json);
    TestPersonDeinit(&original_person);
    TestPersonDeinit(&parsed_person);
    return success;
}

// Test 7: Complex data round-trip
bool test_complex_data_roundtrip(void) {
    printf("Testing complex data round-trip\n");

    bool success = true;

    // Create complex original data
    ComplexData original    = {0};
    original.user.id        = 999;
    original.user.name      = StrInitFromZstr("Complex User");
    original.user.age       = 25;
    original.user.is_active = true;
    original.user.salary    = 50000.0;

    original.config.debug_mode = false;
    original.config.timeout    = 30;
    original.config.log_level  = StrInitFromZstr("INFO");
    original.config.features   = VecInitWithDeepCopyT(original.config.features, NULL, StrDeinit);

    // Create strings and push them properly
    Str feature1 = StrInitFromZstr("auth");
    Str feature2 = StrInitFromZstr("logging");

    VecPushBack(&original.config.features, feature1);
    VecPushBack(&original.config.features, feature2);

    original.numbers = VecInitT(original.numbers);
    i32 vals[3]      = {10, 20, -5};
    for (size i = 0; i < 3; i++) {
        VecPushBack(&original.numbers, vals[i]);
    }

    original.flags = VecInitT(original.flags);
    bool bools[3]  = {true, false, true};
    for (size i = 0; i < 3; i++) {
        VecPushBack(&original.flags, bools[i]);
    }

    // Write to JSON
    Str json = StrInit();
    JW_OBJ(json, {
        JW_OBJ_KV(json, "user", {
            JW_INT_KV(json, "id", original.user.id);
            JW_STR_KV(json, "name", original.user.name);
            JW_INT_KV(json, "age", original.user.age);
            JW_BOOL_KV(json, "is_active", original.user.is_active);
            JW_FLT_KV(json, "salary", original.user.salary);
        });
        JW_OBJ_KV(json, "config", {
            JW_BOOL_KV(json, "debug_mode", original.config.debug_mode);
            JW_INT_KV(json, "timeout", original.config.timeout);
            JW_STR_KV(json, "log_level", original.config.log_level);
            JW_ARR_KV(json, "features", original.config.features, feature, { JW_STR(json, feature); });
        });
        JW_ARR_KV(json, "numbers", original.numbers, num, { JW_INT(json, num); });
        JW_ARR_KV(json, "flags", original.flags, flag, { JW_BOOL(json, flag); });
    });

    printf("[DEBUG] Complex JSON length: %zu\n", json.length);

    // Read back from JSON
    ComplexData parsed       = {0};
    parsed.user              = (TestPerson) {0, StrInit(), 0, false, 0.0};
    parsed.config.debug_mode = true; // opposite of original
    parsed.config.timeout    = 0;
    parsed.config.log_level  = StrInit();
    parsed.config.features   = VecInitWithDeepCopyT(parsed.config.features, NULL, StrDeinit);
    parsed.numbers           = VecInitT(parsed.numbers);
    parsed.flags             = VecInitT(parsed.flags);

    StrIter si = StrIterFromStr(json);
    JR_OBJ(si, {
        JR_OBJ_KV(si, "user", {
            JR_INT_KV(si, "id", parsed.user.id);
            JR_STR_KV(si, "name", parsed.user.name);
            JR_INT_KV(si, "age", parsed.user.age);
            JR_BOOL_KV(si, "is_active", parsed.user.is_active);
            JR_FLT_KV(si, "salary", parsed.user.salary);
        });
        JR_OBJ_KV(si, "config", {
            JR_BOOL_KV(si, "debug_mode", parsed.config.debug_mode);
            JR_INT_KV(si, "timeout", parsed.config.timeout);
            JR_STR_KV(si, "log_level", parsed.config.log_level);
            JR_ARR_KV(si, "features", {
                Str feature = StrInit();
                JR_STR(si, feature);
                VecPushBack(&parsed.config.features, feature);
            });
        });
        JR_ARR_KV(si, "numbers", {
            i32 num = 0;
            JR_INT(si, num);
            VecPushBack(&parsed.numbers, num);
        });
        JR_ARR_KV(si, "flags", {
            bool flag = false;
            JR_BOOL(si, flag);
            VecPushBack(&parsed.flags, flag);
        });
    });

    // Compare complex data
    bool user_match   = compare_persons(&original.user, &parsed.user);
    bool config_match = compare_configs(&original.config, &parsed.config);

    bool numbers_match = (VecLen(&original.numbers) == VecLen(&parsed.numbers));
    if (numbers_match) {
        for (size i = 0; i < VecLen(&original.numbers); i++) {
            if (VecAt(&original.numbers, i) != VecAt(&parsed.numbers, i)) {
                numbers_match = false;
                break;
            }
        }
    }

    bool flags_match = (VecLen(&original.flags) == VecLen(&parsed.flags));
    if (flags_match) {
        for (size i = 0; i < VecLen(&original.flags); i++) {
            if (VecAt(&original.flags, i) != VecAt(&parsed.flags, i)) {
                flags_match = false;
                break;
            }
        }
    }

    if (user_match && config_match && numbers_match && flags_match) {
        printf("[DEBUG] Complex data round-trip test passed\n");
    } else {
        printf("[DEBUG] Complex data round-trip test FAILED\n");
        printf("[DEBUG] User match: %s\n", user_match ? "true" : "false");
        printf("[DEBUG] Config match: %s\n", config_match ? "true" : "false");
        printf("[DEBUG] Numbers match: %s\n", numbers_match ? "true" : "false");
        printf("[DEBUG] Flags match: %s\n", flags_match ? "true" : "false");
        success = false;
    }

    // Cleanup
    StrDeinit(&json);
    ComplexDataDeinit(&original);
    ComplexDataDeinit(&parsed);
    return success;
}

// Test 8: Empty containers round-trip
bool test_empty_containers_roundtrip(void) {
    printf("Testing empty containers round-trip\n");

    bool success = true;

    // Original empty data
    Vec(i32) empty_numbers = VecInit();
    Vec(Str) empty_strings = VecInitWithDeepCopy(NULL, StrDeinit);
    Str empty_str          = StrInit();

    // Write to JSON
    Str json = StrInit();
    JW_OBJ(json, {
        JW_STR_KV(json, "empty_string", empty_str);
        JW_ARR_KV(json, "empty_numbers", empty_numbers, num, { JW_INT(json, num); });
        JW_ARR_KV(json, "empty_strings", empty_strings, str, { JW_STR(json, str); });
        JW_OBJ_KV(
            json,
            "empty_object",
            {
                // Empty nested object
            }
        );
    });

    // Read back from JSON
    Vec(i32) parsed_numbers = VecInit();
    Vec(Str) parsed_strings = VecInitWithDeepCopy(NULL, StrDeinit);
    Str  parsed_str         = StrInit();
    bool found_empty_object = false;

    StrIter si = StrIterFromStr(json);
    JR_OBJ(si, {
        JR_STR_KV(si, "empty_string", parsed_str);
        JR_ARR_KV(si, "empty_numbers", {
            i32 num = 0;
            JR_INT(si, num);
            VecPushBack(&parsed_numbers, num);
        });
        JR_ARR_KV(si, "empty_strings", {
            Str str = StrInit();
            JR_STR(si, str);
            VecPushBack(&parsed_strings, str);
        });
        JR_OBJ_KV(si, "empty_object", { found_empty_object = true; });
    });

    // Compare empty containers
    if (parsed_str.length == 0 && VecLen(&parsed_numbers) == 0 && VecLen(&parsed_strings) == 0 &&
        !found_empty_object) { // Empty object should not execute the content
        printf("[DEBUG] Empty containers round-trip test passed\n");
    } else {
        printf("[DEBUG] Empty containers round-trip test FAILED\n");
        printf(
            "[DEBUG] String length: %zu, numbers: %zu, strings: %zu, found_obj: %s\n",
            parsed_str.length,
            VecLen(&parsed_numbers),
            VecLen(&parsed_strings),
            found_empty_object ? "true" : "false"
        );
        success = false;
    }

    // Cleanup
    StrDeinit(&json);
    StrDeinit(&empty_str);
    StrDeinit(&parsed_str);
    VecDeinit(&empty_numbers);
    VecDeinit(&empty_strings);
    VecDeinit(&parsed_numbers);
    VecDeinit(&parsed_strings);
    return success;
}

// Test 9: Edge cases round-trip
bool test_edge_cases_roundtrip(void) {
    printf("Testing edge cases round-trip\n");

    bool success = true;

    // Edge case data
    struct {
        i64  max_int;
        i64  min_int;
        i64  zero;
        f64  zero_float;
        bool true_val;
        bool false_val;
    } original = {2147483647LL, -2147483648LL, 0, 0.0, true, false};

    // Write to JSON
    Str json = StrInit();
    JW_OBJ(json, {
        JW_INT_KV(json, "max_int", original.max_int);
        JW_INT_KV(json, "min_int", original.min_int);
        JW_INT_KV(json, "zero", original.zero);
        JW_FLT_KV(json, "zero_float", original.zero_float);
        JW_BOOL_KV(json, "true_val", original.true_val);
        JW_BOOL_KV(json, "false_val", original.false_val);
    });

    // Read back from JSON
    struct {
        i64  max_int;
        i64  min_int;
        i64  zero;
        f64  zero_float;
        bool true_val;
        bool false_val;
    } parsed = {0, 0, 999, 999.0, false, true}; // Initialize with different values

    StrIter si = StrIterFromStr(json);
    JR_OBJ(si, {
        JR_INT_KV(si, "max_int", parsed.max_int);
        JR_INT_KV(si, "min_int", parsed.min_int);
        JR_INT_KV(si, "zero", parsed.zero);
        JR_FLT_KV(si, "zero_float", parsed.zero_float);
        JR_BOOL_KV(si, "true_val", parsed.true_val);
        JR_BOOL_KV(si, "false_val", parsed.false_val);
    });

    // Compare edge cases
    if (original.max_int == parsed.max_int && original.min_int == parsed.min_int && original.zero == parsed.zero &&
        original.zero_float == parsed.zero_float && original.true_val == parsed.true_val &&
        original.false_val == parsed.false_val) {
        printf("[DEBUG] Edge cases round-trip test passed\n");
    } else {
        printf("[DEBUG] Edge cases round-trip test FAILED\n");
        success = false;
    }

    StrDeinit(&json);
    return success;
}

// Main function that runs all round-trip tests
int main(int argc, char* argv[]) {
    // Array of test functions
    TestFunction tests[] = {
        test_simple_roundtrip,
        test_numeric_roundtrip,
        test_boolean_roundtrip,
        test_string_roundtrip,
        test_array_roundtrip,
        test_nested_object_roundtrip,
        test_complex_data_roundtrip,
        test_empty_containers_roundtrip,
        test_edge_cases_roundtrip
    };

    int test_count = sizeof(tests) / sizeof(tests[0]);

    // Use centralized test driver
    return run_test_suite(tests, test_count, NULL, 0, "Json.RoundTrip");
}
