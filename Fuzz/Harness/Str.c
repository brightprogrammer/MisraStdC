/// file      : fuzz/harness/str.c
/// author    : Generated for MisraStdC fuzzing
/// This is free and unencumbered software released into the public domain.
///
/// Str specific fuzzing implementation

#include "../Harness.h"
#include "Str.h"
#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Log.h>
#include <string.h>
#include <stdarg.h>

// Note: generate_cstring is already defined in Harness.h

// Generate a Str from fuzz input data
static Str
    generate_str_from_input(const uint8_t *data, size_t *offset, size_t size, size_t max_len, DefaultAllocator *alloc) {
    // Extract length (limit to max_len for sanity)
    uint8_t len = extract_u8(data, offset, size);
    len         = len % (max_len + 1); // 0 to max_len

    // Create Str from input data
    Str str  = StrInitFromCstr((const char *)(data + *offset), len, alloc);
    *offset += len;

    return str;
}

void init_str(Str *str, DefaultAllocator *alloc) {
    *str = StrInit(alloc);
}

void deinit_str(Str *str) {
    StrDeinit(str);
}

void fuzz_str(Str *str, StrFunction func, const uint8_t *data, size_t *offset, size_t size, DefaultAllocator *alloc) {
    switch (func) {
        case STR_INIT : {
            // Reinitialize the string
            StrDeinit(str);
            *str = StrInit(alloc);
            break;
        }

        case STR_INIT_FROM_CSTR : {
            if (*offset + 1 <= size) {
                char *cstr = generate_cstring(data, offset, size, 50);
                if (cstr) {
                    StrDeinit(str);
                    *str = StrInitFromCstr(cstr, strlen(cstr), alloc);
                    free(cstr);
                }
            }
            break;
        }

        case STR_INIT_FROM_ZSTR : {
            if (*offset + 1 <= size) {
                char *zstr = generate_cstring(data, offset, size, 50);
                if (zstr) {
                    StrDeinit(str);
                    *str = StrInitFromZstr(zstr, alloc);
                    free(zstr);
                }
            }
            break;
        }

        case STR_INIT_FROM_STR : {
            if (VecLen(str) > 0) {
                Str temp = StrInitFromStr(str, alloc);
                StrDeinit(str);
                *str = temp;
            }
            break;
        }

        case STR_DUP : {
            if (VecLen(str) > 0) {
                Str temp = StrDup(str, alloc);
                StrDeinit(str);
                *str = temp;
            }
            break;
        }

        case STR_DEINIT : {
            StrDeinit(str);
            *str = StrInit(alloc); // Reinitialize for continued fuzzing
            break;
        }

        // Access operations
        case STR_FIRST : {
            if (VecLen(str) > 0) {
                char first = StrFirst(str);
                (void)first; // Suppress unused variable warning
            }
            break;
        }

        case STR_LAST : {
            if (VecLen(str) > 0) {
                char last = StrLast(str);
                (void)last; // Suppress unused variable warning
            }
            break;
        }

        case STR_BEGIN : {
            char *begin = StrBegin(str);
            (void)begin; // Suppress unused variable warning
            break;
        }

        case STR_END : {
            char *end = StrEnd(str);
            (void)end; // Suppress unused variable warning
            break;
        }

        case STR_CHAR_AT : {
            if (VecLen(str) > 0 && *offset + 2 <= size) {
                size_t idx = extract_u16(data, offset, size) % VecLen(str);
                char   ch  = StrCharAt(str, idx);
                (void)ch; // Suppress unused variable warning
            }
            break;
        }

        case STR_CHAR_PTR_AT : {
            if (VecLen(str) > 0 && *offset + 2 <= size) {
                size_t idx = extract_u16(data, offset, size) % VecLen(str);
                char  *ptr = StrCharPtrAt(str, idx);
                (void)ptr; // Suppress unused variable warning
            }
            break;
        }

        // Memory operations
        case STR_CLEAR : {
            StrClear(str);
            break;
        }

        case STR_RESIZE : {
            if (*offset + 2 <= size) {
                size_t new_size = extract_u16(data, offset, size) % 1000;
                StrResize(str, new_size);
            }
            break;
        }

        case STR_RESERVE : {
            if (*offset + 2 <= size) {
                size_t capacity = extract_u16(data, offset, size) % 1000;
                StrReserve(str, capacity);
            }
            break;
        }

        case STR_TRY_REDUCE_SPACE : {
            StrTryReduceSpace(str);
            break;
        }

        case STR_LEN : {
            size_t len = VecLen(str);
            (void)len; // Suppress unused variable warning
            break;
        }

        case STR_SIZE : {
            size_t size_bytes = VecSize(str);
            (void)size_bytes; // Suppress unused variable warning
            break;
        }

        // String operations
        case STR_CMP : {
            if (VecLen(str) > 0) {
                Str temp   = generate_str_from_input(data, offset, size, 20, alloc);
                int result = StrCmp(str, &temp);
                (void)result; // Suppress unused variable warning
                StrDeinit(&temp);
            }
            break;
        }

        case STR_CMP_CSTR : {
            if (VecLen(str) > 0 && *offset + 1 <= size) {
                char *cstr = generate_cstring(data, offset, size, 20);
                if (cstr) {
                    int result = StrCmpCstr(str, cstr, strlen(cstr));
                    (void)result; // Suppress unused variable warning
                    free(cstr);
                }
            }
            break;
        }

        case STR_CMP_ZSTR : {
            if (VecLen(str) > 0 && *offset + 1 <= size) {
                char *zstr = generate_cstring(data, offset, size, 20);
                if (zstr) {
                    int result = StrCmpZstr(str, zstr);
                    (void)result; // Suppress unused variable warning
                    free(zstr);
                }
            }
            break;
        }

        case STR_FIND_STR : {
            if (VecLen(str) > 0) {
                Str   temp  = generate_str_from_input(data, offset, size, 10, alloc);
                char *found = StrFindStr(str, &temp);
                (void)found; // Suppress unused variable warning
                StrDeinit(&temp);
            }
            break;
        }

        case STR_FIND_ZSTR : {
            if (VecLen(str) > 0 && *offset + 1 <= size) {
                char *zstr = generate_cstring(data, offset, size, 10);
                if (zstr) {
                    char *found = StrFindZstr(str, zstr);
                    (void)found; // Suppress unused variable warning
                    free(zstr);
                }
            }
            break;
        }

        case STR_FIND_CSTR : {
            if (VecLen(str) > 0 && *offset + 1 <= size) {
                char *cstr = generate_cstring(data, offset, size, 10);
                if (cstr) {
                    char *found = StrFindCstr(str, cstr, strlen(cstr));
                    (void)found; // Suppress unused variable warning
                    free(cstr);
                }
            }
            break;
        }

        // Insert operations
        case STR_INSERT : {
            if (*offset + 3 <= size) {
                size_t idx = extract_u16(data, offset, size) % (VecLen(str) + 1);
                char   ch  = (char)extract_u8(data, offset, size);
                StrInsertCharAt(str, ch, idx);
            }
            break;
        }

        case STR_INSERT_CSTR : {
            if (*offset + 3 <= size) {
                size_t idx  = extract_u16(data, offset, size) % (VecLen(str) + 1);
                char  *cstr = generate_cstring(data, offset, size, 20);
                if (cstr) {
                    StrInsertCstr(str, cstr, idx, strlen(cstr));
                    free(cstr);
                }
            }
            break;
        }

        case STR_INSERT_ZSTR : {
            if (*offset + 3 <= size) {
                size_t idx  = extract_u16(data, offset, size) % (VecLen(str) + 1);
                char  *zstr = generate_cstring(data, offset, size, 20);
                if (zstr) {
                    StrInsertZstr(str, zstr, idx);
                    free(zstr);
                }
            }
            break;
        }

        case STR_INSERT_STR : {
            if (*offset + 2 <= size) {
                size_t idx  = extract_u16(data, offset, size) % (VecLen(str) + 1);
                Str    temp = generate_str_from_input(data, offset, size, 20, alloc);
                StrInsert(str, &temp, idx);
                StrDeinit(&temp);
            }
            break;
        }

        case STR_PUSH_BACK : {
            if (*offset + 1 <= size) {
                char ch = (char)extract_u8(data, offset, size);
                StrPushBack(str, ch);
            }
            break;
        }

        case STR_PUSH_FRONT : {
            if (*offset + 1 <= size) {
                char ch = (char)extract_u8(data, offset, size);
                StrPushFront(str, ch);
            }
            break;
        }

        case STR_APPEND : {
            if (*offset + 1 <= size) {
                char ch = (char)extract_u8(data, offset, size);
                StrPushBack(str, ch);
            }
            break;
        }

        case STR_APPEND_CSTR : {
            if (*offset + 1 <= size) {
                char *cstr = generate_cstring(data, offset, size, 20);
                if (cstr) {
                    StrPushBackCstr(str, cstr, strlen(cstr));
                    free(cstr);
                }
            }
            break;
        }

        case STR_APPEND_ZSTR : {
            if (*offset + 1 <= size) {
                char *zstr = generate_cstring(data, offset, size, 20);
                if (zstr) {
                    StrPushBackZstr(str, zstr);
                    free(zstr);
                }
            }
            break;
        }

        case STR_APPEND_STR : {
            Str temp = generate_str_from_input(data, offset, size, 20, alloc);
            StrMerge(str, &temp);
            StrDeinit(&temp);
            break;
        }

        // Remove operations
        case STR_POP_BACK : {
            if (VecLen(str) > 0) {
                char ch;
                StrPopBack(str, &ch);
                (void)ch; // Suppress unused variable warning
            }
            break;
        }

        case STR_POP_FRONT : {
            if (VecLen(str) > 0) {
                char ch;
                StrPopFront(str, &ch);
                (void)ch; // Suppress unused variable warning
            }
            break;
        }

        case STR_REMOVE : {
            if (VecLen(str) > 0 && *offset + 2 <= size) {
                size_t idx = extract_u16(data, offset, size) % VecLen(str);
                char   ch;
                StrRemove(str, &ch, idx);
                (void)ch; // Suppress unused variable warning
            }
            break;
        }

        case STR_REMOVE_RANGE : {
            if (VecLen(str) > 0 && *offset + 4 <= size) {
                size_t start = extract_u16(data, offset, size) % VecLen(str);
                size_t count = extract_u16(data, offset, size) % (VecLen(str) - start + 1);
                StrRemoveRange(str, (char *)NULL, start, count);
            }
            break;
        }

        case STR_DELETE : {
            if (VecLen(str) > 0 && *offset + 2 <= size) {
                size_t idx = extract_u16(data, offset, size) % VecLen(str);
                StrDelete(str, idx);
            }
            break;
        }

        case STR_DELETE_RANGE : {
            if (VecLen(str) > 0 && *offset + 4 <= size) {
                size_t start = extract_u16(data, offset, size) % VecLen(str);
                size_t count = extract_u16(data, offset, size) % (VecLen(str) - start + 1);
                StrDeleteRange(str, start, count);
            }
            break;
        }

        // Advanced operations
        case STR_REVERSE : {
            StrReverse(str);
            break;
        }

        case STR_SWAP_ITEMS : {
            if (VecLen(str) > 1 && *offset + 4 <= size) {
                size_t idx1 = extract_u16(data, offset, size) % VecLen(str);
                size_t idx2 = extract_u16(data, offset, size) % VecLen(str);
                StrSwapCharAt(str, idx1, idx2);
            }
            break;
        }

        case STR_SORT : {
            VecSort(str, NULL); // Use default comparison
            break;
        }

        case STR_MERGE : {
            Str temp = generate_str_from_input(data, offset, size, 20, alloc);
            StrMerge(str, &temp);
            StrDeinit(&temp);
            break;
        }

        case STR_MERGE_CSTR : {
            if (*offset + 1 <= size) {
                char *cstr = generate_cstring(data, offset, size, 20);
                if (cstr) {
                    StrPushBackCstr(str, cstr, strlen(cstr));
                    free(cstr);
                }
            }
            break;
        }

        case STR_MERGE_ZSTR : {
            if (*offset + 1 <= size) {
                char *zstr = generate_cstring(data, offset, size, 20);
                if (zstr) {
                    StrPushBackZstr(str, zstr);
                    free(zstr);
                }
            }
            break;
        }

        case STR_MERGE_STR : {
            Str temp = generate_str_from_input(data, offset, size, 20, alloc);
            StrMerge(str, &temp);
            StrDeinit(&temp);
            break;
        }

        // Foreach operations
        case STR_FOREACH : {
            if (VecLen(str) > 0) {
                size_t total_len = 0;
                StrForeach(str, ch) {
                    total_len += 1;
                }
                (void)total_len; // Suppress unused variable warning
            }
            break;
        }

        case STR_FOREACH_IDX : {
            if (VecLen(str) > 0) {
                size_t total_len = 0;
                StrForeachIdx(str, ch, idx) {
                    total_len += 1 + idx;
                }
                (void)total_len; // Suppress unused variable warning
            }
            break;
        }

        case STR_FOREACH_PTR : {
            if (VecLen(str) > 0) {
                size_t total_len = 0;
                StrForeachPtr(str, ch_ptr) {
                    total_len += 1;
                }
                (void)total_len; // Suppress unused variable warning
            }
            break;
        }

        case STR_FOREACH_PTR_IDX : {
            if (VecLen(str) > 0) {
                size_t total_len = 0;
                StrForeachPtrIdx(str, ch_ptr, idx) {
                    total_len += 1 + idx;
                }
                (void)total_len; // Suppress unused variable warning
            }
            break;
        }

        case STR_FOREACH_REVERSE : {
            if (VecLen(str) > 0) {
                size_t total_len = 0;
                StrForeachReverse(str, ch) {
                    total_len += 1;
                }
                (void)total_len; // Suppress unused variable warning
            }
            break;
        }

        case STR_FOREACH_REVERSE_IDX : {
            if (VecLen(str) > 0) {
                size_t total_len = 0;
                StrForeachReverseIdx(str, ch, idx) {
                    total_len += 1 + idx;
                }
                (void)total_len; // Suppress unused variable warning
            }
            break;
        }

        case STR_FOREACH_PTR_REVERSE : {
            if (VecLen(str) > 0) {
                size_t total_len = 0;
                StrForeachPtrReverse(str, ch_ptr) {
                    total_len += 1;
                }
                (void)total_len; // Suppress unused variable warning
            }
            break;
        }

        case STR_FOREACH_PTR_REVERSE_IDX : {
            if (VecLen(str) > 0) {
                size_t total_len = 0;
                StrForeachReversePtrIdx(str, ch_ptr, idx) {
                    total_len += 1 + idx;
                }
                (void)total_len; // Suppress unused variable warning
            }
            break;
        }

        case STR_FOREACH_IN_RANGE : {
            if (VecLen(str) > 0 && *offset + 4 <= size) {
                size_t start = extract_u16(data, offset, size) % VecLen(str);
                size_t end   = extract_u16(data, offset, size) % (VecLen(str) + 1);
                if (start < end) {
                    size_t total_len = 0;
                    StrForeachInRange(str, ch, start, end) {
                        total_len += 1;
                    }
                    (void)total_len; // Suppress unused variable warning
                }
            }
            break;
        }

        case STR_FOREACH_IN_RANGE_IDX : {
            if (VecLen(str) > 0 && *offset + 4 <= size) {
                size_t start = extract_u16(data, offset, size) % VecLen(str);
                size_t end   = extract_u16(data, offset, size) % (VecLen(str) + 1);
                if (start < end) {
                    size_t total_len = 0;
                    StrForeachInRangeIdx(str, ch, idx, start, end) {
                        total_len += 1 + idx;
                    }
                    (void)total_len; // Suppress unused variable warning
                }
            }
            break;
        }

        case STR_FOREACH_PTR_IN_RANGE : {
            if (VecLen(str) > 0 && *offset + 4 <= size) {
                size_t start = extract_u16(data, offset, size) % VecLen(str);
                size_t end   = extract_u16(data, offset, size) % (VecLen(str) + 1);
                if (start < end) {
                    size_t total_len = 0;
                    StrForeachPtrInRange(str, ch_ptr, start, end) {
                        total_len += 1;
                    }
                    (void)total_len; // Suppress unused variable warning
                }
            }
            break;
        }

        case STR_FOREACH_PTR_IN_RANGE_IDX : {
            if (VecLen(str) > 0 && *offset + 4 <= size) {
                size_t start = extract_u16(data, offset, size) % VecLen(str);
                size_t end   = extract_u16(data, offset, size) % (VecLen(str) + 1);
                if (start < end) {
                    size_t total_len = 0;
                    StrForeachPtrInRangeIdx(str, ch_ptr, idx, start, end) {
                        total_len += 1 + idx;
                    }
                    (void)total_len; // Suppress unused variable warning
                }
            }
            break;
        }

        // Utility operations
        case STR_PRINTF : {
            if (*offset + 4 <= size) {
                int value = (int)extract_u32(data, offset, size);
                StrPrintf(str, "Value: %d", value);
            }
            break;
        }

        case STR_VALIDATE : {
            ValidateStr(str);
            break;
        }

        default :
            break;
    }
}
