/// file      : std/utility/pair.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// `Pair(xT, yT)` -- a comma-clean anonymous struct that nests inside
/// container type slots without tripping the preprocessor's argument-
/// splitting (see `T(...)` for the matching delay-expansion wrapper).

#ifndef MISRA_STD_UTILITY_PAIR_H
#define MISRA_STD_UTILITY_PAIR_H

///
/// A wrapper macro to delay expansion
/// To be used when passing types to containers that have commas in them
///
/// Eg: Vec(Pair(i32, Str))    : This won't work!
///     Vec(T(Pair(i32, Str))) : This will!
///
#ifndef T
#    define T(x) x
#endif

#define Pair(xT, yT)                                                                                                   \
    struct {                                                                                                           \
        xT x;                                                                                                          \
        yT y;                                                                                                          \
    }

#endif // MISRA_STD_UTILITY_PAIR_H
