#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Memory.h>

#include <Misra/Types.h>

// Include test utilities
#include "../Util/TestRunner.h"

// Function prototypes
bool test_str_cmp(void);
bool test_str_find(void);
bool test_str_starts_ends_with(void);
bool test_str_replace(void);
bool test_str_split(void);
bool test_str_strip(void);
bool test_str_contains_index(void);

// Test string comparison functions
bool test_str_cmp(void) {
    WriteFmt("Testing StrCmp variants\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    Str s1 = StrInitFromZstr("Hello", &alloc);
    Str s2 = StrInitFromZstr("Hello", &alloc);
    Str s3 = StrInitFromZstr("World", &alloc);
    Str s4 = StrInitFromZstr("Hello World", &alloc);

    // Test StrCmp with equal strings
    int  cmp1   = StrCmp(&s1, &s2);
    bool result = (cmp1 == 0);

    // Test StrCmp with different strings (H < W in ASCII)
    int cmp2 = StrCmp(&s1, &s3);
    result   = result && (cmp2 < 0);

    // Test StrCmp with string prefix - ZstrCompare compares the entire strings
    int cmp3 = StrCmp(&s1, &s4);
    result   = result && (cmp3 < 0); // "Hello" comes before "Hello World" lexicographically

    // Test StrCmp (Cstr key, key_len)
    int cmp4 = StrCmp(&s1, "Hello", 5);
    result   = result && (cmp4 == 0);

    int cmp5 = StrCmp(&s1, "World", 5);
    result   = result && (cmp5 != 0);

    StrDeinit(&s1);
    StrDeinit(&s2);
    StrDeinit(&s3);
    StrDeinit(&s4);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test string find functions
bool test_str_find(void) {
    WriteFmt("Testing StrFind variants\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    Str haystack = StrInitFromZstr("Hello World", &alloc);
    Str needle1  = StrInitFromZstr("World", &alloc);
    Str needle2  = StrInitFromZstr("Hello", &alloc);
    Str needle3  = StrInitFromZstr("NotFound", &alloc);

    // Test StrFind (Str * key) with match at end
    Zstr found1 = StrFind(&haystack, &needle1);
    bool result = (found1 != NULL && ZstrCompare(found1, "World") == 0);

    // Test StrFind (Str * key) with match at beginning
    Zstr found2 = StrFind(&haystack, &needle2);
    result      = result && (found2 != NULL && ZstrCompare(found2, "Hello World") == 0);

    // Test StrFind (Str * key) with no match
    Zstr found3 = StrFind(&haystack, &needle3);
    result      = result && (found3 == NULL);

    // Test StrFind (Zstr key)
    Zstr found4 = StrFind(&haystack, "World");
    result      = result && (found4 != NULL && ZstrCompare(found4, "World") == 0);

    // Test StrFind (Cstr key, key_len)
    Zstr found5 = StrFind(&haystack, "Wor", 3);
    result      = result && (found5 != NULL && ZstrCompareN(found5, "World", 3) == 0);

    StrDeinit(&haystack);
    StrDeinit(&needle1);
    StrDeinit(&needle2);
    StrDeinit(&needle3);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test string contains/index functions
bool test_str_contains_index(void) {
    WriteFmt("Testing StrContains and StrIndexOf variants\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    Str haystack = StrInitFromZstr("Hello World", &alloc);
    Str needle   = StrInitFromZstr("World", &alloc);

    bool result = StrContains(&haystack, &needle);
    result      = result && StrContains(&haystack, "Hello");
    result      = result && StrContains(&haystack, "lo Wo", 5);
    result      = result && (StrIndexOf(&haystack, &needle) == 6);
    result      = result && (StrIndexOf(&haystack, "Hello") == 0);
    result      = result && (StrIndexOf(&haystack, "World", 5) == 6);
    result      = result && !StrContains(&haystack, "missing");
    result      = result && (StrIndexOf(&haystack, "missing") == SIZE_MAX);
    result      = result && StrContains(&haystack, "");
    result      = result && (StrIndexOf(&haystack, "") == 0);

    StrDeinit(&haystack);
    StrDeinit(&needle);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test string starts/ends with functions
bool test_str_starts_ends_with(void) {
    WriteFmt("Testing StrStartsWith and StrEndsWith variants\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    Str s      = StrInitFromZstr("Hello World", &alloc);
    Str prefix = StrInitFromZstr("Hello", &alloc);
    Str suffix = StrInitFromZstr("World", &alloc);

    // Test Str-form
    bool result = StrStartsWith(&s, &prefix);

    // Test Str-form
    result = result && StrEndsWith(&s, &suffix);

    // Test Zstr-form (string literal)
    result = result && StrStartsWith(&s, "Hello");
    result = result && !StrStartsWith(&s, "World");

    // Test Zstr-form (string literal)
    result = result && StrEndsWith(&s, "World");
    result = result && !StrEndsWith(&s, "Hello");

    // Test Cstr-form (fixed-length view)
    result = result && StrStartsWith(&s, "Hell", 4);
    result = result && !StrStartsWith(&s, "Worl", 4);

    // Test Cstr-form (fixed-length view)
    result = result && StrEndsWith(&s, "orld", 4);
    result = result && !StrEndsWith(&s, "ello", 4);

    StrDeinit(&s);
    StrDeinit(&prefix);
    StrDeinit(&suffix);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test string replace functions
bool test_str_replace(void) {
    WriteFmt("Testing StrReplace variants\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    // Test Zstr-form (string literals)
    Str s1 = StrInitFromZstr("Hello World", &alloc);
    StrReplace(&s1, "World", "Universe", 1);
    bool result = (ZstrCompare(StrBegin(&s1), "Hello Universe") == 0);

    // Test multiple replacements
    StrDeinit(&s1);
    s1 = StrInitFromZstr("Hello Hello Hello", &alloc);
    StrReplace(&s1, "Hello", "Hi", 2);
    result = result && (ZstrCompare(StrBegin(&s1), "Hi Hi Hello") == 0);

    // Test Cstr-form (fixed-length views) - use the full "World" string instead of just "Wo"
    StrDeinit(&s1);
    s1 = StrInitFromZstr("Hello World", &alloc);
    StrReplace(&s1, "World", 5, "Universe", 8, 1);
    result = result && (ZstrCompare(StrBegin(&s1), "Hello Universe") == 0);

    // Test Str-form
    StrDeinit(&s1);
    s1          = StrInitFromZstr("Hello World", &alloc);
    Str find    = StrInitFromZstr("World", &alloc);
    Str replace = StrInitFromZstr("Universe", &alloc);
    StrReplace(&s1, &find, &replace, 1);
    result = result && (ZstrCompare(StrBegin(&s1), "Hello Universe") == 0);

    StrDeinit(&s1);
    StrDeinit(&find);
    StrDeinit(&replace);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test string split functions
bool test_str_split(void) {
    WriteFmt("Testing StrSplit and StrSplitToIters\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    // Test StrSplit
    Str  s     = StrInitFromZstr("Hello,World,Test", &alloc);
    Strs split = StrSplit(&s, ",");

    bool result = (VecLen(&split) == 3);
    if (VecLen(&split) >= 3) {
        result = result && (ZstrCompare(StrBegin(VecPtrAt(&split, 0)), "Hello") == 0);
        result = result && (ZstrCompare(StrBegin(VecPtrAt(&split, 1)), "World") == 0);
        result = result && (ZstrCompare(StrBegin(VecPtrAt(&split, 2)), "Test") == 0);
    }

    VecDeinit(&split);

    // Test StrSplitToIters
    StrIters iters = StrSplitToIters(&s, ",");
    result         = result && (VecLen(&iters) == 3);

    if (VecLen(&iters) >= 3) {
        // .length goes through StrIterLength; .data has no accessor
        // (the base-pointer of an Iter range is the Iter contract --
        // direct read is the documented usage for view types).
        StrIter *iter1       = VecPtrAt(&iters, 0);
        char     buffer1[10] = {0};
        MemCopy(buffer1, iter1->data, StrIterLength(iter1));
        result = result && (ZstrCompare(buffer1, "Hello") == 0);

        StrIter *iter2       = VecPtrAt(&iters, 1);
        char     buffer2[10] = {0};
        MemCopy(buffer2, iter2->data, StrIterLength(iter2));
        result = result && (ZstrCompare(buffer2, "World") == 0);

        StrIter *iter3       = VecPtrAt(&iters, 2);
        char     buffer3[10] = {0};
        MemCopy(buffer3, iter3->data, StrIterLength(iter3));
        result = result && (ZstrCompare(buffer3, "Test") == 0);
    }

    VecDeinit(&iters);
    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test string strip functions
bool test_str_strip(void) {
    WriteFmt("Testing StrStrip variants\n");
    DefaultAllocator alloc = DefaultAllocatorInit();


    // Test StrLStrip
    Str  s1       = StrInitFromZstr("  Hello  ", &alloc);
    Str  stripped = StrLStrip(&s1, NULL);
    bool result   = (ZstrCompare(StrBegin(&stripped), "Hello  ") == 0);
    StrDeinit(&stripped);

    // Test StrRStrip
    stripped = StrRStrip(&s1, NULL);
    result   = result && (ZstrCompare(StrBegin(&stripped), "  Hello") == 0);
    StrDeinit(&stripped);

    // Test StrStrip
    stripped = StrStrip(&s1, NULL);
    result   = result && (ZstrCompare(StrBegin(&stripped), "Hello") == 0);
    StrDeinit(&stripped);

    // Test with custom strip characters
    StrDeinit(&s1);
    s1 = StrInitFromZstr("***Hello***", &alloc);

    stripped = StrLStrip(&s1, "*");
    result   = result && (ZstrCompare(StrBegin(&stripped), "Hello***") == 0);
    StrDeinit(&stripped);

    stripped = StrRStrip(&s1, "*");
    result   = result && (ZstrCompare(StrBegin(&stripped), "***Hello") == 0);
    StrDeinit(&stripped);

    stripped = StrStrip(&s1, "*");
    result   = result && (ZstrCompare(StrBegin(&stripped), "Hello") == 0);
    StrDeinit(&stripped);

    StrDeinit(&s1);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_str_cmp_ignore_case(void);
bool test_str_cmp_ignore_case(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str hello_lc = StrInitFromZstr("hello", &alloc);
    Str hello_uc = StrInitFromZstr("HELLO", &alloc);
    Str hello_mc = StrInitFromZstr("HeLLo", &alloc);
    Str world    = StrInitFromZstr("world", &alloc);
    Str hello_x  = StrInitFromZstr("HelloX", &alloc); // longer

    // Equal under ASCII case folding.
    bool ok = StrCmpIgnoreCase(&hello_lc, &hello_uc) == 0;
    ok      = ok && StrCmpIgnoreCase(&hello_lc, &hello_mc) == 0;

    // 'h' lowers to 'h' (0x68), 'w' to 'w' (0x77); negative.
    ok = ok && StrCmpIgnoreCase(&hello_lc, &world) < 0;
    // Reverse direction.
    ok = ok && StrCmpIgnoreCase(&world, &hello_uc) > 0;

    // Length mismatch: hello < hellox under case-insensitive compare.
    ok = ok && StrCmpIgnoreCase(&hello_lc, &hello_x) < 0;

    // Cstr / Zstr variants share the same underlying helper.
    ok = ok && StrCmpIgnoreCase(&hello_lc, "HELLO") == 0;
    ok = ok && StrCmpIgnoreCase(&hello_uc, "world") < 0;
    ok = ok && StrCmpIgnoreCase(&hello_lc, "HELLO_extra", 5) == 0;
    ok = ok && StrCmpIgnoreCase(&hello_lc, "HellX", 5) != 0;

    // Non-ASCII bytes pass through verbatim (no Unicode folding).
    Str non_ascii_a = StrInitFromZstr("ABC\xC0", &alloc);
    Str non_ascii_b = StrInitFromZstr("abc\xC0", &alloc);
    ok              = ok && StrCmpIgnoreCase(&non_ascii_a, &non_ascii_b) == 0;

    StrDeinit(&hello_lc);
    StrDeinit(&hello_uc);
    StrDeinit(&hello_mc);
    StrDeinit(&world);
    StrDeinit(&hello_x);
    StrDeinit(&non_ascii_a);
    StrDeinit(&non_ascii_b);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ===========================================================================
// Mutation-survivor guards relocated from the Str.Mutants* staging suites.
// Each pins a caller-observable outcome of a comparison / find / contains /
// split / strip / replace path, or a validation barrier (deadend).
// ===========================================================================

// ---- str_ends_with_str (Str.Mutants5) -------------------------------------

// 471:12 cxx_replace_scalar_call -- the ends_with(...) result is replaced by a
// constant false, so a genuine suffix match returns false. "Hello World" ends
// with "World" must be true.
static bool test_str_ends_with_str_true(void) {
    WriteFmt("Testing StrEndsWith reports true on a match and false on a non-match\n");
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str s = StrInitFromZstr("Hello World", &alloc);
    // A genuine suffix must report true...
    Str  suffix = StrInitFromZstr("World", &alloc);
    bool match  = StrEndsWith(&s, &suffix); // Str* -> str_ends_with_str
    // ...and a non-suffix must report false. The replace_scalar_call mutant
    // forces the return to the constant 42 (true), so this false case is what
    // actually pins the call's result.
    Str  nonsuffix = StrInitFromZstr("Hello", &alloc);
    bool no_match  = StrEndsWith(&s, &nonsuffix);

    bool result = match && (no_match == false);

    StrDeinit(&s);
    StrDeinit(&suffix);
    StrDeinit(&nonsuffix);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// ---- strip_str / contains / split_to_iters (Str.Mutants6) -----------------

// strip_str 421:cxx_le_to_lt -- left-strip bound `start <= end` -> `start <
// end`. For an all-strip input the intact loop advances `start` one past the
// final char so the whole string is consumed (length 0); the mutant stops one
// char short and wrongly retains a single strip character.
static bool test_lstrip_all_strip_chars_empty(void) {
    WriteFmt("Testing StrLStrip drops every strip char\n");
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str  s        = StrInitFromZstr("***", &alloc);
    Str  stripped = StrLStrip(&s, "*");
    bool result   = (StrLen(&stripped) == 0);

    StrDeinit(&stripped);
    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// strip_str 427:cxx_ge_to_gt -- right-strip bound `end >= start` -> `end >
// start`. Symmetric to the left case: an all-strip input must collapse to an
// empty result; the mutant leaves a single trailing strip character.
static bool test_rstrip_all_strip_chars_empty(void) {
    WriteFmt("Testing StrRStrip drops every strip char\n");
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str  s        = StrInitFromZstr("***", &alloc);
    Str  stripped = StrRStrip(&s, "*");
    bool result   = (StrLen(&stripped) == 0);

    StrDeinit(&stripped);
    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// strip_str 432:cxx_ge_to_gt -- `new_len = end >= start ? end-start+1 : 0` ->
// `end > start ? ...`. When exactly one character survives stripping,
// `end == start`; intact keeps it (len 1), the mutant drops it (len 0).
static bool test_strip_single_surviving_char_kept(void) {
    WriteFmt("Testing StrStrip keeps a lone surviving char\n");
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str  s        = StrInitFromZstr("x", &alloc);
    Str  stripped = StrStrip(&s, "*");
    bool result   = (StrLen(&stripped) == 1) && (ZstrCompare(StrBegin(&stripped), "x") == 0);

    StrDeinit(&stripped);
    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// str_contains_str 386:cxx_eq_to_ne -- `if (key->length == 0) return true;`
// -> `!= 0`, which short-circuits every NON-empty key to true. A key that is
// not a substring must report false via the Str* path.
static bool test_contains_str_absent_key_false(void) {
    WriteFmt("Testing StrContains(Str*) absent key is false\n");
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str  haystack = StrInitFromZstr("Hello World", &alloc);
    Str  needle   = StrInitFromZstr("missing", &alloc);
    bool result   = (StrContains(&haystack, &needle) == false);

    StrDeinit(&needle);
    StrDeinit(&haystack);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// str_contains_str 390:cxx_replace_scalar_call -- the str_contains_cstr(...)
// result is replaced by a constant, fixing the answer for all non-empty keys.
// Asserting both a present and an absent needle via the Str* path kills any
// constant replacement.
static bool test_contains_str_present_and_absent(void) {
    WriteFmt("Testing StrContains(Str*) present vs absent\n");
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str  haystack = StrInitFromZstr("Hello World", &alloc);
    Str  present  = StrInitFromZstr("World", &alloc);
    Str  absent   = StrInitFromZstr("missing", &alloc);
    bool result   = (StrContains(&haystack, &present) == true) && (StrContains(&haystack, &absent) == false);

    StrDeinit(&absent);
    StrDeinit(&present);
    StrDeinit(&haystack);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// str_contains_cstr 372:cxx_replace_scalar_call -- str_index_of_cstr(...) is
// replaced by a constant scalar, making `... != SIZE_MAX` always true so every
// key reports contained. An absent key via the Cstr/Zstr path must be false.
static bool test_contains_cstr_absent_key_false(void) {
    WriteFmt("Testing StrContains(Zstr) absent key is false\n");
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str  haystack = StrInitFromZstr("Hello World", &alloc);
    bool result   = (StrContains(&haystack, "missing") == false);

    StrDeinit(&haystack);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// str_split_to_iters_impl 262:cxx_le_to_lt -- `while (prev <= end)` ->
// `while (prev < end)`. A trailing delimiter leaves `prev == end`; the intact
// loop runs once more and emits a final empty field. For "a,b," split on ","
// the intact result has 3 iters (last length 0); the mutant drops it (2).
static bool test_split_to_iters_trailing_empty_field(void) {
    WriteFmt("Testing StrSplitToIters trailing empty field\n");
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str      s     = StrInitFromZstr("a,b,", &alloc);
    StrIters iters = StrSplitToIters(&s, ",");

    bool result = (VecLen(&iters) == 3);
    if (VecLen(&iters) == 3) {
        result = result && (StrIterLength(VecPtrAt(&iters, 0)) == 1);
        result = result && (StrIterLength(VecPtrAt(&iters, 1)) == 1);
        result = result && (StrIterLength(VecPtrAt(&iters, 2)) == 0);
    }

    VecDeinit(&iters);
    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// ---- str_split_impl / str_cmp_zstr / index_of / starts_with (Str.Mutants7) -

// str_split_impl @ 305:21 (ZstrCompareN result replaced with the constant 42).
// 42 is truthy, so the final-segment guard always pushes. Splitting "ab" by
// "abc": the remaining text "ab" is a proper NUL-bounded prefix of the key, so
// ZstrCompareN returns 0 and the segment is dropped -> 0 elements. Forcing the
// result to 42 pushes it -> 1 element.
static bool test_split_drops_prefix_of_key(void) {
    WriteFmt("Testing StrSplit drops a remaining proper-prefix-of-key segment\n");
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str  s     = StrInitFromZstr("ab", &alloc);
    Strs split = StrSplit(&s, "abc");

    bool result = (VecLen(&split) == 0);
    if (!result) {
        WriteFmt("    FAIL: Expected 0 split elements, got {}\n", (u64)VecLen(&split));
    }

    VecDeinit(&split);
    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// str_split_impl @ 305:49 (`end - prev` -> `end + prev`). The third argument to
// ZstrCompareN is the remaining length n. Intact n=2 lets the compare match
// "ab" against "ab" and return 0 via i==n -> segment dropped. A huge bogus n
// keeps comparing past prev's NUL where the key has 'c', returning non-zero ->
// the segment is wrongly pushed.
static bool test_split_prefix_compare_length(void) {
    WriteFmt("Testing StrSplit prefix-of-key compare uses remaining length\n");
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str  s     = StrInitFromZstr("ab", &alloc);
    Strs split = StrSplit(&s, "abc");

    bool result = (VecLen(&split) == 0);
    if (!result) {
        WriteFmt("    FAIL: Expected 0 split elements, got {}\n", (u64)VecLen(&split));
    }

    VecDeinit(&split);
    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// str_cmp_zstr @ 215:12 (ZstrCompare result replaced with the constant 42).
// StrCmp(&hello, "Hello") must compare equal (0); a constant replacement makes
// it 42.
static bool test_cmp_zstr_equal(void) {
    WriteFmt("Testing StrCmp (Zstr form) reports equality\n");
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str hello = StrInitFromZstr("Hello", &alloc);

    bool result = (StrCmp(&hello, "Hello") == 0);
    if (!result) {
        WriteFmt("    FAIL: Expected StrCmp(==0) for equal Zstr\n");
    }

    StrDeinit(&hello);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// str_index_of_cstr @ 341:31 (s->length < key_len -> s->length <= key_len).
// For s="abc", key="abc" (len 3) the lengths are equal: the original proceeds
// to search and matches the whole string at index 0; the mutant short-circuits
// to SIZE_MAX.
static bool test_index_of_whole_string(void) {
    WriteFmt("Testing StrIndexOf matches a key equal to the whole string\n");
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str s = StrInitFromZstr("abc", &alloc);

    bool result = (StrIndexOf(&s, "abc", 3) == 0);
    if (!result) {
        WriteFmt("    FAIL: Expected index 0 for whole-string key\n");
    }

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// starts_with @ 437:21 (data_len >= prefix_len -> data_len > prefix_len).
// A prefix equal to the whole string (lengths equal) must still match; the
// mutant rejects it outright.
static bool test_starts_with_full_string(void) {
    WriteFmt("Testing StrStartsWith with a prefix equal to the whole string\n");
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str s = StrInitFromZstr("Hello", &alloc);

    bool result = (StrStartsWith(&s, "Hello") == true);
    if (!result) {
        WriteFmt("    FAIL: Expected StrStartsWith true for full-string prefix\n");
    }

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// ---- str_compare / starts_with / ends_with (Str.Mutants8) -----------------

// str_compare 192:21 cxx_lt_to_ge -- `min = StrLen(a) < StrLen(b) ? lenA :
// lenB` flips to `>=`, turning the shorter-length pick into the longer one.
// With a strict prefix the correct answer is "a < b" (returns -1); the mutant
// reads bytes past `a`'s logical length and compares the wrong region.
//
// `a` is "ab" (length 2) but its buffer is reserved to 8 and its bytes at
// index 2 and 3 are planted with 0xFF -- larger than `b`'s 'Y' bytes. So:
//   real   : min = 2, "ab" == "ab" prefix, lenA(2) < lenB(4) -> returns -1.
//   mutant : min = 4, compares {a,b,0xFF,0xFF} vs {a,b,Y,Y}; index 2 is
//            0xFF(255) > 'Y'(0x59) -> a > b -> returns a positive value.
// Asserting an exactly-negative result kills the mutant by value.
static bool test_compare_min_picks_shorter(void) {
    WriteFmt("Testing str_compare uses the shorter length for the prefix (192:21)\n");
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str a = StrInitFromCstr("ab", 2, &alloc);
    StrReserve(&a, 8);
    // Plant bytes beyond `a`'s logical length (still inside capacity 8) that
    // exceed the corresponding bytes of `b`. Only the mutant ever reads them.
    StrBegin(&a)[2] = (char)0xFF;
    StrBegin(&a)[3] = (char)0xFF;

    Str b = StrInitFromZstr("abYY", &alloc); // length 4

    // Call the mutated function directly: the public StrCmp wrapper
    // (str_cmp_str) does its own ValidateStr + min selection lives only in
    // str_compare, so target it without the wrapper in the way.
    i32  cmp    = str_compare(&a, &b);
    bool result = (cmp == -1);
    if (!result) {
        WriteFmt("    FAIL: expected -1 (a is a strict prefix of b), got {}\n", cmp);
    }

    StrDeinit(&a);
    StrDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// str_starts_with_str 466:12 cxx_replace_scalar_call -- the `starts_with(...)`
// result is replaced by a surviving constant (false), so a genuine prefix
// match wrongly reports false.
//   real   : "Hello World" starts with "Hello" -> true.
//   mutant : always false.
static bool test_starts_with_true_on_match(void) {
    WriteFmt("Testing StrStartsWith returns true on a real prefix match (466:12)\n");
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str s      = StrInitFromZstr("Hello World", &alloc);
    Str prefix = StrInitFromZstr("Hello", &alloc);

    bool result = (StrStartsWith(&s, &prefix) == true);
    if (!result) {
        WriteFmt("    FAIL: expected true for prefix 'Hello'\n");
    }

    StrDeinit(&s);
    StrDeinit(&prefix);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// ends_with 441:21 cxx_ge_to_gt -- `data_len >= suffix_len` becomes
// `data_len > suffix_len`. When the suffix equals the whole string the
// lengths are equal, so the mutant rejects a valid match.
//   real   : "World" ends with "World" -> true.
//   mutant : equal lengths fail the `>` test -> false.
static bool test_ends_with_full_length_suffix(void) {
    WriteFmt("Testing StrEndsWith matches a whole-string suffix (441:21)\n");
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str s = StrInitFromZstr("World", &alloc);

    bool result = (StrEndsWith(&s, "World") == true);
    if (!result) {
        WriteFmt("    FAIL: expected true for suffix equal to the whole string\n");
    }

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// ===========================================================================
// Deadend tests (relocated) -- each removes a sole ValidateStr barrier; a
// corrupted or NULL Str must trip LOG_FATAL on real code.
// ===========================================================================

// str_replace_zstr 492:5 remove_void_call -- removing ValidateStr(s) lets a
// NULL handle reach the inner replace path (Str.Mutants3).
static bool test_replace_zstr_null_aborts(void) {
    WriteFmt("Testing str_replace_zstr NULL aborts\n");
    Str *null_str = NULL;
    StrReplace(null_str, "a", "b", 1);
    return false;
}

// str_find_zstr 245:5 cxx_remove_void_call : drops ValidateStr(s) (Str.Mutants4).
static bool test_find_zstr_validates(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              s     = StrInitFromZstr("hello", &alloc);
    s.__magic              = 0; // corrupt: validate_vec must LOG_FATAL.
    (void)str_find_zstr(&s, "x");
    // Unreached on real code; on the mutant this returns and the test
    // function returns without aborting.
    return true;
}

// str_starts_with_zstr 445:5 cxx_remove_void_call : drops ValidateStr(s)
// (Str.Mutants4).
static bool test_starts_with_zstr_validates(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              s     = StrInitFromZstr("hello", &alloc);
    s.__magic              = 0; // corrupt: validate_vec must LOG_FATAL.
    (void)str_starts_with_zstr(&s, "he");
    return true;
}

// 250:5 cxx_remove_void_call -- removes ValidateStr(s) in str_find_str. A
// corrupted s must abort before StrBegin(s) is read (Str.Mutants5).
static bool test_str_find_str_corrupt_s_aborts(void) {
    WriteFmt("Testing StrFind validates s (should abort)\n");
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str s   = StrInitFromZstr("haystack", &alloc);
    Str key = StrInitFromZstr("stack", &alloc);

    // intentional bypass: corrupt s only.
    s.length   = 100;
    s.capacity = 5;
    MAGIC_MARK_DIRTY(&s);

    (void)StrFind(&s, &key); // should abort here

    return false;            // unreachable on real code
}

// 251:5 cxx_remove_void_call -- removes ValidateStr(key) in str_find_str. A
// corrupted key must abort (s is valid and passes its own check first)
// (Str.Mutants5).
static bool test_str_find_str_corrupt_key_aborts(void) {
    WriteFmt("Testing StrFind validates key (should abort)\n");
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str s   = StrInitFromZstr("haystack", &alloc);
    Str key = StrInitFromZstr("stack", &alloc);

    // intentional bypass: corrupt key only.
    key.length   = 100;
    key.capacity = 5;
    MAGIC_MARK_DIRTY(&key);

    (void)StrFind(&s, &key); // should abort here

    return false;            // unreachable on real code
}

// 470:5 cxx_remove_void_call -- removes ValidateStr(s) in str_ends_with_str. A
// corrupted s must abort before s->data/s->length are read (Str.Mutants5).
static bool test_str_ends_with_str_corrupt_s_aborts(void) {
    WriteFmt("Testing StrEndsWith validates s (should abort)\n");
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str s      = StrInitFromZstr("Hello World", &alloc);
    Str suffix = StrInitFromZstr("World", &alloc);

    // intentional bypass: corrupt s only.
    s.length   = 100;
    s.capacity = 5;
    MAGIC_MARK_DIRTY(&s);

    (void)StrEndsWith(&s, &suffix); // should abort here

    return false;                   // unreachable on real code
}

// 219:5 cxx_remove_void_call -- removes ValidateStr(s) in str_cmp_cstr. A
// corrupted s must abort before StrBegin(s) is read (Str.Mutants5).
static bool test_str_cmp_cstr_corrupt_s_aborts(void) {
    WriteFmt("Testing StrCmp(cstr) validates s (should abort)\n");
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str s = StrInitFromZstr("Hello", &alloc);

    // intentional bypass: corrupt s only.
    s.length   = 100;
    s.capacity = 5;
    MAGIC_MARK_DIRTY(&s);

    (void)StrCmp(&s, "x", 1); // 3-arg form -> str_cmp_cstr; should abort here

    return false;             // unreachable on real code
}

// 362:5 cxx_remove_void_call -- removes ValidateStr(key) in str_index_of_str. A
// corrupted key must abort (s is valid) (Str.Mutants5).
static bool test_str_index_of_str_corrupt_key_aborts(void) {
    WriteFmt("Testing StrIndexOf validates key (should abort)\n");
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str s   = StrInitFromZstr("haystack", &alloc);
    Str key = StrInitFromZstr("stack", &alloc);

    // intentional bypass: corrupt key only.
    key.length   = 100;
    key.capacity = 5;
    MAGIC_MARK_DIRTY(&key);

    (void)StrIndexOf(&s, &key); // Str* -> str_index_of_str; should abort here

    return false;               // unreachable on real code
}

// 450:5 cxx_remove_void_call -- removes ValidateStr(s) in str_ends_with_zstr. A
// NULL s must abort cleanly via validate_vec's NULL check (Str.Mutants5).
static bool test_str_ends_with_zstr_null_aborts(void) {
    WriteFmt("Testing StrEndsWith(zstr) validates a NULL s (should abort)\n");

    (void)StrEndsWith((const Str *)NULL, "x"); // Zstr suffix -> str_ends_with_zstr

    return false;                              // unreachable on real code
}

// strip_str 407:cxx_remove_void_call -- removes ValidateStr(s). A Str with a
// corrupted magic word must abort at the validation barrier (Str.Mutants6).
static bool test_strip_str_corrupt_magic_aborts(void) {
    WriteFmt("Testing StrStrip aborts on corrupt magic\n");
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str s                     = StrInitFromZstr("  x  ", &alloc);
    GENERIC_VEC(&s)->__magic ^= 0x1;
    Str stripped              = StrStrip(&s, NULL);

    StrDeinit(&stripped);
    GENERIC_VEC(&s)->__magic ^= 0x1;
    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// str_contains_str 384:cxx_remove_void_call -- removes ValidateStr(key); a
// corrupt key magic must abort (Str.Mutants6).
static bool test_contains_str_corrupt_key_magic_aborts(void) {
    WriteFmt("Testing StrContains aborts on corrupt key magic\n");
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str haystack                = StrInitFromZstr("Hello World", &alloc);
    Str key                     = StrInitFromZstr("World", &alloc);
    GENERIC_VEC(&key)->__magic ^= 0x1;
    (void)StrContains(&haystack, &key);

    GENERIC_VEC(&key)->__magic ^= 0x1;
    StrDeinit(&key);
    StrDeinit(&haystack);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// str_split_to_iters_impl 256:cxx_remove_void_call -- removes ValidateStr(s);
// a corrupt-magic Str must abort before its data is read (Str.Mutants6).
static bool test_split_to_iters_corrupt_magic_aborts(void) {
    WriteFmt("Testing StrSplitToIters aborts on corrupt magic\n");
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str s                     = StrInitFromZstr("a,b,c", &alloc);
    GENERIC_VEC(&s)->__magic ^= 0x1;
    StrIters iters            = StrSplitToIters(&s, ",");

    VecDeinit(&iters);
    GENERIC_VEC(&s)->__magic ^= 0x1;
    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// str_hash 173:cxx_remove_void_call -- removes ValidateStr(str). A zeroed /
// uninitialized Str (no VEC_MAGIC) must abort at the validation barrier rather
// than dereferencing a bogus header (Str.Mutants6).
static bool test_str_hash_uninitialized_aborts(void) {
    WriteFmt("Testing str_hash aborts on uninitialized Str\n");

    Str bogus = {0};
    (void)str_hash(&bogus, (u32)sizeof(Str));

    return false;
}

// str_cmp_zstr_ignore_case 230:cxx_remove_void_call -- removes ValidateStr(s);
// a corrupt-magic Str must abort (Str.Mutants6).
static bool test_cmp_zstr_ignore_case_corrupt_magic_aborts(void) {
    WriteFmt("Testing StrCmpIgnoreCase aborts on corrupt magic\n");
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str s                     = StrInitFromZstr("hello", &alloc);
    GENERIC_VEC(&s)->__magic ^= 0x1;
    (void)StrCmpIgnoreCase(&s, "HELLO");

    GENERIC_VEC(&s)->__magic ^= 0x1;
    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// str_starts_with_cstr 455:cxx_remove_void_call -- removes ValidateStr(s). A
// NULL Str must abort at the validation barrier instead of dereferencing
// (Str.Mutants6).
static bool test_starts_with_cstr_null_aborts(void) {
    WriteFmt("Testing StrStartsWith(Cstr) aborts on NULL Str\n");

    (void)StrStartsWith((Str *)NULL, "x", (size)1);

    return false;
}

// ValidateStrs 1155:cxx_remove_void_call -- removes the per-element
// ValidateStr(sp). A Strs whose container is valid but one contained Str has a
// corrupted magic word must abort on that element (Str.Mutants6).
static bool test_validate_strs_corrupt_element_aborts(void) {
    WriteFmt("Testing ValidateStrs aborts on corrupt element\n");
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str  s                                    = StrInitFromZstr("a,b,c", &alloc);
    Strs strs                                 = StrSplit(&s, ",");
    GENERIC_VEC(VecPtrAt(&strs, 0))->__magic ^= 0x1;
    ValidateStrs(&strs);

    GENERIC_VEC(VecPtrAt(&strs, 0))->__magic ^= 0x1;
    VecDeinit(&strs);
    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// str_split_impl @ 290:5 (remove ValidateStr): corrupted-magic Str must abort
// (Str.Mutants7).
static bool test_split_validates(void) {
    WriteFmt("Testing StrSplit validates its Str (deadend)\n");
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str s      = StrInitFromZstr("a,b", &alloc);
    s.__magic  = 0; // corrupt the magic so ValidateStr aborts
    Strs split = StrSplit(&s, ",");
    (void)split;

    return false;
}

// str_cmp_zstr @ 214:5 (remove ValidateStr): corrupted-magic Str must abort
// (Str.Mutants7).
static bool test_cmp_zstr_validates(void) {
    WriteFmt("Testing StrCmp (Zstr form) validates its Str (deadend)\n");
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str s     = StrInitFromZstr("Hello", &alloc);
    s.__magic = 0;
    int cmp   = StrCmp(&s, "x");
    (void)cmp;

    return false;
}

// str_index_of_cstr @ 331:5 (remove ValidateStr): corrupted-magic Str must
// abort (Str.Mutants7).
static bool test_index_of_validates(void) {
    WriteFmt("Testing StrIndexOf validates its Str (deadend)\n");
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str s     = StrInitFromZstr("abc", &alloc);
    s.__magic = 0;
    size idx  = StrIndexOf(&s, "x", 1);
    (void)idx;

    return false;
}

// str_cmp_cstr_ignore_case @ 235:5 (remove ValidateStr): corrupted-magic Str
// must abort (Str.Mutants7).
static bool test_cmp_cstr_ignore_case_validates(void) {
    WriteFmt("Testing StrCmpIgnoreCase (Cstr form) validates its Str (deadend)\n");
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str s     = StrInitFromZstr("Hello", &alloc);
    s.__magic = 0;
    int cmp   = StrCmpIgnoreCase(&s, "x", 1);
    (void)cmp;

    return false;
}

// str_ends_with_cstr @ 460:5 (remove ValidateStr): a NULL Str must abort via
// the validator's NULL check rather than dereferencing s->data (Str.Mutants7).
static bool test_ends_with_cstr_validates(void) {
    WriteFmt("Testing StrEndsWith (Cstr form) validates its Str (deadend)\n");

    bool ends = StrEndsWith((const Str *)NULL, "x", 1);
    (void)ends;

    return false;
}

// str_compare 189:5 cxx_remove_void_call -- removes ValidateStr(a). Corrupt
// `a`'s magic, keep `b` valid. Calls str_compare directly: the public StrCmp
// wrapper runs its own ValidateStr first and would mask this barrier
// (Str.Mutants8).
static bool test_compare_validates_a(void) {
    WriteFmt("Testing str_compare validates its first operand (189:5)\n");
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str a     = StrInitFromZstr("alpha", &alloc);
    Str b     = StrInitFromZstr("beta", &alloc);
    a.__magic = 0;             // corrupt: magic mismatch, data/length still valid

    (void)str_compare(&a, &b); // expected to abort on real code via ValidateStr(a)

    StrDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// str_compare 190:5 cxx_remove_void_call -- removes ValidateStr(b). Corrupt
// `b`'s magic, keep `a` valid (Str.Mutants8).
static bool test_compare_validates_b(void) {
    WriteFmt("Testing str_compare validates its second operand (190:5)\n");
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str a     = StrInitFromZstr("alpha", &alloc);
    Str b     = StrInitFromZstr("beta", &alloc);
    b.__magic = 0;

    (void)str_compare(&a, &b);

    StrDeinit(&a);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// str_cmp_str_ignore_case 224:5 cxx_remove_void_call -- removes
// ValidateStr(s). Corrupt `s`, keep `other` valid (Str.Mutants8).
static bool test_cmp_ignore_case_validates_s(void) {
    WriteFmt("Testing str_cmp_str_ignore_case validates s (224:5)\n");
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str s     = StrInitFromZstr("HELLO", &alloc);
    Str other = StrInitFromZstr("hello", &alloc);
    s.__magic = 0;

    (void)StrCmpIgnoreCase(&s, &other);

    StrDeinit(&other);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// str_cmp_str_ignore_case 225:5 cxx_remove_void_call -- removes
// ValidateStr(other). Corrupt `other`, keep `s` valid (Str.Mutants8).
static bool test_cmp_ignore_case_validates_other(void) {
    WriteFmt("Testing str_cmp_str_ignore_case validates other (225:5)\n");
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str s         = StrInitFromZstr("HELLO", &alloc);
    Str other     = StrInitFromZstr("hello", &alloc);
    other.__magic = 0;

    (void)StrCmpIgnoreCase(&s, &other);

    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// str_starts_with_str 465:5 cxx_remove_void_call -- removes ValidateStr(s).
// Corrupt `s`, keep `prefix` valid (Str.Mutants8).
static bool test_starts_with_validates_s(void) {
    WriteFmt("Testing str_starts_with_str validates s (465:5)\n");
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str s      = StrInitFromZstr("Hello World", &alloc);
    Str prefix = StrInitFromZstr("Hello", &alloc);
    s.__magic  = 0;

    (void)StrStartsWith(&s, &prefix);

    StrDeinit(&prefix);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// str_find_cstr 240:5 cxx_remove_void_call -- removes ValidateStr(s). Corrupt
// `s`, then search via the 3-arg StrFind form (-> str_find_cstr) (Str.Mutants8).
static bool test_find_cstr_validates_s(void) {
    WriteFmt("Testing str_find_cstr validates s (240:5)\n");
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str s     = StrInitFromZstr("haystack", &alloc);
    s.__magic = 0;

    (void)StrFind(&s, "x", (size)1);

    DefaultAllocatorDeinit(&alloc);
    return false;
}

// str_replace_cstr 475:5 cxx_remove_void_call -- removes ValidateStr(s). A
// NULL Str must trip the validator before the in-place mutation loop
// (Str.Mutants8).
static bool test_replace_cstr_validates_s(void) {
    WriteFmt("Testing str_replace_cstr validates s (475:5)\n");

    // 6-arg form maps directly to str_replace_cstr(NULL, ...).
    StrReplace((Str *)NULL, "a", (size)1, "b", (size)1, (size)1);

    return false;
}

// Main function that runs all tests
int main(void) {
    WriteFmt("[INFO] Starting Str.Ops tests\n\n");

    // Array of test functions
    TestFunction tests[] = {
        test_str_cmp,
        test_str_cmp_ignore_case,
        test_str_find,
        test_str_contains_index,
        test_str_starts_ends_with,
        test_str_replace,
        test_str_split,
        test_str_strip,
        // str_ends_with_str (Str.Mutants5)
        test_str_ends_with_str_true,
        // strip_str / contains / split_to_iters (Str.Mutants6)
        test_lstrip_all_strip_chars_empty,
        test_rstrip_all_strip_chars_empty,
        test_strip_single_surviving_char_kept,
        test_contains_str_absent_key_false,
        test_contains_str_present_and_absent,
        test_contains_cstr_absent_key_false,
        test_split_to_iters_trailing_empty_field,
        // split / cmp_zstr / index_of / starts_with (Str.Mutants7)
        test_split_drops_prefix_of_key,
        test_split_prefix_compare_length,
        test_cmp_zstr_equal,
        test_index_of_whole_string,
        test_starts_with_full_string,
        // str_compare / starts_with / ends_with (Str.Mutants8)
        test_compare_min_picks_shorter,
        test_starts_with_true_on_match,
        test_ends_with_full_length_suffix
    };

    // Array of deadend test functions (relocated from Str.Mutants*)
    TestFunction deadend_tests[] = {
        test_replace_zstr_null_aborts,                  // Str.Mutants3
        test_find_zstr_validates,                       // Str.Mutants4
        test_starts_with_zstr_validates,                // Str.Mutants4
        test_str_find_str_corrupt_s_aborts,             // Str.Mutants5
        test_str_find_str_corrupt_key_aborts,           // Str.Mutants5
        test_str_ends_with_str_corrupt_s_aborts,        // Str.Mutants5
        test_str_cmp_cstr_corrupt_s_aborts,             // Str.Mutants5
        test_str_index_of_str_corrupt_key_aborts,       // Str.Mutants5
        test_str_ends_with_zstr_null_aborts,            // Str.Mutants5
        test_strip_str_corrupt_magic_aborts,            // Str.Mutants6
        test_contains_str_corrupt_key_magic_aborts,     // Str.Mutants6
        test_split_to_iters_corrupt_magic_aborts,       // Str.Mutants6
        test_str_hash_uninitialized_aborts,             // Str.Mutants6
        test_cmp_zstr_ignore_case_corrupt_magic_aborts, // Str.Mutants6
        test_starts_with_cstr_null_aborts,              // Str.Mutants6
        test_validate_strs_corrupt_element_aborts,      // Str.Mutants6
        test_split_validates,                           // Str.Mutants7
        test_cmp_zstr_validates,                        // Str.Mutants7
        test_index_of_validates,                        // Str.Mutants7
        test_cmp_cstr_ignore_case_validates,            // Str.Mutants7
        test_ends_with_cstr_validates,                  // Str.Mutants7
        test_compare_validates_a,                       // Str.Mutants8
        test_compare_validates_b,                       // Str.Mutants8
        test_cmp_ignore_case_validates_s,               // Str.Mutants8
        test_cmp_ignore_case_validates_other,           // Str.Mutants8
        test_starts_with_validates_s,                   // Str.Mutants8
        test_find_cstr_validates_s,                     // Str.Mutants8
        test_replace_cstr_validates_s                   // Str.Mutants8
    };

    int total_tests         = sizeof(tests) / sizeof(tests[0]);
    int total_deadend_tests = sizeof(deadend_tests) / sizeof(deadend_tests[0]);

    // Run all tests using the centralized test driver
    return run_test_suite(tests, total_tests, deadend_tests, total_deadend_tests, "Str.Ops");
}
