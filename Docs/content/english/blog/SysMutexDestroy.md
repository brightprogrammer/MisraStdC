---
title: "SysMutexDestroy"
meta_title: "SysMutexDestroy"
description: "Function :: SysMutexDestroy"
date: 2025-05-17T12:50:19Z
categories: ['Function']
author: "Siddharth Mishra"
tags: ['sysmutexdestroy']
draft: false
---

# <center>`SysMutexDestroy` - Function</center>

## Description
Destroy the provided mutex object.
Once a mutex is destroyed, all resources held by it will be freed.
Using it after this cal is UB.
m[in] : Mutex object to be destroyed.

## Syntax
```c
void SysMutexDestroy(...);
```

**Returns**: `void`
