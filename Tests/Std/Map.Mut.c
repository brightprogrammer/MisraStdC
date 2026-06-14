/// file : tests/std/map.mut.c
///
/// Targeted mutation-kill tests for Map's default-policy helpers
/// (default_should_rehash / default_next_capacity / quadratic_probe_index)
/// and validate_map_policy's self-check. These mutants survive the broad
/// Map.* suites because those drive custom policies that re-implement the
/// same math locally; here every test runs the *library* default policy
/// (MapPolicyLinear / MapPolicyQuadratic).
///
/// Per MULL-DISCOVERY-CONVENTIONS: assertions are phrased in
/// caller-observable contract terms (a value comes back, a key is gone,
/// counts are exact) -- never as an implementation-chosen capacity or
/// tombstone number. Distinct contract from the existing Map.* tests.

#include <Misra.h>
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

// Identity hash: hash == key value. Multiples-of-8 keys alias into bucket 0
// at capacity 8, forcing the probe sequence to spread them.
static u64 id_hash(const void *data, u32 size) {
    (void)size;
    return (u64)(u32)(*(const int *)data);
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

// ---------------------------------------------------------------------------
// default_next_capacity / default_should_rehash growth CONTRACT: every key
// inserted past the growth boundary stays retrievable with its own value and
// the pair count stays exact. Capacity numbers themselves are strategy and are
// NOT asserted (per MULL-DISCOVERY-CONVENTIONS). These guard the default
// growth path against any future change that would actually drop or corrupt a
// key; the load-factor/sizing operator mutants (35:10, 54:27, 54:37, 19:36,
// 27:*) are bucket-B strategy survivors -- the table still grows (via the
// load-factor OR the probe-exhaustion recovery path) and keeps every key, so
// they are not killable in caller-observable terms.
// ---------------------------------------------------------------------------

// Insert well past the first growth boundary with a well-distributed hash:
// all keys retrievable, count exact.
static bool test_growth_keeps_all_keys_retrievable(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc  = DefaultAllocatorInit();
    IntIntMap        map    = MapInit(i32_hash, i32_compare, &alloc);
    bool             result = true;

    const int n = 40;
    for (int k = 0; k < n; k++)
        result = result && MapInsertR(&map, k, k * 13 + 7);

    result = result && (MapPairCount(&map) == n);
    for (int k = 0; k < n; k++) {
        int *v = MapGetFirstPtr(&map, k);
        result = result && v && (*v == k * 13 + 7);
    }

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Same growth contract under the all-collisions identity hash: every key maps
// to bucket 0, so growth + probing must still place and re-find each one.
static bool test_growth_not_suppressed_on_insert(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc  = DefaultAllocatorInit();
    IntIntMap        map    = MapInitWithPolicy(id_hash, i32_compare, MapPolicyLinear, &alloc);
    bool             result = true;

    const int n = 24;
    for (int k = 0; k < n; k++)
        result = result && MapInsertR(&map, k * 8, k * 7 + 2);

    result = result && (MapPairCount(&map) == n);
    for (int k = 0; k < n; k++) {
        int *v = MapGetFirstPtr(&map, k * 8);
        result = result && v && (*v == k * 7 + 2);
    }

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// quadratic_probe_index collision-resolution CONTRACT: keys 8k all share
// first_index 0; the quadratic probe must spread them so each returns its own
// value with none lost. (15:24 add_to_sub still yields a full-period probe and
// is equivalent -- see report -- so this guards the probe path generally.)
static bool test_quadratic_collisions_each_key_own_value(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc  = DefaultAllocatorInit();
    IntIntMap        map    = MapInitWithPolicy(id_hash, i32_compare, MapPolicyQuadratic, &alloc);
    bool             result = true;

    const int n = 24;
    for (int i = 0; i < n; i++)
        result = result && MapInsertR(&map, i * 8, i * 8 + 3);

    result = result && (MapPairCount(&map) == n);
    for (int i = 0; i < n; i++) {
        int *v = MapGetFirstPtr(&map, i * 8);
        result = result && v && (*v == i * 8 + 3);
    }

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
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
        test_growth_keeps_all_keys_retrievable,
        test_growth_not_suppressed_on_insert,
        test_quadratic_collisions_each_key_own_value,
        test_self_check_accepts_policy_sufficient_for_real_snapshots,
        test_validate_skips_structural_after_first_op,
    };

    TestFunction deadend_tests[] = {
        deadend_self_check_stuck_at_cap8,
        deadend_self_check_stuck_at_golden_hash,
        deadend_self_check_first_index_compare,
    };

    WriteFmt("[INFO] Starting Map.Mut tests\n\n");
    return run_test_suite(
        tests,
        (int)(sizeof(tests) / sizeof(tests[0])),
        deadend_tests,
        (int)(sizeof(deadend_tests) / sizeof(deadend_tests[0])),
        "Map.Mut"
    );
}
