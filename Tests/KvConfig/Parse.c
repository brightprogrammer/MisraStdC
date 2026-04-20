#include <Misra/Parsers/KvConfig.h>
#include <Misra/Std/Io.h>

#include "../Util/TestRunner.h"

static bool test_kvconfig_basic_parse(void) {
    KvConfig cfg = KvConfigInit();
    Str      src = StrInitFromZstr(
        "host = localhost\n"
             "port = 8080\n"
             "debug = true\n"
    );
    StrIter input  = StrIterFromStr(src);
    StrIter si     = KvConfigParse(input, &cfg);
    i64     port   = 0;
    bool    debug  = false;
    bool    result = true;

    result = result && (si.pos == si.length);
    result = result && (KvConfigLen(&cfg) == 3);
    result = result && KvConfigContains(&cfg, "host");
    result = result && KvConfigGet(&cfg, "host") && StrCmpZstr(KvConfigGet(&cfg, "host"), "localhost") == 0;
    result = result && KvConfigGetI64(&cfg, "port", &port) && (port == 8080);
    result = result && KvConfigGetBool(&cfg, "debug", &debug) && debug;

    StrDeinit(&src);
    KvConfigDeinit(&cfg);
    return result;
}

static bool test_kvconfig_comments_quotes_and_duplicates(void) {
    KvConfig cfg = KvConfigInit();
    Str      src = StrInitFromZstr(
        "# comment line\n"
             "path = \"/srv/my app\"   # keep spaces in quotes\n"
             "user: admin\n"
             "user = root\n"
             "; another comment\n"
             "greeting = hello world   ; inline comment\n"
             "empty =\n"
    );
    StrIter input  = StrIterFromStr(src);
    StrIter si     = KvConfigParse(input, &cfg);
    bool    result = true;

    result = result && (si.pos == si.length);
    result = result && (KvConfigLen(&cfg) == 4);
    result = result && KvConfigGet(&cfg, "path") && StrCmpZstr(KvConfigGet(&cfg, "path"), "/srv/my app") == 0;
    result = result && KvConfigGet(&cfg, "user") && StrCmpZstr(KvConfigGet(&cfg, "user"), "root") == 0;
    result = result && KvConfigGet(&cfg, "greeting") && StrCmpZstr(KvConfigGet(&cfg, "greeting"), "hello world") == 0;
    result = result && KvConfigGet(&cfg, "empty") && KvConfigGet(&cfg, "empty")->length == 0;

    StrDeinit(&src);
    KvConfigDeinit(&cfg);
    return result;
}

static bool test_kvconfig_numeric_and_bool_accessors(void) {
    KvConfig cfg = KvConfigInit();
    Str      src = StrInitFromZstr(
        "workers = 16\n"
             "pi = 3.14159\n"
             "enabled = On\n"
             "disabled = off\n"
             "invalid_bool = maybe\n"
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
    KvConfigDeinit(&cfg);
    return result;
}

static bool test_kvconfig_invalid_line_fails(void) {
    KvConfig cfg = KvConfigInit();
    Str      src = StrInitFromZstr(
        "valid = yes\n"
             "broken line\n"
             "later = no\n"
    );
    StrIter input   = StrIterFromStr(src);
    StrIter si      = KvConfigParse(input, &cfg);
    bool    enabled = false;
    bool    result  = true;

    result = result && (si.pos == 0);
    result = result && KvConfigGetBool(&cfg, "valid", &enabled) && enabled;
    result = result && !KvConfigContains(&cfg, "later");

    StrDeinit(&src);
    KvConfigDeinit(&cfg);
    return result;
}

int main(void) {
    TestFunction tests[] = {
        test_kvconfig_basic_parse,
        test_kvconfig_comments_quotes_and_duplicates,
        test_kvconfig_numeric_and_bool_accessors,
        test_kvconfig_invalid_line_fails,
    };

    WriteFmt("[INFO] Starting KvConfig.Parse tests\n\n");
    return run_test_suite(tests, (int)(sizeof(tests) / sizeof(tests[0])), NULL, 0, "KvConfig.Parse");
}
