---
title: "JSkipValue"
meta_title: "JSkipValue"
description: "Function :: JSkipValue"
date: 2025-05-17T12:50:19Z
categories: ['Function']
author: "Siddharth Mishra"
tags: ['jskipvalue']
draft: false
---

# <center>`JSkipValue` - Function</center>

## Description
Skip the value at the current position in the string.
This function is used to skip over the value at the current reading position in the string. It is primarily
used when the `Reader` in `JReadObject` doesn't read a value, allowing for selective skipping of key-value
pairs. It supports skipping different JSON value types like `true`, `false`, `null`, strings, numbers,
objects, and arrays.
si[in] : `StrIter`. Iterator to the current position in the string, where the value is to be skipped.
The error will be logged with the relevant details.
Error Cases:
- Invalid reading position.
- Exhausted string iterator range.
- Failed to parse a boolean (`true`, `false`), null, string, number, object, or array.
- Invalid JSON value encountered.
Supported Value Types:
- Boolean values: "true", "false".
- Null value: "null".
- String values: Enclosed in double quotes (`"`).
- Numbers: Integer or floating-point numbers.
- Objects: Enclosed in curly braces (`{}`).
- Arrays: Enclosed in square brackets (`[]`).

## Syntax
```c
StrIter JSkipValue(...);
```

## Behavior
**Success**: Returns the updated string iterator (`StrIter`) after skipping the value.

**Failure**: Returns the same value as the provided `si` if an error occurs while skipping the value.

**Returns**: `StrIter`
