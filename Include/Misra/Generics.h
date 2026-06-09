/// file      : generics.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Generics subsystem umbrella. Header-only metaprogramming built on
/// `_Generic` and the `__VA_OPT__` foreach engine: a compile-time type match
/// and by-value sum types, sharing one `Match` / `When` / `Otherwise`
/// (`When(T)` on a static type, `When(N, T)` on a `Variant(N, ...)` value).
///
/// Deliberately NOT pulled through the top-level `Misra.h` umbrella: the
/// public macros here use short, ergonomic names (`Match`, `When`,
/// `Otherwise`, `Variant`) that would collide with ordinary downstream
/// identifiers if
/// injected into every translation unit. Include this header (or a specific
/// file under `Generics/`) directly where you want the feature -- the same
/// carve-out the binary-format parsers use.

#ifndef MISRA_GENERICS_H
#define MISRA_GENERICS_H

#include <Misra/Generics/TypeMatch.h>
#include <Misra/Generics/Variant.h>

#endif // MISRA_GENERICS_H
