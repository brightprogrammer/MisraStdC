---
title: "Extending IO with User-Defined Types"
date: 2026-06-03
description: "Make WriteFmt and StrReadFmt work with your own struct."
authors:
  - siddharth-mishra
tags:
  - io
  - formatting
  - extension
---

> **Note**: This post was drafted by an AI assistant under direction from the author. It is not first-hand writing; the design choices it describes are real, the prose explaining them is generated. Treat the technical content as the design talking, and the framing as a translation layer.

`WriteFmt` and `StrReadFmt` know how to format the built-in types. To make them format your own struct, write a tiny pair of functions for it and tell `IOFMT` they exist. After that, the macros work on your type the same way they work on `i32` or `Str`.

Here is the whole thing for a `Point2D`:

```c
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Types.h>

typedef struct {
    i32 x;
    i32 y;
} Point2D;

// Tell IOFMT how to find the reader and writer for Point2D.
// This MUST be defined before Misra/Std/Io.h is included.
#define IOFMT_USER_CASE_(x, addr) \
    Point2D : TO_TYPE_SPECIFIC_IO(Point2D, addr),

#include <Misra/Std/Io.h>

bool _write_Point2D(Str *o, FmtInfo *info, Point2D *p) {
    (void)info;
    return StrAppendFmt(o, "({}, {})", p->x, p->y);
}

Zstr _read_Point2D(Zstr i, FmtInfo *info, Point2D *p) {
    (void)info;
    StrReadFmt(i, "({}, {})", p->x, p->y);
    return i;
}
```

That's all. Now you can use the format macros on your type:

```c
Point2D p = {.x = 3, .y = 4};
WriteFmt("position = {}\n", p);   // prints "position = (3, 4)"

Zstr in   = "(42, -9)";
Point2D q = {0};
StrReadFmt(in, "{}", q);          // q == {42, -9}
```

The writer and the reader use the same format string, so a value written by one is read back by the other.

## Three things to keep in mind

- The function names are not free-form. They must be `_write_<TypeName>` and `_read_<TypeName>`. The IO macros build these names from the type at compile time.
- `IOFMT_USER_CASE_` has to be defined **before** you include `Misra/Std/Io.h`. If `Io.h` is included first (often through another header), an empty default version is used, and your override is silently ignored.
- More than one type? List them all in the same macro, one arm per line:

```c
#define IOFMT_USER_CASE_(x, addr)                  \
    Point2D : TO_TYPE_SPECIFIC_IO(Point2D, addr),  \
    Bounds  : TO_TYPE_SPECIFIC_IO(Bounds,  addr),
```

## Where to put it in a real project

Keep the macro and the function declarations in one project header (for example `MyApp/Io.h`) and the bodies in one source file (`MyApp/Io.c`). Every file that prints or reads your types includes only that header. This keeps the include-order rule in one place instead of spread across the codebase.

A working end-to-end example with several types lives at `Tests/Std/Io.UserTypes.c`.
