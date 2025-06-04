#include <Misra/Parsers/JSON.h>
#include <Misra/Std/Io.h>
#include <Misra/Std/Log.h>
#include <stdio.h>
#include <stdlib.h>
#include <Misra/Types.h>

// Include test utilities
#include "../Util/TestRunner.h"

// Complex data structures for nested JSON writing examples
typedef struct AnnSymbol {
    u64  source_function_id;
    u64  target_function_id;
    f64  distance;
    u64  analysis_id;
    u64  binary_id;
    Str  analysis_name;
    Str  function_name;
    Str  sha256;
    bool debug;
    Str  function_mangled_name;
} AnnSymbol;

typedef Vec(AnnSymbol) AnnSymbols;

typedef struct ApiResponse {
    bool       status;
    Str        message;
    AnnSymbols data;
} ApiResponse;

typedef struct FunctionInfo {
    u64 id;
    Str name;
    u64 size;
    u64 vaddr;
} FunctionInfo;

typedef struct SearchResult {
    u64 binary_id;
    Str binary_name;
    u64 analysis_id;
    Str sha256;
    Vec(Str) tags;
    Str created_at;
    u64 model_id;
    Str model_name;
    Str owned_by;
} SearchResult;

// Cleanup functions
void AnnSymbolDeinit(AnnSymbol* sym) {
    StrDeinit(&sym->analysis_name);
    StrDeinit(&sym->function_name);
    StrDeinit(&sym->sha256);
    StrDeinit(&sym->function_mangled_name);
}

void FunctionInfoDeinit(FunctionInfo* info) {
    StrDeinit(&info->name);
}

void SearchResultDeinit(SearchResult* result) {
    StrDeinit(&result->binary_name);
    StrDeinit(&result->sha256);
    VecDeinit(&result->tags);
    StrDeinit(&result->created_at);
    StrDeinit(&result->model_name);
    StrDeinit(&result->owned_by);
}

// Helper function to compare JSON output (removes whitespace for comparison)
bool compare_json_output(const Str* output, const char* expected) {
    Str expected_str   = StrInitFromZstr(expected);
    Str output_clean   = StrInit();
    Str expected_clean = StrInit();

    // Remove whitespace from both strings for comparison
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

// Function prototypes
bool test_two_level_nesting_writing(void);
bool test_three_level_nesting_writing(void);
bool test_complex_api_response_writing(void);
bool test_function_info_array_writing(void);
bool test_search_results_with_tags_writing(void);
bool test_dynamic_object_keys_writing(void);
bool test_deeply_nested_structure_writing(void);
bool test_mixed_array_types_writing(void);

// Test 1: Two-level nesting writing
bool test_two_level_nesting_writing(void) {
    printf("Testing two-level nesting writing\n");

    bool success = true;
    Str  json    = StrInit();

    struct {
        struct {
            u64 id;
            struct {
                Str name;
                u64 age;
            } profile;
        } user;
        Str status;
    } data = {
        {123, {StrInitFromZstr("Alice"), 30}},
        StrInitFromZstr("active")
    };

    JW_OBJ(json, {
        JW_OBJ_KV(json, "user", {
            JW_INT_KV(json, "id", data.user.id);
            JW_OBJ_KV(json, "profile", {
                JW_STR_KV(json, "name", data.user.profile.name);
                JW_INT_KV(json, "age", data.user.profile.age);
            });
        });
        JW_STR_KV(json, "status", data.status);
    });

    const char* expected = "{\"user\":{\"id\":123,\"profile\":{\"name\":\"Alice\",\"age\":30}},\"status\":\"active\"}";
    if (!compare_json_output(&json, expected)) {
        success = false;
    }

    StrDeinit(&json);
    StrDeinit(&data.user.profile.name);
    StrDeinit(&data.status);
    return success;
}

// Test 2: Three-level nesting writing
bool test_three_level_nesting_writing(void) {
    printf("Testing three-level nesting writing\n");

    bool success = true;
    Str  json    = StrInit();

    struct {
        struct {
            struct {
                struct {
                    Str head;
                    u64 count;
                    f64 budget;
                } engineering;
            } departments;
            Str name;
        } company;
    } data = {
        {{{StrInitFromZstr("John"), 25, 150000.0}}, StrInitFromZstr("TechCorp")}
    };

    JW_OBJ(json, {
        JW_OBJ_KV(json, "company", {
            JW_OBJ_KV(json, "departments", {
                JW_OBJ_KV(json, "engineering", {
                    JW_STR_KV(json, "head", data.company.departments.engineering.head);
                    JW_INT_KV(json, "count", data.company.departments.engineering.count);
                    JW_FLT_KV(json, "budget", data.company.departments.engineering.budget);
                });
            });
            JW_STR_KV(json, "name", data.company.name);
        });
    });

    const char* expected =
        "{\"company\":{\"departments\":{\"engineering\":{\"head\":\"John\",\"count\":25,\"budget\":150000.000000}},"
        "\"name\":\"TechCorp\"}}";
    if (!compare_json_output(&json, expected)) {
        success = false;
    }

    StrDeinit(&json);
    StrDeinit(&data.company.departments.engineering.head);
    StrDeinit(&data.company.name);
    return success;
}

// Test 3: Complex API response writing
bool test_complex_api_response_writing(void) {
    printf("Testing complex API response writing\n");

    bool success = true;
    Str  json    = StrInit();

    ApiResponse response = {true, StrInitFromZstr("Success"), VecInitWithDeepCopy(NULL, AnnSymbolDeinit)};

    // Add sample data
    AnnSymbol sym             = {0};
    sym.analysis_name         = StrInitFromZstr("test_analysis");
    sym.function_name         = StrInitFromZstr("main_func");
    sym.sha256                = StrInitFromZstr("abc123");
    sym.function_mangled_name = StrInitFromZstr("_Z4main");
    sym.source_function_id    = 12345;
    sym.target_function_id    = 67890;
    sym.distance              = 0.85;
    sym.analysis_id           = 999;
    sym.binary_id             = 888;
    sym.debug                 = true;
    VecPushBack(&response.data, sym);

    JW_OBJ(json, {
        JW_BOOL_KV(json, "status", response.status);
        JW_STR_KV(json, "message", response.message);
        JW_OBJ_KV(json, "data", {
            // Write dynamic key for source function ID
            Str source_key = StrInit();
            u64 source_id  = response.data.length > 0 ? VecAt(&response.data, 0).source_function_id : 0;
            StrWriteFmt(&source_key, "{}", FMT(source_id));

            JW_OBJ_KV(json, source_key.data, {
                if (response.data.length > 0) {
                    AnnSymbol* s          = &VecAt(&response.data, 0);
                    Str        target_key = StrInit();
                    StrWriteFmt(&target_key, "{}", FMT(s->target_function_id));

                    JW_OBJ_KV(json, target_key.data, {
                        JW_FLT_KV(json, "distance", s->distance);
                        JW_INT_KV(json, "nearest_neighbor_analysis_id", s->analysis_id);
                        JW_INT_KV(json, "nearest_neighbor_binary_id", s->binary_id);
                        JW_STR_KV(json, "nearest_neighbor_analysis_name", s->analysis_name);
                        JW_STR_KV(json, "nearest_neighbor_function_name", s->function_name);
                        JW_STR_KV(json, "nearest_neighbor_sha_256_hash", s->sha256);
                        JW_BOOL_KV(json, "nearest_neighbor_debug", s->debug);
                        JW_STR_KV(json, "nearest_neighbor_function_name_mangled", s->function_mangled_name);
                    });

                    StrDeinit(&target_key);
                }
            });

            StrDeinit(&source_key);
        });
    });

    const char* expected =
        "{\"status\":true,\"message\":\"Success\",\"data\":{\"12345\":{\"67890\":{\"distance\":0.850000,\"nearest_"
        "neighbor_analysis_id\":999,\"nearest_neighbor_binary_id\":888,\"nearest_neighbor_analysis_name\":\"test_"
        "analysis\",\"nearest_neighbor_function_name\":\"main_func\",\"nearest_neighbor_sha_256_hash\":\"abc123\","
        "\"nearest_neighbor_debug\":true,\"nearest_neighbor_function_name_mangled\":\"_Z4main\"}}}}";
    if (!compare_json_output(&json, expected)) {
        success = false;
    }

    StrDeinit(&json);
    StrDeinit(&response.message);
    VecDeinit(&response.data);
    return success;
}

// Test 4: Function info array writing
bool test_function_info_array_writing(void) {
    printf("Testing function info array writing\n");

    bool success = true;
    Str  json    = StrInit();

    Vec(FunctionInfo) functions = VecInitWithDeepCopy(NULL, FunctionInfoDeinit);

    FunctionInfo func1 = {12345, StrInitFromZstr("test_func"), 1024, 4096};
    FunctionInfo func2 = {54321, StrInitFromZstr("helper_func"), 512, 8192};
    VecPushBack(&functions, func1);
    VecPushBack(&functions, func2);

    JW_OBJ(json, {
        JW_ARR_KV(json, "functions", functions, func, {
            JW_OBJ(json, {
                JW_INT_KV(json, "id", func.id);
                JW_STR_KV(json, "name", func.name);
                JW_INT_KV(json, "size", func.size);
                JW_INT_KV(json, "vaddr", func.vaddr);
            });
        });
    });

    const char* expected =
        "{\"functions\":[{\"id\":12345,\"name\":\"test_func\",\"size\":1024,\"vaddr\":4096},{\"id\":54321,\"name\":"
        "\"helper_func\",\"size\":512,\"vaddr\":8192}]}";
    if (!compare_json_output(&json, expected)) {
        success = false;
    }

    StrDeinit(&json);
    VecDeinit(&functions);
    return success;
}

// Test 5: Search results with tags writing
bool test_search_results_with_tags_writing(void) {
    printf("Testing search results with tags writing\n");

    bool success = true;
    Str  json    = StrInit();

    SearchResult result = {0};
    result.binary_id    = 888;
    result.binary_name  = StrInitFromZstr("test_binary");
    result.analysis_id  = 999;
    result.sha256       = StrInitFromZstr("abc123");
    result.tags         = VecInitWithDeepCopyT(result.tags, NULL, StrDeinit);

    // Create strings and push them properly
    Str tag1 = StrInitFromZstr("malware");
    Str tag2 = StrInitFromZstr("x86");

    VecPushBack(&result.tags, tag1);
    VecPushBack(&result.tags, tag2);

    result.created_at = StrInitFromZstr("2024-04-01");
    result.model_id   = 12345;
    result.model_name = StrInitFromZstr("test_model");
    result.owned_by   = StrInitFromZstr("user1");

    JW_OBJ(json, {
        JW_INT_KV(json, "binary_id", result.binary_id);
        JW_STR_KV(json, "binary_name", result.binary_name);
        JW_INT_KV(json, "analysis_id", result.analysis_id);
        JW_STR_KV(json, "sha256", result.sha256);
        JW_ARR_KV(json, "tags", result.tags, tag, { JW_STR(json, tag); });
        JW_STR_KV(json, "created_at", result.created_at);
        JW_INT_KV(json, "model_id", result.model_id);
        JW_STR_KV(json, "model_name", result.model_name);
        JW_STR_KV(json, "owned_by", result.owned_by);
    });

    const char* expected =
        "{\"binary_id\":888,\"binary_name\":\"test_binary\",\"analysis_id\":999,\"sha256\":\"abc123\",\"tags\":["
        "\"malware\",\"x86\"],\"created_at\":\"2024-04-01\",\"model_id\":12345,\"model_name\":\"test_model\",\"owned_"
        "by\":\"user1\"}";
    if (!compare_json_output(&json, expected)) {
        success = false;
    }

    StrDeinit(&json);
    SearchResultDeinit(&result);
    return success;
}

// Test 6: Dynamic object keys writing
bool test_dynamic_object_keys_writing(void) {
    printf("Testing dynamic object keys writing\n");

    bool success = true;
    Str  json    = StrInit();

    Vec(AnnSymbol) symbols = VecInitWithDeepCopy(NULL, AnnSymbolDeinit);

    AnnSymbol sym1             = {0};
    sym1.analysis_name         = StrInitFromZstr("analysis1");
    sym1.function_name         = StrInitFromZstr("func1");
    sym1.sha256                = StrInitFromZstr("hash1");
    sym1.function_mangled_name = StrInitFromZstr("_Z5func1");
    sym1.source_function_id    = 111;
    sym1.target_function_id    = 222;
    sym1.distance              = 0.9;
    VecPushBack(&symbols, sym1);

    AnnSymbol sym2             = {0};
    sym2.analysis_name         = StrInitFromZstr("analysis2");
    sym2.function_name         = StrInitFromZstr("func2");
    sym2.sha256                = StrInitFromZstr("hash2");
    sym2.function_mangled_name = StrInitFromZstr("_Z5func2");
    sym2.source_function_id    = 333;
    sym2.target_function_id    = 444;
    sym2.distance              = 0.8;
    VecPushBack(&symbols, sym2);

    JW_OBJ(json, {
        JW_OBJ_KV(json, "functions", {
            VecForeach(&symbols, symbol, {
                Str source_key = StrInit();
                StrWriteFmt(&source_key, "{}", FMT(symbol.source_function_id));

                JW_OBJ_KV(json, source_key.data, {
                    Str target_key = StrInit();
                    StrWriteFmt(&target_key, "{}", FMT(symbol.target_function_id));

                    JW_OBJ_KV(json, target_key.data, {
                        JW_FLT_KV(json, "distance", symbol.distance);
                        JW_STR_KV(json, "name", symbol.function_name);
                    });

                    StrDeinit(&target_key);
                });

                StrDeinit(&source_key);
            });
        });
    });

    const char* expected =
        "{\"functions\":{\"111\":{\"222\":{\"distance\":0.900000,\"name\":\"func1\"}},\"333\":{\"444\":{\"distance\":0."
        "800000,\"name\":\"func2\"}}}}";
    if (!compare_json_output(&json, expected)) {
        success = false;
    }

    StrDeinit(&json);
    VecDeinit(&symbols);
    return success;
}

// Test 7: Deeply nested structure writing
bool test_deeply_nested_structure_writing(void) {
    printf("Testing deeply nested structure writing\n");

    bool success = true;
    Str  json    = StrInit();

    // Create strings for the nested structure
    Str deep_message = StrInitFromZstr("deep");
    Str test_name    = StrInitFromZstr("test");

    JW_OBJ(json, {
        JW_OBJ_KV(json, "level1", {
            JW_OBJ_KV(json, "level2", {
                JW_OBJ_KV(json, "level3", {
                    JW_STR_KV(json, "message", deep_message);
                    JW_INT_KV(json, "value", 42);
                });
                JW_BOOL_KV(json, "flag", true);
            });
            JW_STR_KV(json, "name", test_name);
        });
    });

    const char* expected =
        "{\"level1\":{\"level2\":{\"level3\":{\"message\":\"deep\",\"value\":42},\"flag\":true},\"name\":\"test\"}}";
    if (!compare_json_output(&json, expected)) {
        success = false;
    }

    StrDeinit(&json);
    StrDeinit(&deep_message);
    StrDeinit(&test_name);
    return success;
}

// Test 8: Mixed array types writing
bool test_mixed_array_types_writing(void) {
    printf("Testing mixed array types writing\n");

    bool success = true;
    Str  json    = StrInit();

    Vec(u32) numbers = VecInit();
    u32 num1 = 1, num2 = 2, num3 = 3;
    VecPushBack(&numbers, num1);
    VecPushBack(&numbers, num2);
    VecPushBack(&numbers, num3);

    Vec(Str) strings = VecInitWithDeepCopy(NULL, StrDeinit);

    // Create strings and push them properly
    Str str1 = StrInitFromZstr("a");
    Str str2 = StrInitFromZstr("b");
    Str str3 = StrInitFromZstr("c");

    VecPushBack(&strings, str1);
    VecPushBack(&strings, str2);
    VecPushBack(&strings, str3);

    Vec(bool) booleans = VecInit();
    bool bool1 = true, bool2 = false, bool3 = true;
    VecPushBack(&booleans, bool1);
    VecPushBack(&booleans, bool2);
    VecPushBack(&booleans, bool3);

    JW_OBJ(json, {
        JW_ARR_KV(json, "numbers", numbers, num, { JW_INT(json, num); });
        JW_ARR_KV(json, "strings", strings, str, { JW_STR(json, str); });
        JW_ARR_KV(json, "booleans", booleans, b, { JW_BOOL(json, b); });
    });

    const char* expected = "{\"numbers\":[1,2,3],\"strings\":[\"a\",\"b\",\"c\"],\"booleans\":[true,false,true]}";
    if (!compare_json_output(&json, expected)) {
        success = false;
    }

    StrDeinit(&json);
    VecDeinit(&numbers);
    VecDeinit(&strings);
    VecDeinit(&booleans);
    return success;
}

// Main function that runs all nested writing tests
int main(int argc, char* argv[]) {
    // Array of test functions
    TestFunction tests[] = {
        test_two_level_nesting_writing,
        test_three_level_nesting_writing,
        test_complex_api_response_writing,
        test_function_info_array_writing,
        test_search_results_with_tags_writing,
        test_dynamic_object_keys_writing,
        test_deeply_nested_structure_writing,
        test_mixed_array_types_writing
    };

    int test_count = sizeof(tests) / sizeof(tests[0]);

    // Use centralized test driver
    return run_test_suite(tests, test_count, NULL, 0, "Json.Write.Nested");
}