/// file      : std/container/bitvec.c
/// author    : Generated following Misra project patterns
/// This is free and unencumbered software released into the public domain.
///
/// Bit vector implementation - efficient storage for boolean values

#include <Misra/Std/Container/BitVec.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Memory.h>
#include <Misra/Std/Log.h>

// In-tree replacements for the two libm calls BitVec used (sqrt + log2,
// both in similarity / entropy methods). Input domain is positive
// reals -- the callers only ever feed magnitudes (sqrt of bit-counts,
// log2 of probability fractions in (0,1]) -- so we don't have to
// handle negatives, subnormals, or NaN. IEEE-754 f64 assumed.

// Newton-Raphson square root. Initial guess via halving the binary
// exponent (so y0 ≈ sqrt_f64(x) to within a factor of ~1.4); 6 iterations
// then converge to full f64 precision since each step roughly doubles
// the significant bits.
static double sqrt_f64(double x) {
    if (x <= 0.0) {
        return 0.0;
    }
    union {
        u64    i;
        double d;
    } u;
    u.d = x;
    // (exp_unbiased) / 2 + bias, with the mantissa preserved gives a
    // good starting estimate.
    u.i      = (u.i + ((u64)1023 << 52)) >> 1;
    double y = u.d;
    for (int i = 0; i < 6; ++i) {
        y = 0.5 * (y + x / y);
    }
    return y;
}

// log2_f64(x) via exponent extraction plus a Padé-like odd-power series
// on (m-1)/(m+1) for the mantissa. ln(m) ≈ 2y(1 + y²/3 + y⁴/5 + y⁶/7)
// where y = (m-1)/(m+1) and m is in [1, 2); divide by ln(2) to get
// log2. Sub-ULP precision for the probability range BitVec feeds it.
static double log2_f64(double x) {
    if (x <= 0.0) {
        return 0.0; // BitVec entropy: never called on non-positives in practice
    }
    union {
        u64    i;
        double d;
    } u;
    u.d         = x;
    int raw_exp = (int)((u.i >> 52) & 0x7FF);
    int exp     = raw_exp - 1023;
    // Normalize mantissa to [1, 2) by clearing the exponent field
    // and stamping in bias=1023.
    u.i                           = (u.i & ((1ULL << 52) - 1ULL)) | ((u64)1023 << 52);
    double              m         = u.d;
    double              y         = (m - 1.0) / (m + 1.0);
    double              y2        = y * y;
    double              series    = y * (1.0 + y2 * (1.0 / 3.0 + y2 * (1.0 / 5.0 + y2 * (1.0 / 7.0))));
    static const double LN2_RECIP = 1.4426950408889634; // 1 / ln(2)
    return (double)exp + 2.0 * series * LN2_RECIP;
}

// Ensure SIZE_MAX is defined
#ifndef SIZE_MAX
#    if defined(_MSC_VER) || defined(__MSC_VER)
#        if defined(_WIN64)
#            define SIZE_MAX 18446744073709551615ULL
#        else
#            define SIZE_MAX 4294967295U
#        endif
#    else
#        if defined(__SIZEOF_LONG__) && __SIZEOF_LONG__ == 8
#            define SIZE_MAX 18446744073709551615ULL
#        else
#            define SIZE_MAX 4294967295U
#        endif
#    endif
#endif

// Helper macros for bit operations
#define BITS_PER_BYTE        8
#define BIT_INDEX(idx)       ((idx) / BITS_PER_BYTE)
#define BIT_OFFSET(idx)      ((idx) % BITS_PER_BYTE)
#define BYTES_FOR_BITS(bits) (((bits) + BITS_PER_BYTE - 1) / BITS_PER_BYTE)

BitVec bitvec_init_with_capacity(u64 cap, Allocator *alloc) {
    BitVec result = BitVecInit(alloc);

    if (cap == 0) {
        return result;
    }

    result.data = (u8 *)AllocatorAlloc(result.allocator, BITVEC_BYTES_FOR_BITS(cap), true);
    if (!result.data) {
        return result;
    }

    result.capacity  = cap;
    result.byte_size = BITVEC_BYTES_FOR_BITS(cap);
    return result;
}

void BitVecDeinit(BitVec *bitvec) {
    ValidateBitVec(bitvec);
    if (bitvec->data) {
        AllocatorFree(bitvec->allocator, bitvec->data);
    }
    MemSet(bitvec, 0, sizeof(*bitvec));
}

void BitVecClear(BitVec *bitvec) {
    ValidateBitVec(bitvec);
    bitvec->length = 0;
    if (bitvec->data && bitvec->byte_size > 0) {
        MemSet(bitvec->data, 0, bitvec->byte_size);
    }
}

bool BitVecResize(BitVec *bitvec, u64 new_size) {
    ValidateBitVec(bitvec);
    if (new_size > bitvec->capacity) {
        if (!BitVecReserve(bitvec, new_size)) {
            return false;
        }
    }

    // If growing, clear new bits
    if (new_size > bitvec->length && bitvec->data) {
        u64 old_bytes = BYTES_FOR_BITS(bitvec->length);
        u64 new_bytes = BYTES_FOR_BITS(new_size);

        if (new_bytes > old_bytes) {
            MemSet(bitvec->data + old_bytes, 0, new_bytes - old_bytes);
        }

        // Clear any partial bits in the last byte of old length
        if (bitvec->length > 0) {
            u64 last_bit_offset = BIT_OFFSET(bitvec->length);
            if (last_bit_offset != 0) {
                u64 last_byte_idx            = BIT_INDEX(bitvec->length);
                u8  mask                     = (1u << last_bit_offset) - 1u;
                bitvec->data[last_byte_idx] &= mask;
            }
        }
    }

    bitvec->length = new_size;
    return true;
}

bool BitVecReserve(BitVec *bitvec, u64 n) {
    ValidateBitVec(bitvec);
    if (n <= bitvec->capacity)
        return true;
    u64 new_byte_size = BYTES_FOR_BITS(n);
    u8 *new_data      = (u8 *)AllocatorRealloc(bitvec->allocator, bitvec->data, new_byte_size);

    if (!new_data) {
        LOG_ERROR("Failed to allocate memory for bitvec");
        return false;
    }

    // Clear new bytes
    if (new_byte_size > bitvec->byte_size) {
        MemSet(new_data + bitvec->byte_size, 0, new_byte_size - bitvec->byte_size);
    }

    bitvec->data      = new_data;
    bitvec->capacity  = n;
    bitvec->byte_size = new_byte_size;
    return true;
}

void BitVecShrinkToFit(BitVec *bv) {
    ValidateBitVec(bv);
    if (bv->length == 0) {
        // Free all memory if empty
        AllocatorFree(bv->allocator, bv->data);
        bv->data      = NULL;
        bv->capacity  = 0;
        bv->byte_size = 0;
        return;
    }

    if (bv->capacity <= bv->length) {
        return; // Already optimal
    }

    u64 new_byte_size = BYTES_FOR_BITS(bv->length);
    if (new_byte_size == bv->byte_size) {
        // Just update capacity, no need to realloc
        bv->capacity = bv->length;
        return;
    }

    u8 *new_data = (u8 *)AllocatorRealloc(bv->allocator, bv->data, new_byte_size);
    if (!new_data) {
        // Realloc failed, but that's okay for shrinking
        return;
    }

    bv->data      = new_data;
    bv->capacity  = bv->length;
    bv->byte_size = new_byte_size;
}



void BitVecSwap(BitVec *bv1, BitVec *bv2) {
    ValidateBitVec(bv1);
    ValidateBitVec(bv2);

    // Swap all fields
    u8 *temp_data = bv1->data;
    bv1->data     = bv2->data;
    bv2->data     = temp_data;

    u64 temp_length = bv1->length;
    bv1->length     = bv2->length;
    bv2->length     = temp_length;

    u64 temp_capacity = bv1->capacity;
    bv1->capacity     = bv2->capacity;
    bv2->capacity     = temp_capacity;

    u64 temp_byte_size = bv1->byte_size;
    bv1->byte_size     = bv2->byte_size;
    bv2->byte_size     = temp_byte_size;
}

bool BitVecTryClone(BitVec *out, BitVec *bv) {
    ValidateBitVec(bv);
    if (!out) {
        LOG_FATAL("Invalid arguments");
    }

    *out = BitVecInit(bv->allocator);
    if (bv->length == 0) {
        return true;
    }

    // Reserve space for the clone
    if (!BitVecReserve(out, bv->length) || !BitVecResize(out, bv->length)) {
        BitVecDeinit(out);
        *out = BitVecInit(bv->allocator);
        return false;
    }

    // Copy all bits
    for (u64 i = 0; i < bv->length; i++) {
        bool bit = BitVecGet(bv, i);
        BitVecSet(out, i, bit);
    }

    return true;
}

BitVec BitVecClone(BitVec *bv) {
    BitVec clone;

    ValidateBitVec(bv);
    clone = BitVecInit(bv->allocator);
    (void)BitVecTryClone(&clone, bv);
    return clone;
}

bool BitVecGet(BitVec *bitvec, u64 idx) {
    ValidateBitVec(bitvec);
    if (idx >= bitvec->length) {
        LOG_FATAL("Index {} exceeds bitvector length {}", idx, bitvec->length);
    }
    u64 byte_idx   = BIT_INDEX(idx);
    u64 bit_offset = BIT_OFFSET(idx);

    return (bitvec->data[byte_idx] & (1u << bit_offset)) != 0;
}

void BitVecSet(BitVec *bitvec, u64 idx, bool value) {
    ValidateBitVec(bitvec);
    if (idx >= bitvec->length) {
        LOG_FATAL("Index {} exceeds bitvector length {}", idx, bitvec->length);
    }
    u64 byte_idx   = BIT_INDEX(idx);
    u64 bit_offset = BIT_OFFSET(idx);

    if (value) {
        bitvec->data[byte_idx] |= (1u << bit_offset);
    } else {
        bitvec->data[byte_idx] &= ~(1u << bit_offset);
    }
}

void BitVecFlip(BitVec *bitvec, u64 idx) {
    ValidateBitVec(bitvec);
    if (idx >= bitvec->length) {
        LOG_FATAL("Index {} exceeds bitvector length {}", idx, bitvec->length);
    }
    u64 byte_idx   = BIT_INDEX(idx);
    u64 bit_offset = BIT_OFFSET(idx);

    bitvec->data[byte_idx] ^= (1u << bit_offset);
}

bool BitVecPush(BitVec *bitvec, bool value) {
    ValidateBitVec(bitvec);
    if (bitvec->length >= bitvec->capacity) {
        u64 new_capacity = bitvec->capacity == 0 ? 8 : bitvec->capacity * 2;
        if (!BitVecReserve(bitvec, new_capacity)) {
            return false;
        }
    }

    if (!BitVecResize(bitvec, bitvec->length + 1)) {
        return false;
    }
    BitVecSet(bitvec, bitvec->length - 1, value);
    return true;
}

bool BitVecPop(BitVec *bitvec) {
    ValidateBitVec(bitvec);
    if (bitvec->length == 0) {
        LOG_FATAL("Cannot pop from empty bitvector");
    }
    bool value = BitVecGet(bitvec, bitvec->length - 1);
    BitVecResize(bitvec, bitvec->length - 1);
    return value;
}

bool BitVecInsert(BitVec *bitvec, u64 idx, bool value) {
    ValidateBitVec(bitvec);
    if (idx > bitvec->length) {
        LOG_FATAL("Index {} exceeds bitvector length {}", idx, bitvec->length);
    }
    // For now, implement as push + manual bit shifting (simple but not efficient)
    if (!BitVecPush(bitvec, false)) {
        return false;
    }

    // Shift bits right from idx to end
    for (u64 i = bitvec->length - 1; i > idx; i--) {
        bool bit = BitVecGet(bitvec, i - 1);
        BitVecSet(bitvec, i, bit);
    }

    BitVecSet(bitvec, idx, value);
    return true;
}

bool BitVecInsertRange(BitVec *bv, u64 idx, u64 count, bool value) {
    ValidateBitVec(bv);
    if (idx > bv->length) {
        LOG_FATAL("Index {} exceeds bitvector length {}", idx, bv->length);
    }
    if (count == 0) {
        return true;
    }

    // Grow the bitvector to accommodate new bits
    u64 old_length = bv->length;
    if (!BitVecResize(bv, old_length + count)) {
        return false;
    }

    // Shift existing bits to the right
    for (u64 i = old_length; i > idx;) {
        i--;
        bool bit = BitVecGet(bv, i);
        BitVecSet(bv, i + count, bit);
    }

    // Insert the new bits
    for (u64 i = 0; i < count; i++) {
        BitVecSet(bv, idx + i, value);
    }

    return true;
}

bool BitVecInsertMultiple(BitVec *bv, u64 idx, BitVec *other) {
    ValidateBitVec(bv);
    ValidateBitVec(other);
    if (idx > bv->length) {
        LOG_FATAL("Index {} exceeds bitvector length {}", idx, bv->length);
    }
    if (other->length == 0) {
        return true;
    }

    // Grow the bitvector to accommodate new bits
    u64 old_length = bv->length;
    if (!BitVecResize(bv, old_length + other->length)) {
        return false;
    }

    // Shift existing bits to the right
    for (u64 i = old_length; i > idx;) {
        i--;
        bool bit = BitVecGet(bv, i);
        BitVecSet(bv, i + other->length, bit);
    }

    // Insert bits from other bitvector
    for (u64 i = 0; i < other->length; i++) {
        bool bit = BitVecGet(other, i);
        BitVecSet(bv, idx + i, bit);
    }

    return true;
}

bool BitVecInsertPattern(BitVec *bv, u64 idx, u8 pattern, u64 pattern_bits) {
    ValidateBitVec(bv);
    if (idx > bv->length) {
        LOG_FATAL("Index {} exceeds bitvector length {}", idx, bv->length);
    }
    if (pattern_bits == 0 || pattern_bits > 8) {
        return true;
    }

    // Grow the bitvector to accommodate new bits
    u64 old_length = bv->length;
    if (!BitVecResize(bv, old_length + pattern_bits)) {
        return false;
    }

    // Shift existing bits to the right
    for (u64 i = old_length; i > idx;) {
        i--;
        bool bit = BitVecGet(bv, i);
        BitVecSet(bv, i + pattern_bits, bit);
    }

    // Insert bits from pattern (LSB first)
    for (u64 i = 0; i < pattern_bits; i++) {
        bool bit = (pattern & (1u << i)) != 0;
        BitVecSet(bv, idx + i, bit);
    }

    return true;
}

bool BitVecRemove(BitVec *bv, u64 idx) {
    ValidateBitVec(bv);
    if (idx >= bv->length) {
        LOG_FATAL("Index {} exceeds bitvector length {}", idx, bv->length);
    }

    // Get the bit value before removing it
    bool removed_bit = BitVecGet(bv, idx);

    // Shift bits left from idx+1 to end
    for (u64 i = idx; i < bv->length - 1; i++) {
        bool bit = BitVecGet(bv, i + 1);
        BitVecSet(bv, i, bit);
    }

    BitVecResize(bv, bv->length - 1);
    return removed_bit; // Return the actual bit value that was removed
}

void BitVecRemoveRange(BitVec *bv, u64 idx, u64 count) {
    ValidateBitVec(bv);
    if (idx >= bv->length) {
        LOG_FATAL("Index {} exceeds bitvector length {}", idx, bv->length);
    }
    if (count == 0) {
        return;
    }

    // Clamp count to not exceed bitvector length
    if (idx + count > bv->length) {
        count = bv->length - idx;
    }

    // Shift bits left to close the gap
    for (u64 i = idx + count; i < bv->length; i++) {
        bool bit = BitVecGet(bv, i);
        BitVecSet(bv, i - count, bit);
    }

    // Shrink the bitvector
    BitVecResize(bv, bv->length - count);
}

bool BitVecRemoveFirst(BitVec *bv, bool value) {
    ValidateBitVec(bv);

    u64 pos = BitVecFind(bv, value);
    if (pos != SIZE_MAX) {
        BitVecRemove(bv, pos);
        return true;
    }

    return false; // Value not found
}

bool BitVecRemoveLast(BitVec *bv, bool value) {
    ValidateBitVec(bv);

    u64 pos = BitVecFindLast(bv, value);
    if (pos != SIZE_MAX) {
        BitVecRemove(bv, pos);
        return true;
    }

    return false; // Value not found
}

u64 BitVecRemoveAll(BitVec *bv, bool value) {
    ValidateBitVec(bv);

    u64 removed_count = 0;
    u64 write_idx     = 0;

    // Compact the bitvector by copying only bits that don't match the value
    for (u64 read_idx = 0; read_idx < bv->length; read_idx++) {
        bool bit = BitVecGet(bv, read_idx);
        if (bit != value) {
            // Keep this bit
            if (write_idx != read_idx) {
                BitVecSet(bv, write_idx, bit);
            }
            write_idx++;
        } else {
            // Remove this bit
            removed_count++;
        }
    }

    // Shrink the bitvector to the new size
    BitVecResize(bv, write_idx);

    return removed_count;
}

u64 BitVecCountOnes(BitVec *bitvec) {
    ValidateBitVec(bitvec);
    if (!bitvec->data)
        return 0;
    u64 count = 0;
    for (u64 i = 0; i < bitvec->length; i++) {
        if (BitVecGet(bitvec, i)) {
            count++;
        }
    }
    return count;
}

u64 BitVecCountZeros(BitVec *bitvec) {
    ValidateBitVec(bitvec);
    return bitvec->length - BitVecCountOnes(bitvec);
}

void BitVecAnd(BitVec *result, BitVec *a, BitVec *b) {
    ValidateBitVec(result);
    ValidateBitVec(a);
    ValidateBitVec(b);

    u64 min_len = MIN2(a->length, b->length);
    if (!BitVecResize(result, min_len)) {
        return;
    }

    for (u64 i = 0; i < min_len; i++) {
        bool bit_a = BitVecGet(a, i);
        bool bit_b = BitVecGet(b, i);
        BitVecSet(result, i, bit_a && bit_b);
    }
}

void BitVecOr(BitVec *result, BitVec *a, BitVec *b) {
    ValidateBitVec(result);
    ValidateBitVec(a);
    ValidateBitVec(b);

    u64 max_len = MAX2(a->length, b->length);
    if (!BitVecResize(result, max_len)) {
        return;
    }

    for (u64 i = 0; i < max_len; i++) {
        bool bit_a = i < a->length ? BitVecGet(a, i) : false;
        bool bit_b = i < b->length ? BitVecGet(b, i) : false;
        BitVecSet(result, i, bit_a || bit_b);
    }
}

void BitVecXor(BitVec *result, BitVec *a, BitVec *b) {
    ValidateBitVec(result);
    ValidateBitVec(a);
    ValidateBitVec(b);

    u64 max_len = MAX2(a->length, b->length);
    if (!BitVecResize(result, max_len)) {
        return;
    }

    for (u64 i = 0; i < max_len; i++) {
        bool bit_a = i < a->length ? BitVecGet(a, i) : false;
        bool bit_b = i < b->length ? BitVecGet(b, i) : false;
        BitVecSet(result, i, bit_a != bit_b);
    }
}

void BitVecNot(BitVec *result, BitVec *bitvec) {
    ValidateBitVec(result);
    ValidateBitVec(bitvec);

    if (!BitVecResize(result, bitvec->length)) {
        return;
    }

    for (u64 i = 0; i < bitvec->length; i++) {
        bool bit = BitVecGet(bitvec, i);
        BitVecSet(result, i, !bit);
    }
}

// Comparison functions
bool BitVecEquals(BitVec *bv1, BitVec *bv2) {
    ValidateBitVec(bv1);
    ValidateBitVec(bv2);

    if (bv1->length != bv2->length) {
        return false;
    }

    return BitVecEqualsRange(bv1, 0, bv2, 0, bv1->length);
}

bool BitVecEqualsRange(BitVec *bv1, u64 start1, BitVec *bv2, u64 start2, u64 len) {
    ValidateBitVec(bv1);
    ValidateBitVec(bv2);

    if (start1 + len > bv1->length) {
        LOG_FATAL("Range [{}:{}] exceeds bitvector1 length {}", start1, LVAL(start1 + len - 1), bv1->length);
    }
    if (start2 + len > bv2->length) {
        LOG_FATAL("Range [{}:{}] exceeds bitvector2 length {}", start2, LVAL(start2 + len - 1), bv2->length);
    }

    for (u64 i = 0; i < len; i++) {
        if (BitVecGet(bv1, start1 + i) != BitVecGet(bv2, start2 + i)) {
            return false;
        }
    }

    return true;
}

int BitVecCompare(BitVec *bv1, BitVec *bv2) {
    ValidateBitVec(bv1);
    ValidateBitVec(bv2);

    u64 min_len = MIN2(bv1->length, bv2->length);

    // Compare common bits using range comparison
    int range_result = BitVecCompareRange(bv1, 0, bv2, 0, min_len);
    if (range_result != 0) {
        return range_result;
    }

    // If all compared bits are equal, compare lengths
    if (bv1->length < bv2->length) {
        return -1;
    } else if (bv1->length > bv2->length) {
        return 1;
    } else {
        return 0;
    }
}

int BitVecCompareRange(BitVec *bv1, u64 start1, BitVec *bv2, u64 start2, u64 len) {
    ValidateBitVec(bv1);
    ValidateBitVec(bv2);

    if (start1 + len > bv1->length) {
        LOG_FATAL("Range [{}:{}] exceeds bitvector1 length {}", start1, LVAL(start1 + len - 1), bv1->length);
    }
    if (start2 + len > bv2->length) {
        LOG_FATAL("Range [{}:{}] exceeds bitvector2 length {}", start2, LVAL(start2 + len - 1), bv2->length);
    }

    // Compare bit by bit in the specified range
    for (u64 i = 0; i < len; i++) {
        bool bit1 = BitVecGet(bv1, start1 + i);
        bool bit2 = BitVecGet(bv2, start2 + i);

        if (bit1 != bit2) {
            return bit1 ? 1 : -1; // true > false
        }
    }

    return 0; // All bits in range are equal
}



int BitVecNumericalCompare(BitVec *bv1, BitVec *bv2) {
    ValidateBitVec(bv1);
    ValidateBitVec(bv2);

    // Compare as unsigned integers (LSB first)
    // Start from the most significant bit (highest index)
    u64 max_len = MAX2(bv1->length, bv2->length);

    for (u64 i = max_len; i > 0;) {
        i--;
        bool bit1 = i < bv1->length ? BitVecGet(bv1, i) : false;
        bool bit2 = i < bv2->length ? BitVecGet(bv2, i) : false;

        if (bit1 != bit2) {
            return bit1 ? 1 : -1;
        }
    }

    return 0; // Equal
}

int BitVecWeightCompare(BitVec *bv1, BitVec *bv2) {
    ValidateBitVec(bv1);
    ValidateBitVec(bv2);

    u64 weight1 = BitVecCountOnes(bv1);
    u64 weight2 = BitVecCountOnes(bv2);

    if (weight1 < weight2) {
        return -1;
    } else if (weight1 > weight2) {
        return 1;
    } else {
        return 0;
    }
}

int BitVecSignedCompare(BitVec *bv1, BitVec *bv2) {
    ValidateBitVec(bv1);
    ValidateBitVec(bv2);

    if (bv1->length == 0 && bv2->length == 0) {
        return 0;
    }

    // Get sign bits (MSB)
    bool sign1 = bv1->length > 0 ? BitVecGet(bv1, bv1->length - 1) : false;
    bool sign2 = bv2->length > 0 ? BitVecGet(bv2, bv2->length - 1) : false;

    // If signs differ, negative < positive
    if (sign1 != sign2) {
        return sign1 ? -1 : 1; // true = negative, false = positive
    }

    // Same sign, compare numerically
    int result = BitVecNumericalCompare(bv1, bv2);

    // If both negative, reverse the comparison
    if (sign1) {
        result = -result;
    }

    return result;
}

bool BitVecIsSubset(BitVec *bv1, BitVec *bv2) {
    ValidateBitVec(bv1);
    ValidateBitVec(bv2);

    // bv1 is subset of bv2 if all 1-bits in bv1 are also 1-bits in bv2
    u64 max_len = MAX2(bv1->length, bv2->length);

    for (u64 i = 0; i < max_len; i++) {
        bool bit1 = i < bv1->length ? BitVecGet(bv1, i) : false;
        bool bit2 = i < bv2->length ? BitVecGet(bv2, i) : false;

        // If bv1 has a 1-bit where bv2 has a 0-bit, bv1 is not a subset
        if (bit1 && !bit2) {
            return false;
        }
    }

    return true;
}

bool BitVecIsSuperset(BitVec *bv1, BitVec *bv2) {
    return BitVecIsSubset(bv2, bv1);
}

bool BitVecDisjoint(BitVec *bv1, BitVec *bv2) {
    ValidateBitVec(bv1);
    ValidateBitVec(bv2);

    // Disjoint if no common 1-bits
    u64 min_len = MIN2(bv1->length, bv2->length);

    for (u64 i = 0; i < min_len; i++) {
        if (BitVecGet(bv1, i) && BitVecGet(bv2, i)) {
            return false; // Found common 1-bit
        }
    }

    return true;
}

bool BitVecOverlaps(BitVec *bv1, BitVec *bv2) {
    return !BitVecDisjoint(bv1, bv2);
}




bool BitVecIsSorted(BitVec *bv) {
    ValidateBitVec(bv);

    // Sorted means all 0s come before all 1s
    bool found_one = false;

    for (u64 i = 0; i < bv->length; i++) {
        bool bit = BitVecGet(bv, i);

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
Allocator *BitVecGetAllocator(BitVec *bv) {
    ValidateBitVec(bv);
    return bv->allocator;
}

bool bitvec_try_to_str(Str *out, BitVec *bv, Allocator *alloc) {
    ValidateBitVec(bv);
    if (!out) {
        LOG_FATAL("out is NULL");
    }

    *out = StrInit(alloc);
    if (bv->length == 0) {
        return true;
    }

    // Reserve space for the string (length + null terminator)
    if (!StrReserve(out, bv->length + 1)) {
        return false;
    }

    // Convert each bit to '0' or '1'
    for (u64 i = 0; i < bv->length; i++) {
        char bit_char = BitVecGet(bv, i) ? '1' : '0';
        if (!StrPushBack(out, bit_char)) {
            StrDeinit(out);
            *out = StrInit(alloc);
            return false;
        }
    }

    return true;
}

Str bitvec_to_str(BitVec *bv, Allocator *alloc) {
    Str result;

    if (!bitvec_try_to_str(&result, bv, alloc)) {
        result = StrInit(alloc);
    }

    return result;
}

static bool bitvec_try_from_str_impl(BitVec *out, const char *str, u64 str_len, Allocator *alloc) {
    if (!str) {
        LOG_FATAL("str is NULL");
    }
    if (!out) {
        LOG_FATAL("out is NULL");
    }

    *out = BitVecInit(alloc);

    if (!BitVecReserve(out, str_len)) {
        return false;
    }

    for (u64 i = 0; i < str_len; i++) {
        if (str[i] == '1') {
            if (!BitVecPush(out, true)) {
                BitVecDeinit(out);
                *out = BitVecInit(alloc);
                return false;
            }
        } else if (str[i] == '0') {
            if (!BitVecPush(out, false)) {
                BitVecDeinit(out);
                *out = BitVecInit(alloc);
                return false;
            }
        }
        // Ignore other characters
    }

    return true;
}

bool bitvec_try_from_str_zstr(BitVec *out, Zstr str, Allocator *alloc) {
    if (!str) {
        LOG_FATAL("str is NULL");
    }
    return bitvec_try_from_str_impl(out, str, (u64)ZstrLen(str), alloc);
}

bool bitvec_try_from_str_str(BitVec *out, const Str *str, Allocator *alloc) {
    if (!str) {
        LOG_FATAL("str is NULL");
    }
    return bitvec_try_from_str_impl(out, str->data, str->length, alloc);
}

BitVec bitvec_from_str_zstr(Zstr str, Allocator *alloc) {
    BitVec result;

    if (!bitvec_try_from_str_zstr(&result, str, alloc)) {
        result = BitVecInit(alloc);
    }

    return result;
}

BitVec bitvec_from_str_str(const Str *str, Allocator *alloc) {
    BitVec result;

    if (!bitvec_try_from_str_str(&result, str, alloc)) {
        result = BitVecInit(alloc);
    }

    return result;
}

u64 BitVecToBytes(BitVec *bv, u8 *bytes, u64 max_len) {
    ValidateBitVec(bv);
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
        if (BitVecGet(bv, i)) {
            u64 byte_idx     = i / 8;
            u64 bit_offset   = i % 8;
            bytes[byte_idx] |= (1u << bit_offset);
        }
    }

    return bytes_to_copy;
}

bool bitvec_try_from_bytes(BitVec *out, const u8 *bytes, u64 bit_len, Allocator *alloc) {
    if (!bytes) {
        LOG_FATAL("bytes is NULL");
    }
    if (!out) {
        LOG_FATAL("out is NULL");
    }

    *out = BitVecInit(alloc);

    // Handle empty bitvector case
    if (bit_len == 0) {
        return true;
    }

    if (!BitVecReserve(out, bit_len) || !BitVecResize(out, bit_len)) {
        BitVecDeinit(out);
        *out = BitVecInit(alloc);
        return false;
    }

    // Copy bits from bytes
    for (u64 i = 0; i < bit_len; i++) {
        u64  byte_idx   = i / 8;
        u64  bit_offset = i % 8;
        bool bit        = (bytes[byte_idx] & (1u << bit_offset)) != 0;
        BitVecSet(out, i, bit);
    }

    return true;
}

BitVec bitvec_from_bytes(const u8 *bytes, u64 bit_len, Allocator *alloc) {
    BitVec result;

    if (!bitvec_try_from_bytes(&result, bytes, bit_len, alloc)) {
        result = BitVecInit(alloc);
    }

    return result;
}

u64 BitVecToInteger(BitVec *bv) {
    ValidateBitVec(bv);
    if (bv->length == 0) {
        return 0;
    }

    u64 result   = 0;
    u64 max_bits = bv->length < 64 ? bv->length : 64; // Limit to 64 bits

    // Convert bits to integer (LSB first)
    for (u64 i = 0; i < max_bits; i++) {
        if (BitVecGet(bv, i)) {
            result |= (1ULL << i);
        }
    }

    return result;
}

bool bitvec_try_from_integer(BitVec *out, u64 value, u64 bits, Allocator *alloc) {
    if (!out) {
        LOG_FATAL("out is NULL");
    }

    *out = BitVecInit(alloc);
    if (bits == 0) {
        return true;
    }

    // Clamp to 64 bits maximum
    if (bits > 64) {
        bits = 64;
    }

    if (!BitVecReserve(out, bits) || !BitVecResize(out, bits)) {
        BitVecDeinit(out);
        *out = BitVecInit(alloc);
        return false;
    }

    // Convert integer to bits (LSB first)
    for (u64 i = 0; i < bits; i++) {
        bool bit = (value & (1ULL << i)) != 0;
        BitVecSet(out, i, bit);
    }

    return true;
}

BitVec bitvec_from_integer(u64 value, u64 bits, Allocator *alloc) {
    BitVec result;

    if (!bitvec_try_from_integer(&result, value, bits, alloc)) {
        result = BitVecInit(alloc);
    }

    return result;
}

// Shift operations
void BitVecShiftLeft(BitVec *bv, u64 positions) {
    ValidateBitVec(bv);
    if (positions == 0 || bv->length == 0) {
        return;
    }

    if (positions >= bv->length) {
        // Shift everything out
        BitVecClear(bv);
        return;
    }

    // Shift bits left (move bits to higher indices)
    for (u64 i = bv->length - 1; i >= positions; i--) {
        bool bit = BitVecGet(bv, i - positions);
        BitVecSet(bv, i, bit);
    }

    // Clear the leftmost bits (lower indices)
    for (u64 i = 0; i < positions; i++) {
        BitVecSet(bv, i, false);
    }
}

void BitVecShiftRight(BitVec *bv, u64 positions) {
    ValidateBitVec(bv);
    if (positions == 0 || bv->length == 0) {
        return;
    }

    if (positions >= bv->length) {
        // Shift everything out
        BitVecClear(bv);
        return;
    }

    // Shift bits right (move bits to lower indices)
    for (u64 i = 0; i < bv->length - positions; i++) {
        bool bit = BitVecGet(bv, i + positions);
        BitVecSet(bv, i, bit);
    }

    // Clear the high-index bits (rightmost bits that are now empty)
    for (u64 i = bv->length - positions; i < bv->length; i++) {
        BitVecSet(bv, i, false);
    }
}

void BitVecRotateLeft(BitVec *bv, u64 positions) {
    ValidateBitVec(bv);
    if (positions == 0 || bv->length == 0) {
        return;
    }

    positions = positions % bv->length; // Handle positions > length
    if (positions == 0) {
        return;
    }

    // Create a temporary copy
    BitVec temp = BitVecClone(bv);
    if (temp.length != bv->length) {
        BitVecDeinit(&temp);
        return;
    }

    // Rotate left
    for (u64 i = 0; i < bv->length; i++) {
        u64  src_idx = (i + positions) % bv->length;
        bool bit     = BitVecGet(&temp, src_idx);
        BitVecSet(bv, i, bit);
    }

    BitVecDeinit(&temp);
}

void BitVecRotateRight(BitVec *bv, u64 positions) {
    ValidateBitVec(bv);
    if (positions == 0 || bv->length == 0) {
        return;
    }

    positions = positions % bv->length; // Handle positions > length
    if (positions == 0) {
        return;
    }

    // Create a temporary copy
    BitVec temp = BitVecClone(bv);
    if (temp.length != bv->length) {
        BitVecDeinit(&temp);
        return;
    }

    // Rotate right
    for (u64 i = 0; i < bv->length; i++) {
        u64  src_idx = (i + bv->length - positions) % bv->length;
        bool bit     = BitVecGet(&temp, src_idx);
        BitVecSet(bv, i, bit);
    }

    BitVecDeinit(&temp);
}

void BitVecReverse(BitVec *bv) {
    ValidateBitVec(bv);
    if (bv->length <= 1) {
        return;
    }

    // Swap bits from both ends
    for (u64 i = 0; i < bv->length / 2; i++) {
        u64  j     = bv->length - 1 - i;
        bool bit_i = BitVecGet(bv, i);
        bool bit_j = BitVecGet(bv, j);
        BitVecSet(bv, i, bit_j);
        BitVecSet(bv, j, bit_i);
    }
}

// Missing Access functions implementation

u64 BitVecFind(BitVec *bv, bool value) {
    ValidateBitVec(bv);

    for (u64 i = 0; i < bv->length; i++) {
        if (BitVecGet(bv, i) == value) {
            return i;
        }
    }
    return SIZE_MAX; // Not found
}

u64 BitVecFindLast(BitVec *bv, bool value) {
    ValidateBitVec(bv);

    if (bv->length == 0) {
        return SIZE_MAX;
    }

    for (u64 i = bv->length - 1; i < bv->length; i--) { // i < bv->length handles underflow
        if (BitVecGet(bv, i) == value) {
            return i;
        }
        if (i == 0)
            break;   // Prevent underflow
    }
    return SIZE_MAX; // Not found
}

bool BitVecAll(BitVec *bv, bool value) {
    ValidateBitVec(bv);

    for (u64 i = 0; i < bv->length; i++) {
        if (BitVecGet(bv, i) != value) {
            return false;
        }
    }
    return true; // All match (or empty bitvector)
}

bool BitVecAny(BitVec *bv, bool value) {
    ValidateBitVec(bv);
    return BitVecFind(bv, value) != SIZE_MAX;
}

bool BitVecNone(BitVec *bv, bool value) {
    return !BitVecAny(bv, value);
}

u64 BitVecLongestRun(BitVec *bv, bool value) {
    ValidateBitVec(bv);

    if (bv->length == 0) {
        return 0;
    }

    u64 max_run     = 0;
    u64 current_run = 0;

    for (u64 i = 0; i < bv->length; i++) {
        if (BitVecGet(bv, i) == value) {
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
u64 BitVecFindPattern(BitVec *bv, BitVec *pattern) {
    ValidateBitVec(bv);
    ValidateBitVec(pattern);

    if (pattern->length == 0 || pattern->length > bv->length) {
        return SIZE_MAX;
    }

    for (u64 i = 0; i <= bv->length - pattern->length; i++) {
        if (BitVecContainsAt(bv, pattern, i)) {
            return i;
        }
    }
    return SIZE_MAX; // Not found
}

u64 BitVecFindLastPattern(BitVec *bv, BitVec *pattern) {
    ValidateBitVec(bv);
    ValidateBitVec(pattern);

    if (pattern->length == 0 || pattern->length > bv->length) {
        return SIZE_MAX;
    }

    for (u64 i = bv->length - pattern->length; i < bv->length; i--) { // i < bv->length handles underflow
        if (BitVecContainsAt(bv, pattern, i)) {
            return i;
        }
        if (i == 0)
            break;   // Prevent underflow
    }
    return SIZE_MAX; // Not found
}

u64 bitvec_find_all_pattern_raw(BitVec *bv, BitVec *pattern, size *results, u64 max_results) {
    ValidateBitVec(bv);
    ValidateBitVec(pattern);

    if (!results || max_results == 0) {
        LOG_FATAL("results is NULL or max_results is 0");
    }

    if (pattern->length == 0 || pattern->length > bv->length) {
        return 0;
    }

    u64 found_count = 0;

    for (u64 i = 0; i <= bv->length - pattern->length && found_count < max_results; i++) {
        if (BitVecContainsAt(bv, pattern, i)) {
            results[found_count] = i;
            found_count++;
        }
    }

    return found_count;
}

bool bitvec_find_all_pattern_vec(BitVec *bv, BitVec *pattern, BitVecMatchIndices *out) {
    ValidateBitVec(bv);
    ValidateBitVec(pattern);

    if (!out || !out->allocator) {
        LOG_FATAL("output BitVecMatchIndices is NULL or uninitialized");
    }

    if (pattern->length == 0 || pattern->length > bv->length)
        return true;

    for (u64 i = 0; i <= bv->length - pattern->length; i++) {
        if (BitVecContainsAt(bv, pattern, i)) {
            size idx = (size)i;
            if (!VecPushBackR(out, idx))
                return false;
        }
    }
    return true;
}

// Foreach functions

u64 bitvec_run_lengths_raw(BitVec *bv, u64 *runs, bool *values, u64 max_runs) {
    ValidateBitVec(bv);
    if (!runs || !values || max_runs == 0) {
        LOG_FATAL("invalid arguments");
    }

    if (bv->length == 0) {
        return 0;
    }

    u64  run_count          = 0;
    u64  current_run_length = 1;
    bool current_value      = BitVecGet(bv, 0);

    for (u64 i = 1; i < bv->length; i++) {
        bool bit = BitVecGet(bv, i);
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

bool bitvec_run_lengths_vec(BitVec *bv, BitVecRuns *out) {
    ValidateBitVec(bv);
    if (!out || !out->allocator) {
        LOG_FATAL("output BitVecRuns is NULL or uninitialized");
    }

    if (bv->length == 0)
        return true;

    u64  current_run_length = 1;
    bool current_value      = BitVecGet(bv, 0);

    for (u64 i = 1; i < bv->length; i++) {
        bool bit = BitVecGet(bv, i);
        if (bit == current_value) {
            current_run_length++;
        } else {
            BitVecRun r = {.length = current_run_length, .value = current_value};
            if (!VecPushBackR(out, r))
                return false;
            current_value      = bit;
            current_run_length = 1;
        }
    }
    BitVecRun r = {.length = current_run_length, .value = current_value};
    if (!VecPushBackR(out, r))
        return false;
    return true;
}

// Math functions implementation

u64 BitVecHammingDistance(BitVec *bv1, BitVec *bv2) {
    ValidateBitVec(bv1);
    ValidateBitVec(bv2);

    u64 min_length = MIN2(bv1->length, bv2->length);
    u64 max_length = MAX2(bv1->length, bv2->length);
    u64 distance   = 0;

    // Count differences in overlapping region
    for (u64 i = 0; i < min_length; i++) {
        if (BitVecGet(bv1, i) != BitVecGet(bv2, i)) {
            distance++;
        }
    }

    // Add length difference as extra distance
    distance += (max_length - min_length);

    return distance;
}

double BitVecJaccardSimilarity(BitVec *bv1, BitVec *bv2) {
    ValidateBitVec(bv1);
    ValidateBitVec(bv2);

    if (bv1->length == 0 && bv2->length == 0) {
        return 1.0; // Both empty, consider identical
    }

    // Use BitVecDotProduct for intersection calculation
    u64 intersection = BitVecDotProduct(bv1, bv2);

    // Calculate union count
    u64 max_length  = MAX2(bv1->length, bv2->length);
    u64 union_count = 0;

    for (u64 i = 0; i < max_length; i++) {
        bool bit1 = (i < bv1->length) ? BitVecGet(bv1, i) : false;
        bool bit2 = (i < bv2->length) ? BitVecGet(bv2, i) : false;

        if (bit1 || bit2) {
            union_count++;
        }
    }

    return union_count == 0 ? 1.0 : (double)intersection / (double)union_count;
}

double BitVecCosineSimilarity(BitVec *bv1, BitVec *bv2) {
    ValidateBitVec(bv1);
    ValidateBitVec(bv2);

    u64 dot_product = BitVecDotProduct(bv1, bv2);
    u64 ones1       = BitVecCountOnes(bv1);
    u64 ones2       = BitVecCountOnes(bv2);

    if (ones1 == 0 || ones2 == 0) {
        return 0.0;
    }

    double magnitude1 = sqrt_f64((double)ones1);
    double magnitude2 = sqrt_f64((double)ones2);

    return (double)dot_product / (magnitude1 * magnitude2);
}

u64 BitVecDotProduct(BitVec *bv1, BitVec *bv2) {
    ValidateBitVec(bv1);
    ValidateBitVec(bv2);

    u64 min_length = MIN2(bv1->length, bv2->length);
    u64 product    = 0;

    for (u64 i = 0; i < min_length; i++) {
        if (BitVecGet(bv1, i) && BitVecGet(bv2, i)) {
            product++;
        }
    }

    return product;
}

bool BitVecTryEditDistance(BitVec *bv1, BitVec *bv2, u64 *out) {
    ValidateBitVec(bv1);
    ValidateBitVec(bv2);
    if (!out) {
        LOG_FATAL("out is NULL");
    }

    u64 len1 = bv1->length;
    u64 len2 = bv2->length;

    if (len1 == 0) {
        *out = len2;
        return true;
    }
    if (len2 == 0) {
        *out = len1;
        return true;
    }

    // Scratch allocations borrow bv1's allocator. The function is
    // pass-through - results live in `*out` (a u64) and no container
    // escapes - so any allocator that outlives the function works.
    Allocator *scratch = bv1->allocator;

    // Dynamic programming matrix
    u64 *prev_row = AllocatorAlloc(scratch, (len2 + 1) * sizeof(u64), false);
    u64 *curr_row = AllocatorAlloc(scratch, (len2 + 1) * sizeof(u64), false);

    if (!prev_row || !curr_row) {
        AllocatorFree(scratch, prev_row);
        AllocatorFree(scratch, curr_row);
        return false;
    }

    // Initialize first row
    for (u64 j = 0; j <= len2; j++) {
        prev_row[j] = j;
    }

    for (u64 i = 1; i <= len1; i++) {
        curr_row[0] = i;

        for (u64 j = 1; j <= len2; j++) {
            u64 cost = BitVecGet(bv1, i - 1) == BitVecGet(bv2, j - 1) ? 0 : 1;

            u64 deletion     = prev_row[j] + 1;
            u64 insertion    = curr_row[j - 1] + 1;
            u64 substitution = prev_row[j - 1] + cost;

            curr_row[j] = deletion < insertion ? deletion : insertion;
            curr_row[j] = curr_row[j] < substitution ? curr_row[j] : substitution;
        }

        // Swap rows
        u64 *temp = prev_row;
        prev_row  = curr_row;
        curr_row  = temp;
    }

    *out = prev_row[len2];
    AllocatorFree(scratch, prev_row);
    AllocatorFree(scratch, curr_row);

    return true;
}

u64 BitVecEditDistanceWithError(BitVec *bv1, BitVec *bv2, bool *error) {
    u64  result = 0;
    bool ok     = BitVecTryEditDistance(bv1, bv2, &result);

    if (error) {
        *error = !ok;
    }

    return ok ? result : 0;
}

double BitVecCorrelation(BitVec *bv1, BitVec *bv2) {
    ValidateBitVec(bv1);
    ValidateBitVec(bv2);

    u64 max_length = MAX2(bv1->length, bv2->length);
    if (max_length == 0)
        return 1.0;

    double sum1 = 0, sum2 = 0, sum1_sq = 0, sum2_sq = 0, sum_product = 0;

    for (u64 i = 0; i < max_length; i++) {
        double val1 = (i < bv1->length && BitVecGet(bv1, i)) ? 1.0 : 0.0;
        double val2 = (i < bv2->length && BitVecGet(bv2, i)) ? 1.0 : 0.0;

        sum1        += val1;
        sum2        += val2;
        sum1_sq     += val1 * val1;
        sum2_sq     += val2 * val2;
        sum_product += val1 * val2;
    }

    double n           = (double)max_length;
    double numerator   = n * sum_product - sum1 * sum2;
    double denominator = sqrt_f64((n * sum1_sq - sum1 * sum1) * (n * sum2_sq - sum2 * sum2));

    return denominator == 0.0 ? 0.0 : numerator / denominator;
}

double BitVecEntropy(BitVec *bv) {
    ValidateBitVec(bv);

    if (bv->length == 0)
        return 0.0;

    u64 ones  = BitVecCountOnes(bv);
    u64 zeros = bv->length - ones;

    if (ones == 0 || zeros == 0)
        return 0.0; // No entropy in uniform data

    double p1 = (double)ones / (double)bv->length;
    double p0 = (double)zeros / (double)bv->length;

    return -(p1 * log2_f64(p1) + p0 * log2_f64(p0));
}

int BitVecAlignmentScore(BitVec *bv1, BitVec *bv2, int match, int mismatch) {
    ValidateBitVec(bv1);
    ValidateBitVec(bv2);

    u64 min_length = MIN2(bv1->length, bv2->length);
    int score      = 0;

    for (u64 i = 0; i < min_length; i++) {
        if (BitVecGet(bv1, i) == BitVecGet(bv2, i)) {
            score += match;
        } else {
            score += mismatch;
        }
    }

    return score;
}

u64 BitVecBestAlignment(BitVec *bv1, BitVec *bv2) {
    ValidateBitVec(bv1);
    ValidateBitVec(bv2);

    if (bv1->length == 0 || bv2->length == 0) {
        return 0;
    }

    u64 best_offset = 0;
    i32 best_score  = INT32_MIN;

    // Try all possible alignments of bv2 against bv1
    for (u64 offset = 0; offset <= bv1->length; offset++) {
        int score   = 0;
        u64 overlap = 0;

        for (u64 i = 0; i < bv2->length && (offset + i) < bv1->length; i++) {
            if (BitVecGet(bv1, offset + i) == BitVecGet(bv2, i)) {
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

bool BitVecStartsWith(BitVec *bv, BitVec *prefix) {
    ValidateBitVec(bv);
    ValidateBitVec(prefix);

    if (prefix->length > bv->length) {
        return false;
    }

    return BitVecContainsAt(bv, prefix, 0);
}

bool BitVecEndsWith(BitVec *bv, BitVec *suffix) {
    ValidateBitVec(bv);
    ValidateBitVec(suffix);

    if (suffix->length > bv->length) {
        return false;
    }

    u64 start_pos = bv->length - suffix->length;
    return BitVecContainsAt(bv, suffix, start_pos);
}



bool BitVecContainsAt(BitVec *bv, BitVec *pattern, u64 idx) {
    ValidateBitVec(bv);
    ValidateBitVec(pattern);

    if (idx + pattern->length > bv->length) {
        return false;
    }

    for (u64 i = 0; i < pattern->length; i++) {
        if (BitVecGet(bv, idx + i) != BitVecGet(pattern, i)) {
            return false;
        }
    }

    return true;
}

u64 BitVecCountPattern(BitVec *bv, BitVec *pattern) {
    ValidateBitVec(bv);
    ValidateBitVec(pattern);

    if (pattern->length == 0 || pattern->length > bv->length) {
        return 0;
    }

    u64 count = 0;
    for (u64 i = 0; i <= bv->length - pattern->length; i++) {
        if (BitVecContainsAt(bv, pattern, i)) {
            count++;
        }
    }

    return count;
}

u64 BitVecRFindPattern(BitVec *bv, BitVec *pattern, u64 start) {
    ValidateBitVec(bv);
    ValidateBitVec(pattern);

    if (pattern->length == 0 || pattern->length > bv->length || start >= bv->length) {
        return SIZE_MAX;
    }

    u64 search_end = (start + 1 >= pattern->length) ? start + 1 - pattern->length : 0;

    for (u64 i = start + 1; i > search_end; i--) {
        u64 pos = i - 1;
        if (BitVecContainsAt(bv, pattern, pos)) {
            return pos;
        }
    }

    return SIZE_MAX;
}

bool BitVecReplace(BitVec *bv, BitVec *old_pattern, BitVec *new_pattern) {
    ValidateBitVec(bv);
    ValidateBitVec(old_pattern);
    ValidateBitVec(new_pattern);

    u64 pos = BitVecFindPattern(bv, old_pattern);
    if (pos == SIZE_MAX) {
        return false;
    }

    // Remove old pattern
    BitVecRemoveRange(bv, pos, old_pattern->length);

    // Insert new pattern
    for (u64 i = 0; i < new_pattern->length; i++) {
        if (!BitVecInsert(bv, pos + i, BitVecGet(new_pattern, i))) {
            return false;
        }
    }

    return true;
}

u64 BitVecReplaceAll(BitVec *bv, BitVec *old_pattern, BitVec *new_pattern) {
    ValidateBitVec(bv);
    ValidateBitVec(old_pattern);
    ValidateBitVec(new_pattern);

    u64 replacements = 0;
    u64 search_pos   = 0;

    while (search_pos < bv->length) {
        // Find next occurrence
        bool found     = false;
        u64  match_pos = SIZE_MAX;

        if (search_pos + old_pattern->length <= bv->length) {
            for (u64 i = search_pos; i <= bv->length - old_pattern->length; i++) {
                if (BitVecContainsAt(bv, old_pattern, i)) {
                    match_pos = i;
                    found     = true;
                    break;
                }
            }
        }

        if (!found)
            break;

        // Remove old pattern
        BitVecRemoveRange(bv, match_pos, old_pattern->length);

        // Insert new pattern
        for (u64 i = 0; i < new_pattern->length; i++) {
            if (!BitVecInsert(bv, match_pos + i, BitVecGet(new_pattern, i))) {
                return replacements;
            }
        }

        replacements++;
        search_pos = match_pos + new_pattern->length;
    }

    return replacements;
}

bool BitVecMatches(BitVec *bv, BitVec *pattern, BitVec *wildcard) {
    ValidateBitVec(bv);
    ValidateBitVec(pattern);
    ValidateBitVec(wildcard);

    if (bv->length != pattern->length || pattern->length != wildcard->length) {
        return false;
    }

    for (u64 i = 0; i < bv->length; i++) {
        if (!BitVecGet(wildcard, i)) { // Not a wildcard position
            if (BitVecGet(bv, i) != BitVecGet(pattern, i)) {
                return false;
            }
        }
    }

    return true;
}

u64 BitVecFuzzyMatch(BitVec *bv, BitVec *pattern, u64 max_errors) {
    ValidateBitVec(bv);
    ValidateBitVec(pattern);

    if (pattern->length > bv->length) {
        return SIZE_MAX;
    }

    for (u64 i = 0; i <= bv->length - pattern->length; i++) {
        u64 errors = 0;
        for (u64 j = 0; j < pattern->length; j++) {
            if (BitVecGet(bv, i + j) != BitVecGet(pattern, j)) {
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

bool bitvec_regex_match_zstr(BitVec *bv, Zstr pattern) {
    ValidateBitVec(bv);
    if (!pattern) {
        LOG_FATAL("pattern is NULL");
    }

    // Simple regex implementation for basic patterns
    // For now, support only basic patterns like "101*", "1+0", etc.
    // This is a simplified implementation

    // Convert bitvector to string for regex matching
    Str  bv_str = BitVecToStr(bv);
    bool result = false;

    // Very basic pattern matching - just check if pattern is substring
    if (ZstrFindSubstring(bv_str.data, pattern) != NULL) {
        result = true;
    }

    StrDeinit(&bv_str);
    return result;
}

bool bitvec_regex_match_str(BitVec *bv, const Str *pattern) {
    ValidateBitVec(bv);
    if (!pattern) {
        LOG_FATAL("pattern is NULL");
    }

    Str  bv_str = BitVecToStr(bv);
    bool result = false;

    if (ZstrFindSubstringN(bv_str.data, pattern->data, pattern->length) != NULL) {
        result = true;
    }

    StrDeinit(&bv_str);
    return result;
}

u64 BitVecPrefixMatch(BitVec *bv, BitVecs *patterns) {
    ValidateBitVec(bv);
    if (!patterns) {
        LOG_FATAL("invalid BitVecs object provided");
    }

    VecForeachPtrIdx(patterns, pattern, i) {
        if (BitVecStartsWith(bv, pattern)) {
            return i;
        }
    }

    return SIZE_MAX;
}

u64 BitVecSuffixMatch(BitVec *bv, BitVecs *patterns) {
    ValidateBitVec(bv);
    if (!patterns) {
        LOG_FATAL("invalid arguments");
    }

    VecForeachPtrIdx(patterns, pattern, i) {
        if (BitVecEndsWith(bv, pattern)) {
            return i;
        }
    }

    return SIZE_MAX;
}

void ValidateBitVec(const BitVec *bv) {
    if (!(bv)) {
        LOG_FATAL("Invalid bitvec object: NULL.");
    }
    if ((bv)->__magic != BITVEC_MAGIC) {
        LOG_FATAL("Invalid bitvec. Either uninitialized or curropted!");
    }
    if ((bv)->length > (bv)->capacity) {
        LOG_FATAL("Invalid bitvec object: length > capacity.");
    }
    if ((bv)->length > 0 && !(bv)->data) {
        LOG_FATAL("Invalid bitvec object: length > 0 but data is NULL.");
    }
    if ((bv)->capacity > 0 && (bv)->byte_size * 8 < (bv)->capacity) {
        LOG_FATAL("Invalid bitvec object: byte_u64 too small for capacity.");
    }
    if ((bv)->data) {
        (void)((bv)->data[0]);
    }
}
