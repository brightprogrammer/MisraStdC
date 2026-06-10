#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Container/Map.h>
#include <Misra/Std/Log.h>
#include "../Util/TestRunner.h"

static u64 i32_hash(const void *data, u32 size) {
    u64 x = (u64)(u32)(*(const int *)data);
    (void)size;
    x ^= x >> 33;
    x *= 0xff51afd7ed558ccdULL;
    x ^= x >> 33;
    return x;
}

static i32 i32_compare(const void *lhs, const void *rhs) {
    int a = *(const int *)lhs;
    int b = *(const int *)rhs;
    return (a > b) - (a < b);
}

static bool test_map_insert_and_set(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInit(i32_hash, i32_compare, &alloc);

    MapInsertR(&map, 1, 10);
    MapInsertR(&map, 1, 11);
    MapInsertR(&map, 2, 20);
    MapSetOnlyR(&map, 2, 200);
    MapSetOnlyR(&map, 3, 30);

    bool result = MapPairCount(&map) == 4;
    result      = result && (MapValueCountForKey(&map, 1) == 2);
    result      = result && (MapValueCountForKey(&map, 2) == 1);
    result      = result && (MapValueCountForKey(&map, 3) == 1);
    result      = result && MapGetFirstPtr(&map, 1) && (*MapGetFirstPtr(&map, 1) == 10);
    result      = result && MapGetFirstPtr(&map, 2) && (*MapGetFirstPtr(&map, 2) == 200);
    result      = result && MapGetFirstPtr(&map, 3) && (*MapGetFirstPtr(&map, 3) == 30);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_map_set_first(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInitWithValueCompare(i32_hash, i32_compare, i32_compare, &alloc);

    MapInsertR(&map, 1, 10);
    MapInsertR(&map, 1, 11);
    MapInsertR(&map, 1, 12);
    MapSetFirstR(&map, 1, 100);

    bool result = (MapPairCount(&map) == 3);
    result      = result && (MapValueCountForKey(&map, 1) == 3);
    result      = result && MapGetFirstPtr(&map, 1) && (*MapGetFirstPtr(&map, 1) == 100);
    result      = result && MapContainsPair(&map, 1, 11);
    result      = result && MapContainsPair(&map, 1, 12);
    result      = result && !MapContainsPair(&map, 1, 10);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// MapSetFirst is UPDATE-ONLY: on a key miss it must leave the map unchanged
// and return false (it does NOT insert). Covers both miss branches of
// map_set_first -- the empty-table `!capacity` early-out and the
// `idx >= capacity` key-absent early-out. An implementation that inserted on
// miss (or the header claiming it does) turns this RED.
static bool test_map_set_first_miss_returns_false(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInit(i32_hash, i32_compare, &alloc);

    // Miss on an empty (capacity 0) map: returns false, nothing inserted.
    bool result = !MapSetFirstR(&map, 7, 70);
    result      = result && (MapPairCount(&map) == 0) && !MapContainsKey(&map, 7);

    MapInsertR(&map, 1, 10);

    // Miss on a populated map (key absent): returns false, map unchanged.
    result = result && !MapSetFirstR(&map, 7, 70);
    result = result && !MapContainsKey(&map, 7);
    result = result && (MapValueCountForKey(&map, 7) == 0);
    result = result && (MapPairCount(&map) == 1);
    // Pre-existing entry is untouched.
    result = result && MapGetFirstPtr(&map, 1) && (*MapGetFirstPtr(&map, 1) == 10);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// MapSetOnly must collapse a multi-valued key down to exactly one
// mapping carrying the new value (the "Replace" contract in the header).
// Guards map_set_only's remove-all-then-insert path: dropping the
// map_remove_all step would leave the old duplicates behind.
static bool test_map_set_only_collapses_multi(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInitWithValueCompare(i32_hash, i32_compare, i32_compare, &alloc);

    MapInsertR(&map, 5, 50);
    MapInsertR(&map, 5, 51);
    MapInsertR(&map, 5, 52);
    bool result = (MapValueCountForKey(&map, 5) == 3);

    MapSetOnlyR(&map, 5, 500);

    result = result && (MapValueCountForKey(&map, 5) == 1);
    result = result && MapGetFirstPtr(&map, 5) && (*MapGetFirstPtr(&map, 5) == 500);
    result = result && MapContainsPair(&map, 5, 500);
    result = result && !MapContainsPair(&map, 5, 50);
    result = result && !MapContainsPair(&map, 5, 51);
    result = result && !MapContainsPair(&map, 5, 52);
    result = result && (MapPairCount(&map) == 1);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_map_lvalue_zeroing(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInit(i32_hash, i32_compare, &alloc);
    int              key   = 42;
    int              value = 84;

    MapInsertL(&map, key, value);

    bool result = (key == 0) && (value == 0);
    result      = result && (MapValueCountForKey(&map, 42) == 1);
    result      = result && MapGetFirstPtr(&map, 42) && (*MapGetFirstPtr(&map, 42) == 84);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// MapInsertR is the r-value form: it must NOT zero the caller's sources,
// even when no copy_init is configured (the L-form zeroes, the R-form
// does not). Guards that MapInsertR routes through map_insert_r_impl and
// never through map_zero_insert_sources_on_success.
static bool test_map_rvalue_does_not_zero_sources(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInit(i32_hash, i32_compare, &alloc);
    int              key   = 42;
    int              value = 84;

    MapInsertR(&map, key, value);

    bool result = (key == 42) && (value == 84);
    result      = result && (MapValueCountForKey(&map, 42) == 1);
    result      = result && MapGetFirstPtr(&map, 42) && (*MapGetFirstPtr(&map, 42) == 84);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// MapSetOnlyL zeroes BOTH key and value sources on success when neither
// copy_init handler is configured. Mirrors test_map_lvalue_zeroing for
// the set-only path; guards map_zero_insert_sources_on_success wiring on
// the set_only_l form.
static bool test_map_set_only_lvalue_zeroing(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInit(i32_hash, i32_compare, &alloc);
    int              key   = 11;
    int              value = 110;

    MapSetOnlyL(&map, key, value);

    bool result = (key == 0) && (value == 0);
    result      = result && (MapValueCountForKey(&map, 11) == 1);
    result      = result && MapGetFirstPtr(&map, 11) && (*MapGetFirstPtr(&map, 11) == 110);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// MapSetFirstL zeroes ONLY the value source on a successful UPDATE (the key
// is always an r-value lookup and is never zeroed). On a key miss it is
// update-only: returns false, inserts nothing, and leaves the value source
// untouched. Guards map_zero_value_source_on_success wiring on the
// set_first_l form (zero only on success).
static bool test_map_set_first_lvalue_zeroing(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInit(i32_hash, i32_compare, &alloc);
    int              value = 110;

    MapInsertR(&map, 11, 10);

    // Update-existing path: first value replaced, value source zeroed.
    bool result = MapSetFirstL(&map, 11, value);
    result      = result && (value == 0);
    result      = result && (MapValueCountForKey(&map, 11) == 1);
    result      = result && MapGetFirstPtr(&map, 11) && (*MapGetFirstPtr(&map, 11) == 110);

    // Miss path: returns false, value source NOT zeroed, nothing inserted.
    int miss = 220;
    result   = result && !MapSetFirstL(&map, 99, miss);
    result   = result && (miss == 220);
    result   = result && !MapContainsKey(&map, 99);
    result   = result && (MapPairCount(&map) == 1);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Heavy insert+remove churn at near-threshold load. With linear probing
// limited to `max_probe_count` slots, a long collision cluster can force
// scan_slots to fail even though empty slots exist; the prior fix was to
// force `next_capacity` to grow on that path (rehashing at the same size
// would re-probe the same cluster and loop forever).
static bool test_map_churn_does_not_loop(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc  = DefaultAllocatorInit();
    IntIntMap        map    = MapInit(i32_hash, i32_compare, &alloc);
    bool             result = true;

    // Phase 1: fill past the first few growth thresholds.
    for (int i = 0; i < 600; ++i) {
        MapInsertR(&map, i, i * 10);
    }
    result = result && (MapPairCount(&map) == 600);

    // Phase 2: alternating remove+insert at the same churn point. Each
    // cycle exercises rehash + scan with tombstones present, which is
    // what previously triggered the runaway recursion.
    for (int i = 0; i < 4000; ++i) {
        int key = 600 + (i & 0x3f); // small cycling window
        MapRemoveAll(&map, key);
        MapInsertR(&map, key, i);
    }
    // Deterministic count: 600 base keys (0..599) survive untouched, plus
    // the 64 churned keys (600..663) each ending with exactly one value.
    result = result && (MapPairCount(&map) == 664);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_map_ensure_ptr(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInit(i32_hash, i32_compare, &alloc);
    int             *value_ptr;
    bool             result;

    value_ptr = MapEnsurePtr(&map, 8, 80);
    result    = value_ptr && (*value_ptr == 80);
    result    = result && (MapPairCount(&map) == 1);
    result    = result && (MapValueCountForKey(&map, 8) == 1);

    value_ptr = MapEnsurePtr(&map, 8, 800);
    result    = result && value_ptr && (*value_ptr == 80);
    result    = result && (MapPairCount(&map) == 1);
    result    = result && (MapValueCountForKey(&map, 8) == 1);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// The pointer MapEnsurePtr returns must alias the live value slot: a
// write through it has to be observable on a subsequent re-fetch. Guards
// that map_ensure_value_ptr returns the in-table slot, not a copy.
static bool test_map_ensure_ptr_mutation_persists(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInit(i32_hash, i32_compare, &alloc);

    int *value_ptr = MapEnsurePtr(&map, 3, 30);
    bool result    = value_ptr && (*value_ptr == 30);

    *value_ptr = 333; // mutate through the returned pointer

    int *refetched = MapGetFirstPtr(&map, 3);
    result         = result && refetched && (*refetched == 333);
    // Mutation must not have spawned a second entry.
    result = result && (MapValueCountForKey(&map, 3) == 1);
    result = result && (MapPairCount(&map) == 1);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// MapInsert / MapSet default-alias their L-forms: MapInsert == MapInsertL
// (zeroes both sources) and MapSet == MapSetOnlyL (zeroes both sources,
// replace semantics). Guards the aliasing #defines in Insert.h.
static bool test_map_default_aliases(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInit(i32_hash, i32_compare, &alloc);
    int              k1    = 1;
    int              v1    = 10;
    int              v2    = 11;

    // MapInsert aliases MapInsertL: both sources zeroed.
    MapInsert(&map, k1, v1);
    bool result = (k1 == 0) && (v1 == 0);
    result      = result && (MapValueCountForKey(&map, 1) == 1);

    // MapSet aliases MapSetOnlyL: replace + zero both sources.
    MapInsertR(&map, 1, 99); // give key 1 a second value
    result = result && (MapValueCountForKey(&map, 1) == 2);
    int k2 = 1;
    MapSet(&map, k2, v2);
    result = result && (k2 == 0) && (v2 == 0);
    result = result && (MapValueCountForKey(&map, 1) == 1);
    result = result && MapGetFirstPtr(&map, 1) && (*MapGetFirstPtr(&map, 1) == 11);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Success-path coverage for the aborting Must* insert family. We never
// exercise the abort branch (that is the Deadend agent's job); we only
// assert each MapMust* applies the same effect as its fallible form.
static bool test_map_must_family_success(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInitWithValueCompare(i32_hash, i32_compare, i32_compare, &alloc);
    int              k     = 1;
    int              v     = 10;

    MapMustInsertL(&map, k, v); // L-form zeroes sources
    bool result = (k == 0) && (v == 0);
    result      = result && (MapValueCountForKey(&map, 1) == 1);

    MapMustInsertR(&map, 2, 20);
    result = result && (MapValueCountForKey(&map, 2) == 1);

    int ik = 3;
    int iv = 30;
    MapMustInsert(&map, ik, iv); // aliases L-form: zeroes both sources
    result = result && (ik == 0) && (iv == 0);
    result = result && (MapValueCountForKey(&map, 3) == 1);

    // SetFirstR updates the existing first value of key 2 (insert was 20).
    MapMustSetFirstR(&map, 2, 222);
    result = result && MapGetFirstPtr(&map, 2) && (*MapGetFirstPtr(&map, 2) == 222);

    int sf = 333;
    MapMustSetFirstL(&map, 3, sf); // value zeroed on success
    result = result && (sf == 0);
    result = result && MapGetFirstPtr(&map, 3) && (*MapGetFirstPtr(&map, 3) == 333);

    MapMustSetOnlyR(&map, 4, 40);
    result = result && (MapValueCountForKey(&map, 4) == 1);
    result = result && MapGetFirstPtr(&map, 4) && (*MapGetFirstPtr(&map, 4) == 40);

    int so = 50;
    int sk = 4;
    MapMustSetOnlyL(&map, sk, so); // replace + zero both sources
    result = result && (sk == 0) && (so == 0);
    result = result && (MapValueCountForKey(&map, 4) == 1);
    result = result && MapGetFirstPtr(&map, 4) && (*MapGetFirstPtr(&map, 4) == 50);

    int msk = 5;
    int msv = 55;
    MapMustSet(&map, msk, msv); // aliases SetOnlyL: zeroes both sources
    result = result && (msk == 0) && (msv == 0);
    result = result && (MapValueCountForKey(&map, 5) == 1);
    result = result && MapGetFirstPtr(&map, 5) && (*MapGetFirstPtr(&map, 5) == 55);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

int main(void) {
    TestFunction tests[] = {
        test_map_insert_and_set,
        test_map_set_first,
        test_map_set_first_miss_returns_false,
        test_map_set_only_collapses_multi,
        test_map_lvalue_zeroing,
        test_map_rvalue_does_not_zero_sources,
        test_map_set_only_lvalue_zeroing,
        test_map_set_first_lvalue_zeroing,
        test_map_ensure_ptr,
        test_map_ensure_ptr_mutation_persists,
        test_map_default_aliases,
        test_map_must_family_success,
        test_map_churn_does_not_loop,
    };

    WriteFmt("[INFO] Starting Map.Insert tests\n\n");
    return run_test_suite(tests, (int)(sizeof(tests) / sizeof(tests[0])), NULL, 0, "Map.Insert");
}
