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

// Degenerate hash: every key lands in the same bucket, forcing the probe
// sequence to walk a collision chain and the key comparator to disambiguate.
// With a non-degenerate hash a lookup can succeed on the cached-hash check
// alone, so this is what actually exercises map_keys_equal.
static u64 const_hash(const void *data, u32 size) {
    (void)data;
    (void)size;
    return 0;
}

static bool test_map_contains_and_find(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInitWithValueCompare(i32_hash, i32_compare, i32_compare, &alloc);

    MapSetOnlyR(&map, 7, 70);
    MapInsertR(&map, 7, 71);
    MapSetOnlyR(&map, 9, 90);

    bool result = MapContainsKey(&map, 7);
    result      = result && MapContainsKey(&map, 9);
    result      = result && !MapContainsKey(&map, 8);
    result      = result && MapContainsPair(&map, 7, 70);
    result      = result && MapContainsPair(&map, 7, 71);
    result      = result && !MapContainsPair(&map, 7, 72);
    result      = result && (MapValueCountForKey(&map, 7) == 2);
    result      = result && (MapValueCountForKey(&map, 9) == 1);
    result      = result && (MapValueCountForKey(&map, 8) == 0);
    result      = result && (MapUniqueKeyCount(&map) == 2);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_map_get_ptr(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInit(i32_hash, i32_compare, &alloc);

    MapSetOnlyR(&map, 11, 110);
    MapInsertR(&map, 11, 111);

    int *value  = MapGetFirstPtr(&map, 11);
    bool result = value && (*value == 110);
    result      = result && (MapGetFirstPtr(&map, 999) == NULL);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Contract: MapGetFirstPtr returns a *live* pointer into the value slot
// (valid until next rehash). Writing through it must mutate the stored
// value, and the first stored value is the duplicate inserted first.
static bool test_map_get_first_ptr_is_live(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInitWithValueCompare(i32_hash, i32_compare, i32_compare, &alloc);

    MapSetOnlyR(&map, 11, 110);
    MapInsertR(&map, 11, 111);

    int *value  = MapGetFirstPtr(&map, 11);
    bool result = value && (*value == 110);

    // Mutate through the live pointer; the change must persist in the map.
    if (value)
        *value = 999;
    int *value_again = MapGetFirstPtr(&map, 11);
    result           = result && value_again && (*value_again == 999);
    result           = result && (value == value_again);

    // The second duplicate must be untouched (still 111).
    result = result && MapContainsPair(&map, 11, 111);
    result = result && !MapContainsPair(&map, 11, 110);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Contract: every query on an EMPTY map (capacity 0) takes its early-out
// branch in Map.c. Guards `if (!map->capacity) return ...;` lines.
static bool test_map_empty_queries(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInitWithValueCompare(i32_hash, i32_compare, i32_compare, &alloc);

    bool result = (MapGetFirstPtr(&map, 7) == NULL);             // map_get_value_ptr early-out
    result      = result && !MapContainsKey(&map, 7);            // map_contains early-out
    result      = result && !MapContainsPair(&map, 7, 0);        // map_contains_pair early-out
    result      = result && (MapValueCountForKey(&map, 7) == 0); // map_value_count early-out
    result      = result && (MapUniqueKeyCount(&map) == 0);
    result      = result && (MapPairCount(&map) == 0);

    // Cursor APIs on an empty map are invalid / NULL.
    MapValueCursor cursor = MapFindFirstForKey(&map, 7);
    result                = result && !MapValueCursorIsValid(cursor);
    result                = result && (MapValuePtrFromCursor(&map, cursor) == NULL);

    // MapGetOrDefault on an empty map returns the default, inserts nothing.
    int got = MapGetOrDefault(&map, 7, 1234);
    result  = result && (got == 1234);
    result  = result && (MapPairCount(&map) == 0);
    result  = result && !MapContainsKey(&map, 7);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_map_get_or_default(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInit(i32_hash, i32_compare, &alloc);

    MapSetOnlyR(&map, 11, 110);
    MapInsertR(&map, 11, 111);

    int  found  = MapGetOrDefault(&map, 11, 999);
    int  miss   = MapGetOrDefault(&map, 999, 555);
    bool result = (found == 110);
    result      = result && (miss == 555);
    result      = result && !MapContainsKey(&map, 999);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_map_value_cursor_query(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc     = DefaultAllocatorInit();
    IntIntMap        map       = MapInit(i32_hash, i32_compare, &alloc);
    MapValueCursor   cursor    = MapValueCursorInvalid();
    int              value_sum = 0;
    int              seen      = 0;

    MapInsertR(&map, 4, 40);
    MapInsertR(&map, 4, 41);
    MapInsertR(&map, 4, 42);
    MapInsertR(&map, 9, 90);

    cursor = MapFindFirstForKey(&map, 4);
    while (MapValueCursorIsValid(cursor)) {
        int *value_ptr = MapValuePtrFromCursor(&map, cursor);
        if (!value_ptr) {
            MapDeinit(&map);
            DefaultAllocatorDeinit(&alloc);
            return false;
        }

        value_sum += *value_ptr;
        seen      += 1;
        cursor     = MapFindNextForKey(&map, 4, cursor);
    }

    bool result = (seen == 3) && (value_sum == (40 + 41 + 42));
    result      = result && !MapValueCursorIsValid(MapFindFirstForKey(&map, 99));
    result      = result && (MapValuePtrFromCursor(&map, MapValueCursorInvalid()) == NULL);

    // Loop terminated because the cursor went invalid past the last value.
    result = result && !MapValueCursorIsValid(cursor);

    // Single-valued key: first is valid, next past the end stays invalid,
    // and advancing an already-invalid cursor remains invalid.
    MapValueCursor single = MapFindFirstForKey(&map, 9);
    result                = result && MapValueCursorIsValid(single);
    MapValueCursor past   = MapFindNextForKey(&map, 9, single);
    result                = result && !MapValueCursorIsValid(past);
    result                = result && !MapValueCursorIsValid(MapFindNextForKey(&map, 9, past));

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_map_cursor_invalidated_after_removal(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc  = DefaultAllocatorInit();
    IntIntMap        map    = MapInit(i32_hash, i32_compare, &alloc);
    MapValueCursor   cursor = MapValueCursorInvalid();

    MapInsertR(&map, 5, 50);
    MapInsertR(&map, 5, 51);

    cursor = MapFindFirstForKey(&map, 5);
    if (!MapValueCursorIsValid(cursor)) {
        MapDeinit(&map);
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    MapRemoveFirst(&map, 5);

    bool result = (MapValuePtrFromCursor(&map, cursor) == NULL);
    result      = result && (MapValueCountForKey(&map, 5) == 1);
    result      = result && MapGetFirstPtr(&map, 5) && (*MapGetFirstPtr(&map, 5) == 51);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Under a degenerate (all-colliding) hash, lookups must still return the
// value stored for the *requested* key, not whatever shares its bucket. This
// pins map_keys_equal on the probe path: a mutant that bypasses the key
// comparator (treats the first hash-matching slot as a match) returns a
// neighbour's value and turns this RED.
static bool test_map_collision_chain_lookup(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInitWithValueCompare(const_hash, i32_compare, i32_compare, &alloc);

    // Distinct keys, all in one bucket.
    for (int k = 0; k < 8; k++)
        MapInsertR(&map, k, k * 100 + 7);

    bool result = (MapPairCount(&map) == 8) && (MapUniqueKeyCount(&map) == 8);
    for (int k = 0; k < 8; k++) {
        int *v = MapGetFirstPtr(&map, k);
        result = result && v && (*v == k * 100 + 7);
        result = result && MapContainsKey(&map, k) && (MapValueCountForKey(&map, k) == 1);
    }
    // A key that collides into the chain but was never inserted is absent.
    result = result && !MapContainsKey(&map, 99) && (MapGetFirstPtr(&map, 99) == NULL);

    // Multivalued key inside the colliding chain: count is per-key, not
    // per-bucket.
    MapInsertR(&map, 3, 999);
    result = result && (MapValueCountForKey(&map, 3) == 2);
    result = result && (MapValueCountForKey(&map, 4) == 1);
    result = result && MapContainsPair(&map, 3, 307) && MapContainsPair(&map, 3, 999);
    result = result && !MapContainsPair(&map, 4, 999);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// One full forced-collision contract cycle, parameterised over the probing
// policy. Every key shares a single bucket (const_hash), so the probe
// sequence -- whatever the policy chooses -- must walk a multi-step chain to
// place and later find each key. We assert ONLY caller-observable outcomes:
// every key looks up to its OWN value, the count is right, removing an
// interior key drops the count by exactly one and leaves the rest reachable.
// We never assert probe order, slot indices, capacity, or tombstone counts.
//
// KEY_COUNT is chosen well above the old 4-key workload so the chain spans
// many probe steps under both policies; under quadratic that means a key
// landing at a triangular-number offset that a linearised formula would
// never revisit -- so a quadratic-degraded-to-linear mutant makes some key
// unreachable or returns a neighbour's value here.
static bool run_collision_contract_cycle(MapPolicy policy) {
    enum {
        KEY_COUNT = 24
    };
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInitWithValueCompareAndPolicy(const_hash, i32_compare, i32_compare, policy, &alloc);

    // Insert: distinct keys, all colliding into one bucket.
    for (int k = 0; k < KEY_COUNT; k++)
        MapInsertR(&map, k, k * 100 + 7);

    bool result = (MapPairCount(&map) == KEY_COUNT) && (MapUniqueKeyCount(&map) == KEY_COUNT);

    // Lookup: every inserted key must return ITS OWN value, not a neighbour's.
    for (int k = 0; k < KEY_COUNT; k++) {
        int *v = MapGetFirstPtr(&map, k);
        result = result && v && (*v == k * 100 + 7);
        result = result && MapContainsKey(&map, k) && (MapValueCountForKey(&map, k) == 1);
    }
    // A key that collides into the chain but was never inserted is absent.
    result = result && !MapContainsKey(&map, 9999) && (MapGetFirstPtr(&map, 9999) == NULL);

    // Remove an INTERIOR key from the collision chain.
    const int removed = KEY_COUNT / 2;
    MapRemoveFirst(&map, removed);

    // Count dropped by exactly one; the removed key is gone.
    result = result && (MapPairCount(&map) == (KEY_COUNT - 1));
    result = result && (MapUniqueKeyCount(&map) == (KEY_COUNT - 1));
    result = result && !MapContainsKey(&map, removed) && (MapGetFirstPtr(&map, removed) == NULL);

    // Every OTHER key is still reachable with its own value -- removing an
    // interior chain member must not strand keys probed after it.
    for (int k = 0; k < KEY_COUNT; k++) {
        if (k == removed)
            continue;
        int *v = MapGetFirstPtr(&map, k);
        result = result && v && (*v == k * 100 + 7);
        result = result && MapContainsKey(&map, k) && (MapValueCountForKey(&map, k) == 1);
    }

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Run the forced-collision contract cycle under the linear policy.
static bool test_map_collision_contract_linear(void) {
    return run_collision_contract_cycle(MapPolicyLinear);
}

// Run the same cycle under the quadratic policy. Degrading the quadratic
// probe formula to linear must surface a key that becomes unreachable or
// returns a wrong value here (mutation oracle O-7).
static bool test_map_collision_contract_quadratic(void) {
    return run_collision_contract_cycle(MapPolicyQuadratic);
}

int main(void) {
    TestFunction tests[] = {
        test_map_contains_and_find,
        test_map_get_ptr,
        test_map_get_first_ptr_is_live,
        test_map_empty_queries,
        test_map_get_or_default,
        test_map_value_cursor_query,
        test_map_cursor_invalidated_after_removal,
        test_map_collision_chain_lookup,
        test_map_collision_contract_linear,
        test_map_collision_contract_quadratic,
    };

    WriteFmt("[INFO] Starting Map.Access tests\n\n");
    return run_test_suite(tests, (int)(sizeof(tests) / sizeof(tests[0])), NULL, 0, "Map.Access");
}
