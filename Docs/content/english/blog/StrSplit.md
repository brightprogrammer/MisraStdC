---
title: "StrSplit"
meta_title: "StrSplit"
description: "Function :: StrSplit"
date: 2025-05-17T12:50:19Z
categories: ['Function']
author: "Siddharth Mishra"
tags: ['strsplit']
draft: false
---

# <center>`StrSplit` - Function</center>

## Description
Split the given Str object into multiple Str objects stored in a vector
of Str objects. Each Str object in returned vector is a new Str object
and hence must be deinited after use. Calling `VecDeinit()` on the returned
vector will do that for you automatically for all the objects.
This is best used when iterating over a delimited data is not the only goal,
but also other modifications like stripping over whitespaces from returned Str objects.
str[in] : Str object to split
key[in] : Zero-terminated char pointer value to split based on

## Syntax
```c
Strs StrSplit(...);
```

## Behavior
**Success**: Strs vector of non-zero length

**Failure**: Strs vector of zero-length

**Returns**: `Strs`
