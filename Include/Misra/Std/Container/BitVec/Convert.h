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
    bool bitvec_try_to_str(Str *out, BitVec *bv, Allocator *alloc);

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
    Str bitvec_to_str(BitVec *bv, Allocator *alloc);

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
    bool bitvec_try_from_str_zstr(BitVec *out, Zstr str, Allocator *alloc);
    bool bitvec_try_from_str_str(BitVec *out, const Str *str, Allocator *alloc);
#define BitVecTryFromStr(...) MISRA_OVERLOAD(BitVecTryFromStr, __VA_ARGS__)
#define BitVecTryFromStr_2(out, str)                                                                                                                                 \
    _Generic((str), Str *: bitvec_try_from_str_str, char *: bitvec_try_from_str_zstr, Zstr : bitvec_try_from_str_zstr)( \
        (out),                                                                                                                                                       \
        (str),                                                                                                                                                       \
        MisraScope                                                                                                                                                   \
    )
#define BitVecTryFromStr_3(out, str, alloc)                                                                                                                          \
    _Generic((str), Str *: bitvec_try_from_str_str, char *: bitvec_try_from_str_zstr, Zstr : bitvec_try_from_str_zstr)( \
        (out),                                                                                                                                                       \
        (str),                                                                                                                                                       \
        ALLOCATOR_OF(alloc)                                                                                                                                          \
    )

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
    BitVec bitvec_from_str_zstr(Zstr str, Allocator *alloc);
    BitVec bitvec_from_str_str(const Str *str, Allocator *alloc);
#define BitVecFromStr(...) MISRA_OVERLOAD(BitVecFromStr, __VA_ARGS__)
#define BitVecFromStr_1(str)                                                                                                                         \
    _Generic((str), Str *: bitvec_from_str_str, char *: bitvec_from_str_zstr, Zstr : bitvec_from_str_zstr)( \
        (str),                                                                                                                                       \
        MisraScope                                                                                                                                   \
    )
#define BitVecFromStr_2(str, alloc)                                                                                                                  \
    _Generic((str), Str *: bitvec_from_str_str, char *: bitvec_from_str_zstr, Zstr : bitvec_from_str_zstr)( \
        (str),                                                                                                                                       \
        ALLOCATOR_OF(alloc)                                                                                                                          \
    )

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
    bool bitvec_try_from_bytes(BitVec *out, const u8 *bytes, u64 bit_len, Allocator *alloc);
#define BitVecTryFromBytes(...)                   MISRA_OVERLOAD(BitVecTryFromBytes, __VA_ARGS__)
#define BitVecTryFromBytes_3(out, bytes, bit_len) bitvec_try_from_bytes((out), (bytes), (bit_len), MisraScope)
#define BitVecTryFromBytes_4(out, bytes, bit_len, alloc)                                                               \
    bitvec_try_from_bytes((out), (bytes), (bit_len), ALLOCATOR_OF(alloc))

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
    BitVec bitvec_from_bytes(const u8 *bytes, u64 bit_len, Allocator *alloc);
#define BitVecFromBytes(...)                     MISRA_OVERLOAD(BitVecFromBytes, __VA_ARGS__)
#define BitVecFromBytes_2(bytes, bit_len)        bitvec_from_bytes((bytes), (bit_len), MisraScope)
#define BitVecFromBytes_3(bytes, bit_len, alloc) bitvec_from_bytes((bytes), (bit_len), ALLOCATOR_OF(alloc))

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
    bool bitvec_try_from_integer(BitVec *out, u64 value, u64 bits, Allocator *alloc);
#define BitVecTryFromInteger(...)                MISRA_OVERLOAD(BitVecTryFromInteger, __VA_ARGS__)
#define BitVecTryFromInteger_3(out, value, bits) bitvec_try_from_integer((out), (value), (bits), MisraScope)
#define BitVecTryFromInteger_4(out, value, bits, alloc)                                                                \
    bitvec_try_from_integer((out), (value), (bits), ALLOCATOR_OF(alloc))

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
    BitVec bitvec_from_integer(u64 value, u64 bits, Allocator *alloc);
#define BitVecFromInteger(...)                  MISRA_OVERLOAD(BitVecFromInteger, __VA_ARGS__)
#define BitVecFromInteger_2(value, bits)        bitvec_from_integer((value), (bits), MisraScope)
#define BitVecFromInteger_3(value, bits, alloc) bitvec_from_integer((value), (bits), ALLOCATOR_OF(alloc))

#ifdef __cplusplus
}
#endif

///
/// Convert a bitvector to a string. Two forms via argument count:
///
/// - `BitVecTryToStr(out, bv)`        - uses `bv`'s allocator.
/// - `BitVecTryToStr(out, bv, alloc)` - uses the explicit allocator.
///
/// SUCCESS : Returns true and initializes `out`.
/// FAILURE : Returns false if allocation fails.
///
/// TAGS: BitVec, Convert, String
///
#define BitVecTryToStr(...)              MISRA_OVERLOAD(BitVecTryToStr, __VA_ARGS__)
#define BitVecTryToStr_2(out, bv)        bitvec_try_to_str((out), (bv), BitVecGetAllocator((bv)))
#define BitVecTryToStr_3(out, bv, alloc) bitvec_try_to_str((out), (bv), (alloc))

///
/// Convert a bitvector to a string. Two forms via argument count:
///
/// - `BitVecToStr(bv)`        - uses `bv`'s allocator.
/// - `BitVecToStr(bv, alloc)` - uses the explicit allocator.
///
/// SUCCESS : Returns a string containing bit characters.
/// FAILURE : Returns an empty string if allocation fails.
///
/// TAGS: BitVec, Convert, String
///
#define BitVecToStr(...)         MISRA_OVERLOAD(BitVecToStr, __VA_ARGS__)
#define BitVecToStr_1(bv)        bitvec_to_str((bv), BitVecGetAllocator((bv)))
#define BitVecToStr_2(bv, alloc) bitvec_to_str((bv), (alloc))

#endif // MISRA_STD_CONTAINER_BITVEC_CONVERT_H
