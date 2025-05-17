---
title: "StrSplitToIters"
meta_title: "StrSplitToIters"
description: "Function :: StrSplitToIters"
date: 2025-05-17T12:50:19Z
categories: ['Function']
author: "Siddharth Mishra"
tags: ['strsplittoiters']
draft: false
---

# <center>`StrSplitToIters` - Function</center>

## Description
Split given string into multiple StrIter into the same string.
This way the split operation can be performed without creating new strings,
but instead just having an iterated view into the Str object.
This is best used when user never needs to make modifications and save
the modifications. In other words, best used when only need iteration
over string with some delimiters.
str[in] : Str object to split
key[in] : Zero-terminated char pointer value to split based on

## Syntax
```c
StrIters StrSplitToIters(...);
```

## Behavior
**Success**: StrIters vector of non-zero length

**Failure**: StrIters vector of zero-length

**Returns**: `StrIters`
