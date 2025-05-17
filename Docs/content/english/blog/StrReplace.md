---
title: "StrReplace"
meta_title: "StrReplace"
description: "Function :: StrReplace"
date: 2025-05-17T12:50:19Z
categories: ['Function']
author: "Siddharth Mishra"
tags: ['strreplace']
draft: false
---

# <center>`StrReplace` - Function</center>

## Description
Replace occurrences of a Str in string with another Str.
s[in,out]     : Str to modify.
match[in]     : Str to match.
replacement[in]: Str to replace with.
count[in]     : Maximum number of replacements. -1 means replace all occurences.

## Syntax
```c
void StrReplace(...);
```

## Behavior
**Success**: Modifies `s` in place.

**Failure**: No replacement if `match` not found.

**Returns**: `void`
