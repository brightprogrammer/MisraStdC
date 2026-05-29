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

int main(void) {
    TestFunction tests[] = {
        test_map_reserve_and_clear,
        test_map_rehash_policy_switch,
        test_map_custom_policy_growth,
    };

    WriteFmt("[INFO] Starting Map.Init tests\n\n");
    return run_test_suite(tests, (int)(sizeof(tests) / sizeof(tests[0])), NULL, 0, "Map.Init");
}
