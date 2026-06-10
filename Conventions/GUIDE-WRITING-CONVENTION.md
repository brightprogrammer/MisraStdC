# Guide-Writing Convention

> **This document is for AI agents.** Treat it as a hard guardrail when
> writing or editing anything under `Docs/content/english/guides/`. It is
> not advisory. If a rule here would force a worse outcome on a specific
> guide, stop and flag the conflict to the user rather than quietly
> breaking it.

This document captures the rules the project follows for prose guides.
It is short by design — when in doubt, cut.

---

## 0. The mandatory AI-authored banner

Every guide must carry this exact banner, placed immediately after the
YAML frontmatter and before the first sentence of the body:

```markdown
> **Note**: This post was drafted by an AI assistant under direction from the author. It is not first-hand writing; the design choices it describes are real, the prose explaining them is generated. Treat the technical content as the design talking, and the framing as a translation layer.
```

The wording is fixed. Do not paraphrase. Do not add or remove emphasis.
Do not place the banner inside a Hugo shortcode (`{{< notice >}}` etc.) —
plain blockquote only.

This banner is non-negotiable. A guide without it is a guide that lies
about its provenance.

---

## 1. Audience and shape

The audience is a competent C programmer who has chosen to use this
library. They are reading the guide to **do a thing**, not to learn how
the library was designed.

- **General guides are tutorials.** State the goal in one sentence,
  show the minimum working example, list the few things that bite in
  practice, point at where to look for more.
- **A guide titled "Internals" or "Design" may go deep** — algorithm,
  data-structure choices, rejected alternatives, contrast with other
  implementations. The title is the carve-out, not the content. If a
  general guide drifts into design essay, the answer is to trim, not to
  rename.
- **Design-summary posts** (planning documents, refactor direction
  pages) sit between tutorial and internals. They may discuss the
  *what* and the *why*, but not the *how it expands at the
  preprocessor level*. Implementation detail still belongs in code or
  in an internals post.

When in doubt about shape: read the title. If it says "Working With X",
"Building Y", "Extending Z", "Using A" — it is a tutorial.

---

## 2. Talk less, deliver more

The reader wants the answer to "how do I do X" in as few words as
possible. Every paragraph either advances them toward doing X or earns
deletion.

Anti-patterns that always justify deletion:

- **Preambles that defend the design.** "The build story is intentionally
  boring, which is the right choice for a C library." Just describe the
  build.
- **Editorializing asides.** "That is deliberate." "That is the right
  tradeoff for now." "This is the central behavioral change." Pure
  author voice — they tell the reader nothing they could not have
  inferred from the surrounding text.
- **Why-this-matters sections.** If the guide has to argue that its
  topic matters, the topic does not matter enough for a guide.
- **Sections that summarize earlier sections.** "A Good Mental Model"
  paragraphs that restate the whole article in bullet form. Trust the
  reader to remember what they just read.
- **Comparison tables against alternatives the reader did not ask
  about.** The reader picked this library. Do not relitigate.

---

## 3. Simple English, no surprise jargon

Plain words, short sentences. The reader is competent at C, not at
your taste in adverbs.

A term may appear without ceremony if:
- it is a project-specific name and the surrounding sentence gives the
  reader enough to follow (`Scope`, `MisraScope`, `Vec(T)`,
  `GraphNodeId`, `IOFMT_USER_CASE_`, etc.), OR
- it is standard C vocabulary every working C programmer knows
  (l-value, struct, pointer, macro).

A term **must** be introduced if:
- the first appearance is in a heading or a code comment without prose
  context,
- the surrounding sentence assumes the reader already knows the term,
- the meaning differs from how the term is used in other codebases or
  in the C standard.

If the reader has to pause and ask "what does that mean?", that is a
writing failure. Rewrite the sentence; do not add a footnote.

---

## 4. Show, don't describe (when discussing API distinctions)

Distinctions like L-form versus R-form, propagating versus aborting,
deep-copy versus shallow are easier to learn from one runnable code
example than from three paragraphs of prose. Lead with the example.

The minimum-working-example block should be reachable within the first
screenful of the guide. If a reader has to scroll past five paragraphs
to find code, the guide front-loaded the wrong material.

---

## 5. Internals do not leak

A tutorial does not describe:

- macro-expansion shape (`do { ... } while (0)`, `_Generic` arm
  structure, token-paste tricks),
- struct field layouts the reader does not need to set,
- algorithmic invariants the runtime enforces internally,
- which file or function the public macro forwards to.

If the reader will not type any of this, it does not belong in the
guide. It belongs in the header comment for the relevant macro, or in
`allocator-internals.md` / `CODING-CONVENTIONS.md`.

A tutorial *may* say "see `Tests/Std/X.c` for a working end-to-end
example." That is a pointer, not a leak.

---

## 6. Cross-reference instead of duplicating

If another guide already explains a concept the current guide depends
on, link to it. Do not re-explain. Common cross-references:

- Allocator lifetimes → *Scope-Based Allocator Discipline*
- L/R insertion forms → *Generic Containers and Ownership*
- Build / test flow → *Building and Testing MisraStdC*

Duplicating prose across guides creates drift the next time something
changes.

---

## 7. No embedded changelogs, no commit-pinned baselines

Guides describe how the library works **now**. They do not describe
how the library came to work that way. Things that must NOT appear in
a guide:

- "Decisions made during implementation" sections,
- commit hashes named in prose ("written against commit `abc1234`"),
- migration plans that depend on what is in the working tree this
  week,
- "This was decided after considering alternatives X, Y, Z" essays.

These age within weeks and embarrass the guide. Land them as commit
messages, code comments, or `FUTURE-PLANS.md` entries instead.

---

## 8. Frontmatter rules

```yaml
---
title: "<Short, descriptive title in Title Case>"
date: <YYYY-MM-DD>
description: "<One short sentence stating what the reader gets>"
authors:
  - siddharth-mishra
tags:
  - <3-5 tags, lowercase, no spaces>
---
```

The `description` line is what shows up in indexes and search. Keep it
short and concrete. "How MisraStdC's allocator does X and why Y" is
worse than "Build, walk, and modify a directed graph."

---

## 9. Length is not a virtue, but neither is brevity

There is no fixed word count. A 200-word guide that delivers the
answer is better than a 2000-word guide. A 1500-word guide that
genuinely needs every paragraph is better than a 500-word guide that
left the reader stuck.

The honest test: read each paragraph and ask, "if I removed this,
would the reader fail at the task this guide promised them?" If no,
remove it.

---

## 10. The review loop

When you write or rewrite a guide, run it past a reviewer agent before
declaring it done. The reviewer's job is to check this convention is
followed — not to suggest more material to add. If the reviewer asks
for additions, reread *this* document before agreeing.

If you are an AI agent reading this for the first time mid-task: this
is the guardrail. Apply it. If you disagree with a rule, stop the work
and surface the disagreement to the user. Do not silently relax it.

---

## Appendix: the guide set today

The current set of guides under `Docs/content/english/guides/` is:

| File | Shape |
|---|---|
| `allocator-internals.md` | internals |
| `building-and-testing.md` | tutorial |
| `extending-io-with-user-types.md` | tutorial |
| `generic-containers-and-ownership.md` | tutorial |
| `parsing-kv-config-files.md` | tutorial |
| `planned-fallible-apis-and-allocators.md` | design-summary |
| `scope-based-allocator-discipline.md` | tutorial |
| `why-misrastdc-exists.md` | design-summary |
| `working-with-graphs.md` | tutorial |

When adding a new guide, pick the shape from the title and write to
match. If you cannot pick a shape from the title, the title is the
problem — fix it before writing the body.
