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

    bool BitVecTryToStr(Str *out, BitVec *bv);
    bool BitVecTryToStrWithAllocator(Str *out, BitVec *bv, Allocator alloc);
    Str BitVecToStrWithAllocator(BitVec *bv, Allocator alloc);
    Str BitVecToStr(BitVec *bv);

    bool BitVecTryFromStr(BitVec *out, const char *str);
    bool BitVecTryFromStrWithAllocator(BitVec *out, const char *str, Allocator alloc);
    BitVec BitVecFromStrWithAllocator(const char *str, Allocator alloc);
    BitVec BitVecFromStr(const char *str);

    ///
    /// Export bitvector to byte array.
    /// Copies the internal bit representation to the provided byte array.
    ///
    /// bv[in]     : Bitvector to export
    /// bytes[out] : Byte array to write to (must be large enough)
    /// max_len[in]: Maximum bytes to write
    ///
    /// RETURNS: Number of bytes written
    ///
    /// USAGE:
    ///   u8 buffer[16];
    ///   u64 written = BitVecToBytes(&flags, buffer, 16);
    ///
    /// TAGS: BitVec, Convert, Bytes, Export
    ///
    u64 BitVecToBytes(BitVec *bv, u8 *bytes, u64 max_len);

    bool BitVecTryFromBytes(BitVec *out, const u8 *bytes, u64 bit_len);
    bool BitVecTryFromBytesWithAllocator(BitVec *out, const u8 *bytes, u64 bit_len, Allocator alloc);
    BitVec BitVecFromBytesWithAllocator(const u8 *bytes, u64 bit_len, Allocator alloc);
    BitVec BitVecFromBytes(const u8 *bytes, u64 bit_len);

    ///
    /// Convert bitvector to integer (up to 64 bits).
    /// Treats the bitvector as an unsigned integer with LSB first.
    ///
    /// bv[in] : Bitvector to convert (must be <= 64 bits)
    ///
    /// RETURNS: Integer value, or 0 if bitvector is too large or empty
    ///
    /// USAGE:
    ///   u64 value = BitVecToInteger(&flags);
    ///
    /// TAGS: BitVec, Convert, Integer, Export
    ///
    u64 BitVecToInteger(BitVec *bv);

    bool BitVecTryFromInteger(BitVec *out, u64 value, u64 bits);
    bool BitVecTryFromIntegerWithAllocator(BitVec *out, u64 value, u64 bits, Allocator alloc);
    BitVec BitVecFromIntegerWithAllocator(u64 value, u64 bits, Allocator alloc);
    BitVec BitVecFromInteger(u64 value, u64 bits);

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_BITVEC_CONVERT_H
