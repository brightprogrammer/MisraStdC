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

///
/// Aborting variant of `BitVecReserve`. See that function for parameter
/// semantics and success-state effects.
///
/// SUCCESS : Returns to the caller. The underlying `BitVecReserve` call
///           succeeded; see `BitVecReserve` for the post-state.
/// FAILURE : Does not return - aborts via `LOG_FATAL` / `Abort` when
///           the underlying `BitVecReserve` call returns `false`.
///
/// TAGS: BitVec, Reserve, Must, Abort
///
#define BitVecMustReserve(bv, n)                                                                                       \
    do {                                                                                                               \
        if (!BitVecReserve((bv), (n))) {                                                                               \
            LOG_FATAL("BitVecMustReserve failed");                                                                     \
        }                                                                                                              \
    } while (0)

///
/// Aborting variant of `BitVecResize`. See that function for parameter
/// semantics and success-state effects.
///
/// SUCCESS : Returns to the caller. The underlying `BitVecResize` call
///           succeeded; see `BitVecResize` for the post-state.
/// FAILURE : Does not return - aborts via `LOG_FATAL` / `Abort` when
///           the underlying `BitVecResize` call returns `false`.
///
/// TAGS: BitVec, Resize, Must, Abort
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
/// Initialize an empty BitVec. Inside a `Scope` block the allocator
/// argument may be omitted; the internal `MisraScope` allocator is
/// used. Otherwise pass a typed allocator handle or a raw
/// `Allocator *`.
///
/// TAGS: BitVec, Init, API
///
#define BitVecInit(...) OVERLOAD(BitVecInit, __VA_ARGS__)
#define BitVecInit_0()  BitVecInit_1(MisraScope)
#ifdef __cplusplus
#    define BitVecInit_1(allocator_ptr)                                                                                \
        (BitVec {                                                                                                      \
            .length    = 0,                                                                                            \
            .capacity  = 0,                                                                                            \
            .data      = NULL,                                                                                         \
            .byte_size = 0,                                                                                            \
            .allocator = ALLOCATOR_OF(allocator_ptr),                                                                  \
            .__magic   = BITVEC_MAGIC | MAGIC_VALIDATED_BIT                                                                                  \
        })
#else
#    define BitVecInit_1(allocator_ptr)                                                                                \
        ((BitVec) {.length    = 0,                                                                                     \
                   .capacity  = 0,                                                                                     \
                   .data      = NULL,                                                                                  \
                   .byte_size = 0,                                                                                     \
                   .allocator = ALLOCATOR_OF(allocator_ptr),                                                           \
                   .__magic   = BITVEC_MAGIC | MAGIC_VALIDATED_BIT})
#endif

    ///
    /// Initialize an empty `BitVec` and reserve room for at least `cap` bits.
    /// Public callers reach this through the `BitVecInitWithCapacity` overload set.
    ///
    /// cap[in]   : Minimum bit capacity to reserve up front.
    /// alloc[in] : Allocator that owns the backing buffer.
    ///
    /// SUCCESS : Returns a live, empty `BitVec` whose capacity is at least `cap` bits.
    /// FAILURE : Returns an empty `BitVec` with NULL data and zero capacity on allocator OOM;
    ///           the caller must check before use.
    ///
    /// TAGS: BitVec, Init, Capacity, Construct
    ///
    BitVec bitvec_init_with_capacity(u64 cap, Allocator *alloc);
#define BitVecInitWithCapacity(...)         OVERLOAD(BitVecInitWithCapacity, __VA_ARGS__)
#define BitVecInitWithCapacity_1(cap)         bitvec_init_with_capacity((cap), MisraScope)
#define BitVecInitWithCapacity_2(cap, alloc)  bitvec_init_with_capacity((cap), ALLOCATOR_OF(alloc))

    ///
    /// Release all storage owned by `bv` and reset it to the zeroed state.
    ///
    /// bv[in,out] : Bitvector to deinitialize. Must not be used until reinitialized.
    ///
    /// SUCCESS : Backing buffer freed; `bv` left with NULL data, zero length, zero capacity.
    /// FAILURE : Function cannot fail; aborts on a corrupted magic via the validator.
    ///
    /// TAGS: BitVec, Deinit, Memory
    ///
    void BitVecDeinit(BitVec *bv);

    ///
    /// Reset `bv` to length 0 while keeping its allocated capacity.
    ///
    /// bv[in,out] : Bitvector to clear.
    ///
    /// SUCCESS : `bv->length` becomes 0; backing buffer and capacity are retained.
    /// FAILURE : Function cannot fail; aborts on a corrupted magic via the validator.
    ///
    /// TAGS: BitVec, Clear, Reset, Memory
    ///
    void BitVecClear(BitVec *bv);

    ///
    /// Ensure `bv` can hold at least `n` bits without further reallocation.
    ///
    /// bv[in,out] : Bitvector whose capacity should grow.
    /// n[in]      : Minimum bit capacity required after the call.
    ///
    /// SUCCESS : Returns true; `bv->capacity` is at least `n` and existing bits are preserved.
    /// FAILURE : Returns false on allocator OOM; `bv` is left unchanged.
    ///
    /// TAGS: BitVec, Reserve, Capacity, Memory
    ///
    bool BitVecReserve(BitVec *bv, u64 n);

    ///
    /// Resize `bv` to exactly `n` bits, growing or shrinking as needed. Newly added bits are
    /// zero-initialized.
    ///
    /// bv[in,out] : Bitvector to resize.
    /// n[in]      : New bit length.
    ///
    /// SUCCESS : Returns true; `bv->length` equals `n` and any new bits are zeroed.
    /// FAILURE : Returns false on allocator OOM during growth; `bv` is left unchanged.
    ///
    /// TAGS: BitVec, Resize, Length, Memory
    ///
    bool BitVecResize(BitVec *bv, u64 n);

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_BITVEC_INIT_H
