---
title: "SysMutexLock"
meta_title: "SysMutexLock"
description: "Function :: SysMutexLock"
date: 2025-05-17T12:50:19Z
categories: ['Function']
author: "Siddharth Mishra"
tags: ['sysmutexlock']
draft: false
---

# <center>`SysMutexLock` - Function</center>

## Description
Acquire lock on provided mutex object.
m[in,out] : Mutex to lock.

## Syntax
```c
SysMutex * SysMutexLock(...);
```

## Behavior
**Success**: `m`

**Failure**: `NULL`

**Returns**: `SysMutex *`
