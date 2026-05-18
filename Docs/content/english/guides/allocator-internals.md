---
title: "Allocator Internals: Bitmap-Backed, Validation-First"
date: 2026-05-18
description: "How MisraStdC's small-object allocators are laid out, why the freelist allocator they replaced was a design problem rather than an implementation problem, and the limitations of the current shape."
authors:
  - siddharth-mishra
tags:
  - design
  - allocators
  - bitmaps
  - state-machines
---

This document describes the current internal design of MisraStdC's small-object allocators -- `HeapAllocator`, `SlabAllocator`, and `BudgetAllocator` -- and why they look the way they do. The shape is not a claim that this is the *best* allocator design; it is the design that fell out of one specific constraint, given the time and complexity budget the project has.

The constraint is: **if a category of bug can happen, we treat that as a design issue, not as something to patch around with discipline.** The previous freelist-based allocators in this project had one such category, and the rewrite traded away some performance and code compactness to remove it. The new design has its own potential issues, called out at the end of this document.

## The Bug That Triggered The Redesign

The previous fixed-size pool allocator was the textbook freelist shape:

```c
// Alloc: pop a slot from the freelist head.
void *alloc(void) {
    Slot *s = freelist_head;
    freelist_head = s->next;
    return s;
}

// Free: push a slot onto the freelist head.
void free(void *p) {
    Slot *s = p;
    s->next = freelist_head;
    freelist_head = s;
}
```

The compactness is the point of the design. It is also the source of the problem: while a slot is free, the first `sizeof(void *)` bytes of it are the allocator's `next` pointer; while the slot is in use, those same bytes belong to the caller. The same address holds two different *kinds* of value at different times in its life.

When `free` is called with a pointer the allocator didn't hand out (a foreign pointer, a stale pointer, a pointer offset into the middle of a slot, or just a stack address), the line `s->next = freelist_head` writes the freelist head into 8 bytes the allocator does not own. The next allocation interprets those bytes as a slot's `next` pointer; the allocation after that returns whatever address those bytes encoded as if it were a fresh slot.

This is the well-known "freelist primitive" used in heap-exploitation literature. The point worth making here is that it is not a bug in any one line of code -- it is the natural consequence of the shape "embed a pointer in user memory while the slot is unused". Any implementation of that shape has this problem. Magic tags, freelist cycle detection, or per-slot canaries narrow the surface; they do not remove the category.

That left two options:

1. Keep the freelist and add enough patching to make exploitation hard.
2. Pick a different shape.

The redesign picked (2). The trade was roughly:

- **More code.** The bitmap implementation is several hundred lines longer than the freelist it replaced (more validation, more bookkeeping, per-class layout tables).
- **A slower free path in principle.** Old free was O(1) freelist push. New free is O(log N) for `Heap` (binary search over the per-class descriptor array) and O(N) for `Slab` / `Budget`. In practice N stays small enough that the difference is unmeasurable, but it is no longer constant.
- **No memory regression per page.** The old design burned one full slot per page on a `HeapPageChunk` header at offset 0; the new design has zero header in the user page (the 24-byte descriptor lives in a separate, page-backed array), so per-page user capacity went up by one slot across every bin and the per-page metadata budget went down on most bins.

The decision was driven by the rule above: a bug whose root cause is the *shape* of the design is not a bug we want to patch around. The slower free path and the extra code were considered acceptable; the memory regression turned out not to exist.

## The Resulting Shape

The replacement is described in one line: **allocator state lives in memory the allocator owns, and user pointers point at memory the allocator has handed out. These two regions are disjoint.**

That sentence is the only invariant the rest of the design enforces. It does not mean the design is correct in every other respect, and it does not protect against bug classes outside this one. The "Limitations and Open Questions" section at the end covers what it does not catch.

## The Three Allocators

### `HeapAllocator` -- four size classes

`HeapAllocator` is the general-purpose backing for `DefaultAllocator`. Requests are partitioned into four size classes:

| class | sizes served    | layout per 4 KiB user page                                                          |
|-------|-----------------|-------------------------------------------------------------------------------------|
| S     | 16 / 32 / 64    | 64 × 16-B @ 0   +  32 × 32-B @ 1024  +  32 × 64-B @ 2048                            |
| M     | 128 / 256 / 512 | 8 × 128-B @ 0   +  4 × 256-B @ 1024  +  4 × 512-B @ 2048                            |
| L     | 1024 / 2048     | 2 × 1024-B @ 0  +  1 × 2048-B @ 2048                                                |
| XL    | > 2048          | one page-rounded mmap per allocation                                                |

The choice of split inside each class is a tradeoff. The class-S split (64 / 32 / 32) was picked because it makes the bitmap fit in exactly `u64 + u32 + u32` = 128 bits with one `ctz` instruction per sub-bin. A different split (more 16-byte slots, fewer 32 and 64) would suit a workload heavy on small allocations and lose efficiency for everything else. The current split assumes balanced mixed-size workloads. If a real workload skews heavily in one direction, the class layout becomes a bad fit -- this is one of the open questions.

Each user page is dedicated to one class for its lifetime and contains *only* slot bytes. The per-page bitmap lives in a separate descriptor:

```c
typedef struct HeapPageS {
    void *page;        // user page address
    u64   bitmap_16;   // 64 bits, 1 = in use
    u32   bitmap_32;   // 32 bits
    u32   bitmap_64;   // 32 bits
} HeapPageS;           // 24 bytes total
```

Classes M and L pack their sub-bins into a single `u16` or `u8` bitmap. `HeapAllocator` carries four descriptor arrays, one per class, kept sorted by page address:

```c
struct HeapAllocator {
    Allocator     base;
    PageAllocator page;
    HeapPageS  *s;   u32 s_len;  u32 s_cap;
    HeapPageM  *m;   u32 m_len;  u32 m_cap;
    HeapPageL  *l;   u32 l_len;  u32 l_cap;
    HeapPageXL *xl;  u32 xl_len; u32 xl_cap;
};
```

The descriptor arrays themselves are page-backed via the embedded `PageAllocator`.

### `SlabAllocator` -- one slot size, multi-chunk

A `SlabAllocator` serves one configured slot size, growing by appending fresh page-backed chunks as needed. Each chunk is laid out as:

```
[ chunk header | u64 bitmap | alignment pad | slot 0 | slot 1 | ... | slot N-1 ]
```

The header (linked-list link, raw pointer, sizes) and bitmap occupy the front of the chunk. Slots occupy the back. They are disjoint byte ranges.

### `BudgetAllocator` -- caller-buffer, hard cap

A `BudgetAllocator` lives over a caller-supplied buffer with no growth path. The buffer is partitioned at init:

```
[ u64 bitmap | alignment pad | slot 0 | slot 1 | ... | slot N-1 ]
```

The bitmap sits at the front of the caller buffer. Both halves are in caller-owned memory; what matters is that they occupy disjoint byte ranges and `alloc` only ever returns addresses inside the slot range.

### The Two Allocators That Did Not Need Changes

`ArenaAllocator` was already safe by construction. Its `free` is a no-op unless the freed pointer matches the most recent bump, and any foreign free is silently a no-op. There is no metadata write through the user pointer.

`PageAllocator` is a thin syscall wrapper. Its `Free` is `munmap`. The contract is "the caller knows what they own", which is the contract of `munmap` itself. The layering rule for the project is that user code does not call `PageAllocator.Free` directly -- only the higher-level allocators do, and they validate ownership before forwarding. That rule is policy, not enforced by code; it is one of the open questions.

`DebugAllocator` already tracked every allocation in an out-of-band map and detected foreign free, double-free, leaks, and buffer overflow on its own. It was not touched.

## The Slot State Machine

Every slot is in one of two states:

```
       page allocated
             │
             ▼
       ┌──────────┐     Alloc(size)              ┌──────────┐
       │          │     pre:  bit == 0           │          │
       │          │  ─────────────────────────▶  │          │
       │          │     post: bit := 1           │          │
       │   FREE   │                              │  IN_USE  │
       │          │     Free(ptr)                │          │
       │          │     pre:  bit == 1           │          │
       │          │           ptr is slot base,  │          │
       │          │  ◀─────── in region, aligned │          │
       │          │     post: bit := 0           │          │
       └────┬─────┘                              └─────┬────┘
            │                                          │
            │ Alloc found bit == 1                bad  │
            │ (bitmap corruption)                 Free │ (foreign /
            │                                          │  misaligned /
            │                                          │  double-free /
            ▼                                          ▼  mid-allocation)
       ┌─────────────────────────────────────────────────────┐
       │   Aborted   →   LOG_FATAL  +  backtrace             │
       └─────────────────────────────────────────────────────┘
```


Both transitions check the precondition before mutating any bitmap. The alloc-side check is technically redundant because `ctz(~bitmap)` finds a 0 bit by construction; the redundant assert exists to catch bitmap corruption from outside the allocator. The free-side check is doing real work -- it is what catches double-free.

`Free` aborts via `LOG_FATAL` on any precondition failure. The choice is intentional: a bad free is a caller-side memory-safety bug, and continuing past it would leave the program in an undefined state. Aborting with a backtrace points directly at the call site. An earlier version of this design used `LOG_ERROR` + return; that turned out to be the wrong call because a silently-logged bad free leaves the door open for follow-up corruption.

## Tests as State-Machine Coverage

The test suites are split into two halves.

The **normal** half covers the state machine on valid input: alloc returns distinct pointers, free-then-alloc recycles the slot, filling a class triggers a fresh page, allocations across every sub-bin coexist without collision, zeroed allocations are actually zero, sizes round correctly, alignment is honored, two allocators on the same stack don't share state.

The **deadend** half covers every rejection edge -- one test per category:

- foreign pointer (alloc via heap A, free via heap B)
- double-free (alloc, free, free)
- misaligned pointer (`ptr + 1`)
- mid-allocation pointer in XL (`ptr + 128` of a large allocation)

A passing deadend test is one where `LOG_FATAL` fired. The test runner intercepts `Abort` via `setjmp/longjmp` and counts the test as passing if and only if the abort happened. If a future refactor weakens validation, the corresponding deadend will return normally and the test fails -- the deadends are the regression fence.

## Limitations and Open Questions

This is the part of the document the design is genuinely uncertain about.

**The free path uses linear search in two of three allocators.** Slab walks its chunk list; Budget is a single linear scan; Heap binary-searches a sorted array. For small-to-medium N this is fine. For workloads with thousands of chunks, Slab and Budget will be slower than the old O(1) freelist pop. There is no plan yet for what to do about that; the assumption is most allocator instances stay small.

**`Free` no longer takes a size.** An earlier version was `Free(ptr, bytes)` and routed to a size class based on the caller's `bytes`. CI surfaced a real bug -- the PDB parser stored the wrong size, the new bitmap Heap caught the mismatch -- and the diagnosis was that the API itself was the bug class: a free that asks the caller for an allocation-time fact is a free that can be lied to. The current `Free(ptr)` recovers the size class from where the pointer lies inside its page (`HEAP_*_OFFSET`), so wrong-size is structurally not a thing the caller can do.

**`PageAllocator` is a typed-API-only deallocator.** `mmap` / `VirtualFree` need the byte count to release a mapping; the kernel does not maintain a ptr→size table. Page therefore exposes `PageAllocatorFree(&page, ptr, bytes)` as a typed free that takes the size explicitly. Calling `AllocatorFree(&page.base, ptr)` aborts with `LOG_FATAL` -- generic dispatch through Page is a caller bug. Internal users (`Heap`, `Slab`, `Arena`) always know exact sizes and call the typed free directly.

**The class splits are workload-dependent.** Class S at 64/32/32 slots favors balanced workloads. Class M at 8/4/4 has the same shape. If a real workload allocates 1000 × 16-byte structs and nothing in 32-byte or 64-byte sub-bins, the bitmap is half-empty and we burn pages faster than necessary. A hybrid scheme (first page shared, overflow pages dedicated per sub-bin) was considered and not built. If workloads turn out to skew that way, the split is the first thing to revisit.

**The XL list grows unboundedly.** XL allocations are tracked one descriptor per allocation. A workload that does many large allocations grows the `xl` array indefinitely; there is no per-instance memory cap.

**`PageAllocatorFree` still trusts its caller on `bytes`.** Because Page does not maintain a ptr→size index, the typed free's `bytes` argument is taken at face value -- a wrong byte count unmaps too little (leak) or too much (silent corruption of an adjacent mapping). Internal users (Heap/Slab/Arena) always pass the exact mmap size they tracked. External callers should treat Page as a building block under a typed allocator; this is policy, not code.

**Use-after-free is not detected.** Once a slot's bit is cleared, the bytes in the slot remain whatever the previous caller wrote. A caller that holds a stale pointer and reads through it after free sees that data; if the slot has been re-allocated to a new caller, the stale read sees the new caller's data. `DebugAllocator` catches this via page-protect-on-free; the bitmap allocators on their own do not.

**The allocators are single-threaded by design.** Two threads sharing a `HeapAllocator` would race on the descriptor arrays and the bitmap mutations. The intended use is one allocator per work unit. If the project later grows a "shared allocator" requirement, the bitmap mutations would need atomic-set / atomic-clear with retry, and the descriptor-array growth would need a different shape entirely.

## Why This Is Written Down

Most of the cost of the rewrite was not the code change itself. It was figuring out that the freelist was a category-of-bug problem, not a fixable-line-of-code problem. The rule that came out of it -- "if the design enables a bug class, treat that as the bug" -- is more useful than the allocator design. Future audits in the project (containers, parsers, sys primitives) will apply the same rule: look for places where library state can be written through a user-supplied pointer, where untrusted input drives metadata mutation, where a precondition is checked after rather than before mutation. If similar patterns surface there, the fix will be a similar shape -- redesign, not patch.
