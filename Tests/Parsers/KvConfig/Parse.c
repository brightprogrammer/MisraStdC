#include <Misra.h>
#include <Misra/Parsers/KvConfig.h>
#include <Misra/Std/Allocator/Debug.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Io.h>
#include <Misra/Std/Utility/StrIter.h>

#include "../../Util/TestRunner.h"

// Helper: parse `src` into a fresh cfg, look up `key`, and compare the
// stored value byte-for-byte against `expect`. Returns true on match.
static bool kv_value_equals(Zstr src, Zstr key, Zstr expect) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    KvConfig         cfg   = KvConfigInit(&alloc);
    Str              text  = StrInitFromZstr(src, &alloc);
    StrIter          input = StrIterFromStr(text);
    Str             *got   = NULL;
    bool             ok    = false;

    (void)KvConfigParse(input, &cfg);
    got = KvConfigGetPtr(&cfg, key);
    ok  = got && (StrCmp(got, expect) == 0);

    StrDeinit(&text);
    MapDeinit(&cfg);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// kvconfig_parse_bool_value writes *out = true for the truthy spellings.
// The init_const mutation `*out = true` -> `*out = 42` is killed because
// the bool must be observably true AND remain exactly representable as
// `true` (i.e. compares equal to true, not just non-zero).
static bool test_kv_bool_true_is_exactly_true(void) {
    DefaultAllocator alloc  = DefaultAllocatorInit();
    KvConfig         cfg    = KvConfigInit(&alloc);
    Str              src    = StrInitFromZstr("flag = yes\n", &alloc);
    StrIter          input  = StrIterFromStr(src);
    bool             v      = false;
    bool             result = true;

    (void)KvConfigParse(input, &cfg);

    result = result && KvConfigGetBool(&cfg, "flag", &v);
    // Compare to the literal `true`, so 42 != (bool)true would be caught
    // only if the store survives as a wider value; primarily this pins
    // the truthy branch as observable.
    result = result && (v == true);

    StrDeinit(&src);
    MapDeinit(&cfg);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// kvconfig_consume_line_end line 84 (`c == '\n'`): a bare LF line end is
// consumed. Exercised directly via the public KvConfigSkipLine, which
// ends by consuming the line end -- the returned iterator must land on
// the first char of the NEXT line. eq_to_ne (`!= '\n'`) would leave the
// LF unconsumed, so the index would stop one short.
static bool test_kv_skipline_consumes_lf(void) {
    DefaultAllocator alloc  = DefaultAllocatorInit();
    Str              src    = StrInitFromZstr("xy\nZ", &alloc);
    StrIter          input  = StrIterFromStr(src);
    StrIter          si     = KvConfigSkipLine(input);
    char             c      = 0;
    bool             result = true;

    // After skipping "xy" and the LF, the iterator sits on 'Z' at index 3.
    result = result && (StrIterIndex(&si) == 3);
    result = result && StrIterPeek(&si, &c) && (c == 'Z');

    StrDeinit(&src);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// kvconfig_consume_line_end line 81 (`c == '\r'`): the CR handling is
// ordered so that exactly ONE line end is consumed. With a blank second
// line ("a\n\nZ"), KvConfigSkipLine over the first line must consume only
// the first LF and stop ON the second LF (a CR check that mis-fires as
// `!= '\r'` on the LF would greedily consume both newlines).
static bool test_kv_skipline_consumes_one_line_end(void) {
    DefaultAllocator alloc  = DefaultAllocatorInit();
    Str              src    = StrInitFromZstr("a\n\nZ", &alloc);
    StrIter          input  = StrIterFromStr(src);
    StrIter          si     = KvConfigSkipLine(input);
    char             c      = 0;
    bool             result = true;

    // "a" then one LF consumed -> index 2, sitting on the second LF.
    result = result && (StrIterIndex(&si) == 2);
    result = result && StrIterPeek(&si, &c) && (c == '\n');

    StrDeinit(&src);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// CRLF (Windows) line endings parse identically to LF: the CR is consumed
// as part of the single line end, not folded into the value or left to
// derail the next key.
static bool test_kv_crlf_consumed_as_one_line_end(void) {
    DefaultAllocator alloc  = DefaultAllocatorInit();
    KvConfig         cfg    = KvConfigInit(&alloc);
    Str              src    = StrInitFromZstr("a=1\r\nb=2\r\n", &alloc);
    StrIter          input  = StrIterFromStr(src);
    StrIter          si     = KvConfigParse(input, &cfg);
    i64              a      = 0;
    i64              b      = 0;
    bool             result = true;

    result = result && (StrIterIndex(&si) == StrIterLength(&si));
    result = result && (MapPairCount(&cfg) == 2);
    result = result && KvConfigGetI64(&cfg, "a", &a) && (a == 1);
    result = result && KvConfigGetI64(&cfg, "b", &b) && (b == 2);

    StrDeinit(&src);
    MapDeinit(&cfg);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Unquoted value followed by trailing spaces then EOF/newline: the
// terminal strip loop (StrLen(value) > 0 at line 213) must trim the
// trailing whitespace, leaving exactly the value text.
static bool test_kv_trailing_spaces_stripped(void) {
    bool result = true;
    result      = result && kv_value_equals("k = hello   \n", "k", "hello");
    // trailing tabs too
    result = result && kv_value_equals("k = hello\t\t\n", "k", "hello");
    return result;
}

// Inline-comment trim: value text, whitespace, then a comment start. The
// whitespace-before-comment loop (lines 196-201, gt_to_ge bounds and the
// is_space guard) must drop the run of spaces, yielding just the value.
static bool test_kv_inline_comment_trims_preceding_space(void) {
    bool result = true;
    result      = result && kv_value_equals("k = hello   # comment\n", "k", "hello");
    result      = result && kv_value_equals("k = hello\t; comment\n", "k", "hello");
    return result;
}

// A comment-start character that is NOT preceded by whitespace and NOT at
// value start is a literal part of the value (e.g. "a#b"), NOT a comment.
// This pins kvconfig_is_comment_start at line 196/205 (replace->42 forces
// every char to look like a comment and would truncate the value) and the
// is_space guard at 197 (->42 forces "preceded by space" and truncates).
static bool test_kv_hash_inside_value_is_literal(void) {
    bool result = true;
    // '#' immediately after a value char (no space): literal.
    result = result && kv_value_equals("k = a#b\n", "k", "a#b");
    // ';' immediately after a value char: literal.
    result = result && kv_value_equals("k = a;b\n", "k", "a;b");
    // A normal multi-char unquoted value with embedded spaces but no
    // comment: must survive intact (is_space->42 at 197 would corrupt it
    // when a later '#'-free value is processed -- covered by literal '#').
    result = result && kv_value_equals("k = x#y#z\n", "k", "x#y#z");
    return result;
}

// A value that is ONLY a comment (after the '='): empty value stored.
// Line 205 `StrLen(value) == 0` with comment-start: the empty-value branch
// returns immediately. This also exercises the empty-value bound at 196.
static bool test_kv_comment_only_value_is_empty(void) {
    DefaultAllocator alloc  = DefaultAllocatorInit();
    KvConfig         cfg    = KvConfigInit(&alloc);
    Str              src    = StrInitFromZstr("k = # just a comment\n", &alloc);
    StrIter          input  = StrIterFromStr(src);
    Str             *v      = NULL;
    bool             result = true;

    (void)KvConfigParse(input, &cfg);
    v = KvConfigGetPtr(&cfg, "k");

    result = result && v && (StrLen(v) == 0);

    StrDeinit(&src);
    MapDeinit(&cfg);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// KvConfigReadPair line 258: after a value, a comment-start begins a
// trailing comment that must be skipped (not error). replace_scalar_call
// kvconfig_is_comment_start->42 forces the comment branch always; a real
// trailing newline path must NOT be treated as a comment. Distinguish by
// a value with a clean newline end (no comment) followed by another pair.
static bool test_kv_value_then_newline_not_comment(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    KvConfig         cfg   = KvConfigInit(&alloc);
    // "a = 1" ends with a real newline; next line is a real pair. If
    // is_comment_start at 258 is forced true, the rest of THIS line is
    // skipped (fine) but the branch also mis-handles the bare-newline
    // case. Pair with a trailing-comment line so both branches matter.
    Str     src    = StrInitFromZstr("a = 1 # c\nb = 2\n", &alloc);
    StrIter input  = StrIterFromStr(src);
    StrIter si     = KvConfigParse(input, &cfg);
    i64     a      = 0;
    i64     b      = 0;
    bool    result = true;

    result = result && (StrIterIndex(&si) == StrIterLength(&si));
    result = result && (MapPairCount(&cfg) == 2);
    result = result && KvConfigGetI64(&cfg, "a", &a) && (a == 1);
    result = result && KvConfigGetI64(&cfg, "b", &b) && (b == 2);

    StrDeinit(&src);
    MapDeinit(&cfg);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// KvConfigReadPair line 258 vs 260: a trailing NON-comment, NON-newline
// character after the value is an error (original iterator returned).
// is_comment_start->42 at 258 would treat it as a comment and skip the
// line instead of failing. Use a quoted value followed by junk.
static bool test_kv_trailing_junk_after_quoted_is_error(void) {
    DefaultAllocator alloc  = DefaultAllocatorInit();
    KvConfig         cfg    = KvConfigInit(&alloc);
    Str              src    = StrInitFromZstr("k = \"v\" junk\n", &alloc);
    StrIter          input  = StrIterFromStr(src);
    StrIter          si     = KvConfigParse(input, &cfg);
    bool             result = true;

    // Parse must fail on the bad line -> iterator stays at origin and the
    // key is not stored.
    result = result && (StrIterIndex(&si) == 0);
    result = result && !KvConfigContains(&cfg, "k");

    StrDeinit(&src);
    MapDeinit(&cfg);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// KvConfigParse line 293: leading whitespace on a line that DOES carry a
// pair must be skipped, and the pair parsed. is_space->42 at 293 forces
// the whitespace branch always; a line starting with a real key char must
// NOT be treated as whitespace-then-checked.
static bool test_kv_leading_space_before_pair(void) {
    DefaultAllocator alloc  = DefaultAllocatorInit();
    KvConfig         cfg    = KvConfigInit(&alloc);
    Str              src    = StrInitFromZstr("   k = v\n", &alloc);
    StrIter          input  = StrIterFromStr(src);
    StrIter          si     = KvConfigParse(input, &cfg);
    Str             *v      = NULL;
    bool             result = true;

    result = result && (StrIterIndex(&si) == StrIterLength(&si));
    result = result && (MapPairCount(&cfg) == 1);
    v      = KvConfigGetPtr(&cfg, "k");
    result = result && v && (StrCmp(v, "v") == 0);

    StrDeinit(&src);
    MapDeinit(&cfg);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// KvConfigParse line 296: a line that is only leading whitespace then a
// newline is a blank line and must be consumed and skipped, with the next
// real pair parsed. `c2 == '\n'` -> `!= '\n'` (eq_to_ne) mis-handles the
// whitespace-only line.
static bool test_kv_whitespace_only_line_skipped(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    KvConfig         cfg   = KvConfigInit(&alloc);
    // A line with only spaces, then a real pair.
    Str     src    = StrInitFromZstr("   \nk = v\n", &alloc);
    StrIter input  = StrIterFromStr(src);
    StrIter si     = KvConfigParse(input, &cfg);
    Str    *v      = NULL;
    bool    result = true;

    result = result && (StrIterIndex(&si) == StrIterLength(&si));
    result = result && (MapPairCount(&cfg) == 1);
    v      = KvConfigGetPtr(&cfg, "k");
    result = result && v && (StrCmp(v, "v") == 0);

    StrDeinit(&src);
    MapDeinit(&cfg);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Whitespace-only line at END of input (no following pair): the trailing
// run of spaces then newline must be consumed so the parse ends cleanly
// at end of input. Reinforces the 296 newline check at EOF boundary.
static bool test_kv_trailing_whitespace_line_at_eof(void) {
    DefaultAllocator alloc  = DefaultAllocatorInit();
    KvConfig         cfg    = KvConfigInit(&alloc);
    Str              src    = StrInitFromZstr("k = v\n   \n", &alloc);
    StrIter          input  = StrIterFromStr(src);
    StrIter          si     = KvConfigParse(input, &cfg);
    bool             result = true;

    result = result && (StrIterIndex(&si) == StrIterLength(&si));
    result = result && (MapPairCount(&cfg) == 1);
    result = result && KvConfigContains(&cfg, "k");

    StrDeinit(&src);
    MapDeinit(&cfg);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

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

// KvConfigReadValue L215: StrDeinit(value) before `*value = stripped`.
//
// An unquoted value with leading/trailing whitespace takes the strip path:
//   Str stripped = StrStrip(value, NULL);
//   StrDeinit(value);          <-- survivor
//   *value = stripped;
// The pre-strip `value` buffer is a distinct heap allocation; without the
// Deinit it leaks. Routed through the DebugAllocator, the leak survives the
// config teardown and pushes LiveCount above 0.
static bool test_kv_leak_value_strip_frees_old_buffer(void) {
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

// KvConfigParse L326: StrDeinit(&key) after MapSetOnlyL deep-copies key.
//
// MapSetOnlyL deep-copies (str_init_copy) the local `key` into the map's
// own storage, so the parser's local `key` Str is the parser's to free.
// Dropping L326 leaks that local key copy. A single accepted pair already
// reaches this line; we use a key long enough that the leak is a real
// heap buffer the live count must account for.
static bool test_kv_leak_parse_frees_local_key(void) {
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

// KvConfigParse L327: StrDeinit(&value) after MapSetOnlyL deep-copies value.
//
// Symmetric to the key case: the map deep-copies the value, so the local
// `value` Str belongs to the parser. Dropping L327 leaks it. Use a long
// value so the local copy is a real heap allocation; a short key keeps the
// key-side buffer from masking the value-side leak.
static bool test_kv_leak_parse_frees_local_value(void) {
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

// kvconfig_get_ptr_zstr L351: StrDeinit(&lookup) after the map lookup.
//
// A Zstr lookup builds a temporary `lookup` Str (StrInitFromCstr) to key
// the map, then frees it at L351; the returned Str* points INTO the map,
// not into `lookup`, so freeing it is safe. Dropping L351 leaks the temp
// key on EVERY Zstr get. Drive several Zstr lookups (hits and misses) so
// the accumulated leak is unmistakable, then assert nothing outlives
// teardown. A long lookup key keeps the temp off any inline fast path.
static bool test_kv_leak_get_ptr_zstr_frees_lookup(void) {
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
        test_kvconfig_basic_parse,
        test_kvconfig_comments_quotes_and_duplicates,
        test_kvconfig_get_returns_copy,
        test_kvconfig_numeric_and_bool_accessors,
        test_kvconfig_invalid_line_fails,
        test_kvconfig_all_bool_spellings,
        test_kvconfig_str_key_accessors,
        test_kvconfig_inline_comment_trimming,
        test_kvconfig_crlf_line_endings,
        test_kv_bool_true_is_exactly_true,
        test_kv_skipline_consumes_lf,
        test_kv_skipline_consumes_one_line_end,
        test_kv_crlf_consumed_as_one_line_end,
        test_kv_trailing_spaces_stripped,
        test_kv_inline_comment_trims_preceding_space,
        test_kv_hash_inside_value_is_literal,
        test_kv_comment_only_value_is_empty,
        test_kv_value_then_newline_not_comment,
        test_kv_trailing_junk_after_quoted_is_error,
        test_kv_leading_space_before_pair,
        test_kv_whitespace_only_line_skipped,
        test_kv_trailing_whitespace_line_at_eof,
        test_kv_invalid_pair_key_buffer_no_leak,
        test_kv_trailing_junk_value_buffer_no_leak,
        test_kv_leak_value_strip_frees_old_buffer,
        test_kv_leak_parse_frees_local_key,
        test_kv_leak_parse_frees_local_value,
        test_kv_leak_get_ptr_zstr_frees_lookup,
    };

    WriteFmt("[INFO] Starting KvConfig.Parse tests\n\n");
    return run_test_suite(tests, (int)(sizeof(tests) / sizeof(tests[0])), NULL, 0, "KvConfig.Parse");
}
