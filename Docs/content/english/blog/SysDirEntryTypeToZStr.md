---
title: "SysDirEntryTypeToZStr"
meta_title: "SysDirEntryTypeToZStr"
description: "Function :: SysDirEntryTypeToZStr"
date: 2025-05-17T12:50:19Z
categories: ['Function']
author: "Siddharth Mishra"
tags: ['sysdirentrytypetozstr']
draft: false
---

# <center>`SysDirEntryTypeToZStr` - Function</center>

## Description
Convert given entry type to a NULL terminated string.
Provided string must not be freed as it's not allocated.
type[in] : Entry type to get string of.
RETURN : Null terminated string.

## Syntax
```c
const char * SysDirEntryTypeToZStr(...);
```

**Returns**: `const char *`
