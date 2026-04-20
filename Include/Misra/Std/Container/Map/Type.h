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

typedef size (*MapPolicyProbeFn)(u64 hash, size probe_count, size capacity);
typedef bool (*MapPolicyShouldRehashFn)(const GenericMap *map);

typedef struct {
    const char              *name;
    MapPolicyProbeFn         probe_index;
    MapPolicyShouldRehashFn  should_rehash;
} MapPolicy;

extern const MapPolicy MisraMapPolicyLinear;
extern const MapPolicy MisraMapPolicyQuadratic;

struct GenericMap {
    u64               length;
    u64               capacity;
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
#define MapEntry(K, V)                                                                                             \
    struct {                                                                                                           \
        K   key;                                                                                                       \
        V   value;                                                                                                     \
        u64 hash;                                                                                                      \
    }

///
/// Typesafe map definition.
///
/// This behaves like the other generic containers in the project: each use of
/// `Map(K, V)` creates a distinct anonymous type, so reusable aliases should
/// be defined with `typedef`.
///
/// USAGE:
///   typedef Map(int, Str) IntStrMap;
///   typedef Map(T(Pair(i32, i32)), float) PairFloatMap;
///
/// FIELDS:
/// - length            : Number of occupied entries in the map.
/// - capacity          : Total number of probe slots currently allocated.
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
#define Map(K, V)                                                                                                  \
    struct {                                                                                                           \
        u64               length;                                                                                      \
        u64               capacity;                                                                                    \
        GenericCopyInit   key_copy_init;                                                                               \
        GenericCopyDeinit key_copy_deinit;                                                                             \
        GenericCopyInit   value_copy_init;                                                                             \
        GenericCopyDeinit value_copy_deinit;                                                                           \
        GenericCompare    key_compare;                                                                                 \
        GenericHash       key_hash;                                                                                    \
        MapEntry(K, V) *entries;                                                                                       \
        u8               *states;                                                                                      \
        MapPolicy         policy;                                                                                      \
        u64               __magic;                                                                                     \
    }

#define MAP_ENTRY_TYPE(m) TYPE_OF((m)->entries[0])
#define MAP_KEY_TYPE(m)   TYPE_OF((m)->entries[0].key)
#define MAP_VALUE_TYPE(m) TYPE_OF((m)->entries[0].value)

#define MISRA_MAP_MAGIC MISRA_MAKE_NEW_MAGIC_VALUE("map00000")

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
