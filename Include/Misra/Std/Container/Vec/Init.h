/// file      : std/container/vec/init.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Different types of initializers for a vector.

#ifndef MISRA_STD_CONTAINER_VEC_INIT_H
#define MISRA_STD_CONTAINER_VEC_INIT_H

#include "Type.h"
#include <Misra/Std/Allocator.h>
#include <Misra/Std/Memory.h>

///
/// Initialize a Vec bound to an allocator. The argument may be either
/// a typed allocator handle (`&heap`, `&arena`, ...) or a raw
/// `Allocator *` — `ALLOCATOR_OF` typechecks both at compile time and
/// converts to `Allocator *` via a whole-pointer typecast.
///
/// Inside a `Scope(...)` block the allocator argument may be omitted;
/// the macro then binds to the internal `MisraScope` allocator the
/// scope provides. Outside a `Scope`, calling `VecInit()` with no
/// argument fails to compile because `MisraScope` is undeclared - the
/// safety net the design relies on.
///
/// USAGE:
///   Scope(alloc, DefaultAllocator) {
///       Vec(int) v = VecInit();      // uses MisraScope
///       Vec(int) w = VecInit(alloc); // uses the named user-pool
///       ...
///       VecDeinit(&v);
///       VecDeinit(&w);
///   }
///
/// TAGS: Init, Vec, Length, Size
///
#define VecInit(...) MISRA_OVERLOAD(VecInit, __VA_ARGS__)
#define VecInit_0()  VecInit_1(MisraScope)
#define VecInit_1(allocator_ptr)                                                                                       \
    {.length      = 0,                                                                                                 \
     .capacity    = 0,                                                                                                 \
     .copy_init   = NULL,                                                                                              \
     .copy_deinit = NULL,                                                                                              \
     .data        = NULL,                                                                                              \
     .allocator   = ALLOCATOR_OF(allocator_ptr),                                                                       \
     .__magic     = VEC_MAGIC}

///
/// Typed-cast variant of `VecInit` for assigning into a typed Vec
/// variable. The cast makes the macro usable both as an in-place
/// initializer (`Vec(int) v = VecInitT(v, alloc);`) and as an
/// assignment target. The allocator argument is optional inside a
/// `Scope` block.
///
#define VecInitT(v, ...) MISRA_OVERLOAD(VecInitT, v, __VA_ARGS__)
#ifdef __cplusplus
#    define VecInitT_1(v)            (TYPE_OF(v) VecInit_1(MisraScope))
#    define VecInitT_2(v, alloc_ptr) (TYPE_OF(v) VecInit_1(alloc_ptr))
#else
#    define VecInitT_1(v)            ((TYPE_OF(v))VecInit_1(MisraScope))
#    define VecInitT_2(v, alloc_ptr) ((TYPE_OF(v))VecInit_1(alloc_ptr))
#endif

///
/// Initialize a Vec with deep-copy callbacks. The allocator argument
/// is optional in the same way as `VecInit` - inside a `Scope` block
/// you may omit it and `MisraScope` is used automatically.
///
#define VecInitWithDeepCopy(...)      MISRA_OVERLOAD(VecInitWithDeepCopy, __VA_ARGS__)
#define VecInitWithDeepCopy_2(ci, cd) VecInitWithDeepCopy_3(ci, cd, MisraScope)
#define VecInitWithDeepCopy_3(ci, cd, allocator_ptr)                                                                   \
    {.length      = 0,                                                                                                 \
     .capacity    = 0,                                                                                                 \
     .copy_init   = (GenericCopyInit)(ci),                                                                             \
     .copy_deinit = (GenericCopyDeinit)(cd),                                                                           \
     .data        = NULL,                                                                                              \
     .allocator   = ALLOCATOR_OF(allocator_ptr),                                                                       \
     .__magic     = VEC_MAGIC}

#define VecInitWithDeepCopyT(v, ...) MISRA_OVERLOAD(VecInitWithDeepCopyT, v, __VA_ARGS__)
#ifdef __cplusplus
#    define VecInitWithDeepCopyT_3(v, ci, cd)            (TYPE_OF(v) VecInitWithDeepCopy_3(ci, cd, MisraScope))
#    define VecInitWithDeepCopyT_4(v, ci, cd, alloc_ptr) (TYPE_OF(v) VecInitWithDeepCopy_3(ci, cd, alloc_ptr))
#else
#    define VecInitWithDeepCopyT_3(v, ci, cd)            ((TYPE_OF(v))VecInitWithDeepCopy_3(ci, cd, MisraScope))
#    define VecInitWithDeepCopyT_4(v, ci, cd, alloc_ptr) ((TYPE_OF(v))VecInitWithDeepCopy_3(ci, cd, alloc_ptr))
#endif

///
/// Stack-backed `Vec` scope. The macro declares `name` as a
/// `Vec(T)` whose backing storage is a fixed-capacity stack array
/// of `ne` elements, then opens a scope where the body sees `name`
/// as an initialised, empty vector. `name`'s lexical scope is
/// exactly the body: outside the macro the identifier is no longer
/// bound.
///
/// No allocator -- the backing storage is the stack. The body is
/// responsible for keeping content bounded by `ne`. Any operation
/// that would grow `name` past `ne` lands in `reserve_vec`, sees
/// the NULL allocator, and aborts via
/// `LOG_FATAL("vector not growable, no allocator assigned, probably stack inited")`.
/// Deep-copy callbacks are out of scope for stack-backed Vecs: if
/// you need inner-resource deep copies, use a heap-backed Vec where
/// the allocator is unambiguous.
///
/// The macro uses the for-chain scope idiom -- the body is regular
/// code below the macro, not a brace-delimited argument:
///
///     VecInitStack(int, v, 16) {
///         VecPushBackR(&v, 42);
///         ...
///     }
///
/// `T` is the element type; the macro mints a fresh `Vec(T)` struct
/// for `name`. This matches how `StrInitStack(name, ne)` mints a
/// fresh `Str` -- callers do not pre-declare the variable.
///
/// SUCCESS : Body runs once; `name` is valid for the body's scope.
///           On normal fall-through, backing array and the `Vec`
///           handle are both zeroed by the macro's exit updates.
/// FAILURE : The macro itself cannot fail. Operations inside the
///           body that try to grow `name` past `ne` abort.
///
/// CAVEAT  : `return` / `goto` leaving the body skip ALL exit
///           updates. `break` exits the innermost for cleanly: the
///           backing array is still zeroed by the outer for's
///           update, but the inner update that zeroes the `name`
///           handle is skipped -- harmless because `name`'s scope is
///           the body anyway and the identifier is no longer
///           reachable after the macro.
///           `continue` inside the body does NOT restart -- it
///           jumps to the inner for's update clause, which zeroes
///           `name` and exits the scope; treat it as a silent
///           early-exit, not a loop control.
///           `ne` is evaluated four times in the macro body (twice
///           in the array dimension `sizeof(T) * ((ne) + 1)`, once
///           for the `Vec(T)` capacity assignment, and once for the
///           `_s.d` `sizeof` MemSet); pass a side-effect-free
///           expression (literal or simple variable).
///
/// TAGS: Vec, Init, Stack, Scope
///
#define VecInitStack(T, name, ne)                                                                                      \
    for (struct {                                                                                                      \
             _Alignas(T) char d[sizeof(T) * ((ne) + 1)];                                                               \
             int              done;                                                                                    \
         } UNPL(_s) = {{0}, 0};                                                                                        \
         UNPL(_s).done == 0;                                                                                           \
         MemSet(UNPL(_s).d, 0, sizeof(UNPL(_s).d)), UNPL(_s).done = 1)                                                 \
        for (Vec(T) name = {.length      = 0,                                                                          \
                            .capacity    = (ne),                                                                       \
                            .copy_init   = NULL,                                                                       \
                            .copy_deinit = NULL,                                                                       \
                            .data        = (T *)UNPL(_s).d,                                                            \
                            .allocator   = NULL,                                                                       \
                            .__magic     = VEC_MAGIC},                                                                 \
                 *UNPL(_done) = &name;                                                                                  \
             UNPL(_done);                                                                                              \
             MemSet(&name, 0, sizeof(name)), UNPL(_done) = NULL)

///
/// Deinit vec by freeing its backing buffer.
///
#define VecDeinit(v) deinit_vec(GENERIC_VEC(v), sizeof(VEC_DATATYPE(v)))

#endif // MISRA_STD_CONTAINER_VEC_INIT_H
