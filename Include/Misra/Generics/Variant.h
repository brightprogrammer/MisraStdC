/// file      : generics/variant.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// By-value tagged-union sum types. `Variant(Name, T...)` generates a
/// per-variant tag enum, the tagged union, and a typed constructor per
/// payload. Pair with `Match` / `When` from `Generics/TypeMatch.h`:
/// `When(Name, T, bind)` dispatches on the runtime tag and binds the T-typed
/// payload to the name `bind`.
///
/// Self-contained -- no global registration. Two variants sharing a payload
/// type never collide (each tag enum is namespaced by the variant name). Up
/// to 256 payload types (the foreach-expansion ceiling); exceeding it is a
/// loud compile error, not silent truncation. Payload type names must each be
/// a single token -- `typedef` compound types (`unsigned int`, `char *`)
/// before listing them.

#ifndef MISRA_GENERICS_VARIANT_H
#define MISRA_GENERICS_VARIANT_H

#include <Misra/Types.h>

#ifdef __cplusplus
extern "C" {
#endif

// A generated constructor is rarely called for every payload type, so mark
// them maybe-unused. clang's `-Wunused-function` fires for `static inline`
// definitions emitted into a source file; MSVC has no such attribute.
#if defined(__GNUC__) || defined(__clang__)
#    define VARIANT_UNUSED __attribute__((unused))
#else
#    define VARIANT_UNUSED
#endif

// Context-threading sibling of `APPLY_MACRO_FOREACH` (Types.h): applies
// `gen(ctx, elem)` to each element, reusing the same `__VA_OPT__` trampoline
// (`TRICK_EXPAND` / `TRICK_PARENS`) and therefore the same ~256-element
// ceiling. `ctx` is the variant name, threaded so the generators can
// namespace the tag constants and constructors.
#define APPLY_MACRO_FOREACH_C(gen, ctx, ...)                                                                           \
    __VA_OPT__(TRICK_EXPAND(APPLY_MACRO_FOREACH_C_HELPER(gen, ctx, __VA_ARGS__)))
#define APPLY_MACRO_FOREACH_C_HELPER(gen, ctx, a1, ...)                                                                \
    gen(ctx, a1) __VA_OPT__(APPLY_MACRO_FOREACH_C_AGAIN TRICK_PARENS(gen, ctx, __VA_ARGS__))
#define APPLY_MACRO_FOREACH_C_AGAIN() APPLY_MACRO_FOREACH_C_HELPER

// Per-payload code generators (internal).
#define VARIANT_ENUMERATOR(N, T) N##_##T,
#define VARIANT_FIELD(T)         T as_##T;
#define VARIANT_CTOR(N, T)                                                                                             \
    static inline VARIANT_UNUSED N N##_from_##T(T value) {                                                             \
        N out;                                                                                                         \
        out.tag      = N##_##T;                                                                                        \
        out.u.as_##T = value;                                                                                          \
        return out;                                                                                                    \
    }

///
/// Declare a by-value tagged-union sum type `N` over the listed payload types.
/// Emits a per-variant tag enum `enum N##_Tag` with one namespaced constant
/// `N##_<T>` per type, the tagged union, and a `static inline N N##_from_<T>`
/// constructor per type. No global registration -- matchable directly with
/// `Match` / `When(N, T)`.
///
/// SUCCESS : Defines `enum N##_Tag { N_<T>, ... }`, `typedef struct { enum
///           N##_Tag tag; union { T as_<T>; ... } u; } N;`, and a typed
///           constructor per payload. A trailing `;` after the macro is
///           required and consumed.
/// FAILURE : Macro cannot fail. More than 256 payloads is a compile error
///           (the foreach ceiling). Payload type names must each be a single
///           token; compound types must be `typedef`-d first.
///
/// USAGE:
///   Variant(Number, int, float, double);
///   Number n = Number_from_int(7);
///   // match with: Match(n) { When(Number, int, x) { ... x ... } ... }
///
/// TAGS: Generics, Variant, SumType, Constructor
///
#define Variant(N, ...)                                                                                                \
    enum N##_Tag {APPLY_MACRO_FOREACH_C(VARIANT_ENUMERATOR, N, __VA_ARGS__)};                                          \
    typedef struct {                                                                                                   \
        enum N##_Tag tag;                                                                                              \
        union {                                                                                                        \
            APPLY_MACRO_FOREACH(VARIANT_FIELD, __VA_ARGS__)                                                            \
        } u;                                                                                                           \
    } N;                                                                                                               \
    APPLY_MACRO_FOREACH_C(VARIANT_CTOR, N, __VA_ARGS__)                                                                \
    struct N##_variant_force_semicolon /* swallow the trailing `;` */

#ifdef __cplusplus
}
#endif

#endif // MISRA_GENERICS_VARIANT_H
