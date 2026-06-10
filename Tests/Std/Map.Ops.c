#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Std/Container/Map.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Memory.h>
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

static bool test_map_deep_copy_zstrs(void) {
    typedef Map(Zstr, Zstr) ZstrMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    ZstrMap          map   = MapInitWithDeepCopy(
        zstr_hash,
        zstr_compare,
        zstr_init_clone,
        zstr_deinit,
        zstr_init_clone,
        zstr_deinit,
        &alloc
    );
    char        key_buf[]          = "alpha";
    char        value_buf[]        = "first";
    char        second_value_buf[] = "second";
    const char *key                = key_buf;
    const char *value              = value_buf;
    const char *second_value       = second_value_buf;
    Zstr       *stored_value;
    int         value_count = 0;

    MapInsertL(&map, key, value);
    MapInsertL(&map, key, second_value);

    bool result         = (key == key_buf) && (value == value_buf) && (second_value == second_value_buf);
    key_buf[0]          = 'o';
    value_buf[0]        = 'w';
    second_value_buf[0] = 'S';

    result       = result && MapContainsKey(&map, "alpha");
    result       = result && !MapContainsKey(&map, key);
    result       = result && (MapValueCountForKey(&map, "alpha") == 2);
    stored_value = MapGetFirstPtr(&map, "alpha");
    result       = result && stored_value && (*stored_value != value) && (ZstrCompare(*stored_value, "first") == 0);
    MapForeachValueForKey(&map, "alpha", entry_value) {
        if ((ZstrCompare(entry_value, "first") == 0) || (ZstrCompare(entry_value, "second") == 0)) {
            value_count += 1;
        }
    }
    result = result && (value_count == 2);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_map_policy_switch_preserves_entries(void) {
    typedef Map(Zstr, Zstr) ZstrMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    ZstrMap          map   = MapInitWithDeepCopy(
        zstr_hash,
        zstr_compare,
        zstr_init_clone,
        zstr_deinit,
        zstr_init_clone,
        zstr_deinit,
        &alloc
    );

    int red_count = 0;

    MapSetOnlyR(&map, "red", "apple");
    MapInsertR(&map, "red", "cherry");
    MapSetOnlyR(&map, "yellow", "banana");
    MapSetOnlyR(&map, "green", "pear");
    MapRehashWithPolicy(&map, MapPairCount(&map), MapPolicyQuadratic);

    bool result = (MapPolicy(&map).first_index == MapPolicyQuadratic.first_index) &&
                  (MapPolicy(&map).next_index == MapPolicyQuadratic.next_index) &&
                  (MapPolicy(&map).next_capacity == MapPolicyQuadratic.next_capacity) &&
                  (MapPolicy(&map).should_rehash == MapPolicyQuadratic.should_rehash);
    result = result && (MapValueCountForKey(&map, "red") == 2);
    result = result && MapGetFirstPtr(&map, "red") && (ZstrCompare(*MapGetFirstPtr(&map, "red"), "apple") == 0);
    result = result && MapGetFirstPtr(&map, "yellow") && (ZstrCompare(*MapGetFirstPtr(&map, "yellow"), "banana") == 0);
    result = result && MapGetFirstPtr(&map, "green") && (ZstrCompare(*MapGetFirstPtr(&map, "green"), "pear") == 0);
    MapForeachValueForKey(&map, "red", red_value) {
        if ((ZstrCompare(red_value, "apple") == 0) || (ZstrCompare(red_value, "cherry") == 0)) {
            red_count += 1;
        }
    }
    result = result && (red_count == 2);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_map_compact_and_swap(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc  = DefaultAllocatorInit();
    IntIntMap        first  = MapInitWithValueCompare(i32_hash, i32_compare, i32_compare, &alloc);
    IntIntMap        second = MapInitWithValueCompare(i32_hash, i32_compare, i32_compare, &alloc);

    MapInsertR(&first, 1, 10);
    MapInsertR(&first, 1, 11);
    MapInsertR(&first, 2, 20);
    MapRemoveFirst(&first, 1);

    MapInsertR(&second, 9, 90);
    MapInsertR(&second, 10, 100);

    bool result = (MapTombstones(&first) == 1);
    MapCompact(&first);

    result = result && (MapTombstones(&first) == 0);
    result = result && MapContainsPair(&first, 1, 11);
    result = result && MapContainsPair(&first, 2, 20);
    result = result && (MapPairCount(&first) == 2);
    result = result && (MapUniqueKeyCount(&first) == 2);

    MapSwap(&first, &second);

    result = result && MapContainsPair(&first, 9, 90);
    result = result && MapContainsPair(&first, 10, 100);
    result = result && (MapPairCount(&first) == 2);
    result = result && MapContainsPair(&second, 1, 11);
    result = result && MapContainsPair(&second, 2, 20);
    result = result && (MapPairCount(&second) == 2);

    MapDeinit(&first);
    MapDeinit(&second);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool retain_values_above_threshold(const void *key, const void *value, void *ctx) {
    const int *threshold = ctx;

    (void)key;
    return *(const int *)value >= *threshold;
}

// MapEmpty must be true only when length is exactly 0; it must flip to
// false the instant a single pair exists, and back to true once every
// pair is removed/cleared.
static bool test_map_empty_true_and_false(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInit(i32_hash, i32_compare, &alloc);

    bool result = MapEmpty(&map);

    MapInsertR(&map, 1, 10);
    result = result && !MapEmpty(&map);

    MapInsertR(&map, 2, 20);
    result = result && !MapEmpty(&map);

    // Removing one of two pairs must keep it non-empty.
    MapRemoveFirst(&map, 1);
    result = result && !MapEmpty(&map);

    // Removing the last pair must make it empty again.
    MapRemoveFirst(&map, 2);
    result = result && MapEmpty(&map);
    result = result && (MapPairCount(&map) == 0);

    // Refill then clear -> empty.
    MapInsertR(&map, 3, 30);
    MapInsertR(&map, 4, 40);
    result = result && !MapEmpty(&map);
    MapClear(&map);
    result = result && MapEmpty(&map);
    result = result && (MapPairCount(&map) == 0);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// MapMustCompact success path: the effect (tombstone removal, entries
// preserved, length exact) happened and control returned to the caller.
static bool test_map_must_compact_success(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInitWithValueCompare(i32_hash, i32_compare, i32_compare, &alloc);

    MapInsertR(&map, 1, 10);
    MapInsertR(&map, 2, 20);
    MapInsertR(&map, 3, 30);
    MapRemoveFirst(&map, 2);

    bool result = (MapTombstones(&map) == 1);

    MapMustCompact(&map);

    // Returned to caller; tombstones cleared, survivors intact, length exact.
    result = result && (MapTombstones(&map) == 0);
    result = result && MapContainsPair(&map, 1, 10);
    result = result && MapContainsPair(&map, 3, 30);
    result = result && !MapContainsKey(&map, 2);
    result = result && (MapPairCount(&map) == 2);
    result = result && (MapUniqueKeyCount(&map) == 2);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// MapMustRehashWithPolicy success path: policy switched in by value, every
// live entry survives the rehash, tombstones gone, and control returned.
static bool test_map_must_rehash_with_policy_success(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInitWithValueCompare(i32_hash, i32_compare, i32_compare, &alloc);

    MapInsertR(&map, 1, 10);
    MapInsertR(&map, 2, 20);
    MapInsertR(&map, 2, 21);
    MapInsertR(&map, 3, 30);
    MapRemoveFirst(&map, 1);

    bool result = (MapTombstones(&map) == 1);

    MapMustRehashWithPolicy(&map, MapPairCount(&map), MapPolicyQuadratic);

    // Policy is now quadratic (copied in by value).
    result = result && (MapPolicy(&map).first_index == MapPolicyQuadratic.first_index);
    result = result && (MapPolicy(&map).next_index == MapPolicyQuadratic.next_index);
    result = result && (MapPolicy(&map).next_capacity == MapPolicyQuadratic.next_capacity);
    result = result && (MapPolicy(&map).should_rehash == MapPolicyQuadratic.should_rehash);
    // Tombstones gone, survivors intact, exact counts preserved.
    result = result && (MapTombstones(&map) == 0);
    result = result && !MapContainsKey(&map, 1);
    result = result && MapContainsPair(&map, 2, 20);
    result = result && MapContainsPair(&map, 2, 21);
    result = result && MapContainsPair(&map, 3, 30);
    result = result && (MapValueCountForKey(&map, 2) == 2);
    result = result && (MapPairCount(&map) == 3);
    result = result && (MapUniqueKeyCount(&map) == 2);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_map_retain_if(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc     = DefaultAllocatorInit();
    IntIntMap        map       = MapInit(i32_hash, i32_compare, &alloc);
    int              threshold = 30;

    MapInsertR(&map, 1, 10);
    MapInsertR(&map, 2, 20);
    MapInsertR(&map, 3, 30);
    MapInsertR(&map, 4, 40);

    bool result = (MapRetainIf(&map, retain_values_above_threshold, &threshold) == 2);
    result      = result && !MapContainsKey(&map, 1);
    result      = result && !MapContainsKey(&map, 2);
    result      = result && MapContainsKey(&map, 3);
    result      = result && MapContainsKey(&map, 4);
    result      = result && (MapPairCount(&map) == 2);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

int main(void) {
    TestFunction tests[] = {
        test_map_deep_copy_zstrs,
        test_map_policy_switch_preserves_entries,
        test_map_compact_and_swap,
        test_map_retain_if,
        test_map_empty_true_and_false,
        test_map_must_compact_success,
        test_map_must_rehash_with_policy_success,
    };

    WriteFmt("[INFO] Starting Map.Ops tests\n\n");
    return run_test_suite(tests, (int)(sizeof(tests) / sizeof(tests[0])), NULL, 0, "Map.Ops");
}
