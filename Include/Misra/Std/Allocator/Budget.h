/// file      : std/allocator/budget.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Caller-buffer, fixed-budget pool allocator. Carves a user-provided
/// memory region into N fixed-size slots at init, where
/// N = floor(buf_bytes / padded_slot_size), and serves alloc/free out
/// of a u64-word bitmap over those slots (ctz-scan for a free bit on
/// alloc; bit-clear + double-free check on free). There is **no
/// growth path**: once every bitmap bit is set, `AllocatorAlloc`
/// returns NULL.
///
/// `BudgetAllocator` is stateless with respect to the OS: it never calls
/// `mmap` / `VirtualAlloc` and never embeds another allocator. The
/// caller fully controls the backing memory (stack buffer, static
/// buffer, region carved out of an arena, ...), which makes this
/// allocator suitable for embedded / freestanding contexts and bounded
/// scratch pools.
///
/// Use `BudgetAllocator` when you want a hard cap on memory consumption
/// for a specific slot class. Use `SlabAllocator` when you want the
/// same fixed-slot ergonomics but with on-demand growth.
///
/// USAGE:
///
///     u8              buf[4096];
///     BudgetAllocator bp = BudgetAllocatorInit(buf, sizeof(buf), 64);
///     void           *a  = AllocatorAlloc(&bp, 64, true);
///     ...
///     AllocatorFree(&bp, a);
///     BudgetAllocatorDeinit(&bp);  // no-op; the caller owns `buf`

#ifndef MISRA_STD_ALLOCATOR_BUDGET_H
#define MISRA_STD_ALLOCATOR_BUDGET_H

#include <Misra/Std/Allocator.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Memory.h>

///
/// Per-type magic for `BudgetAllocator`. Stamped into
/// `Allocator.base.__magic` by `BudgetAllocatorInit*`. The budget
/// implementation functions validate this exact value so other
/// allocator instances reinterpreted as a `BudgetAllocator *` are
/// rejected at runtime as type-confusion.
///
#define BUDGET_ALLOCATOR_MAGIC MAKE_NEW_MAGIC_VALUE("budgetal")

#ifdef __cplusplus
extern "C" {
#endif

    ///
    /// Caller-buffer fixed-budget bitmap allocator. Carries `Allocator
    /// base` at offset 0 so `(Allocator *)&bp` is well-defined.
    ///
    /// The caller-provided buffer is partitioned at init into a bitmap
    /// region followed by a slot region. User pointers returned by
    /// Alloc always lie in the slot region; on Free the allocator
    /// validates the pointer against the slot range, alignment, and
    /// the bitmap state (catches foreign / misaligned / double-free
    /// without writing through the pointer).
    ///
    /// FIELDS:
    /// - base         : Generic allocator base (function pointers, alignment, ...).
    /// - buf          : Pointer to the caller-owned memory region.
    /// - buf_bytes    : Size of `buf` in bytes.
    /// - bitmap       : u64 bitmap (allocator-owned region inside `buf`).
    /// - bitmap_words : Number of u64 words in `bitmap`.
    /// - slots        : Start of the slot region inside `buf`.
    /// - slot_size    : Slot size in bytes (rounded up to `base.alignment`).
    /// - slot_count   : Number of slots carved out of `buf`.
    ///
    /// TAGS: Allocator, Budget, Pool, Memory
    ///
    typedef struct BudgetAllocator {
        Allocator base;
        u8       *buf;
        size      buf_bytes;
        u64      *bitmap;
        u32       bitmap_words;
        u8       *slots;
        size      slot_size;
        size      slot_count;
    } BudgetAllocator;

    ///
    /// Serve a fixed-budget slot. `bytes` must fit within the budget
    /// allocator's configured `slot_size` (set at init). The bitmap is
    /// scanned word-by-word via `ctz` for the first free bit; that
    /// bit is set and the corresponding slot returned.
    ///
    /// SUCCESS: Returns a writable, `base.alignment`-aligned pointer
    ///          to one slot. Zeroed when `zeroed` is non-zero.
    /// FAILURE: Returns NULL when every bitmap bit is set (pool full)
    ///          or `bytes` exceeds `slot_size` -- there is no growth
    ///          path for a `BudgetAllocator`.
    ///
    /// TAGS: Allocator, Budget, Memory, Allocation
    ///
    void *budget_allocator_allocate(BudgetAllocator *self, size bytes, i8 zeroed);

    ///
    /// In-place resize. Slots are fixed-size, so a resize succeeds iff
    /// `new_size` still fits in `slot_size`.
    ///
    /// SUCCESS: Returns 1 when `new_size <= slot_size`. The pointer
    ///          stays valid; no bitmap state changes.
    /// FAILURE: Returns 0 when `new_size > slot_size`. The slot is
    ///          unchanged.
    ///
    /// TAGS: Allocator, Budget, Memory, InPlace
    ///
    i8 budget_allocator_resize(BudgetAllocator *self, void *ptr, size new_size);

    ///
    /// Resize with relocation allowed. Slots are fixed-size, so any
    /// request that fits in `slot_size` returns the same pointer; any
    /// request larger than `slot_size` fails outright.
    ///
    /// SUCCESS: Returns `ptr` unchanged when `new_size <= slot_size`.
    ///          When `ptr` is NULL this behaves like
    ///          `budget_allocator_allocate(self, new_size, true)` --
    ///          fresh allocations from a remap-NULL are zeroed. When
    ///          `new_size == 0` the allocation is freed and NULL is
    ///          returned.
    /// FAILURE: Returns NULL when `new_size > slot_size`. The old
    ///          allocation is left untouched.
    ///
    /// TAGS: Allocator, Budget, Memory, Reallocation
    ///
    void *budget_allocator_remap(BudgetAllocator *self, void *ptr, size new_size);

    ///
    /// Free a slot. Computes the slot index from `ptr` and the slot
    /// region base, validates the pointer against the slot range and
    /// alignment, and clears the bitmap bit.
    ///
    /// SUCCESS: Returns `slot_size` (what stats accounting sees).
    /// FAILURE: Aborts via `LOG_FATAL` when `ptr` is foreign to this
    ///          allocator's slot region, mis-aligned, or its bitmap
    ///          bit is already clear (double-free).
    ///
    /// TAGS: Allocator, Budget, Memory, Deallocation
    ///
    size budget_allocator_deallocate(BudgetAllocator *self, void *ptr);

///
/// Initialize a `BudgetAllocator` over a caller-owned memory region.
/// Carves `buf` into a `[bitmap | pad | slots]` layout: the bitmap is
/// u64-aligned at the head; slots are `sizeof(void *)`-aligned and
/// padded up to `sizeof(void *)`. The bitmap region is zeroed in place
/// as part of init.
///
/// Side-effect-free args only -- `buf`, `buf_bytes`, and `slot_size`
/// are each evaluated multiple times inside the layout arithmetic.
///
/// buf[in,out]   : Caller-owned memory region used as backing storage.
/// buf_bytes[in] : Size of `buf` in bytes.
/// slot_size[in] : Allocation size class served by this allocator.
///
/// SUCCESS : Returns a fully-initialized `BudgetAllocator` value with
///           bitmap zeroed.
/// FAILURE : Aborts via `LOG_FATAL` when `buf` is NULL, `buf_bytes`
///           or `slot_size` is 0, or the buffer is too small for the
///           bitmap word plus one padded slot. The fatal log points at
///           the caller's source line.
///
/// TAGS: Allocator, Budget, Init
///
#define BudgetAllocatorInit(buf_ptr, total_bytes, slot_size_bytes)                                                     \
    BudgetAllocatorInitAligned((buf_ptr), (total_bytes), (slot_size_bytes), sizeof(void *))

///
/// Initialize a `BudgetAllocator` with a caller-supplied alignment.
/// `alignment` must be a power of two and at least `sizeof(void *)`;
/// slot size is rounded up to it. Otherwise identical to
/// `BudgetAllocatorInit`.
///
/// alignment[in] : Required slot alignment in bytes (power of two).
///
/// SUCCESS : Returns a fully-initialized `BudgetAllocator` value with
///           bitmap zeroed.
/// FAILURE : Aborts via `LOG_FATAL` on bad arguments (see
///           `BudgetAllocatorInit`) or when `alignment` is not a
///           power-of-two or is below `sizeof(void *)`.
///
/// TAGS: Allocator, Budget, Init, Alignment
///
#define BudgetAllocatorInitAligned(buf_ptr, total_bytes, slot_size_bytes, alignment_value)                             \
    (ASSERT_OR_FATAL((buf_ptr) != NULL, "BudgetAllocatorInit: NULL buf"),                                              \
     ASSERT_OR_FATAL((total_bytes) > 0, "BudgetAllocatorInit: zero buf_bytes"),                                        \
     ASSERT_OR_FATAL((slot_size_bytes) > 0, "BudgetAllocatorInit: zero slot_size"),                                    \
     ASSERT_OR_FATAL(                                                                                                  \
         (alignment_value) >= sizeof(void *) && ((alignment_value) & ((alignment_value) - 1)) == 0,                    \
         "BudgetAllocatorInit: alignment must be a power of two >= sizeof(void *)"                                     \
     ),                                                                                                                \
     ASSERT_OR_FATAL(                                                                                                  \
         (size)(total_bytes) >= (size)(ALIGN_UP_POW2((u64)(buf_ptr), 8u) - (u64)(buf_ptr)) + sizeof(u64) +             \
                                    ALIGN_UP_POW2((slot_size_bytes), (alignment_value)) + ((alignment_value) - 1),     \
         "BudgetAllocatorInit: buffer too small for bitmap + one padded slot"                                          \
     ),                                                                                                                \
     MemSet(                                                                                                           \
         PTR_ALIGN_UP_POW2((buf_ptr), 8u),                                                                             \
         0,                                                                                                            \
         CEIL_DIV(                                                                                                     \
             ((size)(total_bytes) - (size)(ALIGN_UP_POW2((u64)(buf_ptr), 8u) - (u64)(buf_ptr))) /                      \
                 ALIGN_UP_POW2((slot_size_bytes), (alignment_value)),                                                  \
             64u                                                                                                       \
         ) * sizeof(u64)                                                                                               \
     ),                                                                                                                \
     ((BudgetAllocator) {                                                                                              \
         .base =                                                                                                       \
             {.allocate        = (AllocatorAllocateFn)budget_allocator_allocate,                                       \
                    .resize          = (AllocatorResizeFn)budget_allocator_resize,                                           \
                    .remap           = (AllocatorRemapFn)budget_allocator_remap,                                             \
                    .deallocate      = (AllocatorDeallocateFn)budget_allocator_deallocate,                                   \
                    .alignment       = (alignment_value),                                                                    \
                    .effort          = ALLOCATOR_EFFORT_ONCE,                                                                \
                    .retry_limit     = 0,                                                                                    \
                    .__magic         = BUDGET_ALLOCATOR_MAGIC | MAGIC_VALIDATED_BIT,                                                               \
                    .footprint_bytes = 0},                                                                                   \
         .buf          = (u8 *)(buf_ptr),                                                                              \
         .buf_bytes    = (total_bytes),                                                                                \
         .bitmap       = (u64 *)PTR_ALIGN_UP_POW2((buf_ptr), 8u),                                                      \
         .bitmap_words = (u32)CEIL_DIV(                                                                                \
             ((size)(total_bytes) - (size)(ALIGN_UP_POW2((u64)(buf_ptr), 8u) - (u64)(buf_ptr))) /                      \
                 ALIGN_UP_POW2((slot_size_bytes), (alignment_value)),                                                  \
             64u                                                                                                       \
         ),                                                                                                            \
         .slots = (u8 *)ALIGN_UP_POW2(                                                                                 \
             ALIGN_UP_POW2((u64)(buf_ptr), 8u) +                                                                       \
                 CEIL_DIV(                                                                                             \
                     ((size)(total_bytes) - (size)(ALIGN_UP_POW2((u64)(buf_ptr), 8u) - (u64)(buf_ptr))) /              \
                         ALIGN_UP_POW2((slot_size_bytes), (alignment_value)),                                          \
                     64u                                                                                               \
                 ) * sizeof(u64),                                                                                      \
             (alignment_value)                                                                                         \
         ),                                                                                                            \
         .slot_size = ALIGN_UP_POW2((slot_size_bytes), (alignment_value)),                                             \
         .slot_count =                                                                                                 \
             ((size)(total_bytes) - (size)(ALIGN_UP_POW2((u64)(buf_ptr), 8u) - (u64)(buf_ptr)) -                       \
              CEIL_DIV(                                                                                                \
                  ((size)(total_bytes) - (size)(ALIGN_UP_POW2((u64)(buf_ptr), 8u) - (u64)(buf_ptr))) /                 \
                      ALIGN_UP_POW2((slot_size_bytes), (alignment_value)),                                             \
                  64u                                                                                                  \
              ) * sizeof(u64) -                                                                                        \
              (size)(ALIGN_UP_POW2(                                                                                    \
                         ALIGN_UP_POW2((u64)(buf_ptr), 8u) +                                                           \
                             CEIL_DIV(                                                                                 \
                                 ((size)(total_bytes) - (size)(ALIGN_UP_POW2((u64)(buf_ptr), 8u) - (u64)(buf_ptr))) /  \
                                     ALIGN_UP_POW2((slot_size_bytes), (alignment_value)),                              \
                                 64u                                                                                   \
                             ) * sizeof(u64),                                                                          \
                         (alignment_value)                                                                             \
                     ) -                                                                                               \
                     ALIGN_UP_POW2((u64)(buf_ptr), 8u) -                                                               \
                     CEIL_DIV(                                                                                         \
                         ((size)(total_bytes) - (size)(ALIGN_UP_POW2((u64)(buf_ptr), 8u) - (u64)(buf_ptr))) /          \
                             ALIGN_UP_POW2((slot_size_bytes), (alignment_value)),                                      \
                         64u                                                                                           \
                     ) * sizeof(u64))) /                                                                               \
             ALIGN_UP_POW2((slot_size_bytes), (alignment_value)),                                                      \
    }))

    ///
    /// Tear down a `BudgetAllocator`. No-op for the backing memory (the
    /// caller owns it); resets internal bookkeeping so subsequent
    /// dispatch calls will trip the magic check.
    ///
    /// self[in,out] : BudgetAllocator instance.
    ///
    /// SUCCESS: Function returns. Internal state is zeroed.
    /// FAILURE: No action when `self` is NULL.
    ///
    /// TAGS: Allocator, Budget, Cleanup
    ///
    void BudgetAllocatorDeinit(BudgetAllocator *self);

#ifdef __cplusplus
}
#endif

///
/// Total slot capacity carved out of the caller's buffer at init.
/// Fixed for the lifetime of the allocator: subtract live allocations
/// to get the remaining budget.
///
/// TAGS: Allocator, Budget, Query
///
#define BudgetAllocatorSlotCount(b) ((void)0, (b)->slot_count)

#endif // MISRA_STD_ALLOCATOR_BUDGET_H
