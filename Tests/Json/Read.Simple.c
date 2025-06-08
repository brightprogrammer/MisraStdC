#include <Misra/Parsers/JSON.h>
#include <Misra/Std/Io.h>
#include <Misra/Std/Log.h>
#include <stdio.h>
#include <stdlib.h>
#include <Misra/Types.h>

// Include test utilities
#include "../Util/TestRunner.h"

// Simple data structures for basic JSON parsing examples
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
bool test_simple_string_parsing(void);
bool test_simple_numbers(void);
bool test_simple_boolean(void);
bool test_simple_person_object(void);
bool test_simple_config_object(void);
bool test_simple_array_of_strings(void);
bool test_simple_nested_object(void);
bool test_simple_product_with_tags(void);

// Test 1: Simple string parsing
bool test_simple_string_parsing(void) {
    printf("Testing simple string parsing\n");

    bool    success = true;
    Str     json    = StrInitFromZstr("{\"name\": \"Alice\", \"city\": \"New York\"}");
    StrIter si      = StrIterFromStr(json);

    Str name = StrInit();
    Str city = StrInit();

    JR_OBJ(si, {
        JR_STR_KV(si, "name", name);
        JR_STR_KV(si, "city", city);
    });

    if (StrCmpCstr(&name, "Alice", 5) != 0) {
        printf("[DEBUG] Name check failed: expected 'Alice', got '");
        for (size i = 0; i < name.length; i++) {
            printf("%c", name.data[i]);
        }
        printf("'\n");
        success = false;
    }

    if (StrCmpCstr(&city, "New York", 8) != 0) {
        printf("[DEBUG] City check failed: expected 'New York', got '");
        for (size i = 0; i < city.length; i++) {
            printf("%c", city.data[i]);
        }
        printf("'\n");
        success = false;
    }

    StrDeinit(&json);
    StrDeinit(&name);
    StrDeinit(&city);
    return success;
}

// Test 2: Simple number parsing
bool test_simple_numbers(void) {
    printf("Testing simple number parsing\n");

    bool    success = true;
    Str     json    = StrInitFromZstr("{\"count\": 42, \"score\": 95.5, \"year\": 2024}");
    StrIter si      = StrIterFromStr(json);

    u32 count = 0;
    f64 score = 0.0;
    u32 year  = 0;

    JR_OBJ(si, {
        JR_INT_KV(si, "count", count);
        JR_FLT_KV(si, "score", score);
        JR_INT_KV(si, "year", year);
    });

    if (count != 42) {
        printf("[DEBUG] Count check failed: expected 42, got %u\n", count);
        success = false;
    }

    if (!(score > 95.4 && score < 95.6)) {
        printf("[DEBUG] Score check failed: expected ~95.5, got %f\n", score);
        success = false;
    }

    if (year != 2024) {
        printf("[DEBUG] Year check failed: expected 2024, got %u\n", year);
        success = false;
    }

    StrDeinit(&json);
    return success;
}

// Test 3: Simple boolean parsing
bool test_simple_boolean(void) {
    printf("Testing simple boolean parsing\n");

    bool    success = true;
    Str     json    = StrInitFromZstr("{\"enabled\": true, \"visible\": false}");
    StrIter si      = StrIterFromStr(json);

    bool enabled = false;
    bool visible = true;

    JR_OBJ(si, {
        JR_BOOL_KV(si, "enabled", enabled);
        JR_BOOL_KV(si, "visible", visible);
    });

    if (enabled != true) {
        printf("[DEBUG] Enabled check failed: expected true, got %s\n", enabled ? "true" : "false");
        success = false;
    }

    if (visible != false) {
        printf("[DEBUG] Visible check failed: expected false, got %s\n", visible ? "true" : "false");
        success = false;
    }

    StrDeinit(&json);
    return success;
}

// Test 4: Simple person object
bool test_simple_person_object(void) {
    printf("Testing simple person object\n");

    bool success = true;
    Str  json =
        StrInitFromZstr("{\"id\": 1001, \"name\": \"Bob\", \"age\": 25, \"is_active\": true, \"salary\": 50000.0}");
    StrIter si = StrIterFromStr(json);

    Person person = {0};
    person.name   = StrInit();

    JR_OBJ(si, {
        JR_INT_KV(si, "id", person.id);
        JR_STR_KV(si, "name", person.name);
        JR_INT_KV(si, "age", person.age);
        JR_BOOL_KV(si, "is_active", person.is_active);
        JR_FLT_KV(si, "salary", person.salary);
    });

    if (person.id != 1001) {
        printf("[DEBUG] Person ID check failed: expected 1001, got %llu\n", person.id);
        success = false;
    }

    if (StrCmpCstr(&person.name, "Bob", 3) != 0) {
        printf("[DEBUG] Person name check failed: expected 'Bob', got '");
        for (size i = 0; i < person.name.length; i++) {
            printf("%c", person.name.data[i]);
        }
        printf("'\n");
        success = false;
    }

    if (person.age != 25) {
        printf("[DEBUG] Person age check failed: expected 25, got %u\n", person.age);
        success = false;
    }

    if (person.is_active != true) {
        printf("[DEBUG] Person is_active check failed: expected true, got %s\n", person.is_active ? "true" : "false");
        success = false;
    }

    if (!(person.salary > 49999.0 && person.salary < 50001.0)) {
        printf("[DEBUG] Person salary check failed: expected ~50000.0, got %f\n", person.salary);
        success = false;
    }

    StrDeinit(&json);
    PersonDeinit(&person);
    return success;
}

// Test 5: Simple config object
bool test_simple_config_object(void) {
    printf("Testing simple config object\n");

    bool    success = true;
    Str     json    = StrInitFromZstr("{\"debug_mode\": false, \"timeout\": 30, \"log_level\": \"INFO\"}");
    StrIter si      = StrIterFromStr(json);

    Config config    = {0};
    config.log_level = StrInit();

    JR_OBJ(si, {
        JR_BOOL_KV(si, "debug_mode", config.debug_mode);
        JR_INT_KV(si, "timeout", config.timeout);
        JR_STR_KV(si, "log_level", config.log_level);
    });

    if (config.debug_mode != false) {
        printf("[DEBUG] Debug mode check failed: expected false, got %s\n", config.debug_mode ? "true" : "false");
        success = false;
    }

    if (config.timeout != 30) {
        printf("[DEBUG] Timeout check failed: expected 30, got %u\n", config.timeout);
        success = false;
    }

    if (StrCmpCstr(&config.log_level, "INFO", 4) != 0) {
        printf("[DEBUG] Log level check failed: expected 'INFO', got '");
        for (size i = 0; i < config.log_level.length; i++) {
            printf("%c", config.log_level.data[i]);
        }
        printf("'\n");
        success = false;
    }

    StrDeinit(&json);
    ConfigDeinit(&config);
    return success;
}

// Test 6: Simple array of strings
bool test_simple_array_of_strings(void) {
    printf("Testing simple array of strings\n");

    bool    success = true;
    Str     json    = StrInitFromZstr("{\"languages\": [\"C\", \"Python\", \"Rust\"]}");
    StrIter si      = StrIterFromStr(json);

    Vec(Str) languages = VecInitWithDeepCopy(NULL, StrDeinit);

    JR_OBJ(si, {
        JR_ARR_KV(si, "languages", {
            Str lang = StrInit();
            JR_STR(si, lang);
            VecPushBack(&languages, lang);
        });
    });

    if (languages.length != 3) {
        printf("[DEBUG] Languages length check failed: expected 3, got %zu\n", languages.length);
        success = false;
    }

    if (languages.length >= 3) {
        Str* lang1 = &VecAt(&languages, 0);
        Str* lang2 = &VecAt(&languages, 1);
        Str* lang3 = &VecAt(&languages, 2);

        if (StrCmpCstr(lang1, "C", 1) != 0) {
            printf("[DEBUG] Language 1 check failed: expected 'C', got '");
            for (size i = 0; i < lang1->length; i++) {
                printf("%c", lang1->data[i]);
            }
            printf("'\n");
            success = false;
        }

        if (StrCmpCstr(lang2, "Python", 6) != 0) {
            printf("[DEBUG] Language 2 check failed: expected 'Python', got '");
            for (size i = 0; i < lang2->length; i++) {
                printf("%c", lang2->data[i]);
            }
            printf("'\n");
            success = false;
        }

        if (StrCmpCstr(lang3, "Rust", 4) != 0) {
            printf("[DEBUG] Language 3 check failed: expected 'Rust', got '");
            for (size i = 0; i < lang3->length; i++) {
                printf("%c", lang3->data[i]);
            }
            printf("'\n");
            success = false;
        }
    }

    StrDeinit(&json);
    VecDeinit(&languages);
    return success;
}

// Test 7: Simple nested object (1 level)
bool test_simple_nested_object(void) {
    printf("Testing simple nested object\n");

    bool success = true;
    Str  json =
        StrInitFromZstr("{\"user\": {\"name\": \"Charlie\", \"email\": \"charlie@example.com\"}, \"active\": true}");
    StrIter si = StrIterFromStr(json);

    struct {
        struct {
            Str name;
            Str email;
        } user;
        bool active;
    } data = {
        {StrInit(), StrInit()},
        false
    };

    JR_OBJ(si, {
        JR_OBJ_KV(si, "user", {
            JR_STR_KV(si, "name", data.user.name);
            JR_STR_KV(si, "email", data.user.email);
        });
        JR_BOOL_KV(si, "active", data.active);
    });

    if (StrCmpCstr(&data.user.name, "Charlie", 7) != 0) {
        printf("[DEBUG] User name check failed: expected 'Charlie', got '");
        for (size i = 0; i < data.user.name.length; i++) {
            printf("%c", data.user.name.data[i]);
        }
        printf("'\n");
        success = false;
    }

    if (StrCmpCstr(&data.user.email, "charlie@example.com", 19) != 0) {
        printf("[DEBUG] User email check failed: expected 'charlie@example.com', got '");
        for (size i = 0; i < data.user.email.length; i++) {
            printf("%c", data.user.email.data[i]);
        }
        printf("'\n");
        success = false;
    }

    if (data.active != true) {
        printf("[DEBUG] Active check failed: expected true, got %s\n", data.active ? "true" : "false");
        success = false;
    }

    StrDeinit(&json);
    StrDeinit(&data.user.name);
    StrDeinit(&data.user.email);
    return success;
}

// Test 8: Simple product with tags array
bool test_simple_product_with_tags(void) {
    printf("Testing simple product with tags array\n");

    bool success = true;
    Str  json    = StrInitFromZstr(
        "{\"id\": 12345, \"name\": \"Laptop\", \"price\": 999.99, \"tags\": [\"electronics\", \"computers\", "
            "\"portable\"]}"
    );
    StrIter si = StrIterFromStr(json);

    SimpleProduct product = {0};
    product.name          = StrInit();
    product.tags          = VecInitWithDeepCopyT(product.tags, NULL, StrDeinit);

    JR_OBJ(si, {
        JR_INT_KV(si, "id", product.id);
        JR_STR_KV(si, "name", product.name);
        JR_FLT_KV(si, "price", product.price);
        JR_ARR_KV(si, "tags", {
            Str tag = StrInit();
            JR_STR(si, tag);
            VecPushBack(&product.tags, tag);
        });
    });

    if (product.id != 12345) {
        printf("[DEBUG] Product ID check failed: expected 12345, got %llu\n", product.id);
        success = false;
    }

    if (StrCmpCstr(&product.name, "Laptop", 6) != 0) {
        printf("[DEBUG] Product name check failed: expected 'Laptop', got '");
        for (size i = 0; i < product.name.length; i++) {
            printf("%c", product.name.data[i]);
        }
        printf("'\n");
        success = false;
    }

    if (!(product.price > 999.98 && product.price < 1000.0)) {
        printf("[DEBUG] Product price check failed: expected ~999.99, got %f\n", product.price);
        success = false;
    }

    if (product.tags.length != 3) {
        printf("[DEBUG] Product tags length check failed: expected 3, got %zu\n", product.tags.length);
        success = false;
    }

    if (product.tags.length >= 3) {
        Str* tag1 = &VecAt(&product.tags, 0);
        Str* tag2 = &VecAt(&product.tags, 1);
        Str* tag3 = &VecAt(&product.tags, 2);

        if (StrCmpCstr(tag1, "electronics", 11) != 0) {
            printf("[DEBUG] Tag 1 check failed: expected 'electronics', got '");
            for (size i = 0; i < tag1->length; i++) {
                printf("%c", tag1->data[i]);
            }
            printf("'\n");
            success = false;
        }

        if (StrCmpCstr(tag2, "computers", 9) != 0) {
            printf("[DEBUG] Tag 2 check failed: expected 'computers', got '");
            for (size i = 0; i < tag2->length; i++) {
                printf("%c", tag2->data[i]);
            }
            printf("'\n");
            success = false;
        }

        if (StrCmpCstr(tag3, "portable", 8) != 0) {
            printf("[DEBUG] Tag 3 check failed: expected 'portable', got '");
            for (size i = 0; i < tag3->length; i++) {
                printf("%c", tag3->data[i]);
            }
            printf("'\n");
            success = false;
        }
    }

    StrDeinit(&json);
    SimpleProductDeinit(&product);
    return success;
}

// Main function that runs all simple tests
int main(int argc, char* argv[]) {
    // Array of test functions
    TestFunction tests[] = {
        test_simple_string_parsing,
        test_simple_numbers,
        test_simple_boolean,
        test_simple_person_object,
        test_simple_config_object,
        test_simple_array_of_strings,
        test_simple_nested_object,
        test_simple_product_with_tags
    };

    int test_count = sizeof(tests) / sizeof(tests[0]);

    // Use centralized test driver
    return run_test_suite(tests, test_count, NULL, 0, "Json.Read.Simple");
}
