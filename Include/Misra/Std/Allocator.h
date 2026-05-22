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
    typedef struct DebugAllocator  DebugAllocator;

    // `zeroed` uses `i8` (signed char) directly instead of `bool` to
    // sidestep TU-to-TU `bool` ambiguity on platforms that transitively
    // pull in `<stdbool.h>` from system headers. See the comment block
    // around the `bool` typedef in `Misra/Types.h`. Same reason
    // `resize` returns `i8` (1 = succeeded in-place, 0 = could not).
    typedef void *(*AllocatorAllocateFn)(Allocator *self, size bytes, i8 zeroed);
    // `resize`, `remap`, and `deallocate` all recover the existing
    // allocation size from the allocator's own bookkeeping -- callers
    // do not pass it. Lying about an allocation-time fact is no longer
    // possible at the API boundary.
    typedef i8 (*AllocatorResizeFn)(Allocator *self, void *ptr, size new_size);
    typedef void *(*AllocatorRemapFn)(Allocator *self, void *ptr, size new_size);
    typedef size (*AllocatorDeallocateFn)(Allocator *self, void *ptr);

#if FEATURE_ALLOC_STATS
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
        AllocatorAllocateFn allocate;
        // `resize` tries to grow / shrink the existing allocation in
        // place. The pointer never moves. Returns 1 on success, 0 if
        // the allocator can't satisfy the request without relocating
        // (in which case the caller can decide whether to fall back
        // to `remap` or give up).
        AllocatorResizeFn resize;
        // `remap` may move the allocation. Returns the new pointer
        // (possibly equal to `ptr` if the allocator could grow in
        // place anyway), or NULL on failure. Equivalent to the
        // realloc-shaped convenience that's been here since v1.
        AllocatorRemapFn      remap;
        AllocatorDeallocateFn deallocate;
        size                  alignment;
        AllocatorEffort       effort;
        u32                   retry_limit;
        u64                   __magic;
#if FEATURE_ALLOC_STATS
        AllocatorStats stats;
#endif
    };

#if FEATURE_ALLOC_STATS
    ///
    /// Snapshot the current stats off `self`. Returns the struct by
    /// value; reading does not perturb the counters. `self` is run
    /// through `ValidateAllocator` first, so a structurally invalid
    /// allocator aborts before the read.
    ///
    /// self[in] : Allocator base to query.
    ///
    /// SUCCESS: Returns a by-value copy of `self->stats`. Counter
    ///          state on `self` is unchanged.
    /// FAILURE: Does not return - `ValidateAllocator` aborts via
    ///          `LOG_FATAL` when `self` is NULL or structurally invalid.
    ///
    /// TAGS: Allocator, Stats, Observability
    ///
    AllocatorStats AllocatorGetStats(const Allocator *self);

    ///
    /// Zero every counter on `self`. `peak_bytes_in_use` is reset to
    /// the current `bytes_in_use` so subsequent peak tracking is
    /// monotonically correct from this point forward. `self` is run
    /// through `ValidateAllocator` first, so a structurally invalid
    /// allocator aborts before any state is touched.
    ///
    /// self[in,out] : Allocator base whose counters are reset.
    ///
    /// SUCCESS: Function returns. All counters except `bytes_in_use`
    ///          and `peak_bytes_in_use` are zero; both of those equal
    ///          the pre-call `bytes_in_use`.
    /// FAILURE: Does not return - `ValidateAllocator` aborts via
    ///          `LOG_FATAL` when `self` is NULL or structurally invalid.
    ///
    /// TAGS: Allocator, Stats, Observability
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
    void *AllocatorAlloc_dyn(Allocator *self, size bytes, i8 zeroed);

    ///
    /// Try to grow / shrink an allocation in place. The pointer never
    /// moves. Use this when the caller can NOT tolerate a relocation
    /// (e.g. external code holds pointers into the buffer). Returns 1
    /// when the allocator could satisfy the new size without moving;
    /// returns 0 when the caller should either fall back to
    /// `AllocatorRemap` (accepting a possible move) or give up.
    ///
    /// self[in,out] : Allocator base.
    /// ptr[in]      : Existing allocation pointer (must be non-NULL --
    ///                resize of nothing is meaningless).
    /// new_size[in] : Requested new allocation size in bytes (must be
    ///                non-zero -- shrink-to-zero is a free, not a resize).
    ///
    /// The previous allocation size is recovered from the allocator's
    /// own bookkeeping. Callers do not pass it.
    ///
    /// SUCCESS: Returns 1. `ptr` remains valid for `new_size` bytes; if
    ///          growing, new bytes are uninitialised.
    /// FAILURE: Returns 0. The allocation is unchanged.
    ///
    /// TAGS: Allocator, Memory, InPlace
    ///
    i8 AllocatorResize_dyn(Allocator *self, void *ptr, size new_size);

    ///
    /// Resize an allocation, allowing relocation. May return a new
    /// pointer that differs from `ptr`. Equivalent to C's realloc
    /// minus the C99-style NULL-on-shrink-failure convention: if
    /// `new_size == 0` the allocation is freed and NULL returned.
    ///
    /// self[in,out]  : Allocator base.
    /// ptr[in]       : Existing allocation pointer, or NULL (then this
    ///                 behaves like AllocatorAlloc(self, new_size, 0)).
    /// new_size[in]  : Requested new allocation size in bytes.
    ///
    /// The previous allocation size is recovered from the allocator's
    /// own bookkeeping. Callers do not pass it.
    ///
    /// SUCCESS: Returns the (possibly moved) pointer, or NULL when
    ///          `new_size` is zero.
    /// FAILURE: Returns NULL when the underlying allocator can't
    ///          satisfy the request.
    ///
    /// TAGS: Allocator, Memory, Reallocation
    ///
    void *AllocatorRemap_dyn(Allocator *self, void *ptr, size new_size);

    ///
    /// Convenience cascade: tries `AllocatorResize` first; on failure
    /// falls back to `AllocatorRemap`. The realloc-shaped entry point
    /// that's been in the API since v1 -- semantics unchanged, but
    /// callers that need the in-place guarantee should now use
    /// `AllocatorResize` directly.
    ///
    /// SUCCESS: Returns the (possibly moved) pointer, or NULL when
    ///          `new_size` is zero.
    /// FAILURE: Returns NULL when reallocation fails.
    ///
    /// TAGS: Allocator, Memory, Reallocation
    ///
    void *AllocatorRealloc_dyn(Allocator *self, void *ptr, size new_size);

    ///
    /// Free memory through an allocator.
    ///
    /// self[in,out] : Allocator base that issued the original allocation.
    /// ptr[in]      : Pointer to the allocation, or NULL.
    ///
    /// The allocator recovers the original allocation size from its own
    /// bookkeeping -- callers do not pass it. Stats accounting reads
    /// the freed-byte count from the dispatch return value.
    ///
    /// SUCCESS: Function returns. The allocation is reclaimed.
    /// FAILURE: No action is taken when `ptr` is NULL. A `ptr` that the
    ///          allocator does not own / has already freed / does not
    ///          point at an allocation's base aborts via `LOG_FATAL`.
    ///
    /// TAGS: Allocator, Memory, Deallocation
    ///
    void AllocatorFree_dyn(Allocator *self, void *ptr);

    // Typed-dispatch leaves. Forward-declared here so the AllocatorAlloc
    // / Resize / Remap / Free macros below can name them in every arm
    // of their _Generic. Definitions live in each typed allocator's .c
    // file; full declarations also appear in the typed allocator's own
    // header. Duplicating here is intentional: consumers that include
    // only <Misra/Std/Allocator.h> still get the dispatch macros.
    void *heap_allocator_allocate(Allocator *self, size bytes, i8 zeroed);
    i8    heap_allocator_resize(Allocator *self, void *ptr, size new_size);
    void *heap_allocator_remap(Allocator *self, void *ptr, size new_size);
    size  heap_allocator_deallocate(Allocator *self, void *ptr);

    void *page_allocator_allocate(Allocator *self, size bytes, i8 zeroed);
    i8    page_allocator_resize(Allocator *self, void *ptr, size new_size);
    void *page_allocator_remap(Allocator *self, void *ptr, size new_size);
    size  page_allocator_deallocate(Allocator *self, void *ptr);

    void *arena_allocator_allocate(Allocator *self, size bytes, i8 zeroed);
    i8    arena_allocator_resize(Allocator *self, void *ptr, size new_size);
    void *arena_allocator_remap(Allocator *self, void *ptr, size new_size);
    size  arena_allocator_deallocate(Allocator *self, void *ptr);

    void *slab_allocator_allocate(Allocator *self, size bytes, i8 zeroed);
    i8    slab_allocator_resize(Allocator *self, void *ptr, size new_size);
    void *slab_allocator_remap(Allocator *self, void *ptr, size new_size);
    size  slab_allocator_deallocate(Allocator *self, void *ptr);

    void *budget_allocator_allocate(Allocator *self, size bytes, i8 zeroed);
    i8    budget_allocator_resize(Allocator *self, void *ptr, size new_size);
    void *budget_allocator_remap(Allocator *self, void *ptr, size new_size);
    size  budget_allocator_deallocate(Allocator *self, void *ptr);

    void *debug_allocator_allocate(Allocator *self, size bytes, i8 zeroed);
    i8    debug_allocator_resize(Allocator *self, void *ptr, size new_size);
    void *debug_allocator_remap(Allocator *self, void *ptr, size new_size);
    size  debug_allocator_deallocate(Allocator *self, void *ptr);

    ///
    /// Validate an allocator base. Aborts via `LOG_FATAL` when the allocator
    /// is structurally invalid (NULL pointer, missing fn pointers, alignment
    /// is zero or not a power of two).
    ///
    /// self[in] : Allocator base to validate.
    ///
    /// SUCCESS: Function returns. The allocator is structurally valid.
    /// FAILURE: Does not return - aborts via `LOG_FATAL` / `Abort`.
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
/// Note for new code: the conventions doc (CODING-CONVENTIONS.md,
/// "_Generic dispatch") asks each macro to inline its own _Generic
/// instead of going through a shared "convert type X to Y" helper.
/// This macro pre-dates that rule. New dispatch macros in this file
/// (AllocatorAlloc / Resize / Remap / Realloc / Free) follow the
/// convention -- they inline `(Allocator *)(self)` per call. Two
/// external callers still use ALLOCATOR_OF directly; the macro is
/// kept for source compatibility until they migrate.
///
#define ALLOCATOR_OF(allocator_ptr)                                                                                    \
    _Generic(                                                                                                          \
        (allocator_ptr),                                                                                               \
        Allocator *: (allocator_ptr),                                                                                  \
        HeapAllocator *: (Allocator *)(allocator_ptr),                                                                 \
        PageAllocator *: (Allocator *)(allocator_ptr),                                                                 \
        ArenaAllocator *: (Allocator *)(allocator_ptr),                                                                \
        SlabAllocator *: (Allocator *)(allocator_ptr),                                                                 \
        BudgetAllocator *: (Allocator *)(allocator_ptr),                                                               \
        DebugAllocator *: (Allocator *)(allocator_ptr)                                                                 \
    )

///
/// Allocate memory through an allocator. The macro dispatches at
/// compile time on the static type of `self`: typed allocator
/// pointers (`HeapAllocator *`, ...) route straight to the concrete
/// `*_allocator_allocate`, skipping the `Allocator *` indirect call;
/// `Allocator *` falls through to the dynamic wrapper for the
/// type-erased path. Wrong types fail at the `_Generic` mismatch.
///
/// The typed direct path skips the dynamic wrapper's outer
/// `ValidateAllocator`, retry loop, and stats accounting. The typed
/// `*_allocator_*` body still self-validates (magic check etc.).
/// Callers that need stats accounting must take the `Allocator *`
/// route, which uses `AllocatorAlloc_dyn`.
///
/// self[in,out] : Typed allocator pointer or `Allocator *`.
/// bytes[in]    : Number of bytes to allocate.
/// zeroed[in]   : Whether the allocated region must be zero-initialized.
///
/// SUCCESS: Returns a writable pointer to allocated memory.
/// FAILURE: Returns NULL when allocation fails or `self` is invalid.
///
/// TAGS: Allocator, Memory, Allocation
///
#define AllocatorAlloc(self, bytes, zeroed)                                                                                                                                                                                                                                                                                \
    _Generic((self), HeapAllocator *: heap_allocator_allocate, PageAllocator *: page_allocator_allocate, ArenaAllocator *: arena_allocator_allocate, SlabAllocator *: slab_allocator_allocate, BudgetAllocator *: budget_allocator_allocate, DebugAllocator *: debug_allocator_allocate, Allocator *: AllocatorAlloc_dyn)( \
        (Allocator *)(self),                                                                                                                                                                                                                                                                                               \
        (bytes),                                                                                                                                                                                                                                                                                                           \
        (zeroed)                                                                                                                                                                                                                                                                                                           \
    )

///
/// Try to grow / shrink an allocation in place. The pointer never moves.
/// Same dispatch shape as `AllocatorAlloc`.
///
/// self[in,out] : Typed allocator pointer or `Allocator *`.
/// ptr[in]      : Existing allocation pointer (non-NULL).
/// new_size[in] : Requested new size in bytes (non-zero).
///
/// SUCCESS: Returns 1. `ptr` remains valid for `new_size` bytes.
/// FAILURE: Returns 0. The allocation is unchanged.
///
/// TAGS: Allocator, Memory, InPlace
///
#define AllocatorResize(self, ptr, new_size)                                                                                                                                                                                                                                                                    \
    _Generic((self), HeapAllocator *: heap_allocator_resize, PageAllocator *: page_allocator_resize, ArenaAllocator *: arena_allocator_resize, SlabAllocator *: slab_allocator_resize, BudgetAllocator *: budget_allocator_resize, DebugAllocator *: debug_allocator_resize, Allocator *: AllocatorResize_dyn)( \
        (Allocator *)(self),                                                                                                                                                                                                                                                                                    \
        (ptr),                                                                                                                                                                                                                                                                                                  \
        (new_size)                                                                                                                                                                                                                                                                                              \
    )

///
/// Resize an allocation, allowing relocation. May return a new pointer
/// that differs from `ptr`. Same dispatch shape as `AllocatorAlloc`.
///
/// self[in,out] : Typed allocator pointer or `Allocator *`.
/// ptr[in]      : Existing allocation pointer, or NULL.
/// new_size[in] : Requested new size in bytes.
///
/// SUCCESS: Returns the (possibly moved) pointer, or NULL when
///          `new_size` is zero.
/// FAILURE: Returns NULL when the allocator can't satisfy the request.
///
/// TAGS: Allocator, Memory, Reallocation
///
#define AllocatorRemap(self, ptr, new_size)                                                                                                                                                                                                                                                              \
    _Generic((self), HeapAllocator *: heap_allocator_remap, PageAllocator *: page_allocator_remap, ArenaAllocator *: arena_allocator_remap, SlabAllocator *: slab_allocator_remap, BudgetAllocator *: budget_allocator_remap, DebugAllocator *: debug_allocator_remap, Allocator *: AllocatorRemap_dyn)( \
        (Allocator *)(self),                                                                                                                                                                                                                                                                             \
        (ptr),                                                                                                                                                                                                                                                                                           \
        (new_size)                                                                                                                                                                                                                                                                                       \
    )

///
/// Convenience cascade: tries `AllocatorResize` first, falls back to
/// `AllocatorRemap`. Both sub-calls are `_Generic`-dispatched, so for
/// a typed pointer the whole cascade resolves to two typed-direct
/// function calls -- no indirect dispatch and no outer wrapper frame.
/// `Allocator *` routes both halves through the dyn wrappers, keeping
/// `ValidateAllocator` + stats accounting.
///
/// Macro hygiene: `self` is evaluated up to twice, `ptr` up to three
/// times, `new_size` up to three times. Pass simple lvalues; do not
/// pass expressions with side effects.
///
/// self[in,out] : Typed allocator pointer or `Allocator *`.
/// ptr[in]      : Existing allocation pointer, or NULL.
/// new_size[in] : Requested new size in bytes.
///
/// SUCCESS: Returns the (possibly moved) pointer, or NULL when
///          `new_size` is zero.
/// FAILURE: Returns NULL when reallocation fails.
///
/// TAGS: Allocator, Memory, Reallocation
///
#define AllocatorRealloc(self, ptr, new_size)                                                                          \
    (((ptr) && (new_size) > 0 && AllocatorResize((self), (ptr), (new_size))) ?                                         \
         (ptr) :                                                                                                       \
         AllocatorRemap((self), (ptr), (new_size)))

///
/// Free memory through an allocator. Typed paths dispatch directly to
/// the concrete `*_allocator_deallocate`, whose `size` return value is
/// the freed-byte count (discarded here -- stats live on the dynamic
/// wrapper path). Type-erased `Allocator *` routes through
/// `AllocatorFree_dyn` (with `ValidateAllocator` + stats).
///
/// self[in,out] : Typed allocator pointer or `Allocator *`.
/// ptr[in]      : Pointer to the allocation, or NULL.
///
/// SUCCESS: Function returns. The allocation is reclaimed.
/// FAILURE: No-op on NULL. A `ptr` the allocator does not own / has
///          already freed / does not point at an allocation's base
///          aborts via `LOG_FATAL`.
///
/// TAGS: Allocator, Memory, Deallocation
///
#define AllocatorFree(self, ptr)                                                                                                                                                                                                                                                                                                      \
    _Generic((self), HeapAllocator *: heap_allocator_deallocate, PageAllocator *: page_allocator_deallocate, ArenaAllocator *: arena_allocator_deallocate, SlabAllocator *: slab_allocator_deallocate, BudgetAllocator *: budget_allocator_deallocate, DebugAllocator *: debug_allocator_deallocate, Allocator *: AllocatorFree_dyn)( \
        (Allocator *)(self),                                                                                                                                                                                                                                                                                                          \
        (ptr)                                                                                                                                                                                                                                                                                                                         \
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
    for (AllocType UNPL(scope_user)     = AllocType##Init(),                                                           \
                   UNPL(scope_internal) = AllocType##Init(),                                                           \
                   *UNPL(scope_loop)    = &UNPL(scope_user);                                                           \
         UNPL(scope_loop);                                                                                             \
         AllocType##Deinit(&UNPL(scope_internal)), AllocType##Deinit(&UNPL(scope_user)), UNPL(scope_loop) = NULL)      \
        for (Allocator *name             = &UNPL(scope_user).base,                                                     \
                       *MisraScope       = &UNPL(scope_internal).base,                                                 \
                       *UNPL(scope_done) = name;                                                                       \
             (UNUSED(MisraScope), UNPL(scope_done));                                                                   \
             UNPL(scope_done) = NULL)

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
    for (Allocator *MisraScope = (alloc_ptr), *UNPL(scope_with_done) = MisraScope;                                     \
         (UNUSED(MisraScope), UNPL(scope_with_done));                                                                  \
         UNPL(scope_with_done) = NULL)

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
