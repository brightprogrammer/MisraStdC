---
title: "JReadBool"
meta_title: "JReadBool"
description: "Function :: JReadBool"
date: 2025-05-17T12:50:19Z
categories: ['Function']
author: "Siddharth Mishra"
tags: ['jreadbool']
draft: false
---

# <center>`JReadBool` - Function</center>

## Description
JRead a boolean value ("true" or "false") from the string.
This function parses a boolean value from the current position of the string iterator. If the value is "true",
the parsed value will be `true`. If the value is "false", it will be parsed as `false`. If anything else is encountered,
the parsing fails, and the iterator is returned without advancement. The parsed boolean value is stored in the `b` parameter.
Parameters:
si[in]   : The `StrIter` pointing to the current position in the input string where the boolean value should be parsed.
b[out]   : A pointer to a boolean (`bool`) where the parsed value will be stored.
Returns:
A `StrIter` pointing to the next position after the parsed boolean value, or the original iterator if parsing fails.
Errors:
- Logs an error if the reading position is invalid or exhausted.
- Logs an error if the `b` pointer is `NULL`.
- Logs an error if the expected "true" or "false" value is not found.
- Logs an error if the string length is insufficient to parse a boolean value.
Example:
StrIter si = some_str_iter;
bool value;
StrIter new_si = JReadBool(si, &value);
// Use the parsed boolean value in `value`

## Syntax
```c
StrIter JReadBool(...);
```

**Returns**: `StrIter`
