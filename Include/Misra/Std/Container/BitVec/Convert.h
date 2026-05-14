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
    /// Return a copy of the allocator descriptor bound to the bitvector.
    ///
    /// bv[in] : Bitvector whose allocator should be returned.
    ///
    /// SUCCESS : Returns allocator config copied from `bv`.
    /// FAILURE : Does not return if `bv` is invalid.
    ///
    /// TAGS: BitVec, Allocator, Access
    ///
    Allocator BitVecGetAllocator(BitVec *bv);

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
    bool BitVecTryToStrAlloc(Str *out, BitVec *bv, Allocator alloc);

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
    Str BitVecToStrAlloc(BitVec *bv, Allocator alloc);

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
    bool BitVecTryFromStrAlloc(BitVec *out, const char *str, Allocator alloc);

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
    BitVec BitVecFromStrAlloc(const char *str, Allocator alloc);

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
    bool BitVecTryFromBytesAlloc(BitVec *out, const u8 *bytes, u64 bit_len, Allocator alloc);

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
    BitVec BitVecFromBytesAlloc(const u8 *bytes, u64 bit_len, Allocator alloc);

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
    bool BitVecTryFromIntegerAlloc(BitVec *out, u64 value, u64 bits, Allocator alloc);

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
    BitVec BitVecFromIntegerAlloc(u64 value, u64 bits, Allocator alloc);

#ifdef __cplusplus
}
#endif

///
/// Convert a bitvector to a string.
///
/// This public API supports both forms:
///
/// - `BitVecTryToStr(out, bv)`
/// - `BitVecTryToStr(out, bv, alloc)`
///
/// Omitting the allocator makes the output string inherit the bitvector
/// allocator configuration.
///
/// SUCCESS : Returns true and initializes `out`.
/// FAILURE : Returns false if allocation fails.
///
/// TAGS: BitVec, Convert, String, Allocator, Macro
///
#define BITVEC_TRY_TO_STR_HAS_ARGS_IMPL(_1, _2, _3, count, ...) count
#define BITVEC_TRY_TO_STR_HAS_ARGS(...)                         BITVEC_TRY_TO_STR_HAS_ARGS_IMPL(__VA_ARGS__, 3, 2, 1, 0)
#define BitVecTryToStr(...)                                     CONCAT(BitVecTryToStr_, BITVEC_TRY_TO_STR_HAS_ARGS(__VA_ARGS__))(__VA_ARGS__)
#define BitVecTryToStr_2(out, bv)                               BitVecTryToStrAlloc((out), (bv), BitVecGetAllocator((bv)))
#define BitVecTryToStr_3(out, bv, alloc)                        BitVecTryToStrAlloc((out), (bv), (alloc))

///
/// Convert a bitvector to a string.
///
/// This public API supports both forms:
///
/// - `BitVecToStr(bv)`
/// - `BitVecToStr(bv, alloc)`
///
/// Omitting the allocator makes the output string inherit the bitvector
/// allocator configuration.
///
/// SUCCESS : Returns a string containing bit characters.
/// FAILURE : Returns an empty string if allocation fails.
///
/// TAGS: BitVec, Convert, String, Allocator, Macro
///
#define BITVEC_TO_STR_HAS_ARGS_IMPL(_1, _2, count, ...) count
#define BITVEC_TO_STR_HAS_ARGS(...)                     BITVEC_TO_STR_HAS_ARGS_IMPL(__VA_ARGS__, 2, 1, 0)
#define BitVecToStr(...)                                CONCAT(BitVecToStr_, BITVEC_TO_STR_HAS_ARGS(__VA_ARGS__))(__VA_ARGS__)
#define BitVecToStr_1(bv)                               BitVecToStrAlloc((bv), BitVecGetAllocator((bv)))
#define BitVecToStr_2(bv, alloc)                        BitVecToStrAlloc((bv), (alloc))

///
/// Parse a bitvector from a null-terminated string.
///
/// This public API supports both forms:
///
/// - `BitVecTryFromStr(out, str)`
/// - `BitVecTryFromStr(out, str, alloc)`
///
/// Omitting the allocator binds the destination to `DefaultAllocator()`.
///
/// SUCCESS : Returns true and initializes `out`.
/// FAILURE : Returns false on invalid input or allocation failure.
///
/// TAGS: BitVec, Convert, String, Allocator, Macro
///
#define BITVEC_TRY_FROM_STR_HAS_ARGS_IMPL(_1, _2, _3, count, ...) count
#define BITVEC_TRY_FROM_STR_HAS_ARGS(...)                         BITVEC_TRY_FROM_STR_HAS_ARGS_IMPL(__VA_ARGS__, 3, 2, 1, 0)
#define BitVecTryFromStr(...)                                     CONCAT(BitVecTryFromStr_, BITVEC_TRY_FROM_STR_HAS_ARGS(__VA_ARGS__))(__VA_ARGS__)
#define BitVecTryFromStr_2(out, str)                              BitVecTryFromStrAlloc((out), (str), DefaultAllocator())
#define BitVecTryFromStr_3(out, str, alloc)                       BitVecTryFromStrAlloc((out), (str), (alloc))

///
/// Parse a bitvector from a null-terminated string.
///
/// This public API supports both forms:
///
/// - `BitVecFromStr(str)`
/// - `BitVecFromStr(str, alloc)`
///
/// Omitting the allocator binds the returned bitvector to `DefaultAllocator()`.
///
/// SUCCESS : Returns parsed bitvector.
/// FAILURE : Returns an empty bitvector on invalid input or allocation failure.
///
/// TAGS: BitVec, Convert, String, Allocator, Macro
///
#define BITVEC_FROM_STR_HAS_ARGS_IMPL(_1, _2, count, ...) count
#define BITVEC_FROM_STR_HAS_ARGS(...)                     BITVEC_FROM_STR_HAS_ARGS_IMPL(__VA_ARGS__, 2, 1, 0)
#define BitVecFromStr(...)                                CONCAT(BitVecFromStr_, BITVEC_FROM_STR_HAS_ARGS(__VA_ARGS__))(__VA_ARGS__)
#define BitVecFromStr_1(str)                              BitVecFromStrAlloc((str), DefaultAllocator())
#define BitVecFromStr_2(str, alloc)                       BitVecFromStrAlloc((str), (alloc))

///
/// Build a bitvector from raw bytes.
///
/// This public API supports both forms:
///
/// - `BitVecTryFromBytes(out, bytes, bit_len)`
/// - `BitVecTryFromBytes(out, bytes, bit_len, alloc)`
///
/// Omitting the allocator binds the destination to `DefaultAllocator()`.
///
/// SUCCESS : Returns true and initializes `out`.
/// FAILURE : Returns false if allocation fails.
///
/// TAGS: BitVec, Convert, Bytes, Allocator, Macro
///
#define BITVEC_TRY_FROM_BYTES_HAS_ARGS_IMPL(_1, _2, _3, _4, count, ...) count
#define BITVEC_TRY_FROM_BYTES_HAS_ARGS(...)                             BITVEC_TRY_FROM_BYTES_HAS_ARGS_IMPL(__VA_ARGS__, 4, 3, 2, 1, 0)
#define BitVecTryFromBytes(...)                                         CONCAT(BitVecTryFromBytes_, BITVEC_TRY_FROM_BYTES_HAS_ARGS(__VA_ARGS__))(__VA_ARGS__)
#define BitVecTryFromBytes_3(out, bytes, bit_len)                       BitVecTryFromBytesAlloc((out), (bytes), (bit_len), DefaultAllocator())
#define BitVecTryFromBytes_4(out, bytes, bit_len, alloc)                BitVecTryFromBytesAlloc((out), (bytes), (bit_len), (alloc))

///
/// Build a bitvector from raw bytes.
///
/// This public API supports both forms:
///
/// - `BitVecFromBytes(bytes, bit_len)`
/// - `BitVecFromBytes(bytes, bit_len, alloc)`
///
/// Omitting the allocator binds the returned bitvector to `DefaultAllocator()`.
///
/// SUCCESS : Returns initialized bitvector.
/// FAILURE : Returns an empty bitvector if allocation fails.
///
/// TAGS: BitVec, Convert, Bytes, Allocator, Macro
///
#define BITVEC_FROM_BYTES_HAS_ARGS_IMPL(_1, _2, _3, count, ...) count
#define BITVEC_FROM_BYTES_HAS_ARGS(...)                         BITVEC_FROM_BYTES_HAS_ARGS_IMPL(__VA_ARGS__, 3, 2, 1, 0)
#define BitVecFromBytes(...)                                    CONCAT(BitVecFromBytes_, BITVEC_FROM_BYTES_HAS_ARGS(__VA_ARGS__))(__VA_ARGS__)
#define BitVecFromBytes_2(bytes, bit_len)                       BitVecFromBytesAlloc((bytes), (bit_len), DefaultAllocator())
#define BitVecFromBytes_3(bytes, bit_len, alloc)                BitVecFromBytesAlloc((bytes), (bit_len), (alloc))

///
/// Build a bitvector from an integer.
///
/// This public API supports both forms:
///
/// - `BitVecTryFromInteger(out, value, bits)`
/// - `BitVecTryFromInteger(out, value, bits, alloc)`
///
/// Omitting the allocator binds the destination to `DefaultAllocator()`.
///
/// SUCCESS : Returns true and initializes `out`.
/// FAILURE : Returns false if allocation fails.
///
/// TAGS: BitVec, Convert, Integer, Allocator, Macro
///
#define BITVEC_TRY_FROM_INTEGER_HAS_ARGS_IMPL(_1, _2, _3, _4, count, ...) count
#define BITVEC_TRY_FROM_INTEGER_HAS_ARGS(...)                             BITVEC_TRY_FROM_INTEGER_HAS_ARGS_IMPL(__VA_ARGS__, 4, 3, 2, 1, 0)
#define BitVecTryFromInteger(...)                                                                                      \
    CONCAT(BitVecTryFromInteger_, BITVEC_TRY_FROM_INTEGER_HAS_ARGS(__VA_ARGS__))(__VA_ARGS__)
#define BitVecTryFromInteger_3(out, value, bits)        BitVecTryFromIntegerAlloc((out), (value), (bits), DefaultAllocator())
#define BitVecTryFromInteger_4(out, value, bits, alloc) BitVecTryFromIntegerAlloc((out), (value), (bits), (alloc))

///
/// Build a bitvector from an integer.
///
/// This public API supports both forms:
///
/// - `BitVecFromInteger(value, bits)`
/// - `BitVecFromInteger(value, bits, alloc)`
///
/// Omitting the allocator binds the returned bitvector to `DefaultAllocator()`.
///
/// SUCCESS : Returns initialized bitvector.
/// FAILURE : Returns an empty bitvector if allocation fails.
///
/// TAGS: BitVec, Convert, Integer, Allocator, Macro
///
#define BITVEC_FROM_INTEGER_HAS_ARGS_IMPL(_1, _2, _3, count, ...) count
#define BITVEC_FROM_INTEGER_HAS_ARGS(...)                         BITVEC_FROM_INTEGER_HAS_ARGS_IMPL(__VA_ARGS__, 3, 2, 1, 0)
#define BitVecFromInteger(...)                                    CONCAT(BitVecFromInteger_, BITVEC_FROM_INTEGER_HAS_ARGS(__VA_ARGS__))(__VA_ARGS__)
#define BitVecFromInteger_2(value, bits)                          BitVecFromIntegerAlloc((value), (bits), DefaultAllocator())
#define BitVecFromInteger_3(value, bits, alloc)                   BitVecFromIntegerAlloc((value), (bits), (alloc))

#endif // MISRA_STD_CONTAINER_BITVEC_CONVERT_H
