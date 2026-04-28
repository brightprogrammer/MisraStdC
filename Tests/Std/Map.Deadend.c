#include <Misra/Std/Container/Map.h>
#include <Misra/Std/Log.h>

#include "../Util/TestRunner.h"

static u64 int_hash(const void *data, u32 size) {
    u64 x = (u64)(u32)(*(const int *)data);
    (void)size;
    x ^= x >> 33;
    x *= 0xff51afd7ed558ccdULL;
    x ^= x >> 33;
    return x;
}

static i32 int_compare(const void *lhs, const void *rhs) {
    int a = *(const int *)lhs;
    int b = *(const int *)rhs;
    return (a > b) - (a < b);
}

static bool test_validate_uninitialized_map_fails(void) {
    WriteFmt("Testing ValidateMap on uninitialized map\n");

    Map(int, int) map = {0};
    ValidateMap(&map);

    return false;
}

static bool test_map_contains_pair_without_value_compare_fails(void) {
    WriteFmt("Testing MapContainsPair without value comparator\n");

    typedef Map(int, int) IntIntMap;
    IntIntMap map = MapInit(int_hash, int_compare);

    MapContainsPair(&map, 1, 10);

    MapDeinit(&map);
    return false;
}

static bool test_map_remove_pair_without_value_compare_fails(void) {
    WriteFmt("Testing MapRemovePair without value comparator\n");

    typedef Map(int, int) IntIntMap;
    IntIntMap map = MapInit(int_hash, int_compare);

    MapRemovePair(&map, 1, 10);

    MapDeinit(&map);
    return false;
}

static bool test_map_remove_if_without_predicate_fails(void) {
    WriteFmt("Testing MapRemoveIf without predicate\n");

    typedef Map(int, int) IntIntMap;
    IntIntMap map = MapInit(int_hash, int_compare);

    MapRemoveIf(&map, NULL, NULL);

    MapDeinit(&map);
    return false;
}

static bool test_map_retain_if_without_predicate_fails(void) {
    WriteFmt("Testing MapRetainIf without predicate\n");

    typedef Map(int, int) IntIntMap;
    IntIntMap map = MapInit(int_hash, int_compare);

    MapRetainIf(&map, NULL, NULL);

    MapDeinit(&map);
    return false;
}

static bool always_rehash(u64 length, u64 capacity, u64 tombstones, size pending_inserts, size probe_pressure) {
    (void)length;
    (void)capacity;
    (void)tombstones;
    (void)pending_inserts;
    (void)probe_pressure;
    return true;
}

static size always_zero_capacity(u64 length, u64 capacity, u64 tombstones, size min_entries) {
    (void)length;
    (void)capacity;
    (void)tombstones;
    (void)min_entries;
    return 0;
}

static size linear_first_index(u64 hash, size capacity) {
    return capacity ? (size)(hash % capacity) : 0;
}

static size linear_next_index(u64 hash, size capacity, size previous_index, size probe_count) {
    (void)hash;
    (void)probe_count;
    return capacity ? ((previous_index + 1) % capacity) : 0;
}

static bool test_validate_map_policy_without_name_fails(void) {
    WriteFmt("Testing ValidateMapPolicy without name\n");

    MapPolicy policy = {
        .name            = "",
        .should_rehash   = always_rehash,
        .next_capacity   = always_zero_capacity,
        .first_index     = linear_first_index,
        .next_index      = linear_next_index,
        .max_probe_count = 8,
    };

    ValidateMapPolicy(policy);
    return false;
}

static bool test_validate_map_policy_without_probe_limit_fails(void) {
    WriteFmt("Testing ValidateMapPolicy without probe limit\n");

    MapPolicy policy = {
        .name            = "invalid",
        .should_rehash   = always_rehash,
        .next_capacity   = always_zero_capacity,
        .first_index     = linear_first_index,
        .next_index      = linear_next_index,
        .max_probe_count = 0,
    };

    ValidateMapPolicy(policy);
    return false;
}

int main(void) {
    TestFunction deadend_tests[] = {
        test_validate_uninitialized_map_fails,
        test_map_contains_pair_without_value_compare_fails,
        test_map_remove_pair_without_value_compare_fails,
        test_map_remove_if_without_predicate_fails,
        test_map_retain_if_without_predicate_fails,
        test_validate_map_policy_without_name_fails,
        test_validate_map_policy_without_probe_limit_fails,
    };

    WriteFmt("[INFO] Starting Map.Deadend tests\n\n");
    return run_test_suite(NULL, 0, deadend_tests, (int)(sizeof(deadend_tests) / sizeof(deadend_tests[0])), "Map.Deadend");
}
