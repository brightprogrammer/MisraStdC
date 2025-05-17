---
title: "SysGetFileSize"
meta_title: "SysGetFileSize"
description: "Function :: SysGetFileSize"
date: 2025-05-17T12:50:19Z
categories: ['Function']
author: "Siddharth Mishra"
tags: ['sysgetfilesize']
draft: false
---

# <center>`SysGetFileSize` - Function</center>

## Description
Get size of file without opening it.
filename[in] : Name/path of file.

## Syntax
```c
i64 SysGetFileSize(...);
```

## Behavior
**Success**: Non-negative value representing size of file in bytes.

**Failure**: -1

**Returns**: `i64`
