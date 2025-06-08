/// file      : std/container/Bits.c
/// author    : Generated following Misra project patterns
/// This is free and unencumbered software released into the public domain.
///
/// Bit vector implementation - efficient storage for boolean values

#include <Misra/Std/Container/Bits.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Memory.h>
#include <Misra/Std/Log.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "Misra/Std/Container/Bits/Type.h"

// Helper macros for bit operations
#define BITS_PER_BYTE        8
#define BIT_INDEX(idx)       ((idx) / BITS_PER_BYTE)
#define BIT_OFFSET(idx)      ((idx) % BITS_PER_BYTE)
#define BYTES_FOR_BITS(bits) (((bits) + BITS_PER_BYTE - 1) / BITS_PER_BYTE)

void BitsDeinit(Bits *bv) {
    ValidateBits(bv);
    if (bv->data) {
        free(bv->data);
        bv->data = NULL;
    }
    bv->length    = 0;
    bv->capacity  = 0;
    bv->byte_size = 0;
}

void BitsClear(Bits *bv) {
    ValidateBits(bv);
    bv->length = 0;
    if (bv->data && bv->byte_size > 0) {
        MemSet(bv->data, 0, bv->byte_size);
    }
}

void BitsResize(Bits *bv, u64 new_size) {
    ValidateBits(bv);

    if (new_size > bv->capacity) {
        BitsReserve(bv, new_size);
    }

    bv->length = new_size;
}

void BitsReserve(Bits *bv, u64 new_cap) {
    ValidateBits(bv);

    if (new_cap <= bv->capacity) {
        return;
    }

    u64 new_byte_size = BYTES_FOR_BITS(new_cap);
    u8 *new_data      = (u8 *)realloc(bv->data, new_byte_size);

    if (!new_data) {
        LOG_FATAL("Failed to allocate memory for Bits");
    }

    // If growing, clear new bits
    if (new_cap > bv->length) {
        u64 old_byte_size = BYTES_FOR_BITS(bv->length);

        if (new_byte_size > old_byte_size && bv->data) {
            MemSet(new_data + old_byte_size, 0, new_byte_size - old_byte_size);
        }
    }

    bv->data      = new_data;
    bv->capacity  = new_cap;
    bv->byte_size = new_byte_size;
}

void BitsTryReduceSpace(Bits *bv) {
    ValidateBits(bv);

    // minimum 32 bytes maintained always
    if (bv->capacity <= 256) {
        return;
    }

    if (bv->capacity == bv->length) {
        return; // Already optimal
    }

    u64 new_byte_size = BYTES_FOR_BITS(MAX2(bv->length, 256));
    if (new_byte_size == bv->byte_size) {
        // Just update capacity, no need to realloc
        bv->capacity = bv->length;
        return;
    }

    u8 *new_data = (u8 *)realloc(bv->data, new_byte_size);
    if (!new_data) {
        // Realloc failed, but that's okay for shrinking
        return;
    }

    bv->data      = new_data;
    bv->capacity  = bv->length;
    bv->byte_size = new_byte_size;
}

Bits BitsClone(Bits *bv) {
    ValidateBits(bv);

    Bits clone = BitsInit();
    if (bv->length == 0) {
        return clone;
    }

    // Reserve space for the clone
    BitsReserve(&clone, bv->length);
    BitsResize(&clone, bv->length);

    // Copy all bits
    for (u64 i = 0; i < bv->length; i++) {
        bool bit = BitsGet(bv, i);
        BitsSet(&clone, i, bit);
    }

    return clone;
}

bool BitsGet(Bits *bv, u64 idx) {
    ValidateBits(bv);

    if (idx >= bv->length) {
        return false;
    }

    u64 byte_idx   = BIT_INDEX(idx);
    u64 bit_offset = BIT_OFFSET(idx);

    return (bv->data[byte_idx] & (1u << bit_offset)) != 0;
}

void BitsSet(Bits *bv, u64 idx, bool value) {
    ValidateBits(bv);

    if (idx >= bv->length) {
        BitsResize(bv, idx + 1);
    }

    u64 byte_idx   = BIT_INDEX(idx);
    u64 bit_offset = BIT_OFFSET(idx);

    if (value) {
        bv->data[byte_idx] |= (1u << bit_offset);
    } else {
        bv->data[byte_idx] &= ~(1u << bit_offset);
    }
}

void BitsFlip(Bits *bv, u64 idx) {
    ValidateBits(bv);

    if (idx >= bv->length) {
        BitsSet(bv, idx, true);
        return;
    }

    u64 byte_idx   = BIT_INDEX(idx);
    u64 bit_offset = BIT_OFFSET(idx);

    bv->data[byte_idx] ^= (1u << bit_offset);
}

void BitsPush(Bits *bv, bool value) {
    ValidateBits(bv);

    if (bv->length == bv->capacity) {
        u64 new_capacity = bv->capacity == 0 ? 256 : bv->capacity * 2;
        BitsReserve(bv, new_capacity);
    }

    BitsResize(bv, bv->length + 1);
    BitsSet(bv, bv->length - 1, value);
}

bool BitsPop(Bits *bv) {
    ValidateBits(bv);

    if (bv->length == 0) {
        LOG_FATAL("Cannot pop from empty Bits");
    }

    bool value = BitsGet(bv, bv->length - 1);
    BitsResize(bv, bv->length - 1);
    return value;
}

void BitsInsert(Bits *bv, u64 idx, bool value) {
    ValidateBits(bv);

    // Shift bits right from idx to end
    for (u64 i = bv->length; i > idx; i--) {
        BitsSet(bv, i, BitsGet(bv, i - 1));
    }

    BitsSet(bv, idx, value);
}

void BitsInsertMultiple(Bits *bv, u64 idx, u64 count, bool value) {
    ValidateBits(bv);

    if (count == 0) {
        return;
    }

    // Shift existing bits to the right
    for (u64 i = bv->length; i > idx; --i) {
        BitsSet(bv, i + count, BitsGet(bv, i));
    }

    // Insert the new bits
    for (u64 i = 0; i < count; i++) {
        BitsSet(bv, idx + i, value);
    }
}

void BitsConcatAt(Bits *bv, u64 idx, Bits *other) {
    ValidateBits(bv);
    ValidateBits(other);

    if (other->length == 0) {
        return;
    }

    // Shift existing bits to the right
    for (u64 i = bv->length; i > idx; i--) {
        BitsSet(bv, i + other->length, BitsGet(bv, i));
    }

    // Insert bits from other Bits
    for (u64 i = 0; i < other->length; i++) {
        BitsSet(bv, idx + i, BitsGet(other, i));
    }
}

void BitsInsertPattern(Bits *bv, u64 idx, u64 pattern, u64 pattern_bits) {
    ValidateBits(bv);

    if (pattern_bits == 0) {
        return;
    }

    // Shift existing bits to the right
    for (u64 i = bv->length; i > idx; i--) {
        BitsSet(bv, i + pattern_bits, BitsGet(bv, i));
    }

    // Insert bits from pattern (MSB first)
    pattern_bits = TO_BIG_ENDIAN8(pattern_bits);
    for (u64 i = 0; i < pattern_bits; i++) {
        bool bit = (pattern & (1u << (i & 63))) != 0;
        BitsSet(bv, idx + i, bit);
    }
}

bool BitsRemove(Bits *bv, u64 idx) {
    ValidateBits(bv);

    if (idx >= bv->length) {
        LOG_FATAL("Index %llu exceeds Bits length %llu", idx, bv->length);
    }

    // Get the bit value before removing it
    bool removed_bit = BitsGet(bv, idx);

    // Shift bits left from idx+1 to end
    for (u64 i = idx; i < bv->length - 1; i++) {
        BitsSet(bv, i, BitsGet(bv, i + 1));
    }

    BitsResize(bv, bv->length - 1);
    return removed_bit; // Return the actual bit value that was removed
}

void BitsRemoveRange(Bits *bv, u64 idx, u64 count) {
    ValidateBits(bv);

    if (idx >= bv->length) {
        LOG_FATAL("Index %llu exceeds Bits length %llu", idx, bv->length);
    }

    if (count == 0) {
        return;
    }

    // Clamp count to not exceed Bits length
    if (idx + count > bv->length) {
        count = bv->length - idx;
    }

    // Shift bits left to close the gap
    for (u64 i = idx + count; i < bv->length; i++) {
        BitsSet(bv, i - count, BitsGet(bv, i));
    }

    // Shrink the Bits
    BitsResize(bv, bv->length - count);
}

bool BitsRemoveFirst(Bits *bv, bool value) {
    ValidateBits(bv);

    // Find first occurrence of the value
    for (u64 i = 0; i < bv->length; i++) {
        if (BitsGet(bv, i) == value) {
            BitsRemove(bv, i);
            return true;
        }
    }

    return false; // Value not found
}

bool BitsRemoveLast(Bits *bv, bool value) {
    ValidateBits(bv);

    // Find last occurrence of the value (search backwards)
    for (u64 i = bv->length; i > 0;) {
        i--;
        if (BitsGet(bv, i) == value) {
            BitsRemove(bv, i);
            return true;
        }
    }

    return false; // Value not found
}

u64 BitsRemoveAll(Bits *bv, bool value) {
    ValidateBits(bv);

    u64 removed_count = 0;
    u64 write_idx     = 0;

    // Compact the Bits by copying only bits that don't match the value
    for (u64 read_idx = 0; read_idx < bv->length; read_idx++) {
        bool bit = BitsGet(bv, read_idx);
        if (bit != value) {
            // Keep this bit
            if (write_idx != read_idx) {
                BitsSet(bv, write_idx, bit);
            }
            write_idx++;
        } else {
            // Remove this bit
            removed_count++;
        }
    }

    // Shrink the Bits to the new size
    BitsResize(bv, write_idx);

    return removed_count;
}

u64 BitsCountOnes(Bits *bv) {
    ValidateBits(bv);

    u64 count = 0;
    for (u64 i = 0; i < bv->length; i++) {
        if (BitsGet(bv, i)) {
            count++;
        }
    }
    return count;
}

u64 BitsCountZeros(Bits *bv) {
    ValidateBits(bv);
    return bv->length - BitsCountOnes(bv);
}

Bits BitsAnd(Bits *a, Bits *b) {
    ValidateBits(a);
    ValidateBits(b);

    u64  min_len = MIN2(a->length, b->length);
    Bits r       = BitsInit();
    BitsReserve(&r, min_len);

    for (u64 i = 0; i < min_len; i++) {
        BitsSet(&r, i, BitsGet(a, i) && BitsGet(b, i));
    }

    return r;
}

Bits BitsOr(Bits *a, Bits *b) {
    ValidateBits(a);
    ValidateBits(b);

    u64  max_len = MAX2(a->length, b->length);
    Bits r       = BitsInit();
    BitsReserve(&r, max_len);

    for (u64 i = 0; i < max_len; i++) {
        BitsSet(&r, i, BitsGet(a, i) || BitsGet(b, i));
    }

    return r;
}

Bits BitsXor(Bits *a, Bits *b) {
    ValidateBits(a);
    ValidateBits(b);

    u64  max_len = a->length > b->length ? a->length : b->length;
    Bits r       = BitsInit();
    BitsReserve(&r, max_len);

    for (u64 i = 0; i < max_len; i++) {
        BitsSet(&r, i, BitsGet(a, i) != BitsGet(b, i));
    }

    return r;
}

Bits BitsNot(Bits *bv) {
    ValidateBits(bv);

    Bits r = BitsInit();
    BitsReserve(&r, bv->length);

    for (u64 i = 0; i < bv->length; i++) {
        BitsSet(&r, i, !BitsGet(bv, i));
    }

    return r;
}

// Comparison functions
bool BitsEquals(Bits *a, Bits *b) {
    return BitsCompare(a, b) == 0;
}

bool BitsEqualsRange(Bits *a, u64 idx_a, Bits *b, u64 idx_b, u64 len) {
    return BitsCompareRange(a, 0, b, 0, MAX2(a->length, b->length)) == 0;
}

int BitsCompare(Bits *a, Bits *b) {
    return BitsCompareRange(a, 0, b, 0, MAX2(a->length, b->length));
}

int BitsCompareRange(Bits *a, u64 idx_a, Bits *b, u64 idx_b, u64 len) {
    ValidateBits(a);
    ValidateBits(b);

    if (idx_a > a->length && idx_b > b->length) {
        return 0;
    } else if (idx_a > a->length) {
        for (u64 i = 0; i < len; i++) {
            if (false != BitsGet(b, idx_b + i)) {
                return -1;
            }
        }
    } else if (idx_b > b->length) {
        for (u64 i = 0; i < len; i++) {
            if (BitsGet(a, idx_a + i) != false) {
                return 1;
            }
        }
    } else {
        for (u64 i = 0; i < len; i++) {
            bool bit_a = BitsGet(a, idx_a + i);
            bool bit_b = BitsGet(b, idx_b + i);
            if (bit_a != bit_b) {
                return bit_a - bit_b;
            }
        }
    }

    return 0;
}

int BitsUnsignedCompareBE(Bits *a, Bits *b) {
    ValidateBits(a);
    ValidateBits(b);

    if (a->length == 0 && b->length == 0) {
        return 0;
    }

    u64 min_bytes_count = a->byte_size;

    if (a->byte_size < b->byte_size) {
        for (u64 i = a->byte_size; i < b->byte_size; i++) {
            if (b->data[i] != 0) {
                return -b->data[i];
            }
        }
    } else if (a->byte_size > b->byte_size) {
        for (u64 i = b->byte_size; i < a->byte_size; i++) {
            if (a->data[i] != 0) {
                return a->data[i];
            }
        }

        min_bytes_count = b->byte_size;
    }

    for (u64 i = min_bytes_count - 1; i > 0; i--) {
        i8 diff = a->data[i] - b->data[i];
        if (diff != 0) {
            return diff;
        }
    }

    return 0;
}

int BitsUnsignedCompareLE(Bits *a, Bits *b) {
    ValidateBits(a);
    ValidateBits(b);

    if (a->length == 0 && b->length == 0) {
        return 0;
    }

    u64 min_bytes_count = a->byte_size;
    u8 *bx              = a->data;
    u8 *by              = b->data;
    if (a->byte_size < b->byte_size) {
        u64 offset = b->byte_size - a->byte_size;

        for (u64 i = 0; i < offset; i++) {
            if (b->data[i] != 0) {
                return -b->data[i];
            }
        }

        by += offset;
    } else if (a->byte_size > b->byte_size) {
        u64 offset = a->byte_size - b->byte_size;

        for (u64 i = 0; i < offset; i++) {
            if (a->data[i] != 0) {
                return a->data[i];
            }
        }

        min_bytes_count = b->byte_size;

        bx += offset;
    }

    for (u64 i = 0; i < min_bytes_count; i++) {
        i8 diff = bx[i] - by[i];
        if (diff != 0) {
            return diff;
        }
    }

    return 0;
}

int BitsUnsignedCompare(Bits *a, Bits *b) {
    return IS_LITTLE_ENDIAN() ? BitsUnsignedCompareLE(a, b) : BitsUnsignedCompareBE(a, b);
}

int BitsWeightCompare(Bits *bv1, Bits *bv2) {
    ValidateBits(bv1);
    ValidateBits(bv2);

    u64 weight1 = BitsCountOnes(bv1);
    u64 weight2 = BitsCountOnes(bv2);

    if (weight1 < weight2) {
        return -1;
    } else if (weight1 > weight2) {
        return 1;
    } else {
        return 0;
    }
}

int BitsSignedCompareLE(Bits *a, Bits *b) {
    ValidateBits(a);
    ValidateBits(b);

    if (a->length == 0 && b->length == 0) {
        return 0;
    }

    // Get sign bits highest bit of lowest byte
    bool sign1 = a->length > 0 ? BitsGet(a, 7) : false;
    bool sign2 = b->length > 0 ? BitsGet(b, 7) : false;

    // If signs differ, negative < positive
    if (sign1 != sign2) {
        return sign1 ? -1 : 1; // true = negative, false = positive
    }

    // Same sign, compare numerically
    int result = BitsUnsignedCompareLE(a, b);

    // If both negative, reverse the comparison
    if (sign1) {
        result = -result;
    }

    return result;
}

int BitsSignedCompareBE(Bits *a, Bits *b) {
    ValidateBits(a);
    ValidateBits(b);

    if (a->length == 0 && b->length == 0) {
        return 0;
    }

    // Get sign bits (MSB)
    bool sign1 = a->length > 0 ? BitsGet(a, a->length - 1) : false;
    bool sign2 = b->length > 0 ? BitsGet(b, b->length - 1) : false;

    // If signs differ, negative < positive
    if (sign1 != sign2) {
        return sign1 ? -1 : 1; // true = negative, false = positive
    }

    // Same sign, compare numerically
    int result = BitsUnsignedCompareBE(a, b);

    // If both negative, reverse the comparison
    if (sign1) {
        result = -result;
    }

    return result;
}

int BitSignedCompare(Bits *a, Bits *b) {
    return IS_LITTLE_ENDIAN() ? BitsSignedCompareLE(a, b) : BitsSignedCompareBE(a, b);
}

bool BitsIsSubset(Bits *a, Bits *b) {
    ValidateBits(a);
    ValidateBits(b);

    // bv1 is subset of bv2 if all 1-bits in bv1 are also 1-bits in bv2
    u64 max_len = a->length > b->length ? a->length : b->length;

    for (u64 i = 0; i < max_len; i++) {
        // If bv1 has a 1-bit where bv2 has a 0-bit, bv1 is not a subset
        if (BitsGet(a, i) && !BitsGet(b, i)) {
            return false;
        }
    }

    return true;
}

bool BitsIsSuperset(Bits *a, Bits *b) {
    return BitsIsSubset(b, a);
}

bool BitsDisjoint(Bits *a, Bits *b) {
    ValidateBits(a);
    ValidateBits(b);

    // Disjoint if no common 1-bits
    u64 min_len = MIN2(a->length, b->length);

    for (u64 i = 0; i < min_len; i++) {
        if (BitsGet(a, i) && BitsGet(b, i)) {
            return false; // Found common 1-bit
        }
    }

    return true;
}

bool BitsOverlaps(Bits *bv1, Bits *bv2) {
    return !BitsDisjoint(bv1, bv2);
}

bool BitsIntersects(Bits *bv1, Bits *bv2) {
    return BitsOverlaps(bv1, bv2);
}

bool BitsIsSorted(Bits *bv) {
    ValidateBits(bv);

    // Sorted means all 0s come before all 1s
    bool found_one = false;

    for (u64 i = 0; i < bv->length; i++) {
        bool bit = BitsGet(bv, i);

        if (bit) {
            found_one = true;
        } else if (found_one) {
            // Found a 0 after a 1, not sorted
            return false;
        }
    }

    return true;
}

// Conversion functions
Str BitsToStr(Bits *bv) {
    ValidateBits(bv);

    Str result = StrInit();
    if (bv->length == 0) {
        return result;
    }

    // Reserve space for the string (length + null terminator)
    StrReserve(&result, bv->length + 1);

    // Convert each bit to '0' or '1'
    for (u64 i = 0; i < bv->length; i++) {
        char bit_char = BitsGet(bv, i) ? '1' : '0';
        StrPushBack(&result, bit_char);
    }

    return result;
}

Bits BitsFromStr(const char *str) {
    if (!str) {
        LOG_FATAL("BitsFromStr: str is NULL");
    }

    Bits result = BitsInit();

    u64 str_len = strlen(str);
    BitsReserve(&result, str_len);

    for (u64 i = 0; i < str_len; i++) {
        if (str[i] == '1') {
            BitsPush(&result, true);
        } else if (str[i] == '0') {
            BitsPush(&result, false);
        }
        // Ignore other characters
    }

    return result;
}

u64 BitsToBytes(Bits *bv, u8 *bytes, u64 max_len) {
    ValidateBits(bv);
    if (!bytes) {
        LOG_FATAL("bytes is NULL");
    }
    if (max_len == 0) {
        LOG_FATAL("max_len is 0");
    }
    if (bv->length == 0) {
        return 0;
    }

    u64 bytes_needed  = BYTES_FOR_BITS(bv->length);
    u64 bytes_to_copy = bytes_needed < max_len ? bytes_needed : max_len;

    // Clear the output buffer
    MemSet(bytes, 0, bytes_to_copy);

    // Copy bits to bytes
    for (u64 i = 0; i < bv->length && i / 8 < bytes_to_copy; i++) {
        if (BitsGet(bv, i)) {
            u64 byte_idx     = i / 8;
            u64 bit_offset   = i % 8;
            bytes[byte_idx] |= (1u << bit_offset);
        }
    }

    return bytes_to_copy;
}

Bits BitsFromBytes(const u8 *bytes, u64 bit_len) {
    if (!bytes) {
        LOG_FATAL("bytes is NULL");
    }

    Bits result = BitsInit();

    // Handle empty Bits case
    if (bit_len == 0) {
        return result;
    }

    BitsReserve(&result, bit_len);
    BitsResize(&result, bit_len);

    // Copy bits from bytes
    for (u64 i = 0; i < bit_len; i++) {
        u64  byte_idx   = i / 8;
        u64  bit_offset = i % 8;
        bool bit        = (bytes[byte_idx] & (1u << bit_offset)) != 0;
        BitsSet(&result, i, bit);
    }

    return result;
}

u64 BitsToInteger(Bits *bv) {
    ValidateBits(bv);

    if (bv->length == 0) {
        return 0;
    }

    if (bv->length > 64) {
        LOG_FATAL("Bits length greater than 64. Converting to integer directly does not make sense.");
    }

    // Convert bits to integer
    return *((u64 *)bv->data);
}

u64 BitsToIntegerBE(Bits *bv) {
    return TO_BIG_ENDIAN8(BitsToInteger(bv));
}

u64 BitsToIntegerLE(Bits *bv) {
    return TO_LITTLE_ENDIAN8(BitsToInteger(bv));
}

Bits BitsFromInteger(u64 value, u64 bits) {
    Bits result = BitsInit();
    if (bits == 0) {
        return result;
    }

    // Clamp to 64 bits maximum
    if (bits > 64) {
        bits = 64;
    }

    BitsReserve(&result, bits);
    BitsResize(&result, bits);

    // Convert integer to bits (LSB first)
    for (u64 i = 0; i < bits; i++) {
        bool bit = (value & (1ULL << i)) != 0;
        BitsSet(&result, i, bit);
    }

    return result;
}

// Shift operations
Bits BitsShiftLeft(Bits *bv, u64 positions) {
    ValidateBits(bv);

    if (positions == 0) {
        return BitsClone(bv);
    }

    Bits r = BitsInit();

    BitsConcatAt(&r, positions, bv);
    return r;
}

Bits BitsShiftRight(Bits *bv, u64 positions) {
    ValidateBits(bv);

    if (positions == 0) {
        return BitsClone(bv);
    }

    if (positions >= bv->length) {
        return BitsInit();
    }

    Bits r = BitsInit();

    // Shift bits right (move bits to lower indices)
    for (u64 i = bv->length - 1; i >= positions; i--) {
        BitsSet(&r, i - positions, BitsGet(bv, i));
    }

    return r;
}

Bits BitsRotateLeft(Bits *bv, u64 positions) {
    ValidateBits(bv);

    positions = positions % bv->length; // Handle positions > length
    if (positions == 0) {
        return BitsClone(bv);
    }

    Bits r = BitsInit();

    // Rotate left
    for (u64 i = 0; i < bv->length; i++) {
        BitsSet(&r, i, BitsGet(bv, (i + positions) % bv->length));
    }

    return r;
}

Bits BitsRotateRight(Bits *bv, u64 positions) {
    ValidateBits(bv);

    positions = positions % bv->length; // Handle positions > length
    if (positions == 0) {
        return BitsClone(bv);
    }

    Bits r = BitsInit();

    // Rotate right
    for (u64 i = 0; i < bv->length; i++) {
        BitsSet(&r, i, BitsGet(bv, (i + bv->length - positions) % bv->length));
    }

    return r;
}

Bits BitsReverse(Bits *bv) {
    ValidateBits(bv);
    if (bv->length <= 1) {
        return BitsClone(bv);
    }

    Bits r = BitsInit();

    for (u64 i = 0; i < bv->length; i++) {
        BitsSet(&r, i, BitsGet(bv, bv->length - i - 1));
    }

    return r;
}

// Missing Access functions implementation

u64 BitsFind(Bits *bv, bool value) {
    ValidateBits(bv);

    for (u64 i = 0; i < bv->length; i++) {
        if (BitsGet(bv, i) == value) {
            return i;
        }
    }

    return UINT64_MAX; // Not found
}

u64 BitsFindLast(Bits *bv, bool value) {
    ValidateBits(bv);

    if (bv->length == 0) {
        return UINT64_MAX;
    }

    for (u64 i = bv->length - 1;; i--) { // i < bv->length handles underflow
        if (BitsGet(bv, i) == value) {
            return i;
        }
        if (i == 0)
            break;     // Prevent underflow
    }
    return UINT64_MAX; // Not found
}

bool BitsAll(Bits *bv, bool value) {
    ValidateBits(bv);

    for (u64 i = 0; i < bv->length; i++) {
        if (BitsGet(bv, i) != value) {
            return false;
        }
    }
    return true; // All match (or empty Bits)
}

bool BitsAny(Bits *bv, bool value) {
    ValidateBits(bv);

    for (u64 i = 0; i < bv->length; i++) {
        if (BitsGet(bv, i) == value) {
            return true;
        }
    }
    return false; // None match
}

bool BitsNone(Bits *bv, bool value) {
    return !BitsAny(bv, value);
}

u64 BitsLongestRun(Bits *bv, bool value) {
    ValidateBits(bv);

    if (bv->length == 0) {
        return 0;
    }

    u64 max_run     = 0;
    u64 current_run = 0;

    for (u64 i = 0; i < bv->length; i++) {
        if (BitsGet(bv, i) == value) {
            current_run++;
            if (current_run > max_run) {
                max_run = current_run;
            }
        } else {
            current_run = 0;
        }
    }

    return max_run;
}

// Pattern search functions
u64 BitsFindPattern(Bits *bv, Bits *pattern) {
    ValidateBits(bv);
    ValidateBits(pattern);

    if (pattern->length == 0 || pattern->length > bv->length) {
        return SIZE_MAX;
    }

    for (u64 i = 0; i <= bv->length - pattern->length; i++) {
        bool match = true;
        for (u64 j = 0; j < pattern->length; j++) {
            if (BitsGet(bv, i + j) != BitsGet(pattern, j)) {
                match = false;
                break;
            }
        }
        if (match) {
            return i;
        }
    }
    return SIZE_MAX; // Not found
}

u64 BitsFindLastPattern(Bits *bv, Bits *pattern) {
    ValidateBits(bv);
    ValidateBits(pattern);

    if (pattern->length == 0 || pattern->length > bv->length) {
        return SIZE_MAX;
    }

    for (u64 i = bv->length - pattern->length; i < bv->length; i--) { // i < bv->length handles underflow
        bool match = true;
        for (u64 j = 0; j < pattern->length; j++) {
            if (BitsGet(bv, i + j) != BitsGet(pattern, j)) {
                match = false;
                break;
            }
        }
        if (match) {
            return i;
        }
        if (i == 0)
            break;   // Prevent underflow
    }
    return SIZE_MAX; // Not found
}

Indices BitsFindAllPattern(Bits *bv, Bits *pattern) {
    ValidateBits(bv);
    ValidateBits(pattern);

    if (pattern->length == 0 || pattern->length > bv->length) {
        return (Indices)VecInit();
    }

    Indices indices = VecInit();

    for (u64 i = 0; i <= bv->length - pattern->length; i++) {
        bool match = true;
        for (u64 j = 0; j < pattern->length; j++) {
            if (BitsGet(bv, i + j) != BitsGet(pattern, j)) {
                match = false;
                break;
            }
        }
        if (match) {
            VecPushBackR(&indices, i);
        }
    }

    return indices;
}

// Foreach functions

u64 BitsRunLengths(Bits *bv, u64 *runs, bool *values, u64 max_runs) {
    ValidateBits(bv);
    if (!runs || !values || max_runs == 0) {
        LOG_FATAL("BitsRunLengths: invalid arguments");
    }

    if (bv->length == 0) {
        return 0;
    }

    u64  run_count          = 0;
    u64  current_run_length = 1;
    bool current_value      = BitsGet(bv, 0);

    for (u64 i = 1; i < bv->length; i++) {
        bool bit = BitsGet(bv, i);
        if (bit == current_value) {
            current_run_length++;
        } else {
            // End of current run - check if we have space before writing
            if (run_count < max_runs) {
                runs[run_count]   = current_run_length;
                values[run_count] = current_value;
                run_count++;
            } else {
                // No more space, stop processing
                break;
            }

            // Start new run
            current_value      = bit;
            current_run_length = 1;
        }
    }

    // Add the last run if there's space
    if (run_count < max_runs) {
        runs[run_count]   = current_run_length;
        values[run_count] = current_value;
        run_count++;
    }

    return run_count;
}

// Math functions implementation

u64 BitsHammingDistance(Bits *bv1, Bits *bv2) {
    ValidateBits(bv1);
    ValidateBits(bv2);

    u64 min_length = bv1->length < bv2->length ? bv1->length : bv2->length;
    u64 max_length = bv1->length > bv2->length ? bv1->length : bv2->length;
    u64 distance   = 0;

    // Count differences in overlapping region
    for (u64 i = 0; i < min_length; i++) {
        if (BitsGet(bv1, i) != BitsGet(bv2, i)) {
            distance++;
        }
    }

    // Add length difference as extra distance
    distance += (max_length - min_length);

    return distance;
}

double BitsJaccardSimilarity(Bits *bv1, Bits *bv2) {
    ValidateBits(bv1);
    ValidateBits(bv2);

    if (bv1->length == 0 && bv2->length == 0) {
        return 1.0; // Both empty, consider identical
    }

    u64 max_length   = bv1->length > bv2->length ? bv1->length : bv2->length;
    u64 intersection = 0;
    u64 union_count  = 0;

    for (u64 i = 0; i < max_length; i++) {
        bool bit1 = (i < bv1->length) ? BitsGet(bv1, i) : false;
        bool bit2 = (i < bv2->length) ? BitsGet(bv2, i) : false;

        if (bit1 && bit2) {
            intersection++;
        }
        if (bit1 || bit2) {
            union_count++;
        }
    }

    return union_count == 0 ? 1.0 : (double)intersection / (double)union_count;
}

double BitsCosineSimilarity(Bits *bv1, Bits *bv2) {
    ValidateBits(bv1);
    ValidateBits(bv2);

    u64 dot_product = BitsDotProduct(bv1, bv2);
    u64 ones1       = BitsCountOnes(bv1);
    u64 ones2       = BitsCountOnes(bv2);

    if (ones1 == 0 || ones2 == 0) {
        return 0.0;
    }

    double magnitude1 = sqrt((double)ones1);
    double magnitude2 = sqrt((double)ones2);

    return (double)dot_product / (magnitude1 * magnitude2);
}

u64 BitsDotProduct(Bits *bv1, Bits *bv2) {
    ValidateBits(bv1);
    ValidateBits(bv2);

    u64 min_length = bv1->length < bv2->length ? bv1->length : bv2->length;
    u64 product    = 0;

    for (u64 i = 0; i < min_length; i++) {
        if (BitsGet(bv1, i) && BitsGet(bv2, i)) {
            product++;
        }
    }

    return product;
}

u64 BitsEditDistance(Bits *a, Bits *b) {
    ValidateBits(a);
    ValidateBits(b);

    u64 len1 = a->length;
    u64 len2 = b->length;

    if (len1 == 0)
        return len2;
    if (len2 == 0)
        return len1;

    // Dynamic programming matrix
    typedef Vec(u64) Distances;
    Distances prev_row = VecInit();
    Distances curr_row = VecInit();

    VecReserve(&prev_row, len2);
    VecReserve(&curr_row, len1);

    // Initialize first row
    for (u64 j = 0; j <= len2; j++) {
        VecPushBackR(&prev_row, j);
    }

    for (u64 i = 1; i <= len1; i++) {
        VecPushBackR(&curr_row, i);

        for (u64 j = 1; j <= len2; j++) {
            u64 deletion     = VecAt(&prev_row, j) + 1;
            u64 insertion    = VecAt(&curr_row, j - 1) + 1;
            u64 substitution = VecAt(&prev_row, j - 1) + BitsGet(a, i - 1) != BitsGet(b, j - 1);

            VecAt(&curr_row, j) = MIN2(deletion, insertion);
            VecAt(&curr_row, j) = MIN2(VecAt(&curr_row, j), substitution);
        }

        // Swap rows
        Distances temp = prev_row;
        prev_row       = curr_row;
        curr_row       = temp;
    }

    u64 result = VecAt(&prev_row, len2);
    VecDeinit(&prev_row);
    VecDeinit(&curr_row);

    return result;
}

double BitsCorrelation(Bits *bv1, Bits *bv2) {
    ValidateBits(bv1);
    ValidateBits(bv2);

    u64 max_length = MAX2(bv1->length, bv2->length);

    if (max_length == 0)
        return 1.0;

    double sum1 = 0, sum2 = 0, sum1_sq = 0, sum2_sq = 0, sum_product = 0;

    for (u64 i = 0; i < max_length; i++) {
        double val1 = (i < bv1->length && BitsGet(bv1, i)) ? 1.0 : 0.0;
        double val2 = (i < bv2->length && BitsGet(bv2, i)) ? 1.0 : 0.0;

        sum1        += val1;
        sum2        += val2;
        sum1_sq     += val1 * val1;
        sum2_sq     += val2 * val2;
        sum_product += val1 * val2;
    }

    double n           = (double)max_length;
    double numerator   = n * sum_product - sum1 * sum2;
    double denominator = sqrt((n * sum1_sq - sum1 * sum1) * (n * sum2_sq - sum2 * sum2));

    return denominator == 0.0 ? 0.0 : numerator / denominator;
}

double BitsEntropy(Bits *bv) {
    ValidateBits(bv);

    if (bv->length == 0)
        return 0.0;

    u64 ones  = BitsCountOnes(bv);
    u64 zeros = bv->length - ones;

    if (ones == 0 || zeros == 0)
        return 0.0; // No entropy in uniform data

    double p1 = (double)ones / (double)bv->length;
    double p0 = (double)zeros / (double)bv->length;

    return -(p1 * log2(p1) + p0 * log2(p0));
}

int BitsAlignmentScore(Bits *bv1, Bits *bv2, int match, int mismatch) {
    ValidateBits(bv1);
    ValidateBits(bv2);

    u64 min_length = bv1->length < bv2->length ? bv1->length : bv2->length;
    int score      = 0;

    for (u64 i = 0; i < min_length; i++) {
        if (BitsGet(bv1, i) == BitsGet(bv2, i)) {
            score += match;
        } else {
            score += mismatch;
        }
    }

    return score;
}

u64 BitsBestAlignment(Bits *bv1, Bits *bv2) {
    ValidateBits(bv1);
    ValidateBits(bv2);

    if (bv1->length == 0 || bv2->length == 0) {
        return 0;
    }

    u64 best_offset = 0;
    int best_score  = INT_MIN;

    // Try all possible alignments of bv2 against bv1
    for (u64 offset = 0; offset <= bv1->length; offset++) {
        int score   = 0;
        u64 overlap = 0;

        for (u64 i = 0; i < bv2->length && (offset + i) < bv1->length; i++) {
            if (BitsGet(bv1, offset + i) == BitsGet(bv2, i)) {
                score++;
            } else {
                score--;
            }
            overlap++;
        }

        if (overlap > 0 && score > best_score) {
            best_score  = score;
            best_offset = offset;
        }
    }

    return best_offset;
}

// Missing Pattern functions implementation

bool BitsStartsWith(Bits *bv, Bits *prefix) {
    ValidateBits(bv);
    ValidateBits(prefix);

    if (prefix->length > bv->length) {
        return false;
    }

    for (u64 i = 0; i < prefix->length; i++) {
        if (BitsGet(bv, i) != BitsGet(prefix, i)) {
            return false;
        }
    }

    return true;
}

bool BitsEndsWith(Bits *bv, Bits *suffix) {
    ValidateBits(bv);
    ValidateBits(suffix);

    if (suffix->length > bv->length) {
        return false;
    }

    u64 start_pos = bv->length - suffix->length;
    for (u64 i = 0; i < suffix->length; i++) {
        if (BitsGet(bv, start_pos + i) != BitsGet(suffix, i)) {
            return false;
        }
    }

    return true;
}

bool BitsContains(Bits *bv, Bits *pattern) {
    return BitsFindPattern(bv, pattern) != SIZE_MAX;
}

u64 BitsCountPattern(Bits *bv, Bits *pattern) {
    ValidateBits(bv);
    ValidateBits(pattern);

    if (pattern->length == 0 || pattern->length > bv->length) {
        return 0;
    }

    u64 count = 0;
    for (u64 i = 0; i <= bv->length - pattern->length; i++) {
        bool match = true;
        for (u64 j = 0; j < pattern->length; j++) {
            if (BitsGet(bv, i + j) != BitsGet(pattern, j)) {
                match = false;
                break;
            }
        }
        if (match) {
            count++;
        }
    }

    return count;
}

u64 BitsRFindPattern(Bits *bv, Bits *pattern, u64 start) {
    ValidateBits(bv);
    ValidateBits(pattern);

    if (pattern->length == 0 || pattern->length > bv->length || start >= bv->length) {
        return SIZE_MAX;
    }

    u64 search_end = (start + 1 >= pattern->length) ? start + 1 - pattern->length : 0;

    for (u64 i = start + 1; i > search_end; i--) {
        u64 pos = i - 1;
        if (pos + pattern->length > bv->length)
            continue;

        bool match = true;
        for (u64 j = 0; j < pattern->length; j++) {
            if (BitsGet(bv, pos + j) != BitsGet(pattern, j)) {
                match = false;
                break;
            }
        }
        if (match) {
            return pos;
        }
    }

    return SIZE_MAX;
}

bool BitsReplace(Bits *bv, Bits *old_pattern, Bits *new_pattern) {
    ValidateBits(bv);
    ValidateBits(old_pattern);
    ValidateBits(new_pattern);

    u64 pos = BitsFindPattern(bv, old_pattern);
    if (pos == SIZE_MAX) {
        return false;
    }

    // Remove old pattern
    BitsRemoveRange(bv, pos, old_pattern->length);

    // Insert new pattern
    for (u64 i = 0; i < new_pattern->length; i++) {
        BitsInsert(bv, pos + i, BitsGet(new_pattern, i));
    }

    return true;
}

u64 BitsReplaceAll(Bits *bv, Bits *old_pattern, Bits *new_pattern) {
    ValidateBits(bv);
    ValidateBits(old_pattern);
    ValidateBits(new_pattern);

    u64 replacements = 0;
    u64 search_pos   = 0;

    while (search_pos < bv->length) {
        // Find next occurrence
        bool found     = false;
        u64  match_pos = SIZE_MAX;

        if (search_pos + old_pattern->length <= bv->length) {
            for (u64 i = search_pos; i <= bv->length - old_pattern->length; i++) {
                bool match = true;
                for (u64 j = 0; j < old_pattern->length; j++) {
                    if (BitsGet(bv, i + j) != BitsGet(old_pattern, j)) {
                        match = false;
                        break;
                    }
                }
                if (match) {
                    match_pos = i;
                    found     = true;
                    break;
                }
            }
        }

        if (!found)
            break;

        // Remove old pattern
        BitsRemoveRange(bv, match_pos, old_pattern->length);

        // Insert new pattern
        for (u64 i = 0; i < new_pattern->length; i++) {
            BitsInsert(bv, match_pos + i, BitsGet(new_pattern, i));
        }

        replacements++;
        search_pos = match_pos + new_pattern->length;
    }

    return replacements;
}

bool BitsMatches(Bits *bv, Bits *pattern, Bits *wildcard) {
    ValidateBits(bv);
    ValidateBits(pattern);
    ValidateBits(wildcard);

    if (bv->length != pattern->length || pattern->length != wildcard->length) {
        return false;
    }

    for (u64 i = 0; i < bv->length; i++) {
        if (!BitsGet(wildcard, i)) { // Not a wildcard position
            if (BitsGet(bv, i) != BitsGet(pattern, i)) {
                return false;
            }
        }
    }

    return true;
}

u64 BitsFuzzyMatch(Bits *bv, Bits *pattern, u64 max_errors) {
    ValidateBits(bv);
    ValidateBits(pattern);

    if (pattern->length > bv->length) {
        return SIZE_MAX;
    }

    for (u64 i = 0; i <= bv->length - pattern->length; i++) {
        u64 errors = 0;
        for (u64 j = 0; j < pattern->length; j++) {
            if (BitsGet(bv, i + j) != BitsGet(pattern, j)) {
                errors++;
                if (errors > max_errors) {
                    break;
                }
            }
        }
        if (errors <= max_errors) {
            return i;
        }
    }

    return SIZE_MAX;
}

u64 BitsPrefixMatch(Bits *bv, Bits *patterns, u64 num_patterns) {
    ValidateBits(bv);
    if (!patterns || num_patterns == 0) {
        LOG_FATAL("BitsPrefixMatch: invalid arguments");
    }

    for (u64 i = 0; i < num_patterns; i++) {
        if (BitsStartsWith(bv, &patterns[i])) {
            return i;
        }
    }

    return UINT64_MAX;
}

u64 BitsSuffixMatch(Bits *bv, Bits *patterns, u64 num_patterns) {
    ValidateBits(bv);
    if (!patterns || num_patterns == 0) {
        LOG_FATAL("BitsSuffixMatch: invalid arguments");
    }

    for (u64 i = 0; i < num_patterns; i++) {
        if (BitsEndsWith(bv, &patterns[i])) {
            return i;
        }
    }

    return UINT64_MAX;
}
