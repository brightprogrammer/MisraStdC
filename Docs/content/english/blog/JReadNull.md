---
title: "JReadNull"
meta_title: "JReadNull"
description: "Function :: JReadNull"
date: 2025-05-17T12:50:19Z
categories: ['Function']
author: "Siddharth Mishra"
tags: ['jreadnull']
draft: false
---

# <center>`JReadNull` - Function</center>

## Description
JRead a null value from the string.
This function parses a "null" value from the current position of the string iterator. If the value is "null",
the parsed value will be set to `true` in the `is_null` parameter. If anything else is encountered, the parsing fails,
and the iterator is returned without advancement. The result will indicate whether a "null" value was found.
Parameters:
si[in]       : The `StrIter` pointing to the current position in the input string where the null value should be parsed.
is_null[out] : A pointer to a boolean (`bool`) that will be set to `true` if a "null" value is parsed, `false` otherwise.
Returns:
A `StrIter` pointing to the next position after the parsed "null" value, or the original iterator if parsing fails.
Errors:
- Logs an error if the reading position is invalid or exhausted.
- Logs an error if the `is_null` pointer is `NULL`.
- Logs an error if the expected "null" value is not found.
- Logs an error if the string length is insufficient to parse a "null" value.
Example:
StrIter si = some_str_iter;
bool is_null_value;
StrIter new_si = CheckNull(si, &is_null_value);
// Use the result in `is_null_value`

## Syntax
```c
StrIter JReadNull(...);
```

**Returns**: `StrIter`
