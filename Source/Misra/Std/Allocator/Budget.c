/// file      : std/allocator/budget.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Caller-buffer, fixed-budget pool allocator implementation. Stateless
/// with respect to the OS - no `mmap` / `VirtualAlloc` / `malloc`, no
/// embedded allocator. Backing memory is owned by the caller.

#include <Misra/Std/Allocator/Budget.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Memory.h>

#include <stdint.h>

struct BudgetFreeSlot {
    struct BudgetFreeSlot *next;
};

static void budget_validate_self(const Allocator *self) {
    if (!self || self->__magic != MISRA_BUDGET_ALLOCATOR_MAGIC) {
        LOG_FATAL("type-confusion: allocator passed to budget_allocator_* is not a BudgetAllocator");
    }
}

static size budget_round_up(size value, size alignment) {
    return (value + (alignment - 1)) & ~(alignment - 1);
}

static bool budget_alignment_is_pow2(size alignment) {
    return alignment != 0 && ((alignment & (alignment - 1)) == 0);
}

void *budget_allocator_allocate(Allocator *self, size bytes, i8 zeroed) {
    budget_validate_self(self);
    BudgetAllocator *bp = (BudgetAllocator *)self;
    if (bytes > bp->slot_size) {
        return NULL;
    }
    if (!bp->free_head) {
        return NULL;
    }
    struct BudgetFreeSlot *slot = bp->free_head;
    bp->free_head               = slot->next;
    if (zeroed) {
        MemSet(slot, 0, bp->slot_size);
    }
    return slot;
}

void *budget_allocator_reallocate(Allocator *self, void *ptr, size old_size, size new_size) {
    budget_validate_self(self);
    BudgetAllocator *bp = (BudgetAllocator *)self;
    (void)old_size;

    if (!ptr) {
        return budget_allocator_allocate(self, new_size, true);
    }
    if (new_size == 0) {
        struct BudgetFreeSlot *slot = (struct BudgetFreeSlot *)ptr;
        slot->next                  = bp->free_head;
        bp->free_head               = slot;
        return NULL;
    }
    if (new_size <= bp->slot_size) {
        return ptr;
    }
    return NULL;
}

void budget_allocator_deallocate(Allocator *self, void *ptr, size bytes) {
    budget_validate_self(self);
    BudgetAllocator *bp = (BudgetAllocator *)self;
    (void)bytes;
    if (!ptr) {
        return;
    }
    struct BudgetFreeSlot *slot = (struct BudgetFreeSlot *)ptr;
    slot->next                  = bp->free_head;
    bp->free_head               = slot;
}

static BudgetAllocator budget_build(void *buf, size buf_bytes, size slot_size, size alignment) {
    BudgetAllocator empty = {0};
    if (!buf || !slot_size) {
        return empty;
    }
    if (!budget_alignment_is_pow2(alignment)) {
        alignment = sizeof(void *);
    }
    if (alignment < sizeof(void *)) {
        alignment = sizeof(void *);
    }

    size padded_slot = budget_round_up(slot_size, alignment);

    // Skip leading bytes so the first slot satisfies `alignment`.
    uintptr_t base_addr    = (uintptr_t)buf;
    uintptr_t aligned_addr = (base_addr + (uintptr_t)(alignment - 1)) & ~(uintptr_t)(alignment - 1);
    size      head_padding = (size)(aligned_addr - base_addr);
    if (head_padding >= buf_bytes) {
        return empty;
    }
    size usable_bytes = buf_bytes - head_padding;
    size slot_count   = usable_bytes / padded_slot;
    if (slot_count == 0) {
        return empty;
    }

    BudgetAllocator bp = {
        .base =
            {.allocate    = budget_allocator_allocate,
                   .reallocate  = budget_allocator_reallocate,
                   .deallocate  = budget_allocator_deallocate,
                   .alignment   = alignment,
                   .effort      = ALLOCATOR_EFFORT_ONCE,
                   .retry_limit = 0,
                   .__magic     = MISRA_BUDGET_ALLOCATOR_MAGIC},
        .buf        = (char *)buf,
        .buf_bytes  = buf_bytes,
        .slot_size  = padded_slot,
        .slot_count = slot_count,
        .free_head  = NULL,
    };

    char *cursor = (char *)(void *)aligned_addr;
    for (size i = 0; i < slot_count; i++) {
        struct BudgetFreeSlot *slot  = (struct BudgetFreeSlot *)(void *)cursor;
        slot->next                   = bp.free_head;
        bp.free_head                 = slot;
        cursor                      += padded_slot;
    }
    return bp;
}

BudgetAllocator BudgetAllocatorInit(void *buf, size buf_bytes, size slot_size) {
    return budget_build(buf, buf_bytes, slot_size, sizeof(void *));
}

BudgetAllocator BudgetAllocatorInitAligned(void *buf, size buf_bytes, size slot_size, size alignment) {
    return budget_build(buf, buf_bytes, slot_size, alignment);
}

void BudgetAllocatorDeinit(BudgetAllocator *self) {
    if (!self) {
        return;
    }
    self->buf          = NULL;
    self->buf_bytes    = 0;
    self->slot_size    = 0;
    self->slot_count   = 0;
    self->free_head    = NULL;
    self->base.__magic = 0u;
}
