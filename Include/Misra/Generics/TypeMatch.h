/// file      : generics/typematch.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// `Match` / `When` / `Otherwise` -- one matcher for two jobs:
///
///   * `When(T, bind)`    -- dispatch on a value's STATIC type via `_Generic`,
///                           binding the value to `bind`. The selector is a
///                           `_Generic` constant, so it folds to the taken arm
///                           under the optimiser (zero runtime cost).
///   * `When(N, T, bind)` -- dispatch on the RUNTIME tag of a `Variant(N, ...)`
///                           value (see Generics/Variant.h), binding the
///                           T-typed payload to `bind`.
///
/// `When` selects the form by argument count; the trailing `bind` is always a
/// name you choose, so the matched value is an explicit, reader-visible
/// variable (no implicit binding). Arms are mutually exclusive; an optional
/// trailing `Otherwise` catches the rest. A non-exhaustive match -- no `When`
/// hit and no `Otherwise` -- does NOT fall through silently: it aborts via
/// `LOG_FATAL`, matching the project's fail-fast-for-correctness stance.
///
/// The static form is type DISPATCH, not value destructuring (no struct
/// take-apart, no literal/nested patterns); the variant form binds the payload.

#ifndef MISRA_GENERICS_TYPEMATCH_H
#define MISRA_GENERICS_TYPEMATCH_H

#include <Misra/Std/Log.h>
#include <Misra/Types.h>

#ifdef __cplusplus
extern "C" {
#endif

///
/// Predicate: does the static type of `x` exactly equal type `T`?
///
/// SUCCESS : Expands to the integer constant `1` if `TYPE_OF(x)` is `T`,
///           otherwise `0`. The result is a `_Generic` constant, usable in
///           any constant context and folded away by the optimiser.
/// FAILURE : Macro cannot fail. `T` must name a single complete type token.
///
/// USAGE:
///   if (Is(x, int)) { ... }
///
/// TAGS: Generics, Match, Type, Predicate
///
#define Is(x, T) _Generic((x), T: 1, default: 0)

///
/// Open a match over `x`. The scrutinee is copied by value (so rvalues /
/// function returns work); each `When` arm binds it (or, for a variant, its
/// payload) to a name you give. Pair with `When` arms and an optional trailing
/// `Otherwise`. Arms are mutually exclusive; the block runs at most one of them.
///
/// For a static match the selector is a `_Generic` constant, so exactly one arm
/// is live per instantiation and the construct folds to it; the others are dead
/// but must still compile against the value's static type. This makes the
/// static form meaningful inside generic/macro code, and free everywhere else.
///
/// SUCCESS : Runs the body of the single matching arm (or `Otherwise` if none),
///           with that arm's named binding in scope. Block exits after one arm.
/// FAILURE : If no `When` matches and there is no `Otherwise`, the match is
///           non-exhaustive and aborts via `LOG_FATAL` -- it never silently
///           falls through. A `When` body that misuses its binding's type is a
///           compile error (every arm is type-checked).
///
/// USAGE:
///   Match(value) {
///       When(int, n)    WriteFmtLn("int {}", n);
///       When(double, d) WriteFmtLn("double {}", d);
///       Otherwise       WriteFmtLn("other");
///   }
///
/// TAGS: Generics, Match, Type, Dispatch, Compile-Time
///
#define Match(x)                                                                                                       \
    for (bool MisraMatched = false, UNPL(tm_once) = true; UNPL(tm_once); UNPL(tm_once) = false,                        \
              ASSERT_OR_FATAL(MisraMatched, "Match: no arm matched and no Otherwise (non-exhaustive)"))                \
        for (TYPE_OF(x) MisraSubject = (x), *UNPL(tm_loop) = &MisraSubject; UNPL(tm_loop); UNPL(tm_loop) = NULL)

///
/// An arm of a `Match`, in one of two forms (selected by argument count). The
/// last argument is always a name you choose; the matched value is bound to it
/// for the arm body (there is no implicit binding -- the reader sees where the
/// variable comes from):
///
///   * `When(T, bind)`    -- static: runs iff no earlier arm matched and the
///                           subject's static type is `T`; binds `bind` to the
///                           value.
///   * `When(N, T, bind)` -- variant: runs iff no earlier arm matched and the
///                           subject's runtime tag is `Variant(N, ...)`'s `T`;
///                           binds `bind` to the T-typed payload.
///
/// SUCCESS : Body runs at most once, only on a match, with `bind` in scope.
/// FAILURE : Macro cannot fail. Usable only inside a `Match`. Forgetting the
///           `bind` name (`When(T)` / `When(N, T)`) is a compile error -- in
///           particular a variant arm missing its name does NOT silently
///           become a static match. `When(N, T, bind)` for a `T` the variant
///           cannot hold is a compile error (no `as_T` member).
///
/// TAGS: Generics, Match, Type, Variant, Arm
///
#define When(...) OVERLOAD(When, __VA_ARGS__)

/* static arm: When(T, bind) -- bind = the value (its static type). */
#define When_2(T, bind)                                                                                                \
    if (!MisraMatched && Is(MisraSubject, T) && (MisraMatched = true))                                                 \
        for (TYPE_OF(MisraSubject) bind = MisraSubject, *UNPL(tm_bind) = &bind; UNPL(tm_bind); UNPL(tm_bind) = NULL)

/* variant arm: When(N, T, bind) -- bind = the T-typed payload of a Variant. */
#define When_3(N, T, bind)                                                                                             \
    if (!MisraMatched && MisraSubject.tag == N##_##T && (MisraMatched = true))                                         \
        for (T bind = MisraSubject.u.as_##T, *UNPL(tm_bind) = &bind; UNPL(tm_bind); UNPL(tm_bind) = NULL)

///
/// The fallback arm of a `Match`. Runs iff no earlier arm matched.
///
/// SUCCESS : Body runs at most once, only when every preceding arm missed.
/// FAILURE : Macro cannot fail. Usable only inside a `Match`.
///
/// TAGS: Generics, Match, Arm, Default
///
#define Otherwise if (!MisraMatched && (MisraMatched = true))

#ifdef __cplusplus
}
#endif

#endif // MISRA_GENERICS_TYPEMATCH_H
