---
title: "SysStrError"
meta_title: "SysStrError"
description: "Function :: SysStrError"
date: 2025-05-17T12:50:19Z
categories: ['Function']
author: "Siddharth Mishra"
tags: ['sysstrerror']
draft: false
---

# <center>`SysStrError` - Function</center>

## Description
Get last error using an error number.
eno[in]      : Unique error number descriptor.
err_str[out] : Error string will be stored in this.

## Syntax
```c
Str * SysStrError(...);
```

## Behavior
**Success**: Error string describing last error.

**Failure**: NULL only if `err_str` is NULL

**Returns**: `Str *`
