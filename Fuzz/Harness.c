/// file      : fuzz/Harness.c
/// author    : Generated for MisraStdC fuzzing
/// This is free and unencumbered software released into the public domain.
///
/// Main fuzzing harness - coordinates sub-harnesses for different Vec types

#include "Harness.h"
#include "Harness/VecInt.h"
#include "Harness/VecCharPtr.h"
#include "Harness/VecStr.h"
#include "Harness/ListInt.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

// Object type enumeration (first 2 bytes)
typedef enum {
    OBJ_INT_VEC = 0,
    OBJ_CHAR_PTR_VEC,
    OBJ_STR_VEC,
    OBJ_INT_LIST,
    OBJ_COUNT
} ObjectType;

// Main fuzzing state containing all container objects directly
typedef struct {
    DefaultAllocator alloc;
    IntVec           int_vec;
    CharPtrVec       char_ptr_vec;
    StrVec           str_vec;
    IntList          int_list;
    bool             initialized;
} FuzzState;

// Helper function to extract uint16 from input data
uint16_t extract_u16(const uint8_t *data, size_t *offset, size_t size) {
    if (*offset + 2 > size) {
        return 0;
    }
    uint16_t result  = (data[*offset] << 8) | data[*offset + 1];
    *offset         += 2;
    return result;
}

// Helper function to extract uint8 from input data
uint8_t extract_u8(const uint8_t *data, size_t *offset, size_t size) {
    if (*offset >= size) {
        return 0;
    }
    return data[(*offset)++];
}

// Helper function to extract uint32 from input data
uint32_t extract_u32(const uint8_t *data, size_t *offset, size_t size) {
    if (*offset + 4 > size) {
        return 0;
    }
    uint32_t result  = (data[*offset] << 24) | (data[*offset + 1] << 16) | (data[*offset + 2] << 8) | data[*offset + 3];
    *offset         += 4;
    return result;
}

// Generate a C-string from fuzz input data
char *generate_cstring(const uint8_t *data, size_t *offset, size_t size, size_t max_len) {
    // Extract length (limit to max_len for sanity)
    uint8_t len = extract_u8(data, offset, size);
    len         = len % (max_len + 1); // 0 to max_len

    // Allocate string
    char *str = malloc(len + 1);
    if (!str) {
        return NULL;
    }

    // Fill with data or generate simple pattern if not enough input
    for (size_t i = 0; i < len; i++) {
        if (*offset < size) {
            str[i] = (char)(data[(*offset)++] % 95 + 32); // Printable ASCII range
        } else {
            str[i] = (char)('A' + (i % 26));              // Fallback pattern
        }
    }
    str[len] = '\0';

    return str;
}

// Clean up a generated C-string
void cleanup_cstring(char *str) {
    free(str);
}

// Initialize all container objects in FuzzState
static void init_fuzz_state(FuzzState *state) {
    if (state->initialized) {
        return;
    }

    state->alloc = DefaultAllocatorInit();

    init_int_vec(&state->int_vec, &state->alloc);
    init_char_ptr_vec(&state->char_ptr_vec, &state->alloc);
    init_str_vec(&state->str_vec, &state->alloc);
    init_int_list(&state->int_list, &state->alloc);

    state->initialized = true;
}

// Deinitialize all container objects in FuzzState
static void deinit_fuzz_state(FuzzState *state) {
    if (!state->initialized) {
        return;
    }

    deinit_int_vec(&state->int_vec);
    deinit_char_ptr_vec(&state->char_ptr_vec);
    deinit_str_vec(&state->str_vec);
    deinit_int_list(&state->int_list);

    DefaultAllocatorDeinit(&state->alloc);

    state->initialized = false;
}

// Main fuzzing entry point
// Return values:
//   0 = Successful execution (input was processed)
//   1 = Input format error (not a bug in the library)
//   Crash/Abort = Actual bug found in library
static int process_fuzz_input(const uint8_t *data, size_t size) {
    // Need at least 4 bytes (2 for object selection, 2 for function selection)
    if (size < 4) {
        return 1; // Input format error, not a library bug
    }

    // Initialize state locally
    FuzzState state = {0};
    init_fuzz_state(&state);

    size_t offset = 0;

    // Extract object type (first 2 bytes)
    uint16_t   obj_selector = extract_u16(data, &offset, size);
    ObjectType obj_type     = (ObjectType)(obj_selector % OBJ_COUNT);

    // Extract function selector (next 2 bytes)
    uint16_t func_selector = extract_u16(data, &offset, size);

    // Route to appropriate sub-harness based on object type
    switch (obj_type) {
        case OBJ_INT_VEC : {
            VecIntFunction func = (VecIntFunction)(func_selector % VEC_INT_COUNT);
            fuzz_int_vec(&state.int_vec, func, data, &offset, size, &state.alloc);
            break;
        }

        case OBJ_CHAR_PTR_VEC : {
            VecCharPtrFunction func = (VecCharPtrFunction)(func_selector % VEC_CHAR_PTR_COUNT);
            fuzz_char_ptr_vec(&state.char_ptr_vec, func, data, &offset, size, &state.alloc);
            break;
        }

        case OBJ_STR_VEC : {
            VecStrFunction func = (VecStrFunction)(func_selector % VEC_STR_COUNT);
            fuzz_str_vec(&state.str_vec, func, data, &offset, size, &state.alloc);
            break;
        }

        case OBJ_INT_LIST : {
            ListIntFunction func = (ListIntFunction)(func_selector % LIST_INT_COUNT);
            fuzz_int_list(&state.int_list, func, data, &offset, size, &state.alloc);
            break;
        }

        default :
            break;
    }

    // Clean up state
    deinit_fuzz_state(&state);

    return 0;
}

// Entry point for AFL++ and other fuzzers
int main(int argc, char *argv[]) {
    uint8_t buffer[65536];
    size_t  total_read = 0;
    bool    close_file = false;
    FILE   *input      = stdin;

    // If a filename is provided, read from file instead of stdin
    if (argc > 1) {
        input = fopen(argv[1], "rb");
        if (!input) {
            fprintf(stderr, "Failed to open file: %s\n", argv[1]);
            return 1;
        }
        close_file = true;
    }

    // Read input data
    size_t bytes_read;
    while ((bytes_read = fread(buffer + total_read, 1, sizeof(buffer) - total_read, input)) > 0) {
        total_read += bytes_read;
        if (total_read >= sizeof(buffer)) {
            break;
        }
    }

    if (close_file) {
        fclose(input);
    }

    // Process the fuzzing input
    int result = process_fuzz_input(buffer, total_read);

    return result;
}
