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

    bool StrTryInitFromCstr(Str *out, const char *cstr, size len, Allocator *alloc);
    Str  StrInitFromCstr(const char *cstr, size len, Allocator *alloc);

#define StrInitFromZstr(zstr, alloc_ptr) StrInitFromCstr((zstr), ZstrLen(zstr), (alloc_ptr))

#define StrZ(zstr, alloc_ptr) StrInitFromZstr((zstr), (alloc_ptr))

#define StrInitFromStr(str, alloc_ptr) StrInitFromCstr((str)->data, (str)->length, (alloc_ptr))

#define StrDup(str, alloc_ptr) StrInitFromStr((str), (alloc_ptr))

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
    void StrDeinitAlloc(void *copy, const Allocator *alloc);
    bool StrInitCopy(Str *dst, const Str *src);
    bool StrInitCopyAlloc(void *dst, const void *src, const Allocator *alloc);

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_STR_INIT_H
