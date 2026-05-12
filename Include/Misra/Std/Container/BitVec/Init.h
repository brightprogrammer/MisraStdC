/// file      : std/container/bitvec/init.h
/// author    : Generated following Misra project patterns
/// This is free and unencumbered software released into the public domain.
///
/// Different types of initializers for a bitvector.

#ifndef MISRA_STD_CONTAINER_BITVEC_INIT_H
#define MISRA_STD_CONTAINER_BITVEC_INIT_H

#include "Type.h"
#include <Misra/Std/Memory.h>
#include <stdio.h>
#include <string.h>

// Helper macro for bit operations
#define BITVEC_BYTES_FOR_BITS(bits) (((bits) + 7) / 8)

void SysAbort(void);

#define BITVEC_INIT_ABORT(message) bitvec_abort_init_operation(__func__, __LINE__, (message))
#define BitVecMustReserve(bv, n)                                                                                       \
    do {                                                                                                               \
        if (!BitVecReserve((bv), (n))) {                                                                               \
            BITVEC_INIT_ABORT("BitVecMustReserve failed");                                                             \
        }                                                                                                              \
    } while (0)
#define BitVecMustResize(bv, n)                                                                                        \
    do {                                                                                                               \
        if (!BitVecResize((bv), (n))) {                                                                                \
            BITVEC_INIT_ABORT("BitVecMustResize failed");                                                              \
        }                                                                                                              \
    } while (0)

static inline void bitvec_abort_init_operation(const char *function, int line, const char *message) {
    fprintf(stderr, "FATAL [%s:%d] %s\n", function, line, message);
    SysAbort();
}

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
#define BITVEC_INIT_HAS_ARGS_IMPL(_0, _1, count, ...) count
#define BITVEC_INIT_HAS_ARGS(...) BITVEC_INIT_HAS_ARGS_IMPL(__VA_OPT__(,) __VA_ARGS__, 1, 0, 0)

#ifdef __cplusplus
#    define BitVecInit(...) CONCAT(BitVecInit_, BITVEC_INIT_HAS_ARGS(__VA_ARGS__))(__VA_ARGS__)
#    define BitVecInit_0()                                                                                            \
        (BitVec {.length = 0,                                                                                         \
                 .capacity = 0,                                                                                       \
                 .data = NULL,                                                                                        \
                 .byte_size = 0,                                                                                      \
                 .allocator = AllocatorBind(DefaultAllocator()),                                                      \
                 .__magic = MISRA_BITVEC_MAGIC})
#    define BitVecInit_1(alloc)                                                                                       \
        (BitVec {.length = 0,                                                                                         \
                 .capacity = 0,                                                                                       \
                 .data = NULL,                                                                                        \
                 .byte_size = 0,                                                                                      \
                 .allocator = AllocatorBind((alloc)),                                                                 \
                 .__magic = MISRA_BITVEC_MAGIC})
#else
#    define BitVecInit(...) CONCAT(BitVecInit_, BITVEC_INIT_HAS_ARGS(__VA_ARGS__))(__VA_ARGS__)
#    define BitVecInit_0()                                                                                            \
        ((BitVec) {.length = 0,                                                                                       \
                   .capacity = 0,                                                                                     \
                   .data = NULL,                                                                                      \
                   .byte_size = 0,                                                                                    \
                   .allocator = AllocatorBind(DefaultAllocator()),                                                    \
                   .__magic = MISRA_BITVEC_MAGIC})
#    define BitVecInit_1(alloc)                                                                                       \
        ((BitVec) {.length = 0,                                                                                       \
                   .capacity = 0,                                                                                     \
                   .data = NULL,                                                                                      \
                   .byte_size = 0,                                                                                    \
                   .allocator = AllocatorBind((alloc)),                                                               \
                   .__magic = MISRA_BITVEC_MAGIC})
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
    BitVec BitVecInitWithCapacityAlloc(u64 cap, Allocator alloc);

#define BitVecInitWithCapacity(cap) BitVecInitWithCapacityAlloc((cap), DefaultAllocator())
#define BitVecInitWithCapacityWithAlloc(cap, alloc) BitVecInitWithCapacityAlloc((cap), (alloc))



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
    bool BitVecReserve(BitVec *bv, u64 n);

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
    bool BitVecResize(BitVec *bv, u64 n);

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_BITVEC_INIT_H
