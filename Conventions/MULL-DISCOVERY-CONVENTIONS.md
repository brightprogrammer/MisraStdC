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

## The accepted-survivor ledger

Bucket-**(B)** and bucket-**(C)** survivors are accepted *by design* — but a
raw report re-lists them on every run, drowning out the bucket-**(A)** gaps that
actually need a test. The ledger records each accepted survivor once, and
`Scripts/mull-filter.py` removes it from the report so downstream readers only
see survivors that still need triage.

**Bucket A is NEVER ignorable.** A contract gap is a missing test, not a
survivor to suppress. The ledger and the filter both hard-reject any entry with
`bucket = "A"` (and any entry missing `bucket` or `rationale`) — the filter
exits non-zero with a clear message. Only `"B"` and `"C"` may appear.

### The ledger file — `Conventions/mull-ignores.toml`

One `[[ignore]]` stanza per accepted survivor:

| field | meaning |
| --- | --- |
| `id` | stable unique string (e.g. `MAP-REHASH-001`) |
| `file` | repo-relative path to the source file |
| selector | **exactly one** of `function = "name"`, `lines = [start, end]` (1-based inclusive), or `pattern = "regex"` |
| `mutator` | *optional* mull mutator id or glob (`cxx_*`); omitted = match any mutator |
| `bucket` | `"B"` or `"C"` only |
| `rationale` | why it's accepted, phrased in caller-contract terms |
| `author`, `created` | who triaged it and when (`YYYY-MM-DD`) |
| `region_hash` | `sha256:<hex>` of the normalised anchored region |

The selector names a source *region*. `function` covers the named C function's
full body (signature line through the matching close brace); `lines` covers an
explicit span; `pattern` covers the **set** of lines matching the regex.

### `region_hash` normalisation

`region_hash` is the SHA-256 of the region's lines after this per-line
normalisation, then joined with `\n`:

1. strip leading and trailing whitespace;
2. collapse every internal run of whitespace to a single space
   (a whitespace-only line becomes empty).

For a `pattern` selector the region is the set of matching lines in **ascending
file (line-number) order** — a deterministic ordering so the hash is stable.
Because whitespace is normalised away, a pure-reindent / reformat of the region
does **not** invalidate an entry; only a change to the region's *tokens* does.

### Invalidation — the git tie-in

Before filtering, the tool recomputes every entry's `region_hash` against the
**current working tree**. On a mismatch the entry is **STALE**: it suppresses
nothing, is listed loudly in the summary, and the filter **exits non-zero**. The
hash is the source of truth — a moved region must be re-triaged and its stanza
updated (new `region_hash`, refreshed `rationale` if the logic changed). As a
performance fast-path the `--check-range` mode may consult
`git diff --name-only <base>..<head>` to skip hashing entries in untouched
files, but the hash, not the diff, decides staleness.

### The filter — `Scripts/mull-filter.py`

`Scripts/mutation.sh` runs `mull-runner` with no `--reporters` flag, so mull
emits its default **IDE** reporter, which `mutation.sh` tees to
`build_mull/<comp>/mutation-<suite>.txt`. A survivor appears as
`…/File.c:LINE:COL: warning: Survived: <desc> [<mutator>]` followed by the
source echo and a caret. The filter keys off that warning line.

```
# Suppress accepted survivors and print a summary:
Scripts/mull-filter.py build_mull/Map/mutation-*.txt
#   -> writes report.filtered.txt; summary block reports
#      total / ignored(by id) / remaining / stale-ignores

# Enforcement hook (CI step or pre-commit): fail if a covered region moved
# in a commit range without its ledger stanza being updated:
Scripts/mull-filter.py --check-range <base>..<head>

# CI gate: suppress accepted survivors AND fail on a survivor that was
# introduced in this change's diff range:
Scripts/mull-filter.py --fail-on-new <base>..<head> build_mull/**/mutation-*.txt
```

Both modes exit non-zero on a hard ledger error (bucket A / missing field) or
on any stale entry; `0` means clean.

### What CI gates on (and what it never gates on)

The `.github/workflows/mutation.yml` job **fails the build** on exactly two
conditions, and nothing else:

- **(a) A stale ignore or a malformed ledger entry.** A `region_hash` mismatch
  (the anchored region moved) makes the entry STALE; a `bucket = "A"` entry, or
  one missing a required field, is a hard ledger error. The filter exits
  non-zero on both, unconditionally — independent of any diff. **Bucket A is
  never ignorable**: a contract gap is a missing test, not a survivor to
  suppress (see *The accepted-survivor ledger* above).
- **(b) A survivor newly introduced in the change.** With
  `--fail-on-new <base>..<head>`, the filter parses
  `git diff --unified=0 <base>..<head>` for the new-file (`+`) line ranges the
  change adds/touches, and fails if any **remaining** (non-ledgered, non-stale)
  survivor's reported line falls inside one of those ranges. A new gap you
  introduced gates your change; you triage it on the spot — add a contract test
  (bucket A) or, if it is genuinely strategy/equivalent, ledger it (bucket B/C).
  **Pre-existing survivors in unchanged code do not gate** — they are a backlog
  to whittle down, not a wall every unrelated change has to clear.

The job passes the diff baseline only when there is one: on `push` it uses
`github.event.before..github.sha`; on `schedule` / `workflow_dispatch` (no PR
diff) it runs the filter **without** a range, so only the stale / bucket-A gate
(a) applies. A first push / branch-create (empty or all-zeroes `before`) also
falls back to no-range gating. The diff baseline is purely informational about
*where* new code is; it never widens or narrows gate (a).

**The absolute mutation score is never a gate.** It stays a discovery signal
printed by `mutation.sh`. Gating on the score would re-introduce exactly the
"drive it to 100%" failure mode this whole document argues against.
