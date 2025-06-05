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
    /// Convert bitvector to string representation.
    /// Each bit becomes '1' or '0' character. Caller must free the returned string.
    ///
    /// bv[in] : Bitvector to convert
    ///
    /// RETURNS: String representation like "10110" or empty string if error
    ///
    /// USAGE:
    ///   Str bit_string = BitVecToString(&flags);
    ///   // ... use string ...
    ///   StrDeinit(&bit_string);
    ///
    /// TAGS: BitVec, Convert, String, Export
    ///
    Str BitVecToStr(BitVec *bv);

    ///
    /// Create bitvector from string representation.
    /// String should contain only '1' and '0' characters.
    ///
    /// str[in] : String containing bit pattern like "10110"
    ///
    /// RETURNS: Bitvector representing the bit pattern
    ///
    /// USAGE:
    ///   BitVec flags = BitVecFromString("10110");
    ///   // ... use bitvector ...
    ///   BitVecDeinit(&flags);
    ///
    /// TAGS: BitVec, Convert, String, Import
    ///
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

    ///
    /// Create bitvector from byte array.
    /// Reads the specified number of bits from the byte array.
    ///
    /// bytes[in]   : Byte array containing bit data
    /// bit_len[in] : Number of bits to read from the byte array
    ///
    /// RETURNS: Bitvector containing the bits
    ///
    /// USAGE:
    ///   u8 data[] = {0xFF, 0x00, 0xAA};
    ///   BitVec flags = BitVecFromBytes(data, 20);  // Read 20 bits (needs 3 bytes)
    ///
    /// TAGS: BitVec, Convert, Bytes, Import
    ///
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

    ///
    /// Create bitvector from integer value.
    /// Creates a bitvector representing the specified number of bits from the integer.
    ///
    /// value[in] : Integer value to convert
    /// bits[in]  : Number of bits to use (1-64)
    ///
    /// RETURNS: Bitvector containing the bits
    ///
    /// USAGE:
    ///   BitVec flags = BitVecFromInteger(0xABCD, 16);  // Use 16 bits
    ///
    /// TAGS: BitVec, Convert, Integer, Import
    ///
    BitVec BitVecFromInteger(u64 value, u64 bits);

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_BITVEC_CONVERT_H