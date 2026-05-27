#include <Misra/Parsers/JSON.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Io.h>
#include <Misra/Std/Log.h>
#include <Misra/Types.h>

// Include test utilities
#include "../Util/JsonReaderAllocAware.h"
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
    WriteFmt("Testing simple string parsing\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    bool    success = true;
    Str     json    = StrInitFromZstr("{\"name\": \"Alice\", \"city\": \"New York\"}", &alloc);
    StrIter si      = StrIterFromStr(json);

    Str name = StrInit(&alloc);
    Str city = StrInit(&alloc);

    JR_OBJ(si, {
        JR_STR_KV(si, "name", name);
        JR_STR_KV(si, "city", city);
    });

    if (StrCmp(&name, "Alice", 5) != 0) {
        WriteFmt("[DEBUG] Name check failed: expected 'Alice', got '");
        for (size i = 0; i < StrLen(&name); i++) {
            WriteFmt("{c}", StrBegin(&name)[i]);
        }
        WriteFmt("'\n");
        success = false;
    }

    if (StrCmp(&city, "New York", 8) != 0) {
        WriteFmt("[DEBUG] City check failed: expected 'New York', got '");
        for (size i = 0; i < StrLen(&city); i++) {
            WriteFmt("{c}", StrBegin(&city)[i]);
        }
        WriteFmt("'\n");
        success = false;
    }

    StrDeinit(&json);
    StrDeinit(&name);
    StrDeinit(&city);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Test 2: Simple number parsing
bool test_simple_numbers(void) {
    WriteFmt("Testing simple number parsing\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    bool    success = true;
    Str     json    = StrInitFromZstr("{\"count\": 42, \"score\": 95.5, \"year\": 2024}", &alloc);
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
        WriteFmt("[DEBUG] Count check failed: expected 42, got {}\n", count);
        success = false;
    }

    if (!(score > 95.4 && score < 95.6)) {
        WriteFmt("[DEBUG] Score check failed: expected ~95.5, got {}\n", score);
        success = false;
    }

    if (year != 2024) {
        WriteFmt("[DEBUG] Year check failed: expected 2024, got {}\n", year);
        success = false;
    }

    StrDeinit(&json);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Test 3: Simple boolean parsing
bool test_simple_boolean(void) {
    WriteFmt("Testing simple boolean parsing\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    bool    success = true;
    Str     json    = StrInitFromZstr("{\"enabled\": true, \"visible\": false}", &alloc);
    StrIter si      = StrIterFromStr(json);

    bool enabled = false;
    bool visible = true;

    JR_OBJ(si, {
        JR_BOOL_KV(si, "enabled", enabled);
        JR_BOOL_KV(si, "visible", visible);
    });

    if (enabled != true) {
        WriteFmt("[DEBUG] Enabled check failed: expected true, got {}\n", enabled ? "true" : "false");
        success = false;
    }

    if (visible != false) {
        WriteFmt("[DEBUG] Visible check failed: expected false, got {}\n", visible ? "true" : "false");
        success = false;
    }

    StrDeinit(&json);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Test 4: Simple person object
bool test_simple_person_object(void) {
    WriteFmt("Testing simple person object\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    bool success = true;
    Str  json    = StrInitFromZstr(
        "{\"id\": 1001, \"name\": \"Bob\", \"age\": 25, \"is_active\": true, \"salary\": 50000.0}",
        &alloc
    );
    StrIter si = StrIterFromStr(json);

    Person person = {0};
    person.name   = StrInit(&alloc);

    JR_OBJ(si, {
        JR_INT_KV(si, "id", person.id);
        JR_STR_KV(si, "name", person.name);
        JR_INT_KV(si, "age", person.age);
        JR_BOOL_KV(si, "is_active", person.is_active);
        JR_FLT_KV(si, "salary", person.salary);
    });

    if (person.id != 1001) {
        WriteFmt("[DEBUG] Person ID check failed: expected 1001, got {}\n", person.id);
        success = false;
    }

    if (StrCmp(&person.name, "Bob", 3) != 0) {
        WriteFmt("[DEBUG] Person name check failed: expected 'Bob', got '");
        for (size i = 0; i < StrLen(&person.name); i++) {
            WriteFmt("{c}", StrBegin(&person.name)[i]);
        }
        WriteFmt("'\n");
        success = false;
    }

    if (person.age != 25) {
        WriteFmt("[DEBUG] Person age check failed: expected 25, got {}\n", person.age);
        success = false;
    }

    if (person.is_active != true) {
        WriteFmt("[DEBUG] Person is_active check failed: expected true, got {}\n", person.is_active ? "true" : "false");
        success = false;
    }

    if (!(person.salary > 49999.0 && person.salary < 50001.0)) {
        WriteFmt("[DEBUG] Person salary check failed: expected ~50000.0, got {}\n", person.salary);
        success = false;
    }

    StrDeinit(&json);
    PersonDeinit(&person);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Test 5: Simple config object
bool test_simple_config_object(void) {
    WriteFmt("Testing simple config object\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    bool    success = true;
    Str     json    = StrInitFromZstr("{\"debug_mode\": false, \"timeout\": 30, \"log_level\": \"INFO\"}", &alloc);
    StrIter si      = StrIterFromStr(json);

    Config config    = {0};
    config.log_level = StrInit(&alloc);

    JR_OBJ(si, {
        JR_BOOL_KV(si, "debug_mode", config.debug_mode);
        JR_INT_KV(si, "timeout", config.timeout);
        JR_STR_KV(si, "log_level", config.log_level);
    });

    if (config.debug_mode != false) {
        WriteFmt("[DEBUG] Debug mode check failed: expected false, got {}\n", config.debug_mode ? "true" : "false");
        success = false;
    }

    if (config.timeout != 30) {
        WriteFmt("[DEBUG] Timeout check failed: expected 30, got {}\n", config.timeout);
        success = false;
    }

    if (StrCmp(&config.log_level, "INFO", 4) != 0) {
        WriteFmt("[DEBUG] Log level check failed: expected 'INFO', got '");
        for (size i = 0; i < StrLen(&config.log_level); i++) {
            WriteFmt("{c}", StrBegin(&config.log_level)[i]);
        }
        WriteFmt("'\n");
        success = false;
    }

    StrDeinit(&json);
    ConfigDeinit(&config);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Test 6: Simple array of strings
bool test_simple_array_of_strings(void) {
    WriteFmt("Testing simple array of strings\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    bool    success = true;
    Str     json    = StrInitFromZstr("{\"languages\": [\"C\", \"Python\", \"Rust\"]}", &alloc);
    StrIter si      = StrIterFromStr(json);

    Vec(Str) languages = VecInitWithDeepCopy(NULL, StrDeinit, &alloc);

    JR_OBJ(si, {
        JR_ARR_KV(si, "languages", {
            Str lang = StrInit(&alloc);
            JR_STR(si, lang);
            VecPushBack(&languages, lang);
        });
    });

    if (VecLen(&languages) != 3) {
        WriteFmt("[DEBUG] Languages length check failed: expected 3, got {}\n", VecLen(&languages));
        success = false;
    }

    if (VecLen(&languages) >= 3) {
        Str *lang1 = &VecAt(&languages, 0);
        Str *lang2 = &VecAt(&languages, 1);
        Str *lang3 = &VecAt(&languages, 2);

        if (StrCmp(lang1, "C", 1) != 0) {
            WriteFmt("[DEBUG] Language 1 check failed: expected 'C', got '");
            for (size i = 0; i < StrLen(lang1); i++) {
                WriteFmt("{c}", StrBegin(lang1)[i]);
            }
            WriteFmt("'\n");
            success = false;
        }

        if (StrCmp(lang2, "Python", 6) != 0) {
            WriteFmt("[DEBUG] Language 2 check failed: expected 'Python', got '");
            for (size i = 0; i < StrLen(lang2); i++) {
                WriteFmt("{c}", StrBegin(lang2)[i]);
            }
            WriteFmt("'\n");
            success = false;
        }

        if (StrCmp(lang3, "Rust", 4) != 0) {
            WriteFmt("[DEBUG] Language 3 check failed: expected 'Rust', got '");
            for (size i = 0; i < StrLen(lang3); i++) {
                WriteFmt("{c}", StrBegin(lang3)[i]);
            }
            WriteFmt("'\n");
            success = false;
        }
    }

    StrDeinit(&json);
    VecDeinit(&languages);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Test 7: Simple nested object (1 level)
bool test_simple_nested_object(void) {
    WriteFmt("Testing simple nested object\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    bool success = true;
    Str  json    = StrInitFromZstr(
        "{\"user\": {\"name\": \"Charlie\", \"email\": \"charlie@example.com\"}, \"active\": true}",
        &alloc
    );
    StrIter si = StrIterFromStr(json);

    struct {
        struct {
            Str name;
            Str email;
        } user;
        bool active;
    } data = {
        {StrInit(&alloc), StrInit(&alloc)},
        false
    };

    JR_OBJ(si, {
        JR_OBJ_KV(si, "user", {
            JR_STR_KV(si, "name", data.user.name);
            JR_STR_KV(si, "email", data.user.email);
        });
        JR_BOOL_KV(si, "active", data.active);
    });

    if (StrCmp(&data.user.name, "Charlie", 7) != 0) {
        WriteFmt("[DEBUG] User name check failed: expected 'Charlie', got '");
        for (size i = 0; i < StrLen(&data.user.name); i++) {
            WriteFmt("{c}", StrBegin(&data.user.name)[i]);
        }
        WriteFmt("'\n");
        success = false;
    }

    if (StrCmp(&data.user.email, "charlie@example.com", 19) != 0) {
        WriteFmt("[DEBUG] User email check failed: expected 'charlie@example.com', got '");
        for (size i = 0; i < StrLen(&data.user.email); i++) {
            WriteFmt("{c}", StrBegin(&data.user.email)[i]);
        }
        WriteFmt("'\n");
        success = false;
    }

    if (data.active != true) {
        WriteFmt("[DEBUG] Active check failed: expected true, got {}\n", data.active ? "true" : "false");
        success = false;
    }

    StrDeinit(&json);
    StrDeinit(&data.user.name);
    StrDeinit(&data.user.email);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Test 8: Simple product with tags array
bool test_simple_product_with_tags(void) {
    WriteFmt("Testing simple product with tags array\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    bool success = true;
    Str  json    = StrInitFromZstr(
        "{\"id\": 12345, \"name\": \"Laptop\", \"price\": 999.99, \"tags\": [\"electronics\", \"computers\", "
            "\"portable\"]}",
        &alloc
    );
    StrIter si = StrIterFromStr(json);

    SimpleProduct product = {0};
    product.name          = StrInit(&alloc);
    product.tags          = VecInitWithDeepCopyT(product.tags, NULL, StrDeinit, &alloc);

    JR_OBJ(si, {
        JR_INT_KV(si, "id", product.id);
        JR_STR_KV(si, "name", product.name);
        JR_FLT_KV(si, "price", product.price);
        JR_ARR_KV(si, "tags", {
            Str tag = StrInit(&alloc);
            JR_STR(si, tag);
            VecPushBack(&product.tags, tag);
        });
    });

    if (product.id != 12345) {
        WriteFmt("[DEBUG] Product ID check failed: expected 12345, got {}\n", product.id);
        success = false;
    }

    if (StrCmp(&product.name, "Laptop", 6) != 0) {
        WriteFmt("[DEBUG] Product name check failed: expected 'Laptop', got '");
        for (size i = 0; i < StrLen(&product.name); i++) {
            WriteFmt("{c}", StrBegin(&product.name)[i]);
        }
        WriteFmt("'\n");
        success = false;
    }

    if (!(product.price > 999.98 && product.price < 1000.0)) {
        WriteFmt("[DEBUG] Product price check failed: expected ~999.99, got {}\n", product.price);
        success = false;
    }

    if (VecLen(&product.tags) != 3) {
        WriteFmt("[DEBUG] Product tags length check failed: expected 3, got {}\n", VecLen(&product.tags));
        success = false;
    }

    if (VecLen(&product.tags) >= 3) {
        Str *tag1 = &VecAt(&product.tags, 0);
        Str *tag2 = &VecAt(&product.tags, 1);
        Str *tag3 = &VecAt(&product.tags, 2);

        if (StrCmp(tag1, "electronics", 11) != 0) {
            WriteFmt("[DEBUG] Tag 1 check failed: expected 'electronics', got '");
            for (size i = 0; i < StrLen(tag1); i++) {
                WriteFmt("{c}", StrBegin(tag1)[i]);
            }
            WriteFmt("'\n");
            success = false;
        }

        if (StrCmp(tag2, "computers", 9) != 0) {
            WriteFmt("[DEBUG] Tag 2 check failed: expected 'computers', got '");
            for (size i = 0; i < StrLen(tag2); i++) {
                WriteFmt("{c}", StrBegin(tag2)[i]);
            }
            WriteFmt("'\n");
            success = false;
        }

        if (StrCmp(tag3, "portable", 8) != 0) {
            WriteFmt("[DEBUG] Tag 3 check failed: expected 'portable', got '");
            for (size i = 0; i < StrLen(tag3); i++) {
                WriteFmt("{c}", StrBegin(tag3)[i]);
            }
            WriteFmt("'\n");
            success = false;
        }
    }

    StrDeinit(&json);
    SimpleProductDeinit(&product);
    DefaultAllocatorDeinit(&alloc);
    return success;
}

// Main function that runs all simple tests
int main(int argc, char *argv[]) {
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
