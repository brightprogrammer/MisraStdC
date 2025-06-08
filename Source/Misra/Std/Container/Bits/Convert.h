/// file      : std/container/Bits/convert.h
/// author    : Generated following Misra project patterns
/// This is free and unencumbered software released into the public domain.
///
/// Conversion operations for Bits vectors.

#ifndef MISRA_STD_CONTAINER_Bits_CONVERT_H
#define MISRA_STD_CONTAINER_Bits_CONVERT_H

#include "Type.h"
#include <Misra/Types.h>
#include <Misra/Std/Container/Str.h>

#ifdef __cplusplus
extern "C" {
#endif

    ///
    /// Convert Bits vector to string representation.
    /// Each bit becomes '1' or '0' character. Caller must free the returned string.
    ///
    /// bv[in] : Bits vector to convert
    ///
    /// RETURNS: String representation like "10110" or empty string if error
    ///
    /// USAGE:
    ///   Str bit_string = BitsToString(&flags);
    ///   // ... use string ...
    ///   StrDeinit(&bit_string);
    ///
    /// TAGS: Bits, Convert, String, Export
    ///
    Str BitsToStr(Bits *bv);

    ///
    /// Create Bits vector from string representation.
    /// String should contain only '1' and '0' characters.
    ///
    /// str[in] : String containing bit pattern like "10110"
    ///
    /// RETURNS: Bits vector representing the bit pattern
    ///
    /// USAGE:
    ///   Bits flags = BitsFromString("10110");
    ///   // ... use Bits vector ...
    ///   BitsDeinit(&flags);
    ///
    /// TAGS: Bits, Convert, String, Import
    ///
    Bits BitsFromStr(const char *str);

    ///
    /// Export Bits vector to byte array.
    /// Copies the internal bit representation to the provided byte array.
    ///
    /// bv[in]     : Bits vector to export
    /// bytes[out] : Byte array to write to (must be large enough)
    /// max_len[in]: Maximum bytes to write
    ///
    /// RETURNS: Number of bytes written
    ///
    /// USAGE:
    ///   u8 buffer[16];
    ///   u64 written = BitsToBytes(&flags, buffer, 16);
    ///
    /// TAGS: Bits, Convert, Bytes, Export
    ///
    u64 BitsToBytes(Bits *bv, u8 *bytes, u64 max_len);

    ///
    /// Create Bits vector from byte array.
    /// Reads the specified number of bits from the byte array.
    ///
    /// bytes[in]   : Byte array containing bit data
    /// bit_len[in] : Number of bits to read from the byte array
    ///
    /// RETURNS: Bits vector containing the bits
    ///
    /// USAGE:
    ///   u8 data[] = {0xFF, 0x00, 0xAA};
    ///   Bits flags = BitsFromBytes(data, 20);  // Read 20 bits (needs 3 bytes)
    ///
    /// TAGS: Bits, Convert, Bytes, Import
    ///
    Bits BitsFromBytes(const u8 *bytes, u64 bit_len);

    ///
    /// Convert Bits vector to integer (up to 64 bits).
    /// Treats the Bits vector as an unsigned integer with LSB first.
    ///
    /// bv[in] : Bits vector to convert (must be <= 64 bits)
    ///
    /// RETURNS: Integer value, or 0 if Bits vector is too large or empty
    ///
    /// USAGE:
    ///   u64 value = BitsToInteger(&flags);
    ///
    /// TAGS: Bits, Convert, Integer, Export
    ///
    u64 BitsToInteger(Bits *bv);

    ///
    /// Convert Bits vector to integer (up to 64 bits).
    /// Bits bitvector is treated in big-endian format.
    ///
    /// bv[in] : Bits vector to convert (must be <= 64 bits)
    ///
    /// RETURNS: Integer value, or 0 if Bits vector is too large or empty
    ///
    /// USAGE:
    ///   u64 value = BitsToInteger(&flags);
    ///
    /// TAGS: Bits, Convert, Integer, Export
    ///
    u64 BitsToIntegerBE(Bits *bv);

    ///
    /// Convert Bits vector to integer (up to 64 bits).
    /// Bits bitvector is treated in little-endian format.
    ///
    /// bv[in] : Bits vector to convert (must be <= 64 bits)
    ///
    /// RETURNS: Integer value, or 0 if Bits vector is too large or empty
    ///
    /// USAGE:
    ///   u64 value = BitsToInteger(&flags);
    ///
    /// TAGS: Bits, Convert, Integer, Export
    ///
    u64 BitsToIntegerLE(Bits *bv);

    ///
    /// Create Bits vector from integer value.
    /// Creates a Bits vector representing the specified number of bits from the integer.
    ///
    /// value[in] : Integer value to convert
    /// bits[in]  : Number of bits to use (1-64)
    ///
    /// RETURNS: Bits vector containing the bits
    ///
    /// USAGE:
    ///   Bits flags = BitsFromInteger(0xABCD, 16);  // Use 16 bits
    ///
    /// TAGS: Bits, Convert, Integer, Import
    ///
    Bits BitsFromInteger(u64 value, u64 bits);

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_Bits_CONVERT_H
