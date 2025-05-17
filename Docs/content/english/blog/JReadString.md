---
title: "JReadString"
meta_title: "JReadString"
description: "Function :: JReadString"
date: 2025-05-17T12:50:19Z
categories: ['Function']
author: "Siddharth Mishra"
tags: ['jreadstring']
draft: false
---

# <center>`JReadString` - Function</center>

## Description
JReads a string from the given string iterator and stores the result in the provided `Str` object.
This function parses a string enclosed in double quotes (`"`) and handles escape sequences such as `\\`, `\"`, `\n`, `\t`, etc.
The parsed string is stored in the provided `Str` object, and the function will handle both normal characters and escape sequences.
Note: Unicode escape sequences (e.g., `\uXXXX`) are not supported in this implementation.
Parameters:
si[in]    : The `StrIter` pointing to the current position in the input string, which will be parsed for a string value.
str[out]  : A pointer to a `Str` object that will hold the parsed string. This object will be populated with the result of the parsing.
Returns:
A `StrIter` that points to the next position in the string after the parsed string. If the parsing fails, it will return the original `si` iterator.
Errors:
- Logs an error if the reading position is invalid or exhausted.
- Logs an error if the `str` parameter is `NULL`.
- Logs an error if there is an invalid escape sequence or an unsupported Unicode sequence.
- Logs an error if the string cannot be parsed correctly (e.g., no closing quotation mark).
Example:
StrIter si = some_str_iter;
Str str;
StrInit(&str);
StrIter new_si = JReadString(si, &str);
// Use the parsed string in `str` object

## Syntax
```c
StrIter JReadString(...);
```

**Returns**: `StrIter`
