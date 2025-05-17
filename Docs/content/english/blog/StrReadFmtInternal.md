---
title: "StrReadFmtInternal"
meta_title: "StrReadFmtInternal"
description: "Function :: StrReadFmtInternal"
date: 2025-05-17T12:50:19Z
categories: ['Function']
author: "Siddharth Mishra"
tags: ['strreadfmtinternal']
draft: false
---

# <center>`StrReadFmtInternal` - Function</center>

## Description
Parse input string according to format string with rust-style placeholders,
extracting values into provided TypeSpecificIO arguments.
Takes in TypeSpecificIO structures as arguments. Use FMT(.) to wrap any
supported-type variable to its TypeSpecificIO object.
input[in]  : Input string to parse (null-terminated)
fmtstr[in] : Format string with placeholders (null-terminated)
argv[in]   : Arguments that will be read into from placeholders.
argc[in]   : Number of arguments.

## Syntax
```c
const char * StrReadFmtInternal(...);
```

## Usage Examples
```c
const char *input = "Count: 42, Name: Alice";
int count;
Str name;
const char *remaining = StrReadFmt(input, "Count: {}, Name: {}", FMT(count), FMT(name));
```

## Behavior
**Success**: After reading through `input`, returns back const char* to start reading from (from inside `input`)

**Failure**: Does not return, displays log error messages.

**Returns**: `const char *`
