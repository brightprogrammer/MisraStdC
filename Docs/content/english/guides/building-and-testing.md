---
title: "Building and Testing MisraStdC"
date: 2026-04-19
description: "Clone, build, and run the test suite."
authors:
  - siddharth-mishra
tags:
  - build
  - testing
  - meson
  - ninja
---

> **Note**: This post was drafted by an AI assistant under direction from the author. It is not first-hand writing; the design choices it describes are real, the prose explaining them is generated. Treat the technical content as the design talking, and the framing as a translation layer.

## What you need

A C11 compiler (GCC, Clang, or MSVC), Meson, and Ninja.

## Build

```bash
git clone --recursive https://github.com/brightprogrammer/MisraStdC.git
cd MisraStdC
meson setup build
ninja -C build
ninja -C build test
```

## Build with sanitizers

When chasing a memory or UB bug, turn on ASan + UBSan from the start:

```bash
meson setup build -Db_sanitize=address,undefined -Db_lundef=false
ninja -C build
ninja -C build test
```

This is the default flavor you want while changing container, string, parser, or ownership code.

## Running a single test

`meson test` accepts test names from `Tests/meson.build`:

```bash
meson test -C build Vec.Insert
meson test -C build -t 4 Io.Write Io.Read   # parallel, multiple
```

Add `--print-errorlogs` to see the failing test's output without re-running it manually.

## Documentation

The Hugo site under `Docs/` is rebuilt by CI on every push to `master`; you can serve it locally too:

```bash
cd Docs
hugo server
```

Public-API reference pages are generated from header comments by `Scripts/DocuGen.py`. If you change a public comment, regenerate the docs before pushing so the site reflects what the headers actually say.
