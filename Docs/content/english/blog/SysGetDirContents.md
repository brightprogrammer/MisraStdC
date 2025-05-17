---
title: "SysGetDirContents"
meta_title: "SysGetDirContents"
description: "Function :: SysGetDirContents"
date: 2025-05-17T12:50:19Z
categories: ['Function']
author: "Siddharth Mishra"
tags: ['sysgetdircontents']
draft: false
---

# <center>`SysGetDirContents` - Function</center>

## Description
Read directory contents into a vector
Current contents of the vector will be cleared out.
path[in]    : Path of directory get content of.

## Syntax
```c
SysDirContents SysGetDirContents(...);
```

## Behavior
**Success**: SysDirContents vector filled with directory contents data.

**Failure**: Empty vector.

**Returns**: `SysDirContents`
