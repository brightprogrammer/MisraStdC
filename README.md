# MisraStdC

[![Linux Build](https://github.com/brightprogrammer/MisraStdC/actions/workflows/test-linux.yml/badge.svg?branch=master)](https://github.com/brightprogrammer/MisraStdC/actions/workflows/test-linux.yml)
[![macOS Build](https://github.com/brightprogrammer/MisraStdC/actions/workflows/test-macos.yml/badge.svg?branch=master)](https://github.com/brightprogrammer/MisraStdC/actions/workflows/test-macos.yml)
[![Windows MSVC](https://github.com/brightprogrammer/MisraStdC/actions/workflows/test-windows-msvc.yml/badge.svg?branch=master)](https://github.com/brightprogrammer/MisraStdC/actions/workflows/test-windows-msvc.yml)
[![Windows LLVM](https://github.com/brightprogrammer/MisraStdC/actions/workflows/test-windows-llvm.yml/badge.svg?branch=master)](https://github.com/brightprogrammer/MisraStdC/actions/workflows/test-windows-llvm.yml)
[![Fuzzing](https://github.com/brightprogrammer/MisraStdC/actions/workflows/fuzz.yml/badge.svg?branch=master)](https://github.com/brightprogrammer/MisraStdC/actions/workflows/fuzz.yml)
[![Chat on Zulip](https://img.shields.io/badge/chat-Zulip-6492FE.svg?logo=zulip&logoColor=white)](https://misrastdc.zulipchat.com/join/pbkj7hlz3ebiug35b3267yct/)

A C11 standard library, made with love, to bring some modern concepts into
plain C. Everything is opt-in at build time — you compile only what you use, and
a single `#include <Misra.h>` pulls in whatever the build enabled.

> Parts of this codebase have been written and re-written with the help of LLMs,
> under close supervision. See
> [Transparency and use of LLMs](#transparency-and-use-of-llms) for how.

> Not related to the MISRA C standard or its guidelines. The name comes from
> the author's surname — Siddharth Mishra, nicknamed "Misra".

## What it can do

- **Allocators** — six you own as plain values, no globals: binned heap, raw
  pages, arena, slab, fixed-budget, and a debug allocator that catches leaks,
  double frees, overflows, and use-after-free.
- **Containers** — `Vec(T)`, `List(T)`, `Map(K,V)`, `Graph(T)`, `BitVec`,
  `Str`, `Buf`. Generic through macros and `_Generic`, no template compiler.
- **Arbitrary-precision numbers** — `Int` and `Float`: radix-string
  conversion, modular arithmetic, primality, exact decimal arithmetic.
- **Formatted I/O** — Rust-style `{}` placeholders; one `WriteFmt` dispatches
  at compile time across ints, floats, `Str`, `Int`, `Float`, `BitVec`, and C
  strings.
- **Type matching & sum types** — one `Match` / `When` / `Otherwise`: a
  compile-time type switch (`When(T)`, zero runtime cost) and by-value tagged
  unions (`Variant(N, …)` + `When(N, T)` binding the payload), non-exhaustive
  matches fail fast.
- **Parsers** — JSON, key-value config, and the binary formats ELF, Mach-O,
  PE, and PDB.
- **System access** — subprocesses, directory listing, mutexes, environment,
  and process/executable info across Linux, macOS, and Windows.
- **Freestanding** — ships libc-free binaries via direct syscalls and a custom
  entry point on all three platforms.
- **Lifetimes** — `Scope` gives lexical RAII in plain C, or manage allocators
  by hand.
- **Safety** — fallible-by-default APIs with `Must` variants; per-object magic
  checks turn type confusion into an immediate abort instead of heap
  corruption.
- **Feature flags** — disable a subsystem and its `.c` and `.h` files drop out
  of the library and the install.

## A taste

```c
#include <Misra.h>

// With Scope — the allocator's lifetime is handled for you.
Scope(alloc, DefaultAllocator) {
    Vec(int) v = VecInit();             // picks up the scope's allocator
    VecPushBackR(&v, 42);
    WriteFmtLn("len = {}", VecLen(&v));
    VecDeinit(&v);
}                                        // allocator destroyed here

// Without Scope — you own the allocator's lifetime.
DefaultAllocator a = DefaultAllocatorInit();
Vec(int) v = VecInit(&a);                // allocator passed explicitly
VecPushBackR(&v, 42);
WriteFmtLn("len = {}", VecLen(&v));
VecDeinit(&v);
DefaultAllocatorDeinit(&a);
```

Type matching dispatches on a value's static type at compile time; the matched
value is bound to a name you choose (the last `When` argument), and the whole
match folds to the taken arm with zero runtime cost:

```c
#include <Misra/Generics.h>   // opt-in; not pulled in by <Misra.h>

Match(value) {
    When(int, n) WriteFmtLn("int {}", n);
    When(f64, x) WriteFmtLn("float {}", x);
    When(Str, s) WriteFmtLn("string {}", s);
    Otherwise    WriteFmtLn("something else");
}
```

Sum types shine where a record's field is a tagged union. A DNS record's RDATA
is exactly that — its layout depends on the record TYPE — so the variant models
it directly, and **serialising it is one `Match`**: each arm emits its wire
layout via the binary fmt I/O, and the `Buf` prints straight through `{}`:

```c
#include <Misra/Generics.h>

// RDATA: a tagged union keyed by the record TYPE.
typedef u32 Ipv4;                            // A
typedef struct { u16 pref; Str host; } Mx;   // MX
typedef Str Txt;                             // TXT
Variant(RData, Ipv4, Mx, Txt);

static void write_name(Buf *b, Str *name) {                  // labels: <len><bytes>..0x00
    Strs labels = StrSplit(name, ".");
    VecForeach (&labels, label) {
        BufAppendFmt(b, "{<1r}", (u8) StrLen(&label));
        BufPushBytes(b, (const u8 *) StrBegin(&label), StrLen(&label));
    }
    BufAppendFmt(b, "{<1r}", (u8) 0);
}

static void write_rdata(Buf *b, RData rd) {                  // the serializer IS the Match
    Match(rd) {
        When(RData, Ipv4, ip)  BufAppendFmt(b, "{>4r}", ip);                              // 4 bytes, BE
        When(RData, Mx,   mx)  { BufAppendFmt(b, "{>2r}", mx.pref); write_name(b, &mx.host); }
        When(RData, Txt,  tx)  { BufAppendFmt(b, "{<1r}", (u8) StrLen(&tx));
                                 BufPushBytes(b, (const u8 *) StrBegin(&tx), StrLen(&tx)); }
    }
}

Scope(a, DefaultAllocator) {
    Vec(RData) rrs = VecInit();
    VecPushBackR(&rrs, RData_from_Ipv4(0xC0A80101u));
    VecPushBackR(&rrs, RData_from_Mx((Mx){10, StrInitFromZstr("mail.example.com")}));
    VecPushBackR(&rrs, RData_from_Txt(StrInitFromZstr("v=spf1")));

    VecForeach (&rrs, rr) {
        Buf b = BufInit(a);
        write_rdata(&b, rr);
        WriteFmtLn("rdata ({} bytes): {}", BufLength(&b), b);   // Buf prints directly; non-printable -> \xHH
        BufDeinit(&b);
    }
    VecDeinit(&rrs);
}
// -> \xc0\xa8\x01\x01   /   \x00\x0a\x04mail\x07example\x03com\x00   /   \x06v=spf1
```

## Build

```sh
meson setup build
meson compile -C build
meson install -C build   # optional
```

Then `#include <Misra.h>` and link against `libmisra_std`.

## Feature flags

Every optional subsystem is a meson option — see
[`meson_options.txt`](meson_options.txt). Disabling one removes its source from
the static library and its headers from the install prefix; the default build
(everything on) is what CI runs.

## Docs, contributing, license

In-depth guides live under [`Docs/`](Docs/). Before contributing, read
[`CODING-CONVENTIONS.md`](Conventions/CODING-CONVENTIONS.md) and run the test suite plus
`clang-format`. Released into the public domain under the
[Unlicense](LICENSE.md).

Questions, ideas, or just want to follow along? Join the chat on
[Zulip](https://misrastdc.zulipchat.com/join/pbkj7hlz3ebiug35b3267yct/).

## Project status

This codebase is in its very early stages. There is no stable branch yet, and
there won't be one for a while. **Treat `master` as unstable** — agents
introduce coding drift that gets caught and fixed later, so the tip of `master`
can carry inconsistencies that are still being ironed out. Build against it with
that expectation.

What *is* stable is the set of standards the code is held to: naming, ownership,
allocator rules, error handling, formatting, and the rest are written down in
[`CODING-CONVENTIONS.md`](Conventions/CODING-CONVENTIONS.md). When the code and the
conventions disagree, the conventions are right and the code is a bug to be
fixed.

The tests aim to cover as much of the codebase as possible, and best effort goes
into keeping everything stable *in its usage* — APIs behave as documented and
the suite is meant to catch regressions across the library. What remains mostly
unstable is **performance**. The prototype code just works; it isn't yet fast.
At the time of writing a significant share of the available time and effort is
going into benchmarking and improving it.

## Transparency and use of LLMs

Parts of this codebase have been written and re-written with the help of large
language models, under close supervision. I want to be upfront about that rather
than pretend every line was typed by hand.

The models do not get a free hand. I work them in tight review loops: I read
what they produce, scrutinize it against the conventions above, and push back
hard when it drifts — and it does drift, sometimes from noise in the model and
sometimes because my own prompt was sloppy. When the output starts to "slop"
(plausible-looking code that quietly ignores an established pattern, reaches for
libc where an in-tree primitive exists, oversells what it actually does, or
invents an API instead of using the real one), I stop it, point at the specific
convention it broke, and make it redo the work until it matches the rest of the
library. Nothing lands because it looked convincing; it lands because I checked
it.

I also drive the commit history deliberately. The models tend to leave a trail
of experimental, half-right commits while we iterate; before anything is
published I have them cherry-pick and squash that down into a small set of
clean, self-contained commits with honest messages, so the history reads as a
sequence of deliberate changes rather than a transcript of the back-and-forth
that produced them.

The intent is simple: the convenience of an LLM for the mechanical work, none of
the abdication of judgement. The author remains responsible for every design
decision that goes in; granular, line-by-line knowledge of the code is not 100%
guaranteed, but a high-level understanding of it is assured.

## Sponsors

<a href="https://zulip.com"><img src="https://raw.githubusercontent.com/zulip/zulip/main/static/images/logo/zulip-icon-circle.svg" alt="Zulip" width="72" height="72"></a>

Zulip is an organized team chat app designed for efficient communication.
