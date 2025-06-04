#include <Misra/Parsers/JSON.h>
#include <Misra/Std/Io.h>
#include <Misra/Std/Log.h>
#include <stdio.h>
#include <stdlib.h>
#include <Misra/Types.h>

// Include test utilities
#include "../Util/TestRunner.h"

// Simple data structures for basic JSON writing examples
typedef struct Person {
    u64  id;
    Str  name;
    u32  age;
    bool is_active;
    f64  salary;
} Person;

typedef struct Config {
    bool debug_mode;
    u32  timeout;
    Str  log_level;
} Config;

typedef struct SimpleProduct {
    u64 id;
    Str name;
    f64 price;
    Vec(Str) tags;
} SimpleProduct;

// Cleanup functions
void PersonDeinit(Person* person) {
    StrDeinit(&person->name);
}

void ConfigDeinit(Config* config) {
    StrDeinit(&config->log_level);
}

void SimpleProductDeinit(SimpleProduct* product) {
    StrDeinit(&product->name);
    VecDeinit(&product->tags);
}

// Function prototypes
bool test_simple_string_writing(void);
bool test_simple_numbers_writing(void);
bool test_simple_boolean_writing(void);
bool test_simple_person_object_writing(void);
bool test_simple_config_object_writing(void);
bool test_simple_array_of_strings_writing(void);
bool test_simple_nested_object_writing(void);
bool test_simple_product_with_tags_writing(void);

// Helper function to compare expected JSON strings (removes spaces for comparison)
bool compare_json_output(const Str* output, const char* expected) {
    // Create a copy of expected without spaces for comparison
    Str expected_str   = StrInitFromZstr(expected);
    Str output_clean   = StrInit();
    Str expected_clean = StrInit();

    // Remove spaces and newlines from both strings for comparison
    for (size i = 0; i < output->length; i++) {
        char c = output->data[i];
        if (c != ' ' && c != '\n' && c != '\r' && c != '\t') {
            StrPushBack(&output_clean, c);
        }
    }

    for (size i = 0; i < expected_str.length; i++) {
        char c = expected_str.data[i];
        if (c != ' ' && c != '\n' && c != '\r' && c != '\t') {
            StrPushBack(&expected_clean, c);
        }
    }

    bool result = StrCmp(&output_clean, &expected_clean) == 0;

    if (!result) {
        printf("[DEBUG] JSON comparison failed\n");
        printf("[DEBUG] Expected: '");
        for (size i = 0; i < expected_clean.length; i++) {
            printf("%c", expected_clean.data[i]);
        }
        printf("'\n");
        printf("[DEBUG] Got: '");
        for (size i = 0; i < output_clean.length; i++) {
            printf("%c", output_clean.data[i]);
        }
        printf("'\n");
    }

    StrDeinit(&expected_str);
    StrDeinit(&output_clean);
    StrDeinit(&expected_clean);
    return result;
}

// Test 1: Simple string writing
bool test_simple_string_writing(void) {
    printf("Testing simple string writing\n");

    bool success = true;
    Str  json    = StrInit();

    Str name = StrInitFromZstr("Alice");
    Str city = StrInitFromZstr("New York");

    JW_OBJ(json, {
        JW_STR_KV(json, "name", name);
        JW_STR_KV(json, "city", city);
    });

    const char* expected = "{\"name\":\"Alice\",\"city\":\"New York\"}";
    if (!compare_json_output(&json, expected)) {
        success = false;
    }

    StrDeinit(&json);
    StrDeinit(&name);
    StrDeinit(&city);
    return success;
}

// Test 2: Simple number writing
bool test_simple_numbers_writing(void) {
    printf("Testing simple number writing\n");

    bool success = true;
    Str  json    = StrInit();

    u32 count = 42;
    f64 score = 95.5;
    u32 year  = 2024;

    JW_OBJ(json, {
        JW_INT_KV(json, "count", count);
        JW_FLT_KV(json, "score", score);
        JW_INT_KV(json, "year", year);
    });

    const char* expected = "{\"count\":42,\"score\":95.500000,\"year\":2024}";
    if (!compare_json_output(&json, expected)) {
        success = false;
    }

    StrDeinit(&json);
    return success;
}

// Test 3: Simple boolean writing
bool test_simple_boolean_writing(void) {
    printf("Testing simple boolean writing\n");

    bool success = true;
    Str  json    = StrInit();

    bool enabled = true;
    bool visible = false;

    JW_OBJ(json, {
        JW_BOOL_KV(json, "enabled", enabled);
        JW_BOOL_KV(json, "visible", visible);
    });

    const char* expected = "{\"enabled\":true,\"visible\":false}";
    if (!compare_json_output(&json, expected)) {
        success = false;
    }

    StrDeinit(&json);
    return success;
}

// Test 4: Simple person object writing
bool test_simple_person_object_writing(void) {
    printf("Testing simple person object writing\n");

    bool success = true;
    Str  json    = StrInit();

    Person person = {1001, StrInitFromZstr("Bob"), 25, true, 50000.0};

    JW_OBJ(json, {
        JW_INT_KV(json, "id", person.id);
        JW_STR_KV(json, "name", person.name);
        JW_INT_KV(json, "age", person.age);
        JW_BOOL_KV(json, "is_active", person.is_active);
        JW_FLT_KV(json, "salary", person.salary);
    });

    const char* expected = "{\"id\":1001,\"name\":\"Bob\",\"age\":25,\"is_active\":true,\"salary\":50000.000000}";
    if (!compare_json_output(&json, expected)) {
        success = false;
    }

    StrDeinit(&json);
    PersonDeinit(&person);
    return success;
}

// Test 5: Simple config object writing
bool test_simple_config_object_writing(void) {
    printf("Testing simple config object writing\n");

    bool success = true;
    Str  json    = StrInit();

    Config config = {false, 30, StrInitFromZstr("INFO")};

    JW_OBJ(json, {
        JW_BOOL_KV(json, "debug_mode", config.debug_mode);
        JW_INT_KV(json, "timeout", config.timeout);
        JW_STR_KV(json, "log_level", config.log_level);
    });

    const char* expected = "{\"debug_mode\":false,\"timeout\":30,\"log_level\":\"INFO\"}";
    if (!compare_json_output(&json, expected)) {
        success = false;
    }

    StrDeinit(&json);
    ConfigDeinit(&config);
    return success;
}

// Test 6: Simple array of strings writing
bool test_simple_array_of_strings_writing(void) {
    printf("Testing simple array of strings writing\n");

    bool success = true;
    Str  json    = StrInit();

    Vec(Str) languages = VecInitWithDeepCopy(NULL, StrDeinit);

    // Create strings and push them properly
    Str lang1 = StrInitFromZstr("C");
    Str lang2 = StrInitFromZstr("Python");
    Str lang3 = StrInitFromZstr("Rust");

    VecPushBack(&languages, lang1);
    VecPushBack(&languages, lang2);
    VecPushBack(&languages, lang3);

    JW_OBJ(json, { JW_ARR_KV(json, "languages", languages, lang, { JW_STR(json, lang); }); });

    const char* expected = "{\"languages\":[\"C\",\"Python\",\"Rust\"]}";
    if (!compare_json_output(&json, expected)) {
        success = false;
    }

    StrDeinit(&json);
    VecDeinit(&languages);
    return success;
}

// Test 7: Simple nested object writing
bool test_simple_nested_object_writing(void) {
    printf("Testing simple nested object writing\n");

    bool success = true;
    Str  json    = StrInit();

    struct {
        struct {
            Str name;
            Str email;
        } user;
        bool active;
    } data = {
        {StrInitFromZstr("Charlie"), StrInitFromZstr("charlie@example.com")},
        true
    };

    JW_OBJ(json, {
        JW_OBJ_KV(json, "user", {
            JW_STR_KV(json, "name", data.user.name);
            JW_STR_KV(json, "email", data.user.email);
        });
        JW_BOOL_KV(json, "active", data.active);
    });

    const char* expected = "{\"user\":{\"name\":\"Charlie\",\"email\":\"charlie@example.com\"},\"active\":true}";
    if (!compare_json_output(&json, expected)) {
        success = false;
    }

    StrDeinit(&json);
    StrDeinit(&data.user.name);
    StrDeinit(&data.user.email);
    return success;
}

// Test 8: Simple product with tags array writing
bool test_simple_product_with_tags_writing(void) {
    printf("Testing simple product with tags array writing\n");

    bool success = true;
    Str  json    = StrInit();

    SimpleProduct product = {0};
    product.id            = 12345;
    product.name          = StrInitFromZstr("Laptop");
    product.price         = 999.99;
    product.tags          = VecInitWithDeepCopyT(product.tags, NULL, StrDeinit);

    // Create strings and push them properly
    Str tag1 = StrInitFromZstr("electronics");
    Str tag2 = StrInitFromZstr("computers");
    Str tag3 = StrInitFromZstr("portable");

    VecPushBack(&product.tags, tag1);
    VecPushBack(&product.tags, tag2);
    VecPushBack(&product.tags, tag3);

    JW_OBJ(json, {
        JW_INT_KV(json, "id", product.id);
        JW_STR_KV(json, "name", product.name);
        JW_FLT_KV(json, "price", product.price);
        JW_ARR_KV(json, "tags", product.tags, tag, { JW_STR(json, tag); });
    });

    const char* expected =
        "{\"id\":12345,\"name\":\"Laptop\",\"price\":999.990000,\"tags\":[\"electronics\",\"computers\",\"portable\"]}";
    if (!compare_json_output(&json, expected)) {
        success = false;
    }

    StrDeinit(&json);
    SimpleProductDeinit(&product);
    return success;
}

// Main function that runs all simple writing tests
int main(int argc, char* argv[]) {
    // Array of test functions
    TestFunction tests[] = {
        test_simple_string_writing,
        test_simple_numbers_writing,
        test_simple_boolean_writing,
        test_simple_person_object_writing,
        test_simple_config_object_writing,
        test_simple_array_of_strings_writing,
        test_simple_nested_object_writing,
        test_simple_product_with_tags_writing
    };

    int test_count = sizeof(tests) / sizeof(tests[0]);

    // Use centralized test driver
    return run_test_suite(tests, test_count, NULL, 0, "Json.Write.Simple");
}