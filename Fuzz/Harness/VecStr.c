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

// Deep-copy init for Str slots. The main StrVec is configured for
// move semantics (`copy_init = NULL`), so `VecPushBack` zeros the
// caller's source and `VecMerge` transfers ownership. `VecInitClone`,
// though, walks element-by-element and falls back to `MemCopy` when
// no copy_init is set -- producing two slots that share one heap
// buffer and double-free on cleanup. The INIT_CLONE test case below
// configures the source vec with this callback so the clone actually
// deep-copies; the matching `copy_deinit` is `str_deinit`. Match the
// `GenericCopyInit` signature `(void *dst, const void *src, const Allocator *)`.
static bool str_clone_init(void *dst_ptr, const void *src_ptr, const Allocator *alloc) {
    const Str *src = (const Str *)src_ptr;
    // StrBegin on an empty Str returns NULL, which str_try_init_from_cstr
    // treats as a fatal arg violation. An empty source clones to an
    // empty fresh Str.
    Str copy = (StrLen(src) == 0) ? StrInit((Allocator *)alloc) :
                                    StrInitFromCstr(StrBegin(src), StrLen(src), (Allocator *)alloc);
    *(Str *)dst_ptr = copy;
    return true;
}

// Generate a Str from fuzz input data
static Str generate_str_from_input(
    const uint8_t    *data,
    size_t           *offset,
    size_t            data_size,
    size_t            max_len,
    DefaultAllocator *alloc
) {
    // Extract length (limit to max_len for sanity)
    uint8_t len = extract_u8(data, offset, data_size);
    len         = len % (max_len + 1); // 0 to max_len

    // Create Str with capacity
    Str str = StrInit(alloc);

    // Fill with data or generate simple pattern if not enough input
    for (size_t i = 0; i < len; i++) {
        char ch;
        if (*offset < data_size) {
            ch = (char)(data[(*offset)++] % 95 + 32); // Printable ASCII range
        } else {
            ch = (char)('A' + (i % 26));              // Fallback pattern
        }
        StrPushBack(&str, ch);
    }

    return str;
}

void init_str_vec(StrVec *vec, DefaultAllocator *alloc) {
    // NULL copy_init + str_deinit copy_deinit = move semantics for the
    // L-form push/insert paths (the source slot is memset to 0 after a
    // successful push, so caller-side Strs don't double-free). The
    // INIT_CLONE test case below switches to str_clone_init for its
    // temporary, because clone walks the source element-by-element and
    // a NULL copy_init would memcpy the Str header and alias the
    // backing buffer.
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
    size_t            data_size,
    DefaultAllocator *alloc
) {
    switch (func) {
        case VEC_STR_PUSH_BACK : {
            Str str = generate_str_from_input(data, offset, data_size, 32, alloc);
            VecPushBack(vec, str);
            break;
        }

        case VEC_STR_PUSH_FRONT : {
            Str str = generate_str_from_input(data, offset, data_size, 32, alloc);
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
            if (*offset + 4 <= data_size) {
                size_t index = extract_u32(data, offset, data_size) % (VecLen(vec) + 1);
                Str    str   = generate_str_from_input(data, offset, data_size, 32, alloc);
                VecInsert(vec, str, index);
            }
            break;
        }

        case VEC_STR_REMOVE : {
            if (VecLen(vec) > 0 && *offset + 4 <= data_size) {
                size_t index = extract_u32(data, offset, data_size) % VecLen(vec);
                Str    str;
                VecRemove(vec, &str, index);
                // StrDeinit is called automatically by the vector
            }
            break;
        }

        case VEC_STR_DELETE : {
            if (VecLen(vec) > 0 && *offset + 4 <= data_size) {
                size_t index = extract_u32(data, offset, data_size) % VecLen(vec);
                VecDelete(vec, index);
                // StrDeinit is called automatically by the vector
            }
            break;
        }

        case VEC_STR_AT : {
            if (VecLen(vec) > 0 && *offset + 4 <= data_size) {
                size_t index = extract_u32(data, offset, data_size) % VecLen(vec);
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
            if (*offset + 4 <= data_size) {
                size_t new_size = extract_u32(data, offset, data_size) % 100; // Limit to reasonable size
                size_t old_size = VecLen(vec);

                // VecResize automatically handles cleanup of removed elements and
                // initializes new elements to default (empty Str)
                VecResize(vec, new_size);

                // Initialize new Str objects if vector grew
                if (new_size > old_size) {
                    for (size_t i = old_size; i < new_size; i++) {
                        Str str       = generate_str_from_input(data, offset, data_size, 16, alloc);
                        VecAt(vec, i) = str;
                    }
                }
            }
            break;
        }

        case VEC_STR_RESERVE : {
            if (*offset + 4 <= data_size) {
                size_t capacity = extract_u32(data, offset, data_size) % 1000; // Limit to reasonable size
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
            if (VecLen(vec) >= 2 && *offset + 8 <= data_size) {
                size_t i = extract_u32(data, offset, data_size) % VecLen(vec);
                size_t j = extract_u32(data, offset, data_size) % VecLen(vec);
                VecSwapItems(vec, i, j);
            }
            break;
        }

        case VEC_STR_INSERT_RANGE : {
            if (*offset + 8 <= data_size) {
                size_t index = extract_u32(data, offset, data_size) % (VecLen(vec) + 1);
                size_t count = extract_u32(data, offset, data_size) % 10; // Limit to reasonable count

                // Create temporary array of Str objects. L-form VecInsertRange
                // moves each Str into the vector and zeros the source slot,
                // so no per-element StrDeinit is needed here.
                Str temp_strings[10];
                for (size_t i = 0; i < count; i++) {
                    temp_strings[i] = generate_str_from_input(data, offset, data_size, 16, alloc);
                }

                VecInsertRange(vec, temp_strings, index, count);
            }
            break;
        }

        case VEC_STR_REMOVE_RANGE : {
            if (VecLen(vec) > 0 && *offset + 8 <= data_size) {
                size_t index = extract_u32(data, offset, data_size) % VecLen(vec);
                size_t count = extract_u32(data, offset, data_size) % (VecLen(vec) - index + 1);
                VecRemoveRange(vec, (Str *)NULL, index, count);
            }
            break;
        }

        case VEC_STR_DELETE_RANGE : {
            if (VecLen(vec) > 0 && *offset + 8 <= data_size) {
                size_t index = extract_u32(data, offset, data_size) % VecLen(vec);
                size_t count = extract_u32(data, offset, data_size) % (VecLen(vec) - index + 1);
                VecDeleteRange(vec, index, count);
            }
            break;
        }

        case VEC_STR_INSERT_FAST : {
            if (*offset + 4 <= data_size) {
                size_t index = extract_u32(data, offset, data_size) % (VecLen(vec) + 1);
                Str    str   = generate_str_from_input(data, offset, data_size, 32, alloc);
                VecInsertFast(vec, str, index);
            }
            break;
        }

        case VEC_STR_REMOVE_FAST : {
            if (VecLen(vec) > 0 && *offset + 4 <= data_size) {
                size_t index = extract_u32(data, offset, data_size) % VecLen(vec);
                Str    str;
                VecRemoveFast(vec, &str, index);
                // StrDeinit is called automatically by the vector
            }
            break;
        }

        case VEC_STR_REMOVE_RANGE_FAST : {
            if (VecLen(vec) > 0 && *offset + 8 <= data_size) {
                size_t index = extract_u32(data, offset, data_size) % VecLen(vec);
                size_t count = extract_u32(data, offset, data_size) % (VecLen(vec) - index + 1);
                VecRemoveRangeFast(vec, (Str *)NULL, index, count);
            }
            break;
        }

        case VEC_STR_DELETE_RANGE_FAST : {
            if (VecLen(vec) > 0 && *offset + 8 <= data_size) {
                size_t index = extract_u32(data, offset, data_size) % VecLen(vec);
                size_t count = extract_u32(data, offset, data_size) % (VecLen(vec) - index + 1);
                VecDeleteRangeFast(vec, index, count);
            }
            break;
        }

        case VEC_STR_PUSH_BACK_ARRAY : {
            if (*offset + 4 <= data_size) {
                size_t count = extract_u32(data, offset, data_size) % 10; // Limit to reasonable count

                // L-form VecPushBackArr moves each Str into the vector and
                // zeros the source slot, so no per-element cleanup needed.
                Str temp_strings[10];
                for (size_t i = 0; i < count; i++) {
                    temp_strings[i] = generate_str_from_input(data, offset, data_size, 16, alloc);
                }

                VecPushBackArr(vec, temp_strings, count);
            }
            break;
        }

        case VEC_STR_PUSH_FRONT_ARRAY : {
            if (*offset + 4 <= data_size) {
                size_t count = extract_u32(data, offset, data_size) % 10; // Limit to reasonable count

                // L-form VecPushFrontArr moves each Str into the vector and
                // zeros the source slot, so no per-element cleanup needed.
                Str temp_strings[10];
                for (size_t i = 0; i < count; i++) {
                    temp_strings[i] = generate_str_from_input(data, offset, data_size, 16, alloc);
                }

                VecPushFrontArr(vec, temp_strings, count);
            }
            break;
        }

        case VEC_STR_PUSH_FRONT_ARRAY_FAST : {
            if (*offset + 4 <= data_size) {
                size_t count = extract_u32(data, offset, data_size) % 10; // Limit to reasonable count

                // L-form VecPushFrontArrFast moves each Str into the vector
                // and zeros the source slot, so no per-element cleanup needed.
                Str temp_strings[10];
                for (size_t i = 0; i < count; i++) {
                    temp_strings[i] = generate_str_from_input(data, offset, data_size, 16, alloc);
                }

                VecPushFrontArrFast(vec, temp_strings, count);
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
            if (VecLen(vec) > 0 && *offset + 4 <= data_size) {
                size_t index = extract_u32(data, offset, data_size) % VecLen(vec);
                Str   *ptr   = VecPtrAt(vec, index);
                (void)ptr; // Use the result to avoid warnings
            }
            break;
        }

        case VEC_STR_MERGE : {
            if (*offset + 4 <= data_size) {
                // Create a temporary vector for merging
                StrVec temp = VecInitWithDeepCopyT(temp, NULL, str_deinit, alloc);

                // Add some strings to temp
                size_t count = extract_u32(data, offset, data_size) % 5;
                for (size_t i = 0; i < count; i++) {
                    Str str = generate_str_from_input(data, offset, data_size, 16, alloc);
                    VecPushBack(&temp, str);
                }

                VecMerge(vec, &temp);
                VecDeinit(&temp);
            }
            break;
        }

        case VEC_STR_INSERT_RANGE_FAST : {
            if (*offset + 8 <= data_size) {
                size_t index = extract_u32(data, offset, data_size) % (VecLen(vec) + 1);
                size_t count = extract_u32(data, offset, data_size) % 10; // Limit to reasonable count

                // L-form VecInsertRangeFast moves each Str into the vector and
                // zeros the source slot, so no per-element cleanup needed.
                Str temp_strings[10];
                for (size_t i = 0; i < count; i++) {
                    temp_strings[i] = generate_str_from_input(data, offset, data_size, 16, alloc);
                }

                VecInsertRangeFast(vec, temp_strings, index, count);
            }
            break;
        }

        case VEC_STR_ALIGNED_OFFSET_AT : {
            if (VecLen(vec) > 0 && *offset + 8 <= data_size) {
                size_t index   = extract_u32(data, offset, data_size) % VecLen(vec);
                size_t aligned = VecAlignedOffsetAt(vec, index);
                (void)aligned;
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
            if (VecLen(vec) > 0 && *offset + 4 <= data_size) {
                size_t index = extract_u32(data, offset, data_size) % VecLen(vec);
                VecDeleteFast(vec, index);
            }
            break;
        }

        case VEC_STR_INIT_CLONE : {
            if (*offset + 4 <= data_size) {
                // Build the source with str_clone_init so VecInitClone
                // deep-copies each element; without a copy_init, clone
                // memcpy's the Str header and the two slots alias one
                // heap buffer -> double-free on cleanup. Pushing into a
                // deep-copy vec leaves the caller's Str alive, so we
                // StrDeinit each source after the push.
                StrVec temp = VecInitWithDeepCopyT(temp, str_clone_init, str_deinit, alloc);

                size_t count = extract_u32(data, offset, data_size) % 5;
                for (size_t i = 0; i < count; i++) {
                    Str str = generate_str_from_input(data, offset, data_size, 16, alloc);
                    VecPushBack(&temp, str);
                    StrDeinit(&str);
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
                    total_len += StrLen(&str);
                }
                (void)total_len; // Suppress unused variable warning
            }
            break;
        }

        case VEC_STR_FOREACH_IDX : {
            if (VecLen(vec) > 0) {
                size_t total_len = 0;
                VecForeachIdx(vec, str, idx) {
                    total_len += StrLen(&str) + idx;
                }
                (void)total_len; // Suppress unused variable warning
            }
            break;
        }

        case VEC_STR_FOREACH_PTR : {
            if (VecLen(vec) > 0) {
                size_t total_len = 0;
                VecForeachPtr(vec, str_ptr) {
                    total_len += StrLen(str_ptr);
                }
                (void)total_len; // Suppress unused variable warning
            }
            break;
        }

        case VEC_STR_FOREACH_PTR_IDX : {
            if (VecLen(vec) > 0) {
                size_t total_len = 0;
                VecForeachPtrIdx(vec, str_ptr, idx) {
                    total_len += StrLen(str_ptr) + idx;
                }
                (void)total_len; // Suppress unused variable warning
            }
            break;
        }

        case VEC_STR_FOREACH_REVERSE : {
            if (VecLen(vec) > 0) {
                size_t total_len = 0;
                VecForeachReverse(vec, str) {
                    total_len += StrLen(&str);
                }
                (void)total_len; // Suppress unused variable warning
            }
            break;
        }

        case VEC_STR_FOREACH_REVERSE_IDX : {
            if (VecLen(vec) > 0) {
                size_t total_len = 0;
                VecForeachReverseIdx(vec, str, idx) {
                    total_len += StrLen(&str) + idx;
                }
                (void)total_len; // Suppress unused variable warning
            }
            break;
        }

        case VEC_STR_FOREACH_PTR_REVERSE : {
            if (VecLen(vec) > 0) {
                size_t total_len = 0;
                VecForeachPtrReverse(vec, str_ptr) {
                    total_len += StrLen(str_ptr);
                }
                (void)total_len; // Suppress unused variable warning
            }
            break;
        }

        case VEC_STR_FOREACH_PTR_REVERSE_IDX : {
            if (VecLen(vec) > 0) {
                size_t total_len = 0;
                VecForeachPtrReverseIdx(vec, str_ptr, idx) {
                    total_len += StrLen(str_ptr) + idx;
                }
                (void)total_len; // Suppress unused variable warning
            }
            break;
        }

        case VEC_STR_FOREACH_IN_RANGE : {
            if (VecLen(vec) > 0 && *offset + 8 <= data_size) {
                size_t start = extract_u32(data, offset, data_size) % VecLen(vec);
                size_t end   = extract_u32(data, offset, data_size) % (VecLen(vec) + 1);
                if (start < end) {
                    size_t total_len = 0;
                    VecForeachInRange(vec, str, start, end) {
                        total_len += StrLen(&str);
                    }
                    (void)total_len; // Suppress unused variable warning
                }
            }
            break;
        }

        case VEC_STR_FOREACH_IN_RANGE_IDX : {
            if (VecLen(vec) > 0 && *offset + 8 <= data_size) {
                size_t start = extract_u32(data, offset, data_size) % VecLen(vec);
                size_t end   = extract_u32(data, offset, data_size) % (VecLen(vec) + 1);
                if (start < end) {
                    size_t total_len = 0;
                    VecForeachInRangeIdx(vec, str, idx, start, end) {
                        total_len += StrLen(&str) + idx;
                    }
                    (void)total_len; // Suppress unused variable warning
                }
            }
            break;
        }

        case VEC_STR_FOREACH_PTR_IN_RANGE : {
            if (VecLen(vec) > 0 && *offset + 8 <= data_size) {
                size_t start = extract_u32(data, offset, data_size) % VecLen(vec);
                size_t end   = extract_u32(data, offset, data_size) % (VecLen(vec) + 1);
                if (start < end) {
                    size_t total_len = 0;
                    VecForeachPtrInRange(vec, str_ptr, start, end) {
                        total_len += StrLen(str_ptr);
                    }
                    (void)total_len; // Suppress unused variable warning
                }
            }
            break;
        }

        case VEC_STR_FOREACH_PTR_IN_RANGE_IDX : {
            if (VecLen(vec) > 0 && *offset + 8 <= data_size) {
                size_t start = extract_u32(data, offset, data_size) % VecLen(vec);
                size_t end   = extract_u32(data, offset, data_size) % (VecLen(vec) + 1);
                if (start < end) {
                    size_t total_len = 0;
                    VecForeachPtrInRangeIdx(vec, str_ptr, idx, start, end) {
                        total_len += StrLen(str_ptr) + idx;
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
