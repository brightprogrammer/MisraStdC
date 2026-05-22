/// file      : std/container/str/init.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Initialization functions for Str

#ifndef MISRA_STD_CONTAINER_STR_INIT_H
#define MISRA_STD_CONTAINER_STR_INIT_H

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
#define StrInitFromStr_1(str)    StrInitFromCstr_2((str)->data, (str)->length)
#define StrInitFromStr_2(str, a) StrInitFromCstr_3((str)->data, (str)->length, (a))

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
/// Initialize a `Str` using stack-allocated backing storage.
/// Such strings cannot be dynamically resized.
///
#define StrInitStack(str, alloc_ptr, ne, scoped_body) VecInitStack(str, alloc_ptr, ne, scoped_body)

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
