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
///     AllocatorFree(ALLOCATOR_OF(&bp), a, 64);
///     BudgetAllocatorDeinit(&bp);  // no-op; the caller owns `buf`

#ifndef MISRA_STD_ALLOCATOR_BUDGET_H
#define MISRA_STD_ALLOCATOR_BUDGET_H

#include <Misra/Std/Allocator.h>

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

    typedef struct BudgetFreeSlot BudgetFreeSlot;

    ///
    /// Caller-buffer fixed-budget allocator. Carries `Allocator base` at
    /// offset 0 so `(Allocator *)&bp` is well-defined.
    ///
    /// FIELDS:
    /// - base       : Generic allocator base (function pointers, alignment, ...).
    /// - buf        : Pointer to the caller-owned memory region.
    /// - buf_bytes  : Size of `buf` in bytes.
    /// - slot_size  : Slot size in bytes (rounded up to `base.alignment`).
    /// - slot_count : Number of slots carved out of `buf`.
    /// - free_head  : Head of the intrusive free list.
    ///
    /// TAGS: Allocator, Budget, Pool, Memory
    ///
    typedef struct BudgetAllocator {
        Allocator       base;
        char           *buf;
        size            buf_bytes;
        size            slot_size;
        size            slot_count;
        BudgetFreeSlot *free_head;
    } BudgetAllocator;

    void *budget_allocator_allocate(Allocator *self, size bytes, i8 zeroed);
    void *budget_allocator_reallocate(Allocator *self, void *ptr, size old_size, size new_size);
    void  budget_allocator_deallocate(Allocator *self, void *ptr, size bytes);

    ///
    /// Initialize a `BudgetAllocator` over a caller-owned memory region.
    /// Carves `buf` into `floor(buf_bytes / padded_slot_size)` slots and
    /// builds an intrusive free list over them. `padded_slot_size` is
    /// `slot_size` rounded up to `sizeof(void *)` so each free slot can
    /// hold the intrusive next pointer.
    ///
    /// buf[in,out]      : Caller-owned memory region used as backing storage.
    /// buf_bytes[in]    : Size of `buf` in bytes.
    /// slot_size[in]    : Allocation size class served by this allocator.
    ///
    /// SUCCESS: Returns a fully-initialized `BudgetAllocator` value.
    /// FAILURE: Returns a zero-initialized allocator whose `__magic` is 0
    ///          when `buf` is NULL, `slot_size` is 0, or the buffer is too
    ///          small to hold a single padded slot. Calling
    ///          `AllocatorAlloc` on it will abort via `ValidateAllocator`.
    ///
    /// TAGS: Allocator, Budget, Init
    ///
    BudgetAllocator BudgetAllocatorInit(void *buf, size buf_bytes, size slot_size);

    ///
    /// Initialize a `BudgetAllocator` with an alignment floor. Slot size
    /// is rounded up to the larger of `alignment` and `sizeof(void *)`,
    /// and the first slot is positioned so that every slot satisfies the
    /// requested alignment.
    ///
    /// alignment[in] : Required slot alignment in bytes (power of two).
    ///
    /// Otherwise identical to `BudgetAllocatorInit`.
    ///
    /// TAGS: Allocator, Budget, Init, Alignment
    ///
    BudgetAllocator BudgetAllocatorInitAligned(void *buf, size buf_bytes, size slot_size, size alignment);

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
