/// file      : std/container/common.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Common definitions for all containers

#ifndef MISRA_STD_CONTAINER_COMMON_H
#define MISRA_STD_CONTAINER_COMMON_H

#include <Misra/Types.h>

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

// All deinit methods are expected to properly deinitialize all pointers
// to NULL. It's better if data is memset to 0.
//
// All init methods must expect to get a pre-initialized dst object.
// If that is the case then they must properly de-initialize the dst object
// or attempt to reuse any resources if possible and then initialize the copy
// from src to dst.

typedef bool (*GenericCopyInit)(void *dst, void *src);
typedef void (*GenericCopyDeinit)(void *copy);
typedef i32 (*GenericCompare)(const void *first, const void *second);
typedef u64 (*GenericHash)(const void *data, u32 size);

#endif // MISRA_STD_CONTAINER_COMMON_H
