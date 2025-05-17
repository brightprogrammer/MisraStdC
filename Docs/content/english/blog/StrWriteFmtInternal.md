---
title: "StrWriteFmtInternal"
meta_title: "StrWriteFmtInternal"
description: "Function :: StrWriteFmtInternal"
date: 2025-05-17T12:50:19Z
categories: ['Function']
author: "Siddharth Mishra"
tags: ['strwritefmtinternal']
draft: false
---

# <center>`StrWriteFmtInternal` - Function</center>

## Description
Print out a formatted string with rust-style placeholders
to given string "o"
NOTE: Directly passing literals like FMT(1337) is not supported, especially const char*
literals. For constants like integers, booleans, you can use `LVAL(r-value)`
to convert an l-value to an r-value an then use in `FMT` like `FMT(LVAL(false))`
Takes in TypeSpecificIO structures as arguments. Use FMT(.)
to wrap any supported-type variable to it's TypeSpecificIO object.
o[out]     : Contents appended to this string.
fmtstr[in] : Format string with placeholders.
argv[in]   : Arguments that placeholders will be replaced with.
argc[in]   : Number of arguments.

## Syntax
```c
void StrWriteFmtInternal(...);
```

## Behavior
**Success**: Placeholders in `fmtstr` are replaced by passed arguments.

**Failure**: Does not return, displays log messages.

**Returns**: `void`
