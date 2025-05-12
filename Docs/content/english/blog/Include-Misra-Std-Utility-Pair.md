---
title: "Include-Misra-Std-Utility-Pair.h"
meta_title: "Include-Misra-Std-Utility-Pair.h"
description: "Documentation for Include-Misra-Std-Utility-Pair.h"
date: 2025-05-12T05:00:00Z
# image: "/images/image-placeholder.png"
categories: ["Vec", "Macro", "Generic"]
author: "Siddharth Mishra"
tags: ["vec", "macro", "generic"]
draft: false
---
```c
/*
 * Filename: Pair.h
 * Author: Siddharth Mishra <admin@brightprogrammer.in>
 * Created: 2025-05-04
 *
 * Copyright (c) 2025 Siddharth Mishra . All rights reserved.
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

```