#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Container/Map.h>
#include <Misra/Std/Log.h>
#include "../../Util/TestRunner.h"

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
    DefaultAllocator alloc     = DefaultAllocatorInit();
    IntIntMap        map       = MapInit(i32_hash, i32_compare, &alloc);
    int              key_sum   = 0;
    int              value_sum = 0;

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
    DefaultAllocator alloc          = DefaultAllocatorInit();
    IntIntMap        map            = MapInit(i32_hash, i32_compare, &alloc);
    int              unique_key_sum = 0;
    int              all_value_sum  = 0;
    int              key_two_sum    = 0;

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

// Contract: MapForeachPair binds key_var/value_var to COPIES of each
// stored pair. Exact key/value sums must match, and mutating the locals
// must NOT write back into the map.
static bool test_map_foreach_pair_by_value(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc     = DefaultAllocatorInit();
    IntIntMap        map       = MapInit(i32_hash, i32_compare, &alloc);
    int              key_sum   = 0;
    int              value_sum = 0;

    for (int i = 1; i <= 4; i++)
        MapSetOnlyR(&map, i, i * 10);
    MapInsertR(&map, 2, 25);

    MapForeachPair(&map, key, value) {
        key_sum   += key;
        value_sum += value;
        // Mutate the by-value locals; this must not touch the map.
        key   += 1000;
        value += 1000;
    }

    bool result = (key_sum == 12) && (value_sum == 125);

    // Re-iterate to confirm the map is unchanged by the local mutation.
    int key_sum_again   = 0;
    int value_sum_again = 0;
    MapForeachPair(&map, key, value) {
        key_sum_again   += key;
        value_sum_again += value;
    }
    result = result && (key_sum_again == 12) && (value_sum_again == 125);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Contract: MapForeachValuePtr binds value_ptr to the in-slot value
// address. Writing through it must persist into the map.
static bool test_map_foreach_value_ptr(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInit(i32_hash, i32_compare, &alloc);

    MapInsertR(&map, 1, 10);
    MapInsertR(&map, 1, 11);
    MapInsertR(&map, 2, 20);
    MapInsertR(&map, 3, 30);

    int before_sum = 0;
    MapForeachValuePtr(&map, value_ptr) {
        before_sum += *value_ptr;
        *value_ptr += 5; // mutate every value in place
    }

    int after_sum = 0;
    MapForeachValue(&map, value) {
        after_sum += value;
    }

    bool result = (before_sum == (10 + 11 + 20 + 30));
    // 4 values, each bumped by 5 -> +20.
    result = result && (after_sum == before_sum + 20);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Contract: EVERY foreach variant skips its body when the map is empty.
// Each counter must stay 0.
static bool test_map_foreach_empty_skips_body(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInit(i32_hash, i32_compare, &alloc);

    int n = 0;

    MapForeachPairPtr(&map, kp, vp) {
        (void)kp;
        (void)vp;
        n += 1;
    }
    MapForeachPair(&map, k, v) {
        (void)k;
        (void)v;
        n += 1;
    }
    MapForeachKey(&map, k) {
        (void)k;
        n += 1;
    }
    MapForeachValue(&map, v) {
        (void)v;
        n += 1;
    }
    MapForeachValuePtr(&map, vp) {
        (void)vp;
        n += 1;
    }
    MapForeachValueForKey(&map, 7, v) {
        (void)v;
        n += 1;
    }
    MapForeachValuePtrForKey(&map, 7, vp) {
        (void)vp;
        n += 1;
    }

    bool result = (n == 0);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

int main(void) {
    TestFunction tests[] = {
        test_map_foreach_ptr,
        test_map_foreach_multimap_iterators,
        test_map_foreach_pair_by_value,
        test_map_foreach_value_ptr,
        test_map_foreach_empty_skips_body,
    };

    WriteFmt("[INFO] Starting Map.Foreach tests\n\n");
    return run_test_suite(tests, (int)(sizeof(tests) / sizeof(tests[0])), NULL, 0, "Map.Foreach");
}
