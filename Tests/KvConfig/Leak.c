/// file : tests/kvconfig/leak.c
/// Success-path leak-guard tests for KvConfig (DebugAllocator live-count round-trips).
///
/// Every allocation the parser makes is routed through an explicit
/// DebugAllocator. After the parsed config is torn down (MapDeinit) every
/// transient Str the parser allocated on the success path must already be
/// freed, so DebugAllocatorLiveCount(&dbg) == 0. Each test pins one
/// internal *Deinit on a reached branch: dropping that Deinit (the
/// cxx_remove_void_call survivor) leaks a buffer the live count then sees,
/// flipping the assertion to false -> the mutant is killed.

#include <Misra.h>
#include <Misra/Parsers/KvConfig.h>
#include <Misra/Std/Allocator/Debug.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Utility/StrIter.h>

#include "../Util/TestRunner.h"

// =============================================================================
// KvConfigReadValue L215: StrDeinit(value) before `*value = stripped`.
//
// An unquoted value with leading/trailing whitespace takes the strip path:
//   Str stripped = StrStrip(value, NULL);
//   StrDeinit(value);          <-- survivor
//   *value = stripped;
// The pre-strip `value` buffer is a distinct heap allocation; without the
// Deinit it leaks. Routed through the DebugAllocator, the leak survives the
// config teardown and pushes LiveCount above 0.

bool test_kv_leak_value_strip_frees_old_buffer(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    KvConfig cfg = KvConfigInit(adbg);
    // Trailing whitespace before the newline forces StrStrip to produce a
    // NEW buffer and the old one to be released at L215. A long-ish value
    // keeps both buffers off any small-string fast path.
    Str     text  = StrInitFromZstr("key =   spaced-out-value-here   \n", adbg);
    StrIter input = StrIterFromStr(text);

    (void)KvConfigParse(input, &cfg);

    Str *got = KvConfigGetPtr(&cfg, "key");
    bool ok  = got && (StrCmp(got, "spaced-out-value-here") == 0);

    StrDeinit(&text);
    MapDeinit(&cfg);

    // The stripped-away old value buffer must already be freed.
    ok = ok && (DebugAllocatorLiveCount(&dbg) == 0);
    ok = ok && (DebugAllocatorLiveBytes(&dbg) == 0);

    DebugAllocatorDeinit(&dbg);
    return ok;
}

// =============================================================================
// KvConfigParse L326: StrDeinit(&key) after MapSetOnlyL deep-copies key.
//
// MapSetOnlyL deep-copies (str_init_copy) the local `key` into the map's
// own storage, so the parser's local `key` Str is the parser's to free.
// Dropping L326 leaks that local key copy. A single accepted pair already
// reaches this line; we use a key long enough that the leak is a real
// heap buffer the live count must account for.

bool test_kv_leak_parse_frees_local_key(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    KvConfig cfg   = KvConfigInit(adbg);
    Str      text  = StrInitFromZstr("a-reasonably-long-config-key = v\n", adbg);
    StrIter  input = StrIterFromStr(text);

    (void)KvConfigParse(input, &cfg);

    Str *got = KvConfigGetPtr(&cfg, "a-reasonably-long-config-key");
    bool ok  = got && (StrCmp(got, "v") == 0);

    StrDeinit(&text);
    MapDeinit(&cfg);

    // The parser's local key copy must already be freed.
    ok = ok && (DebugAllocatorLiveCount(&dbg) == 0);
    ok = ok && (DebugAllocatorLiveBytes(&dbg) == 0);

    DebugAllocatorDeinit(&dbg);
    return ok;
}

// =============================================================================
// KvConfigParse L327: StrDeinit(&value) after MapSetOnlyL deep-copies value.
//
// Symmetric to the key case: the map deep-copies the value, so the local
// `value` Str belongs to the parser. Dropping L327 leaks it. Use a long
// value so the local copy is a real heap allocation; a short key keeps the
// key-side buffer from masking the value-side leak.

bool test_kv_leak_parse_frees_local_value(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    KvConfig cfg   = KvConfigInit(adbg);
    Str      text  = StrInitFromZstr("k = a-reasonably-long-config-value\n", adbg);
    StrIter  input = StrIterFromStr(text);

    (void)KvConfigParse(input, &cfg);

    Str *got = KvConfigGetPtr(&cfg, "k");
    bool ok  = got && (StrCmp(got, "a-reasonably-long-config-value") == 0);

    StrDeinit(&text);
    MapDeinit(&cfg);

    // The parser's local value copy must already be freed.
    ok = ok && (DebugAllocatorLiveCount(&dbg) == 0);
    ok = ok && (DebugAllocatorLiveBytes(&dbg) == 0);

    DebugAllocatorDeinit(&dbg);
    return ok;
}

// =============================================================================
// kvconfig_get_ptr_zstr L351: StrDeinit(&lookup) after the map lookup.
//
// A Zstr lookup builds a temporary `lookup` Str (StrInitFromCstr) to key
// the map, then frees it at L351; the returned Str* points INTO the map,
// not into `lookup`, so freeing it is safe. Dropping L351 leaks the temp
// key on EVERY Zstr get. Drive several Zstr lookups (hits and misses) so
// the accumulated leak is unmistakable, then assert nothing outlives
// teardown. A long lookup key keeps the temp off any inline fast path.

bool test_kv_leak_get_ptr_zstr_frees_lookup(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    KvConfig cfg   = KvConfigInit(adbg);
    Str      text  = StrInitFromZstr("present-config-key = value\n", adbg);
    StrIter  input = StrIterFromStr(text);

    (void)KvConfigParse(input, &cfg);

    bool ok = true;
    // Each KvConfigGetPtr with a Zstr key routes through kvconfig_get_ptr_zstr,
    // building and (at L351) freeing a temp lookup Str.
    Str *hit = KvConfigGetPtr(&cfg, "present-config-key");
    ok       = ok && (hit != NULL) && (StrCmp(hit, "value") == 0);

    // Misses also build + free a lookup; pile several on so a dropped L351
    // leaks multiple buffers.
    ok = ok && (KvConfigGetPtr(&cfg, "absent-config-key-0001") == NULL);
    ok = ok && (KvConfigGetPtr(&cfg, "absent-config-key-0002") == NULL);
    ok = ok && (KvConfigGetPtr(&cfg, "absent-config-key-0003") == NULL);

    StrDeinit(&text);
    MapDeinit(&cfg);

    // Every transient lookup key must already be freed.
    ok = ok && (DebugAllocatorLiveCount(&dbg) == 0);
    ok = ok && (DebugAllocatorLiveBytes(&dbg) == 0);

    DebugAllocatorDeinit(&dbg);
    return ok;
}

int main(void) {
    TestFunction tests[] = {
        test_kv_leak_value_strip_frees_old_buffer,
        test_kv_leak_parse_frees_local_key,
        test_kv_leak_parse_frees_local_value,
        test_kv_leak_get_ptr_zstr_frees_lookup,
    };
    TestFunction deadend_tests[] = {0};
    (void)deadend_tests;
    return run_test_suite(tests, (int)(sizeof(tests) / sizeof(tests[0])), NULL, 0, "KvConfig.Leak");
}
