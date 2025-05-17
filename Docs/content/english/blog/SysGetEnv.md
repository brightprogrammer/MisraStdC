---
title: "SysGetEnv"
meta_title: "SysGetEnv"
description: "Function :: SysGetEnv"
date: 2025-05-17T12:50:19Z
categories: ['Function']
author: "Siddharth Mishra"
tags: ['sysgetenv']
draft: false
---

# <center>`SysGetEnv` - Function</center>

## Description
Get environment value value in a `Str` object.
Object must be destroyed after use.
name[in]   : Name of environment variable.
value[out] : Value of environment variable.

## Syntax
```c
Str * SysGetEnv(...);
```

## Behavior
**Success**: `Str` object containing value of environment variable.

**Failure**: `NULL`

**Returns**: `Str *`
