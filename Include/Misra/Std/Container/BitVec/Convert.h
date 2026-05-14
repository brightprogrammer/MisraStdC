/// file      : std/container/bitvec/convert.h
/// author    : Generated following Misra project patterns
/// This is free and unencumbered software released into the public domain.
///
/// Conversion operations for bitvectors.

#ifndef MISRA_STD_CONTAINER_BITVEC_CONVERT_H
#define MISRA_STD_CONTAINER_BITVEC_CONVERT_H

#include "Type.h"
#include <Misra/Types.h>
#include <Misra/Std/Container/Str.h>

#ifdef __cplusplus
extern "C" {
#endif

    ///
    /// Return the allocator pointer bound to the bitvector.
    ///
    /// bv[in] : Bitvector whose allocator should be returned.
    ///
    /// SUCCESS : Returns the `Allocator *` stored on `bv`.
    /// FAILURE : Does not return if `bv` is invalid.
    ///
    /// TAGS: BitVec, Allocator, Access
    ///
    Allocator *BitVecGetAllocator(BitVec *bv);

    ///
    /// Convert a bitvector to a string using an explicit allocator.
    ///
    /// out[out] : Destination string.
    /// bv[in]   : Bitvector to convert.
    /// alloc[in]: Allocator to bind to the destination string.
    ///
    /// SUCCESS : Returns true and initializes `out`.
    /// FAILURE : Returns false if allocation fails.
    ///
    /// TAGS: BitVec, Convert, String, Allocator
    ///
    bool BitVecTryToStrAlloc(Str *out, BitVec *bv, Allocator *alloc);

    ///
    /// Convert a bitvector to a string using an explicit allocator.
    ///
    /// bv[in]   : Bitvector to convert.
    /// alloc[in]: Allocator to bind to the returned string.
    ///
    /// SUCCESS : Returns a string containing bit characters.
    /// FAILURE : Returns an empty string if allocation fails.
    ///
    /// TAGS: BitVec, Convert, String, Allocator
    ///
    Str BitVecToStrAlloc(BitVec *bv, Allocator *alloc);

    ///
    /// Parse a bitvector from a null-terminated string using an explicit allocator.
    ///
    /// out[out] : Destination bitvector.
    /// str[in]  : Input string containing `0` and `1` characters.
    /// alloc[in]: Allocator to bind to the destination bitvector.
    ///
    /// SUCCESS : Returns true and initializes `out`.
    /// FAILURE : Returns false on invalid input or allocation failure.
    ///
    /// TAGS: BitVec, Convert, String, Allocator
    ///
    bool BitVecTryFromStrAlloc(BitVec *out, const char *str, Allocator *alloc);

    ///
    /// Parse a bitvector from a null-terminated string using an explicit allocator.
    ///
    /// str[in]  : Input string containing `0` and `1` characters.
    /// alloc[in]: Allocator to bind to the returned bitvector.
    ///
    /// SUCCESS : Returns parsed bitvector.
    /// FAILURE : Returns an empty bitvector on invalid input or allocation failure.
    ///
    /// TAGS: BitVec, Convert, String, Allocator
    ///
    BitVec BitVecFromStrAlloc(const char *str, Allocator *alloc);

    ///
    /// Export bitvector to byte array.
    /// Copies the internal bit representation to the provided byte array.
    ///
    /// bv[in]     : Bitvector to export
    /// bytes[out] : Byte array to write to (must be large enough)
    /// max_len[in]: Maximum bytes to write
    ///
    /// SUCCESS : Number of bytes written
    ///
    /// USAGE:
    ///   u8 buffer[16];
    ///   u64 written = BitVecToBytes(&flags, buffer, 16);
    ///
    /// TAGS: BitVec, Convert, Bytes, Export
    ///
    u64 BitVecToBytes(BitVec *bv, u8 *bytes, u64 max_len);

    ///
    /// Build a bitvector from raw bytes using an explicit allocator.
    ///
    /// out[out]   : Destination bitvector.
    /// bytes[in]  : Source byte buffer.
    /// bit_len[in]: Number of bits to read from `bytes`.
    /// alloc[in]  : Allocator to bind to the destination bitvector.
    ///
    /// SUCCESS : Returns true and initializes `out`.
    /// FAILURE : Returns false if allocation fails.
    ///
    /// TAGS: BitVec, Convert, Bytes, Allocator
    ///
    bool BitVecTryFromBytesAlloc(BitVec *out, const u8 *bytes, u64 bit_len, Allocator *alloc);

    ///
    /// Build a bitvector from raw bytes using an explicit allocator.
    ///
    /// bytes[in]  : Source byte buffer.
    /// bit_len[in]: Number of bits to read from `bytes`.
    /// alloc[in]  : Allocator to bind to the returned bitvector.
    ///
    /// SUCCESS : Returns initialized bitvector.
    /// FAILURE : Returns an empty bitvector if allocation fails.
    ///
    /// TAGS: BitVec, Convert, Bytes, Allocator
    ///
    BitVec BitVecFromBytesAlloc(const u8 *bytes, u64 bit_len, Allocator *alloc);

    ///
    /// Convert bitvector to integer (up to 64 bits).
    /// Treats the bitvector as an unsigned integer with LSB first.
    ///
    /// bv[in] : Bitvector to convert (must be <= 64 bits)
    ///
    /// SUCCESS : Integer value, or 0 if bitvector is too large or empty
    ///
    /// USAGE:
    ///   u64 value = BitVecToInteger(&flags);
    ///
    /// TAGS: BitVec, Convert, Integer, Export
    ///
    u64 BitVecToInteger(BitVec *bv);

    ///
    /// Build a bitvector from an integer using an explicit allocator.
    ///
    /// out[out] : Destination bitvector.
    /// value[in]: Integer value to convert.
    /// bits[in] : Number of bits to emit.
    /// alloc[in]: Allocator to bind to the destination bitvector.
    ///
    /// SUCCESS : Returns true and initializes `out`.
    /// FAILURE : Returns false if allocation fails.
    ///
    /// TAGS: BitVec, Convert, Integer, Allocator
    ///
    bool BitVecTryFromIntegerAlloc(BitVec *out, u64 value, u64 bits, Allocator *alloc);

    ///
    /// Build a bitvector from an integer using an explicit allocator.
    ///
    /// value[in]: Integer value to convert.
    /// bits[in] : Number of bits to emit.
    /// alloc[in]: Allocator to bind to the returned bitvector.
    ///
    /// SUCCESS : Returns initialized bitvector.
    /// FAILURE : Returns an empty bitvector if allocation fails.
    ///
    /// TAGS: BitVec, Convert, Integer, Allocator
    ///
    BitVec BitVecFromIntegerAlloc(u64 value, u64 bits, Allocator *alloc);

#ifdef __cplusplus
}
#endif

///
/// Convert a bitvector to a string. The output string inherits the source
/// bitvector's allocator. Callers who want a different allocator should
/// call `BitVecTryToStrAlloc` directly.
///
/// out[out] : Destination string.
/// bv[in]   : Bitvector to convert.
///
/// SUCCESS : Returns true and initializes `out`.
/// FAILURE : Returns false if allocation fails.
///
/// TAGS: BitVec, Convert, String
///
#define BitVecTryToStr(out, bv) BitVecTryToStrAlloc((out), (bv), BitVecGetAllocator((bv)))

///
/// Convert a bitvector to a string. The returned string inherits the source
/// bitvector's allocator. Callers who want a different allocator should
/// call `BitVecToStrAlloc` directly.
///
/// bv[in] : Bitvector to convert.
///
/// SUCCESS : Returns a string containing bit characters.
/// FAILURE : Returns an empty string if allocation fails.
///
/// TAGS: BitVec, Convert, String
///
#define BitVecToStr(bv) BitVecToStrAlloc((bv), BitVecGetAllocator((bv)))

#endif // MISRA_STD_CONTAINER_BITVEC_CONVERT_H
