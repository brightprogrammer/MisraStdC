/// file      : std/container/bitvec/init.h
/// author    : Generated following Misra project patterns
/// This is free and unencumbered software released into the public domain.
///
/// Different types of initializers for a bitvector.

#ifndef MISRA_STD_CONTAINER_BITVEC_INIT_H
#define MISRA_STD_CONTAINER_BITVEC_INIT_H

#include "Type.h"
#include <Misra/Std/Memory.h>
#include <string.h>

// Helper macro for bit operations
#define BITVEC_BYTES_FOR_BITS(bits) (((bits) + 7) / 8)

#ifdef __cplusplus
extern "C" {
#endif

///
/// Initialize bitvector with default values.
/// It is mandatory to initialize bitvectors before use. Not doing so is undefined behaviour.
///
/// USAGE:
///   BitVec flags = BitVecInit();
///
/// TAGS: Init, BitVec, Boolean, Bits
///
#ifdef __cplusplus
#    define BitVecInit() (BitVec {.length = 0, .capacity = 0, .data = NULL, .byte_size = 0})
#else
#    define BitVecInit() ((BitVec) {.length = 0, .capacity = 0, .data = NULL, .byte_size = 0})
#endif

///
/// Initialize bitvector with initial capacity.
/// Creates a bitvector with reserved space for the specified number of bits.
///
/// cap[in] : Initial capacity in bits
///
/// USAGE:
///   BitVec flags = BitVecInitWithCapacity(64);
///
/// TAGS: Init, BitVec, Boolean, Bits, Capacity
///
#ifdef __cplusplus
#    define BitVecInitWithCapacity(cap)                                                                                \
        (BitVec {                                                                                                      \
            .length    = 0,                                                                                            \
            .capacity  = (cap),                                                                                        \
            .data      = (u8 *)calloc(BITVEC_BYTES_FOR_BITS(cap), 1),                                                  \
            .byte_size = BITVEC_BYTES_FOR_BITS(cap)                                                                    \
        })
#else
#    define BitVecInitWithCapacity(cap)                                                                                \
        ((BitVec) {.length    = 0,                                                                                     \
                   .capacity  = (cap),                                                                                 \
                   .data      = (u8 *)calloc(BITVEC_BYTES_FOR_BITS(cap), 1),                                           \
                   .byte_size = BITVEC_BYTES_FOR_BITS(cap)})
#endif



    ///
    /// Deinitialize bitvector and free all allocated memory.
    /// After calling this, the bitvector should not be used unless re-initialized.
    ///
    /// bv[in] : Pointer to bitvector to deinitialize
    ///
    /// USAGE:
    ///   BitVecDeinit(&flags);
    ///
    /// TAGS: Deinit, BitVec, Cleanup, Memory
    ///
    void BitVecDeinit(BitVec *bv);

    ///
    /// Clear all bits in bitvector without deallocating memory.
    /// Sets length to 0 but keeps allocated capacity.
    ///
    /// bv[in] : Pointer to bitvector to clear
    ///
    /// USAGE:
    ///   BitVecClear(&flags);
    ///
    /// TAGS: Clear, BitVec, Reset
    ///
    void BitVecClear(BitVec *bv);

    ///
    /// Reserve space for at least n bits in bitvector.
    /// Does not change the length, only ensures capacity.
    ///
    /// bv[in] : Bitvector to reserve space in
    /// n[in]  : Number of bits to reserve space for
    ///
    /// USAGE:
    ///   BitVecReserve(&flags, 1000);
    ///
    /// TAGS: BitVec, Reserve, Capacity, Memory
    ///
    void BitVecReserve(BitVec *bv, u64 n);

    ///
    /// Reu64 bitvector to hold exactly n bits.
    /// May grow or shrink the bitvector.
    ///
    /// bv[in] : Bitvector to resize
    /// n[in]  : New u64 in bits
    ///
    /// USAGE:
    ///   BitVecResize(&flags, 64);
    ///
    /// TAGS: BitVec, Resize, Length
    ///
    void BitVecResize(BitVec *bv, u64 n);

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_BITVEC_INIT_H