#include <Misra/Std/Container/Map.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Memory.h>
#include <Misra/Std/Log.h>
#include "../Util/TestRunner.h"

static u64 zstr_hash(const void *data, u32 size) {
    const char *str = *(const char *const *)data;
    const unsigned char *ptr = (const unsigned char *)str;
    u64 hash = 1469598103934665603ULL;
    (void)size;

    while (*ptr) {
        hash ^= (u64)(*ptr++);
        hash *= 1099511628211ULL;
    }

    return hash;
}

static i32 zstr_compare_ptr(const void *lhs, const void *rhs) {
    const char *a = *(const char *const *)lhs;
    const char *b = *(const char *const *)rhs;
    return ZstrCompare(a, b);
}

static bool test_map_deep_copy_zstrs(void) {
    typedef Map(const char *, const char *) ZstrMap;
    ZstrMap map = MapInitWithDeepCopy(
        zstr_hash,
        zstr_compare_ptr,
        ZstrInitClone,
        ZstrDeinit,
        ZstrInitClone,
        ZstrDeinit
    );
    char key_buf[] = "alpha";
    char value_buf[] = "first";
    const char *key = key_buf;
    const char *value = value_buf;
    const char **stored_value;

    MapInsertL(&map, key, value);

    bool result = (key == key_buf) && (value == value_buf);
    key_buf[0] = 'o';
    value_buf[0] = 'w';

    result = result && MapContains(&map, "alpha");
    result = result && !MapContains(&map, key);
    stored_value = MapGetPtr(&map, "alpha");
    result = result && stored_value && (*stored_value != value) && (ZstrCompare(*stored_value, "first") == 0);

    MapDeinit(&map);
    return result;
}

static bool test_map_policy_switch_preserves_entries(void) {
    typedef Map(const char *, const char *) ZstrMap;
    ZstrMap map = MapInitWithDeepCopy(
        zstr_hash,
        zstr_compare_ptr,
        ZstrInitClone,
        ZstrDeinit,
        ZstrInitClone,
        ZstrDeinit
    );

    MapSetR(&map, "red", "apple");
    MapSetR(&map, "yellow", "banana");
    MapSetR(&map, "green", "pear");
    MapRehashWithPolicy(&map, MapLen(&map), MisraMapPolicyQuadratic);

    bool result = MapPolicyGet(&map).probe_index == MisraMapPolicyQuadratic.probe_index;
    result = result && MapGetPtr(&map, "red") && (ZstrCompare(*MapGetPtr(&map, "red"), "apple") == 0);
    result = result && MapGetPtr(&map, "yellow") && (ZstrCompare(*MapGetPtr(&map, "yellow"), "banana") == 0);
    result = result && MapGetPtr(&map, "green") && (ZstrCompare(*MapGetPtr(&map, "green"), "pear") == 0);

    MapDeinit(&map);
    return result;
}

int main(void) {
    TestFunction tests[] = {
        test_map_deep_copy_zstrs,
        test_map_policy_switch_preserves_entries,
    };

    WriteFmt("[INFO] Starting Map.Ops tests\n\n");
    return run_test_suite(tests, (int)(sizeof(tests) / sizeof(tests[0])), NULL, 0, "Map.Ops");
}
