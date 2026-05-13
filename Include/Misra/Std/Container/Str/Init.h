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
    ///
    /// Initialize a `Str` from a C string buffer using an explicit allocator.
    ///
    /// out[out] : Destination string.
    /// cstr[in] : Source character buffer.
    /// len[in]  : Number of bytes to copy from `cstr`.
    /// alloc[in]: Allocator to bind to `out`.
    ///
    /// SUCCESS : Returns true and initializes `out`.
    /// FAILURE : Returns false if allocation fails.
    ///
    /// TAGS: Str, Init, CStr, Allocator
    ///
    bool StrTryInitFromCstrAlloc(Str *out, const char *cstr, size len, Allocator alloc);

    ///
    /// Initialize a `Str` from a C string buffer using an explicit allocator.
    ///
    /// cstr[in] : Source character buffer.
    /// len[in]  : Number of bytes to copy from `cstr`.
    /// alloc[in]: Allocator to bind to the returned string.
    ///
    /// SUCCESS : Returns initialized string.
    /// FAILURE : Returns an empty string if allocation fails.
    ///
    /// TAGS: Str, Init, CStr, Allocator
    ///
    Str StrInitFromCstrAlloc(const char *cstr, size len, Allocator alloc);

///
/// Try to initialize a `Str` by copying bytes from a C string buffer.
///
/// This public API supports both of these forms:
///
/// - `StrTryInitFromCstr(out, cstr, len)`
/// - `StrTryInitFromCstr(out, cstr, len, alloc)`
///
/// Omitting the allocator uses `DefaultAllocator()`. Supplying an allocator
/// overrides the default object allocator for the destination string.
///
/// out[out]  : Destination string.
/// cstr[in]  : Source character buffer.
/// len[in]   : Number of bytes to copy from `cstr`.
/// alloc[in] : Optional allocator override for the destination string.
///
/// SUCCESS : Returns true and initializes `out`.
/// FAILURE : Returns false if allocation fails.
///
/// TAGS: Str, Init, Convert, CStr, Allocator
///
#define STR_TRY_INIT_FROM_CSTR_HAS_ARGS_IMPL(_1, _2, _3, _4, count, ...) count
#define STR_TRY_INIT_FROM_CSTR_HAS_ARGS(...) STR_TRY_INIT_FROM_CSTR_HAS_ARGS_IMPL(__VA_ARGS__, 4, 3, 2, 1, 0)
#define StrTryInitFromCstr(...) CONCAT(StrTryInitFromCstr_, STR_TRY_INIT_FROM_CSTR_HAS_ARGS(__VA_ARGS__))(__VA_ARGS__)
#define StrTryInitFromCstr_3(out, cstr, len) StrTryInitFromCstrAlloc((out), (cstr), (len), DefaultAllocator())
#define StrTryInitFromCstr_4(out, cstr, len, alloc) StrTryInitFromCstrAlloc((out), (cstr), (len), (alloc))

///
/// Initialize a `Str` by copying bytes from a C string buffer.
///
/// This public API supports both of these forms:
///
/// - `StrInitFromCstr(cstr, len)`
/// - `StrInitFromCstr(cstr, len, alloc)`
///
/// Omitting the allocator uses `DefaultAllocator()`. Supplying an allocator
/// overrides the default object allocator for the new string.
///
/// cstr[in]  : Source character buffer.
/// len[in]   : Number of bytes to copy from `cstr`.
/// alloc[in] : Optional allocator override for the new string.
///
/// SUCCESS : Returns an initialized `Str` containing a copy of the requested bytes.
/// FAILURE : Returns an empty `Str` if allocation fails.
///
/// TAGS: Str, Init, Convert, CStr, Allocator
///
#define STR_INIT_FROM_CSTR_HAS_ARGS_IMPL(_1, _2, _3, count, ...) count
#define STR_INIT_FROM_CSTR_HAS_ARGS(...) STR_INIT_FROM_CSTR_HAS_ARGS_IMPL(__VA_ARGS__, 3, 2, 1, 0)
#define StrInitFromCstr(...) CONCAT(StrInitFromCstr_, STR_INIT_FROM_CSTR_HAS_ARGS(__VA_ARGS__))(__VA_ARGS__)
#define StrInitFromCstr_2(cstr, len) StrInitFromCstrAlloc((cstr), (len), DefaultAllocator())
#define StrInitFromCstr_3(cstr, len, alloc) StrInitFromCstrAlloc((cstr), (len), (alloc))

///
/// Initialize a `Str` from a null-terminated C string.
///
/// This public API supports both of these forms:
///
/// - `StrInitFromZstr(zstr)`
/// - `StrInitFromZstr(zstr, alloc)`
///
/// Omitting the allocator uses `DefaultAllocator()`.
///
/// zstr[in]  : Null-terminated source string.
/// alloc[in] : Optional allocator override for the new string.
///
/// SUCCESS : Returns an initialized `Str` containing a copy of `zstr`.
/// FAILURE : Returns an empty `Str` if allocation fails.
///
/// TAGS: Str, Init, Zstr, Allocator
///
#define STR_INIT_FROM_ZSTR_HAS_ARGS_IMPL(_1, _2, count, ...) count
#define STR_INIT_FROM_ZSTR_HAS_ARGS(...) STR_INIT_FROM_ZSTR_HAS_ARGS_IMPL(__VA_ARGS__, 2, 1, 0)
#define StrInitFromZstr(...) CONCAT(StrInitFromZstr_, STR_INIT_FROM_ZSTR_HAS_ARGS(__VA_ARGS__))(__VA_ARGS__)
#define StrInitFromZstr_1(zstr) StrInitFromCstr((zstr), ZstrLen(zstr))
#define StrInitFromZstr_2(zstr, alloc) StrInitFromCstr((zstr), ZstrLen(zstr), (alloc))

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
/// Initialize a `Str` by copying another `Str`.
///
/// This public API supports both of these forms:
///
/// - `StrInitFromStr(str)`
/// - `StrInitFromStr(str, alloc)`
///
/// Omitting the allocator makes the new string inherit the source string
/// allocator configuration.
///
/// str[in]   : Source string.
/// alloc[in] : Optional allocator override for the new string.
///
/// SUCCESS : Returns an initialized copy of `str`.
/// FAILURE : Returns an empty string if allocation fails.
///
/// TAGS: Str, Init, Copy, Allocator
///
#define STR_INIT_FROM_STR_HAS_ARGS_IMPL(_1, _2, count, ...) count
#define STR_INIT_FROM_STR_HAS_ARGS(...) STR_INIT_FROM_STR_HAS_ARGS_IMPL(__VA_ARGS__, 2, 1, 0)
#define StrInitFromStr(...) CONCAT(StrInitFromStr_, STR_INIT_FROM_STR_HAS_ARGS(__VA_ARGS__))(__VA_ARGS__)
#define StrInitFromStr_1(str) StrInitFromCstr((str)->data, (str)->length, (str)->allocator)
#define StrInitFromStr_2(str, alloc) StrInitFromCstr((str)->data, (str)->length, (alloc))

///
/// Clone a `Str`, inheriting the source allocator configuration.
///
/// str[in] : Source string.
///
/// SUCCESS : Returns an initialized copy of `str`.
/// FAILURE : Returns an empty string if allocation fails.
///
/// TAGS: Str, Init, Copy, Convenience
///
#define StrDup(str) StrInitFromStr(str)

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

    ///
    /// Deinitialize a copied `Str` through an explicit allocator context.
    ///
    /// This is primarily used by generic containers that own copied `Str`
    /// values and need allocator-aware copy cleanup callbacks.
    ///
    /// copy[in,out] : Pointer to the `Str` object to deinitialize.
    /// alloc[in]    : Allocator context for the owning container.
    ///
    /// SUCCESS : Deinitializes `copy`.
    /// FAILURE : Does not return if arguments violate the callback contract.
    ///
    /// TAGS: Str, Deinit, Allocator, Callback
    ///
    void StrDeinitAlloc(void *copy, const Allocator *alloc);

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

    ///
    /// Copy a `Str` through an explicit destination allocator context.
    ///
    /// This is primarily used by generic containers during deep-copy insertion.
    ///
    /// dst[out] : Destination `Str`.
    /// src[in]  : Source `Str`.
    /// alloc[in]: Allocator context for the owning destination container.
    ///
    /// SUCCESS : Returns true and initializes `dst`.
    /// FAILURE : Returns false if allocation fails.
    ///
    /// TAGS: Str, Init, Copy, Allocator, Callback
    ///
    bool StrInitCopyAlloc(void *dst, const void *src, const Allocator *alloc);

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_STR_INIT_H
