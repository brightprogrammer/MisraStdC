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

// ---------------------------------------------------------------------------
// Probing policy building blocks for the normal validator tests below.
// ---------------------------------------------------------------------------

static size linear_first(u64 hash, size capacity) {
    return capacity ? (size)(hash % capacity) : 0;
}

// Stuck next-index: never advances the probe (next == previous == first).
static size stuck_next(u64 hash, size capacity, size previous_index, size probe_count) {
    (void)hash;
    (void)capacity;
    (void)probe_count;
    return previous_index;
}

static bool load_rehash(u64 length, u64 capacity, u64 tombstones, size pending_inserts, size probe_pressure) {
    (void)probe_pressure;
    if ((length + pending_inserts) == 0)
        return false;
    if (capacity == 0)
        return true;
    return ((length + tombstones + pending_inserts) * 4) >= (capacity * 3);
}

static size pow2_capacity(u64 length, u64 capacity, u64 tombstones, size min_entries) {
    (void)capacity;
    (void)tombstones;
    size needed       = min_entries > length ? min_entries : (size)length;
    size new_capacity = 8;
    if (needed == 0)
        return 0;
    while (((new_capacity * 3) / 4) < needed)
        new_capacity <<= 1;
    return new_capacity;
}

// ===========================================================================
// NORMAL validator tests (real code does NOT abort).
// ===========================================================================

// 412 (validate_map_structural, `length + tombstones` -> `length - tombstones`).
// A structurally valid map may hold MORE tombstones than live entries as long
// as length + tombstones <= capacity. Real code validates cleanly.
static bool test_validate_more_tombstones_than_live_is_valid(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInit(i32_hash, i32_compare, &alloc);

    MapInsertR(&map, 1, 10);
    MapInsertR(&map, 2, 20);
    MapInsertR(&map, 3, 30);

    GenericMap *g           = GENERIC_MAP(&map);
    u64         real_length = g->length;
    u64         real_tombs  = g->tombstones;

    // Valid combo: more tombstones than live entries, sum within capacity.
    g->length      = 2;
    g->tombstones  = 5;
    g->__magic    |= MAGIC_VALIDATED_BIT;

    ValidateMap(&map);

    bool result = true;

    g->length     = real_length;
    g->tombstones = real_tombs;

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// validate_map_policy : (policy->max_probe_count > 1)  (147:56 cxx_gt_to_ge).
// A VALID budget-1 policy with a deliberately stuck next_index: real code skips
// the stuck-probe check (1 > 1 is false) and validates cleanly. The mutant
// (`>= 1`) enters the check and LOG_FATALs.
static bool test_validate_policy_skips_stuck_check_at_probe_budget_one(void) {
    MapPolicy policy = {
        .name            = "stuck-budget-one",
        .should_rehash   = load_rehash,
        .next_capacity   = pow2_capacity,
        .first_index     = linear_first,
        .next_index      = stuck_next,
        .max_probe_count = 1,
    };

    ValidateMapPolicy(policy);
    return true;
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

// 408 (validate_map_structural, removed validate_map_policy(&map->policy)).
// A validated map whose policy has been corrupted (empty name) must abort: the
// structural validator routes the policy through validate_map_policy.
static bool test_validate_corrupt_policy_name_fails(void) {
    WriteFmt("Testing ValidateMap with corrupted policy name\n");

    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInit(i32_hash, i32_compare, &alloc);

    MapInsertR(&map, 1, 10);

    GenericMap *g   = GENERIC_MAP(&map);
    g->policy.name  = ""; // single broken field; everything else valid
    g->__magic     |= MAGIC_VALIDATED_BIT;

    ValidateMap(&map);    // must abort inside validate_map_policy

    return false;
}

// 436 (validate_map, removed validate_map_structural(map) call).
// A map with length > capacity is structurally corrupt and must abort.
static bool test_validate_length_exceeds_capacity_fails(void) {
    WriteFmt("Testing ValidateMap with length exceeding capacity\n");

    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInit(i32_hash, i32_compare, &alloc);

    MapInsertR(&map, 1, 10);          // capacity becomes 8

    GenericMap *g  = GENERIC_MAP(&map);
    g->length      = g->capacity + 1; // structurally impossible
    g->__magic    |= MAGIC_VALIDATED_BIT;

    ValidateMap(&map);                // must abort: "Map length cannot exceed capacity"

    return false;
}

// ---------------------------------------------------------------------------
// Custom policy plumbing for the validate_map_policy self-check kills. Each
// policy is a valid linear policy EXCEPT for a single intentional stuck-probe
// tripwire, exposed only under a specific snapshot/index/hash that the
// corresponding `cxx_init_const` mutant rewrites to 42. Under the real code
// the tripwire fires (LOG_FATAL); under the mutant it is bypassed.
// ---------------------------------------------------------------------------

static bool poly_should_rehash(u64 length, u64 capacity, u64 tombstones, size pending, size probe_pressure) {
    (void)probe_pressure;
    if ((length + pending) == 0)
        return false;
    if (capacity == 0)
        return true;
    return ((length + tombstones + pending) * 4) >= (capacity * 3);
}

static size poly_next_capacity(u64 length, u64 capacity, u64 tombstones, size min_entries) {
    (void)capacity;
    (void)tombstones;
    size needed = min_entries > length ? min_entries : (size)length;
    size cap    = 8;
    if (needed == 0)
        return 0;
    while (((cap * 3) / 4) < needed)
        cap <<= 1;
    return cap;
}

static size poly_first_index(u64 hash, size capacity) {
    return capacity ? (size)(hash % capacity) : 0;
}

static size poly_next_index_linear(u64 hash, size capacity, size previous_index, size probe_count) {
    (void)hash;
    (void)probe_count;
    return capacity ? ((previous_index + 1) % capacity) : 0;
}

// Stuck ONLY at capacity 8. validate_map_policy probes capacities {1,8,17};
// 143:14 (cxx_init_const) rewrites that capacity to 42, so the cap-8 tripwire
// is never exercised and the stuck-probe LOG_FATAL is skipped.
static size poly_next_index_stuck_at_cap8(u64 hash, size capacity, size previous_index, size probe_count) {
    (void)probe_count;
    if (capacity == 8)
        return previous_index; // next == first -> stuck
    return capacity ? ((previous_index + 1) % capacity) : 0;
}

// Stuck for ALL capacities (ignores previous_index, always returns first).
// 145:14 rewrites the captured `first` to 42, so the `next != first` check
// compares against 42 instead of the real first index and passes.
static size poly_next_index_returns_first(u64 hash, size capacity, size previous_index, size probe_count) {
    (void)previous_index;
    (void)probe_count;
    return capacity ? (size)(hash % capacity) : 0;
}

// Stuck ONLY when the hash equals validate_map_policy's golden probe constant.
// 144:14 rewrites that hash to 42, so the tripwire never sees the golden hash.
static size poly_next_index_stuck_at_golden(u64 hash, size capacity, size previous_index, size probe_count) {
    (void)probe_count;
    if (hash == 0x9e3779b97f4a7c15ULL)
        return previous_index; // next == first -> stuck
    return capacity ? ((previous_index + 1) % capacity) : 0;
}

// next_capacity that is sufficient for every snapshot the self-check probes
// (lengths {0,3,5,10}, so `needed` is only ever <= 11) but deliberately
// under-sizes when `needed == 42`. The real self-check never asks for 42, so
// the policy is accepted; 120:14 rewrites the snapshot `length` to 42, which
// makes the loop probe `needed == 42` and the trap trips line-133's
// insufficient-capacity LOG_FATAL -- so under the mutant the (valid) policy is
// spuriously rejected.
static size poly_next_capacity_trap_at_42(u64 length, u64 capacity, u64 tombstones, size min_entries) {
    (void)capacity;
    (void)tombstones;
    size needed = min_entries > length ? min_entries : (size)length;
    if (needed == 0)
        return 0;
    if (needed == 42)
        return 41; // insufficient -- only reachable when length is mutated to 42
    size cap = 8;
    while (((cap * 3) / 4) < needed)
        cap <<= 1;
    return cap;
}

static MapPolicy make_probe_policy(MapPolicyNextIndexFn next_index) {
    MapPolicy p = {
        .name            = "self-check-probe",
        .should_rehash   = poly_should_rehash,
        .next_capacity   = poly_next_capacity,
        .first_index     = poly_first_index,
        .next_index      = next_index,
        .max_probe_count = 8,
    };
    return p;
}

// 120:14 init_const (the self-check's snapshot `length` rewritten to 42).
// CONTRACT: validate_map_policy must accept a policy that is sufficient for
// every snapshot it actually probes. This policy is sufficient for the real
// snapshot lengths but under-sizes at needed==42; the real validator never
// asks 42 and accepts it, while the mutant probes 42 and spuriously aborts.
// Original: init succeeds (test returns true). Mutant: LOG_FATAL during init
// -> a normal test that aborts = mutant killed.
static bool test_self_check_accepts_policy_sufficient_for_real_snapshots(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc  = DefaultAllocatorInit();
    MapPolicy        policy = {
               .name            = "trap-at-42",
               .should_rehash   = poly_should_rehash,
               .next_capacity   = poly_next_capacity_trap_at_42,
               .first_index     = poly_first_index,
               .next_index      = poly_next_index_linear,
               .max_probe_count = 8,
    };
    IntIntMap map    = MapInitWithPolicy(i32_hash, i32_compare, policy, &alloc);
    bool      result = MapInsertR(&map, 1, 10);
    result           = result && MapGetFirstPtr(&map, 1) && (*MapGetFirstPtr(&map, 1) == 10);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// ---------------------------------------------------------------------------
// validate_map_policy self-check deadend kills (143:14 / 144:14 / 145:14).
// Each inits a policy whose stuck-probe tripwire the real self-check catches
// (LOG_FATAL); the matching cxx_init_const mutant rewrites the snapshot value
// that exposes the tripwire to 42, bypassing the abort -> init "succeeds" ->
// the expect-abort deadend FAILS = mutant killed.
// ---------------------------------------------------------------------------

static bool deadend_self_check_stuck_at_cap8(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc  = DefaultAllocatorInit();
    MapPolicy        policy = make_probe_policy(poly_next_index_stuck_at_cap8);
    IntIntMap        map    = MapInitWithPolicy(i32_hash, i32_compare, policy, &alloc); // must LOG_FATAL
    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

static bool deadend_self_check_stuck_at_golden_hash(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc  = DefaultAllocatorInit();
    MapPolicy        policy = make_probe_policy(poly_next_index_stuck_at_golden);
    IntIntMap        map    = MapInitWithPolicy(i32_hash, i32_compare, policy, &alloc); // must LOG_FATAL
    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

static bool deadend_self_check_first_index_compare(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc  = DefaultAllocatorInit();
    MapPolicy        policy = make_probe_policy(poly_next_index_returns_first);
    IntIntMap        map    = MapInitWithPolicy(i32_hash, i32_compare, policy, &alloc); // must LOG_FATAL
    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// ---------------------------------------------------------------------------
// 433:24 cxx_and_to_or in validate_map:
//   if (!(map->__magic & MAGIC_VALIDATED_BIT)) return;
// The `&` becomes `|`. `__magic | MAGIC_VALIDATED_BIT` is always non-zero, so
// `!(non-zero)` is always false: the early-return is never taken and the
// mutant runs validate_map_structural on EVERY entry. Real code clears the
// validated bit after the first validating op (line 437), so a structural
// invariant broken AFTER the first op is never re-checked.
//
// Normal test (returns true): do a first op (clears the bit), then break the
// length<=capacity structural invariant WITHOUT re-arming the bit. Real code
// skips structural and the next validating op proceeds; the always-validate
// mutant re-runs structural, sees length > capacity, and LOG_FATALs. The
// harness counts the abort as a failure of this normal test -> mutant killed.
static bool test_validate_skips_structural_after_first_op(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInit(i32_hash, i32_compare, &alloc);

    bool result = MapInsertR(&map, 7, 70); // first ops clear the validated bit
    result      = result && MapGetFirstPtr(&map, 7) && (*MapGetFirstPtr(&map, 7) == 70);

    // intentional bypass: no public mutator can violate length<=capacity; we
    // poke the field directly to exercise the post-first-op skip. The bit is
    // left CLEARED, so real validate_map returns early and never inspects this.
    GenericMap *g           = GENERIC_MAP(&map);
    u64         real_length = g->length;
    g->length               = g->capacity + 1; // breaks length <= capacity

    // A validating read op. Real: bit cleared -> structural skipped -> ok.
    // Mutant: always validates -> length>capacity -> LOG_FATAL.
    int *v = MapGetFirstPtr(&map, 7);
    result = result && (v != NULL) && (*v == 70);

    // Repair before teardown so Deinit's own validation stays clean.
    g->length = real_length;

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

int main(void) {
    TestFunction tests[] = {
        test_validate_more_tombstones_than_live_is_valid,
        test_validate_policy_skips_stuck_check_at_probe_budget_one,
        test_self_check_accepts_policy_sufficient_for_real_snapshots,
        test_validate_skips_structural_after_first_op,
    };

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
        test_validate_corrupt_policy_name_fails,
        test_validate_length_exceeds_capacity_fails,
        deadend_self_check_stuck_at_cap8,
        deadend_self_check_stuck_at_golden_hash,
        deadend_self_check_first_index_compare,
    };

    WriteFmt("[INFO] Starting Map.Deadend tests\n\n");
    return run_test_suite(
        tests,
        (int)(sizeof(tests) / sizeof(tests[0])),
        deadend_tests,
        (int)(sizeof(deadend_tests) / sizeof(deadend_tests[0])),
        "Map.Deadend"
    );
}
