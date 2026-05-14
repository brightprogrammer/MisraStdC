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

#define BITVEC_INIT_HAS_ARGS_IMPL(_0, _1, count, ...) count
#define BITVEC_INIT_HAS_ARGS(...)                     BITVEC_INIT_HAS_ARGS_IMPL(__VA_OPT__(, ) __VA_ARGS__, 1, 0, 0)

#ifdef __cplusplus
///
/// Initialize bitvector with default values. It is mandatory to initialize
/// bitvectors before use; not doing so is undefined behaviour.
///
/// This public macro supports both forms:
///
/// - `BitVecInit()`         - uses `DefaultAllocator()`.
/// - `BitVecInit(alloc)`    - binds the supplied allocator.
///
/// alloc[in] : Optional allocator override.
///
/// SUCCESS : Returns a fresh `BitVec` with length 0, capacity 0, NULL
///           data pointer, and the chosen allocator bound. No heap
///           allocation is performed until the bitvector is mutated.
/// FAILURE : Function cannot fail.
///
/// USAGE:
///   BitVec flags = BitVecInit();
///
/// TAGS: Init, BitVec, Boolean, Bits
///
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
    /// SUCCESS : Returns an initialized `BitVec`. Length is 0, capacity is
    ///           at least `cap` bits, the byte buffer has been allocated
    ///           and zero-filled, and `alloc` has been bound into the
    ///           bitvector's allocator slot.
    /// FAILURE : Returns an empty bitvector (length 0, capacity 0, data
    ///           NULL) when allocation of the underlying byte buffer
    ///           fails. The allocator is still bound so the returned
    ///           object is safe to `BitVecDeinit` or retry-reserve.
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
/// SUCCESS : Returns an initialized `BitVec` with reserved capacity for at
///           least `cap` bits (see `BitVecInitWithCapacityAlloc`).
/// FAILURE : Returns an empty bitvector (length 0, capacity 0, data NULL,
///           allocator still bound) when the underlying byte buffer
///           allocation fails.
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
    /// bv[in,out] : Pointer to bitvector to deinitialize.
    ///
    /// SUCCESS : Returns to the caller. The underlying byte buffer has
    ///           been released to the allocator; length, capacity, and
    ///           byte_size are reset to 0 and `data` to NULL. The
    ///           allocator is unbound and replaced with
    ///           `AllocatorBind(DefaultAllocator())` so the struct is
    ///           safe to re-initialize.
    /// FAILURE : Function cannot fail. A NULL `bv` or invalid magic is a
    ///           caller bug and aborts via `LOG_FATAL`.
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
    /// bv[in,out] : Pointer to bitvector to clear.
    ///
    /// SUCCESS : Returns to the caller. Length is now 0. Capacity and the
    ///           underlying byte buffer are preserved (bits are not
    ///           zeroed; subsequent inserts will overwrite them).
    /// FAILURE : Function cannot fail. A NULL `bv` or invalid magic is a
    ///           caller bug and aborts via `LOG_FATAL`.
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
    /// SUCCESS : Returns `true`. The byte buffer is grown (if necessary)
    ///           so the bitvector can hold at least `n` bits without
    ///           further allocation; newly-allocated bytes beyond the
    ///           previous buffer are zero-filled. Length and the values
    ///           of all existing bits are unchanged.
    /// FAILURE : Returns `false` on allocation failure during the realloc.
    ///           The bitvector is unchanged.
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
    /// SUCCESS : Returns `true`. The bitvector length is now exactly `n`.
    ///           When shrinking, bits beyond `n` are dropped (capacity is
    ///           preserved). When growing, bits in `[old_length, n)` are
    ///           `false`; capacity is at least `n`.
    /// FAILURE : Returns `false` on allocation failure when growth is
    ///           needed. The bitvector is unchanged. Shrinking does not
    ///           allocate and therefore cannot fail.
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
