/*
 * Filename: Pair.h
 * Author: Siddharth Mishra <admin@brightprogrammer.in>
 * Created: 2025-05-04
 *
 * This is free and unencumbered software released into the public domain.
 *
 * This source code is the intellectual property of the author.
 * Redistribution or use, in whole or in part, with or without
 * modification, is strictly prohibited without prior written permission.
 */

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

