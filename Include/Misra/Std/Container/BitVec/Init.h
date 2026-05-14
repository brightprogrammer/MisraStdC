/// file      : std/container/bitvec/init.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Different types of initializers for a bitvector.

#ifndef MISRA_STD_CONTAINER_BITVEC_INIT_H
#define MISRA_STD_CONTAINER_BITVEC_INIT_H

#include "Private.h"
#include "Type.h"
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
/// Initialize bitvector. The user-owned typed allocator pointer (e.g.
/// `&heap` where `heap` is a `HeapAllocator`) is required.
///
#define BitVecInit(typed_alloc_ptr) bitvec_init_alloc(ALLOCATOR_OF(typed_alloc_ptr))

    BitVec BitVecInitWithCapacityAlloc(u64 cap, Allocator *alloc);

#define BitVecInitWithCapacity(cap, typed_alloc_ptr) BitVecInitWithCapacityAlloc((cap), ALLOCATOR_OF(typed_alloc_ptr))

    void BitVecDeinit(BitVec *bv);
    void BitVecClear(BitVec *bv);
    bool BitVecReserve(BitVec *bv, u64 n);
    bool BitVecResize(BitVec *bv, u64 n);

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_BITVEC_INIT_H
