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

static bool test_map_contains_and_find(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap map = MapInitWithValueCompare(i32_hash, i32_compare, i32_compare, &alloc);

    MapSetOnlyR(&map, 7, 70);
    MapInsertR(&map, 7, 71);
    MapSetOnlyR(&map, 9, 90);

    bool result = MapContainsKey(&map, 7);
    result      = result && MapContainsKey(&map, 9);
    result      = result && !MapContainsKey(&map, 8);
    result      = result && MapContainsPair(&map, 7, 70);
    result      = result && MapContainsPair(&map, 7, 71);
    result      = result && !MapContainsPair(&map, 7, 72);
    result      = result && (MapValueCountForKey(&map, 7) == 2);
    result      = result && (MapValueCountForKey(&map, 9) == 1);
    result      = result && (MapValueCountForKey(&map, 8) == 0);
    result      = result && (MapUniqueKeyCount(&map) == 2);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_map_get_ptr(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap map = MapInit(i32_hash, i32_compare, &alloc);

    MapSetOnlyR(&map, 11, 110);
    MapInsertR(&map, 11, 111);

    int *value  = MapGetFirstPtr(&map, 11);
    bool result = value && (*value == 110);
    result      = result && (MapGetFirstPtr(&map, 999) == NULL);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_map_try_get_ptr(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap map = MapInit(i32_hash, i32_compare, &alloc);

    MapSetOnlyR(&map, 11, 110);
    MapInsertR(&map, 11, 111);

    int *value  = MapTryGetPtr(&map, 11);
    bool result = value && (*value == 110);
    result      = result && (MapTryGetPtr(&map, 999) == NULL);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_map_get_or_default(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap map = MapInit(i32_hash, i32_compare, &alloc);

    MapSetOnlyR(&map, 11, 110);
    MapInsertR(&map, 11, 111);

    int  found  = MapGetOrDefault(&map, 11, 999);
    int  miss   = MapGetOrDefault(&map, 999, 555);
    bool result = (found == 110);
    result      = result && (miss == 555);
    result      = result && !MapContainsKey(&map, 999);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_map_value_cursor_query(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap      map       = MapInit(i32_hash, i32_compare, &alloc);
    MapValueCursor cursor    = MapValueCursorInvalid();
    int            value_sum = 0;
    int            seen      = 0;

    MapInsertR(&map, 4, 40);
    MapInsertR(&map, 4, 41);
    MapInsertR(&map, 4, 42);
    MapInsertR(&map, 9, 90);

    cursor = MapFindFirstForKey(&map, 4);
    while (MapValueCursorIsValid(cursor)) {
        int *value_ptr = MapValuePtrFromCursor(&map, cursor);
        if (!value_ptr) {
            MapDeinit(&map);
            DefaultAllocatorDeinit(&alloc);
            return false;
        }

        value_sum += *value_ptr;
        seen      += 1;
        cursor     = MapFindNextForKey(&map, 4, cursor);
    }

    bool result = (seen == 3) && (value_sum == (40 + 41 + 42));
    result      = result && !MapValueCursorIsValid(MapFindFirstForKey(&map, 99));
    result      = result && (MapValuePtrFromCursor(&map, MapValueCursorInvalid()) == NULL);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_map_cursor_invalidated_after_removal(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap      map    = MapInit(i32_hash, i32_compare, &alloc);
    MapValueCursor cursor = MapValueCursorInvalid();

    MapInsertR(&map, 5, 50);
    MapInsertR(&map, 5, 51);

    cursor = MapFindFirstForKey(&map, 5);
    if (!MapValueCursorIsValid(cursor)) {
        MapDeinit(&map);
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    MapRemoveFirst(&map, 5);

    bool result = (MapValuePtrFromCursor(&map, cursor) == NULL);
    result      = result && (MapValueCountForKey(&map, 5) == 1);
    result      = result && MapGetFirstPtr(&map, 5) && (*MapGetFirstPtr(&map, 5) == 51);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

int main(void) {
    TestFunction tests[] = {
        test_map_contains_and_find,
        test_map_get_ptr,
        test_map_try_get_ptr,
        test_map_get_or_default,
        test_map_value_cursor_query,
        test_map_cursor_invalidated_after_removal,
    };

    WriteFmt("[INFO] Starting Map.Access tests\n\n");
    return run_test_suite(tests, (int)(sizeof(tests) / sizeof(tests[0])), NULL, 0, "Map.Access");
}
