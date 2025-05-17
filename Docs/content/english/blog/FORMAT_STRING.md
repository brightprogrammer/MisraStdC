---
title: "FORMAT_STRING"
meta_title: "FORMAT_STRING"
description: "Function :: FORMAT_STRING"
date: 2025-05-17T12:50:19Z
categories: ['Function']
author: "Siddharth Mishra"
tags: ['format_string']
draft: false
---

# <center>`FORMAT_STRING` - Function</center>

## Description
Print and append into given string object with given format.
str[in,out] : Str to print into.
fmt[in] : Format string, followed by variadic arguments.

## Syntax
```c
Str* StrAppendf(Str* str, const char* fmt, ...) FORMAT_STRING(...);
```

## Behavior
**Success**: `str`

**Failure**: NULL

**Returns**: `Str* StrAppendf(Str* str, const char* fmt, ...)`
