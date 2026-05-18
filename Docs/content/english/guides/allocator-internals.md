---
title: "Allocator Design: Bitmaps over Freelists"
date: 2026-05-18
description: "Why MisraStdC's allocator uses a bitmap instead of a freelist, and which bug class that removes."
authors:
  - siddharth-mishra
tags:
  - design
  - allocators
  - bitmaps
---

> **Note**: This post was drafted by an AI assistant under direction from the author. It is not first-hand writing; the design choices it describes are real, the prose explaining them is generated. Treat the technical content as the design talking, and the framing as a translation layer.

## The Freelist Problem

A classical small-object allocator stores its free-list pointer inside the free slot itself. When the slot is free, the first eight bytes are the allocator's `next` pointer; when the slot is in use, those eight bytes belong to the caller. The same address holds two different *kinds* of value at different times in its life.

This compactness is the point of the design, and it is also the bug. The slot is owned by the allocator while free and by the caller while in use, but the *bytes* are shared. If a caller writes through a pointer they shouldn't — a foreign pointer, a stale pointer, a stack address that happens to alias a freed slot — the line that pushes the slot onto the free-list happily writes the free-list head into eight bytes the allocator doesn't own. The next allocation reads those bytes as a `next` pointer. The allocation after that returns whatever address those bytes encoded, as if it were a fresh slot.

A category of memory-safety bugs — "wild write near a freelist allocator becomes write-anywhere" — is enabled by the shape of the allocator, not by any particular caller mistake.

## Bitmaps Move The Metadata Out

The alternative design splits the allocator's backing buffer into two disjoint regions:

```
   allocator-owned     user-owned
   [   bitmap   ]      [ slot 0 | slot 1 | ... | slot N-1 ]
        │                  ▲
        │                  │
        └── one bit per slot: 0 = free, 1 = in use
```

The bitmap occupies a fixed-size header at the start of the buffer; user code never receives its address. The slot region occupies the rest of the buffer and holds only user bytes. The code that finds, marks, and returns free slots reads and writes the bitmap, not the slot bytes.

All slots are the same size, so the slot a pointer belongs to is `(ptr - slot_region_base) / slot_size`. That index identifies the bit to flip on alloc and free, and it's how the allocator knows the size of any allocation without the caller having to tell it.

A wild write through a stale slot pointer can still corrupt the data the previous caller wrote there, but it cannot reach the bitmap, so it cannot redirect the next allocation to an attacker-chosen address.

## The Slot State Machine

Each slot's bit is in one of two states. The allocator transitions a slot between them on alloc and free, and asserts the bit's current value before flipping it.

```
       Alloc:   asserts bit == 0, then sets it to 1
   ┌──────┐ ─────────────────────────────────────▶ ┌────────┐
   │ FREE │                                        │ IN_USE │
   └──────┘ ◀───────────────────────────────────── └────────┘
        Free:   asserts bit == 1, then sets it to 0
```

If an assertion fails the program aborts with a backtrace at the offending call site. The two assertions catch different bugs.

The alloc-side assertion catches **bitmap corruption from outside**. A healthy bitmap scan finds a clear bit by construction, so the bit it's about to set should already be 0. If it isn't, somebody has written through the bitmap region without going through the allocator.

The free-side assertion catches **double-free**. The caller is freeing a slot whose bit is already 0, meaning the slot was already returned. The check stops at the call site that asked for it.

## Bad Free Aborts. It Doesn't Return An Error.

A bad free is not an error condition; it's a memory-safety bug. The program is in an undefined state by the time the bad free happened — continuing past it is what turns the bug into a corruption. The allocator that detected it is the last well-defined frame on the stack, and that's where the backtrace needs to land.

"Log error and continue" is the wrong policy because a silently-logged bad free can be the first step in a longer corruption chain. Nothing aborts, so nothing anchors a backtrace, so nobody notices.

## What This Catches, And What It Doesn't

Caught and aborted:

- Double-free
- Free of a pointer the allocator doesn't own
- Free of a pointer offset into the middle of an allocation
- Free of a misaligned pointer
- Bitmap corruption from outside

Not caught:

- Use-after-free through a stale pointer that aliases into a re-allocated slot. The new contents are wrong but the allocator's bitmap is honest.
- Buffer overflow past the end of a slot into the next slot. Adjacent caller data is wrong; allocator metadata is fine.

Catching those requires extra structure with measurable cost: a sentinel byte pattern at the tail of every allocation that's verified on free, and freed regions made unreadable so a stale dereference traps with a segfault. Not appropriate for production, but useful as an opt-in mode for tests.

## The Same Rule, Applied To The API

Consider a free function that takes a size:

```c
free(allocator, ptr, size);
```

That size is an allocation-time fact: the caller has to remember the number they passed to `alloc` and pass the same number back at `free`. In practice it drifts. Records get copied, structs get refactored, a length gets stored once and then everything else stays in sync until it doesn't — and the bug only surfaces when the allocator starts validating it.

A free that doesn't ask:

```c
free(allocator, ptr);
```

The allocator already knows the size of the slot, because the index-from-pointer arithmetic above gave it. Recovering the size internally is cheaper than the bug class that came from asking the caller for it.

## The Generalization

**If a design enables a class of bug, treat the design as the bug.**

A freelist that stores its `next` pointer in user-reachable bytes is the design. A free API that takes a caller-supplied size is the design. In both cases the fix isn't to be more careful at the call sites; it's to remove the call-site burden that the bug class needed.

---

## How This Post Was Produced

The AI-assisted draft went through several review passes by a separate AI reviewer reading the post cold (no project context). Each pass surfaced framing problems: forward references to sections that didn't exist, jargon the post hadn't defined, defensive meta-commentary, tangents that broke the through-line, comparisons to past versions of the code a fresh reader couldn't see. Each pass produced a rewrite focused on those specific complaints. The author signed off when the substance held up and further reviewer notes had moved into stylistic taste.

The technical claims about the allocator design — bitmap layout, state machine, abort policy, API shape — match the code in the repository. The framing decisions about what to include, what order to put it in, and what to leave out are AI judgments shaped by the review loop above.
