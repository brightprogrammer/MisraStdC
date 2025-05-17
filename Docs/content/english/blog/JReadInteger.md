---
title: "JReadInteger"
meta_title: "JReadInteger"
description: "Function :: JReadInteger"
date: 2025-05-17T12:50:19Z
categories: ['Function']
author: "Siddharth Mishra"
tags: ['jreadinteger']
draft: false
---

# <center>`JReadInteger` - Function</center>

## Description
Strictly parses an integer from the string, failing if a floating-point value is encountered.
This function will attempt to parse an integer from the current position of the string iterator. If a floating-point value is encountered, the parsing will fail, and no advancement will be made in the iterator. The parsed integer value will be stored in the `val` parameter.
Parameters:
si[in]   : The `StrIter` pointing to the current position in the input string where the integer should be parsed.
val[out] : A pointer to an integer (`i64`) where the parsed value will be stored.
Returns:
A `StrIter` pointing to the next position after the parsed integer, or the original iterator if parsing fails.
Errors:
- Logs an error if the reading position is invalid or exhausted.
- Logs an error if the `val` pointer is `NULL`.
- Logs an error if a floating-point value is encountered during parsing.
- Logs an error if the integer parsing fails for any other reason.
Example:
StrIter si = some_str_iter;
i64 value;
StrIter new_si = JReadInteger(si, &value);
// Use the parsed integer in `value`

## Syntax
```c
StrIter JReadInteger(...);
```

**Returns**: `StrIter`
