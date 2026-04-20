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

static bool test_map_reserve_and_clear(void) {
    typedef Map(int, int) IntIntMap;
    IntIntMap map = MapInit(int_hash, int_compare);

    MapReserve(&map, 32);
    MapSetR(&map, 1, 10);
    MapSetR(&map, 2, 20);
    MapClear(&map);

    bool result = MapCapacity(&map) >= 32 && MapLen(&map) == 0 && MapEmpty(&map);

    MapDeinit(&map);
    return result;
}

static bool test_map_rehash_policy_switch(void) {
    typedef Map(int, int) IntIntMap;
    IntIntMap map = MapInit(int_hash, int_compare);

    for (int i = 0; i < 24; i++) {
        MapSetR(&map, i, i * 10);
    }

    MapRehashWithPolicy(&map, MapLen(&map), MisraMapPolicyQuadratic);

    bool result = MapPolicyGet(&map).probe_index == MisraMapPolicyQuadratic.probe_index;

    for (int i = 0; i < 24; i++) {
        int *value = MapGetPtr(&map, i);
        result = result && value && (*value == i * 10);
    }

    MapDeinit(&map);
    return result;
}

int main(void) {
    TestFunction tests[] = {
        test_map_reserve_and_clear,
        test_map_rehash_policy_switch,
    };

    WriteFmt("[INFO] Starting Map.Init tests\n\n");
    return run_test_suite(tests, (int)(sizeof(tests) / sizeof(tests[0])), NULL, 0, "Map.Init");
}
