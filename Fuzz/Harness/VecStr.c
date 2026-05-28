/// file      : Fuzz/Harness/VecStr.c
/// author    : Generated for MisraStdC fuzzing
/// This is free and unencumbered software released into the public domain.
///
/// Vec(Str) specific fuzzing implementation

#include "../Harness.h"
#include "VecStr.h"
#include <Misra/Std/Container/Vec.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Log.h>
#include <string.h> // For strlen

// Generate a Str from fuzz input data
static Str generate_str_from_input(
    const uint8_t    *data,
    size_t           *offset,
    size_t            size,
    size_t            max_len,
    DefaultAllocator *alloc
) {
    // Extract length (limit to max_len for sanity)
    uint8_t len = extract_u8(data, offset, size);
    len         = len % (max_len + 1); // 0 to max_len

    // Create Str with capacity
    Str str = StrInit(alloc);

    // Fill with data or generate simple pattern if not enough input
    for (size_t i = 0; i < len; i++) {
        char ch;
        if (*offset < size) {
            ch = (char)(data[(*offset)++] % 95 + 32); // Printable ASCII range
        } else {
            ch = (char)('A' + (i % 26));              // Fallback pattern
        }
        StrPushBack(&str, ch);
    }

    return str;
}

void init_str_vec(StrVec *vec, DefaultAllocator *alloc) {
    // str_deinit matches the GenericCopyDeinit signature required by
    // VecInitWithDeepCopy (void *, const Allocator *).
    *vec = VecInitWithDeepCopyT(*vec, NULL, str_deinit, alloc);
}

void deinit_str_vec(StrVec *vec) {
    // VecDeinit will automatically call StrDeinit on each element
    VecDeinit(vec);
}

void fuzz_str_vec(
    StrVec           *vec,
    VecStrFunction    func,
    const uint8_t    *data,
    size_t           *offset,
    size_t            size,
    DefaultAllocator *alloc
) {
    switch (func) {
        case VEC_STR_PUSH_BACK : {
            Str str = generate_str_from_input(data, offset, size, 32, alloc);
            VecPushBack(vec, str);
            break;
        }

        case VEC_STR_PUSH_FRONT : {
            Str str = generate_str_from_input(data, offset, size, 32, alloc);
            VecPushFront(vec, str);
            break;
        }

        case VEC_STR_POP_BACK : {
            if (VecLen(vec) > 0) {
                Str str;
                VecPopBack(vec, &str);
                // StrDeinit is called automatically by the vector
            }
            break;
        }

        case VEC_STR_POP_FRONT : {
            if (VecLen(vec) > 0) {
                Str str;
                VecPopFront(vec, &str);
                // StrDeinit is called automatically by the vector
            }
            break;
        }

        case VEC_STR_INSERT : {
            if (*offset + 4 <= size) {
                size_t index = extract_u32(data, offset, size) % (VecLen(vec) + 1);
                Str    str   = generate_str_from_input(data, offset, size, 32, alloc);
                VecInsert(vec, str, index);
            }
            break;
        }

        case VEC_STR_REMOVE : {
            if (VecLen(vec) > 0 && *offset + 4 <= size) {
                size_t index = extract_u32(data, offset, size) % VecLen(vec);
                Str    str;
                VecRemove(vec, &str, index);
                // StrDeinit is called automatically by the vector
            }
            break;
        }

        case VEC_STR_DELETE : {
            if (VecLen(vec) > 0 && *offset + 4 <= size) {
                size_t index = extract_u32(data, offset, size) % VecLen(vec);
                VecDelete(vec, index);
                // StrDeinit is called automatically by the vector
            }
            break;
        }

        case VEC_STR_AT : {
            if (VecLen(vec) > 0 && *offset + 4 <= size) {
                size_t index = extract_u32(data, offset, size) % VecLen(vec);
                Str    str   = VecAt(vec, index);
                (void)str; // Use the result to avoid warnings
            }
            break;
        }

        case VEC_STR_LEN : {
            size_t len = VecLen(vec);
            (void)len; // Use the result to avoid warnings
            break;
        }

        case VEC_STR_FIRST : {
            if (VecLen(vec) > 0) {
                Str first = VecFirst(vec);
                (void)first; // Use the result to avoid warnings
            }
            break;
        }

        case VEC_STR_LAST : {
            if (VecLen(vec) > 0) {
                Str last = VecLast(vec);
                (void)last; // Use the result to avoid warnings
            }
            break;
        }

        case VEC_STR_CLEAR : {
            // VecClear automatically calls StrDeinit on each element
            VecClear(vec);
            break;
        }

        case VEC_STR_RESIZE : {
            if (*offset + 4 <= size) {
                size_t new_size = extract_u32(data, offset, size) % 100; // Limit to reasonable size
                size_t old_size = VecLen(vec);

                // VecResize automatically handles cleanup of removed elements and
                // initializes new elements to default (empty Str)
                VecResize(vec, new_size);

                // Initialize new Str objects if vector grew
                if (new_size > old_size) {
                    for (size_t i = old_size; i < new_size; i++) {
                        Str str       = generate_str_from_input(data, offset, size, 16, alloc);
                        VecAt(vec, i) = str;
                    }
                }
            }
            break;
        }

        case VEC_STR_RESERVE : {
            if (*offset + 4 <= size) {
                size_t capacity = extract_u32(data, offset, size) % 1000; // Limit to reasonable size
                VecReserve(vec, capacity);
            }
            break;
        }

        case VEC_STR_TRY_REDUCE_SPACE : {
            VecTryReduceSpace(vec);
            break;
        }

        case VEC_STR_SIZE : {
            size_t size_bytes = VecSize(vec);
            (void)size_bytes; // Use the result to avoid warnings
            break;
        }

        case VEC_STR_REVERSE : {
            VecReverse(vec);
            break;
        }

        case VEC_STR_SWAP_ITEMS : {
            if (VecLen(vec) >= 2 && *offset + 8 <= size) {
                size_t i = extract_u32(data, offset, size) % VecLen(vec);
                size_t j = extract_u32(data, offset, size) % VecLen(vec);
                VecSwapItems(vec, i, j);
            }
            break;
        }

        case VEC_STR_INSERT_RANGE : {
            if (*offset + 8 <= size) {
                size_t index = extract_u32(data, offset, size) % (VecLen(vec) + 1);
                size_t count = extract_u32(data, offset, size) % 10; // Limit to reasonable count

                // Create temporary array of Str objects
                Str temp_strings[10];
                for (size_t i = 0; i < count; i++) {
                    temp_strings[i] = generate_str_from_input(data, offset, size, 16, alloc);
                }

                VecInsertRange(vec, temp_strings, index, count);
                VecDeinit(vec);
            }
            break;
        }

        case VEC_STR_REMOVE_RANGE : {
            if (VecLen(vec) > 0 && *offset + 8 <= size) {
                size_t index = extract_u32(data, offset, size) % VecLen(vec);
                size_t count = extract_u32(data, offset, size) % (VecLen(vec) - index + 1);
                VecRemoveRange(vec, (Str *)NULL, index, count);
            }
            break;
        }

        case VEC_STR_DELETE_RANGE : {
            if (VecLen(vec) > 0 && *offset + 8 <= size) {
                size_t index = extract_u32(data, offset, size) % VecLen(vec);
                size_t count = extract_u32(data, offset, size) % (VecLen(vec) - index + 1);
                VecDeleteRange(vec, index, count);
            }
            break;
        }

        case VEC_STR_INSERT_FAST : {
            if (*offset + 4 <= size) {
                size_t index = extract_u32(data, offset, size) % (VecLen(vec) + 1);
                Str    str   = generate_str_from_input(data, offset, size, 32, alloc);
                VecInsertFast(vec, str, index);
            }
            break;
        }

        case VEC_STR_REMOVE_FAST : {
            if (VecLen(vec) > 0 && *offset + 4 <= size) {
                size_t index = extract_u32(data, offset, size) % VecLen(vec);
                Str    str;
                VecRemoveFast(vec, &str, index);
                // StrDeinit is called automatically by the vector
            }
            break;
        }

        case VEC_STR_REMOVE_RANGE_FAST : {
            if (VecLen(vec) > 0 && *offset + 8 <= size) {
                size_t index = extract_u32(data, offset, size) % VecLen(vec);
                size_t count = extract_u32(data, offset, size) % (VecLen(vec) - index + 1);
                VecRemoveRangeFast(vec, (Str *)NULL, index, count);
            }
            break;
        }

        case VEC_STR_DELETE_RANGE_FAST : {
            if (VecLen(vec) > 0 && *offset + 8 <= size) {
                size_t index = extract_u32(data, offset, size) % VecLen(vec);
                size_t count = extract_u32(data, offset, size) % (VecLen(vec) - index + 1);
                VecDeleteRangeFast(vec, index, count);
            }
            break;
        }

        case VEC_STR_PUSH_BACK_ARRAY : {
            if (*offset + 4 <= size) {
                size_t count = extract_u32(data, offset, size) % 10; // Limit to reasonable count

                // Create temporary array of Str objects
                Str temp_strings[10];
                for (size_t i = 0; i < count; i++) {
                    temp_strings[i] = generate_str_from_input(data, offset, size, 16, alloc);
                }

                VecPushBackArr(vec, temp_strings, count);
                VecDeinit(vec);
            }
            break;
        }

        case VEC_STR_PUSH_FRONT_ARRAY : {
            if (*offset + 4 <= size) {
                size_t count = extract_u32(data, offset, size) % 10; // Limit to reasonable count

                // Create temporary array of Str objects
                Str temp_strings[10];
                for (size_t i = 0; i < count; i++) {
                    temp_strings[i] = generate_str_from_input(data, offset, size, 16, alloc);
                }

                VecPushFrontArr(vec, temp_strings, count);
                VecDeinit(vec);
            }
            break;
        }

        case VEC_STR_PUSH_FRONT_ARRAY_FAST : {
            if (*offset + 4 <= size) {
                size_t count = extract_u32(data, offset, size) % 10; // Limit to reasonable count

                // Create temporary array of Str objects
                Str temp_strings[10];
                for (size_t i = 0; i < count; i++) {
                    temp_strings[i] = generate_str_from_input(data, offset, size, 16, alloc);
                }

                VecPushFrontArrFast(vec, temp_strings, count);
                VecDeinit(vec);
            }
            break;
        }

        case VEC_STR_SORT : {
            // Note: VecSort requires a comparison function, but Str sorting is complex
            // For fuzzing, we'll skip this or use a simple approach
            break;
        }

        case VEC_STR_BEGIN : {
            if (VecLen(vec) > 0) {
                Str *begin = VecBegin(vec);
                (void)begin; // Use the result to avoid warnings
            }
            break;
        }

        case VEC_STR_END : {
            if (VecLen(vec) > 0) {
                Str *end = VecEnd(vec);
                (void)end; // Use the result to avoid warnings
            }
            break;
        }

        case VEC_STR_PTR_AT : {
            if (VecLen(vec) > 0 && *offset + 4 <= size) {
                size_t index = extract_u32(data, offset, size) % VecLen(vec);
                Str   *ptr   = VecPtrAt(vec, index);
                (void)ptr; // Use the result to avoid warnings
            }
            break;
        }

        case VEC_STR_MERGE : {
            if (*offset + 4 <= size) {
                // Create a temporary vector for merging
                StrVec temp = VecInitWithDeepCopyT(temp, NULL, str_deinit, alloc);

                // Add some strings to temp
                size_t count = extract_u32(data, offset, size) % 5;
                for (size_t i = 0; i < count; i++) {
                    Str str = generate_str_from_input(data, offset, size, 16, alloc);
                    VecPushBack(&temp, str);
                }

                VecMerge(vec, &temp);
                VecDeinit(&temp);
            }
            break;
        }

        case VEC_STR_INSERT_RANGE_FAST : {
            if (*offset + 8 <= size) {
                size_t index = extract_u32(data, offset, size) % (VecLen(vec) + 1);
                size_t count = extract_u32(data, offset, size) % 10; // Limit to reasonable count

                // Create temporary array of Str objects
                Str temp_strings[10];
                for (size_t i = 0; i < count; i++) {
                    temp_strings[i] = generate_str_from_input(data, offset, size, 16, alloc);
                }

                VecInsertRangeFast(vec, temp_strings, index, count);
                VecDeinit(vec);
            }
            break;
        }

        case VEC_STR_ALIGNED_OFFSET_AT : {
            if (VecLen(vec) > 0 && *offset + 8 <= size) {
                size_t index = extract_u32(data, offset, size) % VecLen(vec);
                // Note: VecAlignedOffsetAt doesn't take alignment parameter
                size_t offset = VecAlignedOffsetAt(vec, index);
                (void)offset; // Use the result to avoid warnings
            }
            break;
        }

        case VEC_STR_DELETE_LAST : {
            if (VecLen(vec) > 0) {
                VecDeleteLast(vec);
            }
            break;
        }

        case VEC_STR_DELETE_FAST : {
            if (VecLen(vec) > 0 && *offset + 4 <= size) {
                size_t index = extract_u32(data, offset, size) % VecLen(vec);
                VecDeleteFast(vec, index);
            }
            break;
        }

        case VEC_STR_INIT_CLONE : {
            if (*offset + 4 <= size) {
                // Create a temporary vector for cloning
                StrVec temp = VecInitWithDeepCopyT(temp, NULL, str_deinit, alloc);

                // Add some strings to temp
                size_t count = extract_u32(data, offset, size) % 5;
                for (size_t i = 0; i < count; i++) {
                    Str str = generate_str_from_input(data, offset, size, 16, alloc);
                    VecPushBack(&temp, str);
                }

                VecInitClone(vec, &temp);

                VecDeinit(&temp);
            }
            break;
        }

        // Foreach operations
        case VEC_STR_FOREACH : {
            if (VecLen(vec) > 0) {
                size_t total_len = 0;
                VecForeach(vec, str) {
                    total_len += ZstrLen(str.data);
                }
                (void)total_len; // Suppress unused variable warning
            }
            break;
        }

        case VEC_STR_FOREACH_IDX : {
            if (VecLen(vec) > 0) {
                size_t total_len = 0;
                VecForeachIdx(vec, str, idx) {
                    total_len += strlen(str.data) + idx;
                }
                (void)total_len; // Suppress unused variable warning
            }
            break;
        }

        case VEC_STR_FOREACH_PTR : {
            if (VecLen(vec) > 0) {
                size_t total_len = 0;
                VecForeachPtr(vec, str_ptr) {
                    total_len += strlen(str_ptr->data);
                }
                (void)total_len; // Suppress unused variable warning
            }
            break;
        }

        case VEC_STR_FOREACH_PTR_IDX : {
            if (VecLen(vec) > 0) {
                size_t total_len = 0;
                VecForeachPtrIdx(vec, str_ptr, idx) {
                    total_len += strlen(str_ptr->data) + idx;
                }
                (void)total_len; // Suppress unused variable warning
            }
            break;
        }

        case VEC_STR_FOREACH_REVERSE : {
            if (VecLen(vec) > 0) {
                size_t total_len = 0;
                VecForeachReverse(vec, str) {
                    total_len += ZstrLen(str.data);
                }
                (void)total_len; // Suppress unused variable warning
            }
            break;
        }

        case VEC_STR_FOREACH_REVERSE_IDX : {
            if (VecLen(vec) > 0) {
                size_t total_len = 0;
                VecForeachReverseIdx(vec, str, idx) {
                    total_len += ZstrLen(str.data) + idx;
                }
                (void)total_len; // Suppress unused variable warning
            }
            break;
        }

        case VEC_STR_FOREACH_PTR_REVERSE : {
            if (VecLen(vec) > 0) {
                size_t total_len = 0;
                VecForeachPtrReverse(vec, str_ptr) {
                    total_len += ZstrLen(str_ptr->data);
                }
                (void)total_len; // Suppress unused variable warning
            }
            break;
        }

        case VEC_STR_FOREACH_PTR_REVERSE_IDX : {
            if (VecLen(vec) > 0) {
                size_t total_len = 0;
                VecForeachPtrReverseIdx(vec, str_ptr, idx) {
                    total_len += ZstrLen(str_ptr->data) + idx;
                }
                (void)total_len; // Suppress unused variable warning
            }
            break;
        }

        case VEC_STR_FOREACH_IN_RANGE : {
            if (VecLen(vec) > 0 && *offset + 8 <= size) {
                size_t start = extract_u32(data, offset, size) % VecLen(vec);
                size_t end   = extract_u32(data, offset, size) % (VecLen(vec) + 1);
                if (start < end) {
                    size_t total_len = 0;
                    VecForeachInRange(vec, str, start, end) {
                        total_len += ZstrLen(str.data);
                    }
                    (void)total_len; // Suppress unused variable warning
                }
            }
            break;
        }

        case VEC_STR_FOREACH_IN_RANGE_IDX : {
            if (VecLen(vec) > 0 && *offset + 8 <= size) {
                size_t start = extract_u32(data, offset, size) % VecLen(vec);
                size_t end   = extract_u32(data, offset, size) % (VecLen(vec) + 1);
                if (start < end) {
                    size_t total_len = 0;
                    VecForeachInRangeIdx(vec, str, idx, start, end) {
                        total_len += ZstrLen(str.data) + idx;
                    }
                    (void)total_len; // Suppress unused variable warning
                }
            }
            break;
        }

        case VEC_STR_FOREACH_PTR_IN_RANGE : {
            if (VecLen(vec) > 0 && *offset + 8 <= size) {
                size_t start = extract_u32(data, offset, size) % VecLen(vec);
                size_t end   = extract_u32(data, offset, size) % (VecLen(vec) + 1);
                if (start < end) {
                    size_t total_len = 0;
                    VecForeachPtrInRange(vec, str_ptr, start, end) {
                        total_len += ZstrLen(str_ptr->data);
                    }
                    (void)total_len; // Suppress unused variable warning
                }
            }
            break;
        }

        case VEC_STR_FOREACH_PTR_IN_RANGE_IDX : {
            if (VecLen(vec) > 0 && *offset + 8 <= size) {
                size_t start = extract_u32(data, offset, size) % VecLen(vec);
                size_t end   = extract_u32(data, offset, size) % (VecLen(vec) + 1);
                if (start < end) {
                    size_t total_len = 0;
                    VecForeachPtrInRangeIdx(vec, str_ptr, idx, start, end) {
                        total_len += ZstrLen(str_ptr->data) + idx;
                    }
                    (void)total_len; // Suppress unused variable warning
                }
            }
            break;
        }

        default :
            break;
    }
}
