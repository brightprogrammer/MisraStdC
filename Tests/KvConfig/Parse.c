#include <Misra/Parsers/KvConfig.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Io.h>

#include "../Util/TestRunner.h"

static bool test_kvconfig_basic_parse(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    KvConfig         cfg   = KvConfigInit(&alloc);
    Str              src   = StrInitFromZstr(
        "host = localhost\n"
                       "port = 8080\n"
                       "debug = true\n",
        &alloc
    );
    StrIter input  = StrIterFromStr(src);
    StrIter si     = KvConfigParse(input, &cfg);
    Str    *host   = KvConfigGetPtr(&cfg, "host");
    i64     port   = 0;
    bool    debug  = false;
    bool    result = true;

    result = result && (si.pos == si.length);
    result = result && (MapPairCount(&cfg) == 3);
    result = result && KvConfigContains(&cfg, "host");
    result = result && host && StrCmpZstr(host, "localhost") == 0;
    result = result && KvConfigGetI64(&cfg, "port", &port) && (port == 8080);
    result = result && KvConfigGetBool(&cfg, "debug", &debug) && debug;

    StrDeinit(&src);
    MapDeinit(&cfg);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_kvconfig_comments_quotes_and_duplicates(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    KvConfig         cfg   = KvConfigInit(&alloc);
    Str              src   = StrInitFromZstr(
        "# comment line\n"
                       "path = \"/srv/my app\"   # keep spaces in quotes\n"
                       "user: admin\n"
                       "user = root\n"
                       "; another comment\n"
                       "greeting = hello world   ; inline comment\n"
                       "empty =\n",
        &alloc
    );
    StrIter input  = StrIterFromStr(src);
    StrIter si     = KvConfigParse(input, &cfg);
    Str    *path   = KvConfigGetPtr(&cfg, "path");
    Str    *user   = KvConfigGetPtr(&cfg, "user");
    Str    *greet  = KvConfigGetPtr(&cfg, "greeting");
    Str    *empty  = KvConfigGetPtr(&cfg, "empty");
    bool    result = true;

    result = result && (si.pos == si.length);
    result = result && (MapPairCount(&cfg) == 4);
    result = result && path && StrCmpZstr(path, "/srv/my app") == 0;
    result = result && user && StrCmpZstr(user, "root") == 0;
    result = result && greet && StrCmpZstr(greet, "hello world") == 0;
    result = result && empty && (empty->length == 0);

    StrDeinit(&src);
    MapDeinit(&cfg);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_kvconfig_get_returns_copy(void) {
    DefaultAllocator alloc       = DefaultAllocatorInit();
    KvConfig         cfg         = KvConfigInit(&alloc);
    Str              src         = StrInitFromZstr("host = localhost\n", &alloc);
    StrIter          input       = StrIterFromStr(src);
    Str              host_copy   = StrInit(&alloc);
    Str             *stored_host = NULL;
    bool             result      = true;

    (void)KvConfigParse(input, &cfg);

    stored_host = KvConfigGetPtr(&cfg, "host");
    host_copy   = KvConfigGet(&cfg, "host");

    result = result && stored_host;
    result = result && (host_copy.data != NULL);
    result = result && (host_copy.length > 0);
    result = result && (host_copy.data != stored_host->data);
    result = result && (StrCmpZstr(&host_copy, "localhost") == 0);
    result = result && (StrCmpZstr(stored_host, "localhost") == 0);

    host_copy.data[0] = 'L';

    result = result && (StrCmpZstr(&host_copy, "Localhost") == 0);
    result = result && (StrCmpZstr(stored_host, "localhost") == 0);

    StrDeinit(&host_copy);
    StrDeinit(&src);
    MapDeinit(&cfg);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_kvconfig_numeric_and_bool_accessors(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    KvConfig         cfg   = KvConfigInit(&alloc);
    Str              src   = StrInitFromZstr(
        "workers = 16\n"
                       "pi = 3.14159\n"
                       "enabled = On\n"
                       "disabled = off\n"
                       "invalid_bool = maybe\n",
        &alloc
    );
    i64     workers  = 0;
    f64     pi       = 0.0;
    bool    enabled  = false;
    bool    disabled = true;
    bool    result   = true;
    StrIter input    = StrIterFromStr(src);

    (void)KvConfigParse(input, &cfg);

    result = result && KvConfigGetI64(&cfg, "workers", &workers) && (workers == 16);
    result = result && KvConfigGetF64(&cfg, "pi", &pi) && (pi > 3.1415 && pi < 3.1416);
    result = result && KvConfigGetBool(&cfg, "enabled", &enabled) && enabled;
    result = result && KvConfigGetBool(&cfg, "disabled", &disabled) && !disabled;
    result = result && !KvConfigGetBool(&cfg, "invalid_bool", &enabled);
    result = result && !KvConfigGetI64(&cfg, "pi", &workers);
    result = result && !KvConfigGetF64(&cfg, "missing", &pi);

    StrDeinit(&src);
    MapDeinit(&cfg);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_kvconfig_invalid_line_fails(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    KvConfig         cfg   = KvConfigInit(&alloc);
    Str              src   = StrInitFromZstr(
        "valid = yes\n"
                       "broken line\n"
                       "later = no\n",
        &alloc
    );
    StrIter input   = StrIterFromStr(src);
    StrIter si      = KvConfigParse(input, &cfg);
    bool    enabled = false;
    bool    result  = true;

    result = result && (si.pos == 0);
    result = result && KvConfigGetBool(&cfg, "valid", &enabled) && enabled;
    result = result && !KvConfigContains(&cfg, "later");

    StrDeinit(&src);
    MapDeinit(&cfg);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

int main(void) {
    TestFunction tests[] = {
        test_kvconfig_basic_parse,
        test_kvconfig_comments_quotes_and_duplicates,
        test_kvconfig_get_returns_copy,
        test_kvconfig_numeric_and_bool_accessors,
        test_kvconfig_invalid_line_fails,
    };

    WriteFmt("[INFO] Starting KvConfig.Parse tests\n\n");
    return run_test_suite(tests, (int)(sizeof(tests) / sizeof(tests[0])), NULL, 0, "KvConfig.Parse");
}
