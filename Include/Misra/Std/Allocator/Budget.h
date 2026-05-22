/// file      : std/allocator/budget.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Caller-buffer, fixed-budget pool allocator. Carves a user-provided
/// memory region into N fixed-size slots at init, where
/// N = floor(buf_bytes / padded_slot_size), and serves alloc/free out of
/// an intrusive free list over those slots. There is **no growth path**:
/// once the free list is empty, `AllocatorAlloc` returns NULL.
///
/// `BudgetAllocator` is stateless with respect to the OS: it never calls
/// `mmap` / `VirtualAlloc` / `malloc` and never embeds another
/// allocator. The caller fully controls the backing memory (stack
/// buffer, static buffer, region carved out of an arena, ...), which
/// makes this allocator suitable for embedded / freestanding contexts
/// and bounded scratch pools.
///
/// Use `BudgetAllocator` when you want a hard cap on memory consumption
/// for a specific slot class. Use `SlabAllocator` when you want the
/// same fixed-slot ergonomics but with on-demand growth.
///
/// USAGE:
///
///     u8              buf[4096];
///     BudgetAllocator bp = BudgetAllocatorInit(buf, sizeof(buf), 64);
///     void           *a  = AllocatorAlloc(ALLOCATOR_OF(&bp), 64, true);
///     ...
///     AllocatorFree(ALLOCATOR_OF(&bp), a);
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

    void *budget_allocator_allocate(Allocator *self, size bytes, i8 zeroed);
    i8    budget_allocator_resize(Allocator *self, void *ptr, size new_size);
    void *budget_allocator_remap(Allocator *self, void *ptr, size new_size);
    size  budget_allocator_deallocate(Allocator *self, void *ptr);

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
     (MemSet(                                                                                                          \
          PTR_ALIGN_UP_POW2((buf_ptr), 8u),                                                                            \
          0,                                                                                                           \
          CEIL_DIV(                                                                                                    \
              ((size)(total_bytes) - (size)(ALIGN_UP_POW2((u64)(buf_ptr), 8u) - (u64)(buf_ptr))) /                     \
                  ALIGN_UP_POW2((slot_size_bytes), (alignment_value)),                                                 \
              64u                                                                                                      \
          ) * sizeof(u64)                                                                                              \
      ),                                                                                                               \
      0),                                                                                                              \
     ((BudgetAllocator) {                                                                                              \
         .base =                                                                                                       \
             {.allocate    = budget_allocator_allocate,                                                                \
                    .resize      = budget_allocator_resize,                                                                  \
                    .remap       = budget_allocator_remap,                                                                   \
                    .deallocate  = budget_allocator_deallocate,                                                              \
                    .alignment   = (alignment_value),                                                                        \
                    .effort      = ALLOCATOR_EFFORT_ONCE,                                                                    \
                    .retry_limit = 0,                                                                                        \
                    .__magic     = BUDGET_ALLOCATOR_MAGIC},                                                                      \
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

#endif // MISRA_STD_ALLOCATOR_BUDGET_H
