/// file      : Private.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// copyright : Copyright (c) 2025, Siddharth Mishra, All rights reserved.
///
/// Provides a type-safe list implementation in C

#ifndef MISRA_STD_CONTAINER_LIST_PRIVATE_H
#define MISRA_STD_CONTAINER_LIST_PRIVATE_H

#include <Misra/Std/Container/List/Type.h>

void             deinit_list(GenericList *list, u64 item_size);
void             insert_into_list(GenericList *list, void *item_data, u64 item_size, u64 idx);
void             remove_range_list(GenericList *list, void *removed_data, u64 item_size, u64 start, u64 count);
void             qsort_list(GenericList *list, u64 item_size, GenericCompare comp);
void             swap_list(GenericList *list, u64 item_size, u64 idx1, u64 idx2);
void             reverse_list(GenericList *list, u64 item_size);
void             push_arr_list(GenericList *list, u64 item_size, void *arr, u64 count);
void             merge_list(GenericList *list1, u64 item_size, GenericList *list2);
void             resize_list(GenericList *list, u64 item_size, u64 new_size);
void             clear_list(GenericList *list, u64 item_size);
GenericListNode *node_at_list(GenericList *list, u64 item_size, u64 idx);
void            *item_ptr_at_list(GenericList *list, u64 item_size, u64 idx);
void             validate_list(const GenericList *list);
GenericListNode *get_node_relative_to_list_node(GenericListNode *node, i64 ridx);

#endif // MISRA_STD_CONTAINER_LIST_PRIVATE_H
