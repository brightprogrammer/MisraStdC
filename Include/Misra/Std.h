/// file      : misra/std.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Std subsystem umbrella. Pulls in foundation pieces plus every
/// optional Std-side feature the current build enabled.

#ifndef MISRA_STD_H
#define MISRA_STD_H

#include <Misra/Types.h>

// Foundation: always available.
#include <Misra/Std/Allocator.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Allocator/Heap.h>
#include <Misra/Std/Allocator/Page.h>
#include <Misra/Std/ArgParse.h>
#include <Misra/Std/Container.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Container/Vec.h>
#include <Misra/Std/Io.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Math.h>
#include <Misra/Std/Memory.h>
#include <Misra/Std/Prng.h>
#include <Misra/Std/Utility.h>
#include <Misra/Std/Zstr.h>

#if FEATURE_ALLOC_ARENA
#    include <Misra/Std/Allocator/Arena.h>
#endif
#if FEATURE_ALLOC_SLAB
#    include <Misra/Std/Allocator/Slab.h>
#endif
#if FEATURE_ALLOC_BUDGET
#    include <Misra/Std/Allocator/Budget.h>
#endif
#if FEATURE_ALLOC_DEBUG
#    include <Misra/Std/Allocator/Debug.h>
#endif

#if FEATURE_FILE
#    include <Misra/Std/File.h>
#endif

#endif // MISRA_STD_H
