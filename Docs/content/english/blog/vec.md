---
title: "Vec"
meta_title: "Vec"
description: "Generic Type Macro :: Vec"
date: 2025-05-12T05:00:00Z
# image: "/images/image-placeholder.png"
categories: ["Vec", "Macro", "Generic"]
author: "Siddharth Mishra"
tags: ["vec", "macro", "generic"]
draft: false
---

# <center>`Vec(T)` - Typesafe Vector</center>

## Description

The `Vec(T)` macro provides a typesafe vector implementation similar to C++'s `std::vector<T>`.
It creates a dynamically resizable array for any given type while maintaining type safety.

## Syntax

```c
Vec(TypeName) variable_name;
```

## Members

The macro expands to a struct with these members:

| Member        | Type                | Description                              |
| ------------- | ------------------- | ---------------------------------------- |
| `length`      | `size`              | Current number of elements in the vector |
| `capacity`    | `size`              | Total allocated storage capacity         |
| `copy_init`   | `GenericCopyInit`   | Function for copy initialization         |
| `copy_deinit` | `GenericCopyDeinit` | Function for copy deinitialization       |
| `data`        | `T*`                | Pointer to the array of elements         |
| `alignment`   | `size`              | Memory alignment requirement             |

## Usage Examples

```c
// Vector of integers
Vec(int) integers;

// Vector of custom structures
Vec(CustomStruct) my_data;

// Vector of float values
Vec(float) real_numbers;

// Vector of C-style strings
Vec(const char*) names;
```

## Notes

`copy_init`/`copy_deinit` need proper setup for types needing special pointer/memory handling

# Advantages

- Compile-time type safety
- Works with any type
- Similar to C++ `std::vector`
- Explicit memory control
