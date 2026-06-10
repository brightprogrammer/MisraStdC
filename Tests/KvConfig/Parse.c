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

    result = result && (StrIterIndex(&si) == StrIterLength(&si));
    result = result && (MapPairCount(&cfg) == 3);
    result = result && KvConfigContains(&cfg, "host");
    result = result && host && StrCmp(host, "localhost") == 0;
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

    result = result && (StrIterIndex(&si) == StrIterLength(&si));
    result = result && (MapPairCount(&cfg) == 4);
    result = result && path && StrCmp(path, "/srv/my app") == 0;
    result = result && user && StrCmp(user, "root") == 0;
    result = result && greet && StrCmp(greet, "hello world") == 0;
    result = result && empty && (StrLen(empty) == 0);

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
    result = result && (StrBegin(&host_copy) != NULL);
    result = result && (StrBegin(&host_copy) != StrBegin(stored_host));
    // exact content (StrCmp below) subsumes any non-emptiness check.
    result = result && (StrCmp(&host_copy, "localhost") == 0);
    result = result && (StrCmp(stored_host, "localhost") == 0);

    StrBegin(&host_copy)[0] = 'L';

    result = result && (StrCmp(&host_copy, "Localhost") == 0);
    result = result && (StrCmp(stored_host, "localhost") == 0);

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

    result = result && (StrIterIndex(&si) == 0);
    result = result && KvConfigGetBool(&cfg, "valid", &enabled) && enabled;
    result = result && !KvConfigContains(&cfg, "later");

    StrDeinit(&src);
    MapDeinit(&cfg);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Every documented boolean spelling must parse to the right value, and
// only documented spellings are accepted (header contract:
// true/false/yes/no/on/off/1/0, case-insensitive).
static bool test_kvconfig_all_bool_spellings(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    KvConfig         cfg   = KvConfigInit(&alloc);
    Str              src   = StrInitFromZstr(
        "t1 = true\n"
                       "t2 = YES\n"
                       "t3 = On\n"
                       "t4 = 1\n"
                       "f1 = false\n"
                       "f2 = No\n"
                       "f3 = OFF\n"
                       "f4 = 0\n"
                       "bad = nope\n",
        &alloc
    );
    StrIter input  = StrIterFromStr(src);
    bool    result = true;
    bool    v      = false;

    (void)KvConfigParse(input, &cfg);

    Zstr trues[]  = {"t1", "t2", "t3", "t4"};
    Zstr falses[] = {"f1", "f2", "f3", "f4"};
    for (u64 i = 0; i < 4; i++) {
        v      = false;
        result = result && KvConfigGetBool(&cfg, trues[i], &v) && v;
    }
    for (u64 i = 0; i < 4; i++) {
        v      = true;
        result = result && KvConfigGetBool(&cfg, falses[i], &v) && !v;
    }
    // A non-boolean spelling is rejected.
    result = result && !KvConfigGetBool(&cfg, "bad", &v);

    StrDeinit(&src);
    MapDeinit(&cfg);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// The Str*-key dispatch arms of the typed accessors are part of the
// public _Generic contract; exercise contains/get/bool/i64/f64 with a
// real Str* key (string literals route to the Zstr arm instead).
static bool test_kvconfig_str_key_accessors(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    KvConfig         cfg   = KvConfigInit(&alloc);
    Str              src   = StrInitFromZstr(
        "name = misra\n"
                       "workers = 16\n"
                       "ratio = 0.25\n"
                       "enabled = yes\n",
        &alloc
    );
    StrIter input  = StrIterFromStr(src);
    bool    result = true;

    (void)KvConfigParse(input, &cfg);

    Str k_name    = StrInitFromZstr("name", &alloc);
    Str k_missing = StrInitFromZstr("missing", &alloc);
    Str k_workers = StrInitFromZstr("workers", &alloc);
    Str k_ratio   = StrInitFromZstr("ratio", &alloc);
    Str k_enabled = StrInitFromZstr("enabled", &alloc);

    // contains via Str*
    result = result && KvConfigContains(&cfg, &k_name);
    result = result && !KvConfigContains(&cfg, &k_missing);

    // get copy via Str*
    Str got = KvConfigGet(&cfg, &k_name);
    result  = result && (StrCmp(&got, "misra") == 0);
    StrDeinit(&got);

    // typed accessors via Str*
    i64  workers = 0;
    f64  ratio   = 0.0;
    bool en      = false;
    result       = result && KvConfigGetI64(&cfg, &k_workers, &workers) && (workers == 16);
    result       = result && KvConfigGetF64(&cfg, &k_ratio, &ratio) && (ratio > 0.24 && ratio < 0.26);
    result       = result && KvConfigGetBool(&cfg, &k_enabled, &en) && en;

    StrDeinit(&k_name);
    StrDeinit(&k_missing);
    StrDeinit(&k_workers);
    StrDeinit(&k_ratio);
    StrDeinit(&k_enabled);
    StrDeinit(&src);
    MapDeinit(&cfg);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Unquoted values: a trailing inline comment together with the
// whitespace before it is stripped, leaving exactly the value text.
static bool test_kvconfig_inline_comment_trimming(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    KvConfig         cfg   = KvConfigInit(&alloc);
    Str              src   = StrInitFromZstr(
        "a = value one     # trailing comment\n"
                       "b = value two\t\t; tab-spaced comment\n"
                       "c = no-trailing-space\n",
        &alloc
    );
    StrIter input  = StrIterFromStr(src);
    Str    *a      = NULL;
    Str    *b      = NULL;
    Str    *c      = NULL;
    bool    result = true;

    (void)KvConfigParse(input, &cfg);

    a = KvConfigGetPtr(&cfg, "a");
    b = KvConfigGetPtr(&cfg, "b");
    c = KvConfigGetPtr(&cfg, "c");

    result = result && a && (StrCmp(a, "value one") == 0);
    result = result && b && (StrCmp(b, "value two") == 0);
    result = result && c && (StrCmp(c, "no-trailing-space") == 0);

    StrDeinit(&src);
    MapDeinit(&cfg);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// CRLF (Windows) line endings parse identically to LF: the trailing
// carriage return is consumed, not folded into the value.
static bool test_kvconfig_crlf_line_endings(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    KvConfig         cfg   = KvConfigInit(&alloc);
    Str              src   = StrInitFromZstr(
        "host = localhost\r\n"
                       "port = 8080\r\n"
                       "name = misra\r\n",
        &alloc
    );
    StrIter input  = StrIterFromStr(src);
    StrIter si     = KvConfigParse(input, &cfg);
    Str    *host   = NULL;
    Str    *name   = NULL;
    i64     port   = 0;
    bool    result = true;

    host = KvConfigGetPtr(&cfg, "host");
    name = KvConfigGetPtr(&cfg, "name");

    result = result && (StrIterIndex(&si) == StrIterLength(&si));
    result = result && (MapPairCount(&cfg) == 3);
    // The CR must not survive in the parsed value.
    result = result && host && (StrCmp(host, "localhost") == 0);
    result = result && name && (StrCmp(name, "misra") == 0);
    result = result && KvConfigGetI64(&cfg, "port", &port) && (port == 8080);

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
        test_kvconfig_all_bool_spellings,
        test_kvconfig_str_key_accessors,
        test_kvconfig_inline_comment_trimming,
        test_kvconfig_crlf_line_endings,
    };

    WriteFmt("[INFO] Starting KvConfig.Parse tests\n\n");
    return run_test_suite(tests, (int)(sizeof(tests) / sizeof(tests[0])), NULL, 0, "KvConfig.Parse");
}
