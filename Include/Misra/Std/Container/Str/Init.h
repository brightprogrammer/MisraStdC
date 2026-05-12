/// file      : std/container/str/init.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Initialization functions for Str

#ifndef MISRA_STD_CONTAINER_STR_INIT_H
#define MISRA_STD_CONTAINER_STR_INIT_H

#include "Misra/Std/Container/Vec/Type.h"
#include "Type.h"
#include <Misra/Std/Memory.h>

#ifdef __cplusplus
extern "C" {
#endif
    bool StrTryInitFromCstrWithAllocator(Str *out, const char *cstr, size len, Allocator alloc);
    Str StrInitFromCstrWithAllocator(const char *cstr, size len, Allocator alloc);

///
/// Initializes a Str object from a null-terminated C-style string (`zstr`).
/// This macro calculates the length of `zstr` using `strlen` and then calls
/// `StrInitFromCstr` to create the Str object.
///
/// zstr[in]    : Pointer to the null-terminated C-style string to initialize from.
///
/// SUCCESS : Returns a newly created Str object with its `data` field pointing to a
///           newly allocated memory containing a copy of `zstr`. The `length` and
///           `capacity` fields are set to the length of `zstr`. `copy_init` and
///           `copy_deinit` are set to NULL, and `alignment` is set to 1.
///
/// FAILURE : Returns a Str object with `data` set to NULL if memory allocation using
///           `strndup` (called internally by `StrInitFromCstr`) fails. In such a case,
///           `length` and `capacity` will likely be uninitialized or zero. It's crucial
///           to check the `data` field for NULL after using this macro to handle
///           potential memory allocation errors.
///
#define StrInitFromCstr(cstr, len) StrInitFromCstrWithAllocator((cstr), (len), DefaultAllocator())
#define StrInitFromCstrAlloc(cstr, len, alloc) StrInitFromCstrWithAllocator((cstr), (len), (alloc))
#define StrInitFromZstr(zstr) StrInitFromCstr((zstr), strlen(zstr))
#define StrInitFromZstrAlloc(zstr, alloc) StrInitFromCstrWithAllocator((zstr), strlen(zstr), (alloc))

///
/// Short alias for `StrInitFromZstr(...)` when an owned temporary string is
/// needed inline.
///
/// This is most useful with APIs that consume or store the resulting `Str`,
/// such as:
///
///   GraphAddNodeR(&graph, StrZ("Alpha"));
///
/// If ownership is not transferred, the resulting `Str` must still be
/// deinited manually.
///
/// zstr[in] : Pointer to a null-terminated C string.
///
/// SUCCESS : Returns a newly created owned `Str`.
/// FAILURE : Same as `StrInitFromZstr(...)`.
///
/// TAGS: Str, Init, Zstr, Convenience
///
#define StrZ(zstr) StrInitFromZstr((zstr))

///
/// Initialize a Str object using another one
///
#define StrInitFromStr(str) StrInitFromCstrWithAllocator((str)->data, (str)->length, (str)->allocator)
#define StrDup(str)         StrInitFromStr(str)

    ///
    /// Init the string using the given format string and arguments.
    /// Current contents of string will be cleared out
    ///
    /// str[in,out] : Str to be inited with format string.
    /// fmt[in]     : Format string, with variadic arguments following.
    ///
    /// SUCCESS : `str`
    /// FAILURE : NULL
    ///
    Str *StrPrintf(Str *str, const char *fmt, ...) FORMAT_STRING(2, 3);

#ifdef __cplusplus
#    define StrInit(...) (Str VecInit(__VA_ARGS__))
#else
///
/// Initialize given string.
///
/// str : Pointer to string memory that needs to be initialized.
///
/// SUCCESS : `str`
/// FAILURE : NULL
///
#    define StrInit(...) ((Str)VecInit(__VA_ARGS__))
#endif

///
/// Initialize given string but use memory from stack.
/// Such strings cannot be dynamically resized!!
///
/// str[in] : String that needs to be initialized.
/// ne[in]  : Number of elements to allocate stack memory for.
///
/// SUCCESS : `str`
/// FAILURE : NULL
///
#define StrInitStack(str, ne, scoped_body) VecInitStack(str, ne, scoped_body)

    ///
    /// Deinit str by freeing all allocations.
    ///
    /// str : Pointer to string to be deinited
    ///
    void StrDeinit(Str *str);
    void StrDeinitWithAllocator(void *copy, const Allocator *alloc);

    ///
    /// Copy data from `src` to `dst`
    ///
    /// dst[out] : Str object to copy into.
    /// src[in]  : Str object to copy from.
    ///
    /// SUCCESS : true
    /// FAILURE : false
    ///
    bool StrInitCopy(Str *dst, const Str *src);
    bool StrInitCopyWithAllocator(void *dst, const void *src, const Allocator *alloc);

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_STR_INIT_H
