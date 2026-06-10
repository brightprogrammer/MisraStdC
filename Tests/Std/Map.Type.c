#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Std/Container/Map.h>
#include <Misra/Std/Container/Str.h>
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

static bool
    custom_should_rehash_snapshot(u64 length, u64 capacity, u64 tombstones, size pending_inserts, size probe_pressure) {
    (void)probe_pressure;
    return capacity == 0 || (length + tombstones + pending_inserts) >= capacity;
}

static size custom_next_capacity(u64 length, u64 capacity, u64 tombstones, size min_entries) {
    size needed       = min_entries > length ? min_entries : (size)length;
    size new_capacity = 5;

    (void)tombstones;
    (void)capacity;

    if (needed == 0) {
        return 0;
    }

    while (new_capacity < needed) {
        new_capacity += 5;
    }

    return new_capacity;
}

static size custom_first_index(u64 hash, size capacity) {
    return capacity ? (size)((hash ^ 0x5au) % capacity) : 0;
}

static size custom_next_index(u64 hash, size capacity, size previous_index, size probe_count) {
    (void)hash;
    return capacity ? ((previous_index + probe_count + 1) % capacity) : 0;
}

static bool test_map_type_defaults(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInit(i32_hash, i32_compare, &alloc);

    bool result = MapPairCount(&map) == 0 && MapCapacity(&map) == 0 && MapTombstones(&map) == 0 &&
                  MapEntries(&map) == NULL && MapStates(&map) == NULL && MapKeyCompare(&map) == i32_compare &&
                  MapValueCompare(&map) == NULL && MapKeyHash(&map) == i32_hash &&
                  MapPolicy(&map).should_rehash == MapPolicyLinear.should_rehash &&
                  MapPolicy(&map).next_capacity == MapPolicyLinear.next_capacity &&
                  MapPolicy(&map).first_index == MapPolicyLinear.first_index &&
                  MapPolicy(&map).next_index == MapPolicyLinear.next_index &&
                  MapPolicy(&map).max_probe_count == MapPolicyLinear.max_probe_count;

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_map_type_with_value_compare(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInitWithValueCompare(i32_hash, i32_compare, i32_compare, &alloc);

    bool result =
        MapKeyCompare(&map) == i32_compare && MapValueCompare(&map) == i32_compare && MapKeyHash(&map) == i32_hash;

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_map_policy_copy(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc         = DefaultAllocatorInit();
    MapPolicy        custom_policy = {
               .name            = "custom-linear",
               .should_rehash   = custom_should_rehash_snapshot,
               .next_capacity   = custom_next_capacity,
               .first_index     = custom_first_index,
               .next_index      = custom_next_index,
               .max_probe_count = 11,
    };
    IntIntMap map = MapInitWithPolicy(i32_hash, i32_compare, custom_policy, &alloc);

    custom_policy.name            = "changed";
    custom_policy.should_rehash   = NULL;
    custom_policy.next_capacity   = NULL;
    custom_policy.first_index     = NULL;
    custom_policy.next_index      = NULL;
    custom_policy.max_probe_count = 0;

    bool result = ZstrCompare(MapPolicy(&map).name, "custom-linear") == 0 &&
                  MapPolicy(&map).should_rehash == custom_should_rehash_snapshot &&
                  MapPolicy(&map).next_capacity == custom_next_capacity &&
                  MapPolicy(&map).first_index == custom_first_index &&
                  MapPolicy(&map).next_index == custom_next_index && MapPolicy(&map).max_probe_count == 11;

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_validate_map_policy(void) {
    MapPolicy custom_policy = {
        .name            = "custom-linear",
        .should_rehash   = custom_should_rehash_snapshot,
        .next_capacity   = custom_next_capacity,
        .first_index     = custom_first_index,
        .next_index      = custom_next_index,
        .max_probe_count = 11,
    };

    ValidateMapPolicy(custom_policy);
    return custom_next_capacity(6, 5, 0, 7) == 10 && custom_first_index(0x55u, 5) < 5 &&
           custom_next_index(0x55u, 5, 1, 2) < 5;
}

static bool test_map_type_value_compare_and_policy(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc         = DefaultAllocatorInit();
    MapPolicy        custom_policy = {
               .name            = "vcmp-policy",
               .should_rehash   = custom_should_rehash_snapshot,
               .next_capacity   = custom_next_capacity,
               .first_index     = custom_first_index,
               .next_index      = custom_next_index,
               .max_probe_count = 13,
    };
    IntIntMap map = MapInitWithValueCompareAndPolicy(i32_hash, i32_compare, i32_compare, custom_policy, &alloc);

    bool result = MapKeyHash(&map) == i32_hash && MapKeyCompare(&map) == i32_compare &&
                  MapValueCompare(&map) == i32_compare && ZstrCompare(MapPolicy(&map).name, "vcmp-policy") == 0 &&
                  MapPolicy(&map).should_rehash == custom_should_rehash_snapshot &&
                  MapPolicy(&map).next_capacity == custom_next_capacity &&
                  MapPolicy(&map).first_index == custom_first_index &&
                  MapPolicy(&map).next_index == custom_next_index && MapPolicy(&map).max_probe_count == 13;

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_map_type_deep_copy_wiring(void) {
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

    bool result = MapKeyHash(&map) == (GenericHash)zstr_hash && MapKeyCompare(&map) == (GenericCompare)zstr_compare &&
                  MapValueCompare(&map) == NULL && MapKeyCopyInit(&map) == (GenericCopyInit)zstr_init_clone &&
                  MapKeyCopyDeinit(&map) == (GenericCopyDeinit)zstr_deinit &&
                  MapValueCopyInit(&map) == (GenericCopyInit)zstr_init_clone &&
                  MapValueCopyDeinit(&map) == (GenericCopyDeinit)zstr_deinit &&
                  MapAllocator(&map) == ALLOCATOR_OF(&alloc);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

int main(void) {
    TestFunction tests[] = {
        test_map_type_defaults,
        test_map_type_with_value_compare,
        test_map_policy_copy,
        test_validate_map_policy,
        test_map_type_value_compare_and_policy,
        test_map_type_deep_copy_wiring,
    };

    WriteFmt("[INFO] Starting Map.Type tests\n\n");
    return run_test_suite(tests, (int)(sizeof(tests) / sizeof(tests[0])), NULL, 0, "Map.Type");
}
