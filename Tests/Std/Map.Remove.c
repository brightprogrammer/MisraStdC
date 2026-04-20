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

static bool test_map_remove_value(void) {
    typedef Map(int, int) IntIntMap;
    IntIntMap map = MapInit(int_hash, int_compare);
    int removed_key = -1;
    int removed_value = -1;

    MapSetR(&map, 1, 10);
    MapSetR(&map, 2, 20);

    bool result = MapRemove(&map, 1, &removed_key, &removed_value);
    result = result && (removed_key == 1) && (removed_value == 10);
    result = result && !MapContains(&map, 1);
    result = result && (MapLen(&map) == 1);

    MapDeinit(&map);
    return result;
}

static bool test_map_tombstone_reuse(void) {
    typedef Map(int, int) IntIntMap;
    IntIntMap map = MapInit(int_hash, int_compare);

    for (int i = 0; i < 12; i++) {
        MapSetR(&map, i, i + 100);
    }

    MapDelete(&map, 5);
    MapSetR(&map, 105, 205);

    bool result = !MapContains(&map, 5);
    result = result && MapContains(&map, 105);
    result = result && MapGetPtr(&map, 105) && (*MapGetPtr(&map, 105) == 205);

    MapDeinit(&map);
    return result;
}

int main(void) {
    TestFunction tests[] = {
        test_map_remove_value,
        test_map_tombstone_reuse,
    };

    WriteFmt("[INFO] Starting Map.Remove tests\n\n");
    return run_test_suite(tests, (int)(sizeof(tests) / sizeof(tests[0])), NULL, 0, "Map.Remove");
}
