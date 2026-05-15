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

static bool test_map_insert_and_set(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInit(i32_hash, i32_compare, &alloc);

    MapInsertR(&map, 1, 10);
    MapInsertR(&map, 1, 11);
    MapInsertR(&map, 2, 20);
    MapSetOnlyR(&map, 2, 200);
    MapSetOnlyR(&map, 3, 30);

    bool result = MapPairCount(&map) == 4;
    result      = result && (MapValueCountForKey(&map, 1) == 2);
    result      = result && (MapValueCountForKey(&map, 2) == 1);
    result      = result && (MapValueCountForKey(&map, 3) == 1);
    result      = result && MapGetFirstPtr(&map, 1) && (*MapGetFirstPtr(&map, 1) == 10);
    result      = result && MapGetFirstPtr(&map, 2) && (*MapGetFirstPtr(&map, 2) == 200);
    result      = result && MapGetFirstPtr(&map, 3) && (*MapGetFirstPtr(&map, 3) == 30);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_map_set_first(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInitWithValueCompare(i32_hash, i32_compare, i32_compare, &alloc);

    MapInsertR(&map, 1, 10);
    MapInsertR(&map, 1, 11);
    MapInsertR(&map, 1, 12);
    MapSetFirstR(&map, 1, 100);

    bool result = (MapPairCount(&map) == 3);
    result      = result && (MapValueCountForKey(&map, 1) == 3);
    result      = result && MapGetFirstPtr(&map, 1) && (*MapGetFirstPtr(&map, 1) == 100);
    result      = result && MapContainsPair(&map, 1, 11);
    result      = result && MapContainsPair(&map, 1, 12);
    result      = result && !MapContainsPair(&map, 1, 10);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_map_lvalue_zeroing(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInit(i32_hash, i32_compare, &alloc);
    int              key   = 42;
    int              value = 84;

    MapInsertL(&map, key, value);

    bool result = (key == 0) && (value == 0);
    result      = result && (MapValueCountForKey(&map, 42) == 1);
    result      = result && MapGetFirstPtr(&map, 42) && (*MapGetFirstPtr(&map, 42) == 84);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Heavy insert+remove churn at near-threshold load. With linear probing
// limited to `max_probe_count` slots, a long collision cluster can force
// scan_slots to fail even though empty slots exist; the prior fix was to
// force `next_capacity` to grow on that path (rehashing at the same size
// would re-probe the same cluster and loop forever).
static bool test_map_churn_does_not_loop(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc  = DefaultAllocatorInit();
    IntIntMap        map    = MapInit(i32_hash, i32_compare, &alloc);
    bool             result = true;

    // Phase 1: fill past the first few growth thresholds.
    for (int i = 0; i < 600; ++i) {
        MapInsertR(&map, i, i * 10);
    }
    result = result && (MapPairCount(&map) == 600);

    // Phase 2: alternating remove+insert at the same churn point. Each
    // cycle exercises rehash + scan with tombstones present, which is
    // what previously triggered the runaway recursion.
    for (int i = 0; i < 4000; ++i) {
        int key = 600 + (i & 0x3f); // small cycling window
        MapRemoveAll(&map, key);
        MapInsertR(&map, key, i);
    }
    result = result && (MapPairCount(&map) >= 600);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_map_ensure_ptr(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInit(i32_hash, i32_compare, &alloc);
    int             *value_ptr;
    bool             result;

    value_ptr = MapEnsurePtr(&map, 8, 80);
    result    = value_ptr && (*value_ptr == 80);
    result    = result && (MapPairCount(&map) == 1);
    result    = result && (MapValueCountForKey(&map, 8) == 1);

    value_ptr = MapGetOrInsertPtr(&map, 8, 800);
    result    = result && value_ptr && (*value_ptr == 80);
    result    = result && (MapPairCount(&map) == 1);
    result    = result && (MapValueCountForKey(&map, 8) == 1);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

int main(void) {
    TestFunction tests[] = {
        test_map_insert_and_set,
        test_map_set_first,
        test_map_lvalue_zeroing,
        test_map_ensure_ptr,
        test_map_churn_does_not_loop,
    };

    WriteFmt("[INFO] Starting Map.Insert tests\n\n");
    return run_test_suite(tests, (int)(sizeof(tests) / sizeof(tests[0])), NULL, 0, "Map.Insert");
}
