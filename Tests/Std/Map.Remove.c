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

static bool test_map_remove_value(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInit(i32_hash, i32_compare, &alloc);

    MapSetOnlyR(&map, 1, 10);
    MapInsertR(&map, 1, 11);
    MapSetOnlyR(&map, 2, 20);

    bool result = MapRemoveFirst(&map, 1);
    result      = result && MapContainsKey(&map, 1);
    result      = result && (MapValueCountForKey(&map, 1) == 1);
    result      = result && MapGetFirstPtr(&map, 1) && (*MapGetFirstPtr(&map, 1) == 11);
    result      = result && (MapPairCount(&map) == 2);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_map_remove_pair(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInitWithValueCompare(i32_hash, i32_compare, i32_compare, &alloc);

    MapInsertR(&map, 5, 50);
    MapInsertR(&map, 5, 51);
    MapInsertR(&map, 5, 52);

    bool result = MapRemovePair(&map, 5, 51);
    result      = result && MapContainsPair(&map, 5, 50);
    result      = result && !MapContainsPair(&map, 5, 51);
    result      = result && MapContainsPair(&map, 5, 52);
    result      = result && (MapValueCountForKey(&map, 5) == 2);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool remove_even_values(const void *key, const void *value, void *ctx) {
    (void)key;
    (void)ctx;
    return (*(const int *)value % 2) == 0;
}

static bool test_map_remove_if(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInit(i32_hash, i32_compare, &alloc);

    MapInsertR(&map, 1, 10);
    MapInsertR(&map, 1, 11);
    MapInsertR(&map, 2, 20);
    MapInsertR(&map, 3, 31);

    bool result = (MapRemoveIf(&map, remove_even_values, NULL) == 2);
    result      = result && !MapContainsKey(&map, 2);
    result      = result && (MapValueCountForKey(&map, 1) == 1);
    result      = result && MapGetFirstPtr(&map, 1) && (*MapGetFirstPtr(&map, 1) == 11);
    result      = result && MapContainsKey(&map, 3);
    result      = result && (MapPairCount(&map) == 2);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_map_remove_all(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInit(i32_hash, i32_compare, &alloc);

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
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_map_tombstone_reuse(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInit(i32_hash, i32_compare, &alloc);

    for (int i = 0; i < 12; i++) {
        MapSetOnlyR(&map, i, i + 100);
    }

    MapRemoveFirst(&map, 5);
    MapSetOnlyR(&map, 105, 205);

    bool result = !MapContainsKey(&map, 5);
    result      = result && MapContainsKey(&map, 105);
    result      = result && MapGetFirstPtr(&map, 105) && (*MapGetFirstPtr(&map, 105) == 205);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

int main(void) {
    TestFunction tests[] = {
        test_map_remove_value,
        test_map_remove_pair,
        test_map_remove_all,
        test_map_tombstone_reuse,
        test_map_remove_if,
    };

    WriteFmt("[INFO] Starting Map.Remove tests\n\n");
    return run_test_suite(tests, (int)(sizeof(tests) / sizeof(tests[0])), NULL, 0, "Map.Remove");
}
