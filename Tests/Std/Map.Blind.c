#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Container/Map.h>
#include <Misra/Std/Log.h>
#include "../Util/TestRunner.h"

// ---------------------------------------------------------------------------
// Shared key callbacks. A mixing hash for general use plus i32 comparison.
// ---------------------------------------------------------------------------
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

// ===========================================================================
// default_should_rehash load-factor growth (lines 19 / 27).
// The default linear policy grows the probe table the first time an insert
// would push the load factor to >= 3/4. With capacity 8 that boundary lands
// when pre-insert length reaches 6 (insert #7 takes capacity 8 -> 16). These
// tests pin the *exact* capacity, which the behavioural suite never asserts.
// ===========================================================================

// 27:50 cxx_mul_to_div: `(length+tomb+pending) * 4` -> `/ 4`. The mutant makes
// the comparison `small/4 >= cap*3` essentially never true, so no preemptive
// growth happens and the table stays at capacity 8 across the first 7 inserts.
// Real code is at capacity 16 by the 7th insert.
static bool test_default_grow_uses_multiply_not_divide(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInit(i32_hash, i32_compare, &alloc);

    for (int k = 0; k < 7; k++)
        MapInsertR(&map, k, k);

    bool result = (MapCapacity(&map) == 16);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 27:55 cxx_ge_to_gt: the load-factor test `>=` -> `>`. Differs only at exact
// equality `(length+tomb+pending)*4 == cap*3`. The length-3 / tombstones-2 /
// capacity-8 state hits 24 == 24 on the next insert: real code rehashes (>=)
// and clears the 2 tombstones; the mutant (>) skips it and the tombstones
// survive.
static bool test_default_rehash_inclusive_at_boundary(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInit(i32_hash, i32_compare, &alloc);

    for (int k = 0; k < 5; k++)
        MapInsertR(&map, k, k);
    MapRemoveAll(&map, 1);
    MapRemoveAll(&map, 3);
    bool result = (MapTombstones(&map) == 2);

    MapInsertR(&map, 200, 200); // exactly on the 3/4 boundary -> must rehash

    result = result && (MapTombstones(&map) == 0);
    result = result && MapGetFirstPtr(&map, 200) && (*MapGetFirstPtr(&map, 200) == 200);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// ===========================================================================
// default_next_capacity doubling loop (line 54).
// ===========================================================================

// 54:37 cxx_lt_to_le: the doubling guard `< needed` -> `<= needed`. The mutant
// loops one extra time exactly when `(cap*3)/4 == needed`. For the 6th insert
// needed is 6 and (8*3)/4 == 6, so real code returns capacity 8 while the
// mutant doubles once more to 16.
static bool test_next_capacity_doubling_strict_less(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInit(i32_hash, i32_compare, &alloc);

    for (int k = 0; k < 6; k++)
        MapInsertR(&map, k, k);

    bool result = (MapCapacity(&map) == 8);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// ===========================================================================
// map_insert_raw_entry tombstone reuse (line 386).
// ===========================================================================

// 386:25 cxx_sub_assign_to_add_assign: `map->tombstones -= 1` -> `+= 1` when a
// raw re-insert lands on a tombstone slot. Exercised by MapSetOnly on an
// existing single-valued key: it saves the entry, removes it (creating one
// tombstone), then raw-reinserts into that same tombstone slot. Real code
// decrements the tombstone count back to 0; the mutant inflates it.
static bool test_setonly_raw_reinsert_decrements_tombstones(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInitWithValueCompare(i32_hash, i32_compare, i32_compare, &alloc);

    MapInsertR(&map, 7, 70);
    bool result = (MapTombstones(&map) == 0);

    MapSetOnlyR(&map, 7, 700); // remove-all (tomb=1) then raw reinsert into the tombstone

    result = result && (MapTombstones(&map) == 0);
    result = result && (MapPairCount(&map) == 1);
    result = result && (MapValueCountForKey(&map, 7) == 1);
    result = result && MapGetFirstPtr(&map, 7) && (*MapGetFirstPtr(&map, 7) == 700);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// ===========================================================================
// map_insert probe-budget recovery forced_n (line 984).
// ===========================================================================

static bool grow_only_from_empty(u64 length, u64 capacity, u64 tombstones, size pending_inserts, size probe_pressure) {
    (void)length;
    (void)tombstones;
    (void)pending_inserts;
    (void)probe_pressure;
    return capacity == 0;
}

static size doubling_next_capacity(u64 length, u64 capacity, u64 tombstones, size min_entries) {
    size new_capacity = 8;
    size needed       = min_entries > length ? min_entries : (size)length;
    (void)tombstones;
    (void)capacity;
    if (needed == 0)
        return 0;
    while (((new_capacity * 3) / 4) < needed)
        new_capacity <<= 1;
    return new_capacity;
}

static size linear_first(u64 hash, size capacity) {
    return capacity ? (size)(hash % capacity) : 0;
}

static size linear_next(u64 hash, size capacity, size previous_index, size probe_count) {
    (void)hash;
    (void)probe_count;
    return capacity ? ((previous_index + 1) % capacity) : 0;
}

static MapPolicy fill_then_grow_policy(void) {
    MapPolicy policy = {
        .name            = "fill-then-grow",
        .should_rehash   = grow_only_from_empty,
        .next_capacity   = doubling_next_capacity,
        .first_index     = linear_first,
        .next_index      = linear_next,
        .max_probe_count = 16,
    };
    return policy;
}

// 984:14 cxx_init_const: `forced_n = map->capacity + 1` -> `forced_n = 42`. The
// recovery path forces a regrow when the probe budget is exhausted. With a
// non-preemptive policy the table fills to capacity 8 (length 8); the 9th
// insert exhausts the budget and recovers by rehashing with n = capacity + 1
// = 9, which yields capacity 16. The const-42 mutant forces n = 42, blowing
// the table up to capacity 64. The behavioural suite checks retrievability but
// not the resulting capacity.
static bool test_probe_recovery_forced_n_is_capacity_plus_one(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc  = DefaultAllocatorInit();
    MapPolicy        policy = fill_then_grow_policy();
    IntIntMap        map    = MapInitWithPolicy(i32_hash, i32_compare, policy, &alloc);

    for (int k = 0; k < 8; k++)
        MapInsertR(&map, k, k * 100 + 3);
    bool result = (MapCapacity(&map) == 8) && (MapPairCount(&map) == 8);

    MapInsertR(&map, 8, 803); // probe budget exhausted -> recover with n = cap+1 = 9

    result = result && (MapCapacity(&map) == 16);
    result = result && (MapPairCount(&map) == 9);
    for (int k = 0; k < 9; k++)
        result = result && MapGetFirstPtr(&map, k) && (*MapGetFirstPtr(&map, k) == k * 100 + 3);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

int main(void) {
    TestFunction tests[] = {
        test_default_grow_uses_multiply_not_divide,
        test_default_rehash_inclusive_at_boundary,
        test_next_capacity_doubling_strict_less,
        test_setonly_raw_reinsert_decrements_tombstones,
        test_probe_recovery_forced_n_is_capacity_plus_one,
    };
    TestFunction deadend_tests[] = {0};
    (void)deadend_tests;

    WriteFmt("[INFO] Starting Map.Blind tests\n\n");
    return run_test_suite(tests, (int)(sizeof(tests) / sizeof(tests[0])), deadend_tests, 0, "Map.Blind");
}
