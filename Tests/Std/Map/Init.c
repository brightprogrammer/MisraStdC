#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Std/Container/Map.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Log.h>
#include "../../Util/TestRunner.h"

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

static bool custom_should_rehash(u64 length, u64 capacity, u64 tombstones, size pending_inserts, size probe_pressure) {
    (void)probe_pressure;
    return capacity == 0 || (length + tombstones + pending_inserts) > capacity;
}

static size custom_next_capacity(u64 length, u64 capacity, u64 tombstones, size min_entries) {
    size needed       = min_entries > length ? min_entries : (size)length;
    size new_capacity = 5;

    if (needed == 0) {
        return 0;
    }

    while (new_capacity < needed) {
        new_capacity += 5;
    }

    (void)capacity;
    (void)tombstones;
    return new_capacity;
}

static size custom_first_index(u64 hash, size capacity) {
    return capacity ? (size)(((hash * 3u) + 1u) % capacity) : 0;
}

static size custom_next_index(u64 hash, size capacity, size previous_index, size probe_count) {
    (void)hash;
    (void)probe_count;
    return capacity ? ((previous_index + 1) % capacity) : 0;
}

static bool test_map_reserve_and_clear(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInit(i32_hash, i32_compare, &alloc);
    size             reserved_capacity;

    MapReserve(&map, 32);
    reserved_capacity = (size)MapCapacity(&map);

    MapInsertR(&map, 1, 10);
    MapInsertR(&map, 1, 11);
    MapInsertR(&map, 2, 20);
    MapRemoveFirst(&map, 1);
    MapClear(&map);

    bool result = (reserved_capacity >= 32) && (MapCapacity(&map) == reserved_capacity) && (MapTombstones(&map) == 0) &&
                  (MapPairCount(&map) == 0) && MapEmpty(&map) && !MapContainsKey(&map, 1) && !MapContainsKey(&map, 2);

    MapSetOnlyR(&map, 7, 70);
    result = result && (MapPairCount(&map) == 1) && (MapValueCountForKey(&map, 7) == 1);
    result = result && MapGetFirstPtr(&map, 7) && (*MapGetFirstPtr(&map, 7) == 70);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_map_rehash_policy_switch(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInit(i32_hash, i32_compare, &alloc);

    for (int i = 0; i < 24; i++) {
        MapSetOnlyR(&map, i, i * 10);
    }

    MapRehashWithPolicy(&map, MapPairCount(&map), MapPolicyQuadratic);

    bool result = (MapPolicy(&map).first_index == MapPolicyQuadratic.first_index) &&
                  (MapPolicy(&map).next_index == MapPolicyQuadratic.next_index) &&
                  (MapPolicy(&map).next_capacity == MapPolicyQuadratic.next_capacity) &&
                  (MapPolicy(&map).should_rehash == MapPolicyQuadratic.should_rehash);

    for (int i = 0; i < 24; i++) {
        int *value = MapGetFirstPtr(&map, i);
        result     = result && value && (*value == i * 10);
    }

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_map_custom_policy_growth(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc         = DefaultAllocatorInit();
    MapPolicy        custom_policy = {
               .name            = "five-step",
               .should_rehash   = custom_should_rehash,
               .next_capacity   = custom_next_capacity,
               .first_index     = custom_first_index,
               .next_index      = custom_next_index,
               .max_probe_count = 32,
    };
    IntIntMap map    = MapInitWithPolicy(i32_hash, i32_compare, custom_policy, &alloc);
    bool      result = true;

    for (int i = 0; i < 6; i++) {
        MapSetOnlyR(&map, i, i + 100);
    }

    result = result && (MapCapacity(&map) == 10);
    result = result && (MapPolicy(&map).next_capacity == custom_next_capacity);

    for (int i = 0; i < 6; i++) {
        int *value = MapGetFirstPtr(&map, i);
        result     = result && value && (*value == (i + 100));
    }

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_map_init_defaults_are_linear(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInit(i32_hash, i32_compare, &alloc);

    // Fresh MapInit installs the linear policy by value and starts with
    // an empty, unallocated probe table.
    bool result = (MapPolicy(&map).should_rehash == MapPolicyLinear.should_rehash) &&
                  (MapPolicy(&map).next_capacity == MapPolicyLinear.next_capacity) &&
                  (MapPolicy(&map).first_index == MapPolicyLinear.first_index) &&
                  (MapPolicy(&map).next_index == MapPolicyLinear.next_index) &&
                  (MapPolicy(&map).max_probe_count == MapPolicyLinear.max_probe_count) && (MapPairCount(&map) == 0) &&
                  (MapCapacity(&map) == 0) && (MapTombstones(&map) == 0) && MapEmpty(&map);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_map_reserve_preserves_entries(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInit(i32_hash, i32_compare, &alloc);
    bool             result;
    int              i;

    for (i = 0; i < 8; i++)
        MapSetOnlyR(&map, i, i * 100);

    // Force a growth-driven rehash. Existing entries must survive it,
    // length must be unchanged, and capacity must grow to at least 64.
    result = MapReserve(&map, 64);
    result = result && (MapCapacity(&map) >= 64);
    result = result && (MapPairCount(&map) == 8);

    for (i = 0; i < 8; i++) {
        int *value = MapGetFirstPtr(&map, i);
        result     = result && value && (*value == i * 100);
    }

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_map_must_reserve(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInit(i32_hash, i32_compare, &alloc);

    MapInsertR(&map, 1, 10);
    MapInsertR(&map, 2, 20);
    MapMustReserve(&map, 48);

    bool result = (MapCapacity(&map) >= 48) && (MapPairCount(&map) == 2);
    result      = result && MapGetFirstPtr(&map, 1) && (*MapGetFirstPtr(&map, 1) == 10);
    result      = result && MapGetFirstPtr(&map, 2) && (*MapGetFirstPtr(&map, 2) == 20);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_map_init_typed(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInitT(map, i32_hash, i32_compare, &alloc);

    MapInsertR(&map, 5, 50);
    MapInsertR(&map, 6, 60);

    bool result = (MapKeyHash(&map) == i32_hash) && (MapKeyCompare(&map) == i32_compare) && (MapPairCount(&map) == 2);
    result      = result && MapGetFirstPtr(&map, 5) && (*MapGetFirstPtr(&map, 5) == 50);
    result      = result && MapGetFirstPtr(&map, 6) && (*MapGetFirstPtr(&map, 6) == 60);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_map_init_deep_copy_typed(void) {
    typedef Map(Zstr, Zstr) ZstrMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    ZstrMap          map   = MapInitWithDeepCopyT(
        map,
        zstr_hash,
        zstr_compare,
        zstr_init_clone,
        zstr_deinit,
        zstr_init_clone,
        zstr_deinit,
        &alloc
    );

    bool result = (MapKeyCopyInit(&map) == (GenericCopyInit)zstr_init_clone) &&
                  (MapValueCopyDeinit(&map) == (GenericCopyDeinit)zstr_deinit);

    MapInsertR(&map, "alpha", "first");
    result = result && (MapPairCount(&map) == 1);
    result = result && MapGetFirstPtr(&map, "alpha") && (ZstrCompare(*MapGetFirstPtr(&map, "alpha"), "first") == 0);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Contract: the map accommodates arbitrarily many inserts and every one stays
// retrievable with its value. HOW it does that -- when it grows, by how much,
// the capacity schedule -- is a performance strategy, NOT a contract, so this
// asserts only retrievability and never a capacity sequence. N is large enough
// to force several growths.
static bool test_map_grows_to_fit_many_inserts(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInit(i32_hash, i32_compare, &alloc);

    enum {
        N = 200
    };
    for (int i = 0; i < N; i++)
        MapMustInsertR(&map, i, i * 7 + 1);

    bool result = (MapPairCount(&map) == N);
    for (int i = 0; i < N; i++) {
        int *value = MapGetFirstPtr(&map, i);
        result     = result && value && (*value == i * 7 + 1);
    }

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

int main(void) {
    TestFunction tests[] = {
        test_map_grows_to_fit_many_inserts,
        test_map_init_defaults_are_linear,
        test_map_reserve_and_clear,
        test_map_reserve_preserves_entries,
        test_map_must_reserve,
        test_map_rehash_policy_switch,
        test_map_custom_policy_growth,
        test_map_init_typed,
        test_map_init_deep_copy_typed,
    };

    WriteFmt("[INFO] Starting Map.Init tests\n\n");
    return run_test_suite(tests, (int)(sizeof(tests) / sizeof(tests[0])), NULL, 0, "Map.Init");
}
