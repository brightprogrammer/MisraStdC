/// file      : std/container/Bits/init.h
/// author    : Generated following Misra project patterns
/// This is free and unencumbered software released into the public domain.
///
/// Different types of initializers for a Bitstor.

#ifndef MISRA_STD_CONTAINER_Bits_INIT_H
#define MISRA_STD_CONTAINER_Bits_INIT_H

#include "Type.h"
#include <Misra/Std/Memory.h>
#include <string.h>

// Helper macro for bit operations
#define Bits_BYTES_FOR_BITS(bits) (((bits) + 7) / 8)

#ifdef __cplusplus
extern "C" {
#endif

///
/// Initialize Bitstor with default values.
/// It is mandatory to initialize Bitstors before use. Not doing so is undefined behaviour.
///
/// USAGE:
///   Bits flags = BitsInit();
///
/// TAGS: Init, Bits, Boolean, Bits
///
#ifdef __cplusplus
#    define BitsInit() (Bits {.length = 0, .capacity = 0, .data = NULL, .byte_size = 0})
#else
#    define BitsInit() ((Bits) {.length = 0, .capacity = 0, .data = NULL, .byte_size = 0})
#endif

///
/// Initialize Bitstor with initial capacity.
/// Creates a Bitstor with reserved space for the specified number of bits.
///
/// cap[in] : Initial capacity in bits
///
/// USAGE:
///   Bits flags = BitsInitWithCapacity(64);
///
/// TAGS: Init, Bits, Boolean, Bits, Capacity
///
#ifdef __cplusplus
#    define BitsInitWithCapacity(cap)                                                                                  \
        (Bits {                                                                                                        \
            .length    = 0,                                                                                            \
            .capacity  = (cap),                                                                                        \
            .data      = (u8 *)calloc(Bits_BYTES_FOR_BITS(MAX2((cap), 256)), 1),                                       \
            .byte_size = Bits_BYTES_FOR_BITS(MAX2((cap), 256))                                                         \
        })
#else
#    define BitsInitWithCapacity(cap)                                                                                  \
        ((Bits) {.length    = 0,                                                                                       \
                 .capacity  = (cap),                                                                                   \
                 .data      = (u8 *)calloc(Bits_BYTES_FOR_BITS(MAX2((cap), 256)), 1),                                  \
                 .byte_size = Bits_BYTES_FOR_BITS(MAX2((cap), 256))})
#endif



    ///
    /// Deinitialize Bitstor and free all allocated memory.
    /// After calling this, the Bitstor should not be used unless re-initialized.
    ///
    /// bv[in] : Pointer to Bitstor to deinitialize
    ///
    /// USAGE:
    ///   BitsDeinit(&flags);
    ///
    /// TAGS: Deinit, Bits, Cleanup, Memory
    ///
    void BitsDeinit(Bits *bv);

    ///
    /// Clear all bits in Bitstor without deallocating memory.
    /// Sets length to 0 but keeps allocated capacity.
    ///
    /// bv[in] : Pointer to Bitstor to clear
    ///
    /// USAGE:
    ///   BitsClear(&flags);
    ///
    /// TAGS: Clear, Bits, Reset
    ///
    void BitsClear(Bits *bv);

    ///
    /// Reserve space for at least n bits in Bitstor.
    /// Does not change the length, only ensures capacity.
    ///
    /// bv[in] : Bitstor to reserve space in
    /// n[in]  : Number of bits to reserve space for
    ///
    /// USAGE:
    ///   BitsReserve(&flags, 1000);
    ///
    /// TAGS: Bits, Reserve, Capacity, Memory
    ///
    void BitsReserve(Bits *bv, u64 n);

    ///
    /// Reu64 Bitstor to hold exactly n bits.
    /// May grow or shrink the Bitstor.
    ///
    /// bv[in] : Bitstor to resize
    /// n[in]  : New u64 in bits
    ///
    /// USAGE:
    ///   BitsResize(&flags, 64);
    ///
    /// TAGS: Bits, Resize, Length
    ///
    void BitsResize(Bits *bv, u64 n);

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_Bits_INIT_H
