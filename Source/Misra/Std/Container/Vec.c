/// file      : std/container/vec.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Generic vector implementation

#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Container/Vec.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Memory.h>
#include <Misra/Sys.h>

// Vec keeps a NUL sentinel byte at `data[length]` so Str (which is a
// Vec(char)) is implicitly C-string-compatible. Allocated capacity is
// always `stored_capacity + 1` items.

static inline size vec_aligned_size(GenericVec *v, size item_size) {
    ValidateVec(v);
    // Stack-init vecs (no allocator) keep element stride at
    // `sizeof(T)`: their backing buffer is a `_Alignas(T) char[]`,
    // so every slot already lies on T's natural boundary.
    if (!v->allocator) {
        return item_size;
    }
    return v->allocator->alignment > 1 ? ALIGN_UP_POW2(item_size, v->allocator->alignment) : item_size;
}

static inline size vec_aligned_offset_at(GenericVec *v, size idx, size item_size) {
    ValidateVec(v);
    return idx * vec_aligned_size(v, item_size);
}

static inline u8 *vec_ptr_at(GenericVec *v, size idx, size item_size) {
    ValidateVec(v);

    return (u8 *)v->data + vec_aligned_offset_at(v, idx, item_size);
}

static inline const u8 *vec_const_ptr_at(const GenericVec *v, size idx, size item_size) {
    ValidateVec(v);

    return (const u8 *)v->data + vec_aligned_offset_at((GenericVec *)v, idx, item_size);
}

void deinit_vec(GenericVec *vec, size item_size) {
    size aligned_size;

    ValidateVec(vec);

    // Stack-init vecs have no allocator and own no heap storage:
    // the InitStack scope macro is the only correct teardown. Calling
    // `VecDeinit` on one would route through `AllocatorFree(NULL, ...)`
    // and crash with a generic "NULL allocator" message; surface the
    // real cause instead.
    if (!vec->allocator) {
        LOG_FATAL("vector not growable, no allocator assigned, probably stack inited");
    }

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

bool reserve_vec(GenericVec *vec, size item_size, size n) {
    size aligned_size;

    ValidateVec(vec);

    aligned_size = vec_aligned_size(vec, item_size);
    if (n > vec->capacity) {
        if (!vec->allocator) {
            LOG_FATAL("vector not growable, no allocator assigned, probably stack inited");
        }
        size old_capacity = (size)vec->capacity;
        u8  *ptr          = (u8 *)AllocatorRealloc(vec->allocator, vec->data, aligned_size * (n + 1));

        if (!ptr) {
            // Not LOG_SYS_ERROR: allocator failures don't flow through the
            // syscall error path; the LOG_ERROR variant is the right report.
            LOG_ERROR("allocator reallocate failed");
            return false;
        }
        vec->data = (char *)ptr;
        MemSet(ptr + old_capacity * aligned_size, 0, aligned_size * (n + 1 - old_capacity));
        vec->capacity = n;
        MAGIC_MARK_DIRTY(vec);
    }

    return true;
}


bool reserve_pow2_vec(GenericVec *vec, size item_size, size n) {
    ValidateVec(vec);

    if (n == 0) {
        return true;
    }
    // Refuse requests above 2^63 -- the doubling loop below would
    // spin forever once n2 == 2^63 (next shift wraps to 0 < n).
    if (n > ((size)1 << 63)) {
        return false;
    }
    size n2 = 1;
    while (n2 < n) {
        n2 <<= 1;
    }

    return reserve_vec(vec, item_size, n2);
}


bool reduce_space_vec(GenericVec *vec, size item_size) {
    size aligned_size;

    ValidateVec(vec);

    // Same rationale as `deinit_vec`: a stack-init vec has no
    // allocator to free into / shrink through.
    if (!vec->allocator) {
        LOG_FATAL("vector not growable, no allocator assigned, probably stack inited");
    }

    aligned_size = vec_aligned_size(vec, item_size);
    if (vec->length == 0) {
        AllocatorFree(vec->allocator, vec->data);
        vec->data     = NULL;
        vec->capacity = 0;
        vec->length   = 0;
        // No MARK_DIRTY: end state (length=0, capacity=0, data=NULL)
        // satisfies every structural check trivially -- the data
        // force-read is gated on `v->data` so a NULL pointer skips it.
        return true;
    } else {
        u8 *ptr = (u8 *)AllocatorRealloc(vec->allocator, vec->data, aligned_size * (vec->length + 1));
        if (!ptr) {
            // Not LOG_SYS_ERROR: allocator failures don't flow through the
            // syscall error path; the LOG_ERROR variant is the right report.
            LOG_ERROR("allocator reallocate failed");
            return false;
        }
        vec->capacity = vec->length;
        vec->data     = (char *)ptr;
        MAGIC_MARK_DIRTY(vec);
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


bool insert_range_into_vec(GenericVec *vec, const u8 *item_data, size item_size, size idx, size count) {
    size aligned_size;
    size inserted_count = 0;

    if (!count) {
        return true;
    }

    ValidateVec(vec);

    if (idx > vec->length) {
        LOG_FATAL("vector index out of bounds, insertion at index greater than length");
    }
    // Overflow check on length + count. A wrapped sum below capacity
    // would skip the reserve and walk past the buffer.
    if (count > (size)-1 - vec->length) {
        LOG_FATAL("vector insert: length + count overflows size");
    }

    aligned_size = vec_aligned_size(vec, item_size);
    if (vec->length + count >= vec->capacity) {
        if (!reserve_pow2_vec(vec, item_size, vec->length + count)) {
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

    MemSet(vec_ptr_at(vec, vec->length, item_size), 0, item_size);
    return true;
}

bool insert_range_fast_into_vec(GenericVec *vec, const u8 *item_data, size item_size, size idx, size count) {
    size aligned_size;
    size inserted_count = 0;
    size displaced      = 0;

    if (!count) {
        return true;
    }

    ValidateVec(vec);

    if (idx > vec->length) {
        LOG_FATAL("vector index out of bounds, insertion at index greater than length");
    }
    // Overflow check on length + count. Same shape as insert_range_into_vec.
    if (count > (size)-1 - vec->length) {
        LOG_FATAL("vector insert (fast): length + count overflows size");
    }

    aligned_size = vec_aligned_size(vec, item_size);
    if (vec->length + count >= vec->capacity) {
        if (!reserve_pow2_vec(vec, item_size, vec->length + count)) {
            return false;
        }
    }

    // Displace at most `length - idx` originals (the actual live tail);
    // a naive `count`-sized move would read past `length` when the new
    // range extends beyond the current end. Park the displaced block at
    // the very end of the new tail so its destination [length+count-d,
    // length+count) never overlaps the new-item slots [idx, idx+count).
    if (idx < vec->length) {
        displaced = vec->length - idx;
        if (displaced > count) {
            displaced = count;
        }
        MemMove(
            vec_ptr_at(vec, vec->length + count - displaced, item_size),
            vec_ptr_at(vec, idx, item_size),
            aligned_size * displaced
        );
    }

    for (size i = 0; i < count; i++) {
        if (vec->copy_init) {
            MemSet(vec_ptr_at(vec, idx + i, item_size), 0, item_size);
            if (!vec->copy_init(vec_ptr_at(vec, idx + i, item_size), item_data + i * item_size, vec->allocator)) {
                for (size s = 0; s < inserted_count; s++) {
                    vec->copy_deinit(vec_ptr_at(vec, idx + s, item_size), vec->allocator);
                }

                if (displaced) {
                    MemMove(
                        vec_ptr_at(vec, idx, item_size),
                        vec_ptr_at(vec, vec->length + count - displaced, item_size),
                        aligned_size * displaced
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

    MemSet(vec_ptr_at(vec, vec->length, item_size), 0, item_size);
    return true;
}


void remove_range_vec(GenericVec *vec, void *removed_data, size item_size, size start, size count) {
    ValidateVec(vec);

    // `start + count` can wrap if both are huge -- a wrapped sum
    // below length would pass the bound check. Catch it first.
    if (count > (size)-1 - start) {
        LOG_FATAL("vector remove range: start + count overflows size");
    }
    if (start + count > vec->length) {
        LOG_FATAL("vector range out of bounds.");
    }

    // Drain payloads first: handing them to the caller buffer xor
    // running copy_deinit. Doing it before the slide keeps the source
    // bytes live for both paths and lets the slide be a single MemMove.
    if (removed_data) {
        MemCopy(removed_data, vec_ptr_at(vec, start, item_size), count * vec_aligned_size(vec, item_size));
    } else {
        if (vec->copy_deinit) {
            u8 *vec_data = vec_ptr_at(vec, start, item_size);
            for (size s = 0; s < count; s++) {
                vec->copy_deinit(vec_data, vec->allocator);
                vec_data += vec_aligned_size(vec, item_size);
            }
        } else {
            MemSet(vec_ptr_at(vec, start, item_size), 0, count * vec_aligned_size(vec, item_size));
        }
    }

    // Compact the tail leftward over the drained window. MemMove (not
    // MemCopy) because the source and destination ranges overlap when
    // start + count < length.
    MemMove(
        vec_ptr_at(vec, start, item_size),
        vec_ptr_at(vec, start + count, item_size),
        (vec->length - start - count) * vec_aligned_size(vec, item_size)
    );
    MemSet(vec_ptr_at(vec, (vec->length - count), item_size), 0, count * vec_aligned_size(vec, item_size));

    vec->length -= count;

    MemSet(vec_ptr_at(vec, vec->length, item_size), 0, item_size);
}


void fast_remove_range_vec(GenericVec *vec, void *removed_data, size item_size, size start, size count) {
    ValidateVec(vec);

    // `start + count` can wrap if both are huge -- a wrapped sum
    // below length would pass the bound check. Catch it first.
    if (count > (size)-1 - start) {
        LOG_FATAL("vector fast remove range: start + count overflows size");
    }
    if (start + count > vec->length) {
        LOG_FATAL("vector range out of bounds.");
    }

    if (removed_data) {
        MemCopy(removed_data, vec_ptr_at(vec, start, item_size), count * vec_aligned_size(vec, item_size));
    } else {
        if (vec->copy_deinit) {
            u8 *vec_data = vec_ptr_at(vec, start, item_size);
            for (size s = 0; s < count; s++) {
                vec->copy_deinit(vec_data, vec->allocator);
                vec_data += vec_aligned_size(vec, item_size);
            }
        } else {
            MemSet(vec_ptr_at(vec, start, item_size), 0, count * vec_aligned_size(vec, item_size));
        }
    }

    // Swap-from-end remove: fill the just-vacated gap with the last
    // `count` elements. When the gap touches the tail there are fewer
    // than `count` survivors past it, so cap the move to whatever exists.
    size available_elements = vec->length - (start + count);
    size elements_to_move   = count;

    if (elements_to_move > available_elements) {
        elements_to_move = available_elements;
    }

    if (elements_to_move > 0) {
        MemMove(
            vec_ptr_at(vec, start, item_size),
            vec_ptr_at(vec, vec->length - elements_to_move, item_size),
            elements_to_move * vec_aligned_size(vec, item_size)
        );
    }

    MemSet(vec_ptr_at(vec, vec->length - count, item_size), 0, count * vec_aligned_size(vec, item_size));

    vec->length -= count;

    // Keep the post-length sentinel slot zeroed so VecBegin-returned arrays
    // are safe to treat as NUL-terminated where the element type allows.
    MemSet(vec_ptr_at(vec, vec->length, item_size), 0, item_size);
}


void vec_sort(GenericVec *vec, size item_size, GenericCompare comp) {
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

    u8 *a, *b, tmp;
    a = vec_ptr_at(vec, idx1, item_size);
    b = vec_ptr_at(vec, idx2, item_size);
    // Swap the user bytes only; alignment padding is never read.
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

// Structural body for Vec. Inspects length<=capacity, the allocator
// vtable (when present), and force-reads the first byte of `data` so
// a freed/garbage pointer faults at the validate site rather than
// downstream. Memoized via MAGIC_VALIDATED_BIT; capacity / data /
// allocator changes (reserve_vec, reduce_space_vec) flip the bit.
static void validate_vec_structural(const GenericVec *v) {
    if (v->length > v->capacity) {
        LOG_FATAL("Invalid vec object.");
    }
    if (v->allocator &&
        (!v->allocator->allocate || !v->allocator->resize || !v->allocator->remap || !v->allocator->deallocate)) {
        LOG_FATAL("Invalid vec allocator.");
    }
    if (v->data) {
        (void)(*(char *)(void *)(v->data));
    }
}

void validate_vec(const GenericVec *v) {
    if (!v) {
        LOG_FATAL("NULL vec object pointer.");
    }
    if (!MAGIC_MATCHES(v->__magic, VEC_MAGIC)) {
        LOG_FATAL("Invalid vec object. Either uninitialized or corrupted!");
    }
    if (!(v->__magic & MAGIC_VALIDATED_BIT)) {
        return;
    }
    validate_vec_structural(v);
    ((GenericVec *)(void *)v)->__magic &= ~MAGIC_VALIDATED_BIT;
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
