/// file      : std/vec.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Generic vector implementation

// ct
#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Container/Vec.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Memory.h>
#include <Misra/Sys.h>

// libc

// NOTE: Because Str derives of Vec, the vector implementation is designed to always have actual capacity
// one more than length and set the space just after length to 0 (memset to 0)
// actual capacity may differ from stored capacity value

static inline size vec_aligned_size(GenericVec *v, size item_size) {
    ValidateVec(v);
    // Default allocator alignment of 1 leaves element stride at sizeof(T).
    // Stronger allocator alignment requests pad each element so that every
    // slot lies on the requested boundary.
    return v->allocator->alignment > 1 ? ALIGN_UP_POW2(item_size, v->allocator->alignment) : item_size;
}

static inline size vec_aligned_offset_at(GenericVec *v, size idx, size item_size) {
    ValidateVec(v);
    return idx * vec_aligned_size(v, item_size);
}

static inline char *vec_ptr_at(GenericVec *v, size idx, size item_size) {
    ValidateVec(v);

    return v->data + vec_aligned_offset_at(v, idx, item_size);
}

static inline const char *vec_const_ptr_at(const GenericVec *v, size idx, size item_size) {
    ValidateVec(v);

    return v->data + vec_aligned_offset_at((GenericVec *)v, idx, item_size);
}

void deinit_vec(GenericVec *vec, size item_size) {
    size aligned_size;

    ValidateVec(vec);

    aligned_size = vec_aligned_size(vec, item_size);
    if (vec->data) {
        if (vec->copy_deinit) {
            for (size i = 0; i < vec->length; i++) {
                vec->copy_deinit(vec_ptr_at(vec, i, item_size), vec->allocator);
            }
        } else {
            MemSet(vec->data, 0, aligned_size * (vec->capacity + 1));
        }

        AllocatorFree(vec->allocator, vec->data);
    }

    // Zero the whole header so any use-after-deinit hits a zeroed
    // __magic at the next validate call instead of silently dispatching
    // into freed pointers.
    MemSet(vec, 0, sizeof(*vec));
}


void clear_vec(GenericVec *vec, size item_size) {
    size aligned_size;

    ValidateVec(vec);

    aligned_size = vec_aligned_size(vec, item_size);
    if (vec->data) {
        if (vec->copy_deinit) {
            for (size i = 0; i < vec->length; i++) {
                vec->copy_deinit(vec_ptr_at(vec, i, item_size), vec->allocator);
            }
        } else {
            MemSet(vec->data, 0, aligned_size * (vec->capacity + 1));
        }
    }

    vec->length = 0;
}

// Reserve new space if n > capacity
bool reserve_vec(GenericVec *vec, size item_size, size n) {
    size aligned_size;

    ValidateVec(vec);

    aligned_size = vec_aligned_size(vec, item_size);
    if (n > vec->capacity) {
        size  old_capacity = (size)vec->capacity;
        char *ptr          = (char *)
            AllocatorRealloc(vec->allocator, vec->data, aligned_size * (old_capacity + 1), aligned_size * (n + 1));

        if (!ptr) {
            // Not LOG_SYS_ERROR: allocator failures don't set errno.
            LOG_ERROR("allocator reallocate failed");
            return false;
        }
        vec->data = ptr;
        MemSet(ptr + old_capacity * aligned_size, 0, aligned_size * (n + 1 - old_capacity));
        vec->capacity = n;
    }

    return true;
}


bool reserve_pow2_vec(GenericVec *vec, size item_size, size n) {
    ValidateVec(vec);

    size n2 = 1;
    if (n == 0) {
        return true;
    }

    while (n2 < n) {
        n2 <<= 1;
    }

    return reserve_vec(vec, item_size, n2);
}


bool reduce_space_vec(GenericVec *vec, size item_size) {
    size aligned_size;

    ValidateVec(vec);

    aligned_size = vec_aligned_size(vec, item_size);
    if (vec->length == 0) {
        AllocatorFree(vec->allocator, vec->data);
        vec->data     = NULL;
        vec->capacity = 0;
        vec->length   = 0;
        return true;
    } else {
        char *ptr = (char *)AllocatorRealloc(
            vec->allocator,
            vec->data,
            aligned_size * (vec->capacity + 1),
            aligned_size * (vec->length + 1)
        );
        if (!ptr) {
            // Not LOG_SYS_ERROR: allocator failures don't set errno.
            LOG_ERROR("allocator reallocate failed");
            return false;
        }
        vec->capacity = vec->length;
        vec->data     = ptr;
    }

    return true;
}


bool clone_vec(GenericVec *dst, const GenericVec *src, size item_size) {
    ValidateVec(dst);
    ValidateVec(src);

    if (src->length == 0) {
        return true;
    }

    if (!reserve_pow2_vec(dst, item_size, src->length)) {
        return false;
    }

    for (size i = 0; i < src->length; i++) {
        if (!insert_range_into_vec(dst, vec_const_ptr_at(src, i, item_size), item_size, dst->length, 1)) {
            return false;
        }
    }

    return true;
}


bool insert_range_into_vec(GenericVec *vec, const char *item_data, size item_size, size idx, size count) {
    size aligned_size;
    size inserted_count = 0;

    if (!count) {
        return true;
    }

    ValidateVec(vec);

    if (idx > vec->length) {
        LOG_FATAL("vector index out of bounds, insertion at index greater than length");
    }

    aligned_size = vec_aligned_size(vec, item_size);
    if (vec->length + count >= vec->capacity) {
        if (!reserve_pow2_vec(vec, item_size, vec->capacity + count)) {
            return false;
        }
    }

    if (idx < vec->length) {
        MemMove(
            vec_ptr_at(vec, idx + count, item_size),
            vec_ptr_at(vec, idx, item_size),
            (vec->length - idx) * aligned_size
        );
    }

    for (size i = 0; i < count; i++) {
        if (vec->copy_init) {
            MemSet(vec_ptr_at(vec, idx + i, item_size), 0, item_size);
            if (!vec->copy_init(vec_ptr_at(vec, idx + i, item_size), item_data + i * item_size, vec->allocator)) {
                for (size s = 0; s < inserted_count; s++) {
                    vec->copy_deinit(vec_ptr_at(vec, idx + s, item_size), vec->allocator);
                }

                MemSet(vec_ptr_at(vec, idx, item_size), 0, count * aligned_size);
                if (idx < vec->length) {
                    MemMove(
                        vec_ptr_at(vec, idx, item_size),
                        vec_ptr_at(vec, idx + count, item_size),
                        (vec->length - idx) * aligned_size
                    );
                    MemSet(vec_ptr_at(vec, vec->length, item_size), 0, count * aligned_size);
                }

                return false;
            }
            inserted_count++;
        } else {
            MemCopy(vec_ptr_at(vec, idx + i, item_size), item_data + i * item_size, item_size);
        }
    }

    vec->length += count;

    // make sure space just after vector length is memeset to 0
    MemSet(vec_ptr_at(vec, vec->length, item_size), 0, item_size);
    return true;
}

bool insert_range_fast_into_vec(GenericVec *vec, const char *item_data, size item_size, size idx, size count) {
    size aligned_size;
    size inserted_count = 0;

    if (!count) {
        return true;
    }

    ValidateVec(vec);

    if (idx > vec->length) {
        LOG_FATAL("vector index out of bounds, insertion at index greater than length");
    }

    aligned_size = vec_aligned_size(vec, item_size);
    if (vec->length + count >= vec->capacity) {
        if (!reserve_pow2_vec(vec, item_size, vec->length + count)) {
            return false;
        }
    }

    if (idx < vec->length) {
        MemMove(vec_ptr_at(vec, vec->length, item_size), vec_ptr_at(vec, idx, item_size), aligned_size * count);
    }

    for (size i = 0; i < count; i++) {
        if (vec->copy_init) {
            MemSet(vec_ptr_at(vec, idx + i, item_size), 0, item_size);
            if (!vec->copy_init(vec_ptr_at(vec, idx + i, item_size), item_data + i * item_size, vec->allocator)) {
                for (size s = 0; s < inserted_count; s++) {
                    vec->copy_deinit(vec_ptr_at(vec, idx + s, item_size), vec->allocator);
                }

                if (idx < vec->length) {
                    MemMove(
                        vec_ptr_at(vec, idx, item_size),
                        vec_ptr_at(vec, vec->length, item_size),
                        aligned_size * count
                    );
                }

                MemSet(vec_ptr_at(vec, vec->length, item_size), 0, aligned_size * count);
                return false;
            }
            inserted_count++;
        } else {
            MemCopy(vec_ptr_at(vec, idx + i, item_size), item_data + i * item_size, item_size);
        }
    }

    vec->length += count;

    // make sure space just after vector length is memeset to 0
    MemSet(vec_ptr_at(vec, vec->length, item_size), 0, item_size);
    return true;
}


void remove_range_vec(GenericVec *vec, void *removed_data, size item_size, size start, size count) {
    ValidateVec(vec);

    if (start + count > vec->length) {
        LOG_FATAL("vector range out of bounds.");
    }

    if (removed_data) {
        // make copy of data if user want's a copy
        MemCopy(removed_data, vec_ptr_at(vec, start, item_size), count * vec_aligned_size(vec, item_size));
    } else {
        // if no space provided to copy data over to, just destroy or memset it
        if (vec->copy_deinit) {
            char *vec_data = vec_ptr_at(vec, start, item_size);
            for (size s = 0; s < count; s++) {
                vec->copy_deinit(vec_data, vec->allocator);
                vec_data += vec_aligned_size(vec, item_size);
            }
        } else {
            MemSet(vec_ptr_at(vec, start, item_size), 0, count * vec_aligned_size(vec, item_size));
        }
    }

    // all elements to new created space
    MemMove(
        // move to freed up space
        vec_ptr_at(vec, start, item_size),
        // start moving all elements just after the freed up space
        vec_ptr_at(vec, start + count, item_size),
        // these elements appear after "start + count" index
        (vec->length - start - count) * vec_aligned_size(vec, item_size)
    );
    MemSet(vec_ptr_at(vec, (vec->length - count), item_size), 0, count * vec_aligned_size(vec, item_size));

    vec->length -= count;

    // make sure space just after vector length is memeset to 0
    MemSet(vec_ptr_at(vec, vec->length, item_size), 0, item_size);
}


void fast_remove_range_vec(GenericVec *vec, void *removed_data, size item_size, size start, size count) {
    ValidateVec(vec);

    if (start + count > vec->length) {
        LOG_FATAL("vector range out of bounds.");
    }

    // Save the data to be removed if requested
    if (removed_data) {
        MemCopy(removed_data, vec_ptr_at(vec, start, item_size), count * vec_aligned_size(vec, item_size));
    } else {
        // Otherwise, properly clean up the memory
        if (vec->copy_deinit) {
            char *vec_data = vec_ptr_at(vec, start, item_size);
            for (size s = 0; s < count; s++) {
                vec->copy_deinit(vec_data, vec->allocator);
                vec_data += vec_aligned_size(vec, item_size);
            }
        } else {
            MemSet(vec_ptr_at(vec, start, item_size), 0, count * vec_aligned_size(vec, item_size));
        }
    }

    // Calculate how many elements we can move from the end
    size available_elements = vec->length - (start + count);
    size elements_to_move   = count;

    // If we don't have enough elements at the end, adjust the count
    if (elements_to_move > available_elements) {
        elements_to_move = available_elements;
    }

    if (elements_to_move > 0) {
        // Move the last 'elements_to_move' elements to the gap
        MemMove(
            // Move to freed up space
            vec_ptr_at(vec, start, item_size),
            // Start from the position that leaves exactly 'elements_to_move' elements
            vec_ptr_at(vec, vec->length - elements_to_move, item_size),
            // Move 'elements_to_move' elements
            elements_to_move * vec_aligned_size(vec, item_size)
        );
    }

    // Clear the remaining elements at the end
    MemSet(vec_ptr_at(vec, vec->length - count, item_size), 0, count * vec_aligned_size(vec, item_size));

    vec->length -= count;

    // Make sure space just after vector length is memset to 0
    MemSet(vec_ptr_at(vec, vec->length, item_size), 0, item_size);
}


void qsort_vec(GenericVec *vec, size item_size, GenericCompare comp) {
    ValidateVec(vec);

    if (vec_aligned_size(vec, item_size) != item_size) {
        LOG_FATAL(
            "QSort not implemented for vectors wherein the size of items don't "
            "match their aligned size."
        );
    }

    MemSort(vec->data, vec->length, item_size, comp);
}


void swap_vec(GenericVec *vec, size item_size, size idx1, size idx2) {
    ValidateVec(vec);

    if (idx1 >= vec->length || idx2 >= vec->length) {
        LOG_FATAL("vector index out of bounds.");
    }

    if (idx1 == idx2) {
        return;
    }

    char *a, *b, tmp;
    a = vec_ptr_at(vec, idx1, item_size);
    b = vec_ptr_at(vec, idx2, item_size);
    // here it's ok to use item_size directly ig, because data after that is always untouched
    // never read, and never written to
    while (item_size--) {
        tmp = *a;
        *a  = *b;
        *b  = tmp;
        a++, b++;
    }
}


void reverse_vec(GenericVec *vec, size item_size) {
    ValidateVec(vec);

    size i = vec->length / 2;
    while (i--) {
        swap_vec(vec, item_size, i, vec->length - (i + 1));
    }
}

size find_idx_vec(GenericVec *vec, const void *item_data, size item_size, GenericCompare comp) {
    ValidateVec(vec);

    if (!item_data || !comp) {
        LOG_FATAL("Invalid arguments.");
    }

    for (size i = 0; i < vec->length; i++) {
        if (comp(vec_ptr_at(vec, i, item_size), item_data) == 0) {
            return i;
        }
    }

    return SIZE_MAX;
}

bool resize_vec(GenericVec *vec, size item_size, size new_size) {
    ValidateVec(vec);

    if (new_size <= vec->capacity) {
        if (new_size < vec->length) {
            remove_range_vec(vec, NULL, item_size, new_size, vec->length - new_size);
        }
        vec->length = new_size;
    } else {
        if (!reserve_pow2_vec(vec, item_size, new_size)) {
            return false;
        }
        vec->length = new_size;
    }

    if (vec->data) {
        MemSet(vec_ptr_at(vec, vec->length, item_size), 0, item_size);
    }

    return true;
}

void validate_vec(const GenericVec *v) {
    if (!(v)) {
        LOG_FATAL("NULL vec object pointer.");
    }
    if ((v)->__magic != VEC_MAGIC) {
        LOG_FATAL("Invalid vec object. Either uninitialized or corrupted!");
    }
    if ((v)->length > (v)->capacity) {
        LOG_FATAL("Invalid vec object.");
    }
    if (!(v)->allocator->allocate || !(v)->allocator->resize || !(v)->allocator->remap || !(v)->allocator->deallocate) {
        LOG_FATAL("Invalid vec allocator.");
    }
    // if memory is invalid, system will segfault here
    if ((v)->data) {
        (void)(*(char *)(void *)((v)->data));
    }
}

bool vec_insert_one_l(
    GenericVec *vec,
    const void *item_copy,
    void       *source,
    size        item_size,
    size        idx,
    bool        preserve_order
) {
    bool success = preserve_order ? insert_range_into_vec(vec, item_copy, item_size, idx, 1) :
                                    insert_range_fast_into_vec(vec, item_copy, item_size, idx, 1);

    return vec_zero_source_on_success(vec, source, item_size, success);
}

bool vec_insert_one_r(GenericVec *vec, const void *item_copy, size item_size, size idx, bool preserve_order) {
    return preserve_order ? insert_range_into_vec(vec, item_copy, item_size, idx, 1) :
                            insert_range_fast_into_vec(vec, item_copy, item_size, idx, 1);
}

bool vec_insert_range_l(GenericVec *vec, void *items, size item_size, size idx, size count, bool preserve_order) {
    bool success;

    if (!items) {
        LOG_FATAL("Expected a valid pointer");
    }

    success = preserve_order ? insert_range_into_vec(vec, items, item_size, idx, count) :
                               insert_range_fast_into_vec(vec, items, item_size, idx, count);

    return vec_zero_source_on_success(vec, items, count * item_size, success);
}

bool vec_insert_range_r(GenericVec *vec, const void *items, size item_size, size idx, size count, bool preserve_order) {
    if (!items) {
        LOG_FATAL("Expected a valid pointer");
    }

    return preserve_order ? insert_range_into_vec(vec, items, item_size, idx, count) :
                            insert_range_fast_into_vec(vec, items, item_size, idx, count);
}

bool vec_merge_l(GenericVec *dst, GenericVec *src, size item_size) {
    if (!src->data || !src->length) {
        return true;
    }

    return vec_release_merged_source_on_success(
        dst,
        src,
        item_size,
        vec_insert_range_l(dst, src->data, item_size, dst->length, src->length, true)
    );
}

bool vec_merge_r(GenericVec *dst, const GenericVec *src, size item_size) {
    if (!src->data || !src->length) {
        return true;
    }

    return vec_insert_range_r(dst, src->data, item_size, dst->length, src->length, true);
}
