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
    /// SUCCESS : Returns `true`. `*out` is initialized: length is `len`, the
    ///           first `len` bytes are a copy of `cstr`, capacity is at
    ///           least `len + 1` (for the implicit terminator slot), and
    ///           `alloc` is bound into the string's allocator.
    /// FAILURE : Returns `false` on allocation failure for the byte buffer.
    ///           `*out` is left as an initialized-but-empty `Str` so the
    ///           caller can safely deinit it.
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
    /// SUCCESS : Returns a fully initialized `Str` whose first `len` bytes
    ///           are a copy of `cstr`, with capacity for at least
    ///           `len + 1` bytes and `alloc` bound as the string's
    ///           allocator.
    /// FAILURE : Returns an empty `Str` (length 0, capacity 0, data NULL,
    ///           allocator still bound) when allocation fails. Use
    ///           `StrTryInitFromCstrAlloc` if you need explicit failure
    ///           propagation.
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
/// SUCCESS : Returns `true`. `*out` is initialized with a copy of `cstr`'s
///           first `len` bytes; capacity is at least `len + 1` and the
///           chosen allocator is bound. See `StrTryInitFromCstrAlloc` for
///           full state details.
/// FAILURE : Returns `false` on allocation failure. `*out` is left as an
///           initialized-but-empty `Str`.
///
/// TAGS: Str, Init, Convert, CStr, Allocator
///
#define STR_TRY_INIT_FROM_CSTR_HAS_ARGS_IMPL(_1, _2, _3, _4, count, ...) count
#define STR_TRY_INIT_FROM_CSTR_HAS_ARGS(...)                             STR_TRY_INIT_FROM_CSTR_HAS_ARGS_IMPL(__VA_ARGS__, 4, 3, 2, 1, 0)
#define StrTryInitFromCstr(...)                                          CONCAT(StrTryInitFromCstr_, STR_TRY_INIT_FROM_CSTR_HAS_ARGS(__VA_ARGS__))(__VA_ARGS__)
#define StrTryInitFromCstr_3(out, cstr, len)                             StrTryInitFromCstrAlloc((out), (cstr), (len), DefaultAllocator())
#define StrTryInitFromCstr_4(out, cstr, len, alloc)                      StrTryInitFromCstrAlloc((out), (cstr), (len), (alloc))

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
/// SUCCESS : Returns a fully initialized `Str` whose first `len` bytes are
///           a copy of `cstr`, with capacity for at least `len + 1` bytes
///           and the chosen allocator bound.
/// FAILURE : Returns an empty `Str` (length 0, capacity 0, data NULL,
///           allocator bound) when allocation fails.
///
/// TAGS: Str, Init, Convert, CStr, Allocator
///
#define STR_INIT_FROM_CSTR_HAS_ARGS_IMPL(_1, _2, _3, count, ...) count
#define STR_INIT_FROM_CSTR_HAS_ARGS(...)                         STR_INIT_FROM_CSTR_HAS_ARGS_IMPL(__VA_ARGS__, 3, 2, 1, 0)
#define StrInitFromCstr(...)                                     CONCAT(StrInitFromCstr_, STR_INIT_FROM_CSTR_HAS_ARGS(__VA_ARGS__))(__VA_ARGS__)
#define StrInitFromCstr_2(cstr, len)                             StrInitFromCstrAlloc((cstr), (len), DefaultAllocator())
#define StrInitFromCstr_3(cstr, len, alloc)                      StrInitFromCstrAlloc((cstr), (len), (alloc))

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
/// SUCCESS : Returns a fully initialized `Str` whose contents are a copy
///           of `zstr` (length set to `ZstrLen(zstr)`); capacity is sized
///           for at least one more byte; the chosen allocator is bound.
/// FAILURE : Returns an empty `Str` (length 0, capacity 0, data NULL,
///           allocator bound) when allocation fails.
///
/// TAGS: Str, Init, Zstr, Allocator
///
#define STR_INIT_FROM_ZSTR_HAS_ARGS_IMPL(_1, _2, count, ...) count
#define STR_INIT_FROM_ZSTR_HAS_ARGS(...)                     STR_INIT_FROM_ZSTR_HAS_ARGS_IMPL(__VA_ARGS__, 2, 1, 0)
#define StrInitFromZstr(...)                                 CONCAT(StrInitFromZstr_, STR_INIT_FROM_ZSTR_HAS_ARGS(__VA_ARGS__))(__VA_ARGS__)
#define StrInitFromZstr_1(zstr)                              StrInitFromCstr((zstr), ZstrLen(zstr))
#define StrInitFromZstr_2(zstr, alloc)                       StrInitFromCstr((zstr), ZstrLen(zstr), (alloc))

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
/// SUCCESS : Returns a newly created owned `Str` containing a copy of
///           `zstr`, bound to `DefaultAllocator()`. Same state effects
///           as `StrInitFromZstr(zstr)`.
/// FAILURE : Returns an empty `Str` if allocation fails - same fallback
///           as `StrInitFromZstr(...)`.
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
/// SUCCESS : Returns a fully initialized `Str` whose contents are a deep
///           copy of `str->data` (length `str->length`); capacity is
///           sized for at least one more byte; the chosen allocator is
///           bound (defaulting to the source string's allocator).
/// FAILURE : Returns an empty `Str` (length 0, capacity 0, data NULL,
///           allocator bound) when allocation fails.
///
/// TAGS: Str, Init, Copy, Allocator
///
#define STR_INIT_FROM_STR_HAS_ARGS_IMPL(_1, _2, count, ...) count
#define STR_INIT_FROM_STR_HAS_ARGS(...)                     STR_INIT_FROM_STR_HAS_ARGS_IMPL(__VA_ARGS__, 2, 1, 0)
#define StrInitFromStr(...)                                 CONCAT(StrInitFromStr_, STR_INIT_FROM_STR_HAS_ARGS(__VA_ARGS__))(__VA_ARGS__)
#define StrInitFromStr_1(str)                               StrInitFromCstr((str)->data, (str)->length, (str)->allocator)
#define StrInitFromStr_2(str, alloc)                        StrInitFromCstr((str)->data, (str)->length, (alloc))

///
/// Clone a `Str`, inheriting the source allocator configuration.
///
/// str[in] : Source string.
///
/// SUCCESS : Returns a fully initialized `Str` that is a deep copy of
///           `str`, inheriting its allocator binding. Same state effects
///           as `StrInitFromStr(str)`.
/// FAILURE : Returns an empty `Str` (length 0, capacity 0, data NULL,
///           allocator bound) when allocation fails.
///
/// TAGS: Str, Init, Copy, Convenience
///
#define StrDup(str) StrInitFromStr(str)

    ///
    /// Init the string using the given format string and arguments.
    /// Current contents of string will be cleared out.
    ///
    /// str[in,out] : Str to be inited with format string.
    /// fmt[in]     : Format string, with variadic arguments following.
    ///
    /// SUCCESS : Returns `str`. The string has been cleared and then
    ///           populated by formatting `fmt` and the variadic
    ///           arguments; length reflects the formatted byte count.
    /// FAILURE : Returns `NULL` on allocation failure during the
    ///           formatting append. `str` may be left partially filled;
    ///           caller should treat as opaque on failure.
    ///
    Str *StrPrintf(Str *str, const char *fmt, ...) FORMAT_STRING(2, 3);

#ifdef __cplusplus
#    define StrInit(...) (Str VecInit(__VA_ARGS__))
#else
///
/// Initialize given string.
///
/// ... : Optional allocator override. If omitted, `DefaultAllocator()` is
///       used.
///
/// SUCCESS : Returns a fresh empty `Str` value with length 0, capacity 0,
///           data NULL, and the chosen allocator bound. No allocation
///           happens until the string is mutated.
/// FAILURE : Function cannot fail.
///
#    define StrInit(...) ((Str)VecInit(__VA_ARGS__))
#endif

///
/// Initialize given string but use memory from stack.
/// Such strings cannot be dynamically resized!!
///
/// str[in,out]      : Variable name to declare and initialize as a stack-backed Str.
/// ne[in]           : Number of bytes to allocate on the stack.
/// scoped_body[in]  : Code block that uses `str`; the storage is valid only
///                    within this block.
///
/// SUCCESS : Returns to the caller (the macro expands to a scoped block).
///           Inside `scoped_body`, `str` is a non-allocating Str whose
///           `data` points to stack memory sized for `ne` bytes. Capacity
///           is fixed at `ne`; any operation that would grow it will
///           crash since the stack buffer cannot be reallocated.
/// FAILURE : Function cannot fail. The stack allocation is automatic.
///
#define StrInitStack(str, ne, scoped_body) VecInitStack(str, ne, scoped_body)

    ///
    /// Deinit str by freeing all allocations.
    ///
    /// str[in,out] : Pointer to string to deinitialize.
    ///
    /// SUCCESS : Returns to the caller. The underlying byte buffer (if
    ///           any) has been freed back to the string's allocator;
    ///           length, capacity, and `data` are reset; the allocator
    ///           is unbound and reset to `AllocatorBind(DefaultAllocator())`.
    ///           The struct is safe to re-initialize or discard.
    /// FAILURE : Function cannot fail. A NULL `str` or invalid magic is
    ///           a caller bug and aborts via `LOG_FATAL`.
    ///
    void StrDeinit(Str *str);

    ///
    /// Deinitialize a copied `Str` through an explicit allocator context.
    ///
    /// This is primarily used by generic containers that own copied `Str`
    /// values and need allocator-aware copy cleanup callbacks.
    ///
    /// copy[in,out] : Pointer to the `Str` object to deinitialize.
    /// alloc[in]    : Allocator context for the owning container. The
    ///                callback must use this allocator (not the bound
    ///                one) for the underlying free.
    ///
    /// SUCCESS : Returns to the caller. The byte buffer of `*copy` has
    ///           been freed via `alloc`; length, capacity, and `data`
    ///           are reset. The struct is left in a deinited state
    ///           consistent with the container's deep-copy contract.
    /// FAILURE : Does not return if `copy` or `alloc` are NULL or
    ///           otherwise violate the callback contract; aborts via
    ///           `LOG_FATAL`.
    ///
    /// TAGS: Str, Deinit, Allocator, Callback
    ///
    void StrDeinitAlloc(void *copy, const Allocator *alloc);

    ///
    /// Copy data from `src` to `dst`.
    ///
    /// dst[out] : Str object to copy into. Must be already initialized;
    ///            current contents are discarded.
    /// src[in]  : Str object to copy from.
    ///
    /// SUCCESS : Returns `true`. `*dst` holds a deep copy of `src->data`:
    ///           length matches `src->length`; capacity is at least
    ///           `length + 1`; `dst`'s previously-allocated buffer (if
    ///           any) is reused or grown via its own allocator.
    /// FAILURE : Returns `false` on allocation failure for the
    ///           destination buffer. `*dst` is left in a valid but
    ///           empty state; caller should treat as opaque on failure.
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
    /// SUCCESS : Returns `true`. `*dst` is a deep copy of `*src` with the
    ///           byte buffer allocated through `alloc` (not the source's
    ///           allocator). Length matches `src->length`.
    /// FAILURE : Returns `false` on allocation failure for the
    ///           destination buffer. `*dst` is left in an initialized-
    ///           but-empty state.
    ///
    /// TAGS: Str, Init, Copy, Allocator, Callback
    ///
    bool StrInitCopyAlloc(void *dst, const void *src, const Allocator *alloc);

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_STR_INIT_H
