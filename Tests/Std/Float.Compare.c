#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Container/Float.h>
#include <Misra/Std/Container/Int.h>
#include <Misra/Std/Container/Map.h>
#include <Misra/Std/Log.h>

#include "../Util/FloatTestData.h"
#include "../Util/TestRunner.h"

bool test_float_compare_small_small(void);
bool test_float_compare_very_large_large(void);
bool test_float_compare_very_large_small(void);
bool test_float_compare_wrappers(void);
bool test_float_compare_generic(void);
bool test_float_hash_determinism(void);
bool test_float_hash_distinguishes(void);
bool test_float_hash_as_map_key(void);

bool test_float_compare_small_small(void) {
    WriteFmt("Testing FloatCompare with small floats\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float a = FloatFromStr("1.23", &alloc.base);
    Float b = FloatFromStr("123e-2", &alloc.base);
    Float c = FloatFromStr("-1.23", &alloc.base);

    bool result = FloatCompare(&a, &b) == 0;
    result      = result && FloatEQ(&a, &b);
    result      = result && (FloatCompare(&c, &a) < 0);
    result      = result && (FloatCompare(&a, &c) > 0);

    FloatDeinit(&a);
    FloatDeinit(&b);
    FloatDeinit(&c);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_float_compare_very_large_large(void) {
    WriteFmt("Testing FloatCompare with very large floats\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float a = FloatFromStr(FLOAT_TEST_VERY_LARGE_ONES, &alloc.base);
    Float b = FloatFromStr(FLOAT_TEST_VERY_LARGE_TWOS, &alloc.base);
    Float c = FloatFromStr(FLOAT_TEST_VERY_LARGE_ONES, &alloc.base);

    bool result = FloatLT(&a, &b);
    result      = result && FloatGT(&b, &a);
    result      = result && (FloatCompare(&a, &c) == 0);
    result      = result && FloatEQ(&a, &c);

    FloatDeinit(&a);
    FloatDeinit(&b);
    FloatDeinit(&c);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_float_compare_very_large_small(void) {
    WriteFmt("Testing FloatCompare with very large and small floats\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float large          = FloatFromStr(FLOAT_TEST_VERY_LARGE_ONES, &alloc.base);
    Float negative_large = FloatFromStr("-" FLOAT_TEST_VERY_LARGE_ONES, &alloc.base);
    Float small          = FloatFromStr("2.5", &alloc.base);

    bool result = FloatGT(&large, &small);
    result      = result && FloatLT(&negative_large, &small);
    result      = result && FloatNE(&large, &small);

    FloatDeinit(&large);
    FloatDeinit(&negative_large);
    FloatDeinit(&small);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_float_compare_wrappers(void) {
    WriteFmt("Testing Float compare macros\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float a        = FloatFromStr("-2", &alloc.base);
    Float b        = FloatFromStr("0.5", &alloc.base);
    Float expected = FloatFromStr("5e-1", &alloc.base);

    bool result = FloatLT(&a, &b);
    result      = result && FloatLE(&a, &b);
    result      = result && FloatGT(&b, &a);
    result      = result && FloatGE(&b, &a);
    result      = result && FloatNE(&a, &b);
    result      = result && FloatEQ(&b, &expected);

    FloatDeinit(&a);
    FloatDeinit(&b);
    FloatDeinit(&expected);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_float_compare_generic(void) {
    WriteFmt("Testing FloatCompare generic dispatch\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float value = FloatFromStr("12.5", &alloc.base);
    Float same  = FloatFromStr("12.5", &alloc.base);
    Int   whole = IntFrom(12, &alloc.base);
    Int   next  = IntFrom(13, &alloc.base);

    bool result = (FloatCompare(&value, &same) == 0);
    result      = result && (FloatCompare(&value, &whole) > 0);
    result      = result && (FloatCompare(&value, &next) < 0);
    result      = result && (FloatCompare(&value, 12) > 0);
    result      = result && (FloatCompare(&value, -1) > 0);
    result      = result && (FloatCompare(&value, 12.5f) == 0);
    result      = result && (FloatCompare(&value, 12.5) == 0);
    result      = result && FloatEQ(&value, &same);
    result      = result && FloatEQ(&value, 12.5);
    result      = result && FloatGE(&value, 12.5f);
    result      = result && FloatGT(&value, &whole);
    result      = result && FloatLE(&value, 13);
    result      = result && FloatNE(&value, 12);

    FloatDeinit(&value);
    FloatDeinit(&same);
    IntDeinit(&whole);
    IntDeinit(&next);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Two construction paths for the same value must hash to the same bucket.
bool test_float_hash_determinism(void) {
    WriteFmt("Testing float_hash determinism\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float a = FloatFromStr("1.23", &alloc.base);
    Float b = FloatFromStr("123e-2", &alloc.base);
    Float zero1 = FloatFromStr("0", &alloc.base);
    Float zero2 = FloatFromStr("0", &alloc.base);

    bool result = (float_hash(&a, 0) == float_hash(&b, 0));
    result      = result && (float_hash(&zero1, 0) == float_hash(&zero2, 0));

    FloatDeinit(&a);
    FloatDeinit(&b);
    FloatDeinit(&zero1);
    FloatDeinit(&zero2);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Sign, exponent, and magnitude must each pull the hash apart, so
// +1.5e3 / -1.5e3 / 1.5e2 all land in distinct buckets.
bool test_float_hash_distinguishes(void) {
    WriteFmt("Testing float_hash sensitivity to sign / exponent / magnitude\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float pos  = FloatFromStr("1.5e3", &alloc.base);
    Float neg  = FloatFromStr("-1.5e3", &alloc.base);
    Float small = FloatFromStr("1.5e2", &alloc.base);
    Float zero = FloatFromStr("0", &alloc.base);
    Float one  = FloatFromStr("1", &alloc.base);

    u64 h_pos   = float_hash(&pos, 0);
    u64 h_neg   = float_hash(&neg, 0);
    u64 h_small = float_hash(&small, 0);
    u64 h_zero  = float_hash(&zero, 0);
    u64 h_one   = float_hash(&one, 0);

    bool result = (h_pos != h_neg);     // sign matters
    result      = result && (h_pos != h_small); // exponent matters
    result      = result && (h_neg != h_small);
    result      = result && (h_zero != h_one);
    result      = result && (h_zero != h_pos);

    FloatDeinit(&pos);
    FloatDeinit(&neg);
    FloatDeinit(&small);
    FloatDeinit(&zero);
    FloatDeinit(&one);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// End-to-end: plug float_hash + float_compare into a Map and verify
// the GenericHash / GenericCompare cast through MapInit works.
bool test_float_hash_as_map_key(void) {
    WriteFmt("Testing float_hash as Map<Float, u64> key\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Map(Float, u64) counts = MapInit(float_hash, float_compare, &alloc);

    Float k1 = FloatFromStr("3.14", &alloc.base);
    Float k2 = FloatFromStr("2.71", &alloc.base);
    MapInsertR(&counts, k1, 1u);
    MapInsertR(&counts, k2, 2u);

    Float probe   = FloatFromStr("314e-2", &alloc.base); // same value as k1
    u64  *got     = MapTryGetPtr(&counts, probe);
    Float missing = FloatFromStr("9.99", &alloc.base);
    u64  *gone    = MapTryGetPtr(&counts, missing);

    bool result = (got != NULL && *got == 1u);
    result      = result && (gone == NULL);
    result      = result && (MapPairCount(&counts) == 2);

    FloatDeinit(&k1);
    FloatDeinit(&k2);
    FloatDeinit(&probe);
    FloatDeinit(&missing);
    MapDeinit(&counts);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

int main(void) {
    WriteFmt("[INFO] Starting Float.Compare tests\n\n");

    TestFunction tests[] = {
        test_float_compare_small_small,
        test_float_compare_very_large_large,
        test_float_compare_very_large_small,
        test_float_compare_wrappers,
        test_float_compare_generic,
        test_float_hash_determinism,
        test_float_hash_distinguishes,
        test_float_hash_as_map_key,
    };

    int total_tests = sizeof(tests) / sizeof(tests[0]);
    return run_test_suite(tests, total_tests, NULL, 0, "Float.Compare");
}
