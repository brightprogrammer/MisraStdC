/// file      : std/allocator.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Allocator base type and dispatch API. Concrete allocator types
/// (`HeapAllocator`, `PageAllocator`, `ArenaAllocator`, `SlabAllocator`)
/// embed an `Allocator base` at offset 0 and carry their state inline so
/// the library never owns mutable global state. Users construct typed
/// allocators with their `*Init` macros and pass `&heap` / `&arena` /
/// `&page` / `&slab` to container constructors - the container macros
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

    typedef struct Allocator       Allocator;
    typedef struct HeapAllocator   HeapAllocator;
    typedef struct PageAllocator   PageAllocator;
    typedef struct ArenaAllocator  ArenaAllocator;
    typedef struct SlabAllocator   SlabAllocator;
    typedef struct BudgetAllocator BudgetAllocator;

    // `zeroed` uses `i8` (signed char) directly instead of `bool` to
    // sidestep TU-to-TU `bool` ambiguity on platforms that transitively
    // pull in `<stdbool.h>` from system headers. See the comment block
    // around the `bool` typedef in `Misra/Types.h`.
    typedef void *(*AllocatorAllocateFn)(Allocator *self, size bytes, i8 zeroed);
    typedef void *(*AllocatorReallocateFn)(Allocator *self, void *ptr, size old_size, size new_size);
    typedef void (*AllocatorDeallocateFn)(Allocator *self, void *ptr, size bytes);

#if MISRA_HAVE_ALLOC_STATS
    ///
    /// Per-allocator memory-pressure counters. Updated by the dispatch
    /// wrappers (`AllocatorAlloc` / `AllocatorRealloc` / `AllocatorFree`)
    /// so every typed allocator gets accounting for free. Reset with
    /// `AllocatorResetStats(...)`, read with `AllocatorGetStats(...)`.
    ///
    /// FIELDS:
    /// - bytes_requested     : cumulative bytes ever requested via
    ///                         allocate (does not decrease on free).
    /// - bytes_in_use        : currently outstanding bytes
    ///                         (allocate + realloc-grow - realloc-shrink - deallocate).
    /// - peak_bytes_in_use   : historical max of bytes_in_use.
    /// - allocations         : count of successful allocate calls.
    /// - reallocations       : count of successful reallocate calls.
    /// - deallocations       : count of deallocate calls.
    /// - failed_allocations  : count of allocate / reallocate calls that
    ///                         returned NULL.
    ///
    /// TAGS: Allocator, Stats, Observability
    ///
    typedef struct AllocatorStats {
        u64 bytes_requested;
        u64 bytes_in_use;
        u64 peak_bytes_in_use;
        u64 allocations;
        u64 reallocations;
        u64 deallocations;
        u64 failed_allocations;
    } AllocatorStats;
#endif

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
        u64                   __magic;
#if MISRA_HAVE_ALLOC_STATS
        AllocatorStats        stats;
#endif
    };

#if MISRA_HAVE_ALLOC_STATS
    ///
    /// Snapshot the current stats off `self`. Returns the struct by
    /// value; reading does not perturb the counters.
    ///
    AllocatorStats AllocatorGetStats(const Allocator *self);

    ///
    /// Zero every counter on `self`. `peak_bytes_in_use` is reset to
    /// the current `bytes_in_use` so subsequent peak tracking is
    /// monotonically correct from this point forward.
    ///
    void AllocatorResetStats(Allocator *self);
#endif

    ///
    /// Magic sentinel used to identify a properly-initialized allocator
    /// base at runtime. Every typed allocator's `*Init` macro stamps this
    /// into the embedded `Allocator.base.__magic`. `ValidateAllocator`
    /// checks it before dispatching through the function pointers.
    ///
    /// The sentinel both catches use-of-uninitialized-allocator and acts
    /// as a weak type-confusion guard - random pointers reinterpreted as
    /// `Allocator *` will almost never match.
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
    void *AllocatorAlloc(Allocator *self, size bytes, i8 zeroed);

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
/// Convert any allocator pointer to `Allocator *`. The argument may be:
///
///   - a typed allocator pointer (`HeapAllocator *`, `PageAllocator *`,
///     `ArenaAllocator *`, `SlabAllocator *`, `BudgetAllocator *`),
///     in which case the macro
///     typecasts the whole pointer to `Allocator *`. The cast is safe
///     because every typed allocator carries `Allocator base` at offset
///     zero — the C-style inheritance contract.
///
///   - a raw `Allocator *`, which is returned unchanged.
///
/// Any other pointer type triggers a `_Generic` mismatch at compile
/// time, which is the type-safety check the macro layer provides.
///
/// Callers do not write `&heap.base` or `(Allocator *)&heap` by hand —
/// every macro that takes an allocator pointer routes it through this
/// macro first.
///
/// USAGE:
///
///     HeapAllocator heap = HeapAllocatorInit();
///     Vec(int) v = VecInit(&heap);   // macro internally does ALLOCATOR_OF(&heap)
///
///     void library_helper(Vec *v, Allocator *alloc) {
///         Vec(int) scratch = VecInit(alloc);   // raw Allocator* also accepted
///     }
///
/// Adding a new typed allocator requires adding it to this whitelist
/// (and forward-declaring it above).
///
#define ALLOCATOR_OF(allocator_ptr)                                                                                    \
    _Generic(                                                                                                          \
        (allocator_ptr),                                                                                               \
        Allocator *: (allocator_ptr),                                                                                  \
        HeapAllocator *: (Allocator *)(allocator_ptr),                                                                 \
        PageAllocator *: (Allocator *)(allocator_ptr),                                                                 \
        ArenaAllocator *: (Allocator *)(allocator_ptr),                                                                \
        SlabAllocator *: (Allocator *)(allocator_ptr),                                                                 \
        BudgetAllocator *: (Allocator *)(allocator_ptr)                                                                \
    )

// Typed allocator headers (PageAllocator, HeapAllocator, ArenaAllocator,
// SlabAllocator) are NOT included here to avoid include-guard cycles:
// each typed header includes this one to get the `Allocator` base, and
// some embed `PageAllocator`. Users / library .c files must include the
// specific typed allocator they need:
//
//   #include <Misra/Std/Allocator/Heap.h>
//
// The Container umbrella `<Misra/Std/Container.h>` does not transitively
// pull these in either - including allocators is the caller's choice.

///
/// Reserved identifier used by Scope-aware public macros to find the
/// "current" allocator. The `Scope` and `ScopeWith` macros introduce this
/// name into the enclosing block; tier-1 public macros (`VecInit`, ...)
/// expand to references to it. Outside any `Scope`/`ScopeWith` block the
/// name is undeclared and the call site is a compile error.
///
/// Do not declare a local variable with this name in user code.
///
#define MisraScope __misra_scope_alloc

///
/// Open a fresh-allocator scope. Constructs TWO independent `AllocType`
/// instances on the stack:
///
///   - the **user-visible** allocator, exposed as `name`, for the
///     caller to pass deliberately to helpers and to use through
///     `ScopeWith(name) { ... }` blocks when they want allocations
///     to land in the user pool.
///   - the **internal** allocator, aliased as `MisraScope`, used by
///     all tier-1 library macros (`VecInit`, `StrInitFromCstr`, ...).
///
/// Library scratch allocations and the user's named-pool allocations
/// therefore never share a backing pool by default. Both instances are
/// destroyed together when the block exits, so anything allocated
/// through either pool is invalid memory after the block.
///
/// USAGE:
///   Scope(lifetimeA, DefaultAllocator) {
///       Vec(int) v = VecInit();          // INTERNAL pool (via MisraScope)
///       my_helper(&v, lifetimeA);        // helper receives USER pool pointer
///   }
///
/// CONTROL FLOW: normal fall-through, `break` (or `ExitScope`), and
/// `continue` at the scope's top level all run the auto-deinit cleanly.
/// `return` and `goto` out of the scope skip deinit and leak both
/// allocators - a C-level limitation that has no portable workaround.
///
/// TAGS: Allocator, Scope, Lifetime
///
#define Scope(name, AllocType)                                                                                         \
    for (AllocType _scope_user_##name     = AllocType##Init(),                                                         \
                   _scope_internal_##name = AllocType##Init(),                                                         \
                   *_scope_loop_##name    = &_scope_user_##name;                                                       \
         _scope_loop_##name;                                                                                           \
         AllocType##Deinit(&_scope_internal_##name),                                                                   \
                   AllocType##Deinit(&_scope_user_##name),                                                             \
                   _scope_loop_##name = NULL)                                                                          \
        for (Allocator *name               = &_scope_user_##name.base,                                                 \
                       *MisraScope         = &_scope_internal_##name.base,                                             \
                       *_scope_done_##name = name;                                                                     \
             _scope_done_##name;                                                                                       \
             _scope_done_##name = NULL)

///
/// Open a scope that borrows an already-initialized allocator pointer.
/// The pointer is exposed as `MisraScope` for the duration of the block.
/// Nothing is destroyed on block exit - the caller still owns the
/// allocator.
///
/// USAGE: typical helper pattern -
///
///   void my_helper(Vec(int) *v, Allocator *alloc) {
///       ScopeWith(alloc) {
///           Str scratch = StrInitFromCstr("hi", 2);
///           StrDeinit(&scratch);
///       }
///   }
///
/// TAGS: Allocator, Scope, Lifetime
///
#define ScopeWith(alloc_ptr)                                                                                           \
    for (Allocator *MisraScope = (alloc_ptr), *_scope_with_done = MisraScope; _scope_with_done; _scope_with_done = NULL)

///
/// Early-exit the nearest enclosing `Scope` / `ScopeWith` block, running
/// auto-deinit cleanly when used inside a `Scope`. Equivalent to a plain
/// `break`. Like any C `break`, it escapes only the innermost enclosing
/// loop - if you are inside a user `for`/`while` inside `Scope`, exit
/// your loop first and then `ExitScope`.
///
/// TAGS: Allocator, Scope, Control-Flow
///
#define ExitScope break

#endif // MISRA_STD_ALLOCATOR_H
