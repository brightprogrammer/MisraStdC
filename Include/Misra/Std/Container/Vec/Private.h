/// file      : std/container/vec/private.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// These functions are the backbone of vec operations.

#ifndef MISRA_STD_CONTAINER_VEC_PRIVATE_H
#define MISRA_STD_CONTAINER_VEC_PRIVATE_H

#include "Type.h"
#include <Misra/Std/Memory.h>

#ifdef __cplusplus
extern "C" {
#endif

    void init_vec(
        GenericVec       *vec,
        size              item_size,
        GenericCopyInit   copy_init,
        GenericCopyDeinit copy_deinit,
        size              alignment,
        Allocator         allocator
    );
    void deinit_vec(GenericVec *vec, size item_size);
    void clear_vec(GenericVec *vec, size item_size);
    bool resize_vec(GenericVec *vec, size item_size, size new_size);
    bool reserve_vec(GenericVec *vec, size item_size, size n);
    bool reserve_pow2_vec(GenericVec *vec, size item_size, size n);
    bool reduce_space_vec(GenericVec *vec, size item_size);
    bool clone_vec(GenericVec *dst, const GenericVec *src, size item_size);
    bool insert_range_into_vec(GenericVec *vec, const char *item_data, size item_size, size idx, size count);
    bool insert_range_fast_into_vec(GenericVec *vec, const char *item_data, size item_size, size idx, size count);
    void remove_range_vec(GenericVec *vec, void *removed_data, size item_size, size start, size count);
    void fast_remove_range_vec(GenericVec *vec, void *removed_data, size item_size, size start, size count);
    void qsort_vec(GenericVec *vec, size item_size, GenericCompare comp);
    void swap_vec(GenericVec *vec, size item_size, size idx1, size idx2);
    void reverse_vec(GenericVec *vec, size item_size);
    size find_idx_vec(GenericVec *vec, const void *item_data, size item_size, GenericCompare comp);
    void validate_vec(const GenericVec *v);

    bool vec_insert_one_l(
        GenericVec *vec,
        const void *item_copy,
        void       *source,
        size        item_size,
        size        idx,
        bool        preserve_order
    );
    bool vec_insert_one_r(GenericVec *vec, const void *item_copy, size item_size, size idx, bool preserve_order);
    bool vec_insert_range_l(GenericVec *vec, void *items, size item_size, size idx, size count, bool preserve_order);
    bool vec_insert_range_r(
        GenericVec *vec,
        const void *items,
        size        item_size,
        size        idx,
        size        count,
        bool        preserve_order
    );
    bool vec_merge_l(GenericVec *dst, GenericVec *src, size item_size);
    bool vec_merge_r(GenericVec *dst, const GenericVec *src, size item_size);

    static inline bool vec_zero_source_on_success(GenericVec *vec, void *src, size bytes, bool success) {
        if (success && !vec->copy_init) {
            MemSet(src, 0, bytes);
        }

        return success;
    }

    static inline bool
        vec_release_merged_source_on_success(GenericVec *dst, GenericVec *src, size item_size, bool success) {
        size aligned_item_size;

        if (!success) {
            return false;
        }

        if (!dst->copy_init && src->data) {
            aligned_item_size = ALIGN_UP_POW2(item_size, src->alignment);
            AllocatorFree(&src->allocator, src->data, (src->capacity + 1) * aligned_item_size);
            src->data     = NULL;
            src->length   = 0;
            src->capacity = 0;
        }

        return true;
    }

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_VEC_PRIVATE_H
