/// file      : Private.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// copyright : Copyright (c) 2025, Siddharth Mishra, All rights reserved.
///
/// Provides a type-safe list implementation in C

#ifndef MISRA_STD_CONTAINER_LIST_PRIVATE_H
#define MISRA_STD_CONTAINER_LIST_PRIVATE_H

#include <Misra/Std/Container/List/Type.h>
#include <Misra/Std/Memory.h>

void             deinit_list(GenericList *list, u64 item_size);
bool             insert_into_list(GenericList *list, const void *item_data, u64 item_size, u64 idx);
void             remove_range_list(GenericList *list, void *removed_data, u64 item_size, u64 start, u64 count);
bool             qsort_list(GenericList *list, u64 item_size, GenericCompare comp);
void             swap_list(GenericList *list, u64 item_size, u64 idx1, u64 idx2);
void             reverse_list(GenericList *list, u64 item_size);
bool             push_arr_list(GenericList *list, u64 item_size, const void *arr, u64 count);
bool             merge_list(GenericList *list1, u64 item_size, GenericList *list2);
void             resize_list(GenericList *list, u64 item_size, u64 new_size);
void             clear_list(GenericList *list, u64 item_size);
GenericListNode *node_at_list(GenericList *list, u64 item_size, u64 idx);
void            *item_ptr_at_list(GenericList *list, u64 item_size, u64 idx);
size             find_idx_list(GenericList *list, const void *item_data, u64 item_size, GenericCompare comp);
void             validate_list(const GenericList *list);
GenericListNode *get_node_relative_to_list_node(GenericListNode *node, i64 ridx);
GenericListNode *get_node_random_access(GenericList *list, GenericListNode *node, u64 nidx, i64 ridx);
GenericListNode *get_node_for_list_iteration(GenericList *list, GenericListNode *node, u64 nidx, u64 target_idx);

static inline bool list_zero_source_on_success(GenericList *list, void *src, u64 bytes, bool success) {
    if (success && !list->copy_init) {
        MemSet(src, 0, bytes);
    }

    return success;
}

static inline bool list_release_merged_source_on_success(
    GenericList *dst,
    GenericList *src,
    u64          item_size,
    bool         success
) {
    GenericCopyInit   copy_init;
    GenericCopyDeinit copy_deinit;

    if (!success) {
        return false;
    }

    if (!dst->copy_init && src->length) {
        copy_init        = src->copy_init;
        copy_deinit      = src->copy_deinit;
        src->copy_init   = NULL;
        src->copy_deinit = NULL;
        clear_list(src, item_size);
        src->copy_init   = copy_init;
        src->copy_deinit = copy_deinit;
    }

    return true;
}

#endif // MISRA_STD_CONTAINER_LIST_PRIVATE_H
