---
title: "Building and Testing MisraStdC"
date: 2026-04-19
description: "A practical local workflow for cloning, building, testing, and debugging the library."
authors:
  - siddharth-mishra
tags:
  - build
  - testing
  - meson
  - ninja
---

MisraStdC uses a conventional Meson plus Ninja workflow. The build story is intentionally boring, which is the right choice for a C library.

## Requirements

You need:

- a C11 compiler
- Meson
- Ninja

The project is meant to work across GCC, Clang, and modern MSVC.

## Standard Build Flow

The normal local flow is:

```bash
git clone --recursive https://github.com/brightprogrammer/MisraStdC.git
cd MisraStdC

meson setup builddir
ninja -C builddir
ninja -C builddir test
```

That gets you a clean local build and runs the test suite in the same build directory.

## Development Build With Sanitizers

When debugging memory or UB problems, use sanitizers early instead of waiting until the code is already tangled.

```bash
meson setup builddir -Db_sanitize=address,undefined -Db_lundef=false
ninja -C builddir
ninja -C builddir test
```

This is a good default when changing low-level container code, string code, parsing code, or ownership-related paths.

## What To Expect From Tests

The test suite is not only regression coverage. It also acts as documentation.

The generated API docs cross-reference usages found in `Tests/`, which means good tests do double duty:

- they validate behavior
- they provide concrete call-site examples for the published reference pages

That matters especially for public generic APIs such as:

- `IntCompare`
- `IntAdd`, `IntSub`, `IntMul`, `IntDiv`, `IntPow`
- `FloatFrom`, `FloatCompare`, `FloatAdd`, `FloatMul`, `FloatDiv`

If a public API has no test usage, its generated documentation tends to look thinner and less trustworthy.

## A Good Workflow For Local Changes

For most changes, the practical loop is:

1. change one focused area
2. run the smallest relevant test targets first
3. run a broader suite if the change touches shared runtime helpers
4. regenerate docs if public comments or exported APIs changed

That last point matters because the docs are partly generated from the source tree, not maintained as a separate manual.

## Documentation Build Flow

The documentation site combines two pieces:

- generated API pages from `Scripts/DocuGen.py`
- curated prose content under `Docs/content/english`

So when public APIs move, the correct workflow is not just "fix code and push". It is:

1. update public comments
2. regenerate docs
3. make sure the generated pages still reflect the intended public surface

That keeps the published site aligned with the code instead of drifting into a stale wrapper around the headers.
