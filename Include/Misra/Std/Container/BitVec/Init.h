/// file      : std/container/bitvec/init.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Different types of initializers for a bitvector.

#ifndef MISRA_STD_CONTAINER_BITVEC_INIT_H
#define MISRA_STD_CONTAINER_BITVEC_INIT_H

#include "Type.h"
#include <Misra/Std/Allocator.h>
#include <Misra/Std/Memory.h>

#define BITVEC_BYTES_FOR_BITS(bits) (((bits) + 7) / 8)

#define BitVecMustReserve(bv, n)                                                                                       \
    do {                                                                                                               \
        if (!BitVecReserve((bv), (n))) {                                                                               \
            LOG_FATAL("BitVecMustReserve failed");                                                                     \
        }                                                                                                              \
    } while (0)

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
/// Initialize an empty BitVec. Inside a `Scope` block the allocator
/// argument may be omitted; the internal `MisraScope` allocator is
/// used. Otherwise pass a typed allocator handle or a raw
/// `Allocator *`.
///
#define BitVecInit(...) MISRA_OVERLOAD(BitVecInit, __VA_ARGS__)
#define BitVecInit_0()  BitVecInit_1(MisraScope)
#ifdef __cplusplus
#    define BitVecInit_1(allocator_ptr)                                                                                \
        (BitVec {                                                                                                      \
            .length    = 0,                                                                                            \
            .capacity  = 0,                                                                                            \
            .data      = NULL,                                                                                         \
            .byte_size = 0,                                                                                            \
            .allocator = ALLOCATOR_OF(allocator_ptr),                                                                  \
            .__magic   = BITVEC_MAGIC                                                                                  \
        })
#else
#    define BitVecInit_1(allocator_ptr)                                                                                \
        ((BitVec) {.length    = 0,                                                                                     \
                   .capacity  = 0,                                                                                     \
                   .data      = NULL,                                                                                  \
                   .byte_size = 0,                                                                                     \
                   .allocator = ALLOCATOR_OF(allocator_ptr),                                                           \
                   .__magic   = BITVEC_MAGIC})
#endif

    BitVec BitVecInitWithCapacity(u64 cap, Allocator *alloc);

#define BitVecInitWithCapacityMacro(cap, allocator_ptr) BitVecInitWithCapacity((cap), ALLOCATOR_OF(allocator_ptr))

    void BitVecDeinit(BitVec *bv);
    void BitVecClear(BitVec *bv);
    bool BitVecReserve(BitVec *bv, u64 n);
    bool BitVecResize(BitVec *bv, u64 n);

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_BITVEC_INIT_H
