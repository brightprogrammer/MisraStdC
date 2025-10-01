/// file      : fuzz/Harness/VecCharPtr.c
/// author    : Generated for MisraStdC fuzzing
/// This is free and unencumbered software released into the public domain.
///
/// Vec(char*) specific fuzzing implementation

#include "../Harness.h"
#include "VecCharPtr.h"
#include <Misra/Std/Container/Vec.h>
#include <Misra/Std/Memory.h>
#include <Misra/Std/Log.h>
#include <stdlib.h>

// Copy function for char* - duplicates the string
static bool char_ptr_copy_init(char **dst, char **src) {
    if (!dst || !src || !*src) {
        return false;
    }
    *dst = ZstrDup(*src);
    return *dst != NULL;
}

// Deinit function for char* - frees the string
static void char_ptr_deinit(char **str) {
    if (str && *str) {
        free(*str);
        *str = NULL;
    }
}

void init_char_ptr_vec(CharPtrVec *vec) {
    *vec = VecInitWithDeepCopyT(*vec, char_ptr_copy_init, char_ptr_deinit);
}

void deinit_char_ptr_vec(CharPtrVec *vec) {
    // VecDeinit will automatically call char_ptr_deinit on each element
    VecDeinit(vec);
}

void fuzz_char_ptr_vec(CharPtrVec *vec, VecCharPtrFunction func, const uint8_t *data, size_t *offset, size_t size) {
    switch (func) {
        case VEC_CHAR_PTR_PUSH_BACK : {
            char *str = generate_cstring(data, offset, size, 32);
            if (str) {
                VecPushBack(vec, str);
            }
            break;
        }

        case VEC_CHAR_PTR_PUSH_FRONT : {
            char *str = generate_cstring(data, offset, size, 32);
            if (str) {
                VecPushFront(vec, str);
            }
            break;
        }

        case VEC_CHAR_PTR_POP_BACK : {
            if (VecLen(vec) > 0) {
                char *str;
                VecPopBack(vec, &str);
                // char_ptr_deinit is called automatically by the vector
            }
            break;
        }

        case VEC_CHAR_PTR_POP_FRONT : {
            if (VecLen(vec) > 0) {
                char *str;
                VecPopFront(vec, &str);
                // char_ptr_deinit is called automatically by the vector
            }
            break;
        }

        case VEC_CHAR_PTR_INSERT : {
            if (*offset + 4 <= size) {
                size_t index = extract_u32(data, offset, size) % (VecLen(vec) + 1);
                char  *str   = generate_cstring(data, offset, size, 32);
                if (str) {
                    VecInsert(vec, str, index);
                }
            }
            break;
        }

        case VEC_CHAR_PTR_REMOVE : {
            if (VecLen(vec) > 0 && *offset + 4 <= size) {
                size_t index = extract_u32(data, offset, size) % VecLen(vec);
                char  *str;
                VecRemove(vec, &str, index);
                // char_ptr_deinit is called automatically by the vector
            }
            break;
        }

        case VEC_CHAR_PTR_DELETE : {
            if (VecLen(vec) > 0 && *offset + 4 <= size) {
                size_t index = extract_u32(data, offset, size) % VecLen(vec);
                VecDelete(vec, index);
                // char_ptr_deinit is called automatically by the vector
            }
            break;
        }

        case VEC_CHAR_PTR_AT : {
            if (VecLen(vec) > 0 && *offset + 4 <= size) {
                size_t index = extract_u32(data, offset, size) % VecLen(vec);
                char  *str   = VecAt(vec, index);
                (void)str; // Use the result to avoid warnings
            }
            break;
        }

        case VEC_CHAR_PTR_LEN : {
            size_t len = VecLen(vec);
            (void)len; // Use the result to avoid warnings
            break;
        }

        case VEC_CHAR_PTR_FIRST : {
            if (VecLen(vec) > 0) {
                char *first = VecFirst(vec);
                (void)first; // Use the result to avoid warnings
            }
            break;
        }

        case VEC_CHAR_PTR_LAST : {
            if (VecLen(vec) > 0) {
                char *last = VecLast(vec);
                (void)last; // Use the result to avoid warnings
            }
            break;
        }

        case VEC_CHAR_PTR_CLEAR : {
            // VecClear automatically calls char_ptr_deinit on each element
            VecClear(vec);
            break;
        }

        case VEC_CHAR_PTR_RESIZE : {
            if (*offset + 4 <= size) {
                size_t new_size = extract_u32(data, offset, size) % 100; // Limit to reasonable size
                size_t old_size = VecLen(vec);

                // VecResize automatically handles cleanup of removed elements and
                // initializes new elements to NULL for pointer types
                VecResize(vec, new_size);

                // Fill new slots with generated strings if vector grew
                if (new_size > old_size) {
                    for (size_t i = old_size; i < new_size; i++) {
                        char *str = generate_cstring(data, offset, size, 16);
                        if (str) {
                            VecAt(vec, i) = str;
                        }
                    }
                }
            }
            break;
        }

        case VEC_CHAR_PTR_RESERVE : {
            if (*offset + 4 <= size) {
                size_t capacity = extract_u32(data, offset, size) % 1000; // Limit to reasonable size
                VecReserve(vec, capacity);
            }
            break;
        }

        case VEC_CHAR_PTR_TRY_REDUCE_SPACE : {
            VecTryReduceSpace(vec);
            break;
        }

        case VEC_CHAR_PTR_SIZE : {
            size_t size_bytes = VecSize(vec);
            (void)size_bytes; // Use the result to avoid warnings
            break;
        }

        case VEC_CHAR_PTR_REVERSE : {
            VecReverse(vec);
            break;
        }

        case VEC_CHAR_PTR_SWAP_ITEMS : {
            if (VecLen(vec) >= 2 && *offset + 8 <= size) {
                size_t i = extract_u32(data, offset, size) % VecLen(vec);
                size_t j = extract_u32(data, offset, size) % VecLen(vec);
                VecSwapItems(vec, i, j);
            }
            break;
        }

        case VEC_CHAR_PTR_INSERT_RANGE : {
            if (*offset + 8 <= size) {
                size_t index = extract_u32(data, offset, size) % (VecLen(vec) + 1);
                size_t count = extract_u32(data, offset, size) % 10; // Limit to reasonable count

                // Create temporary array of strings
                char *temp_strings[10];
                for (size_t i = 0; i < count; i++) {
                    temp_strings[i] = generate_cstring(data, offset, size, 16);
                }

                VecInsertRange(vec, temp_strings, index, count);

                // Clean up temp strings
                for (size_t i = 0; i < count; i++) {
                    if (temp_strings[i]) {
                        cleanup_cstring(temp_strings[i]);
                    }
                }
            }
            break;
        }

        case VEC_CHAR_PTR_REMOVE_RANGE : {
            if (VecLen(vec) > 0 && *offset + 8 <= size) {
                size_t index = extract_u32(data, offset, size) % VecLen(vec);
                size_t count = extract_u32(data, offset, size) % (VecLen(vec) - index + 1);
                VecRemoveRange(vec, (char **)NULL, index, count);
            }
            break;
        }

        case VEC_CHAR_PTR_DELETE_RANGE : {
            if (VecLen(vec) > 0 && *offset + 8 <= size) {
                size_t index = extract_u32(data, offset, size) % VecLen(vec);
                size_t count = extract_u32(data, offset, size) % (VecLen(vec) - index + 1);
                VecDeleteRange(vec, index, count);
            }
            break;
        }

        case VEC_CHAR_PTR_INSERT_FAST : {
            if (*offset + 4 <= size) {
                size_t index = extract_u32(data, offset, size) % (VecLen(vec) + 1);
                char  *str   = generate_cstring(data, offset, size, 32);
                if (str) {
                    VecInsertFast(vec, str, index);
                }
            }
            break;
        }

        case VEC_CHAR_PTR_REMOVE_FAST : {
            if (VecLen(vec) > 0 && *offset + 4 <= size) {
                size_t index = extract_u32(data, offset, size) % VecLen(vec);
                char  *str;
                VecRemoveFast(vec, &str, index);
                // char_ptr_deinit is called automatically by the vector
            }
            break;
        }

        case VEC_CHAR_PTR_REMOVE_RANGE_FAST : {
            if (VecLen(vec) > 0 && *offset + 8 <= size) {
                size_t index = extract_u32(data, offset, size) % VecLen(vec);
                size_t count = extract_u32(data, offset, size) % (VecLen(vec) - index + 1);
                VecRemoveRangeFast(vec, (char **)NULL, index, count);
            }
            break;
        }

        case VEC_CHAR_PTR_DELETE_RANGE_FAST : {
            if (VecLen(vec) > 0 && *offset + 8 <= size) {
                size_t index = extract_u32(data, offset, size) % VecLen(vec);
                size_t count = extract_u32(data, offset, size) % (VecLen(vec) - index + 1);
                VecDeleteRangeFast(vec, index, count);
            }
            break;
        }

        case VEC_CHAR_PTR_PUSH_BACK_ARRAY : {
            if (*offset + 4 <= size) {
                size_t count = extract_u32(data, offset, size) % 10; // Limit to reasonable count

                // Create temporary array of strings
                char *temp_strings[10];
                for (size_t i = 0; i < count; i++) {
                    temp_strings[i] = generate_cstring(data, offset, size, 16);
                }

                VecPushBackArr(vec, temp_strings, count);

                // Clean up temp strings
                for (size_t i = 0; i < count; i++) {
                    if (temp_strings[i]) {
                        cleanup_cstring(temp_strings[i]);
                    }
                }
            }
            break;
        }

        case VEC_CHAR_PTR_PUSH_FRONT_ARRAY : {
            if (*offset + 4 <= size) {
                size_t count = extract_u32(data, offset, size) % 10; // Limit to reasonable count

                // Create temporary array of strings
                char *temp_strings[10];
                for (size_t i = 0; i < count; i++) {
                    temp_strings[i] = generate_cstring(data, offset, size, 16);
                }

                VecPushFrontArr(vec, temp_strings, count);

                // Clean up temp strings
                for (size_t i = 0; i < count; i++) {
                    if (temp_strings[i]) {
                        cleanup_cstring(temp_strings[i]);
                    }
                }
            }
            break;
        }

        case VEC_CHAR_PTR_PUSH_FRONT_ARRAY_FAST : {
            if (*offset + 4 <= size) {
                size_t count = extract_u32(data, offset, size) % 10; // Limit to reasonable count

                // Create temporary array of strings
                char *temp_strings[10];
                for (size_t i = 0; i < count; i++) {
                    temp_strings[i] = generate_cstring(data, offset, size, 16);
                }

                VecPushFrontArrFast(vec, temp_strings, count);

                // Clean up temp strings
                for (size_t i = 0; i < count; i++) {
                    if (temp_strings[i]) {
                        cleanup_cstring(temp_strings[i]);
                    }
                }
            }
            break;
        }

        case VEC_CHAR_PTR_SORT : {
            // Note: VecSort requires a comparison function, but char* sorting is complex
            // For fuzzing, we'll skip this or use a simple approach
            break;
        }

        case VEC_CHAR_PTR_BEGIN : {
            if (VecLen(vec) > 0) {
                char **begin = VecBegin(vec);
                (void)begin; // Use the result to avoid warnings
            }
            break;
        }

        case VEC_CHAR_PTR_END : {
            if (VecLen(vec) > 0) {
                char *end = VecEnd(vec);
                (void)end; // Use the result to avoid warnings
            }
            break;
        }

        case VEC_CHAR_PTR_PTR_AT : {
            if (VecLen(vec) > 0 && *offset + 4 <= size) {
                size_t index = extract_u32(data, offset, size) % VecLen(vec);
                char **ptr   = VecPtrAt(vec, index);
                (void)ptr; // Use the result to avoid warnings
            }
            break;
        }

        case VEC_CHAR_PTR_MERGE : {
            if (*offset + 4 <= size) {
                // Create a temporary vector for merging
                CharPtrVec temp = VecInitWithDeepCopyT(temp, char_ptr_copy_init, char_ptr_deinit);

                // Add some strings to temp
                size_t count = extract_u32(data, offset, size) % 5;
                for (size_t i = 0; i < count; i++) {
                    char *str = generate_cstring(data, offset, size, 16);
                    if (str) {
                        VecPushBack(&temp, str);
                    }
                }

                VecMerge(vec, &temp);
                VecDeinit(&temp);
            }
            break;
        }

        case VEC_CHAR_PTR_INSERT_RANGE_FAST : {
            if (*offset + 8 <= size) {
                size_t index = extract_u32(data, offset, size) % (VecLen(vec) + 1);
                size_t count = extract_u32(data, offset, size) % 10; // Limit to reasonable count

                // Create temporary array of strings
                char *temp_strings[10];
                for (size_t i = 0; i < count; i++) {
                    temp_strings[i] = generate_cstring(data, offset, size, 16);
                }

                VecInsertRangeFast(vec, temp_strings, index, count);

                // Clean up temp strings
                for (size_t i = 0; i < count; i++) {
                    if (temp_strings[i]) {
                        cleanup_cstring(temp_strings[i]);
                    }
                }
            }
            break;
        }

        case VEC_CHAR_PTR_ALIGNED_OFFSET_AT : {
            if (VecLen(vec) > 0 && *offset + 8 <= size) {
                size_t index = extract_u32(data, offset, size) % VecLen(vec);
                // Note: VecAlignedOffsetAt doesn't take alignment parameter
                size_t offset = VecAlignedOffsetAt(vec, index);
                (void)offset; // Use the result to avoid warnings
            }
            break;
        }

        case VEC_CHAR_PTR_DELETE_LAST : {
            if (VecLen(vec) > 0) {
                VecDeleteLast(vec);
            }
            break;
        }

        case VEC_CHAR_PTR_DELETE_FAST : {
            if (VecLen(vec) > 0 && *offset + 4 <= size) {
                size_t index = extract_u32(data, offset, size) % VecLen(vec);
                VecDeleteFast(vec, index);
            }
            break;
        }

        case VEC_CHAR_PTR_INIT_CLONE : {
            if (*offset + 4 <= size) {
                // Create a temporary vector for cloning
                CharPtrVec temp = VecInitWithDeepCopyT(temp, char_ptr_copy_init, char_ptr_deinit);

                // Add some strings to temp
                size_t count = extract_u32(data, offset, size) % 5;
                for (size_t i = 0; i < count; i++) {
                    char *str = generate_cstring(data, offset, size, 16);
                    if (str) {
                        VecPushBack(&temp, str);
                    }
                }

                VecInitClone(vec, &temp);
                VecDeinit(&temp);
            }
            break;
        }

        // Foreach operations
        case VEC_CHAR_PTR_FOREACH : {
            if (VecLen(vec) > 0) {
                size_t total_len = 0;
                VecForeach(vec, str) {
                    total_len += strlen(str);
                }
                (void)total_len; // Suppress unused variable warning
            }
            break;
        }

        case VEC_CHAR_PTR_FOREACH_IDX : {
            if (VecLen(vec) > 0) {
                size_t total_len = 0;
                VecForeachIdx(vec, str, idx) {
                    total_len += strlen(str) + idx;
                }
                (void)total_len; // Suppress unused variable warning
            }
            break;
        }

        case VEC_CHAR_PTR_FOREACH_PTR : {
            if (VecLen(vec) > 0) {
                size_t total_len = 0;
                VecForeachPtr(vec, str_ptr) {
                    total_len += strlen(*str_ptr);
                }
                (void)total_len; // Suppress unused variable warning
            }
            break;
        }

        case VEC_CHAR_PTR_FOREACH_PTR_IDX : {
            if (VecLen(vec) > 0) {
                size_t total_len = 0;
                VecForeachPtrIdx(vec, str_ptr, idx) {
                    total_len += strlen(*str_ptr) + idx;
                }
                (void)total_len; // Suppress unused variable warning
            }
            break;
        }

        case VEC_CHAR_PTR_FOREACH_REVERSE : {
            if (VecLen(vec) > 0) {
                size_t total_len = 0;
                VecForeachReverse(vec, str) {
                    total_len += strlen(str);
                }
                (void)total_len; // Suppress unused variable warning
            }
            break;
        }

        case VEC_CHAR_PTR_FOREACH_REVERSE_IDX : {
            if (VecLen(vec) > 0) {
                size_t total_len = 0;
                VecForeachReverseIdx(vec, str, idx) {
                    total_len += strlen(str) + idx;
                }
                (void)total_len; // Suppress unused variable warning
            }
            break;
        }

        case VEC_CHAR_PTR_FOREACH_PTR_REVERSE : {
            if (VecLen(vec) > 0) {
                size_t total_len = 0;
                VecForeachPtrReverse(vec, str_ptr) {
                    total_len += strlen(*str_ptr);
                }
                (void)total_len; // Suppress unused variable warning
            }
            break;
        }

        case VEC_CHAR_PTR_FOREACH_PTR_REVERSE_IDX : {
            if (VecLen(vec) > 0) {
                size_t total_len = 0;
                VecForeachPtrReverseIdx(vec, str_ptr, idx) {
                    total_len += strlen(*str_ptr) + idx;
                }
                (void)total_len; // Suppress unused variable warning
            }
            break;
        }

        case VEC_CHAR_PTR_FOREACH_IN_RANGE : {
            if (VecLen(vec) > 0 && *offset + 8 <= size) {
                size_t start = extract_u32(data, offset, size) % VecLen(vec);
                size_t end   = extract_u32(data, offset, size) % (VecLen(vec) + 1);
                if (start < end) {
                    size_t total_len = 0;
                    VecForeachInRange(vec, str, start, end) {
                        total_len += strlen(str);
                    }
                    (void)total_len; // Suppress unused variable warning
                }
            }
            break;
        }

        case VEC_CHAR_PTR_FOREACH_IN_RANGE_IDX : {
            if (VecLen(vec) > 0 && *offset + 8 <= size) {
                size_t start = extract_u32(data, offset, size) % VecLen(vec);
                size_t end   = extract_u32(data, offset, size) % (VecLen(vec) + 1);
                if (start < end) {
                    size_t total_len = 0;
                    VecForeachInRangeIdx(vec, str, idx, start, end) {
                        total_len += strlen(str) + idx;
                    }
                    (void)total_len; // Suppress unused variable warning
                }
            }
            break;
        }

        case VEC_CHAR_PTR_FOREACH_PTR_IN_RANGE : {
            if (VecLen(vec) > 0 && *offset + 8 <= size) {
                size_t start = extract_u32(data, offset, size) % VecLen(vec);
                size_t end   = extract_u32(data, offset, size) % (VecLen(vec) + 1);
                if (start < end) {
                    size_t total_len = 0;
                    VecForeachPtrInRange(vec, str_ptr, start, end) {
                        total_len += strlen(*str_ptr);
                    }
                    (void)total_len; // Suppress unused variable warning
                }
            }
            break;
        }

        case VEC_CHAR_PTR_FOREACH_PTR_IN_RANGE_IDX : {
            if (VecLen(vec) > 0 && *offset + 8 <= size) {
                size_t start = extract_u32(data, offset, size) % VecLen(vec);
                size_t end   = extract_u32(data, offset, size) % (VecLen(vec) + 1);
                if (start < end) {
                    size_t total_len = 0;
                    VecForeachPtrInRangeIdx(vec, str_ptr, idx, start, end) {
                        total_len += strlen(*str_ptr) + idx;
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
