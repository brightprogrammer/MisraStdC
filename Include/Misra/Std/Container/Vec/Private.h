/// file      : std/container/vec/private.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// copyright : Copyright (c) 2025, Siddharth Mishra, All rights reserved.
///
/// These functions are the backbone of vec operations.

#ifndef MISRA_STD_CONTAINER_VEC_PRIVATE_H
#define MISRA_STD_CONTAINER_VEC_PRIVATE_H

#include "Type.h"

#ifdef __cplusplus
extern "C" {
#endif

    void init_vec(
        GenericVec       *vec,
        size              item_size,
        GenericCopyInit   copy_init,
        GenericCopyDeinit copy_deinit,
        size              alignment
    );
    void deinit_vec(GenericVec *vec, size item_size);
    void clear_vec(GenericVec *vec, size item_size);
    void resize_vec(GenericVec *vec, size item_size, size new_size);
    void reserve_vec(GenericVec *vec, size item_size, size n);
    void reserve_pow2_vec(GenericVec *vec, size item_size, size n);
    void reduce_space_vec(GenericVec *vec, size item_size);
    void insert_range_into_vec(GenericVec *vec, char *item_data, size item_size, size idx, size count);
    void insert_range_fast_into_vec(GenericVec *vec, char *item_data, size item_size, size idx, size count);
    void remove_range_vec(GenericVec *vec, void *removed_data, size item_size, size start, size count);
    void fast_remove_range_vec(GenericVec *vec, void *removed_data, size item_size, size start, size count);
    void qsort_vec(GenericVec *vec, size item_size, GenericCompare comp);
    void swap_vec(GenericVec *vec, size item_size, size idx1, size idx2);
    void reverse_vec(GenericVec *vec, size item_size);

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_VEC_PRIVATE_H
