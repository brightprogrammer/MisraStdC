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

static bool test_map_contains_and_find(void) {
    typedef Map(int, int) IntIntMap;
    IntIntMap map = MapInit(int_hash, int_compare);

    MapSetR(&map, 7, 70);
    MapSetR(&map, 9, 90);

    bool result = MapContains(&map, 7);
    result = result && MapContains(&map, 9);
    result = result && !MapContains(&map, 8);
    result = result && (MapFindIndex(&map, 7) < MapCapacity(&map));
    result = result && (MapFindIndex(&map, 8) == MapCapacity(&map));
    result = result && (MapLoadFactor(&map) > 0.0);

    MapDeinit(&map);
    return result;
}

static bool test_map_get_ptr(void) {
    typedef Map(int, int) IntIntMap;
    IntIntMap map = MapInit(int_hash, int_compare);

    MapSetR(&map, 11, 110);

    int *value = MapGetPtr(&map, 11);
    bool result = value && (*value == 110);
    result = result && (MapGetPtr(&map, 999) == NULL);

    MapDeinit(&map);
    return result;
}

int main(void) {
    TestFunction tests[] = {
        test_map_contains_and_find,
        test_map_get_ptr,
    };

    WriteFmt("[INFO] Starting Map.Access tests\n\n");
    return run_test_suite(tests, (int)(sizeof(tests) / sizeof(tests[0])), NULL, 0, "Map.Access");
}
