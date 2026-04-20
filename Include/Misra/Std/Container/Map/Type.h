/// file      : std/container/map/type.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Generic map type definition

#ifndef MISRA_STD_CONTAINER_MAP_TYPE_H
#define MISRA_STD_CONTAINER_MAP_TYPE_H

#include <stddef.h>

#include <Misra/Std/Container/Common.h>
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

typedef struct {
    const char             *name;
    MapPolicyShouldRehashFn should_rehash;
    MapPolicyNextCapacityFn next_capacity;
    MapPolicyFirstIndexFn   first_index;
    MapPolicyNextIndexFn    next_index;
    size                    max_probe_count;
} MapPolicy;

extern const MapPolicy MisraMapPolicyLinear;
extern const MapPolicy MisraMapPolicyQuadratic;

struct GenericMap {
    u64               length;
    u64               capacity;
    u64               tombstones;
    GenericCopyInit   key_copy_init;
    GenericCopyDeinit key_copy_deinit;
    GenericCopyInit   value_copy_init;
    GenericCopyDeinit value_copy_deinit;
    GenericCompare    key_compare;
    GenericHash       key_hash;
    char             *entries;
    u8               *states;
    MapPolicy         policy;
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
        GenericHash       key_hash;                                                                                    \
        MapEntry(K, V) * entries;                                                                                      \
        u8       *states;                                                                                              \
        MapPolicy policy;                                                                                              \
        u64       __magic;                                                                                             \
    }

#define MAP_ENTRY_TYPE(m) TYPE_OF((m)->entries[0])
#define MAP_KEY_TYPE(m)   TYPE_OF((m)->entries[0].key)
#define MAP_VALUE_TYPE(m) TYPE_OF((m)->entries[0].value)

#define MISRA_MAP_MAGIC MISRA_MAKE_NEW_MAGIC_VALUE("map00000")

///
/// Validate whether a given `MapPolicy` object is valid.
/// Aborts if the policy is structurally invalid.
///
/// policy_value[in] : Policy to validate.
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
#define ValidateMap(m) validate_map((const GenericMap *)GENERIC_MAP(m))

#endif // MISRA_STD_CONTAINER_MAP_TYPE_H
