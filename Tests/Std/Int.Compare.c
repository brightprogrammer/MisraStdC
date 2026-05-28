#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Container/Int.h>
#include <Misra/Std/Container/Map.h>
#include <Misra/Std/Log.h>
#include <Misra/Types.h>

#include "../Util/TestRunner.h"

bool test_int_compare(void);
bool test_int_compare_wrappers(void);
bool test_int_compare_generic(void);
bool test_int_hash_determinism(void);
bool test_int_hash_distinguishes(void);
bool test_int_hash_as_map_key(void);

bool test_int_compare(void) {
    WriteFmt("Testing IntCompare\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int a = IntFrom(41, &alloc.base);
    Int b = IntFrom(42, &alloc.base);
    Int c = IntFromBinary("000101010", &alloc.base);

    bool result = IntCompare(&a, &b) < 0;
    result      = result && (IntCompare(&b, &a) > 0);
    result      = result && (IntCompare(&b, &c) == 0);

    IntDeinit(&a);
    IntDeinit(&b);
    IntDeinit(&c);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_compare_wrappers(void) {
    WriteFmt("Testing Int compare wrappers\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int a = IntFrom(41, &alloc.base);
    Int b = IntFrom(42, &alloc.base);
    Int c = IntFromBinary("000101010", &alloc.base);

    bool result = IntLT(&a, &b);
    result      = result && IntLE(&a, &b);
    result      = result && IntGT(&b, &a);
    result      = result && IntGE(&b, &a);
    result      = result && IntEQ(&b, &c);
    result      = result && IntGE(&b, &c);
    result      = result && IntLE(&b, &c);
    result      = result && IntNE(&a, &b);

    IntDeinit(&a);
    IntDeinit(&b);
    IntDeinit(&c);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_compare_generic(void) {
    WriteFmt("Testing IntCompare generic dispatch\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value = IntFrom(42, &alloc.base);
    Int same  = IntFromBinary("00101010", &alloc.base);
    Int big   = IntFrom(1, &alloc.base);

    IntShiftLeft(&big, 80);

    bool result = (IntCompare(&value, &same) == 0);
    result      = result && (IntCompare(&value, &same) == 0);
    result      = result && (IntCompare(&value, 42) == 0);
    result      = result && (IntCompare(&value, 100ULL) < 0);
    result      = result && (IntCompare(&value, -1) > 0);
    result      = result && (IntCompare(&big, UINT64_MAX) > 0);
    result      = result && IntEQ(&value, 42);
    result      = result && IntLE(&value, 42);
    result      = result && IntGT(&value, -1);
    result      = result && IntLT(&value, 100ULL);
    result      = result && IntNE(&value, 43);

    IntDeinit(&value);
    IntDeinit(&same);
    IntDeinit(&big);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Same magnitude built two ways must hash to the same bucket.
bool test_int_hash_determinism(void) {
    WriteFmt("Testing int_hash determinism\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int a = IntFrom(42u, &alloc.base);
    Int b = IntFromBinary("101010", &alloc.base);
    Int c = IntFrom(0u, &alloc.base);
    Int d = IntFrom(0u, &alloc.base);

    bool result = (int_hash(&a, 0) == int_hash(&b, 0));
    result      = result && (int_hash(&c, 0) == int_hash(&d, 0));

    IntDeinit(&a);
    IntDeinit(&b);
    IntDeinit(&c);
    IntDeinit(&d);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Distinct values should land in distinct buckets for the typical
// range. Collisions are inevitable in general, but a 64-bit FNV-1a
// over a handful of small distinct inputs should not clash here.
bool test_int_hash_distinguishes(void) {
    WriteFmt("Testing int_hash distinguishes distinct values\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int zero    = IntFrom(0u, &alloc.base);
    Int one     = IntFrom(1u, &alloc.base);
    Int small   = IntFrom(42u, &alloc.base);
    Int large   = IntFrom(1u, &alloc.base);
    IntShiftLeft(&large, 80);
    Int decimal = IntFromStr("12345678901234567890", &alloc.base);

    u64 h_zero    = int_hash(&zero, 0);
    u64 h_one     = int_hash(&one, 0);
    u64 h_small   = int_hash(&small, 0);
    u64 h_large   = int_hash(&large, 0);
    u64 h_decimal = int_hash(&decimal, 0);

    bool result = (h_zero != h_one);
    result      = result && (h_one != h_small);
    result      = result && (h_small != h_large);
    result      = result && (h_large != h_decimal);
    result      = result && (h_zero != h_decimal);

    IntDeinit(&zero);
    IntDeinit(&one);
    IntDeinit(&small);
    IntDeinit(&large);
    IntDeinit(&decimal);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// End-to-end: plug int_hash + int_compare into a Map and verify the
// GenericHash / GenericCompare cast at the call site works.
bool test_int_hash_as_map_key(void) {
    WriteFmt("Testing int_hash as Map<Int,u64> key\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Map(Int, u64) counts = MapInit(int_hash, int_compare, &alloc);

    Int k1 = IntFrom(100u, &alloc.base);
    Int k2 = IntFrom(200u, &alloc.base);
    Int k3 = IntFrom(100u, &alloc.base); // duplicate of k1 by value

    MapInsertR(&counts, k1, 1u);
    MapInsertR(&counts, k2, 2u);

    Int    probe   = IntFrom(100u, &alloc.base);
    u64   *got     = MapGetFirstPtr(&counts, probe);
    Int    missing = IntFrom(999u, &alloc.base);
    u64   *gone    = MapGetFirstPtr(&counts, missing);

    bool result = (got != NULL && *got == 1u);
    result      = result && (gone == NULL);
    result      = result && (MapPairCount(&counts) == 2);

    IntDeinit(&k1);
    IntDeinit(&k2);
    IntDeinit(&k3);
    IntDeinit(&probe);
    IntDeinit(&missing);
    MapDeinit(&counts);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

int main(void) {
    WriteFmt("[INFO] Starting Int.Compare tests\n\n");

    TestFunction tests[] = {
        test_int_compare,
        test_int_compare_wrappers,
        test_int_compare_generic,
        test_int_hash_determinism,
        test_int_hash_distinguishes,
        test_int_hash_as_map_key,
    };

    int total_tests = sizeof(tests) / sizeof(tests[0]);
    return run_test_suite(tests, total_tests, NULL, 0, "Int.Compare");
}
