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

static size custom_probe(u64 hash, size probe_count, size capacity) {
    (void)capacity;
    return (size)(hash + probe_count);
}

static bool custom_should_rehash(const GenericMap *map) {
    return map->capacity == 0 || map->length >= map->capacity;
}

static bool test_map_type_defaults(void) {
    typedef Map(int, int) IntIntMap;
    IntIntMap map = MapInit(int_hash, int_compare);

    return map.length == 0 &&
           map.capacity == 0 &&
           map.entries == NULL &&
           map.states == NULL &&
           map.key_compare == int_compare &&
           map.key_hash == int_hash &&
           map.policy.probe_index == MisraMapPolicyLinear.probe_index &&
           map.policy.should_rehash == MisraMapPolicyLinear.should_rehash;
}

static bool test_map_policy_copy(void) {
    typedef Map(int, int) IntIntMap;
    MapPolicy custom_policy = {
        .name          = "custom-linear",
        .probe_index   = custom_probe,
        .should_rehash = custom_should_rehash,
    };
    IntIntMap map = MapInitWithPolicy(int_hash, int_compare, custom_policy);

    custom_policy.name          = "changed";
    custom_policy.probe_index   = NULL;
    custom_policy.should_rehash = NULL;

    return ZstrCompare(MapPolicyName(&map), "custom-linear") == 0 &&
           MapPolicyGet(&map).probe_index == custom_probe &&
           MapPolicyGet(&map).should_rehash == custom_should_rehash;
}

int main(void) {
    TestFunction tests[] = {
        test_map_type_defaults,
        test_map_policy_copy,
    };

    WriteFmt("[INFO] Starting Map.Type tests\n\n");
    return run_test_suite(tests, (int)(sizeof(tests) / sizeof(tests[0])), NULL, 0, "Map.Type");
}
