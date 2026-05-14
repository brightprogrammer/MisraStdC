/// file      : std/allocator.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Allocator base type and dispatch API. Concrete allocator types
/// (`HeapAllocator`, `PageAllocator`, `ArenaAllocator`, `PoolAllocator`)
/// embed an `Allocator base` at offset 0 and carry their state inline so
/// the library never owns mutable global state. Users construct typed
/// allocators with their `*Init` macros and pass `&heap` / `&arena` /
/// `&page` / `&pool` to container constructors - the container macros
/// compile-check that the argument has an `Allocator base` and store a
/// pointer to it.

#ifndef MISRA_STD_ALLOCATOR_H
#define MISRA_STD_ALLOCATOR_H

#include <Misra/Types.h>

#ifdef __cplusplus
extern "C" {
#endif

    typedef enum {
        ALLOCATOR_EFFORT_ONCE = 0,
        ALLOCATOR_EFFORT_RETRY,
        ALLOCATOR_EFFORT_RETRY_FALLBACK,
    } AllocatorEffort;

    typedef struct Allocator Allocator;

    typedef void *(*AllocatorAllocateFn)(Allocator *self, size bytes, bool zeroed);
    typedef void *(*AllocatorReallocateFn)(Allocator *self, void *ptr, size old_size, size new_size);
    typedef void (*AllocatorDeallocateFn)(Allocator *self, void *ptr, size bytes);

    ///
    /// Generic allocator base. Every typed allocator carries this struct as
    /// its first member (named `base`). Pointers downcast cleanly between
    /// `Allocator *` and the typed allocator pointer because `base` is at
    /// offset 0 in every typed struct.
    ///
    /// State is **not** stored here. Each typed allocator stores its own
    /// state inline after the base, so the library has no `void *state`
    /// pointer to manage and never allocates state behind the caller's
    /// back.
    ///
    struct Allocator {
        AllocatorAllocateFn   allocate;
        AllocatorReallocateFn reallocate;
        AllocatorDeallocateFn deallocate;
        size                  alignment;
        AllocatorEffort       effort;
        u32                   retry_limit;
        u32                   flags;
    };

    ///
    /// Magic sentinel used to identify a properly-initialized allocator base
    /// at runtime. Embedded inside the base via the `*Init` macros so
    /// `ValidateAllocator` can catch use-of-uninitialized-allocator bugs.
    ///
    /// Stored in `flags` because it doesn't otherwise carry state in current
    /// allocators - leaves room to add real flag bits later if needed by
    /// using the lower bits and keeping the magic in the upper bits.
    ///

    ///
    /// Allocate memory through an allocator.
    /// Allocations honor the allocator's configured alignment.
    ///
    /// self[in,out]  : Allocator base used for the allocation.
    /// bytes[in]     : Number of bytes to allocate.
    /// zeroed[in]    : Whether the allocated region must be zero-initialized.
    ///
    /// SUCCESS: Returns a writable pointer to allocated memory.
    /// FAILURE: Returns NULL when allocation fails or `self` is invalid.
    ///
    /// TAGS: Allocator, Memory, Allocation
    ///
    void *AllocatorAlloc(Allocator *self, size bytes, bool zeroed);

    ///
    /// Reallocate memory through an allocator. Preserves the allocator's
    /// configured alignment across the resize.
    ///
    /// self[in,out]  : Allocator base used for the reallocation.
    /// ptr[in]       : Existing allocation pointer, or NULL.
    /// old_size[in]  : Previous allocation size in bytes.
    /// new_size[in]  : Requested new allocation size in bytes.
    ///
    /// SUCCESS: Returns a pointer to the resized allocation, or NULL when
    ///          `new_size` is zero.
    /// FAILURE: Returns NULL when reallocation fails.
    ///
    /// TAGS: Allocator, Memory, Reallocation
    ///
    void *AllocatorRealloc(Allocator *self, void *ptr, size old_size, size new_size);

    ///
    /// Free memory through an allocator.
    ///
    /// self[in,out] : Allocator base that issued the original allocation.
    /// ptr[in]      : Pointer to the allocation, or NULL.
    /// bytes[in]    : Allocation size in bytes.
    ///
    /// SUCCESS: Function returns. The allocation is reclaimed.
    /// FAILURE: No action is taken when `ptr` or `self` is invalid.
    ///
    /// TAGS: Allocator, Memory, Deallocation
    ///
    void AllocatorFree(Allocator *self, void *ptr, size bytes);

    ///
    /// Validate an allocator base. Aborts via `LOG_FATAL` when the allocator
    /// is structurally invalid (NULL pointer, missing fn pointers, alignment
    /// is zero or not a power of two).
    ///
    /// self[in] : Allocator base to validate.
    ///
    /// SUCCESS: Function returns. The allocator is structurally valid.
    /// FAILURE: Does not return - aborts via `LOG_FATAL` / `SysAbort`.
    ///
    /// TAGS: Allocator, Validation, Contract
    ///
    void ValidateAllocator(const Allocator *self);

#ifdef __cplusplus
}
#endif

///
/// Compile-time-check that `typed_alloc_ptr` points at a struct whose first
/// field is named `base` of type `Allocator`, and yield `&typed_alloc_ptr->base`.
///
/// This is the bridge between the user's typed allocator handle
/// (`HeapAllocator *`, `PageAllocator *`, ...) and the generic
/// `Allocator *` that container init macros store internally. User code
/// never writes `.base` by hand:
///
///     HeapAllocator heap = HeapAllocatorInit();
///     Vec(int) v = VecInit(&heap);   // container macro uses ALLOCATOR_OF(&heap)
///
/// Passing a pointer to a struct that lacks an `Allocator base` field
/// triggers a compile error from `CHECK_TYPE_EQUIVALENCE`.
///
#define ALLOCATOR_OF(typed_alloc_ptr)                                                                                  \
    (CHECK_TYPE_EQUIVALENCE(TYPE_OF((typed_alloc_ptr)->base), Allocator), &(typed_alloc_ptr)->base)

#include <Misra/Std/Allocator/Arena.h>
#include <Misra/Std/Allocator/Heap.h>
#include <Misra/Std/Allocator/Page.h>
#include <Misra/Std/Allocator/Pool.h>

#endif // MISRA_STD_ALLOCATOR_H
