/// file      : std/allocator.h
/// author    : Generated during allocator refactor
/// This is free and unencumbered software released into the public domain.
///
/// Allocator configuration and generic helper functions.

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

    typedef bool (*AllocatorStateInit)(Allocator *alloc);
    typedef void (*AllocatorStateDeinit)(Allocator *alloc);
    typedef void *(*AllocatorAllocateFn)(Allocator *alloc, size bytes, bool zeroed);
    typedef void *(*AllocatorReallocateFn)(Allocator *alloc, void *ptr, size old_size, size new_size);
    typedef void (*AllocatorDeallocateFn)(Allocator *alloc, void *ptr, size bytes);

    struct Allocator {
        void                 *state;
        AllocatorStateInit    state_init;
        AllocatorStateDeinit  state_deinit;
        AllocatorAllocateFn   allocate;
        AllocatorReallocateFn reallocate;
        AllocatorDeallocateFn deallocate;
        AllocatorEffort       effort;
        u32                   retry_limit;
        u32                   flags;
        size                  alignment;
    };

    ///
    /// Prepare an allocator for object binding.
    /// Missing allocation callbacks are filled from the default heap allocator and
    /// the runtime `state` pointer is reset so each bound object starts with a fresh
    /// allocator instance state.
    ///
    /// alloc[in] : Allocator template to bind
    ///
    /// SUCCESS: Returns a bound allocator descriptor.
    /// FAILURE: Function cannot fail.
    ///
    /// TAGS: Allocator, Binding, Initialization, Memory
    ///
    Allocator AllocatorBind(Allocator alloc);

    ///
    /// Ensure that an allocator has initialized runtime state.
    /// If the allocator has no `state_init` callback or already has a state object,
    /// this succeeds immediately.
    ///
    /// alloc[in,out] : Allocator to prepare for allocation
    ///
    /// SUCCESS: Returns `true` when allocator state is ready.
    /// FAILURE: Returns `false` when state initialization fails or `alloc` is NULL.
    ///
    /// TAGS: Allocator, State, Initialization, Memory
    ///
    bool AllocatorEnsureState(Allocator *alloc);

    ///
    /// Allocate memory through an allocator.
    /// Allocations honor the allocator's configured alignment (`alloc->alignment`).
    /// The allocator effort policy controls how many attempts are made before the
    /// allocation is reported as failed.
    ///
    /// alloc[in,out]   : Allocator used for the allocation
    /// bytes[in]       : Number of bytes to allocate
    /// zeroed[in]      : Whether the allocated region must be zero-initialized
    ///
    /// SUCCESS: Returns a writable pointer to allocated memory.
    /// FAILURE: Returns NULL when allocation fails or allocator is invalid.
    ///
    /// TAGS: Allocator, Memory, Allocation, Runtime
    ///
    void *AllocatorAlloc(Allocator *alloc, size bytes, bool zeroed);

    ///
    /// Reallocate memory through an allocator.
    /// Reallocations preserve the allocator's configured alignment.
    ///
    /// alloc[in,out]     : Allocator used for the reallocation
    /// ptr[in]           : Existing allocation pointer, or NULL
    /// old_size[in]      : Previous allocation size in bytes
    /// new_size[in]      : Requested new allocation size in bytes
    ///
    /// SUCCESS: Returns a pointer to the resized allocation, or NULL when
    ///          `new_size` is zero.
    /// FAILURE: Returns NULL when reallocation fails or allocator is invalid.
    ///
    /// TAGS: Allocator, Memory, Reallocation, Runtime
    ///
    void *AllocatorRealloc(Allocator *alloc, void *ptr, size old_size, size new_size);

    ///
    /// Free memory through an allocator.
    ///
    /// alloc[in,out]   : Allocator that owns the allocation
    /// ptr[in]         : Pointer to the allocation, or NULL
    /// bytes[in]       : Allocation size in bytes
    ///
    /// SUCCESS: Function cannot fail.
    /// FAILURE: No action is taken when `ptr` or `alloc` is invalid.
    ///
    /// TAGS: Allocator, Memory, Deallocation, Runtime
    ///
    void AllocatorFree(Allocator *alloc, void *ptr, size bytes);

    ///
    /// Return a copy of `alloc` with its `alignment` field raised to at least
    /// `min_alignment`. Existing alignment greater than `min_alignment` is kept.
    /// Passing `0` for `min_alignment` returns `alloc` unchanged. Useful when an
    /// object knows a minimum alignment requirement and wants to bind it onto
    /// whatever allocator the caller supplied.
    ///
    /// alloc[in]         : Allocator template.
    /// min_alignment[in] : Floor alignment in bytes (power of two).
    ///
    /// SUCCESS: Returns an allocator descriptor with the higher alignment.
    /// FAILURE: Function cannot fail.
    ///
    /// TAGS: Allocator, Alignment, Builder, Memory
    ///
    static inline Allocator AllocatorWithMinAlignment(Allocator alloc, size min_alignment) {
        if (min_alignment > alloc.alignment) {
            alloc.alignment = min_alignment;
        }
        return alloc;
    }

    ///
    /// Release allocator runtime state bound to an object.
    /// This does not free allocations owned by containers; it only tears down the
    /// allocator's internal state object and resets `state` to NULL.
    ///
    /// alloc[in,out] : Allocator to unbind
    ///
    /// SUCCESS: Function cannot fail.
    /// FAILURE: No action is taken when `alloc` is NULL.
    ///
    /// TAGS: Allocator, State, Cleanup, Memory
    ///
    void AllocatorUnbind(Allocator *alloc);

#ifdef __cplusplus
}
#endif

#include <Misra/Std/Allocator/Arena.h>
#include <Misra/Std/Allocator/Heap.h>
#include <Misra/Std/Allocator/Page.h>

///
/// Obtain the library default allocator.
/// This currently expands to the heap allocator descriptor.
///
/// USAGE:
///   Allocator alloc = DefaultAllocator();
///
/// TAGS: Allocator, Macro, Default, Memory
///
#define DefaultAllocator() HeapAllocator()

#endif // MISRA_STD_ALLOCATOR_H
