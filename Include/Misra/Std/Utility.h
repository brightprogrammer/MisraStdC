/*
 * Filename: Utility.h
 * Author: Siddharth Mishra <admin@brightprogrammer.in>
 * Created: 2025-05-04
 *
 * This is free and unencumbered software released into the public domain.
 *
 * This source code is the intellectual property of the author.
 * Redistribution or use, in whole or in part, with or without
 * modification, is strictly prohibited without prior written permission.
 */

#ifndef MISRA_STD_UTILITY_H
#define MISRA_STD_UTILITY_H

#include <Misra/Types.h>

// Foundation: header-only utilities.
#include <Misra/Std/Utility/Pair.h>
#include <Misra/Std/Utility/StrIter.h>

#if FEATURE_ITER
#    include <Misra/Std/Utility/Iter.h>
#endif

#endif // MISRA_STD_UTILITY_H
