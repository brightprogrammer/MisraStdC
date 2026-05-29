/// file      : std/container/str/init.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Initialization functions for Str

#ifndef MISRA_STD_CONTAINER_STR_INIT_H
#define MISRA_STD_CONTAINER_STR_INIT_H

#include "Access.h"
#include "Type.h"
#include <Misra/Std/Memory.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Std/Container/Vec/Type.h>

#ifdef __cplusplus
extern "C" {
#endif

    ///
    /// Initialise `*out` as a `Str` backed by `alloc` and fill it with
    /// the first `len` bytes copied from `cstr`. `cstr` is treated as
    /// an opaque byte range -- a NUL inside `[cstr, cstr + len)` is
    /// copied as a regular byte; the resulting `Str` is still
    /// NUL-terminated past `length`. A trailing NUL is written at
    /// `data[len]` so `StrBegin(out)` is safely usable as a `Zstr`.
    ///
    /// `len == 0` is a valid degenerate input -- `*out` becomes an
    /// empty, capacity-zero `Str` with no allocation performed.
    ///
    /// SUCCESS : Returns `true`. `*out` is a usable `Str` holding the
    ///           copied bytes. Capacity covers at least `len + 1`.
    /// FAILURE : Returns `false` on allocator OOM (only reachable when
    ///           `len > 0`). `*out` is left as an empty `Str` bound to
    ///           `alloc` -- safe to `StrDeinit` but does NOT hold the
    ///           requested content. `LOG_FATAL` if `out` or `cstr` is
    ///           NULL.
    ///
    /// TAGS: Str, Init, Cstr
    ///
    bool str_try_init_from_cstr(Str *out, Zstr cstr, size len, Allocator *alloc);

    ///
    /// By-value `Str` constructor that copies `len` bytes from `cstr`
    /// into a freshly allocated buffer owned by `alloc`, with a
    /// trailing NUL placed at `data[len]`. Use this form when the
    /// caller treats OOM and intentionally empty input the same way;
    /// use the try-style allocator-explicit init (which returns
    /// `false` on OOM without aborting) when the distinction matters.
    ///
    /// SUCCESS : Returns a usable `Str` holding the copied bytes.
    /// FAILURE : Returns an empty `Str` bound to `alloc` on allocator
    ///           OOM (only reachable when `len > 0`). The caller cannot
    ///           distinguish OOM from an intentionally empty input via
    ///           the return value alone. `LOG_FATAL` if `cstr` is
    ///           NULL.
    ///
    /// TAGS: Str, Init, Cstr
    ///
    Str  str_init_from_cstr(Zstr cstr, size len, Allocator *alloc);

///
/// Initialise `*out` as a `Str` holding `len` bytes copied from `cstr`,
/// allocated through `allocator_ptr` (accepts a typed allocator handle
/// or a raw `Allocator *`).
///
/// SUCCESS : Returns `true`. `*out` is a usable `Str` holding the
///           copied bytes; trailing NUL written at `out->data[len]`.
/// FAILURE : Returns `false` on allocator OOM. `*out` is left as an
///           empty `Str` bound to the allocator. `LOG_FATAL` if `out`
///           or `cstr` is NULL.
///
/// TAGS: Str, Init, Cstr, API
///
#define StrTryInitFromCstr(out, cstr, len, allocator_ptr)                                                              \
    str_try_init_from_cstr((out), (cstr), (len), ALLOCATOR_OF(allocator_ptr))

///
/// Initialise a `Str` by value from a byte range `[cstr, cstr + len)`.
/// The 2-arg form uses the enclosing `MisraScope` allocator; the 3-arg
/// form takes an explicit allocator (typed handle or raw `Allocator *`).
///
/// SUCCESS : Returns a usable `Str` holding the copied bytes.
/// FAILURE : Returns an empty `Str` on allocator OOM. The empty return
///           is indistinguishable from `len == 0`; use
///           `StrTryInitFromCstr` when OOM must be detected.
///           `LOG_FATAL` if `cstr` is NULL.
///
/// TAGS: Str, Init, Cstr, API
///
#define StrInitFromCstr(...)                OVERLOAD(StrInitFromCstr, __VA_ARGS__)
#define StrInitFromCstr_2(cstr, len)        str_init_from_cstr((cstr), (len), MisraScope)
#define StrInitFromCstr_3(cstr, len, alloc) str_init_from_cstr((cstr), (len), ALLOCATOR_OF(alloc))

///
/// Initialise a `Str` by value from a NUL-terminated `Zstr`. Length is
/// derived from `ZstrLen(zstr)`, so `zstr` is walked once to find the
/// terminator before the copy is performed. The 1-arg form uses the
/// enclosing `MisraScope` allocator; the 2-arg form takes an explicit
/// allocator.
///
/// SUCCESS : Returns a usable `Str` holding the bytes of `zstr` up to
///           (but not including) the terminator.
/// FAILURE : Returns an empty `Str` on allocator OOM. `LOG_FATAL` if
///           `zstr` is NULL (via `ZstrLen` / underlying Cstr backend).
///
/// TAGS: Str, Init, Zstr, API
///
#define StrInitFromZstr(...)       OVERLOAD(StrInitFromZstr, __VA_ARGS__)
#define StrInitFromZstr_1(zstr)    StrInitFromCstr_2((zstr), ZstrLen(zstr))
#define StrInitFromZstr_2(zstr, a) StrInitFromCstr_3((zstr), ZstrLen(zstr), (a))

///
/// Short alias for `StrInitFromZstr`. Initialises a `Str` by value from
/// a NUL-terminated `Zstr`; 1-arg form uses `MisraScope`, 2-arg form
/// takes an explicit allocator. Intended for terse call sites such as
/// `Foo(StrZ("literal"))`.
///
/// SUCCESS : Returns a usable `Str` holding the bytes of `zstr`.
/// FAILURE : Returns an empty `Str` on allocator OOM. `LOG_FATAL` if
///           `zstr` is NULL.
///
/// TAGS: Str, Init, Zstr, Alias, API
///
#define StrZ(...)       OVERLOAD(StrZ, __VA_ARGS__)
#define StrZ_1(zstr)    StrInitFromZstr_1((zstr))
#define StrZ_2(zstr, a) StrInitFromZstr_2((zstr), (a))

///
/// Deep-copy an existing `Str` by value. Length is taken from
/// `StrLen(str)` and bytes are read via `StrBegin(str)`, so embedded
/// NULs in the source are preserved. The 1-arg form uses the enclosing
/// `MisraScope` allocator; the 2-arg form takes an explicit allocator,
/// which may differ from the source's allocator.
///
/// SUCCESS : Returns a usable `Str` holding an independent copy of
///           `str`'s contents. The returned handle does NOT share
///           storage with `str`.
/// FAILURE : Returns an empty `Str` on allocator OOM. `LOG_FATAL` if
///           the source handle is invalid.
///
/// TAGS: Str, Init, Copy, API
///
#define StrInitFromStr(...)      OVERLOAD(StrInitFromStr, __VA_ARGS__)
#define StrInitFromStr_1(str)    StrInitFromCstr_2(StrBegin(str), StrLen(str))
#define StrInitFromStr_2(str, a) StrInitFromCstr_3(StrBegin(str), StrLen(str), (a))

///
/// Short alias for `StrInitFromStr`. Deep-copies an existing `Str` by
/// value; 1-arg form uses `MisraScope`, 2-arg form takes an explicit
/// allocator. Use when reading at the call site as "duplicate this
/// string" is clearer than the longer name.
///
/// SUCCESS : Returns a usable `Str` holding an independent copy of the
///           source's contents.
/// FAILURE : Returns an empty `Str` on allocator OOM. `LOG_FATAL` if
///           the source handle is invalid.
///
/// TAGS: Str, Init, Copy, Alias, API
///
#define StrDup(...)      OVERLOAD(StrDup, __VA_ARGS__)
#define StrDup_1(str)    StrInitFromStr_1((str))
#define StrDup_2(str, a) StrInitFromStr_2((str), (a))

///
/// Initialize a Str. Inside a `Scope` block the allocator argument may
/// be omitted; the internal `MisraScope` allocator is used. Otherwise
/// pass a typed allocator handle or a raw `Allocator *`.
///
/// TAGS: Str, Init, API
///
#define StrInit(...) OVERLOAD(StrInit, __VA_ARGS__)
#ifdef __cplusplus
#    define StrInit_0()          (Str VecInit_1(MisraScope))
#    define StrInit_1(alloc_ptr) (Str VecInit_1(alloc_ptr))
#else
#    define StrInit_0()          ((Str)VecInit_1(MisraScope))
#    define StrInit_1(alloc_ptr) ((Str)VecInit_1(alloc_ptr))
#endif

///
/// Open a scope that declares a `Str` named `name`, backed by a
/// fixed-capacity stack array of `ne` characters. The body that
/// follows the macro call (NOT a macro argument) sees `name` as an
/// initialised, empty `Str` with capacity `ne`. `name`'s lexical
/// scope is exactly the body: outside the macro the identifier is
/// no longer bound.
///
/// No allocator -- the backing storage is the stack. The body is
/// responsible for keeping content bounded by `ne`. Any operation
/// that would grow `name` past `ne` ends up calling `StrReserve`,
/// which sees the NULL allocator and aborts via
/// `LOG_FATAL("vector not growable, no allocator assigned, probably stack inited")`.
/// Use a heap-backed `Str` if you need spill behaviour.
///
/// The macro uses the for-chain scope idiom -- the body is regular
/// code below the macro, not a brace-delimited argument:
///
///     StrInitStack(buf, 1024) {
///         ssize_t n = read(fd, StrBegin(&buf), 1023);
///         StrResize(&buf, (size)n);
///         StrMergeR(out, &buf);
///     }
///
/// SUCCESS : Body runs once; `name` is valid for the body's scope.
///           On normal fall-through both `name` and the backing
///           array are zeroed by the macro's exit updates before
///           `name` falls out of scope.
/// FAILURE : The macro itself cannot fail. Operations inside the
///           body that try to grow `name` past `ne` abort.
///
/// CAVEAT  : `return` / `goto` leaving the body skip ALL exit
///           updates (the same C-level limitation that applies to
///           `Scope`). `break` exits the innermost for cleanly: the
///           backing array is still zeroed by the outer for's
///           update, but the `name` handle's MemSet is skipped --
///           harmless because `name`'s scope is the body anyway and
///           the identifier is no longer reachable after the macro.
///           `continue` inside the body does NOT restart -- it
///           jumps to the inner for's update clause, which zeroes
///           `name` and then re-checks the (already-false)
///           condition, exiting the scope. Treat it as a silent
///           early-exit, not a loop control.
///           `ne` is evaluated twice (the `+1` for the array
///           dimension, and the `capacity` assignment);
///           `sizeof(UNPL(_d))` reads the array's type, not `ne`.
///           Pass a side-effect-free expression (literal or simple
///           variable) regardless.
///
/// TAGS: Str, Init, Stack, Scope
///
#define StrInitStack(name, ne)                                                                                         \
    for (char UNPL(_d)[(ne) + 1] = {0}, *UNPL(_loop) = UNPL(_d); UNPL(_loop);                                          \
         MemSet(UNPL(_d), 0, sizeof(UNPL(_d))), UNPL(_loop) = NULL)                                                    \
        for (Str name = {.length      = 0,                                                                             \
                         .capacity    = (ne),                                                                          \
                         .data        = UNPL(_d),                                                                      \
                         .allocator   = NULL,                                                                          \
                         .copy_init   = NULL,                                                                          \
                         .copy_deinit = NULL,                                                                          \
                         .__magic     = VEC_MAGIC},                                                                    \
                 *UNPL(_done) = &name;                                                                                  \
             UNPL(_done);                                                                                              \
             MemSet(&name, 0, sizeof(name)), UNPL(_done) = NULL)

    ///
    /// Release the backing storage of `str` through its inline allocator
    /// and zero the handle so a later double-Deinit is a no-op.
    ///
    /// SUCCESS : Returns to the caller. `*str` is zeroed.
    /// FAILURE : Function cannot fail. NULL `str` is a no-op.
    ///
    /// TAGS: Str, Deinit, Init
    ///
    void StrDeinit(Str *str);

    ///
    /// Deep-copy callback used by `Map<Str, ...>` / `Vec<Str>` etc. to
    /// release an owned `Str` element. Mirrors `StrDeinit` but takes the
    /// erased-type signature container deep-copy slots expect.
    ///
    /// SUCCESS : Returns to the caller. `*(Str *)copy` is zeroed.
    /// FAILURE : Function cannot fail. NULL `copy` is a no-op.
    ///
    /// TAGS: Str, Deinit, Init
    ///
    void str_deinit(void *copy, const Allocator *alloc);

    ///
    /// Deep-copy `src` into a freshly initialised `dst`, allocating
    /// through `src`'s inline allocator. Both ends carry independent
    /// backing storage afterwards.
    ///
    /// SUCCESS : Returns `true`. `*dst` is a usable Str with the same
    ///           contents and allocator as `*src`.
    /// FAILURE : Returns `false` on allocator OOM. `*dst` is left zeroed.
    ///
    /// TAGS: Str, Init, Copy
    ///
    bool StrInitCopy(Str *dst, const Str *src);

    ///
    /// Deep-copy callback used by `Map<..., Str>` / `Vec<Str>` etc. to
    /// duplicate an owned `Str` value. Mirrors `StrInitCopy` but takes
    /// the erased-type signature container deep-copy slots expect.
    ///
    /// SUCCESS : Returns `true`. `*(Str *)dst` is a deep copy of `*(const Str *)src`.
    /// FAILURE : Returns `false` on allocator OOM. `*(Str *)dst` is left zeroed.
    ///
    /// TAGS: Str, Init, Copy
    ///
    bool str_init_copy(void *dst, const void *src, const Allocator *alloc);

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_STR_INIT_H
