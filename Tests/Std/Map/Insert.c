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

// Identity hash: the hash IS the key value. Multiples-of-8 keys then alias
// into one bucket at capacity 8 but spread across two buckets at capacity 16.
static u64 i32_identity_hash(const void *data, u32 size) {
    (void)size;
    return (u64)(u32)(*(const int *)data);
}

// Identity-ish hash on the raw int value; collisions depend on key % capacity.
static u64 id_hash(const void *data, u32 size) {
    (void)size;
    return (u64)(u32)(*(const int *)data);
}

// ---------------------------------------------------------------------------
// Custom probing policies (rehash_map / reserve_map mutants).
// ---------------------------------------------------------------------------

static bool policy_should_rehash(u64 length, u64 capacity, u64 tombstones, size pending_inserts, size probe_pressure) {
    if ((length + pending_inserts) == 0)
        return false;
    if (capacity == 0)
        return true;
    if (((length + tombstones + pending_inserts) * 4) >= (capacity * 3))
        return true;
    return probe_pressure > 0 && (probe_pressure * 4) >= capacity;
}

static size policy_next_capacity_doubling(u64 length, u64 capacity, u64 tombstones, size min_entries) {
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

static size policy_next_capacity_tight(u64 length, u64 capacity, u64 tombstones, size min_entries) {
    (void)capacity;
    (void)tombstones;
    return min_entries > length ? min_entries : (size)length;
}

static size policy_next_capacity_sabotage(u64 length, u64 capacity, u64 tombstones, size min_entries) {
    (void)capacity;
    (void)tombstones;
    if (min_entries > 0 && min_entries < length)
        return (size)length - 1;
    return min_entries > length ? min_entries : (size)length;
}

static size policy_first_index_linear(u64 hash, size capacity) {
    return capacity ? (size)(hash % capacity) : 0;
}

static size policy_next_index_linear(u64 hash, size capacity, size previous_index, size probe_count) {
    (void)hash;
    (void)probe_count;
    return capacity ? ((previous_index + 1) % capacity) : 0;
}

// Stuck next_index: the probe never advances, so next == first.
static size policy_next_index_stuck(u64 hash, size capacity, size previous_index, size probe_count) {
    (void)hash;
    (void)capacity;
    (void)probe_count;
    return previous_index;
}

// Huge next_capacity: always demands ~2^50 slots so the rehash allocation
// genuinely fails (NULL) and rehash_map takes its restore-and-return-false
// path WITHOUT installing the policy.
static size policy_next_capacity_huge(u64 length, u64 capacity, u64 tombstones, size min_entries) {
    (void)length;
    (void)capacity;
    (void)tombstones;
    (void)min_entries;
    return (size)1 << 50;
}

static MapPolicy make_tight_policy(void) {
    MapPolicy p       = {0};
    p.name            = "tight-linear";
    p.should_rehash   = policy_should_rehash;
    p.next_capacity   = policy_next_capacity_tight;
    p.first_index     = policy_first_index_linear;
    p.next_index      = policy_next_index_linear;
    p.max_probe_count = 128;
    return p;
}

static MapPolicy make_sabotage_policy(void) {
    MapPolicy p       = {0};
    p.name            = "sabotage-linear";
    p.should_rehash   = policy_should_rehash;
    p.next_capacity   = policy_next_capacity_sabotage;
    p.first_index     = policy_first_index_linear;
    p.next_index      = policy_next_index_linear;
    p.max_probe_count = 128;
    return p;
}

static MapPolicy make_small_probe_policy(void) {
    MapPolicy p       = {0};
    p.name            = "small-probe-linear";
    p.should_rehash   = policy_should_rehash;
    p.next_capacity   = policy_next_capacity_doubling;
    p.first_index     = policy_first_index_linear;
    p.next_index      = policy_next_index_linear;
    p.max_probe_count = 4;
    return p;
}

static MapPolicy make_validate_only_reject_policy(void) {
    MapPolicy p       = {0};
    p.name            = "stuck-probe-huge-cap";
    p.should_rehash   = policy_should_rehash;
    p.next_capacity   = policy_next_capacity_huge;
    p.first_index     = policy_first_index_linear;
    p.next_index      = policy_next_index_stuck; // next == first -> validate FATAL
    p.max_probe_count = 4;
    return p;
}

// ---------------------------------------------------------------------------
// map_insert growth-path policy building blocks.
// ---------------------------------------------------------------------------

// Only grow when the table has no capacity at all. Suppresses preemptive
// load-factor growth so the probe-budget-exhaustion recovery path is the only
// growth mechanism.
static bool grow_only_from_empty(u64 length, u64 capacity, u64 tombstones, size pending_inserts, size probe_pressure) {
    (void)length;
    (void)tombstones;
    (void)pending_inserts;
    (void)probe_pressure;
    return capacity == 0;
}

// Standard doubling capacity policy (mirrors the library default).
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

// Power-of-two doubling capacity, load-factor 3/4. Used by the
// map_insert_raw_entry budget-exhaustion test.
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

// Load-factor driven rehash, like the built-in default.
static bool load_rehash(u64 length, u64 capacity, u64 tombstones, size pending_inserts, size probe_pressure) {
    (void)probe_pressure;
    if ((length + pending_inserts) == 0)
        return false;
    if (capacity == 0)
        return true;
    return ((length + tombstones + pending_inserts) * 4) >= (capacity * 3);
}

// ---------------------------------------------------------------------------
// reserve_map const-42 policy building blocks.
// ---------------------------------------------------------------------------

static bool
    policy42_should_rehash(u64 length, u64 capacity, u64 tombstones, size pending_inserts, size probe_pressure) {
    (void)length;
    (void)capacity;
    (void)tombstones;
    (void)pending_inserts;
    (void)probe_pressure;
    return false;
}

static size policy42_next_capacity(u64 length, u64 capacity, u64 tombstones, size min_entries) {
    size needed = min_entries > length ? min_entries : (size)length;
    (void)capacity;
    (void)tombstones;
    if (needed == 0) {
        return 0;
    }
    if (needed <= 31) {
        return 42;
    }
    size c = 64;
    while (((c * 3) / 4) < needed) {
        c <<= 1;
    }
    return c;
}

static size policy42_first_index(u64 hash, size capacity) {
    return capacity ? (size)(hash % capacity) : 0;
}

static size policy42_next_index(u64 hash, size capacity, size previous_index, size probe_count) {
    (void)hash;
    (void)probe_count;
    return capacity ? ((previous_index + 1) % capacity) : 0;
}

static MapPolicy make_policy42(void) {
    MapPolicy policy = {
        .name            = "const42",
        .should_rehash   = policy42_should_rehash,
        .next_capacity   = policy42_next_capacity,
        .first_index     = policy42_first_index,
        .next_index      = policy42_next_index,
        .max_probe_count = 200,
    };
    return policy;
}

// ---------------------------------------------------------------------------
// Toggled value_copy_init: lets the first insert succeed and a later
// re-insert fail on demand.
// ---------------------------------------------------------------------------
static bool g_value_copy_should_fail = false;

static bool toggled_value_copy_init(void *dst, const void *src, const Allocator *alloc) {
    (void)alloc;
    if (g_value_copy_should_fail)
        return false;
    *(int *)dst = *(const int *)src;
    return true;
}

// ---------------------------------------------------------------------------
// Failing allocator: delegates to a real DefaultAllocator until `fail_now`
// is flipped, after which every allocation returns NULL.
// ---------------------------------------------------------------------------

typedef struct {
    Allocator         base;
    DefaultAllocator *inner;
    bool              fail_now;
} FailAlloc;

static void *fail_alloc_allocate(Allocator *self, size bytes, i8 zeroed) {
    FailAlloc *f = (FailAlloc *)(void *)self;
    if (f->fail_now)
        return NULL;
    return AllocatorAlloc(f->inner, bytes, zeroed);
}

static i8 fail_alloc_resize(Allocator *self, void *ptr, size new_size) {
    FailAlloc *f = (FailAlloc *)(void *)self;
    return AllocatorResize(f->inner, ptr, new_size);
}

static void *fail_alloc_remap(Allocator *self, void *ptr, size new_size) {
    FailAlloc *f = (FailAlloc *)(void *)self;
    if (f->fail_now)
        return NULL;
    return AllocatorRemap(f->inner, ptr, new_size);
}

static size fail_alloc_deallocate(Allocator *self, void *ptr) {
    FailAlloc *f = (FailAlloc *)(void *)self;
    AllocatorFree(f->inner, ptr);
    return 0;
}

static FailAlloc fail_alloc_init(DefaultAllocator *inner) {
    FailAlloc f        = {0};
    f.base.allocate    = fail_alloc_allocate;
    f.base.resize      = fail_alloc_resize;
    f.base.remap       = fail_alloc_remap;
    f.base.deallocate  = fail_alloc_deallocate;
    f.base.alignment   = 16;
    f.base.effort      = ALLOCATOR_EFFORT_ONCE;
    f.base.retry_limit = 0;
    f.base.__magic     = 0x1u; // ValidateAllocator only requires non-zero
    f.inner            = inner;
    f.fail_now         = false;
    return f;
}

static bool test_map_insert_and_set(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInit(i32_hash, i32_compare, &alloc);

    MapInsertR(&map, 1, 10);
    MapInsertR(&map, 1, 11);
    MapInsertR(&map, 2, 20);
    MapSetOnlyR(&map, 2, 200);
    MapSetOnlyR(&map, 3, 30);

    bool result = MapPairCount(&map) == 4;
    result      = result && (MapValueCountForKey(&map, 1) == 2);
    result      = result && (MapValueCountForKey(&map, 2) == 1);
    result      = result && (MapValueCountForKey(&map, 3) == 1);
    result      = result && MapGetFirstPtr(&map, 1) && (*MapGetFirstPtr(&map, 1) == 10);
    result      = result && MapGetFirstPtr(&map, 2) && (*MapGetFirstPtr(&map, 2) == 200);
    result      = result && MapGetFirstPtr(&map, 3) && (*MapGetFirstPtr(&map, 3) == 30);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_map_set_first(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInitWithValueCompare(i32_hash, i32_compare, i32_compare, &alloc);

    MapInsertR(&map, 1, 10);
    MapInsertR(&map, 1, 11);
    MapInsertR(&map, 1, 12);
    MapSetFirstR(&map, 1, 100);

    bool result = (MapPairCount(&map) == 3);
    result      = result && (MapValueCountForKey(&map, 1) == 3);
    result      = result && MapGetFirstPtr(&map, 1) && (*MapGetFirstPtr(&map, 1) == 100);
    result      = result && MapContainsPair(&map, 1, 11);
    result      = result && MapContainsPair(&map, 1, 12);
    result      = result && !MapContainsPair(&map, 1, 10);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// MapSetFirst is UPDATE-ONLY: on a key miss it must leave the map unchanged
// and return false (it does NOT insert). Covers both miss branches of
// map_set_first -- the empty-table `!capacity` early-out and the
// `idx >= capacity` key-absent early-out. An implementation that inserted on
// miss (or the header claiming it does) turns this RED.
static bool test_map_set_first_miss_returns_false(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInit(i32_hash, i32_compare, &alloc);

    // Miss on an empty (capacity 0) map: returns false, nothing inserted.
    bool result = !MapSetFirstR(&map, 7, 70);
    result      = result && (MapPairCount(&map) == 0) && !MapContainsKey(&map, 7);

    MapInsertR(&map, 1, 10);

    // Miss on a populated map (key absent): returns false, map unchanged.
    result = result && !MapSetFirstR(&map, 7, 70);
    result = result && !MapContainsKey(&map, 7);
    result = result && (MapValueCountForKey(&map, 7) == 0);
    result = result && (MapPairCount(&map) == 1);
    // Pre-existing entry is untouched.
    result = result && MapGetFirstPtr(&map, 1) && (*MapGetFirstPtr(&map, 1) == 10);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// MapSetOnly must collapse a multi-valued key down to exactly one
// mapping carrying the new value (the "Replace" contract in the header).
// Guards map_set_only's remove-all-then-insert path: dropping the
// map_remove_all step would leave the old duplicates behind.
static bool test_map_set_only_collapses_multi(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInitWithValueCompare(i32_hash, i32_compare, i32_compare, &alloc);

    MapInsertR(&map, 5, 50);
    MapInsertR(&map, 5, 51);
    MapInsertR(&map, 5, 52);
    bool result = (MapValueCountForKey(&map, 5) == 3);

    MapSetOnlyR(&map, 5, 500);

    result = result && (MapValueCountForKey(&map, 5) == 1);
    result = result && MapGetFirstPtr(&map, 5) && (*MapGetFirstPtr(&map, 5) == 500);
    result = result && MapContainsPair(&map, 5, 500);
    result = result && !MapContainsPair(&map, 5, 50);
    result = result && !MapContainsPair(&map, 5, 51);
    result = result && !MapContainsPair(&map, 5, 52);
    result = result && (MapPairCount(&map) == 1);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_map_lvalue_zeroing(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInit(i32_hash, i32_compare, &alloc);
    int              key   = 42;
    int              value = 84;

    MapInsertL(&map, key, value);

    bool result = (key == 0) && (value == 0);
    result      = result && (MapValueCountForKey(&map, 42) == 1);
    result      = result && MapGetFirstPtr(&map, 42) && (*MapGetFirstPtr(&map, 42) == 84);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// MapInsertR is the r-value form: it must NOT zero the caller's sources,
// even when no copy_init is configured (the L-form zeroes, the R-form
// does not). Guards that MapInsertR routes through map_insert_r_impl and
// never through map_zero_insert_sources_on_success.
static bool test_map_rvalue_does_not_zero_sources(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInit(i32_hash, i32_compare, &alloc);
    int              key   = 42;
    int              value = 84;

    MapInsertR(&map, key, value);

    bool result = (key == 42) && (value == 84);
    result      = result && (MapValueCountForKey(&map, 42) == 1);
    result      = result && MapGetFirstPtr(&map, 42) && (*MapGetFirstPtr(&map, 42) == 84);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// MapSetOnlyL zeroes BOTH key and value sources on success when neither
// copy_init handler is configured. Mirrors test_map_lvalue_zeroing for
// the set-only path; guards map_zero_insert_sources_on_success wiring on
// the set_only_l form.
static bool test_map_set_only_lvalue_zeroing(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInit(i32_hash, i32_compare, &alloc);
    int              key   = 11;
    int              value = 110;

    MapSetOnlyL(&map, key, value);

    bool result = (key == 0) && (value == 0);
    result      = result && (MapValueCountForKey(&map, 11) == 1);
    result      = result && MapGetFirstPtr(&map, 11) && (*MapGetFirstPtr(&map, 11) == 110);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// MapSetFirstL zeroes ONLY the value source on a successful UPDATE (the key
// is always an r-value lookup and is never zeroed). On a key miss it is
// update-only: returns false, inserts nothing, and leaves the value source
// untouched. Guards map_zero_value_source_on_success wiring on the
// set_first_l form (zero only on success).
static bool test_map_set_first_lvalue_zeroing(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInit(i32_hash, i32_compare, &alloc);
    int              value = 110;

    MapInsertR(&map, 11, 10);

    // Update-existing path: first value replaced, value source zeroed.
    bool result = MapSetFirstL(&map, 11, value);
    result      = result && (value == 0);
    result      = result && (MapValueCountForKey(&map, 11) == 1);
    result      = result && MapGetFirstPtr(&map, 11) && (*MapGetFirstPtr(&map, 11) == 110);

    // Miss path: returns false, value source NOT zeroed, nothing inserted.
    int miss = 220;
    result   = result && !MapSetFirstL(&map, 99, miss);
    result   = result && (miss == 220);
    result   = result && !MapContainsKey(&map, 99);
    result   = result && (MapPairCount(&map) == 1);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Heavy insert+remove churn at near-threshold load. With linear probing
// limited to `max_probe_count` slots, a long collision cluster can force
// scan_slots to fail even though empty slots exist; the prior fix was to
// force `next_capacity` to grow on that path (rehashing at the same size
// would re-probe the same cluster and loop forever).
static bool test_map_churn_does_not_loop(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc  = DefaultAllocatorInit();
    IntIntMap        map    = MapInit(i32_hash, i32_compare, &alloc);
    bool             result = true;

    // Phase 1: fill past the first few growth thresholds.
    for (int i = 0; i < 600; ++i) {
        MapInsertR(&map, i, i * 10);
    }
    result = result && (MapPairCount(&map) == 600);

    // Phase 2: alternating remove+insert at the same churn point. Each
    // cycle exercises rehash + scan with tombstones present, which is
    // what previously triggered the runaway recursion.
    for (int i = 0; i < 4000; ++i) {
        int key = 600 + (i & 0x3f); // small cycling window
        MapRemoveAll(&map, key);
        MapInsertR(&map, key, i);
    }
    // Deterministic count: 600 base keys (0..599) survive untouched, plus
    // the 64 churned keys (600..663) each ending with exactly one value.
    result = result && (MapPairCount(&map) == 664);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_map_ensure_ptr(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInit(i32_hash, i32_compare, &alloc);
    int             *value_ptr;
    bool             result;

    value_ptr = MapEnsurePtr(&map, 8, 80);
    result    = value_ptr && (*value_ptr == 80);
    result    = result && (MapPairCount(&map) == 1);
    result    = result && (MapValueCountForKey(&map, 8) == 1);

    value_ptr = MapEnsurePtr(&map, 8, 800);
    result    = result && value_ptr && (*value_ptr == 80);
    result    = result && (MapPairCount(&map) == 1);
    result    = result && (MapValueCountForKey(&map, 8) == 1);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// The pointer MapEnsurePtr returns must alias the live value slot: a
// write through it has to be observable on a subsequent re-fetch. Guards
// that map_ensure_value_ptr returns the in-table slot, not a copy.
static bool test_map_ensure_ptr_mutation_persists(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInit(i32_hash, i32_compare, &alloc);

    int *value_ptr = MapEnsurePtr(&map, 3, 30);
    bool result    = value_ptr && (*value_ptr == 30);

    *value_ptr = 333; // mutate through the returned pointer

    int *refetched = MapGetFirstPtr(&map, 3);
    result         = result && refetched && (*refetched == 333);
    // Mutation must not have spawned a second entry.
    result = result && (MapValueCountForKey(&map, 3) == 1);
    result = result && (MapPairCount(&map) == 1);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// MapInsert / MapSet default-alias their L-forms: MapInsert == MapInsertL
// (zeroes both sources) and MapSet == MapSetOnlyL (zeroes both sources,
// replace semantics). Guards the aliasing #defines in Insert.h.
static bool test_map_default_aliases(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInit(i32_hash, i32_compare, &alloc);
    int              k1    = 1;
    int              v1    = 10;
    int              v2    = 11;

    // MapInsert aliases MapInsertL: both sources zeroed.
    MapInsert(&map, k1, v1);
    bool result = (k1 == 0) && (v1 == 0);
    result      = result && (MapValueCountForKey(&map, 1) == 1);

    // MapSet aliases MapSetOnlyL: replace + zero both sources.
    MapInsertR(&map, 1, 99); // give key 1 a second value
    result = result && (MapValueCountForKey(&map, 1) == 2);
    int k2 = 1;
    MapSet(&map, k2, v2);
    result = result && (k2 == 0) && (v2 == 0);
    result = result && (MapValueCountForKey(&map, 1) == 1);
    result = result && MapGetFirstPtr(&map, 1) && (*MapGetFirstPtr(&map, 1) == 11);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Success-path coverage for the aborting Must* insert family. We never
// exercise the abort branch (that is the Deadend agent's job); we only
// assert each MapMust* applies the same effect as its fallible form.
static bool test_map_must_family_success(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInitWithValueCompare(i32_hash, i32_compare, i32_compare, &alloc);
    int              k     = 1;
    int              v     = 10;

    MapMustInsertL(&map, k, v); // L-form zeroes sources
    bool result = (k == 0) && (v == 0);
    result      = result && (MapValueCountForKey(&map, 1) == 1);

    MapMustInsertR(&map, 2, 20);
    result = result && (MapValueCountForKey(&map, 2) == 1);

    int ik = 3;
    int iv = 30;
    MapMustInsert(&map, ik, iv); // aliases L-form: zeroes both sources
    result = result && (ik == 0) && (iv == 0);
    result = result && (MapValueCountForKey(&map, 3) == 1);

    // SetFirstR updates the existing first value of key 2 (insert was 20).
    MapMustSetFirstR(&map, 2, 222);
    result = result && MapGetFirstPtr(&map, 2) && (*MapGetFirstPtr(&map, 2) == 222);

    int sf = 333;
    MapMustSetFirstL(&map, 3, sf); // value zeroed on success
    result = result && (sf == 0);
    result = result && MapGetFirstPtr(&map, 3) && (*MapGetFirstPtr(&map, 3) == 333);

    MapMustSetOnlyR(&map, 4, 40);
    result = result && (MapValueCountForKey(&map, 4) == 1);
    result = result && MapGetFirstPtr(&map, 4) && (*MapGetFirstPtr(&map, 4) == 40);

    int so = 50;
    int sk = 4;
    MapMustSetOnlyL(&map, sk, so); // replace + zero both sources
    result = result && (sk == 0) && (so == 0);
    result = result && (MapValueCountForKey(&map, 4) == 1);
    result = result && MapGetFirstPtr(&map, 4) && (*MapGetFirstPtr(&map, 4) == 50);

    int msk = 5;
    int msv = 55;
    MapMustSet(&map, msk, msv); // aliases SetOnlyL: zeroes both sources
    result = result && (msk == 0) && (msv == 0);
    result = result && (MapValueCountForKey(&map, 5) == 1);
    result = result && MapGetFirstPtr(&map, 5) && (*MapGetFirstPtr(&map, 5) == 55);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// ===========================================================================
// rehash_map / reserve_map / MapCompact contract guards (from staging).
// ===========================================================================

// 508: MapRehashWithPolicy validates the new policy up front. Real code aborts
// on the stuck-probe check before allocating; the mutant runs the rehash, the
// 2^50-slot allocation fails, rehash_map restores and returns false without
// installing the policy, and nothing aborts -> deadend FAILS = mutant killed.
static bool test_rehash_rejects_invalid_policy(void) {
    WriteFmt("Testing MapRehashWithPolicy validates the new policy up front\n");

    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInit(i32_hash, i32_compare, &alloc);

    MapInsertR(&map, 1, 10);
    MapInsertR(&map, 2, 20);
    MapInsertR(&map, 3, 30);

    MapPolicy bad = make_validate_only_reject_policy();
    MapRehashWithPolicy(&map, 1, bad); // real: LOG_FATAL on the stuck-probe check

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// 510: the empty-fast-path guard `(length==0 && n==0)`. Flipping `==` to `!=`
// breaks MapCompact on a fresh/empty map.
static bool test_compact_empty_map_succeeds(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInit(i32_hash, i32_compare, &alloc);

    bool result = MapCompact(&map); // empty map: must succeed via fast path

    result = result && (MapPairCount(&map) == 0);
    MapInsertR(&map, 5, 50);
    result = result && MapGetFirstPtr(&map, 5) && (*MapGetFirstPtr(&map, 5) == 50);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 515/516/517: the empty-reset path sets length/capacity/tombstones to 0. An
// empty MapCompact must leave the map valid for further use.
static bool test_compact_empty_then_insert_length(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInit(i32_hash, i32_compare, &alloc);

    bool result = MapCompact(&map);
    MapInsertR(&map, 7, 70);
    result = result && MapGetFirstPtr(&map, 7) && (*MapGetFirstPtr(&map, 7) == 70);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_compact_empty_then_insert_capacity(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInit(i32_hash, i32_compare, &alloc);

    bool result = MapCompact(&map);
    MapInsertR(&map, 8, 80);
    MapInsertR(&map, 9, 90);
    result = result && MapGetFirstPtr(&map, 8) && (*MapGetFirstPtr(&map, 8) == 80);
    result = result && MapGetFirstPtr(&map, 9) && (*MapGetFirstPtr(&map, 9) == 90);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_compact_empty_then_insert_tombstones(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInit(i32_hash, i32_compare, &alloc);

    bool result = MapCompact(&map);
    MapInsertR(&map, 11, 110);
    result = result && MapGetFirstPtr(&map, 11) && (*MapGetFirstPtr(&map, 11) == 110);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 525:22: `new_capacity < required` must NOT reject a policy that returns
// capacity exactly equal to `required`. A tight (load-factor-1.0) policy is
// valid and must be accepted.
static bool test_rehash_tight_capacity_accepted(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInit(i32_hash, i32_compare, &alloc);

    MapInsertR(&map, 1, 10);
    MapInsertR(&map, 2, 20);
    MapInsertR(&map, 3, 30);

    MapPolicy tight  = make_tight_policy();
    bool      result = MapRehashWithPolicy(&map, 0, tight);

    result = result && (MapPairCount(&map) == 3);
    result = result && MapGetFirstPtr(&map, 1) && (*MapGetFirstPtr(&map, 1) == 10);
    result = result && MapGetFirstPtr(&map, 2) && (*MapGetFirstPtr(&map, 2) == 20);
    result = result && MapGetFirstPtr(&map, 3) && (*MapGetFirstPtr(&map, 3) == 30);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 525:27: `required = max(n, length)`. Turning max into min weakens the
// insufficiency guard. With n < length and a policy returning a capacity in
// [n, length), the real code must reject it (length is required).
static bool test_rehash_rejects_insufficient_with_small_n(void) {
    WriteFmt("Testing MapRehashWithPolicy rejects under-sized capacity for n<length\n");

    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInit(i32_hash, i32_compare, &alloc);

    for (int k = 0; k < 10; k++)
        MapInsertR(&map, k, k * 10);

    MapPolicy sab = make_sabotage_policy();
    MapRehashWithPolicy(&map, 2, sab); // must LOG_FATAL "insufficient capacity"

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// 549: on allocation failure the restore path must set capacity = old_capacity
// so the map stays consistent (FAILURE contract: map unchanged).
static bool test_rehash_alloc_failure_keeps_map_usable(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator inner = DefaultAllocatorInit();
    FailAlloc        fa    = fail_alloc_init(&inner);
    IntIntMap        map   = MapInit(i32_hash, i32_compare, &fa.base);

    for (int k = 0; k < 6; k++)
        MapInsertR(&map, k, k * 10);

    fa.fail_now = true;              // force the new-table allocation to fail
    bool failed = !MapCompact(&map); // must return false
    fa.fail_now = false;

    bool result = failed && (MapPairCount(&map) == 6);
    for (int k = 0; k < 6; k++)
        result = result && MapGetFirstPtr(&map, k) && (*MapGetFirstPtr(&map, k) == k * 10);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&inner);
    return result;
}

// Clustering fixture for the doubling-retry mutants (573, 585:14, 585:38, 586,
// 593). Multiples-of-8 keys under identity hash + a tiny probe budget: at the
// compacted capacity (8) they alias into one bucket and the 5th reinsert
// exhausts the 4-probe budget; one doubling (to 16) splits them and succeeds.
static bool run_doubling_retry_compact(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc  = DefaultAllocatorInit();
    MapPolicy        policy = make_small_probe_policy();
    IntIntMap        map    = MapInitWithPolicy(i32_identity_hash, i32_compare, policy, &alloc);

    const int keys[] = {0, 8, 16, 24, 32, 40};
    for (int i = 0; i < 6; i++)
        MapInsertR(&map, keys[i], keys[i] + 1);

    bool result = (MapPairCount(&map) == 6);

    result = result && MapCompact(&map);
    result = result && (MapCapacity(&map) == 16);

    result = result && (MapPairCount(&map) == 6);
    for (int i = 0; i < 6; i++)
        result = result && MapGetFirstPtr(&map, keys[i]) && (*MapGetFirstPtr(&map, keys[i]) == keys[i] + 1);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 573: on a reinsert failure, `ok = false` then break-to-retry.
static bool test_doubling_retry_preserves_entries(void) {
    return run_doubling_retry_compact();
}

// 585:14: `next_cap = new_capacity * 2`. Forcing a constant <= new_capacity
// trips the overflow guard so the retry fails instead of doubling.
static bool test_doubling_retry_next_cap_init(void) {
    return run_doubling_retry_compact();
}

// 585:38: `* 2` -> `/ 2` makes next_cap < new_capacity, always tripping the
// overflow guard; the retry can never grow.
static bool test_doubling_retry_next_cap_grows(void) {
    return run_doubling_retry_compact();
}

// 586: `next_cap <= new_capacity` (overflow guard) -> `>` misreads a normal
// doubling as overflow and fails the retry.
static bool test_doubling_retry_overflow_guard(void) {
    return run_doubling_retry_compact();
}

// 593: `new_capacity = next_cap` carries the doubled capacity into the next
// retry. A constant breaks growth.
static bool test_doubling_retry_capacity_propagates(void) {
    return run_doubling_retry_compact();
}

// ===========================================================================
// map_insert growth/recovery/tombstone contract guards (from staging).
// ===========================================================================

// Mutant 940. Under the default policy the 6th insert into a capacity-8 table
// crosses the 3/4 load threshold and must grow.
static bool test_map_preemptive_grow_succeeds(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc  = DefaultAllocatorInit();
    IntIntMap        map    = MapInit(i32_hash, i32_compare, &alloc);
    bool             result = true;

    for (int k = 0; k < 6; k++)
        result = result && MapInsertR(&map, k, k * 100 + 1);

    result = result && (MapPairCount(&map) == 6);
    for (int k = 0; k < 6; k++) {
        int *v = MapGetFirstPtr(&map, k);
        result = result && v && (*v == k * 100 + 1);
    }

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Mutants 975 and 988. A non-preemptive policy with a small probe budget lets
// the table fill completely; the next insert exhausts the budget and can only
// succeed by taking the forced-grow recovery path.
static bool test_map_probe_exhaustion_recovers(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc  = DefaultAllocatorInit();
    MapPolicy        policy = fill_then_grow_policy();
    IntIntMap        map    = MapInitWithPolicy(i32_hash, i32_compare, policy, &alloc);
    bool             result = true;

    for (int k = 0; k < 8; k++)
        result = result && MapInsertR(&map, k, k * 100 + 3);

    result = result && MapInsertR(&map, 8, 8 * 100 + 3);

    result = result && (MapPairCount(&map) == 9);
    for (int k = 0; k < 9; k++) {
        int *v = MapGetFirstPtr(&map, k);
        result = result && v && (*v == k * 100 + 3);
    }

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Mutant 1008 (`tombstones -= 1` -> `+= 1` when reusing a tombstone slot).
static bool test_map_tombstone_reuse_decrements(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap        map   = MapInit(i32_hash, i32_compare, &alloc);

    MapInsertR(&map, 7, 70);
    MapRemoveFirst(&map, 7);
    bool result = (MapTombstones(&map) == 1);

    MapInsertR(&map, 7, 71); // reuses the tombstone slot for key 7
    result = result && (MapTombstones(&map) == 0);
    result = result && MapGetFirstPtr(&map, 7) && (*MapGetFirstPtr(&map, 7) == 71);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Mutants 1024 and 1025: the rollback when an insert into a tombstone slot
// fails to copy must restore the tombstone count to its pre-insert value.
static bool test_map_tombstone_rollback_on_copy_fail(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc = DefaultAllocatorInit();
    IntIntMap map = MapInitWithDeepCopy(i32_hash, i32_compare, NULL, NULL, toggled_value_copy_init, NULL, &alloc);

    g_value_copy_should_fail = false;
    bool result              = MapInsertR(&map, 5, 50);
    MapRemoveFirst(&map, 5);
    result = result && (MapTombstones(&map) == 1);

    g_value_copy_should_fail = true;
    bool reinserted          = MapInsertR(&map, 5, 51);
    g_value_copy_should_fail = false;

    result = result && !reinserted;
    result = result && (MapTombstones(&map) == 1);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// ===========================================================================
// map_insert_raw_entry / reserve_map contract guards (from staging).
// ===========================================================================

// map_insert_raw_entry : insert_idx >= capacity  (381:20 cxx_ge_to_gt).
// During a rehash, a re-insert whose probe chain exhausts the budget returns
// insert_idx == capacity, which must be caught (-> false) so rehash_map grows
// and retries. The mutant lets it slip into an out-of-bounds write.
static bool test_insert_raw_entry_grows_on_budget_exhaustion(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc  = DefaultAllocatorInit();
    MapPolicy        policy = {
               .name            = "tight-linear",
               .should_rehash   = load_rehash,
               .next_capacity   = pow2_capacity,
               .first_index     = linear_first,
               .next_index      = linear_next,
               .max_probe_count = 4,
    };
    IntIntMap map = MapInitWithPolicy(id_hash, i32_compare, policy, &alloc);

    const int keys[] = {0, 16, 32, 48, 64};
    for (int i = 0; i < 5; i++)
        MapInsertR(&map, keys[i], keys[i] + 1);

    MapRehashWithPolicy(&map, 8, policy);

    bool result = (MapPairCount(&map) == 5);
    for (int i = 0; i < 5; i++) {
        int *v = MapGetFirstPtr(&map, keys[i]);
        result = result && v && (*v == keys[i] + 1);
        result = result && MapContainsKey(&map, keys[i]);
    }

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 619 (reserve_map, `target_capacity = policy.next_capacity(...)` -> 42).
// reserve_map must grow the table when the requested element count exceeds the
// current capacity.
static bool test_reserve_grows_when_target_exceeds_capacity(void) {
    typedef Map(int, int) IntIntMap;
    DefaultAllocator alloc  = DefaultAllocatorInit();
    MapPolicy        policy = make_policy42();
    IntIntMap        map    = MapInitWithPolicy(i32_hash, i32_compare, policy, &alloc);

    MapInsertR(&map, 1, 10);
    bool result = (MapCapacity(&map) == 42);

    bool reserved = MapReserve(&map, 100);
    result        = result && reserved;
    result        = result && (MapCapacity(&map) >= 100);

    MapDeinit(&map);
    DefaultAllocatorDeinit(&alloc);
    return result;
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
        test_map_insert_and_set,
        test_map_set_first,
        test_map_set_first_miss_returns_false,
        test_map_set_only_collapses_multi,
        test_map_lvalue_zeroing,
        test_map_rvalue_does_not_zero_sources,
        test_map_set_only_lvalue_zeroing,
        test_map_set_first_lvalue_zeroing,
        test_map_ensure_ptr,
        test_map_ensure_ptr_mutation_persists,
        test_map_default_aliases,
        test_map_must_family_success,
        test_map_churn_does_not_loop,
        test_compact_empty_map_succeeds,
        test_compact_empty_then_insert_length,
        test_compact_empty_then_insert_capacity,
        test_compact_empty_then_insert_tombstones,
        test_rehash_tight_capacity_accepted,
        test_rehash_alloc_failure_keeps_map_usable,
        test_doubling_retry_preserves_entries,
        test_doubling_retry_next_cap_init,
        test_doubling_retry_next_cap_grows,
        test_doubling_retry_overflow_guard,
        test_doubling_retry_capacity_propagates,
        test_map_preemptive_grow_succeeds,
        test_map_probe_exhaustion_recovers,
        test_map_tombstone_reuse_decrements,
        test_map_tombstone_rollback_on_copy_fail,
        test_insert_raw_entry_grows_on_budget_exhaustion,
        test_reserve_grows_when_target_exceeds_capacity,
        test_default_grow_uses_multiply_not_divide,
        test_default_rehash_inclusive_at_boundary,
        test_next_capacity_doubling_strict_less,
        test_setonly_raw_reinsert_decrements_tombstones,
        test_probe_recovery_forced_n_is_capacity_plus_one,
    };

    TestFunction deadend_tests[] = {
        test_rehash_rejects_invalid_policy,
        test_rehash_rejects_insufficient_with_small_n,
    };

    WriteFmt("[INFO] Starting Map.Insert tests\n\n");
    return run_test_suite(
        tests,
        (int)(sizeof(tests) / sizeof(tests[0])),
        deadend_tests,
        (int)(sizeof(deadend_tests) / sizeof(deadend_tests[0])),
        "Map.Insert"
    );
}
