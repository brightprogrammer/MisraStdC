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

// ---------------------------------------------------------------------------
// Map-object validation deadends.
// ---------------------------------------------------------------------------

static bool test_validate_uninitialized_map_fails(void) {
    WriteFmt("Testing ValidateMap on uninitialized map\n");

    Map(int, int) map = {0};
    ValidateMap(&map);

    return false;
}

// Isolates the magic-mismatch check in validate_map: a fully-shaped map
// whose only defect is a corrupted magic word must abort. The map carries
// valid callbacks/allocator/policy, so the only check that can fire is the
// `!MAGIC_MATCHES(...)` branch.
static bool test_validate_map_with_corrupt_magic_fails(void) {
    WriteFmt("Testing ValidateMap with corrupted magic\n");

    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInit(i32_hash, i32_compare, &alloc);

    GENERIC_MAP(&map)->__magic ^= 0x1;
    ValidateMap(&map);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// ---------------------------------------------------------------------------
// Pair / predicate API deadends (map otherwise valid -> already isolated).
// ---------------------------------------------------------------------------

static bool test_map_contains_pair_without_value_compare_fails(void) {
    WriteFmt("Testing MapContainsPair without value comparator\n");

    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInit(i32_hash, i32_compare, &alloc);

    MapContainsPair(&map, 1, 10);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

static bool test_map_remove_pair_without_value_compare_fails(void) {
    WriteFmt("Testing MapRemovePair without value comparator\n");

    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInit(i32_hash, i32_compare, &alloc);

    MapRemovePair(&map, 1, 10);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

static bool test_map_remove_if_without_predicate_fails(void) {
    WriteFmt("Testing MapRemoveIf without predicate\n");

    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInit(i32_hash, i32_compare, &alloc);

    MapRemoveIf(&map, NULL, NULL);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

static bool test_map_retain_if_without_predicate_fails(void) {
    WriteFmt("Testing MapRetainIf without predicate\n");

    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInit(i32_hash, i32_compare, &alloc);

    MapRetainIf(&map, NULL, NULL);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// ---------------------------------------------------------------------------
// Policy validation deadends.
//
// Every test below starts from an OTHERWISE-VALID baseline policy and breaks
// exactly one field/condition, so deleting the matching check in
// validate_map_policy makes the abort disappear (the test would then go red).
// ---------------------------------------------------------------------------

// Valid building blocks mirroring the built-in linear defaults.
static bool valid_should_rehash(u64 length, u64 capacity, u64 tombstones, size pending_inserts, size probe_pressure) {
    (void)length;
    (void)capacity;
    (void)tombstones;
    (void)pending_inserts;
    (void)probe_pressure;
    return true;
}

// A faithful clone of default_next_capacity: passes every snapshot check.
static size valid_next_capacity(u64 length, u64 capacity, u64 tombstones, size min_entries) {
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

static size valid_first_index(u64 hash, size capacity) {
    return capacity ? (size)(hash % capacity) : 0;
}

static size valid_next_index(u64 hash, size capacity, size previous_index, size probe_count) {
    (void)hash;
    (void)probe_count;
    return capacity ? ((previous_index + 1) % capacity) : 0;
}

// Returns a baseline policy that passes validate_map_policy in full.
static MapPolicy valid_baseline_policy(void) {
    MapPolicy policy = {
        .name            = "baseline",
        .should_rehash   = valid_should_rehash,
        .next_capacity   = valid_next_capacity,
        .first_index     = valid_first_index,
        .next_index      = valid_next_index,
        .max_probe_count = 8,
    };
    return policy;
}

// Isolates: `if (!policy) ...`. Pass a NULL pointer directly; every later
// check dereferences policy, so only the NULL guard can fire here.
static bool test_validate_map_policy_null_pointer_fails(void) {
    WriteFmt("Testing validate_map_policy(NULL)\n");

    validate_map_policy(NULL);
    return false;
}

// Isolates: empty-name check. Everything else is valid, so removing the
// name check lets the policy pass cleanly (no later abort).
static bool test_validate_map_policy_without_name_fails(void) {
    WriteFmt("Testing ValidateMapPolicy without name\n");

    MapPolicy policy = valid_baseline_policy();
    policy.name      = "";

    ValidateMapPolicy(policy);
    return false;
}

// Isolates: missing should_rehash callback.
static bool test_validate_map_policy_without_should_rehash_fails(void) {
    WriteFmt("Testing ValidateMapPolicy without should_rehash\n");

    MapPolicy policy     = valid_baseline_policy();
    policy.should_rehash = NULL;

    ValidateMapPolicy(policy);
    return false;
}

// Isolates: missing next_capacity callback.
static bool test_validate_map_policy_without_next_capacity_fails(void) {
    WriteFmt("Testing ValidateMapPolicy without next_capacity\n");

    MapPolicy policy     = valid_baseline_policy();
    policy.next_capacity = NULL;

    ValidateMapPolicy(policy);
    return false;
}

// Isolates: missing first_index callback.
static bool test_validate_map_policy_without_first_index_fails(void) {
    WriteFmt("Testing ValidateMapPolicy without first_index\n");

    MapPolicy policy   = valid_baseline_policy();
    policy.first_index = NULL;

    ValidateMapPolicy(policy);
    return false;
}

// Isolates: missing next_index callback.
static bool test_validate_map_policy_without_next_index_fails(void) {
    WriteFmt("Testing ValidateMapPolicy without next_index\n");

    MapPolicy policy  = valid_baseline_policy();
    policy.next_index = NULL;

    ValidateMapPolicy(policy);
    return false;
}

// Isolates: max_probe_count == 0 check.
static bool test_validate_map_policy_without_probe_limit_fails(void) {
    WriteFmt("Testing ValidateMapPolicy without probe limit\n");

    MapPolicy policy       = valid_baseline_policy();
    policy.max_probe_count = 0;

    ValidateMapPolicy(policy);
    return false;
}

// next_capacity that returns 0 only for the `min_entries == 0` probe
// (the `next0` call). It returns a valid capacity for every other request,
// so the `next_same`/`next_more` checks pass and the ONLY firing check is
// "returned zero capacity for a non-empty snapshot".
static size zero_for_first_nonempty_snapshot(u64 length, u64 capacity, u64 tombstones, size min_entries) {
    if (min_entries == 0)
        return 0;
    return valid_next_capacity(length, capacity, tombstones, min_entries);
}

static bool test_validate_map_policy_zero_capacity_nonempty_fails(void) {
    WriteFmt("Testing ValidateMapPolicy zero-capacity for non-empty snapshot\n");

    MapPolicy policy     = valid_baseline_policy();
    policy.next_capacity = zero_for_first_nonempty_snapshot;

    ValidateMapPolicy(policy);
    return false;
}

// next_capacity that returns a too-small capacity ONLY for the `next_same`
// probe (min_entries == length, length > 0). Every other request gets a
// valid capacity, so zero-capacity and next_more checks pass; only the
// "capacity smaller than current length" check fires.
static size small_for_same_min(u64 length, u64 capacity, u64 tombstones, size min_entries) {
    if (length > 0 && min_entries == (size)length)
        return 1;
    return valid_next_capacity(length, capacity, tombstones, min_entries);
}

static bool test_validate_map_policy_capacity_below_length_fails(void) {
    WriteFmt("Testing ValidateMapPolicy capacity smaller than length\n");

    MapPolicy policy     = valid_baseline_policy();
    policy.next_capacity = small_for_same_min;

    ValidateMapPolicy(policy);
    return false;
}

// next_capacity that returns a too-small capacity ONLY for the `next_more`
// probe (min_entries == length + 1, length > 0). Only the "capacity smaller
// than requested minimum entries" check fires.
static size small_for_more_min(u64 length, u64 capacity, u64 tombstones, size min_entries) {
    if (length > 0 && min_entries == (size)length + 1)
        return 1;
    return valid_next_capacity(length, capacity, tombstones, min_entries);
}

static bool test_validate_map_policy_capacity_below_min_entries_fails(void) {
    WriteFmt("Testing ValidateMapPolicy capacity smaller than min_entries\n");

    MapPolicy policy     = valid_baseline_policy();
    policy.next_capacity = small_for_more_min;

    ValidateMapPolicy(policy);
    return false;
}

// first_index always returns `capacity` (one past the last valid slot).
// Isolates the first_index range check in map_validate_policy_index.
static size out_of_range_first_index(u64 hash, size capacity) {
    (void)hash;
    return capacity;
}

static bool test_validate_map_policy_first_index_out_of_range_fails(void) {
    WriteFmt("Testing ValidateMapPolicy first_index out of range\n");

    MapPolicy policy   = valid_baseline_policy();
    policy.first_index = out_of_range_first_index;

    ValidateMapPolicy(policy);
    return false;
}

// next_index always returns `capacity` (out of range). first_index stays
// valid, so the first_index range check passes; only the next_index range
// check fires.
static size out_of_range_next_index(u64 hash, size capacity, size previous_index, size probe_count) {
    (void)hash;
    (void)previous_index;
    (void)probe_count;
    return capacity;
}

static bool test_validate_map_policy_next_index_out_of_range_fails(void) {
    WriteFmt("Testing ValidateMapPolicy next_index out of range\n");

    MapPolicy policy  = valid_baseline_policy();
    policy.next_index = out_of_range_next_index;

    ValidateMapPolicy(policy);
    return false;
}

// next_index returns the same slot as first_index (in range, but stuck).
// The range check passes; only the stuck-probe (`next == first`) check fires.
static size stuck_next_index(u64 hash, size capacity, size previous_index, size probe_count) {
    (void)hash;
    (void)capacity;
    (void)probe_count;
    return previous_index;
}

static bool test_validate_map_policy_stuck_probe_fails(void) {
    WriteFmt("Testing ValidateMapPolicy stuck probe sequence\n");

    MapPolicy policy  = valid_baseline_policy();
    policy.next_index = stuck_next_index;

    ValidateMapPolicy(policy);
    return false;
}

// Initialising a map with an invalid policy must abort at construction:
// MapInit* routes the policy through validate_map_policy_copy, so a bad policy
// is rejected even when the caller never invokes ValidateMapPolicy directly.
// Isolates the validate_map_policy_copy call inside the init path; removing it
// would let an invalid-policy map be built silently.
static bool test_map_init_with_invalid_policy_fails(void) {
    WriteFmt("Testing MapInitWithPolicy with an invalid policy\n");

    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc  = DefaultAllocatorInit();
    MapPolicy        policy = valid_baseline_policy();
    policy.max_probe_count  = 0; // single broken field

    IntIntMap map = MapInitWithPolicy(i32_hash, i32_compare, policy, &alloc);
    (void)map;
    return false;
}

int main(void) {
    TestFunction deadend_tests[] = {
        test_validate_uninitialized_map_fails,
        test_map_init_with_invalid_policy_fails,
        test_validate_map_with_corrupt_magic_fails,
        test_map_contains_pair_without_value_compare_fails,
        test_map_remove_pair_without_value_compare_fails,
        test_map_remove_if_without_predicate_fails,
        test_map_retain_if_without_predicate_fails,
        test_validate_map_policy_null_pointer_fails,
        test_validate_map_policy_without_name_fails,
        test_validate_map_policy_without_should_rehash_fails,
        test_validate_map_policy_without_next_capacity_fails,
        test_validate_map_policy_without_first_index_fails,
        test_validate_map_policy_without_next_index_fails,
        test_validate_map_policy_without_probe_limit_fails,
        test_validate_map_policy_zero_capacity_nonempty_fails,
        test_validate_map_policy_capacity_below_length_fails,
        test_validate_map_policy_capacity_below_min_entries_fails,
        test_validate_map_policy_first_index_out_of_range_fails,
        test_validate_map_policy_next_index_out_of_range_fails,
        test_validate_map_policy_stuck_probe_fails,
    };

    WriteFmt("[INFO] Starting Map.Deadend tests\n\n");
    return run_test_suite(
        NULL,
        0,
        deadend_tests,
        (int)(sizeof(deadend_tests) / sizeof(deadend_tests[0])),
        "Map.Deadend"
    );
}
