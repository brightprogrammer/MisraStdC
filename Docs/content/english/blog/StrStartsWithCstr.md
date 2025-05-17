---
title: "StrStartsWithCstr"
meta_title: "StrStartsWithCstr"
description: "Function :: StrStartsWithCstr"
date: 2025-05-17T12:50:19Z
categories: ['Function']
author: "Siddharth Mishra"
tags: ['strstartswithcstr']
draft: false
---

# <center>`StrStartsWithCstr` - Function</center>

## Description
Check if string starts with a fixed-length C-style string (Cstr).
s[in]         : Str to check.
prefix[in]    : Pointer to prefix character array.
prefix_len[in]: Length of prefix.

## Syntax
```c
bool StrStartsWithCstr(...);
```

## Behavior
**Success**: Returns true if `s` starts with `prefix`.

**Failure**: Returns false.

**Returns**: `bool`
