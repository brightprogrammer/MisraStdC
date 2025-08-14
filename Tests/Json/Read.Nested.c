#include <Misra/Parsers/JSON.h>
#include <Misra/Std/Io.h>
#include <Misra/Std/Log.h>
#include <stdio.h>
#include <stdlib.h>
#include <Misra/Types.h>

// Include test utilities
#include "../Util/TestRunner.h"

// Test data structures for complex nested parsing
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

void AnnSymbolDeinit(AnnSymbol* sym) {
    StrDeinit(&sym->analysis_name);
    StrDeinit(&sym->function_name);
    StrDeinit(&sym->sha256);
    StrDeinit(&sym->function_mangled_name);
}

// Test data structures based on user's real examples
typedef struct FunctionInfo {
    u64 id;
    Str name;
    u64 size;
    u64 vaddr;
} FunctionInfo;

typedef Vec(FunctionInfo) FunctionInfos;

typedef struct ModelInfo {
    u64 id;
    Str name;
} ModelInfo;

typedef Vec(ModelInfo) ModelInfos;

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

typedef Vec(SearchResult) SearchResults;

void FunctionInfoDeinit(FunctionInfo* info) {
    StrDeinit(&info->name);
}

void ModelInfoDeinit(ModelInfo* info) {
    StrDeinit(&info->name);
}

void SearchResultDeinit(SearchResult* result) {
    StrDeinit(&result->binary_name);
    StrDeinit(&result->sha256);
    VecForeach(&result->tags, tag, { StrDeinit(&tag); });
    VecDeinit(&result->tags);
    StrDeinit(&result->created_at);
    StrDeinit(&result->model_name);
    StrDeinit(&result->owned_by);
}

// Function prototypes
bool test_basic_iterator_functionality(void);
bool test_simple_json_object(void);
bool test_two_level_nesting(void);
bool test_three_level_nesting(void);
bool test_dynamic_key_parsing(void);
bool test_complex_api_response(void);
bool test_function_info_parsing(void);
bool test_model_info_parsing(void);
bool test_search_results_with_tags(void);
bool test_conditional_parsing(void);
bool test_status_response_pattern(void);

// Test basic iterator functionality
bool test_basic_iterator_functionality(void) {
    printf("Testing basic iterator functionality\n");

    Str     json = StrInitFromZstr("{\"test\": \"value\"}");
    StrIter si   = StrIterFromStr(json);

    bool success = true;

    if (!StrIterRemainingLength(&si)) {
        printf("[DEBUG] Remaining length check failed: expected > 0, got %zu\n", StrIterRemainingLength(&si));
        success = false;
    }

    if (StrIterPeek(&si) != '{') {
        printf("[DEBUG] Peek check failed: expected '{', got '%c'\n", StrIterPeek(&si));
        success = false;
    }

    StrDeinit(&json);
    return success;
}

// Test simple JSON object (1 level)
bool test_simple_json_object(void) {
    printf("Testing simple JSON object (1 level)\n");

    bool    success = true;
    Str     json    = StrInitFromZstr("{\"id\": 12345, \"name\": \"test\", \"active\": true, \"score\": 98.5}");
    StrIter si      = StrIterFromStr(json);

    struct {
        u64  id;
        Str  name;
        bool active;
        f64  score;
    } data = {0, StrInit(), false, 0.0};

    JR_OBJ(si, {
        JR_INT_KV(si, "id", data.id);
        JR_STR_KV(si, "name", data.name);
        JR_BOOL_KV(si, "active", data.active);
        JR_FLT_KV(si, "score", data.score);
    });

    if (data.id != 12345) {
        printf("[DEBUG] ID check failed: expected 12345, got %llu\n", data.id);
        success = false;
    }

    if (StrCmpCstr(&data.name, "test", 4) != 0) {
        printf("[DEBUG] Name check failed: expected 'test', got '");
        for (size i = 0; i < data.name.length; i++) {
            printf("%c", data.name.data[i]);
        }
        printf("'\n");
        success = false;
    }

    if (data.active != true) {
        printf("[DEBUG] Active check failed: expected true, got %s\n", data.active ? "true" : "false");
        success = false;
    }

    if (!(data.score > 98.4 && data.score < 98.6)) {
        printf("[DEBUG] Score check failed: expected 98.4-98.6, got %f\n", data.score);
        success = false;
    }

    StrDeinit(&json);
    StrDeinit(&data.name);
    return success;
}

// Test two-level nesting
bool test_two_level_nesting(void) {
    printf("Testing two-level nesting\n");

    bool success = true;
    Str  json    = StrInitFromZstr(
        "{\"user\": {\"id\": 123, \"profile\": {\"name\": \"Alice\", \"age\": 30}}, \"status\": \"active\"}"
    );
    StrIter si = StrIterFromStr(json);

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
        {0, {StrInit(), 0}},
        StrInit()
    };

    JR_OBJ(si, {
        JR_OBJ_KV(si, "user", {
            JR_INT_KV(si, "id", data.user.id);
            JR_OBJ_KV(si, "profile", {
                JR_STR_KV(si, "name", data.user.profile.name);
                JR_INT_KV(si, "age", data.user.profile.age);
            });
        });
        JR_STR_KV(si, "status", data.status);
    });

    if (data.user.id != 123) {
        printf("[DEBUG] User ID check failed: expected 123, got %llu\n", data.user.id);
        success = false;
    }

    if (StrCmpCstr(&data.user.profile.name, "Alice", 5) != 0) {
        printf("[DEBUG] Profile name check failed: expected 'Alice', got '");
        for (size i = 0; i < data.user.profile.name.length; i++) {
            printf("%c", data.user.profile.name.data[i]);
        }
        printf("'\n");
        success = false;
    }

    if (data.user.profile.age != 30) {
        printf("[DEBUG] Profile age check failed: expected 30, got %llu\n", data.user.profile.age);
        success = false;
    }

    if (StrCmpCstr(&data.status, "active", 6) != 0) {
        printf("[DEBUG] Status check failed: expected 'active', got '");
        for (size i = 0; i < data.status.length; i++) {
            printf("%c", data.status.data[i]);
        }
        printf("'\n");
        success = false;
    }

    StrDeinit(&json);
    StrDeinit(&data.user.profile.name);
    StrDeinit(&data.status);
    return success;
}

// Test three-level nesting
bool test_three_level_nesting(void) {
    printf("Testing three-level nesting\n");

    bool success = true;
    Str  json    = StrInitFromZstr(
        "{\"company\": {\"departments\": {\"engineering\": {\"head\": \"John\", \"count\": 25, \"budget\": 150000.0}}, "
            "\"name\": \"TechCorp\"}}"
    );
    StrIter si = StrIterFromStr(json);

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
        {{{StrInit(), 0, 0.0}}, StrInit()}
    };

    JR_OBJ(si, {
        JR_OBJ_KV(si, "company", {
            JR_OBJ_KV(si, "departments", {
                JR_OBJ_KV(si, "engineering", {
                    JR_STR_KV(si, "head", data.company.departments.engineering.head);
                    JR_INT_KV(si, "count", data.company.departments.engineering.count);
                    JR_FLT_KV(si, "budget", data.company.departments.engineering.budget);
                });
            });
            JR_STR_KV(si, "name", data.company.name);
        });
    });

    if (StrCmpCstr(&data.company.departments.engineering.head, "John", 4) != 0) {
        printf("[DEBUG] Engineering head check failed: expected 'John', got '");
        for (size i = 0; i < data.company.departments.engineering.head.length; i++) {
            printf("%c", data.company.departments.engineering.head.data[i]);
        }
        printf("'\n");
        success = false;
    }

    if (data.company.departments.engineering.count != 25) {
        printf(
            "[DEBUG] Engineering count check failed: expected 25, got %llu\n",
            data.company.departments.engineering.count
        );
        success = false;
    }

    if (data.company.departments.engineering.budget < 149999.0) {
        printf(
            "[DEBUG] Engineering budget check failed: expected > 149999.0, got %f\n",
            data.company.departments.engineering.budget
        );
        success = false;
    }

    if (StrCmpCstr(&data.company.name, "TechCorp", 8) != 0) {
        printf("[DEBUG] Company name check failed: expected 'TechCorp', got '");
        for (size i = 0; i < data.company.name.length; i++) {
            printf("%c", data.company.name.data[i]);
        }
        printf("'\n");
        success = false;
    }

    StrDeinit(&json);
    StrDeinit(&data.company.departments.engineering.head);
    StrDeinit(&data.company.name);
    return success;
}

// Test dynamic key parsing (like your example) - Fixed initialization
bool test_dynamic_key_parsing(void) {
    printf("Testing dynamic key parsing\n");

    bool success = true;
    Str  json    = StrInitFromZstr(
        "{\"functions\": {\"12345\": {\"67890\": {\"distance\": 0.85, \"name\": \"main\"}}, \"54321\": {\"98765\": "
            "{\"distance\": 0.92, \"name\": \"helper\"}}}}"
    );
    StrIter si = StrIterFromStr(json);

    typedef Vec(AnnSymbol) Symbols;
    Symbols symbols = VecInitWithDeepCopy(NULL, AnnSymbolDeinit);

    JR_OBJ(si, {
        JR_OBJ_KV(si, "functions", {
            // First level: source function ID from key
            u64 source_function_id = strtoull(key.data, NULL, 10);
            JR_OBJ(si, {
                // Second level: target function ID from key
                u64 target_function_id = strtoull(key.data, NULL, 10);

                // Properly initialize all Str fields
                AnnSymbol sym             = {0};
                sym.analysis_name         = StrInit();
                sym.function_name         = StrInit();
                sym.sha256                = StrInit();
                sym.function_mangled_name = StrInit();
                sym.source_function_id    = source_function_id;
                sym.target_function_id    = target_function_id;

                JR_OBJ(si, {
                    JR_FLT_KV(si, "distance", sym.distance);
                    JR_STR_KV(si, "name", sym.function_name);
                });

                VecPushBack(&symbols, sym);
            });
        });
    });

    if (symbols.length != 2) {
        printf("[DEBUG] Symbols length check failed: expected 2, got %zu\n", symbols.length);
        success = false;
    }

    if (symbols.length >= 2) {
        AnnSymbol* sym1 = &VecAt(&symbols, 0);
        AnnSymbol* sym2 = &VecAt(&symbols, 1);

        if (!(sym1->source_function_id == 12345 || sym1->source_function_id == 54321)) {
            printf(
                "[DEBUG] Sym1 source function ID check failed: expected 12345 or 54321, got %llu\n",
                sym1->source_function_id
            );
            success = false;
        }

        if (!(sym1->target_function_id == 67890 || sym1->target_function_id == 98765)) {
            printf(
                "[DEBUG] Sym1 target function ID check failed: expected 67890 or 98765, got %llu\n",
                sym1->target_function_id
            );
            success = false;
        }

        if (!(sym1->distance > 0.8 && sym1->distance < 1.0)) {
            printf("[DEBUG] Sym1 distance check failed: expected 0.8-1.0, got %f\n", sym1->distance);
            success = false;
        }

        if (!(sym2->source_function_id == 12345 || sym2->source_function_id == 54321)) {
            printf(
                "[DEBUG] Sym2 source function ID check failed: expected 12345 or 54321, got %llu\n",
                sym2->source_function_id
            );
            success = false;
        }

        if (!(sym2->target_function_id == 67890 || sym2->target_function_id == 98765)) {
            printf(
                "[DEBUG] Sym2 target function ID check failed: expected 67890 or 98765, got %llu\n",
                sym2->target_function_id
            );
            success = false;
        }

        if (!(sym2->distance > 0.8 && sym2->distance < 1.0)) {
            printf("[DEBUG] Sym2 distance check failed: expected 0.8-1.0, got %f\n", sym2->distance);
            success = false;
        }
    }

    StrDeinit(&json);
    VecDeinit(&symbols);
    return success;
}

// Test complex API response similar to your example - Fixed initialization
bool test_complex_api_response(void) {
    printf("Testing complex API response structure\n");

    bool success = true;
    Str  json    = StrInitFromZstr(
        "{\"status\": true, \"message\": \"Success\", \"data\": {\"12345\": {\"67890\": {\"distance\": 0.85, "
            "\"nearest_neighbor_analysis_id\": 999, \"nearest_neighbor_binary_id\": 888, "
            "\"nearest_neighbor_analysis_name\": \"test_analysis\", \"nearest_neighbor_function_name\": \"main_func\", "
            "\"nearest_neighbor_sha_256_hash\": \"abc123\", \"nearest_neighbor_debug\": true, "
            "\"nearest_neighbor_function_name_mangled\": \"_Z4main\"}}}}"
    );
    StrIter si = StrIterFromStr(json);

    ApiResponse response = {false, StrInit(), VecInitWithDeepCopy(NULL, AnnSymbolDeinit)};

    JR_OBJ(si, {
        JR_BOOL_KV(si, "status", response.status);
        JR_STR_KV(si, "message", response.message);

        if (response.status) {
            JR_OBJ_KV(si, "data", {
                u64 source_function_id = strtoull(key.data, NULL, 10);
                JR_OBJ(si, {
                    // Properly initialize all Str fields
                    AnnSymbol sym             = {0};
                    sym.analysis_name         = StrInit();
                    sym.function_name         = StrInit();
                    sym.sha256                = StrInit();
                    sym.function_mangled_name = StrInit();
                    sym.source_function_id    = source_function_id;
                    sym.target_function_id    = strtoull(key.data, NULL, 10);

                    JR_OBJ(si, {
                        JR_FLT_KV(si, "distance", sym.distance);
                        JR_INT_KV(si, "nearest_neighbor_analysis_id", sym.analysis_id);
                        JR_INT_KV(si, "nearest_neighbor_binary_id", sym.binary_id);
                        JR_STR_KV(si, "nearest_neighbor_analysis_name", sym.analysis_name);
                        JR_STR_KV(si, "nearest_neighbor_function_name", sym.function_name);
                        JR_STR_KV(si, "nearest_neighbor_sha_256_hash", sym.sha256);
                        JR_BOOL_KV(si, "nearest_neighbor_debug", sym.debug);
                        JR_STR_KV(si, "nearest_neighbor_function_name_mangled", sym.function_mangled_name);
                    });

                    VecPushBack(&response.data, sym);
                });
            });
        }
    });

    // Debug status check
    if (response.status != true) {
        printf("[DEBUG] Status check failed: expected true, got %s\n", response.status ? "true" : "false");
        success = false;
    }

    // Debug message check
    if (StrCmpCstr(&response.message, "Success", 7) != 0) {
        printf("[DEBUG] Message check failed: expected 'Success', got '");
        for (size i = 0; i < response.message.length; i++) {
            printf("%c", response.message.data[i]);
        }
        printf("'\n");
        success = false;
    }

    // Debug data length check
    if (response.data.length != 1) {
        printf("[DEBUG] Data length check failed: expected 1, got %zu\n", response.data.length);
        success = false;
    }

    if (response.data.length > 0) {
        AnnSymbol* sym = &VecAt(&response.data, 0);

        // Debug individual symbol checks
        if (sym->source_function_id != 12345) {
            printf("[DEBUG] Source function ID check failed: expected 12345, got %llu\n", sym->source_function_id);
            success = false;
        }

        if (sym->target_function_id != 67890) {
            printf("[DEBUG] Target function ID check failed: expected 67890, got %llu\n", sym->target_function_id);
            success = false;
        }

        if (!(sym->distance > 0.84 && sym->distance < 0.86)) {
            printf("[DEBUG] Distance check failed: expected 0.84-0.86, got %f\n", sym->distance);
            success = false;
        }

        if (sym->analysis_id != 999) {
            printf("[DEBUG] Analysis ID check failed: expected 999, got %llu\n", sym->analysis_id);
            success = false;
        }

        if (sym->binary_id != 888) {
            printf("[DEBUG] Binary ID check failed: expected 888, got %llu\n", sym->binary_id);
            success = false;
        }

        if (StrCmpCstr(&sym->analysis_name, "test_analysis", 13) != 0) {
            printf("[DEBUG] Analysis name check failed: expected 'test_analysis', got '");
            for (size i = 0; i < sym->analysis_name.length; i++) {
                printf("%c", sym->analysis_name.data[i]);
            }
            printf("'\n");
            success = false;
        }

        if (StrCmpCstr(&sym->function_name, "main_func", 9) != 0) {
            printf(
                "[DEBUG] Function name check failed: expected 'main_func', got string of length %zu\n",
                sym->function_name.length
            );
            success = false;
        }

        if (StrCmpCstr(&sym->sha256, "abc123", 6) != 0) {
            printf("[DEBUG] SHA256 check failed: expected 'abc123', got '");
            for (size i = 0; i < sym->sha256.length; i++) {
                printf("%c", sym->sha256.data[i]);
            }
            printf("'\n");
            success = false;
        }

        if (sym->debug != true) {
            printf("[DEBUG] Debug flag check failed: expected true, got %s\n", sym->debug ? "true" : "false");
            success = false;
        }

        if (StrCmpCstr(&sym->function_mangled_name, "_Z4main", 7) != 0) {
            printf("[DEBUG] Mangled name check failed: expected '_Z4main', got '");
            for (size i = 0; i < sym->function_mangled_name.length; i++) {
                printf("%c", sym->function_mangled_name.data[i]);
            }
            printf("'\n");
            success = false;
        }
    }

    StrDeinit(&json);
    StrDeinit(&response.message);
    VecDeinit(&response.data);
    return success;
}

// Test function info parsing
bool test_function_info_parsing(void) {
    printf("Testing function info parsing\n");

    bool    success = true;
    Str     json    = StrInitFromZstr("{\"id\": 12345, \"name\": \"test_func\", \"size\": 1024, \"vaddr\": 4096}");
    StrIter si      = StrIterFromStr(json);

    FunctionInfo info = {0};
    info.name         = StrInit();

    JR_OBJ(si, {
        JR_INT_KV(si, "id", info.id);
        JR_STR_KV(si, "name", info.name);
        JR_INT_KV(si, "size", info.size);
        JR_INT_KV(si, "vaddr", info.vaddr);
    });

    if (info.id != 12345) {
        printf("[DEBUG] Function ID check failed: expected 12345, got %llu\n", info.id);
        success = false;
    }

    if (StrCmpCstr(&info.name, "test_func", 9) != 0) {
        printf("[DEBUG] Function name check failed: expected 'test_func', got '");
        for (size i = 0; i < info.name.length; i++) {
            printf("%c", info.name.data[i]);
        }
        printf("'\n");
        success = false;
    }

    if (info.size != 1024) {
        printf("[DEBUG] Function size check failed: expected 1024, got %llu\n", info.size);
        success = false;
    }

    if (info.vaddr != 4096) {
        printf("[DEBUG] Function vaddr check failed: expected 4096, got %llu\n", info.vaddr);
        success = false;
    }

    StrDeinit(&json);
    StrDeinit(&info.name);
    return success;
}

// Test model info parsing
bool test_model_info_parsing(void) {
    printf("Testing model info parsing\n");

    bool    success = true;
    Str     json    = StrInitFromZstr("{\"id\": 54321, \"name\": \"test_model\"}");
    StrIter si      = StrIterFromStr(json);

    ModelInfo info = {0};
    info.name      = StrInit();

    JR_OBJ(si, {
        JR_INT_KV(si, "id", info.id);
        JR_STR_KV(si, "name", info.name);
    });

    if (info.id != 54321) {
        printf("[DEBUG] Model ID check failed: expected 54321, got %llu\n", info.id);
        success = false;
    }

    if (StrCmpCstr(&info.name, "test_model", 10) != 0) {
        printf("[DEBUG] Model name check failed: expected 'test_model', got '");
        for (size i = 0; i < info.name.length; i++) {
            printf("%c", info.name.data[i]);
        }
        printf("'\n");
        success = false;
    }

    StrDeinit(&json);
    StrDeinit(&info.name);
    return success;
}

// Test search results with tags - Fixed implementation
bool test_search_results_with_tags(void) {
    printf("Testing search results with tags\n");

    bool success = true;
    Str  json    = StrInitFromZstr(
        "{\"binary_id\": 888, \"binary_name\": \"test_binary\", \"analysis_id\": 999, \"sha256\": \"abc123\", "
            "\"created_at\": \"2024-04-01\", \"model_id\": 12345, \"model_name\": \"test_model\", \"owned_by\": \"user1\"}"
    );
    StrIter si = StrIterFromStr(json);

    SearchResult result = {0};
    result.binary_name  = StrInit();
    result.sha256       = StrInit();
    result.tags         = VecInitWithDeepCopyT(result.tags, NULL, StrDeinit);
    result.created_at   = StrInit();
    result.model_name   = StrInit();
    result.owned_by     = StrInit();

    JR_OBJ(si, {
        JR_INT_KV(si, "binary_id", result.binary_id);
        JR_STR_KV(si, "binary_name", result.binary_name);
        JR_INT_KV(si, "analysis_id", result.analysis_id);
        JR_STR_KV(si, "sha256", result.sha256);
        JR_STR_KV(si, "created_at", result.created_at);
        JR_INT_KV(si, "model_id", result.model_id);
        JR_STR_KV(si, "model_name", result.model_name);
        JR_STR_KV(si, "owned_by", result.owned_by);
    });

    if (result.binary_id != 888) {
        printf("[DEBUG] Binary ID check failed: expected 888, got %llu\n", result.binary_id);
        success = false;
    }

    if (StrCmpCstr(&result.binary_name, "test_binary", 11) != 0) {
        printf("[DEBUG] Binary name check failed: expected 'test_binary', got '");
        for (size i = 0; i < result.binary_name.length; i++) {
            printf("%c", result.binary_name.data[i]);
        }
        printf("'\n");
        success = false;
    }

    if (result.analysis_id != 999) {
        printf("[DEBUG] Analysis ID check failed: expected 999, got %llu\n", result.analysis_id);
        success = false;
    }

    if (StrCmpCstr(&result.sha256, "abc123", 6) != 0) {
        printf("[DEBUG] SHA256 check failed: expected 'abc123', got '");
        for (size i = 0; i < result.sha256.length; i++) {
            printf("%c", result.sha256.data[i]);
        }
        printf("'\n");
        success = false;
    }

    if (result.model_id != 12345) {
        printf("[DEBUG] Model ID check failed: expected 12345, got %llu\n", result.model_id);
        success = false;
    }

    if (StrCmpCstr(&result.model_name, "test_model", 10) != 0) {
        printf("[DEBUG] Model name check failed: expected 'test_model', got '");
        for (size i = 0; i < result.model_name.length; i++) {
            printf("%c", result.model_name.data[i]);
        }
        printf("'\n");
        success = false;
    }

    if (StrCmpCstr(&result.owned_by, "user1", 5) != 0) {
        printf("[DEBUG] Owned by check failed: expected 'user1', got '");
        for (size i = 0; i < result.owned_by.length; i++) {
            printf("%c", result.owned_by.data[i]);
        }
        printf("'\n");
        success = false;
    }

    StrDeinit(&json);
    SearchResultDeinit(&result);
    return success;
}

// Test conditional parsing
bool test_conditional_parsing(void) {
    printf("Testing conditional parsing\n");

    bool success = true;
    Str  json    = StrInitFromZstr(
        "{\"status\": true, \"message\": \"Success\", \"data\": {\"12345\": {\"67890\": {\"distance\": 0.85, "
            "\"nearest_neighbor_analysis_id\": 999, \"nearest_neighbor_binary_id\": 888, "
            "\"nearest_neighbor_analysis_name\": \"test_analysis\", \"nearest_neighbor_function_name\": \"main_func\", "
            "\"nearest_neighbor_sha_256_hash\": \"abc123\", \"nearest_neighbor_debug\": true, "
            "\"nearest_neighbor_function_name_mangled\": \"_Z4main\"}}}}"
    );
    StrIter si = StrIterFromStr(json);

    ApiResponse response = {false, StrInit(), VecInitWithDeepCopy(NULL, AnnSymbolDeinit)};

    printf("[DEBUG] About to parse JSON...\n");

    JR_OBJ(si, {
        JR_BOOL_KV(si, "status", response.status);
        JR_STR_KV(si, "message", response.message);

        printf("[DEBUG] Parsed status: %s, message: '", response.status ? "true" : "false");
        for (size i = 0; i < response.message.length; i++) {
            printf("%c", response.message.data[i]);
        }
        printf("'\n");

        if (response.status) {
            printf("[DEBUG] Status is true, parsing data...\n");
            JR_OBJ_KV(si, "data", {
                u64 source_function_id = strtoull(key.data, NULL, 10);
                printf("[DEBUG] Source function ID from key: %llu\n", source_function_id);
                JR_OBJ(si, {
                    // Properly initialize all Str fields
                    AnnSymbol sym             = {0};
                    sym.analysis_name         = StrInit();
                    sym.function_name         = StrInit();
                    sym.sha256                = StrInit();
                    sym.function_mangled_name = StrInit();
                    sym.source_function_id    = source_function_id;
                    sym.target_function_id    = strtoull(key.data, NULL, 10);

                    printf("[DEBUG] Target function ID from key: %llu\n", sym.target_function_id);

                    JR_OBJ(si, {
                        JR_FLT_KV(si, "distance", sym.distance);
                        JR_INT_KV(si, "nearest_neighbor_analysis_id", sym.analysis_id);
                        JR_INT_KV(si, "nearest_neighbor_binary_id", sym.binary_id);
                        JR_STR_KV(si, "nearest_neighbor_analysis_name", sym.analysis_name);
                        JR_STR_KV(si, "nearest_neighbor_function_name", sym.function_name);
                        JR_STR_KV(si, "nearest_neighbor_sha_256_hash", sym.sha256);
                        JR_BOOL_KV(si, "nearest_neighbor_debug", sym.debug);
                        JR_STR_KV(si, "nearest_neighbor_function_name_mangled", sym.function_mangled_name);
                    });

                    printf(
                        "[DEBUG] Parsed symbol - distance: %f, analysis_id: %llu, binary_id: %llu\n",
                        sym.distance,
                        sym.analysis_id,
                        sym.binary_id
                    );

                    VecPushBack(&response.data, sym);
                    printf("[DEBUG] Added symbol to vector, length now: %zu\n", response.data.length);
                });
            });
        } else {
            printf("[DEBUG] Status is false, skipping data parsing\n");
        }
    });

    printf("[DEBUG] Finished parsing, response.data.length = %zu\n", response.data.length);

    // Debug checks
    if (response.status != true) {
        printf("[DEBUG] Status check failed: expected true, got %s\n", response.status ? "true" : "false");
        success = false;
    }

    if (StrCmpCstr(&response.message, "Success", 7) != 0) {
        printf("[DEBUG] Message check failed: expected 'Success', got '");
        for (size i = 0; i < response.message.length; i++) {
            printf("%c", response.message.data[i]);
        }
        printf("'\n");
        success = false;
    }

    if (response.data.length != 1) {
        printf("[DEBUG] Data length check failed: expected 1, got %zu\n", response.data.length);
        success = false;
    }

    if (response.data.length > 0) {
        AnnSymbol* sym = &VecAt(&response.data, 0);

        if (sym->source_function_id != 12345) {
            printf("[DEBUG] Source function ID check failed: expected 12345, got %llu\n", sym->source_function_id);
            success = false;
        }

        if (sym->target_function_id != 67890) {
            printf("[DEBUG] Target function ID check failed: expected 67890, got %llu\n", sym->target_function_id);
            success = false;
        }

        if (!(sym->distance > 0.84 && sym->distance < 0.86)) {
            printf("[DEBUG] Distance check failed: expected 0.84-0.86, got %f\n", sym->distance);
            success = false;
        }

        if (sym->analysis_id != 999) {
            printf("[DEBUG] Analysis ID check failed: expected 999, got %llu\n", sym->analysis_id);
            success = false;
        }

        if (sym->binary_id != 888) {
            printf("[DEBUG] Binary ID check failed: expected 888, got %llu\n", sym->binary_id);
            success = false;
        }

        if (StrCmpCstr(&sym->analysis_name, "test_analysis", 13) != 0) {
            printf("[DEBUG] Analysis name check failed: expected 'test_analysis', got '");
            for (size i = 0; i < sym->analysis_name.length; i++) {
                printf("%c", sym->analysis_name.data[i]);
            }
            printf("'\n");
            success = false;
        }

        if (StrCmpCstr(&sym->function_name, "main_func", 9) != 0) {
            printf(
                "[DEBUG] Function name check failed: expected 'main_func', got string of length %zu\n",
                sym->function_name.length
            );
            success = false;
        }

        if (StrCmpCstr(&sym->sha256, "abc123", 6) != 0) {
            printf("[DEBUG] SHA256 check failed: expected 'abc123', got '");
            for (size i = 0; i < sym->sha256.length; i++) {
                printf("%c", sym->sha256.data[i]);
            }
            printf("'\n");
            success = false;
        }

        if (sym->debug != true) {
            printf("[DEBUG] Debug flag check failed: expected true, got %s\n", sym->debug ? "true" : "false");
            success = false;
        }

        if (StrCmpCstr(&sym->function_mangled_name, "_Z4main", 7) != 0) {
            printf("[DEBUG] Mangled name check failed: expected '_Z4main', got '");
            for (size i = 0; i < sym->function_mangled_name.length; i++) {
                printf("%c", sym->function_mangled_name.data[i]);
            }
            printf("'\n");
            success = false;
        }
    }

    StrDeinit(&json);
    StrDeinit(&response.message);
    VecDeinit(&response.data);
    return success;
}

// Test status response pattern
bool test_status_response_pattern(void) {
    printf("Testing status response pattern\n");

    bool success = true;
    Str  json    = StrInitFromZstr(
        "{\"status\": true, \"message\": \"Success\", \"data\": {\"12345\": {\"67890\": {\"distance\": 0.85, "
            "\"nearest_neighbor_analysis_id\": 999, \"nearest_neighbor_binary_id\": 888, "
            "\"nearest_neighbor_analysis_name\": \"test_analysis\", \"nearest_neighbor_function_name\": \"main_func\", "
            "\"nearest_neighbor_sha_256_hash\": \"abc123\", \"nearest_neighbor_debug\": true, "
            "\"nearest_neighbor_function_name_mangled\": \"_Z4main\"}}}}"
    );
    StrIter si = StrIterFromStr(json);

    ApiResponse response = {false, StrInit(), VecInitWithDeepCopy(NULL, AnnSymbolDeinit)};

    printf("[DEBUG] About to parse JSON...\n");

    JR_OBJ(si, {
        JR_BOOL_KV(si, "status", response.status);
        JR_STR_KV(si, "message", response.message);

        printf("[DEBUG] Parsed status: %s, message: '", response.status ? "true" : "false");
        for (size i = 0; i < response.message.length; i++) {
            printf("%c", response.message.data[i]);
        }
        printf("'\n");

        if (response.status) {
            printf("[DEBUG] Status is true, parsing data...\n");
            JR_OBJ_KV(si, "data", {
                u64 source_function_id = strtoull(key.data, NULL, 10);
                printf("[DEBUG] Source function ID from key: %llu\n", source_function_id);
                JR_OBJ(si, {
                    // Properly initialize all Str fields
                    AnnSymbol sym             = {0};
                    sym.analysis_name         = StrInit();
                    sym.function_name         = StrInit();
                    sym.sha256                = StrInit();
                    sym.function_mangled_name = StrInit();
                    sym.source_function_id    = source_function_id;
                    sym.target_function_id    = strtoull(key.data, NULL, 10);

                    printf("[DEBUG] Target function ID from key: %llu\n", sym.target_function_id);

                    JR_OBJ(si, {
                        JR_FLT_KV(si, "distance", sym.distance);
                        JR_INT_KV(si, "nearest_neighbor_analysis_id", sym.analysis_id);
                        JR_INT_KV(si, "nearest_neighbor_binary_id", sym.binary_id);
                        JR_STR_KV(si, "nearest_neighbor_analysis_name", sym.analysis_name);
                        JR_STR_KV(si, "nearest_neighbor_function_name", sym.function_name);
                        JR_STR_KV(si, "nearest_neighbor_sha_256_hash", sym.sha256);
                        JR_BOOL_KV(si, "nearest_neighbor_debug", sym.debug);
                        JR_STR_KV(si, "nearest_neighbor_function_name_mangled", sym.function_mangled_name);
                    });

                    printf(
                        "[DEBUG] Parsed symbol - distance: %f, analysis_id: %llu, binary_id: %llu\n",
                        sym.distance,
                        sym.analysis_id,
                        sym.binary_id
                    );

                    VecPushBack(&response.data, sym);
                    printf("[DEBUG] Added symbol to vector, length now: %zu\n", response.data.length);
                });
            });
        } else {
            printf("[DEBUG] Status is false, skipping data parsing\n");
        }
    });

    printf("[DEBUG] Finished parsing, response.data.length = %zu\n", response.data.length);

    // Debug checks
    if (response.status != true) {
        printf("[DEBUG] Status check failed: expected true, got %s\n", response.status ? "true" : "false");
        success = false;
    }

    if (StrCmpCstr(&response.message, "Success", 7) != 0) {
        printf("[DEBUG] Message check failed: expected 'Success', got '");
        for (size i = 0; i < response.message.length; i++) {
            printf("%c", response.message.data[i]);
        }
        printf("'\n");
        success = false;
    }

    if (response.data.length != 1) {
        printf("[DEBUG] Data length check failed: expected 1, got %zu\n", response.data.length);
        success = false;
    }

    if (response.data.length > 0) {
        AnnSymbol* sym = &VecAt(&response.data, 0);

        if (sym->source_function_id != 12345) {
            printf("[DEBUG] Source function ID check failed: expected 12345, got %llu\n", sym->source_function_id);
            success = false;
        }

        if (sym->target_function_id != 67890) {
            printf("[DEBUG] Target function ID check failed: expected 67890, got %llu\n", sym->target_function_id);
            success = false;
        }

        if (!(sym->distance > 0.84 && sym->distance < 0.86)) {
            printf("[DEBUG] Distance check failed: expected 0.84-0.86, got %f\n", sym->distance);
            success = false;
        }

        if (sym->analysis_id != 999) {
            printf("[DEBUG] Analysis ID check failed: expected 999, got %llu\n", sym->analysis_id);
            success = false;
        }

        if (sym->binary_id != 888) {
            printf("[DEBUG] Binary ID check failed: expected 888, got %llu\n", sym->binary_id);
            success = false;
        }

        if (StrCmpCstr(&sym->analysis_name, "test_analysis", 13) != 0) {
            printf("[DEBUG] Analysis name check failed: expected 'test_analysis', got '");
            for (size i = 0; i < sym->analysis_name.length; i++) {
                printf("%c", sym->analysis_name.data[i]);
            }
            printf("'\n");
            success = false;
        }

        if (StrCmpCstr(&sym->function_name, "main_func", 9) != 0) {
            printf("[DEBUG] Function name check failed: expected 'main_func', got '");
            for (size i = 0; i < sym->function_name.length; i++) {
                printf("%c", sym->function_name.data[i]);
            }
            printf("'\n");
            success = false;
        }

        if (StrCmpCstr(&sym->sha256, "abc123", 6) != 0) {
            printf("[DEBUG] SHA256 check failed: expected 'abc123', got '");
            for (size i = 0; i < sym->sha256.length; i++) {
                printf("%c", sym->sha256.data[i]);
            }
            printf("'\n");
            success = false;
        }

        if (sym->debug != true) {
            printf("[DEBUG] Debug flag check failed: expected true, got %s\n", sym->debug ? "true" : "false");
            success = false;
        }

        if (StrCmpCstr(&sym->function_mangled_name, "_Z4main", 7) != 0) {
            printf("[DEBUG] Mangled name check failed: expected '_Z4main', got '");
            for (size i = 0; i < sym->function_mangled_name.length; i++) {
                printf("%c", sym->function_mangled_name.data[i]);
            }
            printf("'\n");
            success = false;
        }
    }

    StrDeinit(&json);
    StrDeinit(&response.message);
    VecDeinit(&response.data);
    return success;
}

// Main function that runs all tests
int main(int argc, char* argv[]) {
    // Array of test functions
    TestFunction tests[] = {
        test_basic_iterator_functionality,
        test_simple_json_object,
        test_two_level_nesting,
        test_three_level_nesting,
        test_dynamic_key_parsing,
        test_complex_api_response,
        test_function_info_parsing,
        test_model_info_parsing,
        test_search_results_with_tags,
        test_conditional_parsing,
        test_status_response_pattern
    };

    int test_count = sizeof(tests) / sizeof(tests[0]);

    // Use centralized test driver
    return run_test_suite(tests, test_count, NULL, 0, "Json.Read");
}
