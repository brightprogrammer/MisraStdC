#include <Misra.h>
#include <Misra/Parsers/KvConfig.h>
#include <Misra/Std/Allocator/Debug.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Io.h>
#include <Misra/Std/Utility/StrIter.h>

#include "../Util/TestRunner.h"

// Lean DebugAllocator config: no trace capture / overflow guards / freed
// history. We only need the live-allocation round-trip count.
static DebugAllocator kv_lean_debug_allocator(void) {
    DebugAllocatorConfig cfg = {.capture_traces = false, .detect_overflow = false, .track_freed_history = false};
    return DebugAllocatorInitWith(cfg);
}

// Drive a full parse of `src` through a DebugAllocator and confirm every
// allocation made during parsing is released by MapDeinit + the text's
// StrDeinit. A surviving `StrDeinit(&key)` / `StrDeinit(&value)` on a
// KvConfigParse branch where that Str actually holds a heap buffer leaks
// it, leaving DebugAllocatorLiveCount > 0.
static bool kv_parse_is_leak_free(Zstr src) {
    DebugAllocator dbg  = kv_lean_debug_allocator();
    Allocator     *base = ALLOCATOR_OF(&dbg);
    bool           ok   = true;

    KvConfig cfg   = KvConfigInit(base);
    Str      text  = StrInitFromZstr(src, base);
    StrIter  input = StrIterFromStr(text);

    (void)KvConfigParse(input, &cfg);

    StrDeinit(&text);
    MapDeinit(&cfg);

    ok = DebugAllocatorLiveCount(&dbg) == 0;
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// KvConfigParse line 320 (StrDeinit(&key)): an invalid line makes
// KvConfigReadPair return its original iterator (read_si == si at 319), so
// KvConfigParse frees key/value and bails with saved_si. "broken line" is
// rejected at the missing-separator check AFTER KvConfigReadKey already
// pushed "broken" into `key` -- so `key` owns a heap buffer that StrClear
// keeps and that the 320 StrDeinit must free. Removing it leaks the buffer.
static bool test_kv_invalid_pair_key_buffer_no_leak(void) {
    bool result = true;
    // ReadKey populates `key` (a real buffer); no separator -> failure
    // branch frees it at 320.
    result = result && kv_parse_is_leak_free("broken line\n");
    // A valid pair first (stored + freed by MapDeinit), then the broken
    // line exercising the failure free path after prior insertions.
    result = result && kv_parse_is_leak_free("ok = 1\nbroken line\n");
    return result;
}

// KvConfigParse line 321 (StrDeinit(&value)): the failure branch must also
// free `value` when it carries a buffer. A quoted value followed by junk
// makes KvConfigReadValue fill `value` ("v") before KvConfigReadPair detects
// the trailing junk at line 260, StrClears (keeping the buffer) and returns
// saved_si. Back in KvConfigParse the 319 failure branch frees key/value;
// removing the 321 StrDeinit leaks the value buffer.
static bool test_kv_trailing_junk_value_buffer_no_leak(void) {
    bool result = true;
    // Double-quoted value with a real buffer, then trailing junk -> fail.
    result = result && kv_parse_is_leak_free("k = \"v\" junk\n");
    // Single-quoted form, longer value buffer.
    result = result && kv_parse_is_leak_free("key = 'value' trailing\n");
    return result;
}

int main(void) {
    TestFunction tests[] = {
        test_kv_invalid_pair_key_buffer_no_leak,
        test_kv_trailing_junk_value_buffer_no_leak,
    };
    TestFunction deadend_tests[] = {0};
    (void)deadend_tests;

    WriteFmt("[INFO] Starting KvConfig.Blind tests\n\n");
    return run_test_suite(tests, (int)(sizeof(tests) / sizeof(tests[0])), deadend_tests, 0, "KvConfig.Blind");
}
