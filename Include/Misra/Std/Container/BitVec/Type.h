/// file      : std/container/bitvec/type.h
/// author    : Generated following Misra project patterns
/// This is free and unencumbered software released into the public domain.
///
/// Bit vector type definition - efficient storage for boolean values

#ifndef MISRA_STD_CONTAINER_BITVEC_TYPE_H
#define MISRA_STD_CONTAINER_BITVEC_TYPE_H

#include <Misra/Std/Container/Common.h>
#include <Misra/Types.h>
#include <Misra/Std/Container/Vec.h>

///
/// Bit vector definition.
/// This is a specialized container for efficiently storing boolean values as bits.
///
/// Each bit represents a boolean value, with 8 bits packed into each byte.
/// This provides significant memory savings over storing booleans as separate bytes.
///
/// USAGE:
///   BitVec flags;       // Bit vector for boolean flags
///
/// FIELDS:
/// - length     : Number of bits currently in bitvector (always <= capacity)
/// - capacity   : Max number of bits this bitvector can hold before doing a resize
/// - data       : Bit data stored as bytes. Don't access directly. Use BitVecGet/Set
/// - byte_size  : Size of data array in bytes
///
/// TAGS: BitVec, Bits, Boolean, Packed, Memory
///
typedef struct {
    u64       length;    // Number of bits currently in bitvector
    u64       capacity;  // Max number of bits this bitvector can hold (in bits)
    u8       *data;      // Bit data stored as bytes
    u64       byte_size; // Size of data array in bytes
    Allocator allocator;
    u64       __magic;   // private, must not be modified
} BitVec;

typedef Vec(BitVec) BitVecs;

#define MISRA_BITVEC_MAGIC MISRA_MAKE_NEW_MAGIC_VALUE("bitvectr")

///
/// Validate whether a given `BitVec` object is valid.
/// Not foolproof but will work most of the time.
/// Aborts if provided `BitVec` is not valid.
///
/// bv[in] : Pointer to `BitVec` object to validate.
///
/// SUCCESS: Continue execution, meaning given `BitVec` object is most probably valid.
/// FAILURE: `abort`
///
void ValidateBitVec(const BitVec *bv);

#endif // MISRA_STD_CONTAINER_BITVEC_TYPE_H
