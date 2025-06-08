/// file      : std/container/Bits/type.h
/// author    : Generated following Misra project patterns
/// This is free and unencumbered software released into the public domain.
///
/// Bit vector type definition - efficient storage for boolean values

#ifndef MISRA_STD_CONTAINER_Bits_TYPE_H
#define MISRA_STD_CONTAINER_Bits_TYPE_H

#include <Misra/Std/Container/Common.h>
#include <Misra/Types.h>
#include <Misra/Std/Log.h>

///
/// Bit vector definition.
/// This is a specialized container for efficiently storing boolean values as bits.
///
/// Each bit represents a boolean value, with 8 bits packed into each byte.
/// This provides significant memory savings over storing booleans as separate bytes.
///
/// USAGE:
///   Bits flags;       // Bit vector for boolean flags
///
/// FIELDS:
/// - length     : Number of bits currently in Bitstor (always <= capacity)
/// - capacity   : Max number of bits this Bitstor can hold before doing a resize
/// - data       : Bit data stored as bytes. Don't access directly. Use BitsGet/Set
/// - byte_size  : Size of data array in bytes
///
/// TAGS: Bits, Bits, Boolean, Packed, Memory
///
typedef struct {
    u64 length;    // Number of bits currently in Bitstor
    u64 capacity;  // Max number of bits this Bitstor can hold (in bits)
    u8 *data;      // Bit data stored as bytes
    u64 byte_size; // Size of data array in bytes
} Bits;

///
/// Validate whether a given `Bits` object is valid.
/// Not foolproof but will work most of the time.
/// Aborts if provided `Bits` is not valid.
///
/// bv[in] : Pointer to `Bits` object to validate.
///
/// SUCCESS : Continue execution, meaning given `Bits` object is most probably valid.
/// FAILURE : `abort`
///
#define ValidateBits(bv)                                                                                               \
    do {                                                                                                               \
        if (!(bv)) {                                                                                                   \
            LOG_FATAL("Invalid Bits object: NULL.");                                                                   \
        }                                                                                                              \
        if ((bv)->capacity > 0 && (bv)->capacity < 256) {                                                              \
            LOG_FATAL("INvalid Bits object: When not zero, capacity must always be greater than 256 bits!");           \
        }                                                                                                              \
        if ((bv)->length > (bv)->capacity) {                                                                           \
            LOG_FATAL("Invalid Bits object: length > capacity.");                                                      \
        }                                                                                                              \
        if ((bv)->length > 0 && !(bv)->data) {                                                                         \
            LOG_FATAL("Invalid Bits object: length > 0 but data is NULL.");                                            \
        }                                                                                                              \
        if ((bv)->capacity > 0 && (bv)->byte_size * 8 < (bv)->capacity) {                                              \
            LOG_FATAL("Invalid Bits object: byte_u64 too small for capacity.");                                        \
        }                                                                                                              \
        if ((bv)->data) {                                                                                              \
            (void)((bv)->data[0]);                                                                                     \
        }                                                                                                              \
    } while (0)

#endif // MISRA_STD_CONTAINER_Bits_TYPE_H
