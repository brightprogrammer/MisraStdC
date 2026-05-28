/// file      : Fuzz/Harness/VecInt.c
/// author    : Generated for MisraStdC fuzzing
/// This is free and unencumbered software released into the public domain.
///
/// Vec(i32) specific fuzzing implementation

#include "../Harness.h"
#include "VecInt.h"
#include <Misra/Std/Container/Vec.h>
#include <Misra/Std/Log.h>

// Comparator function for sorting integers
static int compare_ints(const void *a, const void *b) {
    i32 ia = *(const i32 *)a;
    i32 ib = *(const i32 *)b;
    return (ia > ib) - (ia < ib);
}

void init_int_vec(IntVec *vec, DefaultAllocator *alloc) {
    *vec = VecInitT(*vec, alloc);
}

void deinit_int_vec(IntVec *vec) {
    VecDeinit(vec);
}

void fuzz_int_vec(
    IntVec           *vec,
    VecIntFunction    func,
    const uint8_t    *data,
    size_t           *offset,
    size_t            size,
    DefaultAllocator *alloc
) {
    switch (func) {
        case VEC_INT_PUSH_BACK : {
            i32 value = (i32)extract_u32(data, offset, size);
            VecPushBack(vec, value);
            break;
        }

        case VEC_INT_PUSH_FRONT : {
            i32 value = (i32)extract_u32(data, offset, size);
            VecPushFront(vec, value);
            break;
        }

        case VEC_INT_POP_BACK : {
            if (VecLen(vec) > 0) {
                i32 popped;
                VecPopBack(vec, &popped);
            }
            break;
        }

        case VEC_INT_POP_FRONT : {
            if (VecLen(vec) > 0) {
                i32 popped;
                VecPopFront(vec, &popped);
            }
            break;
        }

        case VEC_INT_INSERT : {
            uint16_t idx   = extract_u16(data, offset, size);
            i32      value = (i32)extract_u32(data, offset, size);

            if (idx <= VecLen(vec)) {
                VecInsert(vec, value, idx);
            }
            break;
        }

        case VEC_INT_REMOVE : {
            uint16_t idx = extract_u16(data, offset, size);
            if (idx < VecLen(vec)) {
                i32 removed;
                VecRemove(vec, &removed, idx);
            }
            break;
        }

        case VEC_INT_DELETE : {
            uint16_t idx = extract_u16(data, offset, size);
            if (idx < VecLen(vec)) {
                VecDelete(vec, idx);
            }
            break;
        }

        case VEC_INT_AT : {
            uint16_t idx = extract_u16(data, offset, size);
            if (idx < VecLen(vec)) {
                volatile i32 value = VecAt(vec, idx);
                (void)value; // Prevent optimization
            }
            break;
        }

        case VEC_INT_LEN : {
            volatile uint64_t len = VecLen(vec);
            (void)len;
            break;
        }

        case VEC_INT_FIRST : {
            if (VecLen(vec) > 0) {
                volatile i32 first = VecFirst(vec);
                (void)first;
            }
            break;
        }

        case VEC_INT_LAST : {
            if (VecLen(vec) > 0) {
                volatile i32 last = VecLast(vec);
                (void)last;
            }
            break;
        }

        // Memory operations
        case VEC_INT_CLEAR : {
            VecClear(vec);
            break;
        }

        case VEC_INT_RESIZE : {
            uint16_t new_size = extract_u16(data, offset, size);
            new_size          = new_size % 1000; // Limit to reasonable size
            VecResize(vec, new_size);
            break;
        }

        case VEC_INT_RESERVE : {
            uint16_t capacity = extract_u16(data, offset, size);
            capacity          = capacity % 1000; // Limit to reasonable capacity
            VecReserve(vec, capacity);
            break;
        }

        case VEC_INT_TRY_REDUCE_SPACE : {
            VecTryReduceSpace(vec);
            break;
        }

        case VEC_INT_SIZE : {
            volatile uint64_t size_val = VecSize(vec);
            (void)size_val;
            break;
        }

        // Advanced operations
        case VEC_INT_REVERSE : {
            VecReverse(vec);
            break;
        }

        case VEC_INT_SWAP_ITEMS : {
            uint16_t idx1 = extract_u16(data, offset, size);
            uint16_t idx2 = extract_u16(data, offset, size);

            uint64_t len = VecLen(vec);
            if (len > 1 && idx1 < len && idx2 < len) {
                VecSwapItems(vec, idx1, idx2);
            }
            break;
        }

        // Range operations
        case VEC_INT_INSERT_RANGE : {
            uint16_t idx   = extract_u16(data, offset, size);
            uint8_t  count = extract_u8(data, offset, size);
            count          = count % 16;

            if (idx <= VecLen(vec) && count > 0) {
                i32 values[16];
                for (uint8_t i = 0; i < count && *offset + 4 <= size; i++) {
                    values[i] = (i32)extract_u32(data, offset, size);
                }
                VecInsertRangeR(vec, values, idx, count);
            }
            break;
        }

        case VEC_INT_REMOVE_RANGE : {
            uint16_t start = extract_u16(data, offset, size);
            uint8_t  count = extract_u8(data, offset, size);
            count          = count % 16;

            uint64_t len = VecLen(vec);
            if (len > 0 && start < len && count > 0 && start + count <= len) {
                i32 removed_items[16];
                VecRemoveRange(vec, removed_items, start, count);
            }
            break;
        }

        case VEC_INT_DELETE_RANGE : {
            uint16_t start = extract_u16(data, offset, size);
            uint8_t  count = extract_u8(data, offset, size);
            count          = count % 16;

            uint64_t len = VecLen(vec);
            if (len > 0 && start < len && count > 0 && start + count <= len) {
                VecDeleteRange(vec, start, count);
            }
            break;
        }

        case VEC_INT_INSERT_FAST : {
            uint16_t idx   = extract_u16(data, offset, size);
            i32      value = (i32)extract_u32(data, offset, size);

            if (idx <= VecLen(vec)) {
                VecInsertFast(vec, value, idx);
            }
            break;
        }

        case VEC_INT_REMOVE_FAST : {
            uint16_t idx = extract_u16(data, offset, size);
            if (idx < VecLen(vec)) {
                i32 removed;
                VecRemoveFast(vec, &removed, idx);
            }
            break;
        }

        case VEC_INT_REMOVE_RANGE_FAST : {
            uint16_t start = extract_u16(data, offset, size);
            uint8_t  count = extract_u8(data, offset, size);
            count          = count % 16;

            uint64_t len = VecLen(vec);
            if (len > 0 && start < len && count > 0 && start + count <= len) {
                i32 removed_items[16];
                VecRemoveRangeFast(vec, removed_items, start, count);

                // Access removed items to test functionality
                volatile i32 sum = 0;
                for (uint8_t i = 0; i < count; i++) {
                    sum += removed_items[i];
                }
                (void)sum;
            }
            break;
        }

        case VEC_INT_DELETE_RANGE_FAST : {
            uint16_t start = extract_u16(data, offset, size);
            uint8_t  count = extract_u8(data, offset, size);
            count          = count % 16;

            uint64_t len = VecLen(vec);
            if (len > 0 && start < len && count > 0 && start + count <= len) {
                VecDeleteRangeFast(vec, start, count);
            }
            break;
        }

        // Array operations
        case VEC_INT_PUSH_BACK_ARRAY : {
            uint8_t count = extract_u8(data, offset, size);
            count         = count % 8;

            if (count > 0) {
                i32 values[8];
                for (uint8_t i = 0; i < count && *offset + 4 <= size; i++) {
                    values[i] = (i32)extract_u32(data, offset, size);
                }
                VecPushBackArrR(vec, values, count);
            }
            break;
        }

        case VEC_INT_PUSH_FRONT_ARRAY : {
            uint8_t count = extract_u8(data, offset, size);
            count         = count % 8;

            if (count > 0) {
                i32 values[8];
                for (uint8_t i = 0; i < count && *offset + 4 <= size; i++) {
                    values[i] = (i32)extract_u32(data, offset, size);
                }
                VecPushFrontArrR(vec, values, count);
            }
            break;
        }

        case VEC_INT_PUSH_FRONT_ARRAY_FAST : {
            uint8_t count = extract_u8(data, offset, size);
            count         = count % 8;

            if (count > 0) {
                i32 values[8];
                for (uint8_t i = 0; i < count && *offset + 4 <= size; i++) {
                    values[i] = (i32)extract_u32(data, offset, size);
                }
                VecPushFrontArrFastR(vec, values, count);
            }
            break;
        }

        // Advanced functions
        case VEC_INT_SORT : {
            VecSort(vec, compare_ints);
            break;
        }

        case VEC_INT_BEGIN : {
            volatile i32 *begin_ptr = (i32 *)VecBegin(vec);
            (void)begin_ptr;
            break;
        }

        case VEC_INT_END : {
            volatile void *end_ptr = VecEnd(vec);
            (void)end_ptr;
            break;
        }

        case VEC_INT_PTR_AT : {
            uint16_t idx = extract_u16(data, offset, size);
            if (idx < VecLen(vec)) {
                volatile i32 *ptr = VecPtrAt(vec, idx);
                volatile i32  val = *ptr;
                (void)val;
            }
            break;
        }

        case VEC_INT_MERGE : {
            IntVec  temp  = VecInitT(temp, alloc);
            uint8_t count = extract_u8(data, offset, size);
            count         = count % 4;

            for (uint8_t i = 0; i < count && *offset + 4 <= size; i++) {
                i32 value = (i32)extract_u32(data, offset, size);
                VecPushBack(&temp, value);
            }

            VecMerge(vec, &temp);
            VecDeinit(&temp); // Clean up temp to prevent memory leak
            break;
        }

        case VEC_INT_INSERT_RANGE_FAST : {
            uint16_t idx   = extract_u16(data, offset, size);
            uint8_t  count = extract_u8(data, offset, size);
            count          = count % 16;

            if (idx <= VecLen(vec) && count > 0) {
                i32 values[16];
                for (uint8_t i = 0; i < count && *offset + 4 <= size; i++) {
                    values[i] = (i32)extract_u32(data, offset, size);
                }
                VecInsertRangeFastR(vec, values, idx, count);
            }
            break;
        }

        case VEC_INT_ALIGNED_OFFSET_AT : {
            uint16_t idx = extract_u16(data, offset, size);
            if (idx <= VecLen(vec)) {
                volatile uint64_t offset_val = VecAlignedOffsetAt(vec, idx);
                (void)offset_val;
            }
            break;
        }

        case VEC_INT_DELETE_LAST : {
            if (VecLen(vec) > 0) {
                VecDeleteLast(vec);
            }
            break;
        }

        case VEC_INT_DELETE_FAST : {
            uint16_t idx = extract_u16(data, offset, size);
            if (idx < VecLen(vec)) {
                VecDeleteFast(vec, idx);
            }
            break;
        }

        case VEC_INT_INIT_CLONE : {
            IntVec  temp  = VecInitT(temp, alloc);
            uint8_t count = extract_u8(data, offset, size);
            count         = count % 4;

            for (uint8_t i = 0; i < count && *offset + 4 <= size; i++) {
                i32 value = (i32)extract_u32(data, offset, size);
                VecPushBack(&temp, value);
            }

            VecInitClone(vec, &temp);

            VecDeinit(&temp); // Clean up temp to prevent memory leak
            break;
        }

        // Foreach operations
        case VEC_INT_FOREACH : {
            if (VecLen(vec) > 0) {
                int sum = 0;
                VecForeach(vec, item) {
                    sum += item;
                }
                (void)sum; // Suppress unused variable warning
            }
            break;
        }

        case VEC_INT_FOREACH_IDX : {
            if (VecLen(vec) > 0) {
                int sum = 0;
                VecForeachIdx(vec, item, idx) {
                    sum += item + (int)idx;
                }
                (void)sum; // Suppress unused variable warning
            }
            break;
        }

        case VEC_INT_FOREACH_PTR : {
            if (VecLen(vec) > 0) {
                int sum = 0;
                VecForeachPtr(vec, item_ptr) {
                    sum += *item_ptr;
                }
                (void)sum; // Suppress unused variable warning
            }
            break;
        }

        case VEC_INT_FOREACH_PTR_IDX : {
            if (VecLen(vec) > 0) {
                int sum = 0;
                VecForeachPtrIdx(vec, item_ptr, idx) {
                    sum += *item_ptr + (int)idx;
                }
                (void)sum; // Suppress unused variable warning
            }
            break;
        }

        case VEC_INT_FOREACH_REVERSE : {
            if (VecLen(vec) > 0) {
                int sum = 0;
                VecForeachReverse(vec, item) {
                    sum += item;
                }
                (void)sum; // Suppress unused variable warning
            }
            break;
        }

        case VEC_INT_FOREACH_REVERSE_IDX : {
            if (VecLen(vec) > 0) {
                int sum = 0;
                VecForeachReverseIdx(vec, item, idx) {
                    sum += item + (int)idx;
                }
                (void)sum; // Suppress unused variable warning
            }
            break;
        }

        case VEC_INT_FOREACH_PTR_REVERSE : {
            if (VecLen(vec) > 0) {
                int sum = 0;
                VecForeachPtrReverse(vec, item_ptr) {
                    sum += *item_ptr;
                }
                (void)sum; // Suppress unused variable warning
            }
            break;
        }

        case VEC_INT_FOREACH_PTR_REVERSE_IDX : {
            if (VecLen(vec) > 0) {
                int sum = 0;
                VecForeachPtrReverseIdx(vec, item_ptr, idx) {
                    sum += *item_ptr + (int)idx;
                }
                (void)sum; // Suppress unused variable warning
            }
            break;
        }

        case VEC_INT_FOREACH_IN_RANGE : {
            if (VecLen(vec) > 0 && *offset + 8 <= size) {
                size_t start = extract_u32(data, offset, size) % VecLen(vec);
                size_t end   = extract_u32(data, offset, size) % (VecLen(vec) + 1);
                if (start < end) {
                    int sum = 0;
                    VecForeachInRange(vec, item, start, end) {
                        sum += item;
                    }
                    (void)sum; // Suppress unused variable warning
                }
            }
            break;
        }

        case VEC_INT_FOREACH_IN_RANGE_IDX : {
            if (VecLen(vec) > 0 && *offset + 8 <= size) {
                size_t start = extract_u32(data, offset, size) % VecLen(vec);
                size_t end   = extract_u32(data, offset, size) % (VecLen(vec) + 1);
                if (start < end) {
                    int sum = 0;
                    VecForeachInRangeIdx(vec, item, idx, start, end) {
                        sum += item + (int)idx;
                    }
                    (void)sum; // Suppress unused variable warning
                }
            }
            break;
        }

        case VEC_INT_FOREACH_PTR_IN_RANGE : {
            if (VecLen(vec) > 0 && *offset + 8 <= size) {
                size_t start = extract_u32(data, offset, size) % VecLen(vec);
                size_t end   = extract_u32(data, offset, size) % (VecLen(vec) + 1);
                if (start < end) {
                    int sum = 0;
                    VecForeachPtrInRange(vec, item_ptr, start, end) {
                        sum += *item_ptr;
                    }
                    (void)sum; // Suppress unused variable warning
                }
            }
            break;
        }

        case VEC_INT_FOREACH_PTR_IN_RANGE_IDX : {
            if (VecLen(vec) > 0 && *offset + 8 <= size) {
                size_t start = extract_u32(data, offset, size) % VecLen(vec);
                size_t end   = extract_u32(data, offset, size) % (VecLen(vec) + 1);
                if (start < end) {
                    int sum = 0;
                    VecForeachPtrInRangeIdx(vec, item_ptr, idx, start, end) {
                        sum += *item_ptr + (int)idx;
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
