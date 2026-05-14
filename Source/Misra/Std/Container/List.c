#include <Misra/Std/Container/List.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Memory.h>

#include <stddef.h>

static inline size list_alloc_alignment(void) {
    return _Alignof(max_align_t);
}

static inline GenericListNode *alloc_list_node(GenericList *list) {
    return AllocatorAlloc(&list->allocator, sizeof(GenericListNode), true);
}

static inline void free_list_node(GenericList *list, GenericListNode *node) {
    AllocatorFree(&list->allocator, node, sizeof(GenericListNode));
}

static inline void *alloc_list_item(GenericList *list, u64 item_size) {
    return AllocatorAlloc(&list->allocator, item_size, true);
}

static inline void free_list_item(GenericList *list, void *item, u64 item_size) {
    AllocatorFree(&list->allocator, item, item_size);
}

void deinit_list(GenericList *list, u64 item_size) {
    if (!list || !item_size) {
        LOG_FATAL("invalid arguments.");
    }

    ValidateList(list);

    clear_list(list, item_size);

    list->head        = NULL;
    list->tail        = NULL;
    list->copy_init   = NULL;
    list->copy_deinit = NULL;
    list->length      = 0;
    AllocatorUnbind(&list->allocator);
    list->allocator = AllocatorBind(DefaultAllocator());
}


bool insert_into_list(GenericList *list, const void *item_data, u64 item_size, u64 idx) {
    GenericListNode *new_node;
    GenericListNode *next_node;
    GenericListNode *prev_node;

    if (!list || !item_size || !item_data) {
        LOG_FATAL("invalid arguments.");
    }

    ValidateList(list);

    if (idx > list->length) {
        LOG_FATAL("list index out of range.");
    }

    new_node = alloc_list_node(list);
    if (!new_node) {
        return false;
    }

    new_node->data = alloc_list_item(list, item_size);
    if (!new_node->data) {
        free_list_node(list, new_node);
        return false;
    }

    if (list->copy_init) {
        if (!list->copy_init(new_node->data, item_data, &list->allocator)) {
            free_list_item(list, new_node->data, item_size);
            free_list_node(list, new_node);
            return false;
        }
    } else {
        MemCopy(new_node->data, item_data, item_size);
    }

    if (idx == list->length) {
        prev_node      = list->tail;
        new_node->prev = prev_node;
        new_node->next = NULL;

        if (prev_node) {
            prev_node->next = new_node;
        } else {
            list->head = new_node;
        }

        list->tail = new_node;
    } else {
        next_node       = node_at_list(list, item_size, idx);
        prev_node       = next_node->prev;
        new_node->next  = next_node;
        new_node->prev  = prev_node;
        next_node->prev = new_node;

        if (prev_node) {
            prev_node->next = new_node;
        } else {
            list->head = new_node;
        }
    }

    list->length += 1;
    return true;
}

void remove_range_list(GenericList *list, void *removed_data, u64 item_size, u64 start, u64 count) {
    if (!list || !item_size) {
        LOG_FATAL("invalid arguments.");
    }

    ValidateList(list);

    if (count == 0) {
        return;
    }

    if (start + count > list->length) {
        LOG_FATAL("List range out of bounds.");
    }

    // if a buffer is provided, move data there
    if (removed_data) {
        GenericListNode *node = node_at_list(list, item_size, start);
        for (u64 c = 0; (c < count) && node; c++) {
            MemCopy((u8 *)removed_data + c * item_size, node->data, item_size);

            MemSet(node->data, 0, item_size);
            free_list_item(list, node->data, item_size);
            node->data = NULL;

            node = node->next;
        }
    } else {
        // else destroy all data one by one
        GenericListNode *node = node_at_list(list, item_size, start);
        for (u64 c = 0; (c < count) && node; c++) {
            if (list->copy_deinit) {
                list->copy_deinit(node->data, &list->allocator);
            } else {
                MemSet(node->data, 0, item_size);
            }

            free_list_item(list, node->data, item_size);
            node->data = NULL;
            node       = node->next;
        }
    }

    // remove nodes
    GenericListNode *node = node_at_list(list, item_size, start);
    while (node && count-- && list->length--) {
        // update link
        GenericListNode *next = node->next;
        GenericListNode *prev = node->prev;
        if (prev) {
            prev->next = next;
        } else {
            list->head = next;
        }
        if (next) {
            next->prev = prev;
        } else {
            list->tail = prev;
        }

        // remove link
        node->next = NULL;
        node->prev = NULL;

        // destroy and move ahead
        free_list_node(list, node);
        node = next;
    }
}


bool qsort_list(GenericList *list, u64 item_size, GenericCompare comp) {
    GenericListNode *node;
    void            *data;
    u64              item_count;
    u64              index;

    if (!list || !item_size || !comp) {
        LOG_FATAL("invalid arguments.");
    }

    ValidateList(list);

    if (list->length < 2) {
        return true;
    }

    item_count = list->length;
    data       = AllocatorAlloc(&list->allocator, item_size * item_count, false);
    if (!data) {
        return false;
    }

    node = list->head;
    for (index = 0; node && index < item_count; index++) {
        MemCopy((u8 *)data + index * item_size, node->data, item_size);
        node = node->next;
    }

    qsort(data, item_count, item_size, comp);

    node = list->head;
    for (index = 0; node && index < item_count; index++) {
        MemCopy(node->data, (u8 *)data + index * item_size, item_size);
        node = node->next;
    }

    AllocatorFree(&list->allocator, data, item_size * item_count);
    return true;
}


void swap_list(GenericList *list, u64 item_size, u64 idx1, u64 idx2) {
    if (!list || !item_size) {
        LOG_FATAL("invalid arguments.");
    }

    ValidateList(list);

    GenericListNode *n1 = node_at_list(list, item_size, idx1);
    if (!n1) {
        LOG_FATAL("failed to get node at specified index");
    }

    GenericListNode *n2 = node_at_list(list, item_size, idx2);
    if (!n2) {
        LOG_FATAL("failed to get node at specified index");
    }

    unsigned char *a, *b, tmp;
    a = n1->data;
    b = n2->data;
    while (item_size--) {
        tmp = *a;
        *a  = *b;
        *b  = tmp;
        a++, b++;
    }
}


void reverse_list(GenericList *list, u64 item_size) {
    if (!list || !item_size) {
        LOG_FATAL("invalid arguments.");
    }

    ValidateList(list);

    u64 i = list->length / 2;
    while (i--) {
        swap_list(list, item_size, i, list->length - (i + 1));
    }
}


bool push_arr_list(GenericList *list, u64 item_size, const void *arr, u64 count) {
    const u8 *cursor;
    u64       old_length;
    u64       inserted;

    if (!list || !item_size) {
        LOG_FATAL("invalid arguments.");
    }

    ValidateList(list);

    if (!count) {
        return true;
    }

    if (!arr) {
        LOG_FATAL("invalid arguments.");
    }

    old_length = list->length;
    cursor     = arr;
    inserted   = 0;

    while (inserted < count) {
        if (!insert_into_list(list, cursor, item_size, list->length)) {
            if (list->length > old_length) {
                remove_range_list(list, NULL, item_size, old_length, list->length - old_length);
            }
            return false;
        }

        inserted += 1;
        cursor   += item_size;
    }

    return true;
}


bool merge_list(GenericList *list1, u64 item_size, GenericList *list2) {
    GenericListNode *node;
    u64              old_length;

    if (!list1 || !item_size || !list2) {
        LOG_FATAL("invalid arguments.");
    }

    ValidateList(list1);
    ValidateList(list2);

    if (list1 == list2) {
        LOG_FATAL("cannot merge list with itself.");
    }

    old_length = list1->length;
    node       = list2->head;
    while (node) {
        if (!insert_into_list(list1, node->data, item_size, list1->length)) {
            if (list1->length > old_length) {
                remove_range_list(list1, NULL, item_size, old_length, list1->length - old_length);
            }
            return false;
        }
        node = node->next;
    }

    return true;
}


void clear_list(GenericList *list, u64 item_size) {
    if (!list || !item_size) {
        LOG_FATAL("invalid arguments.");
    }

    ValidateList(list);

    remove_range_list(list, NULL, item_size, 0, list->length);
}


GenericListNode *node_at_list(GenericList *list, u64 item_size, u64 idx) {
    if (!list || !item_size) {
        LOG_FATAL("invalid arguments.");
    }

    ValidateList(list);

    if (idx >= list->length) {
        LOG_FATAL("list index out of range.");
    }

    GenericListNode *node = list->head;
    for (u64 i = 0; i < idx; i++) {
        node = node->next;
    }
    return node;
}


void *item_ptr_at_list(GenericList *list, u64 item_size, u64 idx) {
    if (!list || !item_size) {
        LOG_FATAL("invalid arguments.");
    }

    ValidateList(list);

    if (idx >= list->length) {
        LOG_FATAL("list index out of bounds.");
    }

    GenericListNode *node = node_at_list(list, item_size, idx);
    return node->data;
}

size find_idx_list(GenericList *list, const void *item_data, u64 item_size, GenericCompare comp) {
    if (!list || !item_data || !item_size || !comp) {
        LOG_FATAL("invalid arguments.");
    }

    ValidateList(list);

    GenericListNode *node = list->head;
    size             idx  = 0;
    while (node) {
        if (comp(node->data, item_data) == 0) {
            return idx;
        }
        node = node->next;
        idx++;
    }

    return SIZE_MAX;
}

void validate_list(const GenericList *l) {
    if (!(l)) {
        LOG_FATAL("List pointer is NULL.");
    }
    if ((l)->__magic != MISRA_LIST_MAGIC) {
        LOG_FATAL("Invalid list. Either not initialized or corrupted!");
    }
    if (!(l)->allocator.allocate || !(l)->allocator.reallocate || !(l)->allocator.deallocate) {
        LOG_FATAL("Invalid list allocator.");
    }
    if ((l)->length == 0) {
        if ((l)->head || (l)->tail) {
            LOG_FATAL("Empty list must have NULL head and tail.");
        }
    } else {
        if (!(l)->head) {
            LOG_FATAL("Non-empty list has NULL head.");
        }
        if (!(l)->tail) {
            LOG_FATAL("Non-empty list has NULL tail.");
        }
        if ((l)->head->prev) {
            LOG_FATAL("List head must not have a previous node.");
        }
        if ((l)->tail->next) {
            LOG_FATAL("List tail must not have a next node.");
        }
    }
}

GenericListNode *get_node_relative_to_list_node(GenericListNode *node, i64 ridx) {
    if (!node) {
        LOG_FATAL("Invalid arguments");
    }

    if (ridx > 0) {
        while (node->next && ridx) {
            node = node->next;
            ridx--;
        }
        if (!node->next && ridx) {
            return NULL;
        }
    } else if (ridx < 0) {
        while (node->prev && ridx < 0) {
            node = node->prev;
            ridx++;
        }
        if (!node->prev && ridx < 0) {
            return NULL;
        }
    }

    return node;
}

GenericListNode *get_node_random_access(GenericList *list, GenericListNode *node, u64 nidx, i64 ridx) {
    if (!list || !node) {
        LOG_FATAL("Invalid arguments");
    }

    if (nidx >= list->length) {
        LOG_FATAL("Node index exceeds list bounds");
    }

    if ((ridx < 0 && (u64)(-ridx) > nidx) || (ridx > 0 && nidx + (u64)ridx >= list->length)) {
        LOG_FATAL("Relative node index outside of list bounds");
    }

    ValidateList(list);

    u64 abs_target_idx = nidx + ridx;
    u64 dist_from_node = (nidx > abs_target_idx) ? nidx - abs_target_idx : abs_target_idx - nidx;
    u64 dist_from_head = abs_target_idx;
    u64 dist_from_tail = list->length - 1 - abs_target_idx;

    GenericListNode *cur = NULL;
    if (dist_from_node <= dist_from_head && dist_from_node <= dist_from_tail) {
        // Traverse from current node
        cur       = node;
        i64 steps = ridx;
        while (steps > 0 && cur) {
            cur = cur->next;
            steps--;
        }
        while (steps < 0 && cur) {
            cur = cur->prev;
            steps++;
        }
        return cur;
    } else if (dist_from_head <= dist_from_tail) {
        // Traverse from head
        cur = list->head;
        for (u64 i = 0; i < abs_target_idx && cur; i++) {
            cur = cur->next;
        }
        return cur;
    } else {
        // Traverse from tail
        cur = list->tail;
        for (u64 i = list->length - 1; i > abs_target_idx && cur; i--) {
            cur = cur->prev;
        }
        return cur;
    }
}

GenericListNode *get_node_for_list_iteration(GenericList *list, GenericListNode *node, u64 nidx, u64 target_idx) {
    if (!list) {
        LOG_FATAL("Invalid arguments");
    }

    ValidateList(list);

    if (target_idx >= list->length) {
        LOG_FATAL("Node index exceeds list bounds");
    }

    if (!node) {
        u64 dist_from_head = target_idx;
        u64 dist_from_tail = list->length - 1 - target_idx;

        if (dist_from_head <= dist_from_tail) {
            GenericListNode *cur = list->head;
            for (u64 i = 0; i < target_idx && cur; i++) {
                cur = cur->next;
            }
            return cur;
        } else {
            GenericListNode *cur = list->tail;
            for (u64 i = list->length - 1; i > target_idx && cur; i--) {
                cur = cur->prev;
            }
            return cur;
        }
    }

    return get_node_random_access(list, node, nidx, (i64)target_idx - (i64)nidx);
}

bool list_insert_one_l(GenericList *list, const void *item_copy, void *source, u64 item_size, u64 idx) {
    return list_zero_source_on_success(list, source, item_size, insert_into_list(list, item_copy, item_size, idx));
}

bool list_insert_one_r(GenericList *list, const void *item_copy, u64 item_size, u64 idx) {
    return insert_into_list(list, item_copy, item_size, idx);
}

bool list_insert_range_l(GenericList *list, void *items, u64 item_size, u64 count) {
    if (!count) {
        return true;
    }

    if (!items) {
        LOG_FATAL("Expected a valid pointer");
    }

    return list_zero_source_on_success(list, items, item_size * count, push_arr_list(list, item_size, items, count));
}

bool list_insert_range_r(GenericList *list, const void *items, u64 item_size, u64 count) {
    if (!count) {
        return true;
    }

    if (!items) {
        LOG_FATAL("Expected a valid pointer");
    }

    return push_arr_list(list, item_size, items, count);
}

bool list_merge_l(GenericList *dst, GenericList *src, u64 item_size) {
    if (!src->length) {
        return true;
    }

    return list_release_merged_source_on_success(dst, src, item_size, merge_list(dst, item_size, src));
}

bool list_merge_r(GenericList *dst, GenericList *src, u64 item_size) {
    if (!src->length) {
        return true;
    }

    return merge_list(dst, item_size, src);
}
