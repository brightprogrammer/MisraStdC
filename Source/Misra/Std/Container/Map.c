/// file      : std/container/map.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Generic map implementation

#include <Misra/Std/Container/Map.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Memory.h>
#include <Misra/Sys.h>


static size quadratic_probe_index(u64 hash, size probe_count, size capacity) {
    (void)capacity;
    return (size)(hash + ((probe_count * (probe_count + 1)) / 2));
}

static bool default_should_rehash(u64 length, u64 capacity, u64 tombstones, size pending_inserts, size probe_pressure) {
    if ((length + pending_inserts) == 0) {
        return false;
    }

    if (capacity == 0) {
        return true;
    }

    if (((length + tombstones + pending_inserts) * 4) >= (capacity * 3)) {
        return true;
    }

    return probe_pressure > 0 && (probe_pressure * 4) >= capacity;
}

static size default_next_capacity(u64 length, u64 capacity, u64 tombstones, size min_entries) {
    size new_capacity = 8;
    size needed       = min_entries > length ? min_entries : (size)length;

    (void)tombstones;
    (void)capacity;

    if (needed == 0) {
        return 0;
    }
    // Cap `needed` at what the doubling loop can actually reach. A
    // `needed` value above (3/4) * 2^63 makes the loop spin forever
    // -- once new_capacity hits 2^63, the next `<<= 1` wraps to 0,
    // (0 * 3) / 4 == 0 < needed, and we shift 0 forever. Refuse the
    // request instead.
    size max_needed = (((size)1 << 63) / 4u) * 3u; // (2^63) * 3/4
    if (needed > max_needed) {
        return 0;
    }

    while (((new_capacity * 3) / 4) < needed) {
        new_capacity <<= 1;
    }

    return new_capacity;
}

static size linear_first_index(u64 hash, size capacity) {
    return capacity ? (size)(hash % capacity) : 0;
}

static size linear_next_index(u64 hash, size capacity, size previous_index, size probe_count) {
    (void)hash;
    (void)probe_count;
    return capacity ? ((previous_index + 1) % capacity) : 0;
}

static size quadratic_first_index(u64 hash, size capacity) {
    return capacity ? (size)(hash % capacity) : 0;
}

static size quadratic_next_index(u64 hash, size capacity, size previous_index, size probe_count) {
    (void)previous_index;
    return capacity ? (quadratic_probe_index(hash, probe_count, capacity) % capacity) : 0;
}

static size map_validate_policy_index(size idx, size capacity, Zstr callback_name) {
    if (capacity && idx >= capacity) {
        LOG_FATAL("{} returned index {} for capacity {}", callback_name, idx, capacity);
    }

    return idx;
}

void validate_map_policy(const MapPolicy *policy) {
    static const struct {
        u64 length;
        u64 capacity;
        u64 tombstones;
    } snapshots[] = {
        { .length = 0,  .capacity = 0, .tombstones = 0},
        { .length = 3,  .capacity = 8, .tombstones = 0},
        { .length = 5,  .capacity = 8, .tombstones = 2},
        {.length = 10, .capacity = 16, .tombstones = 3},
    };
    static const size capacities[] = {1, 8, 17};
    size              idx_i;
    size              cap_i;

    if (!policy) {
        LOG_FATAL("Expected a valid MapPolicy pointer");
    }

    if (!policy->name || !policy->name[0]) {
        LOG_FATAL("MapPolicy must have a non-empty name");
    }

    if (!policy->should_rehash || !policy->next_capacity || !policy->first_index || !policy->next_index) {
        LOG_FATAL("MapPolicy '{}' must provide all required callbacks", policy->name);
    }

    if (policy->max_probe_count == 0) {
        LOG_FATAL("MapPolicy '{}' must provide a non-zero max_probe_count", policy->name);
    }

    for (idx_i = 0; idx_i < (sizeof(snapshots) / sizeof(snapshots[0])); idx_i++) {
        u64  length     = snapshots[idx_i].length;
        u64  capacity   = snapshots[idx_i].capacity;
        u64  tombstones = snapshots[idx_i].tombstones;
        size next0      = policy->next_capacity(length, capacity, tombstones, 0);
        size next_same  = policy->next_capacity(length, capacity, tombstones, (size)length);
        size next_more  = policy->next_capacity(length, capacity, tombstones, (size)length + 1);
        (void)policy->should_rehash(length, capacity, tombstones, 0, 0);
        (void)policy->should_rehash(length, capacity, tombstones, 1, policy->max_probe_count);

        if ((next0 == 0) && ((length != 0) || (capacity != 0) || (tombstones != 0))) {
            LOG_FATAL("MapPolicy '{}' returned zero capacity for a non-empty snapshot", policy->name);
        }

        if ((next_same != 0) && (next_same < length)) {
            LOG_FATAL("MapPolicy '{}' returned capacity smaller than current length", policy->name);
        }

        if (next_more < ((size)length + 1)) {
            LOG_FATAL("MapPolicy '{}' returned capacity smaller than requested minimum entries", policy->name);
        }
    }

    for (cap_i = 0; cap_i < (sizeof(capacities) / sizeof(capacities[0])); cap_i++) {
        size capacity = capacities[cap_i];
        u64  hash     = 0x9e3779b97f4a7c15ULL;
        size first    = map_validate_policy_index(policy->first_index(hash, capacity), capacity, "first_index");

        if ((capacity > 1) && (policy->max_probe_count > 1)) {
            size next = map_validate_policy_index(policy->next_index(hash, capacity, first, 1), capacity, "next_index");

            if (next == first) {
                LOG_FATAL("MapPolicy '{}' produced a stuck probe sequence for capacity {}", policy->name, capacity);
            }
        }
    }
}

MapPolicy validate_map_policy_copy(MapPolicy policy) {
    validate_map_policy(&policy);
    return policy;
}

const MapPolicy MapPolicyLinear = {
    .name            = "linear",
    .should_rehash   = default_should_rehash,
    .next_capacity   = default_next_capacity,
    .first_index     = linear_first_index,
    .next_index      = linear_next_index,
    .max_probe_count = 128,
};

const MapPolicy MapPolicyQuadratic = {
    .name            = "quadratic",
    .should_rehash   = default_should_rehash,
    .next_capacity   = default_next_capacity,
    .first_index     = quadratic_first_index,
    .next_index      = quadratic_next_index,
    .max_probe_count = 128,
};

static inline u8 *map_entry_ptr(const GenericMap *map, size entry_size, size idx) {
    return map->entries + (idx * entry_size);
}

static inline void *map_key_ptr(const GenericMap *map, size entry_size, size key_offset, size idx) {
    return map_entry_ptr(map, entry_size, idx) + key_offset;
}

static inline void *map_value_ptr(const GenericMap *map, size entry_size, size value_offset, size idx) {
    return map_entry_ptr(map, entry_size, idx) + value_offset;
}

static inline u64 *map_hash_ptr(const GenericMap *map, size entry_size, size hash_offset, size idx) {
    return (u64 *)(void *)(map_entry_ptr(map, entry_size, idx) + hash_offset);
}

static u64 map_hash_key(const GenericMap *map, const void *key, size key_size) {
    return map->key_hash(key, (u32)key_size);
}

static bool map_keys_equal(const GenericMap *map, size entry_size, size key_offset, size idx, const void *key) {
    return map->key_compare(map_key_ptr(map, entry_size, key_offset, idx), key) == 0;
}

static void map_deinit_slot(GenericMap *map, size entry_size, size key_offset, size value_offset, size idx) {
    if (map->key_copy_deinit) {
        map->key_copy_deinit(map_key_ptr(map, entry_size, key_offset, idx), map->allocator);
    }

    if (map->value_copy_deinit) {
        map->value_copy_deinit(map_value_ptr(map, entry_size, value_offset, idx), map->allocator);
    }

    MemSet(map_entry_ptr(map, entry_size, idx), 0, entry_size);
}

static bool map_copy_into_entry(
    GenericMap *map,
    u8         *entry,
    size        entry_size,
    size        key_offset,
    size        key_size,
    size        value_offset,
    size        value_size,
    size        hash_offset,
    const void *key,
    const void *value,
    u64         hash
) {
    void *dst_key = entry + key_offset;
    void *dst_val = entry + value_offset;

    MemSet(entry, 0, entry_size);

    if (map->key_copy_init) {
        if (!map->key_copy_init(dst_key, key, map->allocator)) {
            if (map->key_copy_deinit) {
                map->key_copy_deinit(dst_key, map->allocator);
            }
            MemSet(entry, 0, entry_size);
            return false;
        }
    } else {
        MemCopy(dst_key, key, key_size);
    }

    if (map->value_copy_init) {
        if (!map->value_copy_init(dst_val, value, map->allocator)) {
            if (map->value_copy_deinit) {
                map->value_copy_deinit(dst_val, map->allocator);
            }
            if (map->key_copy_deinit) {
                map->key_copy_deinit(dst_key, map->allocator);
            }
            MemSet(entry, 0, entry_size);
            return false;
        }
    } else {
        MemCopy(dst_val, value, value_size);
    }

    *(u64 *)(void *)(entry + hash_offset) = hash;
    return true;
}

static void map_scan_slots(
    GenericMap *map,
    const void *key,
    size        entry_size,
    size        key_offset,
    size        key_size,
    size        hash_offset,
    u64         hash,
    size       *first_match_idx,
    size       *insert_idx,
    size       *probe_pressure
) {
    size first_tombstone = map->capacity;
    size first_match     = map->capacity;
    size candidate_empty = map->capacity;
    size idx             = 0;
    size pressure        = 0;
    size probe_count;
    size limit;

    if (!map->capacity) {
        if (first_match_idx) {
            *first_match_idx = 0;
        }
        if (insert_idx) {
            *insert_idx = 0;
        }
        if (probe_pressure) {
            *probe_pressure = 0;
        }
        return;
    }

    limit = map->policy.max_probe_count;
    if (limit == 0) {
        LOG_FATAL("Map policy '{}' has invalid max_probe_count", map->policy.name);
    }

    idx = map_validate_policy_index(map->policy.first_index(hash, map->capacity), map->capacity, "first_index");

    for (probe_count = 0; probe_count < limit; probe_count++) {
        if (probe_count > 0) {
            idx = map_validate_policy_index(
                map->policy.next_index(hash, map->capacity, idx, probe_count),
                map->capacity,
                "next_index"
            );
        }

        pressure = probe_count + 1;

        if (map->states[idx] == MAP_SLOT_EMPTY) {
            candidate_empty = idx;
            break;
        }

        if (map->states[idx] == MAP_SLOT_TOMBSTONE) {
            if (first_tombstone == map->capacity) {
                first_tombstone = idx;
            }
            continue;
        }

        if ((first_match == map->capacity) && (*map_hash_ptr(map, entry_size, hash_offset, idx) == hash) &&
            map_keys_equal(map, entry_size, key_offset, idx, key)) {
            first_match = idx;
        }
    }

    if (first_match_idx) {
        *first_match_idx = first_match;
    }

    if (insert_idx) {
        if (candidate_empty < map->capacity) {
            *insert_idx = first_tombstone < map->capacity ? first_tombstone : candidate_empty;
        } else {
            *insert_idx = first_tombstone;
        }
    }

    if (probe_pressure) {
        *probe_pressure = pressure;
    }
}

// Returns true on success. Returns false if the probe budget
// (max_probe_count) was exhausted before finding an empty / tombstone
// slot -- the caller (rehash_map) responds by growing the table and
// retrying the entire rehash. This is not a corruption case; long
// linear-probe clusters can defeat a fixed probe budget even on a
// table sized correctly for load factor.
static bool map_insert_raw_entry(
    GenericMap *map,
    const void *entry,
    size        entry_size,
    size        key_offset,
    size        key_size,
    size        hash_offset
) {
    u64  hash       = *(const u64 *)(const void *)((Zstr)entry + hash_offset);
    size insert_idx = map->capacity;

    map_scan_slots(
        map,
        (Zstr)entry + key_offset,
        entry_size,
        key_offset,
        key_size,
        hash_offset,
        hash,
        NULL,
        &insert_idx,
        NULL
    );

    if (insert_idx >= map->capacity) {
        return false;
    }

    if (map->states[insert_idx] == MAP_SLOT_TOMBSTONE) {
        map->tombstones -= 1;
    }

    MemCopy(map_entry_ptr(map, entry_size, insert_idx), entry, entry_size);
    map->states[insert_idx]  = MAP_SLOT_OCCUPIED;
    map->length             += 1;
    return true;
}

void validate_map(const GenericMap *map) {
    if (!map) {
        LOG_FATAL("Expected a valid Map pointer");
    }

    if (map->__magic != MAP_MAGIC) {
        LOG_FATAL("Map is uninitialized or corrupted");
    }

    if (!map->key_compare || !map->key_hash) {
        LOG_FATAL("Map must have valid key compare and key hash callbacks");
    }

    // Maps have no stack-init form; a NULL allocator means the handle
    // is corrupted between magic check and use. Surface that before
    // dereferencing the method table below.
    if (!map->allocator) {
        LOG_FATAL("Map allocator pointer is NULL");
    }

    if (!map->allocator->allocate || !map->allocator->resize || !map->allocator->remap || !map->allocator->deallocate) {
        LOG_FATAL("Map allocator is invalid");
    }

    validate_map_policy(&map->policy);

    if (map->length > map->capacity) {
        LOG_FATAL("Map length cannot exceed capacity");
    }

    if ((map->length + map->tombstones) > map->capacity) {
        LOG_FATAL("Map occupied slots and tombstones cannot exceed capacity");
    }

    if (!map->capacity) {
        if (map->entries || map->states) {
            LOG_FATAL("Map with zero capacity must not have allocated storage");
        }
        return;
    }

    if (!map->entries || !map->states) {
        LOG_FATAL("Map storage is corrupted");
    }
}

void deinit_map(
    GenericMap *map,
    size        entry_size,
    size        key_offset,
    size        key_size,
    size        value_offset,
    size        value_size,
    size        hash_offset
) {
    ValidateMap(map);

    clear_map(map, entry_size, key_offset, key_size, value_offset, value_size, hash_offset);

    AllocatorFree(map->allocator, map->entries);
    AllocatorFree(map->allocator, map->states);

    MemSet(map, 0, sizeof(*map));
}

void clear_map(
    GenericMap *map,
    size        entry_size,
    size        key_offset,
    size        key_size,
    size        value_offset,
    size        value_size,
    size        hash_offset
) {
    size idx;

    ValidateMap(map);

    for (idx = 0; idx < map->capacity; idx++) {
        if (map->states[idx] != MAP_SLOT_OCCUPIED) {
            continue;
        }

        map_deinit_slot(map, entry_size, key_offset, value_offset, idx);
        map->states[idx] = MAP_SLOT_EMPTY;
    }

    map->length     = 0;
    map->tombstones = 0;

    (void)key_size;
    (void)value_size;
    (void)hash_offset;
}

bool rehash_map(
    GenericMap *map,
    size        entry_size,
    size        key_offset,
    size        key_size,
    size        value_offset,
    size        value_size,
    size        hash_offset,
    size        n,
    MapPolicy   policy
) {
    u8 *old_entries;
    u8 *old_states;
    u8 *new_entries;
    u8 *new_states;
    size  old_capacity;
    size  new_capacity;
    size  idx;
    ValidateMap(map);
    validate_map_policy(&policy);

    if ((map->length == 0) && (n == 0)) {
        AllocatorFree(map->allocator, map->entries);
        AllocatorFree(map->allocator, map->states);
        map->entries    = NULL;
        map->states     = NULL;
        map->length     = 0;
        map->capacity   = 0;
        map->tombstones = 0;
        map->policy     = policy;
        return true;
    }

    new_capacity = policy.next_capacity(map->length, map->capacity, map->tombstones, n);

    if (new_capacity < (n > map->length ? n : (size)map->length)) {
        LOG_FATAL("Map policy '{}' returned insufficient capacity {}", policy.name, new_capacity);
    }

    old_entries  = map->entries;
    old_states   = map->states;
    old_capacity = map->capacity;

    // Try to rehash into `new_capacity`. If any re-insert exhausts the
    // probe budget (long cluster), free the new table, double the
    // target capacity, and try again. Long linear-probe clusters can
    // defeat a 128-slot probe budget even when the load factor is
    // healthy -- doubling capacity halves expected cluster length
    // and is the standard fix.
    for (;;) {
        new_entries = AllocatorAlloc(map->allocator, new_capacity * entry_size, true);
        new_states  = AllocatorAlloc(map->allocator, new_capacity * sizeof(u8), true);

        if (!new_entries || !new_states) {
            AllocatorFree(map->allocator, new_entries);
            AllocatorFree(map->allocator, new_states);
            // Restore the original table so the map stays usable.
            map->entries  = old_entries;
            map->states   = old_states;
            map->capacity = old_capacity;
            return false;
        }

        map->entries    = new_entries;
        map->states     = new_states;
        map->capacity   = new_capacity;
        map->length     = 0;
        map->tombstones = 0;
        map->policy     = policy;

        bool ok = true;
        for (idx = 0; idx < old_capacity; idx++) {
            if (!old_states || old_states[idx] != MAP_SLOT_OCCUPIED) {
                continue;
            }
            if (!map_insert_raw_entry(
                    map,
                    old_entries + (idx * entry_size),
                    entry_size,
                    key_offset,
                    key_size,
                    hash_offset
                )) {
                ok = false;
                break;
            }
        }

        if (ok)
            break;

        // Probe-budget exhausted. Free the failed new table, double
        // capacity, retry.
        AllocatorFree(map->allocator, new_entries);
        AllocatorFree(map->allocator, new_states);
        size next_cap = new_capacity * 2;
        if (next_cap <= new_capacity) {
            // Overflow / no headroom. Restore and fail.
            map->entries  = old_entries;
            map->states   = old_states;
            map->capacity = old_capacity;
            return false;
        }
        new_capacity = next_cap;
    }

    AllocatorFree(map->allocator, old_entries);
    AllocatorFree(map->allocator, old_states);

    (void)value_offset;
    (void)value_size;
    return true;
}

bool reserve_map(
    GenericMap *map,
    size        entry_size,
    size        key_offset,
    size        key_size,
    size        value_offset,
    size        value_size,
    size        hash_offset,
    size        n
) {
    size target_capacity;

    ValidateMap(map);

    target_capacity = map->policy.next_capacity(map->length, map->capacity, map->tombstones, n);

    if ((target_capacity == map->capacity) &&
        !map->policy.should_rehash(map->length, map->capacity, map->tombstones, 0, 0)) {
        return true;
    }

    return rehash_map(map, entry_size, key_offset, key_size, value_offset, value_size, hash_offset, n, map->policy);
}

size map_find_index(
    GenericMap *map,
    const void *key,
    size        entry_size,
    size        key_offset,
    size        key_size,
    size        hash_offset
) {
    size found_idx = 0;
    u64  hash;

    ValidateMap(map);

    if (!map->capacity) {
        return 0;
    }

    hash = map_hash_key(map, key, key_size);
    map_scan_slots(map, key, entry_size, key_offset, key_size, hash_offset, hash, &found_idx, NULL, NULL);

    return found_idx;
}

bool map_contains(GenericMap *map, const void *key, size entry_size, size key_offset, size key_size, size hash_offset) {
    ValidateMap(map);

    return map->capacity && (map_find_index(map, key, entry_size, key_offset, key_size, hash_offset) < map->capacity);
}

bool map_contains_pair(
    GenericMap *map,
    const void *key,
    const void *value,
    size        entry_size,
    size        key_offset,
    size        key_size,
    size        value_offset,
    size        hash_offset
) {
    size idx;

    ValidateMap(map);

    if (!map->value_compare) {
        LOG_FATAL("MapContainsPair requires a value comparator");
    }

    if (!map->capacity) {
        return false;
    }

    idx = map_find_index(map, key, entry_size, key_offset, key_size, hash_offset);
    while (idx < map->capacity) {
        if (map->value_compare(map_value_ptr(map, entry_size, value_offset, idx), value) == 0) {
            return true;
        }
        idx = map_find_next_index(map, key, idx, entry_size, key_offset, key_size, hash_offset);
    }

    return false;
}

size map_unique_key_count(GenericMap *map, size entry_size, size key_offset, size key_size, size hash_offset) {
    size idx;
    size count = 0;

    ValidateMap(map);

    for (idx = 0; idx < map->capacity; idx++) {
        if ((map->states[idx] != MAP_SLOT_OCCUPIED) || (map_find_index(
                                                            map,
                                                            map_key_ptr(map, entry_size, key_offset, idx),
                                                            entry_size,
                                                            key_offset,
                                                            key_size,
                                                            hash_offset
                                                        ) != idx)) {
            continue;
        }

        count += 1;
    }

    return count;
}

size map_find_next_index(
    GenericMap *map,
    const void *key,
    size        previous_index,
    size        entry_size,
    size        key_offset,
    size        key_size,
    size        hash_offset
) {
    size idx = 0;
    size probe_count;
    size limit;
    u64  hash;
    bool previous_seen = false;

    ValidateMap(map);

    if (!map->capacity || previous_index >= map->capacity) {
        return map->capacity;
    }

    limit = map->policy.max_probe_count;
    hash  = map_hash_key(map, key, key_size);
    idx   = map_validate_policy_index(map->policy.first_index(hash, map->capacity), map->capacity, "first_index");

    for (probe_count = 0; probe_count < limit; probe_count++) {
        if (probe_count > 0) {
            idx = map_validate_policy_index(
                map->policy.next_index(hash, map->capacity, idx, probe_count),
                map->capacity,
                "next_index"
            );
        }

        if (idx == previous_index) {
            previous_seen = true;
        }

        if (map->states[idx] == MAP_SLOT_EMPTY) {
            return map->capacity;
        }

        if ((map->states[idx] == MAP_SLOT_OCCUPIED) && previous_seen &&
            (*map_hash_ptr(map, entry_size, hash_offset, idx) == hash) &&
            map_keys_equal(map, entry_size, key_offset, idx, key) && (idx != previous_index)) {
            return idx;
        }
    }

    return map->capacity;
}

size map_value_count(
    GenericMap *map,
    const void *key,
    size        entry_size,
    size        key_offset,
    size        key_size,
    size        hash_offset
) {
    size idx;
    size count = 0;

    ValidateMap(map);

    if (!map->capacity) {
        return 0;
    }

    idx = map_find_index(map, key, entry_size, key_offset, key_size, hash_offset);
    while (idx < map->capacity) {
        count += 1;
        idx    = map_find_next_index(map, key, idx, entry_size, key_offset, key_size, hash_offset);
    }

    return count;
}

void *map_get_value_ptr(
    GenericMap *map,
    const void *key,
    size        entry_size,
    size        key_offset,
    size        key_size,
    size        value_offset,
    size        hash_offset
) {
    size idx;

    ValidateMap(map);

    if (!map->capacity) {
        return NULL;
    }

    idx = map_find_index(map, key, entry_size, key_offset, key_size, hash_offset);
    if (idx >= map->capacity) {
        return NULL;
    }

    return map_value_ptr(map, entry_size, value_offset, idx);
}

void *map_get_value_or_default(
    GenericMap *map,
    const void *key,
    const void *default_value,
    size        entry_size,
    size        key_offset,
    size        key_size,
    size        value_offset,
    size        value_size,
    size        hash_offset,
    void       *out_value
) {
    void *value_ptr = NULL;

    ValidateMap(map);

    if (!key || !default_value || !out_value) {
        LOG_FATAL("Invalid arguments");
    }

    value_ptr = map_get_value_ptr(map, key, entry_size, key_offset, key_size, value_offset, hash_offset);
    if (value_ptr) {
        MemCopy(out_value, value_ptr, value_size);
    } else {
        MemCopy(out_value, default_value, value_size);
    }

    return out_value;
}

void *map_ensure_value_ptr(
    GenericMap *map,
    const void *key,
    const void *value,
    size        entry_size,
    size        key_offset,
    size        key_size,
    size        value_offset,
    size        value_size,
    size        hash_offset
) {
    void *value_ptr = map_get_value_ptr(map, key, entry_size, key_offset, key_size, value_offset, hash_offset);

    if (value_ptr) {
        return value_ptr;
    }

    if (!map_insert(map, key, value, entry_size, key_offset, key_size, value_offset, value_size, hash_offset)) {
        return NULL;
    }
    return map_get_value_ptr(map, key, entry_size, key_offset, key_size, value_offset, hash_offset);
}

MapValueCursor map_find_first_cursor(
    GenericMap *map,
    const void *key,
    size        entry_size,
    size        key_offset,
    size        key_size,
    size        hash_offset
) {
    size idx = map_find_index(map, key, entry_size, key_offset, key_size, hash_offset);

    return (MapValueCursor) {.__index = idx < map->capacity ? idx : (size)-1};
}

MapValueCursor map_find_next_cursor(
    GenericMap    *map,
    const void    *key,
    MapValueCursor cursor,
    size           entry_size,
    size           key_offset,
    size           key_size,
    size           hash_offset
) {
    size idx;

    ValidateMap(map);

    if (cursor.__index == (size)-1) {
        return cursor;
    }

    idx = map_find_next_index(map, key, cursor.__index, entry_size, key_offset, key_size, hash_offset);
    return (MapValueCursor) {.__index = idx < map->capacity ? idx : (size)-1};
}

void *map_value_ptr_from_cursor(GenericMap *map, MapValueCursor cursor, size entry_size, size value_offset) {
    ValidateMap(map);

    if (cursor.__index == (size)-1 || cursor.__index >= map->capacity ||
        map->states[cursor.__index] != MAP_SLOT_OCCUPIED) {
        return NULL;
    }

    return map_value_ptr(map, entry_size, value_offset, cursor.__index);
}

bool map_insert(
    GenericMap *map,
    const void *key,
    const void *value,
    size        entry_size,
    size        key_offset,
    size        key_size,
    size        value_offset,
    size        value_size,
    size        hash_offset
) {
    size insert_idx     = 0;
    size probe_pressure = 0;
    u64  hash;

    ValidateMap(map);

    if (map->capacity == 0) {
        if (!rehash_map(map, entry_size, key_offset, key_size, value_offset, value_size, hash_offset, 1, map->policy)) {
            return false;
        }
    }

    if (map->policy.should_rehash(map->length, map->capacity, map->tombstones, 1, 0)) {
        if (!rehash_map(
                map,
                entry_size,
                key_offset,
                key_size,
                value_offset,
                value_size,
                hash_offset,
                map->length + 1,
                map->policy
            )) {
            return false;
        }
    }

    hash = map_hash_key(map, key, key_size);

    // Scan; on probe-budget exhaustion, rehash to a larger capacity
    // and retry. Bounded to 32 attempts.
    for (int attempt = 0; attempt < 32; attempt++) {
        insert_idx     = 0;
        probe_pressure = 0;
        map_scan_slots(
            map,
            key,
            entry_size,
            key_offset,
            key_size,
            hash_offset,
            hash,
            NULL,
            &insert_idx,
            &probe_pressure
        );

        if (insert_idx < map->capacity) {
            break;
        }

        // Probe budget exhausted. Pass n=capacity+1 so next_capacity
        // grows the table; rehash_map itself further doubles
        // internally if the new size still can't fit existing entries.
        (void)map->policy.should_rehash(map->length, map->capacity, map->tombstones, 1, probe_pressure);

        size forced_n = map->capacity + 1;
        if (forced_n < map->length + 1) {
            forced_n = map->length + 1;
        }
        if (!rehash_map(
                map,
                entry_size,
                key_offset,
                key_size,
                value_offset,
                value_size,
                hash_offset,
                forced_n,
                map->policy
            )) {
            return false;
        }
    }

    if (insert_idx >= map->capacity) {
        LOG_FATAL("map_insert: probe budget exhausted after 32 rehash attempts (capacity {})", map->capacity);
    }

    if (map->states[insert_idx] == MAP_SLOT_TOMBSTONE) {
        map->tombstones -= 1;
    }

    if (!map_copy_into_entry(
            map,
            map_entry_ptr(map, entry_size, insert_idx),
            entry_size,
            key_offset,
            key_size,
            value_offset,
            value_size,
            hash_offset,
            key,
            value,
            hash
        )) {
        if (map->states[insert_idx] == MAP_SLOT_TOMBSTONE) {
            map->tombstones += 1;
        }
        return false;
    }
    map->states[insert_idx]  = MAP_SLOT_OCCUPIED;
    map->length             += 1;
    return true;
}

bool map_set_only(
    GenericMap *map,
    const void *key,
    const void *value,
    size        entry_size,
    size        key_offset,
    size        key_size,
    size        value_offset,
    size        value_size,
    size        hash_offset
) {
    u8   *temp_entry;
    size  existing_idx;
    u64   hash;

    ValidateMap(map);

    existing_idx = map_find_index(map, key, entry_size, key_offset, key_size, hash_offset);
    if (existing_idx >= map->capacity) {
        return map_insert(map, key, value, entry_size, key_offset, key_size, value_offset, value_size, hash_offset);
    }

    hash       = map_hash_key(map, key, key_size);
    temp_entry = AllocatorAlloc(map->allocator, entry_size, true);
    if (!temp_entry) {
        return false;
    }

    if (!map_copy_into_entry(
            map,
            temp_entry,
            entry_size,
            key_offset,
            key_size,
            value_offset,
            value_size,
            hash_offset,
            key,
            value,
            hash
        )) {
        AllocatorFree(map->allocator, temp_entry);
        return false;
    }

    (void)map_remove_all(map, key, entry_size, key_offset, key_size, value_offset, value_size, hash_offset);
    // map_insert_raw_entry can fail if the probe budget is exhausted in the
    // freshly-cleared region (rare, but possible on pathological key sets).
    // The temp_entry holds deep-copied payloads; on failure deinit them so
    // the allocator doesn't leak Strs / sub-vecs the copy_init produced.
    if (!map_insert_raw_entry(map, temp_entry, entry_size, key_offset, key_size, hash_offset)) {
        if (map->key_copy_deinit) {
            map->key_copy_deinit(temp_entry + key_offset, map->allocator);
        }
        if (map->value_copy_deinit) {
            map->value_copy_deinit(temp_entry + value_offset, map->allocator);
        }
        AllocatorFree(map->allocator, temp_entry);
        return false;
    }
    AllocatorFree(map->allocator, temp_entry);
    return true;
}

bool map_set_first(
    GenericMap *map,
    const void *key,
    const void *value,
    size        entry_size,
    size        key_offset,
    size        key_size,
    size        value_offset,
    size        value_size,
    size        hash_offset
) {
    size  idx;
    void *dst_value;
    void *temp_value;

    ValidateMap(map);

    if (!map->capacity) {
        return false;
    }

    idx = map_find_index(map, key, entry_size, key_offset, key_size, hash_offset);
    if (idx >= map->capacity) {
        return false;
    }

    dst_value  = map_value_ptr(map, entry_size, value_offset, idx);
    temp_value = NULL;

    if (map->value_copy_init) {
        temp_value = AllocatorAlloc(map->allocator, value_size, true);
        if (!temp_value) {
            return false;
        }

        if (!map->value_copy_init(temp_value, value, map->allocator)) {
            if (map->value_copy_deinit) {
                map->value_copy_deinit(temp_value, map->allocator);
            }
            AllocatorFree(map->allocator, temp_value);
            return false;
        }
    }

    if (map->value_copy_deinit) {
        map->value_copy_deinit(dst_value, map->allocator);
    }

    MemSet(dst_value, 0, value_size);

    if (map->value_copy_init) {
        MemCopy(dst_value, temp_value, value_size);
        AllocatorFree(map->allocator, temp_value);
    } else {
        MemCopy(dst_value, value, value_size);
    }

    return true;
}

static void map_remove_at_index(
    GenericMap *map,
    size        idx,
    size        entry_size,
    size        key_offset,
    size        key_size,
    size        value_offset,
    size        value_size
) {
    if (map->key_copy_deinit) {
        map->key_copy_deinit(map_key_ptr(map, entry_size, key_offset, idx), map->allocator);
    } else {
        (void)key_size;
    }

    if (map->value_copy_deinit) {
        map->value_copy_deinit(map_value_ptr(map, entry_size, value_offset, idx), map->allocator);
    } else {
        (void)value_size;
    }

    MemSet(map_entry_ptr(map, entry_size, idx), 0, entry_size);
    map->states[idx]  = MAP_SLOT_TOMBSTONE;
    map->length      -= 1;
    map->tombstones  += 1;
}

bool map_remove(
    GenericMap *map,
    const void *key,
    size        entry_size,
    size        key_offset,
    size        key_size,
    size        value_offset,
    size        value_size,
    size        hash_offset
) {
    size idx;

    ValidateMap(map);

    if (!map->capacity) {
        return false;
    }

    idx = map_find_index(map, key, entry_size, key_offset, key_size, hash_offset);
    if (idx >= map->capacity) {
        return false;
    }

    map_remove_at_index(map, idx, entry_size, key_offset, key_size, value_offset, value_size);
    return true;
}

bool map_remove_pair(
    GenericMap *map,
    const void *key,
    const void *value,
    size        entry_size,
    size        key_offset,
    size        key_size,
    size        value_offset,
    size        value_size,
    size        hash_offset
) {
    size idx;

    ValidateMap(map);

    if (!map->value_compare) {
        LOG_FATAL("MapRemovePair requires a value comparator");
    }

    if (!map->capacity) {
        return false;
    }

    idx = map_find_index(map, key, entry_size, key_offset, key_size, hash_offset);
    while (idx < map->capacity) {
        if (map->value_compare(map_value_ptr(map, entry_size, value_offset, idx), value) == 0) {
            map_remove_at_index(map, idx, entry_size, key_offset, key_size, value_offset, value_size);
            return true;
        }
        idx = map_find_next_index(map, key, idx, entry_size, key_offset, key_size, hash_offset);
    }

    return false;
}

size map_remove_all(
    GenericMap *map,
    const void *key,
    size        entry_size,
    size        key_offset,
    size        key_size,
    size        value_offset,
    size        value_size,
    size        hash_offset
) {
    size idx;
    size removed = 0;

    ValidateMap(map);

    if (!map->capacity) {
        return 0;
    }

    idx = map_find_index(map, key, entry_size, key_offset, key_size, hash_offset);
    while (idx < map->capacity) {
        size previous_idx = idx;

        map_remove_at_index(map, idx, entry_size, key_offset, key_size, value_offset, value_size);
        removed += 1;
        idx      = map_find_next_index(map, key, previous_idx, entry_size, key_offset, key_size, hash_offset);
    }

    return removed;
}

size map_remove_if(
    GenericMap    *map,
    MapPredicateFn predicate,
    void          *ctx,
    size           entry_size,
    size           key_offset,
    size           key_size,
    size           value_offset,
    size           value_size
) {
    size idx;
    size removed = 0;

    ValidateMap(map);

    if (!predicate) {
        LOG_FATAL("MapRemoveIf requires a predicate");
    }

    for (idx = 0; idx < map->capacity; idx++) {
        if ((map->states[idx] != MAP_SLOT_OCCUPIED) || !predicate(
                                                           map_key_ptr(map, entry_size, key_offset, idx),
                                                           map_value_ptr(map, entry_size, value_offset, idx),
                                                           ctx
                                                       )) {
            continue;
        }

        map_remove_at_index(map, idx, entry_size, key_offset, key_size, value_offset, value_size);
        removed += 1;
    }

    return removed;
}

size map_retain_if(
    GenericMap    *map,
    MapPredicateFn predicate,
    void          *ctx,
    size           entry_size,
    size           key_offset,
    size           key_size,
    size           value_offset,
    size           value_size
) {
    size idx;
    size removed = 0;

    ValidateMap(map);

    if (!predicate) {
        LOG_FATAL("MapRetainIf requires a predicate");
    }

    for (idx = 0; idx < map->capacity; idx++) {
        if ((map->states[idx] != MAP_SLOT_OCCUPIED) || predicate(
                                                           map_key_ptr(map, entry_size, key_offset, idx),
                                                           map_value_ptr(map, entry_size, value_offset, idx),
                                                           ctx
                                                       )) {
            continue;
        }

        map_remove_at_index(map, idx, entry_size, key_offset, key_size, value_offset, value_size);
        removed += 1;
    }

    return removed;
}
