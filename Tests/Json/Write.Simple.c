#include <Misra/Parsers/JSON.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Io.h>
#include <Misra/Std/Log.h>
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
void PersonDeinit(Person *person) {
    StrDeinit(&person->name);
}

void ConfigDeinit(Config *config) {
    StrDeinit(&config->log_level);
}

void SimpleProductDeinit(SimpleProduct *product) {
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
        WriteFmt("[DEBUG] JSON comparison failed\n");
        WriteFmt("[DEBUG] Expected: '");
        for (size i = 0; i < StrLen(&expected_clean); i++) {
            WriteFmt("{c}", StrBegin(&expected_clean)[i]);
        }
        WriteFmt("'\n");
        WriteFmt("[DEBUG] Got: '");
        for (size i = 0; i < StrLen(&output_clean); i++) {
            WriteFmt("{c}", StrBegin(&output_clean)[i]);
        }
        WriteFmt("'\n");
    }

    StrDeinit(&expected_str);
    StrDeinit(&output_clean);
    StrDeinit(&expected_clean);
    return result;
}

// Test 1: Simple string writing
bool test_simple_string_writing(void) {
    WriteFmt("Testing simple string writing\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    bool success = true;
    Str  json    = StrInit(&alloc);

    Str name = StrInitFromZstr("Alice", &alloc);
    Str city = StrInitFromZstr("New York", &alloc);

    JW_OBJ(json, {
        JW_STR_KV(json, "name", name);
        JW_STR_KV(json, "city", city);
    });

    Zstr expected = "{\"name\":\"Alice\",\"city\":\"New York\"}";
    if (!compare_json_output(&json, expected, &alloc)) {
        success = false;
    }

    StrDeinit(&json);
    StrDeinit(&name);
    StrDeinit(&city);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Test 2: Simple number writing
bool test_simple_numbers_writing(void) {
    WriteFmt("Testing simple number writing\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    bool success = true;
    Str  json    = StrInit(&alloc);

    u32 count = 42;
    f64 score = 95.5;
    u32 year  = 2024;

    JW_OBJ(json, {
        JW_INT_KV(json, "count", count);
        JW_FLT_KV(json, "score", score);
        JW_INT_KV(json, "year", year);
    });

    Zstr expected = "{\"count\":42,\"score\":95.500000,\"year\":2024}";
    if (!compare_json_output(&json, expected, &alloc)) {
        success = false;
    }

    StrDeinit(&json);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Test 3: Simple boolean writing
bool test_simple_boolean_writing(void) {
    WriteFmt("Testing simple boolean writing\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    bool success = true;
    Str  json    = StrInit(&alloc);

    bool enabled = true;
    bool visible = false;

    JW_OBJ(json, {
        JW_BOOL_KV(json, "enabled", enabled);
        JW_BOOL_KV(json, "visible", visible);
    });

    Zstr expected = "{\"enabled\":true,\"visible\":false}";
    if (!compare_json_output(&json, expected, &alloc)) {
        success = false;
    }

    StrDeinit(&json);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Test 4: Simple person object writing
bool test_simple_person_object_writing(void) {
    WriteFmt("Testing simple person object writing\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    bool success = true;
    Str  json    = StrInit(&alloc);

    Person person = {1001, StrInitFromZstr("Bob", &alloc), 25, true, 50000.0};

    JW_OBJ(json, {
        JW_INT_KV(json, "id", person.id);
        JW_STR_KV(json, "name", person.name);
        JW_INT_KV(json, "age", person.age);
        JW_BOOL_KV(json, "is_active", person.is_active);
        JW_FLT_KV(json, "salary", person.salary);
    });

    Zstr expected = "{\"id\":1001,\"name\":\"Bob\",\"age\":25,\"is_active\":true,\"salary\":50000.000000}";
    if (!compare_json_output(&json, expected, &alloc)) {
        success = false;
    }

    StrDeinit(&json);
    PersonDeinit(&person);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Test 5: Simple config object writing
bool test_simple_config_object_writing(void) {
    WriteFmt("Testing simple config object writing\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    bool success = true;
    Str  json    = StrInit(&alloc);

    Config config = {false, 30, StrInitFromZstr("INFO", &alloc)};

    JW_OBJ(json, {
        JW_BOOL_KV(json, "debug_mode", config.debug_mode);
        JW_INT_KV(json, "timeout", config.timeout);
        JW_STR_KV(json, "log_level", config.log_level);
    });

    Zstr expected = "{\"debug_mode\":false,\"timeout\":30,\"log_level\":\"INFO\"}";
    if (!compare_json_output(&json, expected, &alloc)) {
        success = false;
    }

    StrDeinit(&json);
    ConfigDeinit(&config);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Test 6: Simple array of strings writing
bool test_simple_array_of_strings_writing(void) {
    WriteFmt("Testing simple array of strings writing\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    bool success = true;
    Str  json    = StrInit(&alloc);

    Vec(Str) languages = VecInitWithDeepCopy(NULL, StrDeinit, &alloc);

    // Create strings and push them properly
    Str lang1 = StrInitFromZstr("C", &alloc);
    Str lang2 = StrInitFromZstr("Python", &alloc);
    Str lang3 = StrInitFromZstr("Rust", &alloc);

    VecPushBack(&languages, lang1);
    VecPushBack(&languages, lang2);
    VecPushBack(&languages, lang3);

    JW_OBJ(json, { JW_ARR_KV(json, "languages", languages, lang, { JW_STR(json, lang); }); });

    Zstr expected = "{\"languages\":[\"C\",\"Python\",\"Rust\"]}";
    if (!compare_json_output(&json, expected, &alloc)) {
        success = false;
    }

    StrDeinit(&json);
    VecDeinit(&languages);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Test 7: Simple nested object writing
bool test_simple_nested_object_writing(void) {
    WriteFmt("Testing simple nested object writing\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    bool success = true;
    Str  json    = StrInit(&alloc);

    struct {
        struct {
            Str name;
            Str email;
        } user;
        bool active;
    } data = {
        {StrInitFromZstr("Charlie", &alloc), StrInitFromZstr("charlie@example.com", &alloc)},
        true
    };

    JW_OBJ(json, {
        JW_OBJ_KV(json, "user", {
            JW_STR_KV(json, "name", data.user.name);
            JW_STR_KV(json, "email", data.user.email);
        });
        JW_BOOL_KV(json, "active", data.active);
    });

    Zstr expected = "{\"user\":{\"name\":\"Charlie\",\"email\":\"charlie@example.com\"},\"active\":true}";
    if (!compare_json_output(&json, expected, &alloc)) {
        success = false;
    }

    StrDeinit(&json);
    StrDeinit(&data.user.name);
    StrDeinit(&data.user.email);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Test 8: Simple product with tags array writing
bool test_simple_product_with_tags_writing(void) {
    WriteFmt("Testing simple product with tags array writing\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    bool success = true;
    Str  json    = StrInit(&alloc);

    SimpleProduct product = {0};
    product.id            = 12345;
    product.name          = StrInitFromZstr("Laptop", &alloc);
    product.price         = 999.99;
    product.tags          = VecInitWithDeepCopyT(product.tags, NULL, StrDeinit, &alloc);

    // Create strings and push them properly
    Str tag1 = StrInitFromZstr("electronics", &alloc);
    Str tag2 = StrInitFromZstr("computers", &alloc);
    Str tag3 = StrInitFromZstr("portable", &alloc);

    VecPushBack(&product.tags, tag1);
    VecPushBack(&product.tags, tag2);
    VecPushBack(&product.tags, tag3);

    JW_OBJ(json, {
        JW_INT_KV(json, "id", product.id);
        JW_STR_KV(json, "name", product.name);
        JW_FLT_KV(json, "price", product.price);
        JW_ARR_KV(json, "tags", product.tags, tag, { JW_STR(json, tag); });
    });

    Zstr expected =
        "{\"id\":12345,\"name\":\"Laptop\",\"price\":999.990000,\"tags\":[\"electronics\",\"computers\",\"portable\"]}";
    if (!compare_json_output(&json, expected, &alloc)) {
        success = false;
    }

    StrDeinit(&json);
    SimpleProductDeinit(&product);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Main function that runs all simple writing tests
int main(int argc, char *argv[]) {
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
