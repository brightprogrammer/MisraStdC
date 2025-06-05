/// file      : std/container/bitvec.c
/// author    : Generated following Misra project patterns
/// This is free and unencumbered software released into the public domain.
///
/// Bit vector implementation - efficient storage for boolean values

#include <Misra/Std/Container/BitVec.h>
#include <Misra/Std/Memory.h>
#include <Misra/Std/Log.h>
#include <string.h>

// Helper macros for bit operations
#define BITS_PER_BYTE        8
#define BIT_INDEX(idx)       ((idx) / BITS_PER_BYTE)
#define BIT_OFFSET(idx)      ((idx) % BITS_PER_BYTE)
#define BYTES_FOR_BITS(bits) (((bits) + BITS_PER_BYTE - 1) / BITS_PER_BYTE)

void BitVecInitWithData(BitVec *bitvec, const u8 *data, size byte_len, size bit_len) {
    *bitvec = BitVecInit();
    ValidateBitVec(bitvec);

    if (!data || byte_len == 0 || bit_len == 0)
        return;

    BitVecReserve(bitvec, bit_len);
    bitvec->length = bit_len;

    size copy_bytes = byte_len < bitvec->byte_size ? byte_len : bitvec->byte_size;
    memcpy(bitvec->data, data, copy_bytes);
}

void BitVecDeinit(BitVec *bitvec) {
    ValidateBitVec(bitvec);
    if (bitvec->data) {
        free(bitvec->data);
        bitvec->data = NULL;
    }
    bitvec->length    = 0;
    bitvec->capacity  = 0;
    bitvec->byte_size = 0;
}

void BitVecClear(BitVec *bitvec) {
    ValidateBitVec(bitvec);
    bitvec->length = 0;
    if (bitvec->data && bitvec->byte_size > 0) {
        memset(bitvec->data, 0, bitvec->byte_size);
    }
}

void BitVecResize(BitVec *bitvec, size new_size) {
    ValidateBitVec(bitvec);
    if (new_size > bitvec->capacity) {
        BitVecReserve(bitvec, new_size);
    }

    // If growing, clear new bits
    if (new_size > bitvec->length && bitvec->data) {
        size old_bytes = BYTES_FOR_BITS(bitvec->length);
        size new_bytes = BYTES_FOR_BITS(new_size);

        if (new_bytes > old_bytes) {
            memset(bitvec->data + old_bytes, 0, new_bytes - old_bytes);
        }

        // Clear any partial bits in the last byte of old length
        if (bitvec->length > 0) {
            size last_bit_offset = BIT_OFFSET(bitvec->length);
            if (last_bit_offset != 0) {
                size last_byte_idx           = BIT_INDEX(bitvec->length);
                u8   mask                    = (1u << last_bit_offset) - 1u;
                bitvec->data[last_byte_idx] &= mask;
            }
        }
    }

    bitvec->length = new_size;
}

void BitVecReserve(BitVec *bitvec, size n) {
    ValidateBitVec(bitvec);
    if (n <= bitvec->capacity)
        return;
    size new_byte_size = BYTES_FOR_BITS(n);
    u8  *new_data      = (u8 *)realloc(bitvec->data, new_byte_size);

    if (!new_data) {
        LOG_FATAL("Failed to allocate memory for bitvec");
        return;
    }

    // Clear new bytes
    if (new_byte_size > bitvec->byte_size) {
        memset(new_data + bitvec->byte_size, 0, new_byte_size - bitvec->byte_size);
    }

    bitvec->data      = new_data;
    bitvec->capacity  = n;
    bitvec->byte_size = new_byte_size;
}

bool BitVecGet(BitVec *bitvec, size idx) {
    ValidateBitVec(bitvec);
    if (idx >= bitvec->length) {
        return false;
    }
    size byte_idx   = BIT_INDEX(idx);
    size bit_offset = BIT_OFFSET(idx);

    return (bitvec->data[byte_idx] & (1u << bit_offset)) != 0;
}

void BitVecSet(BitVec *bitvec, size idx, bool value) {
    ValidateBitVec(bitvec);
    if (idx >= bitvec->length) {
        return;
    }
    size byte_idx   = BIT_INDEX(idx);
    size bit_offset = BIT_OFFSET(idx);

    if (value) {
        bitvec->data[byte_idx] |= (1u << bit_offset);
    } else {
        bitvec->data[byte_idx] &= ~(1u << bit_offset);
    }
}

void BitVecFlip(BitVec *bitvec, size idx) {
    ValidateBitVec(bitvec);
    if (idx >= bitvec->length) {
        return;
    }
    size byte_idx   = BIT_INDEX(idx);
    size bit_offset = BIT_OFFSET(idx);

    bitvec->data[byte_idx] ^= (1u << bit_offset);
}

void BitVecPush(BitVec *bitvec, bool value) {
    ValidateBitVec(bitvec);
    if (bitvec->length >= bitvec->capacity) {
        size new_capacity = bitvec->capacity == 0 ? 8 : bitvec->capacity * 2;
        BitVecReserve(bitvec, new_capacity);
    }

    BitVecResize(bitvec, bitvec->length + 1);
    BitVecSet(bitvec, bitvec->length - 1, value);
}

bool BitVecPop(BitVec *bitvec) {
    ValidateBitVec(bitvec);
    if (bitvec->length == 0) {
        return false;
    }
    bool value = BitVecGet(bitvec, bitvec->length - 1);
    BitVecResize(bitvec, bitvec->length - 1);
    return value;
}

void BitVecInsert(BitVec *bitvec, size idx, bool value) {
    ValidateBitVec(bitvec);
    if (idx > bitvec->length)
        return;
    // For now, implement as push + manual bit shifting (simple but not efficient)
    BitVecPush(bitvec, false);

    // Shift bits right from idx to end
    for (size i = bitvec->length - 1; i > idx; i--) {
        bool bit = BitVecGet(bitvec, i - 1);
        BitVecSet(bitvec, i, bit);
    }

    BitVecSet(bitvec, idx, value);
}

void BitVecRemove(BitVec *bitvec, size idx) {
    ValidateBitVec(bitvec);
    if (idx >= bitvec->length)
        return;
    // Shift bits left from idx+1 to end
    for (size i = idx; i < bitvec->length - 1; i++) {
        bool bit = BitVecGet(bitvec, i + 1);
        BitVecSet(bitvec, i, bit);
    }

    BitVecResize(bitvec, bitvec->length - 1);
}

size BitVecCountOnes(BitVec *bitvec) {
    ValidateBitVec(bitvec);
    if (!bitvec->data)
        return 0;
    size count = 0;
    for (size i = 0; i < bitvec->length; i++) {
        if (BitVecGet(bitvec, i)) {
            count++;
        }
    }
    return count;
}

size BitVecCountZeros(BitVec *bitvec) {
    ValidateBitVec(bitvec);
    return bitvec->length - BitVecCountOnes(bitvec);
}

void BitVecAnd(BitVec *result, BitVec *a, BitVec *b) {
    ValidateBitVec(result);
    ValidateBitVec(a);
    ValidateBitVec(b);

    size min_len = a->length < b->length ? a->length : b->length;
    BitVecResize(result, min_len);

    for (size i = 0; i < min_len; i++) {
        bool bit_a = BitVecGet(a, i);
        bool bit_b = BitVecGet(b, i);
        BitVecSet(result, i, bit_a && bit_b);
    }
}

void BitVecOr(BitVec *result, BitVec *a, BitVec *b) {
    ValidateBitVec(result);
    ValidateBitVec(a);
    ValidateBitVec(b);

    size max_len = a->length > b->length ? a->length : b->length;
    BitVecResize(result, max_len);

    for (size i = 0; i < max_len; i++) {
        bool bit_a = i < a->length ? BitVecGet(a, i) : false;
        bool bit_b = i < b->length ? BitVecGet(b, i) : false;
        BitVecSet(result, i, bit_a || bit_b);
    }
}

void BitVecXor(BitVec *result, BitVec *a, BitVec *b) {
    ValidateBitVec(result);
    ValidateBitVec(a);
    ValidateBitVec(b);

    size max_len = a->length > b->length ? a->length : b->length;
    BitVecResize(result, max_len);

    for (size i = 0; i < max_len; i++) {
        bool bit_a = i < a->length ? BitVecGet(a, i) : false;
        bool bit_b = i < b->length ? BitVecGet(b, i) : false;
        BitVecSet(result, i, bit_a != bit_b);
    }
}

void BitVecNot(BitVec *result, BitVec *bitvec) {
    ValidateBitVec(result);
    ValidateBitVec(bitvec);

    BitVecResize(result, bitvec->length);

    for (size i = 0; i < bitvec->length; i++) {
        bool bit = BitVecGet(bitvec, i);
        BitVecSet(result, i, !bit);
    }
}