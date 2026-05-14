/// file      : std/container/str/init.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Initialization functions for Str

#ifndef MISRA_STD_CONTAINER_STR_INIT_H
#define MISRA_STD_CONTAINER_STR_INIT_H

#include "Type.h"
#include <Misra/Std/Memory.h>
#include <Misra/Std/Container/Vec/Type.h>

#ifdef __cplusplus
extern "C" {
#endif

    bool str_try_init_from_cstr(Str *out, const char *cstr, size len, Allocator *alloc);
    Str  str_init_from_cstr(const char *cstr, size len, Allocator *alloc);

#define StrTryInitFromCstr(out, cstr, len, allocator_ptr)                                                              \
    str_try_init_from_cstr((out), (cstr), (len), ALLOCATOR_OF(allocator_ptr))

#define StrInitFromCstr(cstr, len, allocator_ptr)                                                                      \
    str_init_from_cstr((cstr), (len), ALLOCATOR_OF(allocator_ptr))

#define StrInitFromZstr(zstr, allocator_ptr) StrInitFromCstr((zstr), ZstrLen(zstr), (allocator_ptr))

#define StrZ(zstr, allocator_ptr) StrInitFromZstr((zstr), (allocator_ptr))

#define StrInitFromStr(str, allocator_ptr) StrInitFromCstr((str)->data, (str)->length, (allocator_ptr))

#define StrDup(str, allocator_ptr) StrInitFromStr((str), (allocator_ptr))

    Str *StrPrintf(Str *str, const char *fmt, ...) FORMAT_STRING(2, 3);

///
/// Initialize a Str bound to an `Allocator *`. The argument is a raw
/// `Allocator *` (use `&heap.base`, `ALLOCATOR_OF(&heap)`, or
/// `MisraScope` to get one).
///
#ifdef __cplusplus
#    define StrInit(alloc_ptr) (Str VecInit(alloc_ptr))
#else
#    define StrInit(alloc_ptr) ((Str)VecInit(alloc_ptr))
#endif

///
/// Initialize a `Str` using stack-allocated backing storage.
/// Such strings cannot be dynamically resized.
///
#define StrInitStack(str, alloc_ptr, ne, scoped_body) VecInitStack(str, alloc_ptr, ne, scoped_body)

    void StrDeinit(Str *str);
    void str_deinit(void *copy, const Allocator *alloc);
    bool StrInitCopy(Str *dst, const Str *src);
    bool str_init_copy(void *dst, const void *src, const Allocator *alloc);

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_STR_INIT_H
