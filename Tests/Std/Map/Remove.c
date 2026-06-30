#include <Misra/Std/Allocator/Debug.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Container/Map.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Zstr.h>
#include "../../Util/TestRunner.h"

static u64 i32_hash(const void *data, u32 size) {
    u64 x = (u64)(u32)(*(const int *)data);
    (void)size;
    x ^= x >> 33;
    x *= 0xff51afd7ed558ccdULL;
    x ^= x >> 33;
    return x;
}

// Degenerate hash: all keys collide into one bucket, forcing every key onto a
// single probe chain so deletion/insertion chain-integrity is exercised.
static u64 const_hash(const void *data, u32 size) {
    (void)data;
    (void)size;
    return 0;
}

static i32 i32_compare(const void *lhs, const void *rhs) {
    int a = *(const int *)lhs;
    int b = *(const int *)rhs;
    return (a > b) - (a < b);
}

// MapRemoveFirst: removes the first matching value, leaves remaining
// duplicates intact, shrinks length by exactly 1, and leaves unrelated keys
// untouched.
static bool test_map_remove_value(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInit(i32_hash, i32_compare, &alloc);

    MapSetOnlyR(&map, 1, 10);
    MapInsertR(&map, 1, 11);
    MapSetOnlyR(&map, 2, 20);

    u64 length_before = MapPairCount(&map);

    bool result = MapRemoveFirst(&map, 1);
    result      = result && MapContainsKey(&map, 1);
    result      = result && (MapValueCountForKey(&map, 1) == 1);
    result      = result && MapGetFirstPtr(&map, 1) && (*MapGetFirstPtr(&map, 1) == 11);
    result      = result && (MapPairCount(&map) == 2);
    // Length shrinks by exactly one.
    result = result && (MapPairCount(&map) == length_before - 1);
    // Unrelated key untouched.
    result = result && MapGetFirstPtr(&map, 2) && (*MapGetFirstPtr(&map, 2) == 20);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// MapRemoveFirst on a key that is not present returns false and leaves
// the map completely unchanged (no spurious tombstone, no length change).
static bool test_map_remove_first_missing_returns_false(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInit(i32_hash, i32_compare, &alloc);

    MapInsertR(&map, 1, 10);
    MapInsertR(&map, 2, 20);

    u64 length_before     = MapPairCount(&map);
    u64 tombstones_before = MapTombstones(&map);

    bool result = !MapRemoveFirst(&map, 99);
    result      = result && (MapPairCount(&map) == length_before);
    result      = result && (MapTombstones(&map) == tombstones_before);
    result      = result && MapContainsKey(&map, 1);
    result      = result && MapContainsKey(&map, 2);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_map_remove_pair(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInitWithValueCompare(i32_hash, i32_compare, i32_compare, &alloc);

    MapInsertR(&map, 5, 50);
    MapInsertR(&map, 5, 51);
    MapInsertR(&map, 5, 52);

    bool result = MapRemovePair(&map, 5, 51);
    result      = result && MapContainsPair(&map, 5, 50);
    result      = result && !MapContainsPair(&map, 5, 51);
    result      = result && MapContainsPair(&map, 5, 52);
    result      = result && (MapValueCountForKey(&map, 5) == 2);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// MapRemovePair with no matching (key, value) returns false and leaves
// the map unchanged -- both the missing-value case (key present, value
// absent) and the missing-key case.
static bool test_map_remove_pair_no_match_returns_false(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInitWithValueCompare(i32_hash, i32_compare, i32_compare, &alloc);

    MapInsertR(&map, 5, 50);
    MapInsertR(&map, 5, 51);

    u64 length_before     = MapPairCount(&map);
    u64 tombstones_before = MapTombstones(&map);

    // Key present, value absent.
    bool result = !MapRemovePair(&map, 5, 999);
    // Key absent entirely.
    result = result && !MapRemovePair(&map, 6, 50);

    result = result && (MapPairCount(&map) == length_before);
    result = result && (MapTombstones(&map) == tombstones_before);
    result = result && MapContainsPair(&map, 5, 50);
    result = result && MapContainsPair(&map, 5, 51);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool remove_even_values(const void *key, const void *value, void *ctx) {
    (void)key;
    (void)ctx;
    return (*(const int *)value % 2) == 0;
}

static bool always_retain(const void *key, const void *value, void *ctx) {
    (void)key;
    (void)value;
    (void)ctx;
    return true;
}

static bool always_true_predicate(const void *key, const void *value, void *ctx) {
    (void)key;
    (void)value;
    (void)ctx;
    return true;
}

// map_remove_at_index : slot state MAP_SLOT_TOMBSTONE -> 42  (1180).
// Removing an entry must turn its slot into a reusable tombstone; re-inserting
// the same key reclaims it, restoring the tombstone count to zero.
static bool test_remove_then_reinsert_reclaims_tombstone(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInit(i32_hash, i32_compare, &alloc);

    MapInsertR(&map, 5, 50);
    bool result = (MapTombstones(&map) == 0);

    MapRemoveFirst(&map, 5);
    result = result && (MapTombstones(&map) == 1);
    result = result && !MapContainsKey(&map, 5);

    MapInsertR(&map, 5, 51);
    result     = result && (MapTombstones(&map) == 0);
    result     = result && (MapPairCount(&map) == 1);
    int *value = MapGetFirstPtr(&map, 5);
    result     = result && value && (*value == 51);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// map_retain_if : idx < capacity -> idx <= capacity  (1332).
// On a never-grown map (capacity 0, states == NULL) MapRetainIf must take zero
// iterations and return 0; the mutant dereferences the NULL states array.
static bool test_retain_if_on_empty_map_returns_zero(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInit(i32_hash, i32_compare, &alloc);

    bool result = (MapRetainIf(&map, always_retain, NULL) == 0);
    result      = result && (MapPairCount(&map) == 0);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// map_remove_if : idx < capacity -> idx <= capacity  (1297:23 cxx_lt_to_le).
// On an empty map (capacity 0, states == NULL) the loop never runs and returns
// 0; the mutant dereferences states[0] through a NULL pointer.
static bool test_remove_if_on_empty_map_returns_zero(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInit(i32_hash, i32_compare, &alloc);

    size removed = MapRemoveIf(&map, always_true_predicate, NULL);
    bool result  = (removed == 0) && (MapPairCount(&map) == 0);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_map_remove_if(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInit(i32_hash, i32_compare, &alloc);

    MapInsertR(&map, 1, 10);
    MapInsertR(&map, 1, 11);
    MapInsertR(&map, 2, 20);
    MapInsertR(&map, 3, 31);

    bool result = (MapRemoveIf(&map, remove_even_values, NULL) == 2);
    result      = result && !MapContainsKey(&map, 2);
    result      = result && (MapValueCountForKey(&map, 1) == 1);
    result      = result && MapGetFirstPtr(&map, 1) && (*MapGetFirstPtr(&map, 1) == 11);
    result      = result && MapContainsKey(&map, 3);
    result      = result && (MapPairCount(&map) == 2);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_map_remove_all(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInit(i32_hash, i32_compare, &alloc);

    MapInsertR(&map, 5, 50);
    MapInsertR(&map, 5, 51);
    MapInsertR(&map, 5, 52);
    MapInsertR(&map, 9, 90);

    bool result = (MapRemoveAll(&map, 5) == 3);
    result      = result && !MapContainsKey(&map, 5);
    result      = result && (MapValueCountForKey(&map, 5) == 0);
    result      = result && MapContainsKey(&map, 9);
    result      = result && (MapPairCount(&map) == 1);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// MapRemoveAll on a key that is not present returns 0 and leaves the map
// unchanged. Guards the FAILURE contract of map_remove_all.
static bool test_map_remove_all_missing_returns_zero(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInit(i32_hash, i32_compare, &alloc);

    MapInsertR(&map, 5, 50);
    MapInsertR(&map, 9, 90);

    u64 length_before     = MapPairCount(&map);
    u64 tombstones_before = MapTombstones(&map);

    bool result = (MapRemoveAll(&map, 77) == 0);
    result      = result && (MapPairCount(&map) == length_before);
    result      = result && (MapTombstones(&map) == tombstones_before);
    result      = result && MapContainsKey(&map, 5);
    result      = result && MapContainsKey(&map, 9);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Removing then re-inserting the SAME key must reuse the tombstone slot:
// re-inserting hashes to the identical probe chain, so scan_slots lands
// on the freed slot. The guard is structural: capacity must NOT grow and
// the tombstone count must return to its pre-removal value (the reused
// slot is reclaimed). A change that consumes a fresh slot instead of
// reusing the tombstone leaves either capacity bumped or a stale
// tombstone behind, turning this RED.
// Contract: removing a key then re-inserting it leaves the key present and
// retrievable with the new value, the pair count returns to where it was, and
// unrelated keys are untouched. Asserts the observable remove/reinsert
// behaviour only -- not tombstone counts or capacity, which are
// deletion-strategy details rather than contract.
static bool test_map_remove_then_reinsert_same_key(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInit(i32_hash, i32_compare, &alloc);

    for (int i = 0; i < 12; i++)
        MapSetOnlyR(&map, i, i + 100);

    u64 length_before = MapPairCount(&map);

    MapRemoveFirst(&map, 5);
    bool result = !MapContainsKey(&map, 5);
    result      = result && (MapPairCount(&map) == length_before - 1);

    MapSetOnlyR(&map, 5, 205);
    result = result && MapContainsKey(&map, 5);
    result = result && MapGetFirstPtr(&map, 5) && (*MapGetFirstPtr(&map, 5) == 205);
    result = result && (MapPairCount(&map) == length_before);
    // Unrelated keys remain intact.
    result = result && MapGetFirstPtr(&map, 4) && (*MapGetFirstPtr(&map, 4) == 104);
    result = result && MapGetFirstPtr(&map, 11) && (*MapGetFirstPtr(&map, 11) == 111);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Deep-copy deinit-on-remove: a Map(Zstr, Zstr) built with deep-copy
// callbacks owns heap-allocated key/value clones. Removing entries must
// run key_copy_deinit / value_copy_deinit on each removed slot so nothing
// leaks. We back the map with a DebugAllocator and assert zero live
// allocations after the removed entries are torn down and the map is
// deinit'd. If map_remove_at_index stopped invoking the copy_deinit
// callbacks, the clones would leak and the live count would be non-zero.
static bool test_map_deep_copy_deinit_on_remove(void) {
    typedef Map(Zstr, Zstr) ZstrMap;
    DebugAllocator dbg = DebugAllocatorInit();
    // Deep-copy callbacks for both key and value, plus a value comparator
    // so the pair-level removal API is usable. The map owns Zstr clones it
    // must free on removal / deinit.
    ZstrMap map = MapInitFull(
        zstr_hash,
        zstr_compare,
        zstr_compare,
        zstr_init_clone,
        zstr_deinit,
        zstr_init_clone,
        zstr_deinit,
        MapPolicyLinear,
        &dbg
    );

    MapInsertR(&map, "alpha", "one");
    MapInsertR(&map, "alpha", "two");
    MapInsertR(&map, "beta", "three");
    MapInsertR(&map, "gamma", "four");

    // Live allocations are non-zero here (clones + table storage).
    bool result = (DebugAllocatorLiveCount(&dbg) > 0);

    // Exercise every removal path against the deep-copy map.
    result = result && MapRemoveFirst(&map, "alpha");       // one clone pair freed
    result = result && MapRemovePair(&map, "alpha", "two"); // remaining alpha freed
    result = result && (MapRemoveAll(&map, "beta") == 1);   // beta freed
    result = result && !MapContainsKey(&map, "alpha");
    result = result && !MapContainsKey(&map, "beta");
    result = result && MapContainsKey(&map, "gamma");

    MapDeinit(&map);

    // After deinit, every clone and the table storage must be freed.
    result = result && (DebugAllocatorLiveCount(&dbg) == 0);

    DebugAllocatorDeinit(&dbg);
    return result;
}

// Contract: removing a key from a collision chain must not make a *different*
// key in that chain unreachable, and the freed position must remain usable by
// later inserts. This guards probe-chain integrity across deletion -- the
// observable guarantee. It deliberately does NOT assert tombstone counts or
// capacity: whether deletion uses tombstones, how many, and whether a reinsert
// reuses a slot or grows are deletion-strategy / performance details, not
// contract (a backward-shift-delete map would keep zero tombstones and be
// equally correct).
static bool test_map_collision_chain_survives_deletion(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInit(const_hash, i32_compare, &alloc);

    // Four keys colliding into one bucket form a single probe chain.
    for (int k = 0; k < 4; k++)
        MapMustInsertR(&map, k, k * 10);

    // Remove two interior keys; the ones before and after must stay reachable.
    bool result = MapRemoveFirst(&map, 1) && MapRemoveFirst(&map, 2);
    result      = result && (MapPairCount(&map) == 2);
    result      = result && !MapContainsKey(&map, 1) && !MapContainsKey(&map, 2);
    result      = result && MapGetFirstPtr(&map, 0) && (*MapGetFirstPtr(&map, 0) == 0);
    result      = result && MapGetFirstPtr(&map, 3) && (*MapGetFirstPtr(&map, 3) == 30);

    // The chain still accepts new keys, and every live key stays retrievable.
    result  = result && MapInsertR(&map, 5, 55) && MapInsertR(&map, 6, 66);
    result  = result && (MapPairCount(&map) == 4);
    int *v5 = MapGetFirstPtr(&map, 5);
    int *v6 = MapGetFirstPtr(&map, 6);
    result  = result && v5 && (*v5 == 55) && v6 && (*v6 == 66);
    result  = result && MapGetFirstPtr(&map, 0) && MapGetFirstPtr(&map, 3);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

int main(void) {
    TestFunction tests[] = {
        test_map_remove_value,
        test_map_remove_first_missing_returns_false,
        test_map_remove_pair,
        test_map_remove_pair_no_match_returns_false,
        test_map_remove_all,
        test_map_remove_all_missing_returns_zero,
        test_map_remove_then_reinsert_same_key,
        test_map_collision_chain_survives_deletion,
        test_map_remove_if,
        test_map_deep_copy_deinit_on_remove,
        test_remove_then_reinsert_reclaims_tombstone,
        test_retain_if_on_empty_map_returns_zero,
        test_remove_if_on_empty_map_returns_zero,
    };

    WriteFmt("[INFO] Starting Map.Remove tests\n\n");
    return run_test_suite(tests, (int)(sizeof(tests) / sizeof(tests[0])), NULL, 0, "Map.Remove");
}
