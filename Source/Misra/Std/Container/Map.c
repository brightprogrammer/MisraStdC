/// file      : std/map.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Generic map implementation

#include <Misra/Std/Container/Map.h>
#include <Misra/Std/Log.h>
#include <Misra/Sys.h>

#include <stdlib.h>
#include <string.h>

enum {
    MAP_SLOT_EMPTY     = 0,
    MAP_SLOT_OCCUPIED  = 1,
    MAP_SLOT_TOMBSTONE = 2,
};

static size linear_probe_index(u64 hash, size probe_count, size capacity) {
    (void)capacity;
    return (size)(hash + probe_count);
}

static size quadratic_probe_index(u64 hash, size probe_count, size capacity) {
    (void)capacity;
    return (size)(hash + ((probe_count * (probe_count + 1)) / 2));
}

static bool default_should_rehash(const GenericMap *map) {
    return map->capacity == 0 || ((map->length * 4) >= (map->capacity * 3));
}

const MapPolicy MisraMapPolicyLinear = {
    .name          = "linear",
    .probe_index   = linear_probe_index,
    .should_rehash = default_should_rehash,
};

const MapPolicy MisraMapPolicyQuadratic = {
    .name          = "quadratic",
    .probe_index   = quadratic_probe_index,
    .should_rehash = default_should_rehash,
};

static size map_capacity_for_entries(size n) {
    size capacity = 8;
    size needed   = n == 0 ? 8 : n;

    while (((capacity * 3) / 4) < needed) {
        capacity <<= 1;
    }

    return capacity;
}

static inline char *map_entry_ptr(const GenericMap *map, size entry_size, size idx) {
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

static inline size map_probe_slot(const MapPolicy *policy, u64 hash, size probe_count, size capacity) {
    if (!capacity) {
        return 0;
    }

    return policy->probe_index(hash, probe_count, capacity) % capacity;
}

static u64 map_hash_key(const GenericMap *map, const void *key, size key_size) {
    return map->key_hash(key, (u32)key_size);
}

static bool map_keys_equal(
    const GenericMap *map,
    size                  entry_size,
    size                  key_offset,
    size                  idx,
    const void           *key
) {
    return map->key_compare(map_key_ptr(map, entry_size, key_offset, idx), key) == 0;
}

static void map_deinit_slot(
    GenericMap *map,
    size            entry_size,
    size            key_offset,
    size            value_offset,
    size            idx
) {
    if (map->key_copy_deinit) {
        map->key_copy_deinit(map_key_ptr(map, entry_size, key_offset, idx));
    }

    if (map->value_copy_deinit) {
        map->value_copy_deinit(map_value_ptr(map, entry_size, value_offset, idx));
    }

    memset(map_entry_ptr(map, entry_size, idx), 0, entry_size);
}

static void map_copy_into_slot(
    GenericMap *map,
    size            entry_size,
    size            key_offset,
    size            key_size,
    size            value_offset,
    size            value_size,
    size            hash_offset,
    size            idx,
    const void     *key,
    const void     *value,
    u64             hash
) {
    char *entry   = map_entry_ptr(map, entry_size, idx);
    void *dst_key = entry + key_offset;
    void *dst_val = entry + value_offset;

    memset(entry, 0, entry_size);

    if (map->key_copy_init) {
        map->key_copy_init(dst_key, (void *)key);
    } else {
        memcpy(dst_key, key, key_size);
    }

    if (map->value_copy_init) {
        map->value_copy_init(dst_val, (void *)value);
    } else {
        memcpy(dst_val, value, value_size);
    }

    *map_hash_ptr(map, entry_size, hash_offset, idx) = hash;
}

static bool map_find_slot(
    GenericMap *map,
    const void     *key,
    size            entry_size,
    size            key_offset,
    size            key_size,
    size            hash_offset,
    u64             hash,
    size           *found_idx,
    size           *insert_idx
) {
    size first_tombstone = map->capacity;
    size probe_count;
    size idx;

    if (!map->capacity) {
        if (found_idx) {
            *found_idx = 0;
        }
        if (insert_idx) {
            *insert_idx = 0;
        }
        return false;
    }

    for (probe_count = 0; probe_count < map->capacity; probe_count++) {
        idx = map_probe_slot(&map->policy, hash, probe_count, map->capacity);

        if (map->states[idx] == MAP_SLOT_EMPTY) {
            if (found_idx) {
                *found_idx = idx;
            }
            if (insert_idx) {
                *insert_idx = first_tombstone < map->capacity ? first_tombstone : idx;
            }
            return false;
        }

        if (map->states[idx] == MAP_SLOT_TOMBSTONE) {
            if (first_tombstone == map->capacity) {
                first_tombstone = idx;
            }
            continue;
        }

        if ((*map_hash_ptr(map, entry_size, hash_offset, idx) == hash) &&
            map_keys_equal(map, entry_size, key_offset, idx, key)) {
            if (found_idx) {
                *found_idx = idx;
            }
            if (insert_idx) {
                *insert_idx = idx;
            }
            return true;
        }
    }

    if (found_idx) {
        *found_idx = map->capacity;
    }

    if (insert_idx) {
        *insert_idx = first_tombstone;
    }

    return false;
}

static void map_insert_raw_entry(
    GenericMap *map,
    const void     *entry,
    size            entry_size,
    size            key_offset,
    size            key_size,
    size            hash_offset
) {
    u64  hash       = *(const u64 *)(const void *)((const char *)entry + hash_offset);
    size insert_idx = map->capacity;
    bool found;

    found = map_find_slot(
        map,
        (const char *)entry + key_offset,
        entry_size,
        key_offset,
        key_size,
        hash_offset,
        hash,
        NULL,
        &insert_idx
    );

    if (found || insert_idx >= map->capacity) {
        LOG_FATAL("Failed to insert raw map entry during rehash");
    }

    memcpy(map_entry_ptr(map, entry_size, insert_idx), entry, entry_size);
    map->states[insert_idx] = MAP_SLOT_OCCUPIED;
    map->length += 1;
}

void validate_map(const GenericMap *map) {
    if (!map) {
        LOG_FATAL("Expected a valid Map pointer");
    }

    if (map->__magic != MISRA_MAP_MAGIC) {
        LOG_FATAL("Map is uninitialized or corrupted");
    }

    if (!map->key_compare || !map->key_hash) {
        LOG_FATAL("Map must have valid key compare and key hash callbacks");
    }

    if (!map->policy.probe_index || !map->policy.should_rehash) {
        LOG_FATAL("Map policy must provide valid probe and rehash callbacks");
    }

    if (map->length > map->capacity) {
        LOG_FATAL("Map length cannot exceed capacity");
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
    size            entry_size,
    size            key_offset,
    size            key_size,
    size            value_offset,
    size            value_size,
    size            hash_offset
) {
    ValidateMap(map);

    clear_map(map, entry_size, key_offset, key_size, value_offset, value_size, hash_offset);

    free(map->entries);
    free(map->states);

    map->entries            = NULL;
    map->states             = NULL;
    map->length             = 0;
    map->capacity           = 0;
    map->key_copy_init      = NULL;
    map->key_copy_deinit    = NULL;
    map->value_copy_init    = NULL;
    map->value_copy_deinit  = NULL;
    map->key_compare        = NULL;
    map->key_hash           = NULL;
    map->policy.name          = NULL;
    map->policy.probe_index   = NULL;
    map->policy.should_rehash = NULL;
    map->__magic            = 0;
}

void clear_map(
    GenericMap *map,
    size            entry_size,
    size            key_offset,
    size            key_size,
    size            value_offset,
    size            value_size,
    size            hash_offset
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

    map->length = 0;

    (void)key_size;
    (void)value_size;
    (void)hash_offset;
}

void rehash_map(
    GenericMap *map,
    size            entry_size,
    size            key_offset,
    size            key_size,
    size            value_offset,
    size            value_size,
    size            hash_offset,
    size            n,
    MapPolicy   policy
) {
    char *old_entries;
    u8   *old_states;
    size  old_capacity;
    size  new_capacity;
    size  idx;

    ValidateMap(map);

    if (!policy.probe_index || !policy.should_rehash) {
        LOG_FATAL("Map rehash requires a valid policy");
    }

    if ((map->length == 0) && (n == 0)) {
        free(map->entries);
        free(map->states);
        map->entries  = NULL;
        map->states   = NULL;
        map->capacity = 0;
        map->policy   = policy;
        return;
    }

    new_capacity = map_capacity_for_entries(n > map->length ? n : map->length);

    if ((new_capacity == map->capacity) &&
        (map->policy.probe_index == policy.probe_index) &&
        (map->policy.should_rehash == policy.should_rehash) &&
        (map->policy.name == policy.name)) {
        return;
    }

    old_entries  = map->entries;
    old_states   = map->states;
    old_capacity = map->capacity;

    map->entries = calloc(new_capacity, entry_size);
    map->states  = calloc(new_capacity, sizeof(u8));

    if (!map->entries || !map->states) {
        free(map->entries);
        free(map->states);
        map->entries  = old_entries;
        map->states   = old_states;
        map->capacity = old_capacity;
        LOG_SYS_FATAL("calloc() failed");
    }

    map->capacity = new_capacity;
    map->length   = 0;
    map->policy   = policy;

    for (idx = 0; idx < old_capacity; idx++) {
        if (!old_states || old_states[idx] != MAP_SLOT_OCCUPIED) {
            continue;
        }

        map_insert_raw_entry(
            map,
            old_entries + (idx * entry_size),
            entry_size,
            key_offset,
            key_size,
            hash_offset
        );
    }

    free(old_entries);
    free(old_states);

    (void)value_offset;
    (void)value_size;
}

void reserve_map(
    GenericMap *map,
    size            entry_size,
    size            key_offset,
    size            key_size,
    size            value_offset,
    size            value_size,
    size            hash_offset,
    size            n
) {
    ValidateMap(map);

    if ((n <= map->capacity) && (((map->capacity * 3) / 4) >= n)) {
        return;
    }

    rehash_map(
        map,
        entry_size,
        key_offset,
        key_size,
        value_offset,
        value_size,
        hash_offset,
        n,
        map->policy
    );
}

size map_find_index(
    GenericMap *map,
    const void     *key,
    size            entry_size,
    size            key_offset,
    size            key_size,
    size            hash_offset
) {
    size found_idx = 0;
    u64  hash;

    ValidateMap(map);

    if (!map->capacity) {
        return 0;
    }

    hash = map_hash_key(map, key, key_size);
    if (map_find_slot(map, key, entry_size, key_offset, key_size, hash_offset, hash, &found_idx, NULL)) {
        return found_idx;
    }

    return map->capacity;
}

bool map_contains(
    GenericMap *map,
    const void     *key,
    size            entry_size,
    size            key_offset,
    size            key_size,
    size            hash_offset
) {
    ValidateMap(map);

    return map->capacity && (map_find_index(map, key, entry_size, key_offset, key_size, hash_offset) < map->capacity);
}

void *map_get_value_ptr(
    GenericMap *map,
    const void     *key,
    size            entry_size,
    size            key_offset,
    size            key_size,
    size            value_offset,
    size            hash_offset
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

void map_insert(
    GenericMap *map,
    const void     *key,
    const void     *value,
    size            entry_size,
    size            key_offset,
    size            key_size,
    size            value_offset,
    size            value_size,
    size            hash_offset,
    bool            replace_existing
) {
    size found_idx  = 0;
    size insert_idx = 0;
    bool found;
    u64  hash;

    ValidateMap(map);

    if (map->capacity == 0) {
        reserve_map(
            map,
            entry_size,
            key_offset,
            key_size,
            value_offset,
            value_size,
            hash_offset,
            1
        );
    } else if (map->policy.should_rehash(map)) {
        rehash_map(
            map,
            entry_size,
            key_offset,
            key_size,
            value_offset,
            value_size,
            hash_offset,
            map->length + 1,
            map->policy
        );
    }

    hash = map_hash_key(map, key, key_size);
    found = map_find_slot(
        map,
        key,
        entry_size,
        key_offset,
        key_size,
        hash_offset,
        hash,
        &found_idx,
        &insert_idx
    );

    if (found) {
        if (!replace_existing) {
            LOG_FATAL("Attempt to insert duplicate key into Map");
        }

        map_deinit_slot(map, entry_size, key_offset, value_offset, found_idx);
        map_copy_into_slot(
            map,
            entry_size,
            key_offset,
            key_size,
            value_offset,
            value_size,
            hash_offset,
            found_idx,
            key,
            value,
            hash
        );
        map->states[found_idx] = MAP_SLOT_OCCUPIED;
        return;
    }

    if (insert_idx >= map->capacity) {
        rehash_map(
            map,
            entry_size,
            key_offset,
            key_size,
            value_offset,
            value_size,
            hash_offset,
            map->length + 1,
            map->policy
        );
        map_insert(
            map,
            key,
            value,
            entry_size,
            key_offset,
            key_size,
            value_offset,
            value_size,
            hash_offset,
            replace_existing
        );
        return;
    }

    map_copy_into_slot(
        map,
        entry_size,
        key_offset,
        key_size,
        value_offset,
        value_size,
        hash_offset,
        insert_idx,
        key,
        value,
        hash
    );
    map->states[insert_idx] = MAP_SLOT_OCCUPIED;
    map->length += 1;
}

bool map_remove(
    GenericMap *map,
    const void     *key,
    void           *removed_key,
    void           *removed_value,
    size            entry_size,
    size            key_offset,
    size            key_size,
    size            value_offset,
    size            value_size,
    size            hash_offset
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

    if (removed_key) {
        memcpy(removed_key, map_key_ptr(map, entry_size, key_offset, idx), key_size);
    } else if (map->key_copy_deinit) {
        map->key_copy_deinit(map_key_ptr(map, entry_size, key_offset, idx));
    }

    if (removed_value) {
        memcpy(removed_value, map_value_ptr(map, entry_size, value_offset, idx), value_size);
    } else if (map->value_copy_deinit) {
        map->value_copy_deinit(map_value_ptr(map, entry_size, value_offset, idx));
    }

    memset(map_entry_ptr(map, entry_size, idx), 0, entry_size);
    map->states[idx] = MAP_SLOT_TOMBSTONE;
    map->length -= 1;
    return true;
}
