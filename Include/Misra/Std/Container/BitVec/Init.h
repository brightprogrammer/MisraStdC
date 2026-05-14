/// file      : std/container/bitvec/init.h
/// author    : Generated following Misra project patterns
/// This is free and unencumbered software released into the public domain.
///
/// Different types of initializers for a bitvector.

#ifndef MISRA_STD_CONTAINER_BITVEC_INIT_H
#define MISRA_STD_CONTAINER_BITVEC_INIT_H

#include "Type.h"
#include <Misra/Std/Memory.h>

// Helper macro for bit operations
#define BITVEC_BYTES_FOR_BITS(bits) (((bits) + 7) / 8)

///
/// Aborting variant of `BitVecReserve`. Calls `LOG_FATAL` on allocation
/// failure.
///
/// bv[in,out] : Bitvector to grow.
/// n[in]      : Number of bits to reserve space for.
///
/// SUCCESS : Returns to the caller.
/// FAILURE : Does not return - aborts via `LOG_FATAL` / `SysAbort`.
///
/// TAGS: BitVec, Reserve, Capacity, Must, Abort
///
#define BitVecMustReserve(bv, n)                                                                                       \
    do {                                                                                                               \
        if (!BitVecReserve((bv), (n))) {                                                                               \
            LOG_FATAL("BitVecMustReserve failed");                                                                     \
        }                                                                                                              \
    } while (0)

///
/// Aborting variant of `BitVecResize`. Calls `LOG_FATAL` on allocation
/// failure.
///
/// bv[in,out] : Bitvector to resize.
/// n[in]      : New length in bits.
///
/// SUCCESS : Returns to the caller.
/// FAILURE : Does not return - aborts via `LOG_FATAL` / `SysAbort`.
///
/// TAGS: BitVec, Resize, Length, Must, Abort
///
#define BitVecMustResize(bv, n)                                                                                        \
    do {                                                                                                               \
        if (!BitVecResize((bv), (n))) {                                                                                \
            LOG_FATAL("BitVecMustResize failed");                                                                      \
        }                                                                                                              \
    } while (0)

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
#define BITVEC_INIT_HAS_ARGS(...)                     BITVEC_INIT_HAS_ARGS_IMPL(__VA_OPT__(, ) __VA_ARGS__, 1, 0, 0)

#ifdef __cplusplus
#    define BitVecInit(...) CONCAT(BitVecInit_, BITVEC_INIT_HAS_ARGS(__VA_ARGS__))(__VA_ARGS__)
#    define BitVecInit_0()                                                                                             \
        (BitVec {                                                                                                      \
            .length    = 0,                                                                                            \
            .capacity  = 0,                                                                                            \
            .data      = NULL,                                                                                         \
            .byte_size = 0,                                                                                            \
            .allocator = AllocatorBind(DefaultAllocator()),                                                            \
            .__magic   = MISRA_BITVEC_MAGIC                                                                            \
        })
#    define BitVecInit_1(alloc)                                                                                        \
        (BitVec {                                                                                                      \
            .length    = 0,                                                                                            \
            .capacity  = 0,                                                                                            \
            .data      = NULL,                                                                                         \
            .byte_size = 0,                                                                                            \
            .allocator = AllocatorBind((alloc)),                                                                       \
            .__magic   = MISRA_BITVEC_MAGIC                                                                            \
        })
#else
#    define BitVecInit(...) CONCAT(BitVecInit_, BITVEC_INIT_HAS_ARGS(__VA_ARGS__))(__VA_ARGS__)
#    define BitVecInit_0()                                                                                             \
        ((BitVec) {.length    = 0,                                                                                     \
                   .capacity  = 0,                                                                                     \
                   .data      = NULL,                                                                                  \
                   .byte_size = 0,                                                                                     \
                   .allocator = AllocatorBind(DefaultAllocator()),                                                     \
                   .__magic   = MISRA_BITVEC_MAGIC})
#    define BitVecInit_1(alloc)                                                                                        \
        ((BitVec) {.length    = 0,                                                                                     \
                   .capacity  = 0,                                                                                     \
                   .data      = NULL,                                                                                  \
                   .byte_size = 0,                                                                                     \
                   .allocator = AllocatorBind((alloc)),                                                                \
                   .__magic   = MISRA_BITVEC_MAGIC})
#endif

    ///
    /// Initialize bitvector with initial capacity and explicit allocator.
    /// Creates a bitvector with reserved space for the specified number of bits.
    ///
    /// cap[in]   : Initial capacity in bits.
    /// alloc[in] : Allocator to bind to the bitvector.
    ///
    /// SUCCESS : Returns initialized bitvector.
    /// FAILURE : Returns an empty bitvector if allocation fails.
    ///
    /// USAGE:
    ///   BitVec flags = BitVecInitWithCapacityAlloc(64, allocator);
    ///
    /// TAGS: Init, BitVec, Boolean, Bits, Capacity, Allocator
    ///
    BitVec BitVecInitWithCapacityAlloc(u64 cap, Allocator alloc);

///
/// Initialize bitvector with initial capacity.
///
/// This public API supports both forms:
///
/// - `BitVecInitWithCapacity(cap)`
/// - `BitVecInitWithCapacity(cap, alloc)`
///
/// Omitting the allocator uses `DefaultAllocator()`.
///
/// SUCCESS : Returns initialized bitvector.
/// FAILURE : Returns an empty bitvector if allocation fails.
///
/// TAGS: Init, BitVec, Boolean, Bits, Capacity, Allocator, Macro
///
#define BITVEC_INIT_WITH_CAPACITY_HAS_ARGS_IMPL(_1, _2, count, ...) count
#define BITVEC_INIT_WITH_CAPACITY_HAS_ARGS(...)                     BITVEC_INIT_WITH_CAPACITY_HAS_ARGS_IMPL(__VA_ARGS__, 2, 1, 0)
#define BitVecInitWithCapacity(...)                                                                                    \
    CONCAT(BitVecInitWithCapacity_, BITVEC_INIT_WITH_CAPACITY_HAS_ARGS(__VA_ARGS__))(__VA_ARGS__)
#define BitVecInitWithCapacity_1(cap)        BitVecInitWithCapacityAlloc((cap), DefaultAllocator())
#define BitVecInitWithCapacity_2(cap, alloc) BitVecInitWithCapacityAlloc((cap), (alloc))



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
    /// bv[in,out] : Bitvector to reserve space in.
    /// n[in]      : Number of bits to reserve space for.
    ///
    /// SUCCESS : `true`. The bitvector capacity is at least `n` bits.
    /// FAILURE : `false` on allocation failure. The bitvector is unchanged.
    ///
    /// USAGE:
    ///   if (!BitVecReserve(&flags, 1000)) { /* recover */ }
    ///
    /// TAGS: BitVec, Reserve, Capacity, Memory
    ///
    bool BitVecReserve(BitVec *bv, u64 n);

    ///
    /// Resize bitvector to hold exactly `n` bits.
    /// Grows or shrinks the bitvector. New bits (when growing) are initialized
    /// to `false`.
    ///
    /// bv[in,out] : Bitvector to resize.
    /// n[in]      : New length in bits.
    ///
    /// SUCCESS : `true`. The bitvector length is exactly `n`.
    /// FAILURE : `false` on allocation failure when growth is needed. The
    ///           bitvector is unchanged.
    ///
    /// USAGE:
    ///   if (!BitVecResize(&flags, 64)) { /* recover */ }
    ///
    /// TAGS: BitVec, Resize, Length
    ///
    bool BitVecResize(BitVec *bv, u64 n);

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_BITVEC_INIT_H
