# Mutation Testing as Gap Discovery

This project uses [Mull](https://github.com/mull-project/mull) for mutation
testing. This document defines **how we use it and how we read its results**,
because the obvious interpretation — "drive the mutation score to 100%" — is
wrong here and leads to brittle tests.

## The one rule

> A test documents a **contract**. It must fail when, and only when, the
> contract is violated — never when a valid implementation change keeps the
> contract but alters a strategy.

A *contract* is the observable input/output guarantee a caller depends on. For
a map: what you insert is retrievable with the right value and count; what you
remove is gone; the others survive; multi-valued keys count correctly; the
container stays correct under churn. A *strategy* is how the implementation
delivers that: growth schedule, tombstones vs. backward-shift deletion, probe
order, when and how much it rehashes. Breaking a strategy is, at worst, a
**performance** regression — not a contract violation.

## Mutation testing is discovery, not a target

Mull mutates the implementation one change at a time and reports which mutants
the tests **kill** (caught) and which **survive** (unnoticed). A survivor is a
change the suite is blind to. That is a *signal to investigate*, not a defect
to eliminate. **The score is a discovery tool; it is not a number to maximize.**

Every surviving mutant is triaged into exactly one bucket:

- **(A) Contract gap** — the mutated behaviour is observable at the API and a
  caller would be wrong, but no test looked. **Fix it: add a contract test.**
- **(B) Strategy / performance** — the mutation changes *how* the work is done
  (capacity numbers, tombstone counts, probe order) without changing any
  documented guarantee. **Accept the survivor.** Killing it would require
  asserting an implementation-chosen value, which over-fits the test to the
  current code.
- **(C) Equivalent** — the mutation cannot change observable behaviour at all
  (e.g. a constant swapped in an unreachable branch, or a rehash threshold that
  shifts by one insert where `next_capacity` returns the same size). **Accept
  the survivor.** No test can or should kill it.

Only **(A)** is a gap. **(B)** and **(C)** are accepted survivors by design.

## A litmus test for (A) vs (B)

> If you can only kill the mutant by asserting a specific number the
> *implementation chose* (a capacity, a tombstone count, a slot index), the
> mutant is **(B)**. Accept it.
>
> If you can kill it by asserting an outcome a *caller relies on* (a value
> comes back, a key is gone, a different key is still reachable), it is **(A)**.
> Write that test — phrased in caller-observable terms.

## Worked examples (Map)

| Mutated thing | Bucket | Why |
| --- | --- | --- |
| Key comparator bypassed on the probe path | **A** | Lookups return the wrong key's value — a caller is wrong. Killed by a forced-collision test that checks each key returns *its own* value. |
| Removing an interior collision-chain key breaks the chain | **A** | A *different*, still-present key becomes unreachable — silent data loss. Killed by removing an interior key and asserting a later key is still found. |
| Default growth schedule (`8 → 16 → 32`, load-factor constants) | **B** | The map still inserts and retrieves everything; a different schedule is only slower. Asserted only as "N inserts, all N retrievable", never as a capacity sequence. |
| Exact `MapTombstones` count / "the slot is reused" / capacity unchanged | **B** | A backward-shift-delete map keeps zero tombstones and is equally correct. Asserted only as remove/reinsert *working*, never as counts. |
| Constant swap in a capacity-0 early return | **C** | `0 >= 0` and `42 >= 0` both mean "not found". Unobservable. |
| One-past-the-end loop read (`idx < cap` → `<=`) | **deferred** | A real memory-safety change, but invisible to value assertions and to compiler ASAN (the containers use the project allocator, not libc malloc). See *Deferred* below. |

`MapTombstones()` / `MapCapacity()` are **diagnostic** accessors: tests may read
them to verify wiring, but must not assert specific values as a proxy for
correctness. The exception is an operation whose *contract is about that value*
— e.g. `MapCompact()` documents "tombstones are removed", so asserting
`MapTombstones() == 0` after `MapCompact()` is a contract test, not a strategy
test.

## Running it

`Scripts/mutation.sh [Component ...]` mutation-tests each component scoped to
its own source (via a generated `MULL_CONFIG`), runs that component's suites,
and reports a per-suite score. With no arguments it runs the default component
set. Locally use `nix develop .#mutation`; CI installs clang + mull from
packages (see `.github/workflows/mutation.yml`). The full sweep is heavy (one
library build per component), so CI runs it nightly / on demand, not per-PR.

A score going **down** after a test is loosened from strategy to contract is the
**correct** outcome — the suite now documents the guarantee instead of casting
the current implementation.

## Deferred: memory-safety survivors

One-past-the-end reads (loop-bound mutants) survive because they are invisible
to value assertions, and compiler AddressSanitizer cannot see them — the
containers run on the project's own allocator, not libc `malloc`. The right
tool is the project's page-backed `DebugAllocator`, which traps such reads.
Wiring it into the mull build is **deferred until after the allocator rewrite**;
do not add it before then. Until it lands, these are accepted survivors.
