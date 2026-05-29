/// file      : std/container/map/type.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Generic map type definition

#ifndef MISRA_STD_CONTAINER_MAP_TYPE_H
#define MISRA_STD_CONTAINER_MAP_TYPE_H


#include <Misra/Std/Container/Common.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Types.h>

typedef struct GenericMap GenericMap;

typedef bool (*MapPolicyShouldRehashFn)(
    u64  length,
    u64  capacity,
    u64  tombstones,
    size pending_inserts,
    size probe_pressure
);
typedef size (*MapPolicyNextCapacityFn)(u64 length, u64 capacity, u64 tombstones, size min_entries);
typedef size (*MapPolicyFirstIndexFn)(u64 hash, size capacity);
typedef size (*MapPolicyNextIndexFn)(u64 hash, size capacity, size previous_index, size probe_count);
typedef bool (*MapPredicateFn)(const void *key, const void *value, void *ctx);

///
/// Policy object controlling probe-table behavior of `Map`.
///
/// `MapPolicy` is intentionally narrower than a full backend interface.
/// The map runtime still owns allocation, rollback, copying, and slot writes.
/// The policy answers strategy questions:
///
/// - when probing pressure is unhealthy
/// - what capacity should be chosen next
/// - where probing starts
/// - how probing advances after collisions
///
/// This keeps policies powerful enough to tune behavior for different workloads
/// without letting them redefine container ownership or memory semantics.
///
/// USAGE:
///   MapPolicy policy = {
///       .name            = "dense-linear",
///       .should_rehash   = my_should_rehash,
///       .next_capacity   = my_next_capacity,
///       .first_index     = my_first_index,
///       .next_index      = my_next_index,
///       .max_probe_count = 64,
///   };
///
///   typedef Map(Str, Str) StrMap;
///   StrMap map = MapInitWithPolicy(str_hash, str_compare, policy);
///
/// FIELDS:
/// - name            : Human-readable identifier used in diagnostics and validation errors.
/// - should_rehash   : Callback deciding whether current occupancy, tombstones, pending inserts, and recent probe pressure require a rebuild.
/// - next_capacity   : Callback choosing the next table capacity. It must return enough capacity to hold at least `min_entries`.
/// - first_index     : Callback mapping a key hash to the first slot to probe in a table of the given capacity.
/// - next_index      : Callback choosing the next slot after a collision, using the previous index and probe count.
/// - max_probe_count : Hard limit on probe attempts before the map forces a rebuild or fails validation.
///
/// NOTE:
/// - Use a low probe count and aggressive growth when lookup latency matters more than memory density.
/// - Use a denser growth policy when memory footprint matters and longer probe chains are acceptable.
/// - Linear probing is a good default for cache-friendly general workloads.
/// - Quadratic probing is useful when you want to reduce primary clustering without changing the public map API.
///
/// TAGS: Map, Policy, Hashing, Probing, Configuration
///
typedef struct {
    Zstr                    name;
    MapPolicyShouldRehashFn should_rehash;
    MapPolicyNextCapacityFn next_capacity;
    MapPolicyFirstIndexFn   first_index;
    MapPolicyNextIndexFn    next_index;
    size                    max_probe_count;
} MapPolicy;

///
/// Built-in linear probing policy.
///
/// INFO: This is the best general-purpose starting point when you do not have a workload-specific reason to choose something else.
///
/// TAGS: Map, Constant, Policy
///
extern const MapPolicy MapPolicyLinear;

///
/// Built-in quadratic probing policy.
///
/// INFO: This is useful when you want to reduce clustering pressure while keeping the same `Map` API and runtime ownership model.
///
/// TAGS: Map, Constant, Policy
///
extern const MapPolicy MapPolicyQuadratic;

typedef struct {
    size __index;
} MapValueCursor;

struct GenericMap {
    u64               length;
    u64               capacity;
    u64               tombstones;
    GenericCopyInit   key_copy_init;
    GenericCopyDeinit key_copy_deinit;
    GenericCopyInit   value_copy_init;
    GenericCopyDeinit value_copy_deinit;
    GenericCompare    key_compare;
    GenericCompare    value_compare;
    GenericHash       key_hash;
    u8               *entries;
    u8               *states;
    MapPolicy         policy;
    Allocator        *allocator;
    u64               __magic;
};

#define GENERIC_MAP(x) ((GenericMap *)(void *)(x))

///
/// Single entry stored by `Map`.
///
/// FIELDS:
/// - key   : Key stored in this entry.
/// - value : Value stored in this entry.
/// - hash  : Cached key hash used for probing.
///
/// TAGS: Map, Type, API
///
#define MapEntry(K, V)                                                                                                 \
    struct {                                                                                                           \
        K   key;                                                                                                       \
        V   value;                                                                                                     \
        u64 hash;                                                                                                      \
    }

///
/// Typesafe multimap definition.
///
/// This behaves like the other generic containers in the project: each use of
/// `Map(K, V)` creates a distinct anonymous type, so reusable aliases should
/// be defined with `typedef`. Multiple values may be stored for the same key.
///
/// USAGE:
///   typedef Map(int, Str) IntStrMap;
///   typedef Map(T(Pair(i32, i32)), float) PairFloatMap;
///
/// FIELDS:
/// - length            : Number of stored key/value pairs, including duplicate keys.
/// - capacity          : Total number of probe slots currently allocated.
/// - tombstones        : Number of deleted slots currently retained for probing.
/// - key_copy_init     : Optional deep-copy callback for keys.
/// - key_copy_deinit   : Optional deinit callback for keys held by the map.
/// - value_copy_init   : Optional deep-copy callback for values.
/// - value_copy_deinit : Optional deinit callback for values held by the map.
/// - key_compare       : Required comparator for keys. Equality is `compare == 0`.
/// - value_compare     : Optional comparator for values. Required for pair-level APIs.
/// - key_hash          : Required hash callback for keys.
/// - entries           : Pointer to entry storage. Do not index directly.
/// - states            : Slot-state storage used internally by the probing policy.
/// - policy            : Copy of the probing policy used by this map.
///
/// TAGS: Map, Generic, KeyValue, Policy, Lookup
///
#define Map(K, V)                                                                                                      \
    struct {                                                                                                           \
        u64               length;                                                                                      \
        u64               capacity;                                                                                    \
        u64               tombstones;                                                                                  \
        GenericCopyInit   key_copy_init;                                                                               \
        GenericCopyDeinit key_copy_deinit;                                                                             \
        GenericCopyInit   value_copy_init;                                                                             \
        GenericCopyDeinit value_copy_deinit;                                                                           \
        GenericCompare    key_compare;                                                                                 \
        GenericCompare    value_compare;                                                                               \
        GenericHash       key_hash;                                                                                    \
        MapEntry(K, V) * entries;                                                                                      \
        u8        *states;                                                                                             \
        MapPolicy  policy;                                                                                             \
        Allocator *allocator;                                                                                          \
        u64        __magic;                                                                                            \
    }

#define MAP_ENTRY_TYPE(m) TYPE_OF((m)->entries[0])
#define MAP_KEY_TYPE(m)   TYPE_OF((m)->entries[0].key)
#define MAP_VALUE_TYPE(m) TYPE_OF((m)->entries[0].value)

#define MAP_MAGIC MAKE_NEW_MAGIC_VALUE("map00000")

///
/// Validate whether a given `MapPolicy` object is valid.
/// Aborts if the policy is structurally invalid.
///
/// policy_value[in] : Policy to validate.
///
/// SUCCESS: Continue execution, meaning the policy has a non-empty
///          name, all required callbacks, a non-zero `max_probe_count`,
///          and `first_index` / `next_index` return in-range indices
///          across a fixed set of probe-snapshot inputs.
/// FAILURE: `abort` via `LOG_FATAL` when any of those invariants is
///          broken (NULL pointer, missing name, missing callback,
///          zero `max_probe_count`, or a callback returning an index
///          past `capacity`).
///
/// TAGS: Map, Validate, API
///
#define ValidateMapPolicy(policy_value) validate_map_policy(&(policy_value))

///
/// Validate whether a given `Map` object is valid.
/// Aborts if provided map is uninitialized or corrupted.
///
/// m[in] : Pointer to `Map` object to validate.
///
/// SUCCESS: Continue execution, meaning given map is most probably valid.
/// FAILURE: `abort`
///
/// TAGS: Map, Validate, API
///
#define ValidateMap(m) validate_map((const GenericMap *)GENERIC_MAP(m))

#endif // MISRA_STD_CONTAINER_MAP_TYPE_H
