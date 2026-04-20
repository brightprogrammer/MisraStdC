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

static bool test_map_foreach_ptr(void) {
    typedef Map(int, int) IntIntMap;
    IntIntMap map = MapInit(int_hash, int_compare);
    int key_sum = 0;
    int value_sum = 0;

    for (int i = 1; i <= 4; i++) {
        MapSetR(&map, i, i * 10);
    }

    MapForeachPtr(&map, key_ptr, value_ptr) {
        key_sum += *key_ptr;
        value_sum += *value_ptr;
    }

    bool result = (key_sum == 10) && (value_sum == 100);

    MapDeinit(&map);
    return result;
}

static bool test_map_foreach_values(void) {
    typedef Map(int, int) IntIntMap;
    IntIntMap map = MapInit(int_hash, int_compare);
    int seen = 0;

    for (int i = 0; i < 3; i++) {
        MapSetR(&map, i, i + 1);
    }

    MapForeach(&map, key, value) {
        seen += key + value;
    }

    bool result = (seen == 9);

    MapDeinit(&map);
    return result;
}

int main(void) {
    TestFunction tests[] = {
        test_map_foreach_ptr,
        test_map_foreach_values,
    };

    WriteFmt("[INFO] Starting Map.Foreach tests\n\n");
    return run_test_suite(tests, (int)(sizeof(tests) / sizeof(tests[0])), NULL, 0, "Map.Foreach");
}
