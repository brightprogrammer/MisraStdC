---
title: "ReadCompleteFile"
meta_title: "ReadCompleteFile"
description: "Function :: ReadCompleteFile"
date: 2025-05-17T12:50:19Z
categories: ['Function']
author: "Siddharth Mishra"
tags: ['readcompletefile']
draft: false
---

# <center>`ReadCompleteFile` - Function</center>

## Description
Read complete contents of file at once.
Pointer returned is malloc'd and hence must be freed after use.
The returned pointer can also be reused by providing pointer to it
in `data` parameter.
`realloc` is called on `*data` in order to expand it's size.
If `*capacity` exceeds the size of file to be loaded, then no reallocation
is performed. This means the provided buffer will automatically be expanded
if required.
The returned buffer is null-terminated just-in-case.
The implementation and API is designed in such a way that it can be used
with containers like Vec and Str.
filename[in]     : Name/path of file to be read.
data[in,out]     : Memory buffer where loaded file will be stored.
file_size[out]   : Complete size of file in bytes will be stored here.
capacity[in,out] : Hints towards current capacity of `data` buffer.
New capacity of `data` buffer is automatically stored here if
realloc is performed.

## Syntax
```c
bool ReadCompleteFile(...);
```

## Behavior
**Success**: true

**Failure**: false

**Returns**: `bool`
