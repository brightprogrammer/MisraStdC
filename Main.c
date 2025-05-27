#include <Misra/Std.h>

// libc
#include <stdlib.h>
#include "Misra/Std/Container/Vec/Type.h"

///
/// Arbitrary length integer implementation.
/// Length in bits of these integer can increase dynamically
/// as the operation demands.
///
/// Integers are always signed. We allocate one bit more than
/// the required number of bits to store the signed bit information.
/// If this bit is flagged then the integer is negative, otherwise
/// positive. This is just how integer types work as well, except
/// that they store the signed information in same 64 bits storage
/// area, but for `Int` if you create storage for 64 bits then
/// it'll actually create space for 65 bits, were the 65th bit (index 64)
/// will be the sign bit.
///
/// Even though `Int` objects are stored with a length field, it
/// only is to remember the storage size allocated. Theoretically
/// these must be treated as of infinite length. Meaning operations
/// will never fail to compute something due to `Int` length constraints.
///
/// FIELDS:
/// - length : Length in bits of integer
/// - data   : Integer data
///
typedef struct {
    size length;
    u8*  data;
} Int;

#define ValidateInt(z)                                                                                                 \
    do {                                                                                                               \
        if (!IntLength(z)) {                                                                                           \
            LOG_FATAL("Non-zero Int length expected");                                                                 \
        }                                                                                                              \
    } while (0)

///
/// Initialize a new integer of given length.
///
/// Initially no storage space is allocated for storing `Int` data.
/// This state is treated as `ZERO` for faster manipulation.
/// The state is also `ZERO` when all bits are set to 0 (exept the sign-bit).
///
/// N[in] : Length of integer in bits.
///
#define IntInit(N) {.length = N, .data = NULL}

///
/// Get length in bits of given integer `z`
///
/// z[in] : `Int` to get length in bits of.
///
/// SUCCESS : Length in bits.
/// FAILURE : Only fails when `z` is invalid, in which case it's UB,
///           or an abort in best case.
///
#define IntLength(z) ((size)((z)->length))

// NOTE: above is just a trick to make the expression non assignable

///
/// Size if integer in bytes.
///
/// z[in] : Integer to get size of.
///
/// SUCCESS : Size of `Int` `z` in bits.
/// FAILURE : Only fails when `z` is invalid, in which case it's UB,
///           or an abort in best case.
///
#define IntSize(z) ((IntLength(z) / 8) + !!(IntLength(z) % 8))

///
/// Deinit given `Int` `z`
///
/// z[in] : `Int` object to be deinitialized.
///
/// SUCCESS : `z` de-initialized and `memset` to 0.
/// FAILURE : `abort` if `z` is invalid, indicating a bug in program.
///
void IntDeinit(Int* z) {
    ValidateInt(z);

    if (z->data) {
        FREE(z);
    }
    memset(z, 0, sizeof(Int));
}

///
/// Get bit at specific index. If index exceeds `Int` length,
/// then simply return 0, emulating the property of infinite length.
///
/// z[in]   : `Int` object to get bit from.
/// idx[in] : Index of bit to get.
///
/// SUCCESS : `true`/`false` depending on whether the bit at given index is flagged.
/// FAILURE : If `z` is `NULL` then results in an abort, indicating a bug in program.
///
bool IntBit(Int* z, size idx) {
    ValidateInt(z);

    // zero-state or index greater than integer bit length
    if (idx >= IntLength(z) || !z->data) {
        return 0;
    }

    size q = idx / 8;
    size r = idx % 8;

    if (r) {
        return (z->data[q + !!r - 1] >> (r - 1)) & 1;
    } else {
        return z->data[q + !!r] & 1;
    }
}

///
/// Fetch most-significant-bit (MSB) of `Int` object.
/// MSB is not the same as sign-bit.
/// MSB is bit at index `z->length - 1`
///
/// z[in] : `Int` object to fetch MSB of.
///
/// SUCCESS : `true`/`false` depending on whether the bit is flagged.
/// FAILURE : If `z` is `NULL` then results in an abort.
///
bool IntMSB(Int* z) {
    ValidateInt(z);
    return IntBit(z, z->length - 1);
}

///
/// Fetch least-significant-bit (LSB) of `Int` object.
/// This is the bit at index 0.
///
/// z[in] : `Int` object to fetch LSB of.
///
/// SUCCESS : `true`/`false` depending on whether the bit is flagged.
/// FAILURE : If `z` is `NULL` then results in an abort.
///
bool IntLSB(Int* z) {
    ValidateInt(z);
    return IntBit(z, 0);
}

///
/// Fetch sign-bit (LSB) of `Int` object.
/// This is the bit at index `z->length`.
///
/// z[in] : `Int` object to fetch LSB of.
///
/// SUCCESS : `true`/`false` depending on whether the bit is flagged.
/// FAILURE : If `z` is `NULL` then results in an abort.
///
bool IntSign(Int* z) {
    ValidateInt(z);
    return IntBit(z, z->length);
}

///
/// Resize an `Int` object to increase it's number holding
/// capacity/range.
///
/// The resize operation will happen only if `num_bits` is greater
/// than current `Int` length.
///
/// If `num_bits` is 0, then it means resize vector to it's own
/// length. This is used when reserving space for vector first time
/// it leaves the `ZERO` state.
///
/// z[in,out]    : `Int` object to resize.
/// num_bits[in] : Number of bits to resize to. Can be 0.
///
/// SUCCESS : `Int` object resized to new `num_bits` length.
/// FAILURE : No resize operation performed. `abort` is called if given integer is invalid.
///
void IntResize(Int* z, size num_bits) {
    ValidateInt(z);

    // resize to same length
    if ((num_bits < IntLength(z)) && !z->data) {
        num_bits = IntLength(z);
    }

    // resize if new size is greater than current size
    // or if we're initializing a `ZERO` state `Int` object.
    if (IntLength(z) < num_bits || (!z->data && (IntLength(z) == num_bits))) {
        // store sign bit bc we might have to shift it lateron
        bool sign = false;
        if (z->data) {
            sign = IntSign(z);
        }

        // resize
        size s = (num_bits + 1) / 8 + !!((num_bits + 1) % 8);
        u8*  p = realloc(z->data, s);
        if (p) {
            // unset new bits
            memset(p + (z->data ? IntSize(z) : 0), 0, s - (z->data ? IntSize(z) : 0));

            z->data = p;

            // shift sign bit
            if (sign) {
                // unset old sign bit
                size q      = (IntLength(z) + 1) / 8;
                size r      = (IntLength(z) + 1) % 8;
                z->data[q] &= ~(1 << r);

                // set new sign bit
                q           = (num_bits + 1) / 8;
                r           = (num_bits + 1) % 8;
                z->data[q] |= (1 << r);
            }

            z->length = num_bits;
        }
    }
}

///
/// Set bit at given index to 1.
///
/// z[in]   : `Int` object to set bit into.
/// idx[in] : Index of bit to set.
///
/// SUCCESS : Bit at given index set to 0.
/// FAILURE : If `z` is `NULL` then results in an abort, indicating a bug in program.
///
void IntSetBit(Int* z, size idx) {
    ValidateInt(z);

    // have enough storage to store at least bit at index `idx`
    IntResize(z, idx + 1);

    size q = idx / 8;
    size r = idx % 8;

    z->data[q] |= (1 << r);
}

///
/// Set bit at given index to 0.
///
/// z[in]   : `Int` object to set bit into.
/// idx[in] : Index of bit to set.
///
/// SUCCESS : Bit at given index set to 0.
/// FAILURE : If `z` is `NULL` then results in an abort, indicating a bug in program.
///
void IntUnsetBit(Int* z, size idx) {
    ValidateInt(z);

    // have enough storage to store at least bit at index `idx`
    IntResize(z, idx + 1);

    size q = idx / 8;
    size r = idx % 8;

    z->data[q] &= ~(1 << r);
}

///
/// Check if given integer is zero.
///
/// z[in] : `Int` object to check whether is zero.
///
/// SUCCESS : `true`/`false` depending on whether `Int` object represents `ZERO` state or not.
/// FAILURE : If `z` is `NULL` then `abort`.
///
bool IntIsZero(Int* z) {
    ValidateInt(z);

    // ZERO‐state if no storage yet
    if (!z->data) {
        return true;
    }

    size     bitlen     = IntLength(z); // user‐visible length in bits
    size     bytelen    = IntSize(z);   // ceil((bitlen + 1) / 8)
    size     full_bytes = bitlen / 8;   // number of whole‐byte magnitude
    unsigned rem_bits   = bitlen % 8;   // leftover magnitude bits in last byte

    // Check all full bytes [0 … full_bytes-1]
    for (size i = 0; i < full_bytes; i++) {
        if (z->data[i] != 0) {
            return false;
        }
    }

    // If there are any remaining magnitude bits in the next byte, check those
    if (rem_bits) {
        // mask to exclude sign bit
        u8 mask = (1 << rem_bits) - 1;
        if ((z->data[full_bytes] & mask) != 0) {
            return false;
        }
    }

    return true;
}

///
/// Add two `Int` objects `z1` and `z2`
///
/// z1[in] : First addition operand.
/// z2[in] : Second addition operand.
///
/// SUCCESS : `Int`
///
Int IntAdd(Int* z1, Int* z2) {
    ValidateInt(z1);
    ValidateInt(z2);

    // Length of result is usually the maximum of the length of the two operands,
    // but in case of overflow during addition, we need one extra bit.
    // Overflow can be checked by taking bitwise and of bit at "index = (max-length) - 1"
    // If the result is true then there'll be an overflow of 1 bit, otherwise length is
    // just maximum out of the two.
    size rzl = MAX2(IntLength(z1), IntLength(z2));
    rzl      = rzl + IntBit(z1, rzl - 1) & IntBit(z2, rzl - 1);

    Int rz = IntInit(rzl);
    IntResize(&rz, 0);

    // TODO: add

    return rz;
}

Int IntSub(Int* z1, Int* z2) {
    ValidateInt(z1);
    ValidateInt(z2);
}

void _write_Int(Str* o, FmtInfo* fmt_info, Int* z) {
    ValidateInt(z);
    ValidateStr(o);

    if (!o || !fmt_info || !z) {
        LOG_FATAL("Invalid arguments");
    }
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    LogInit(false);

    Int z1 = IntInit(1024);
    Int z2 = IntInit(1024);

    Int r = IntAdd(&z1, &z2);

    (void)r;

    LogDeinit();
    return 0;
}
