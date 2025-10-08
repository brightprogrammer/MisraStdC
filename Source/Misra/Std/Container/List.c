#include <Misra/Std/Container/List.h>
#include <Misra/Std/Log.h>

void deinit_list(GenericList *list, u64 item_size) {
    if (!list || !item_size) {
        LOG_ERROR("invalid arguments");
        return;
    }

    ValidateList(list);

    clear_list(list, item_size);

    list->head        = NULL;
    list->tail        = NULL;
    list->copy_init   = NULL;
    list->copy_deinit = NULL;
    list->length      = 0;
}


void insert_into_list(GenericList *list, void *item_data, u64 item_size, u64 idx) {
    if (!list || !item_size || !item_data) {
        LOG_FATAL("invalid arguments.");
    }

    ValidateList(list);

    GenericListNode *new_node = calloc(sizeof(GenericListNode), 1);
    new_node->data            = calloc(item_size, 1);

    if (idx < list->length) {
        // get node after which insertion will take place
        GenericListNode *node = 0 == list->length ? NULL : node_at_list(list, item_size, idx)->prev;

        // node can be NULL only when we're inserting at head
        if (node) {
            new_node->prev = node;
            new_node->next = node->next;
            node->next     = new_node;
        } else {
            new_node->next = list->head;
            list->head     = new_node;
            new_node->prev = NULL;
        }

        // insert data new data into current node
        if (list->copy_init) {
            memset(new_node->data, 0, item_size);
            list->copy_init(new_node->data, item_data);
        } else {
            memcpy(new_node->data, item_data, item_size);
        }
    } else if (idx == list->length) {
        GenericListNode *new_tail = new_node;
        GenericListNode *old_tail = list->tail;
        GenericListNode *head     = list->head;

        if (!head) {
            list->head = new_tail;
        }

        if (old_tail) {
            old_tail->next = new_tail;
            new_tail->prev = old_tail;
        } else {
            new_tail->prev = NULL;
        }

        // create dual link & update tail
        list->tail     = new_tail;
        new_tail->next = NULL;

        if (list->copy_init) {
            memset(new_node->data, 0, item_size);
            list->copy_init(new_node->data, item_data);
        } else {
            memcpy(new_node->data, item_data, item_size);
        }
    } else {
        LOG_FATAL("list index out of range.");
    }

    list->length += 1;
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
            memcpy((u8 *)removed_data + c * item_size, node->data, item_size);

            memset(node->data, 0, item_size);
            free(node->data);
            node->data = NULL;

            node = node->next;
        }
    } else {
        // else destroy all data one by one
        GenericListNode *node = node_at_list(list, item_size, start);
        for (u64 c = 0; (c < count) && node; c++) {
            if (list->copy_deinit) {
                list->copy_deinit(node->data);
            } else {
                memset(node->data, 0, item_size);
            }

            free(node->data);
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
        }
        if (next) {
            next->prev = prev;
        }

        // remove link
        node->next = NULL;
        node->prev = NULL;

        // destroy and move ahead
        free(node);
        node = next;
    }
}


void qsort_list(GenericList *list, u64 item_size, GenericCompare comp) {
    if (!list || !item_size) {
        LOG_FATAL("invalid arguments.");
    }

    ValidateList(list);

    void *data       = malloc(item_size * list->length);
    u64   item_count = list->length;
    remove_range_list(list, data, item_size, 0, list->length);
    qsort(data, item_count, item_size, comp);
    push_arr_list(list, item_size, data, item_count);
    free(data);
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


void push_arr_list(GenericList *list, u64 item_size, void *arr, u64 count) {
    if (!list || !arr || !item_size) {
        LOG_FATAL("invalid arguments.");
    }

    if (!count) {
        return;
    }

    ValidateList(list);

    while (count--) {
        GenericListNode *old_tail = list->tail;
        GenericListNode *new_tail = malloc(sizeof(GenericListNode));

        if (!new_tail) {
            LOG_FATAL("Failed to allocate memory for new node");
        }

        new_tail->data = malloc(item_size);
        if (!new_tail->data) {
            free(new_tail);
            LOG_FATAL("Failed to allocate memory for node data");
        }

        // Handle empty list case
        if (old_tail) {
            // List is not empty - create dual link
            old_tail->next = new_tail;
            new_tail->prev = old_tail;
        } else {
            // List is empty - set as head
            list->head     = new_tail;
            new_tail->prev = NULL;
        }

        new_tail->next = NULL;
        list->tail     = new_tail;

        // insert data
        if (list->copy_init) {
            list->copy_init(new_tail->data, arr);
        } else {
            memcpy(new_tail->data, arr, item_size);
        }

        // Only increment length after successful insertion
        list->length++;
        arr = (u8 *)arr + item_size;
    }
}


void merge_list(GenericList *list1, u64 item_size, GenericList *list2) {
    if (!list1 || !item_size || !list2) {
        LOG_FATAL("invalid arguments.");
    }

    ValidateList(list1);
    ValidateList(list2);

    GenericListNode *node = list2->head;
    while (node) {
        insert_into_list(list1, node->data, item_size, list1->length);
        node = node->next;
    }
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

void validate_list(const GenericList *l) {
    if (!(l)) {
        LOG_FATAL("List pointer is NULL.");
    }
    if ((l)->__magic != MISRA_LIST_MAGIC) {
        LOG_FATAL("Invalid list. Either not initialized or corrupted!");
    }
    if ((l)->length > 0) {
        if (!(l)->head) {
            LOG_FATAL("Non-empty list has NULL head.");
        }
        if (!(l)->tail) {
            LOG_FATAL("Non-empty list has NULL tail.");
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
