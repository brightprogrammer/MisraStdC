---
title: "StrEndsWithCstr"
meta_title: "StrEndsWithCstr"
description: "Function :: StrEndsWithCstr"
date: 2025-05-17T12:50:19Z
categories: ['Function']
author: "Siddharth Mishra"
tags: ['strendswithcstr']
draft: false
---

# <center>`StrEndsWithCstr` - Function</center>

## Description
Check if string ends with a fixed-length C-style string (Cstr).
s[in]         : Str to check.
suffix[in]    : Pointer to suffix character array.
suffix_len[in]: Length of suffix.

## Syntax
```c
bool StrEndsWithCstr(...);
```

## Behavior
**Success**: Returns true if `s` ends with `suffix`.

**Failure**: Returns false.

**Returns**: `bool`
