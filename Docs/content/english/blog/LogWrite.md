---
title: "LogWrite"
meta_title: "LogWrite"
description: "Function :: LogWrite"
date: 2025-05-17T12:50:19Z
categories: ['Function']
author: "Siddharth Mishra"
tags: ['logwrite']
draft: false
---

# <center>`LogWrite` - Function</center>

## Description
Generate the log message
type[in]   : Log message type (info, error, fatal)
tag[in]    : Log message idenfifier, something like file, function, name etc...
line[in]   : Line number at which this log was generated.
format[in] : Format string and following variadic arguments

## Syntax
```c
void LogWrite(...);
```

**Returns**: `void`
