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

static bool test_map_insert_and_set(void) {
    typedef Map(int, int) IntIntMap;
    IntIntMap map = MapInit(int_hash, int_compare);

    MapInsertR(&map, 1, 10);
    MapInsertR(&map, 1, 11);
    MapInsertR(&map, 2, 20);
    MapSetR(&map, 2, 200);
    MapSetR(&map, 3, 30);

    bool result = MapPairCount(&map) == 4;
    result      = result && (MapValueCountForKey(&map, 1) == 2);
    result      = result && (MapValueCountForKey(&map, 2) == 1);
    result      = result && (MapValueCountForKey(&map, 3) == 1);
    result      = result && MapGetFirstPtr(&map, 1) && (*MapGetFirstPtr(&map, 1) == 10);
    result      = result && MapGetFirstPtr(&map, 2) && (*MapGetFirstPtr(&map, 2) == 200);
    result      = result && MapGetFirstPtr(&map, 3) && (*MapGetFirstPtr(&map, 3) == 30);

    MapDeinit(&map);
    return result;
}

static bool test_map_lvalue_zeroing(void) {
    typedef Map(int, int) IntIntMap;
    IntIntMap map   = MapInit(int_hash, int_compare);
    int       key   = 42;
    int       value = 84;

    MapInsertL(&map, key, value);

    bool result = (key == 0) && (value == 0);
    result      = result && (MapValueCountForKey(&map, 42) == 1);
    result      = result && MapGetFirstPtr(&map, 42) && (*MapGetFirstPtr(&map, 42) == 84);

    MapDeinit(&map);
    return result;
}

static bool test_map_ensure_ptr(void) {
    typedef Map(int, int) IntIntMap;
    IntIntMap map = MapInit(int_hash, int_compare);
    int      *value_ptr;
    bool      result;

    value_ptr = MapEnsurePtr(&map, 8, 80);
    result    = value_ptr && (*value_ptr == 80);
    result    = result && (MapPairCount(&map) == 1);
    result    = result && (MapValueCountForKey(&map, 8) == 1);

    value_ptr = MapGetOrInsertPtr(&map, 8, 800);
    result    = result && value_ptr && (*value_ptr == 80);
    result    = result && (MapPairCount(&map) == 1);
    result    = result && (MapValueCountForKey(&map, 8) == 1);

    MapDeinit(&map);
    return result;
}

int main(void) {
    TestFunction tests[] = {
        test_map_insert_and_set,
        test_map_lvalue_zeroing,
        test_map_ensure_ptr,
    };

    WriteFmt("[INFO] Starting Map.Insert tests\n\n");
    return run_test_suite(tests, (int)(sizeof(tests) / sizeof(tests[0])), NULL, 0, "Map.Insert");
}
