---
title: "SysMutexUnlock"
meta_title: "SysMutexUnlock"
description: "Function :: SysMutexUnlock"
date: 2025-05-17T12:50:19Z
categories: ['Function']
author: "Siddharth Mishra"
tags: ['sysmutexunlock']
draft: false
---

# <center>`SysMutexUnlock` - Function</center>

## Description
Release lock on provided mutex object.
m[in,out] : Mutex to unlock.

## Syntax
```c
SysMutex * SysMutexUnlock(...);
```

## Behavior
**Success**: `m`

**Failure**: `NULL`

**Returns**: `SysMutex *`
