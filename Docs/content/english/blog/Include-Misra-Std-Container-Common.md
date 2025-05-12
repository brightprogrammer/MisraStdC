---
title: "Include-Misra-Std-Container-Common.h"
meta_title: "Include-Misra-Std-Container-Common.h"
description: "Documentation for Include-Misra-Std-Container-Common.h"
date: 2025-05-12T05:00:00Z
# image: "/images/image-placeholder.png"
categories: ["Vec", "Macro", "Generic"]
author: "Siddharth Mishra"
tags: ["vec", "macro", "generic"]
draft: false
---
```c
/// file      : std/container/common.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// copyright : Copyright (c) 2025, Siddharth Mishra, All rights reserved.
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
typedef int (*GenericCompare)(const void *first, const void *second);

#endif // MISRA_STD_CONTAINER_COMMON_H

```