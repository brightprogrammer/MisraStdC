/// file      : std/vec.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// copyright : Copyright (c) 2025, Siddharth Mishra, All rights reserved.
///
/// Generic vector implementation

// ct
#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Container/Vec.h>
#include <Misra/Std/Log.h>
#include <Misra/Sys.h>

static inline size_t vec_aligned_size(GenericVec *v, size_t item_size) {
    if (!v || !item_size) {
        LOG_FATAL("Invalid arguments. Aborting...");
    }

    if (!v->alignment) {
        LOG_FATAL("Invalid alignment. Did you initialize before use? Aborting...");
    }

    return v->alignment > 1 ? ALIGN_UP_POW2(item_size, v->alignment) : item_size;
}

static inline size_t vec_aligned_offset_at(GenericVec *v, size_t idx, size_t item_size) {
    if (!v || !item_size) {
        LOG_FATAL("Invalid arguments. Aborting...");
    }

    return idx * vec_aligned_size(v, item_size);
}

static inline char *vec_ptr_at(GenericVec *v, size_t idx, size_t item_size) {
    if (!v || !item_size) {
        LOG_FATAL("Invalid arguments");
    }

    return v->data + vec_aligned_offset_at(v, idx, item_size);
}

void init_vec(
    GenericVec       *vec,
    size_t            item_size,
    GenericCopyInit   copy_init,
    GenericCopyDeinit copy_deinit,
    size_t            alignment
) {
    if (!vec || !item_size || !alignment) {
        LOG_FATAL("invalid arguments.");
    }

    memset(vec, 0, sizeof(GenericVec));
    vec->alignment   = alignment;
    vec->copy_init   = copy_init;
    vec->copy_deinit = copy_deinit;
}

void init_vec_on_stack(
    GenericVec       *vec,
    char             *stack_mem,
    size_t            capacity,
    size_t            item_size,
    GenericCopyInit   copy_init,
    GenericCopyDeinit copy_deinit,
    size_t            alignment
) {
    if (!vec || !item_size || !alignment) {
        LOG_FATAL("invalid arguments.");
    }

    init_vec(vec, item_size, copy_init, copy_deinit, alignment);
    vec->data     = stack_mem;
    vec->capacity = capacity;
}


void deinit_vec(GenericVec *vec, size_t item_size) {
    if (!vec || !item_size) {
        LOG_FATAL("invalid arguments");
    }

    if (vec->data) {
        if (vec->copy_deinit) {
            for (size_t i = 0; i < vec->length; i++) {
                vec->copy_deinit(vec_ptr_at(vec, i, item_size));
            }
        } else {
            memset(vec->data, 0, vec_aligned_size(vec, item_size) * vec->capacity);
        }

        free(vec->data);
    }

    memset(vec, 0, sizeof(GenericVec));
}


void clear_vec(GenericVec *vec, size_t item_size) {
    if (!vec || !item_size) {
        LOG_FATAL("invalid arguments.");
    }

    if (vec->data) {
        if (vec->copy_deinit) {
            for (size_t i = 0; i < vec->length; i++) {
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


// Increase size for one more item to be stored.
void expand_vec(GenericVec *vec, size_t item_size) {
    if (!vec || !item_size) {
        LOG_FATAL("invalid arguments.");
    }

    if (vec->length + 1 > vec->capacity) {
        char *ptr;
        int   n = (vec->capacity == 0) ? 1 : vec->capacity << 1;
        ptr     = realloc(vec->data, n * vec_aligned_size(vec, item_size));
        if (!ptr) {
            Str syserr;
            StrStackInit(&syserr, SYS_ERROR_STR_MAX_LENGTH, {
                LOG_FATAL("realloc() failed : %s.", SysStrError(errno, &syserr)->data);
            });
        }
        memset(
            ptr + vec_aligned_offset_at(vec, vec->capacity, item_size),
            0,
            vec_aligned_size(vec, item_size) * (n - vec->capacity)
        );
        vec->data     = ptr;
        vec->capacity = n;
    }
}


// Reserve new space if n > capacity
void reserve_vec(GenericVec *vec, size_t item_size, size_t n) {
    if (!vec || !item_size) {
        LOG_FATAL("invalid arguments.");
    }

    if (n > vec->capacity) {
        char *ptr = realloc(vec->data, n * vec_aligned_size(vec, item_size));
        if (!ptr) {
            Str syserr;
            StrStackInit(&syserr, SYS_ERROR_STR_MAX_LENGTH, {
                LOG_FATAL("realloc() failed : %s.", SysStrError(errno, &syserr)->data);
            });
        }
        memset(
            ptr + vec_aligned_offset_at(vec, vec->capacity, item_size),
            0,
            vec_aligned_size(vec, item_size) * (n - vec->capacity)
        );
        vec->data     = ptr;
        vec->capacity = n;
    }
}


void reserve_pow2_vec(GenericVec *vec, size_t item_size, size_t n) {
    if (!vec || !item_size) {
        LOG_FATAL("invalid arguments.");
    }

    size_t n2 = 1;
    if (n == 0) {
        return;
    }

    while (n2 < n) {
        n2 <<= 1;
    }

    reserve_vec(vec, item_size, n2);
}


void reduce_space_vec(GenericVec *vec, size_t item_size) {
    if (!vec || !item_size) {
        LOG_FATAL("invalid arguments.");
    }

    if (vec->length == 0) {
        free(vec->data);
        vec->data     = NULL;
        vec->capacity = 0;
        vec->length   = 0;
        return;
    } else {
        char *ptr;
        ptr = realloc(vec->data, vec->length * vec_aligned_size(vec, item_size));
        if (!ptr) {
            Str syserr;
            StrStackInit(&syserr, SYS_ERROR_STR_MAX_LENGTH, {
                LOG_FATAL("realloc() failed : %s.", SysStrError(errno, &syserr)->data);
            });
        }
        vec->capacity = vec->length;
        vec->data     = ptr;
    }
}


void insert_range_into_vec(
    GenericVec *vec,
    char       *item_data,
    size_t      item_size,
    size_t      idx,
    size_t      count
) {
    if (!vec || !item_size || !item_data) {
        LOG_FATAL("invalid arguments.");
    }

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

    for (i64 i = 0; i < count; i++) {
        if (vec->copy_init) {
            memset(vec_ptr_at(vec, idx + i, item_size), 0, item_size);
            vec->copy_init(vec_ptr_at(vec, idx + i, item_size), item_data + i * item_size);
        } else {
            memcpy(vec_ptr_at(vec, idx + i, item_size), item_data + i * item_size, item_size);
        }
    }

    vec->length += count;
}

void insert_range_fast_into_vec(
    GenericVec *vec,
    char       *item_data,
    size_t      item_size,
    size_t      idx,
    size_t      count
) {
    if (!vec || !item_size || !item_data) {
        LOG_FATAL("invalid arguments.");
    }

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

    for (i64 i = 0; i < count; i++) {
        if (vec->copy_init) {
            memset(vec_ptr_at(vec, idx + i, item_size), 0, item_size);
            vec->copy_init(vec_ptr_at(vec, idx + i, item_size), item_data + i * item_size);
        } else {
            memcpy(vec_ptr_at(vec, idx + i, item_size), item_data + i * item_size, item_size);
        }
    }

    vec->length += 1;
}


void remove_range_vec(
    GenericVec *vec,
    void       *removed_data,
    size_t      item_size,
    size_t      start,
    size_t      count
) {
    if (!vec || !item_size) {
        LOG_FATAL("invalid arguments.");
    }

    if (start + count > vec->length) {
        LOG_FATAL("vector range out of bounds.");
    }

    if (removed_data) {
        // make copy of data if user want's a copy
        memcpy(
            removed_data,
            vec_ptr_at(vec, start, item_size),
            count * vec_aligned_size(vec, item_size)
        );
    } else {
        // if no space provided to copy data over to, just destroy or memset it
        if (vec->copy_deinit) {
            char *vec_data = vec_ptr_at(vec, start, item_size);
            for (size_t s = 0; s < count; s++) {
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
    memset(
        vec_ptr_at(vec, (vec->length - count), item_size),
        0,
        count * vec_aligned_size(vec, item_size)
    );

    vec->length -= count;
}


void fast_remove_range_vec(
    GenericVec *vec,
    void       *removed_data,
    size_t      item_size,
    size_t      start,
    size_t      count
) {
    if (!vec || !item_size) {
        LOG_FATAL("invalid arguments.");
    }

    if (start + count > vec->length) {
        LOG_FATAL("vector range out of bounds.");
    }

    if (removed_data) {
        memcpy(
            removed_data,
            vec_ptr_at(vec, start, item_size),
            count * vec_aligned_size(vec, item_size)
        );
    } else {
        if (vec->copy_deinit) {
            char *vec_data = vec_ptr_at(vec, start, item_size);
            for (size_t s = 0; s < count; s++) {
                vec->copy_deinit(vec_data);
                vec_data += vec_aligned_size(vec, item_size);
            }
        } else {
            memset(vec_ptr_at(vec, start, item_size), 0, count * vec_aligned_size(vec, item_size));
        }
    }

    // move just last "count" elements to new created space
    memmove(
        // move to freed up space
        vec_ptr_at(vec, start, item_size),
        // last "count" elements
        vec_ptr_at(vec, (vec->length - count), item_size),
        // "count" items
        count * vec_aligned_size(vec, item_size)
    );
    // memset last count items to 0
    memset(
        vec_ptr_at(vec, (vec->length - count), item_size),
        0,
        count * vec_aligned_size(vec, item_size)
    );

    vec->length -= count;
}


void qsort_vec(GenericVec *vec, size_t item_size, GenericCompare comp) {
    if (!vec || !item_size) {
        LOG_FATAL("invalid arguments.");
    }

    if (vec_aligned_size(vec, item_size) != item_size) {
        LOG_FATAL(
            "QSort not implemented for vectors wherein the size of items don't match their aligned "
            "size."
        );
    }

    qsort(vec->data, vec->length, item_size, comp);
}


void swap_vec(GenericVec *vec, size_t item_size, size_t idx1, size_t idx2) {
    if (!vec || !item_size) {
        LOG_FATAL("invalid arguments.");
    }

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


void reverse_vec(GenericVec *vec, size_t item_size) {
    if (!vec || !item_size) {
        LOG_FATAL("invalid arguments.");
    }

    size_t i = vec->length / 2;
    while (i--) {
        swap_vec(vec, item_size, i, vec->length - (i + 1));
    }
}


void push_arr_vec(GenericVec *vec, size_t item_size, char *arr, size_t count, size_t pos) {
    if (!vec || !arr || !count || !item_size) {
        LOG_FATAL("invalid arguments.");
    }

    if (pos > vec->length) {
        LOG_FATAL("vector index out of range.");
    }

    reserve_pow2_vec(vec, item_size, vec->length + count);

    // shift data if being inserted in the middle
    if (pos < vec->length) {
        memmove(
            vec_ptr_at(vec, (pos + count), item_size),
            vec_ptr_at(vec, pos, item_size),
            count * vec_aligned_size(vec, item_size)
        );

        memset(vec_ptr_at(vec, pos, item_size), 0, count * vec_aligned_size(vec, item_size));
    }

    // insert data
    if (vec->copy_init) {
        char *data = vec_ptr_at(vec, pos, item_size);
        while (count--) {
            vec->copy_init(data, arr);
            arr  += vec_aligned_size(vec, item_size);
            data += vec_aligned_size(vec, item_size);
        }
    } else {
        memcpy(vec_ptr_at(vec, pos, item_size), arr, count * vec_aligned_size(vec, item_size));
    }

    vec->length += count;
}


void resize_vec(GenericVec *vec, size_t item_size, size_t new_size) {
    if (!vec || !item_size) {
        LOG_FATAL("invalid arguments.");
    }

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
