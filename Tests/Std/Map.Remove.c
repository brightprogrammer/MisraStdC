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
    IntIntMap map           = MapInit(int_hash, int_compare);
    int       removed_key   = -1;
    int       removed_value = -1;

    MapSetR(&map, 1, 10);
    MapInsertR(&map, 1, 11);
    MapSetR(&map, 2, 20);

    bool result = MapRemoveFirst(&map, 1, &removed_key, &removed_value);
    result      = result && (removed_key == 1) && (removed_value == 10);
    result      = result && MapContainsKey(&map, 1);
    result      = result && (MapValueCountForKey(&map, 1) == 1);
    result      = result && MapGetFirstPtr(&map, 1) && (*MapGetFirstPtr(&map, 1) == 11);
    result      = result && (MapPairCount(&map) == 2);

    MapDeinit(&map);
    return result;
}

static bool test_map_remove_all(void) {
    typedef Map(int, int) IntIntMap;
    IntIntMap map = MapInit(int_hash, int_compare);

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
    return result;
}

static bool test_map_tombstone_reuse(void) {
    typedef Map(int, int) IntIntMap;
    IntIntMap map = MapInit(int_hash, int_compare);

    for (int i = 0; i < 12; i++) {
        MapSetR(&map, i, i + 100);
    }

    MapRemoveFirst(&map, 5, NULL, NULL);
    MapSetR(&map, 105, 205);

    bool result = !MapContainsKey(&map, 5);
    result      = result && MapContainsKey(&map, 105);
    result      = result && MapGetFirstPtr(&map, 105) && (*MapGetFirstPtr(&map, 105) == 205);

    MapDeinit(&map);
    return result;
}

int main(void) {
    TestFunction tests[] = {
        test_map_remove_value,
        test_map_remove_all,
        test_map_tombstone_reuse,
    };

    WriteFmt("[INFO] Starting Map.Remove tests\n\n");
    return run_test_suite(tests, (int)(sizeof(tests) / sizeof(tests[0])), NULL, 0, "Map.Remove");
}
