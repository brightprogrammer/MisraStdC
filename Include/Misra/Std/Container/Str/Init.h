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

    bool str_try_init_from_cstr(Str *out, Zstr cstr, size len, Allocator *alloc);
    Str  str_init_from_cstr(Zstr cstr, size len, Allocator *alloc);

#define StrTryInitFromCstr(out, cstr, len, allocator_ptr)                                                              \
    str_try_init_from_cstr((out), (cstr), (len), ALLOCATOR_OF(allocator_ptr))

#define StrInitFromCstr(...)                MISRA_OVERLOAD(StrInitFromCstr, __VA_ARGS__)
#define StrInitFromCstr_2(cstr, len)        str_init_from_cstr((cstr), (len), MisraScope)
#define StrInitFromCstr_3(cstr, len, alloc) str_init_from_cstr((cstr), (len), ALLOCATOR_OF(alloc))

#define StrInitFromZstr(...)       MISRA_OVERLOAD(StrInitFromZstr, __VA_ARGS__)
#define StrInitFromZstr_1(zstr)    StrInitFromCstr_2((zstr), ZstrLen(zstr))
#define StrInitFromZstr_2(zstr, a) StrInitFromCstr_3((zstr), ZstrLen(zstr), (a))

#define StrZ(...)       MISRA_OVERLOAD(StrZ, __VA_ARGS__)
#define StrZ_1(zstr)    StrInitFromZstr_1((zstr))
#define StrZ_2(zstr, a) StrInitFromZstr_2((zstr), (a))

#define StrInitFromStr(...)      MISRA_OVERLOAD(StrInitFromStr, __VA_ARGS__)
#define StrInitFromStr_1(str)    StrInitFromCstr_2(StrBegin(str), StrLen(str))
#define StrInitFromStr_2(str, a) StrInitFromCstr_3(StrBegin(str), StrLen(str), (a))

#define StrDup(...)      MISRA_OVERLOAD(StrDup, __VA_ARGS__)
#define StrDup_1(str)    StrInitFromStr_1((str))
#define StrDup_2(str, a) StrInitFromStr_2((str), (a))

///
/// Initialize a Str. Inside a `Scope` block the allocator argument may
/// be omitted; the internal `MisraScope` allocator is used. Otherwise
/// pass a typed allocator handle or a raw `Allocator *`.
///
#define StrInit(...) MISRA_OVERLOAD(StrInit, __VA_ARGS__)
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
/// that would grow `name` past `ne` lands in `reserve_vec`, sees
/// the NULL allocator, and aborts via
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
///           `ne` is evaluated three times (the `+1` for the array
///           dimension, the `sizeof` against the resulting array
///           through `_d`, and the capacity assignment); pass a
///           side-effect-free expression (literal or simple
///           variable).
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
    void StrDeinit(Str *str);

    ///
    /// Deep-copy callback used by `Map<Str, ...>` / `Vec<Str>` etc. to
    /// release an owned `Str` element. Mirrors `StrDeinit` but takes the
    /// erased-type signature container deep-copy slots expect.
    ///
    /// SUCCESS : Returns to the caller. `*(Str *)copy` is zeroed.
    /// FAILURE : Function cannot fail. NULL `copy` is a no-op.
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
    bool StrInitCopy(Str *dst, const Str *src);

    ///
    /// Deep-copy callback used by `Map<..., Str>` / `Vec<Str>` etc. to
    /// duplicate an owned `Str` value. Mirrors `StrInitCopy` but takes
    /// the erased-type signature container deep-copy slots expect.
    ///
    /// SUCCESS : Returns `true`. `*(Str *)dst` is a deep copy of `*(const Str *)src`.
    /// FAILURE : Returns `false` on allocator OOM. `*(Str *)dst` is left zeroed.
    ///
    bool str_init_copy(void *dst, const void *src, const Allocator *alloc);

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_STR_INIT_H
