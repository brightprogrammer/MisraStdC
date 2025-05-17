---
title: "StrReplaceZstr"
meta_title: "StrReplaceZstr"
description: "Function :: StrReplaceZstr"
date: 2025-05-17T12:50:19Z
categories: ['Function']
author: "Siddharth Mishra"
tags: ['strreplacezstr']
draft: false
---

# <center>`StrReplaceZstr` - Function</center>

## Description
Replace occurrences of a null-terminated string (Zstr) in string.
s[in,out]      : Str to modify.
match[in]      : Null-terminated match string.
replacement[in]: Null-terminated replacement string.
count[in]      : Maximum number of replacements. -1 means replace all occurences.

## Syntax
```c
void StrReplaceZstr(...);
```

## Behavior
**Success**: Modifies `s` in place.

**Failure**: No replacement if `match` not found.

**Returns**: `void`
