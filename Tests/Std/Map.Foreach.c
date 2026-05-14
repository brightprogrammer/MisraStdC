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

static bool test_map_foreach_ptr(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap map       = MapInit(i32_hash, i32_compare, &alloc);
    int       key_sum   = 0;
    int       value_sum = 0;

    for (int i = 1; i <= 4; i++) {
        MapSetOnlyR(&map, i, i * 10);
    }
    MapInsertR(&map, 2, 25);

    MapForeachPairPtr(&map, key_ptr, value_ptr) {
        key_sum   += *key_ptr;
        value_sum += *value_ptr;
    }

    bool result = (key_sum == 12) && (value_sum == 125);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_map_foreach_multimap_iterators(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap map            = MapInit(i32_hash, i32_compare, &alloc);
    int       unique_key_sum = 0;
    int       all_value_sum  = 0;
    int       key_two_sum    = 0;

    MapInsertR(&map, 1, 10);
    MapInsertR(&map, 1, 11);
    MapInsertR(&map, 2, 20);
    MapInsertR(&map, 2, 21);
    MapInsertR(&map, 3, 30);

    MapForeachKey(&map, key) {
        unique_key_sum += key;
    }

    MapForeachValuePtrForKey(&map, 2, value_ptr) {
        *value_ptr += 100;
    }

    MapForeachValueForKey(&map, 2, value) {
        key_two_sum += value;
    }

    MapForeachValue(&map, value) {
        all_value_sum += value;
    }

    bool result = (unique_key_sum == 6);
    result      = result && (key_two_sum == 241);
    result      = result && (all_value_sum == 292);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

int main(void) {
    TestFunction tests[] = {
        test_map_foreach_ptr,
        test_map_foreach_multimap_iterators,
    };

    WriteFmt("[INFO] Starting Map.Foreach tests\n\n");
    return run_test_suite(tests, (int)(sizeof(tests) / sizeof(tests[0])), NULL, 0, "Map.Foreach");
}
