/// file      : std/vec.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Generic vector implementation

// ct
#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Container/Vec.h>
#include <Misra/Std/Log.h>
#include <Misra/Sys.h>

// libc
#include <stdlib.h>

// NOTE: Because Str derives of Vec, the vector implementation is designed to always have actual capacity
// one more than length and set the space just after length to 0 (memset to 0)
// actual capacity may differ from stored capacity value

static inline size vec_aligned_size(GenericVec *v, size item_size) {
    ValidateVec(v);

    if (!v->alignment) {
        LOG_FATAL("Invalid alignment. Did you initialize before use? Aborting...");
    }

    return v->alignment > 1 ? ALIGN_UP_POW2(item_size, v->alignment) : item_size;
}

static inline size vec_aligned_offset_at(GenericVec *v, size idx, size item_size) {
    ValidateVec(v);

    return idx * vec_aligned_size(v, item_size);
}

static inline char *vec_ptr_at(GenericVec *v, size idx, size item_size) {
    ValidateVec(v);

    return v->data + vec_aligned_offset_at(v, idx, item_size);
}

void deinit_vec(GenericVec *vec, size item_size) {
    ValidateVec(vec);

    if (vec->data) {
        if (vec->copy_deinit) {
            for (size i = 0; i < vec->length; i++) {
                vec->copy_deinit(vec_ptr_at(vec, i, item_size));
            }
        } else {
            memset(vec->data, 0, vec_aligned_size(vec, item_size) * vec->capacity);
        }

        free(vec->data);
    }

    vec->data   = NULL;
    vec->length = vec->capacity = 0;
}


void clear_vec(GenericVec *vec, size item_size) {
    ValidateVec(vec);

    if (vec->data) {
        if (vec->copy_deinit) {
            for (size i = 0; i < vec->length; i++) {
                // don't ever check the return value of deinit function
                // it's not guaranteed to be a function that actually returns something!
                // typecasting is not safe in C
                vec->copy_deinit(vec_ptr_at(vec, i, item_size));
            }
        } else {
            memset(vec->data, 0, vec_aligned_size(vec, item_size) * vec->capacity);
        }
    }

    vec->length = 0;
}

// Reserve new space if n > capacity
void reserve_vec(GenericVec *vec, size item_size, size n) {
    ValidateVec(vec);

    if (n > vec->capacity) {
        // make sure actual capacity is always at-least one greater than given capacity
        // this way, actual capacity is always at least one greater than length of vector (as required for strings)
        char *ptr = realloc(vec->data, (n + 1) * vec_aligned_size(vec, item_size));
        if (!ptr) {
            LOG_SYS_FATAL("realloc() failed");
        }
        // it's mandatory to set the pointer here, because next call to any vec_ will do a validation check
        // this could've resulted in a heap-use-after-free bug, which was caught with help of ValidateVec
        vec->data = ptr;
        memset(
            ptr + vec_aligned_offset_at(vec, vec->capacity, item_size),
            0,
            vec_aligned_size(vec, item_size) * (n + 1 - vec->capacity)
        );
        vec->capacity = n;
    }
}


void reserve_pow2_vec(GenericVec *vec, size item_size, size n) {
    ValidateVec(vec);

    size n2 = 1;
    if (n == 0) {
        return;
    }

    while (n2 < n) {
        n2 <<= 1;
    }

    reserve_vec(vec, item_size, n2);
}


void reduce_space_vec(GenericVec *vec, size item_size) {
    ValidateVec(vec);

    if (vec->length == 0) {
        free(vec->data);
        vec->data     = NULL;
        vec->capacity = 0;
        vec->length   = 0;
        return;
    } else {
        char *ptr;
        // again make sure that actual capacity is at least one greater than length of vector (required for strings)
        ptr = realloc(vec->data, (vec->length + 1) * vec_aligned_size(vec, item_size));
        if (!ptr) {
            LOG_SYS_FATAL("realloc() failed");
        }
        vec->capacity = vec->length;
        vec->data     = ptr;
    }
}


void insert_range_into_vec(GenericVec *vec, char *item_data, size item_size, size idx, size count) {
    if (!count) {
        return;
    }

    ValidateVec(vec);

    if (idx > vec->length) {
        LOG_FATAL("vector index out of bounds, insertion at index greater than length");
    }

    if (vec->length + count >= vec->capacity) {
        reserve_pow2_vec(vec, item_size, vec->capacity + count);
    }

    if (idx < vec->length) {
        memmove(
            vec_ptr_at(vec, idx + count, item_size),
            vec_ptr_at(vec, idx, item_size),
            (vec->length - idx) * vec_aligned_size(vec, item_size)
        );
    }

    for (size i = 0; i < count; i++) {
        if (vec->copy_init) {
            memset(vec_ptr_at(vec, idx + i, item_size), 0, item_size);
            vec->copy_init(vec_ptr_at(vec, idx + i, item_size), item_data + i * item_size);
        } else {
            memcpy(vec_ptr_at(vec, idx + i, item_size), item_data + i * item_size, item_size);
        }
    }

    vec->length += count;

    // make sure space just after vector length is memeset to 0
    memset(vec_ptr_at(vec, vec->length, item_size), 0, item_size);
}

void insert_range_fast_into_vec(GenericVec *vec, char *item_data, size item_size, size idx, size count) {
    if (!count) {
        return;
    }

    ValidateVec(vec);

    if (idx > vec->length) {
        LOG_FATAL("vector index out of bounds, insertion at index greater than length");
    }

    if (vec->length + count >= vec->capacity) {
        reserve_pow2_vec(vec, item_size, count);
    }

    if (idx < vec->length) {
        // move item at index to last and insert the new item directly at index
        memmove(
            vec_ptr_at(vec, vec->length, item_size),
            vec_ptr_at(vec, idx, item_size),
            vec_aligned_size(vec, item_size) * count
        );
    }

    for (size i = 0; i < count; i++) {
        if (vec->copy_init) {
            memset(vec_ptr_at(vec, idx + i, item_size), 0, item_size);
            vec->copy_init(vec_ptr_at(vec, idx + i, item_size), item_data + i * item_size);
        } else {
            memcpy(vec_ptr_at(vec, idx + i, item_size), item_data + i * item_size, item_size);
        }
    }

    vec->length += count;

    // make sure space just after vector length is memeset to 0
    memset(vec_ptr_at(vec, vec->length, item_size), 0, item_size);
}


void remove_range_vec(GenericVec *vec, void *removed_data, size item_size, size start, size count) {
    ValidateVec(vec);

    if (start + count > vec->length) {
        LOG_FATAL("vector range out of bounds.");
    }

    if (removed_data) {
        // make copy of data if user want's a copy
        memcpy(removed_data, vec_ptr_at(vec, start, item_size), count * vec_aligned_size(vec, item_size));
    } else {
        // if no space provided to copy data over to, just destroy or memset it
        if (vec->copy_deinit) {
            char *vec_data = vec_ptr_at(vec, start, item_size);
            for (size s = 0; s < count; s++) {
                vec->copy_deinit(vec_data);
                vec_data += vec_aligned_size(vec, item_size);
            }
        } else {
            memset(vec_ptr_at(vec, start, item_size), 0, count * vec_aligned_size(vec, item_size));
        }
    }

    // all elements to new created space
    memmove(
        // move to freed up space
        vec_ptr_at(vec, start, item_size),
        // start moving all elements just after the freed up space
        vec_ptr_at(vec, start + count, item_size),
        // these elements appear after "start + count" index
        (vec->length - start - count) * vec_aligned_size(vec, item_size)
    );
    memset(vec_ptr_at(vec, (vec->length - count), item_size), 0, count * vec_aligned_size(vec, item_size));

    vec->length -= count;

    // make sure space just after vector length is memeset to 0
    memset(vec_ptr_at(vec, vec->length, item_size), 0, item_size);
}


void fast_remove_range_vec(GenericVec *vec, void *removed_data, size item_size, size start, size count) {
    ValidateVec(vec);

    if (start + count > vec->length) {
        LOG_FATAL("vector range out of bounds.");
    }

    // Save the data to be removed if requested
    if (removed_data) {
        memcpy(removed_data, vec_ptr_at(vec, start, item_size), count * vec_aligned_size(vec, item_size));
    } else {
        // Otherwise, properly clean up the memory
        if (vec->copy_deinit) {
            char *vec_data = vec_ptr_at(vec, start, item_size);
            for (size s = 0; s < count; s++) {
                vec->copy_deinit(vec_data);
                vec_data += vec_aligned_size(vec, item_size);
            }
        } else {
            memset(vec_ptr_at(vec, start, item_size), 0, count * vec_aligned_size(vec, item_size));
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
        memmove(
            // Move to freed up space
            vec_ptr_at(vec, start, item_size),
            // Start from the position that leaves exactly 'elements_to_move' elements
            vec_ptr_at(vec, vec->length - elements_to_move, item_size),
            // Move 'elements_to_move' elements
            elements_to_move * vec_aligned_size(vec, item_size)
        );
    }

    // Clear the remaining elements at the end
    memset(vec_ptr_at(vec, vec->length - count, item_size), 0, count * vec_aligned_size(vec, item_size));

    vec->length -= count;

    // Make sure space just after vector length is memset to 0
    memset(vec_ptr_at(vec, vec->length, item_size), 0, item_size);
}


void qsort_vec(GenericVec *vec, size item_size, GenericCompare comp) {
    ValidateVec(vec);

    if (vec_aligned_size(vec, item_size) != item_size) {
        LOG_FATAL(
            "QSort not implemented for vectors wherein the size of items don't "
            "match their aligned size."
        );
    }

    qsort(vec->data, vec->length, item_size, comp);
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

void resize_vec(GenericVec *vec, size item_size, size new_size) {
    ValidateVec(vec);

    if (new_size <= vec->capacity) {
        // if we're shrinking then we need to remove some part of the data
        if (new_size < vec->length) {
            remove_range_vec(vec, NULL, item_size, new_size, vec->length - new_size);
        }
        vec->length = new_size;
    } else {
        reserve_pow2_vec(vec, item_size, new_size);
        vec->length = new_size;
    }
}

void validate_vec(const GenericVec *v) {
    if (!(v)) {
        LOG_FATAL("NULL vec object pointer.");
    }
    if ((v)->__magic != MISRA_VEC_MAGIC) {
        LOG_FATAL("Invalid vec object. Either uninitialized or corrupted!");
    }
    if (!(v)->alignment || (v)->length > (v)->capacity) {
        LOG_FATAL("Invalid vec object.");
    }
    // if memory is invalid, system will segfault here
    if ((v)->data) {
        (void)(*(char *)(void *)((v)->data));
    }
}
