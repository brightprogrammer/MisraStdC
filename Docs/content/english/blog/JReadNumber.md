---
title: "JReadNumber"
meta_title: "JReadNumber"
description: "Function :: JReadNumber"
date: 2025-05-17T12:50:19Z
categories: ['Function']
author: "Siddharth Mishra"
tags: ['jreadnumber']
draft: false
---

# <center>`JReadNumber` - Function</center>

## Description
JReads a number from the given string iterator and stores the result in the provided `Number` object.
This function handles parsing integers and floating-point numbers, including those with exponents. It supports both positive and negative values.
It parses the number in a manner consistent with the JSON specification for numbers.
Parameters:
si[in]    : The `StrIter` pointing to the current position in the input string, which will be parsed for a number.
num[out]  : A pointer to a `Number` object that will hold the parsed number (either an integer or floating-point value).
This object will be populated with the result of the parsing, including the number's value and whether it is a floating-point number.
Returns:
A `StrIter` that points to the next position in the string after the parsed number. If the parsing fails, it will return the original `si` iterator.
Errors:
- Logs an error if the reading position is invalid or exhausted.
- Logs an error if the `num` parameter is `NULL`.
- Logs an error if there is an invalid number format (e.g., multiple decimal points, multiple exponent indicators, invalid characters).
- Logs an error if the number is empty after parsing (e.g., no digits found).
- Logs an error if the string cannot be converted to a valid number.
Example:
StrIter si = some_str_iter;
Number num;
StrIter new_si = JReadNumber(si, &num);
// Use the parsed number in `num` object

## Syntax
```c
StrIter JReadNumber(...);
```

**Returns**: `StrIter`
