/// file      : Fuzz/Harness/ListInt.c
/// author    : Generated for MisraStdC fuzzing
/// This is free and unencumbered software released into the public domain.
///
/// List(i32) specific fuzzing implementation

#include "../Harness.h"
#include "ListInt.h"
#include <Misra/Std/Container/List.h>
#include <Misra/Std/Log.h>

// Comparator function for sorting integers
static int compare_ints(const void *a, const void *b) {
    i32 ia = *(const i32 *)a;
    i32 ib = *(const i32 *)b;
    return (ia > ib) - (ia < ib);
}

void init_int_list(IntList *list, DefaultAllocator *alloc) {
    *list = ListInitT(*list, alloc);
}

void deinit_int_list(IntList *list) {
    ListDeinit(list);
}

void fuzz_int_list(
    IntList          *list,
    ListIntFunction   func,
    const uint8_t    *data,
    size_t           *offset,
    size_t            data_size,
    DefaultAllocator *alloc
) {
    switch (func) {
        case LIST_INT_PUSH_BACK : {
            i32 value = (i32)extract_u32(data, offset, data_size);
            ListPushBackR(list, value);
            break;
        }

        case LIST_INT_PUSH_FRONT : {
            i32 value = (i32)extract_u32(data, offset, data_size);
            ListPushFrontR(list, value);
            break;
        }

        case LIST_INT_POP_BACK : {
            if (ListLen(list) > 0) {
                i32 popped;
                ListPopBack(list, &popped);
            }
            break;
        }

        case LIST_INT_POP_FRONT : {
            if (ListLen(list) > 0) {
                i32 popped;
                ListPopFront(list, &popped);
            }
            break;
        }

        case LIST_INT_INSERT : {
            uint16_t idx   = extract_u16(data, offset, data_size);
            i32      value = (i32)extract_u32(data, offset, data_size);

            if (idx <= ListLen(list)) {
                ListInsertR(list, value, idx);
            }
            break;
        }

        case LIST_INT_REMOVE : {
            uint16_t idx = extract_u16(data, offset, data_size);
            if (idx < ListLen(list)) {
                i32 removed;
                ListRemove(list, &removed, idx);
            }
            break;
        }

        case LIST_INT_DELETE : {
            uint16_t idx = extract_u16(data, offset, data_size);
            if (idx < ListLen(list)) {
                ListDelete(list, idx);
            }
            break;
        }

        case LIST_INT_AT : {
            uint16_t idx = extract_u16(data, offset, data_size);
            if (idx < ListLen(list)) {
                volatile i32 value = ListAt(list, idx);
                (void)value; // Prevent optimization
            }
            break;
        }

        case LIST_INT_LEN : {
            volatile uint64_t len = ListLen(list);
            (void)len;
            break;
        }

        case LIST_INT_FIRST : {
            if (ListLen(list) > 0) {
                volatile i32 first = ListFirst(list);
                (void)first;
            }
            break;
        }

        case LIST_INT_LAST : {
            if (ListLen(list) > 0) {
                volatile i32 last = ListLast(list);
                (void)last;
            }
            break;
        }

        // Memory operations
        case LIST_INT_CLEAR : {
            ListClear(list);
            break;
        }

        // Advanced operations
        case LIST_INT_REVERSE : {
            ListReverse(list);
            break;
        }

        case LIST_INT_SWAP_ITEMS : {
            uint16_t idx1 = extract_u16(data, offset, data_size);
            uint16_t idx2 = extract_u16(data, offset, data_size);

            if (ListLen(list) > 1 && idx1 < ListLen(list) && idx2 < ListLen(list)) {
                ListSwapItems(list, idx1, idx2);
            }
            break;
        }

        // Range operations
        case LIST_INT_REMOVE_RANGE : {
            uint16_t start = extract_u16(data, offset, data_size);
            uint8_t  count = extract_u8(data, offset, data_size);
            count          = count % 16;

            if (ListLen(list) > 0 && start < ListLen(list) && count > 0 && start + count <= ListLen(list)) {
                i32 removed_items[16];
                ListRemoveRange(list, removed_items, start, count);
            }
            break;
        }

        case LIST_INT_DELETE_RANGE : {
            uint16_t start = extract_u16(data, offset, data_size);
            uint8_t  count = extract_u8(data, offset, data_size);
            count          = count % 16;

            if (ListLen(list) > 0 && start < ListLen(list) && count > 0 && start + count <= ListLen(list)) {
                ListDeleteRange(list, start, count);
            }
            break;
        }

        // Array operations
        case LIST_INT_PUSH_ARR : {
            uint8_t count = extract_u8(data, offset, data_size);
            count         = count % 8;

            if (count > 0) {
                i32 values[8];
                for (uint8_t i = 0; i < count && *offset + 4 <= data_size; i++) {
                    values[i] = (i32)extract_u32(data, offset, data_size);
                }
                ListPushArrL(list, values, count);
            }
            break;
        }

        // Advanced functions
        case LIST_INT_SORT : {
            ListSort(list, compare_ints);
            break;
        }

        case LIST_INT_PTR_AT : {
            uint16_t idx = extract_u16(data, offset, data_size);
            if (idx < ListLen(list)) {
                volatile i32 *ptr = ListPtrAt(list, idx);
                if (ptr) {
                    volatile i32 val = *ptr;
                    (void)val;
                }
            }
            break;
        }

        case LIST_INT_MERGE : {
            IntList temp  = ListInitT(temp, alloc);
            uint8_t count = extract_u8(data, offset, data_size);
            count         = count % 4;

            for (uint8_t i = 0; i < count && *offset + 4 <= data_size; i++) {
                i32 value = (i32)extract_u32(data, offset, data_size);
                ListPushBackR(&temp, value);
            }

            ListMergeR(list, &temp);
            ListDeinit(&temp); // Clean up temp to prevent memory leak
            break;
        }

        // Foreach operations
        case LIST_INT_FOREACH : {
            if (ListLen(list) > 0) {
                int sum = 0;
                ListForeach(list, item) {
                    sum += item;
                }
                (void)sum; // Suppress unused variable warning
            }
            break;
        }

        case LIST_INT_FOREACH_IDX : {
            if (ListLen(list) > 0) {
                int sum = 0;
                ListForeachIdx(list, item, idx) {
                    sum += item + (int)idx;
                }
                (void)sum; // Suppress unused variable warning
            }
            break;
        }

        case LIST_INT_FOREACH_PTR : {
            if (ListLen(list) > 0) {
                int sum = 0;
                ListForeachPtr(list, item_ptr) {
                    sum += *item_ptr;
                }
                (void)sum; // Suppress unused variable warning
            }
            break;
        }

        case LIST_INT_FOREACH_PTR_IDX : {
            if (ListLen(list) > 0) {
                int sum = 0;
                ListForeachPtrIdx(list, item_ptr, idx) {
                    sum += *item_ptr + (int)idx;
                }
                (void)sum; // Suppress unused variable warning
            }
            break;
        }

        case LIST_INT_FOREACH_REVERSE : {
            if (ListLen(list) > 0) {
                int sum = 0;
                ListForeachReverse(list, item) {
                    sum += item;
                }
                (void)sum; // Suppress unused variable warning
            }
            break;
        }

        case LIST_INT_FOREACH_REVERSE_IDX : {
            if (ListLen(list) > 0) {
                int sum = 0;
                ListForeachReverseIdx(list, item, idx) {
                    sum += item + (int)idx;
                }
                (void)sum; // Suppress unused variable warning
            }
            break;
        }

        case LIST_INT_FOREACH_PTR_REVERSE : {
            if (ListLen(list) > 0) {
                int sum = 0;
                ListForeachPtrReverse(list, item_ptr) {
                    sum += *item_ptr;
                }
                (void)sum; // Suppress unused variable warning
            }
            break;
        }

        case LIST_INT_FOREACH_PTR_REVERSE_IDX : {
            if (ListLen(list) > 0) {
                int sum = 0;
                ListForeachPtrReverseIdx(list, item_ptr, idx) {
                    sum += *item_ptr + (int)idx;
                }
                (void)sum; // Suppress unused variable warning
            }
            break;
        }

        case LIST_INT_FOREACH_IN_RANGE : {
            if (ListLen(list) > 0 && *offset + 8 <= data_size) {
                size_t start = extract_u32(data, offset, data_size) % ListLen(list);
                size_t end   = extract_u32(data, offset, data_size) % (ListLen(list) + 1);
                if (start < end) {
                    int sum = 0;
                    ListForeachInRange(list, item, start, end) {
                        sum += item;
                    }
                    (void)sum; // Suppress unused variable warning
                }
            }
            break;
        }

        case LIST_INT_FOREACH_PTR_IN_RANGE : {
            if (ListLen(list) > 0 && *offset + 8 <= data_size) {
                size_t start = extract_u32(data, offset, data_size) % ListLen(list);
                size_t end   = extract_u32(data, offset, data_size) % (ListLen(list) + 1);
                if (start < end) {
                    int sum = 0;
                    ListForeachPtrInRange(list, item_ptr, start, end) {
                        sum += *item_ptr;
                    }
                    (void)sum; // Suppress unused variable warning
                }
            }
            break;
        }

        case LIST_INT_FOREACH_REVERSE_IN_RANGE : {
            if (ListLen(list) > 0 && *offset + 8 <= data_size) {
                size_t start = extract_u32(data, offset, data_size) % ListLen(list);
                size_t end   = extract_u32(data, offset, data_size) % (ListLen(list) + 1);
                if (start < end) {
                    int sum = 0;
                    ListForeachReverseInRange(list, item, start, end) {
                        sum += item;
                    }
                    (void)sum; // Suppress unused variable warning
                }
            }
            break;
        }

        case LIST_INT_FOREACH_PTR_REVERSE_IN_RANGE : {
            if (ListLen(list) > 0 && *offset + 8 <= data_size) {
                size_t start = extract_u32(data, offset, data_size) % ListLen(list);
                size_t end   = extract_u32(data, offset, data_size) % (ListLen(list) + 1);
                if (start < end) {
                    int sum = 0;
                    ListForeachPtrReverseInRange(list, item_ptr, start, end) {
                        sum += *item_ptr;
                    }
                    (void)sum; // Suppress unused variable warning
                }
            }
            break;
        }

        default :
            break;
    }
}
