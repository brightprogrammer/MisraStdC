---
title: "JReadFloat"
meta_title: "JReadFloat"
description: "Function :: JReadFloat"
date: 2025-05-17T12:50:19Z
categories: ['Function']
author: "Siddharth Mishra"
tags: ['jreadfloat']
draft: false
---

# <center>`JReadFloat` - Function</center>

## Description
JReads a floating-point number from the string. If an integer is encountered, it will be converted to a float.
This function parses a floating-point number from the current position of the string iterator. If an integer is encountered instead, it will be converted into a floating-point value. The parsed value is stored in the `val` parameter.
Parameters:
si[in]   : The `StrIter` pointing to the current position in the input string where the floating-point number should be parsed.
val[out] : A pointer to a float (`f64`) where the parsed value will be stored.
Returns:
A `StrIter` pointing to the next position after the parsed floating-point number, or the original iterator if parsing fails.
Errors:
- Logs an error if the reading position is invalid or exhausted.
- Logs an error if the `val` pointer is `NULL`.
- Logs an error if the floating-point parsing fails for any other reason.
Example:
StrIter si = some_str_iter;
f64 value;
StrIter new_si = JReadFloat(si, &value);
// Use the parsed floating-point number in `value`

## Syntax
```c
StrIter JReadFloat(...);
```

**Returns**: `StrIter`
