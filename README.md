# MisraStdC

[![Build and Test](https://github.com/brightprogrammer/MisraStdC/actions/workflows/test.yml/badge.svg)](https://github.com/brightprogrammer/MisraStdC/actions/workflows/test.yml)

A library to make programming in C less painful for you and me.

Features :
- MSVC, GCC, Clang, all three major compilers supported
- Generic containers
  - `Vec(T)` : Work with any type in a type-safe manner with strict type checking.
  - `Str`    : Just a `typedef` of `Vec(char)` but provides it's own wrapper functions.
  - `Map(K, V)` : Generic key-value hash-map storage container (Work in progress...)
  - `Int` : A custom big int implementation (Work in progress...)
- Rust style Fmt IO
  - `WriteFmt`, `ReadFmt` : To write and read from standard I/O in a type-safe formatted manner.
  - `StrWriteFmt`, `StrReadFmt` :  To write and read from strigs in a type-safe formatted manner.

Take a look at [docs](https://docs.brightprogrammer.in) to get a list of functions and generic macros with their usage examples.

## Example

Here are examples demonstrating the key features of MisraStdC:

### Vector Container (Vec)
```c
#include <Misra.h>

int compare_ints(const void* a, const void* b) {
    return *(const int*)a - *(const int*)b;
}

int main() {
    // Initialize vector with default alignment
    Vec(int) numbers = VecInit();
    
    // Pre-allocate space for better performance
    VecReserve(&numbers, 10);
    
    // Insert elements (ownership transfer for l-values)
    int val = 42;
    VecInsertL(&numbers, &val, 0);      // val is now owned by vector
    VecInsertR(&numbers, 10, 0);        // Insert at front
    VecInsertR(&numbers, 30, 1);        // Insert in middle
    
    // Access elements safely
    int first = VecAt(&numbers, 0);                // Get by value
    int* first_ptr = VecPtrAt(&numbers, 0);       // Get by pointer
    int last = VecLast(&numbers);                 // Last element
    
    // Batch operations
    int items[] = {15, 25, 35};
    VecInsertRange(&numbers, items, VecLen(&numbers), 3);
    
    // Sort the vector
    VecSort(&numbers, compare_ints);
    
    // Different iteration patterns
    VecForeachIdx(&numbers, val, idx, {
        printf("[%zu] = %d\n", idx, val);
    });
    
    // Modify elements in-place
    VecForeachPtr(&numbers, ptr, {
        *ptr *= 2;
    });
    
    // Memory management
    VecTryReduceSpace(&numbers);  // Optimize memory usage
    size_t size = VecSize(&numbers);  // Size in bytes
    
    // Batch removal
    VecDeleteRange(&numbers, 1, 2);  // Remove 2 elements starting at index 1
    
    // Clear all elements but keep capacity
    VecClear(&numbers);
    
    // Final cleanup
    VecDeinit(&numbers);
}
```

### Vector with Complex Types
```c
#include <Misra.h>

// Simple type (no pointers/resources)
typedef struct {
    int x;
    int y;
} Point;

// Complex type with owned resources
typedef struct {
    int id;
    Vec(int) data;
} ComplexType;

// Copy initialization for deep copying
bool ComplexTypeCopyInit(ComplexType* dst, const ComplexType* src) {
    dst->id = src->id;
    dst->data = VecInit();
    
    // Copy all elements from source vector
    VecForeachIdx(&src->data, val, idx, {
        VecInsertR(&dst->data, val, idx);
    });
    return true;
}

// Proper cleanup of owned resources
void ComplexTypeDeinit(ComplexType* ct) {
    VecDeinit(&ct->data);
}

int main() {
    // 1. Vector of simple types (no init/deinit needed)
    Vec(Point) points = VecInit();  // No copy_init or copy_deinit
    
    Point p1 = {1, 2};
    Point p2 = {3, 4};
    VecInsertL(&points, &p1, 0);
    VecInsertL(&points, &p2, 1);
    
    // Simple deletion (no cleanup needed)
    VecDelete(&points, 0);  // Safe to just delete
    
    // 2. Vector of complex types with resource management
    Vec(ComplexType) objects = VecInitWithDeepCopy(ComplexTypeCopyInit, ComplexTypeDeinit);
    
    // Create and insert items
    ComplexType item = {
        .id = 1,
        .data = VecInit()
    };
    VecInsertR(&item.data, 42, 0);
    VecInsertR(&item.data, 43, 1);
    
    // Insert with ownership transfer
    VecInsertL(&objects, &item, 0);  // item is now owned by vector
    
    // Two ways to handle removal:
    
    // Method 1: Remove and manually cleanup
    ComplexType removed;
    VecRemove(&objects, &removed, 0);
    ComplexTypeDeinit(&removed);
    
    // Method 2: Direct deletion (vector handles cleanup)
    // Since we provided ComplexTypeDeinit during initialization,
    // the vector will automatically call it when deleting items
    VecDelete(&objects, 0);  // ComplexTypeDeinit is called automatically
    
    // Cleanup
    VecDeinit(&points);      // Simple cleanup, no per-element deinit
    VecDeinit(&objects);     // Calls ComplexTypeDeinit for each remaining element
}
```

### String Operations (Str)
```c
#include <Misra.h>

int main() {
    // Strings are just Vec(char) with special operations
    Str text = StrInit();
    
    // String creation
    Str hello = StrInitFromZstr("Hello");
    Str world = StrInitFromCstr(", World!", 8);
    
    // Formatted append
    StrAppendf(&text, "%.*s%.*s\n", 
               (int)hello.length, hello.data,
               (int)world.length, world.data);
    
    // String operations
    bool starts = StrStartsWithZstr(&text, "Hello");
    bool ends = StrEndsWithZstr(&text, "!\n");
    
    // Split into vector of strings
    Str csv = StrInitFromZstr("one,two,three");
    Strs parts = StrSplit(&csv, ",");
    
    // Process split results
    VecForeachPtr(&parts, str, {
        printf("Part: %.*s\n", (int)str->length, str->data);
    });
    
    // Cleanup
    StrDeinit(&text);
    StrDeinit(&hello);
    StrDeinit(&world);
    StrDeinit(&csv);
    
    // Cleanup split results
    VecForeachPtr(&parts, str, {
        StrDeinit(str);
    });
    VecDeinit(&parts);
}
```

### Formatted I/O
```c
#include <Misra.h>

int main() {
    // String formatting
    Str output = StrInit();
    
    // Basic formatting with direct values
    int count = 42;
    const char* name = "Test";
    StrWriteFmt(&output, "Count: {}, Name: {}\n", 
                FMT(count), FMT(name));  // Pass values directly, not pointers
    
    // Format with alignment and hex
    u32 hex_val = 0xDEADBEEF;
    StrWriteFmt(&output, "Hex: {:#X}\n", FMT(hex_val));
    
    // Read formatted input
    const char* input = "Count: 42, Name: Test";
    int read_count;
    Str read_name = StrInit();
    
    // For reading, we pass the variables directly
    StrReadFmt(input, "Count: {}, Name: {}", 
               FMT(read_count), FMT(read_name));  // No & operator needed
    
    // Multiple value types
    float pi = 3.14159f;
    u64 big_num = 123456789ULL;
    StrWriteFmt(&output, 
                "Float: {:.2f}, Integer: {}, Hex: {:#x}\n",
                FMT(pi), FMT(big_num), FMT(big_num));
    
    // String formatting
    Str hello = StrInitFromZstr("Hello");
    StrWriteFmt(&output, "String: {}\n", FMT(hello));  // Pass Str directly
    
    // Cleanup
    StrDeinit(&output);
    StrDeinit(&read_name);
    StrDeinit(&hello);
}
```

### JSON Parsing and Writing
```c
#include <Misra.h>

// Define our data structures
typedef struct Point {
    float x;
    float y;
} Point;

typedef struct Shape {
    Str name;
    Point position;
    Vec(Point) vertices;
    bool filled;
} Shape;

int main() {
    // Example JSON string
    Str json = StrInitFromZstr(
        "{"
        "  \"name\": \"polygon\","
        "  \"position\": {\"x\": 10.5, \"y\": 20.0},"
        "  \"vertices\": ["
        "    {\"x\": 0.0, \"y\": 0.0},"
        "    {\"x\": 10.0, \"y\": 0.0},"
        "    {\"x\": 5.0, \"y\": 10.0}"
        "  ],"
        "  \"filled\": true"
        "}"
    );

    // Create our shape object
    Shape shape = {
        .name = StrInit(),
        .vertices = VecInit()
    };

    // Parse JSON into our structure
    StrIter si = StrIterFromStr(&json);
    JR_OBJ(si, {
        // Read string value with key "name"
        JR_STR_KV(si, "name", shape.name);
        
        // Read nested object with key "position"
        JR_OBJ_KV(si, "position", {
            JR_FLT_KV(si, "x", shape.position.x);
            JR_FLT_KV(si, "y", shape.position.y);
        });
        
        // Read array of objects with key "vertices"
        JR_ARR_KV(si, "vertices", {
            Point vertex = {0};
            JR_OBJ(si, {
                JR_FLT_KV(si, "x", vertex.x);
                JR_FLT_KV(si, "y", vertex.y);
            });
            VecInsertR(&shape.vertices, vertex, VecLen(&shape.vertices));
        });
        
        // Read boolean value with key "filled"
        JR_BOOL_KV(si, "filled", shape.filled);
    });

    // Modify some values
    shape.position.x += 5.0;
    VecForeachPtr(&shape.vertices, vertex, {
        vertex->y += 1.0;  // Move all points up by 1
    });

    // Write back to JSON
    StrClear(&json);  // Clear existing content
    JW_OBJ(json, {
        // Write string key-value
        JW_STR_KV(json, "name", shape.name);
        
        // Write nested object
        JW_OBJ_KV(json, "position", {
            JW_FLT_KV(json, "x", shape.position.x);
            JW_FLT_KV(json, "y", shape.position.y);
        });
        
        // Write array of objects
        JW_ARR_KV(json, "vertices", shape.vertices, vertex, {
            JW_OBJ(json, {
                JW_FLT_KV(json, "x", vertex.x);
                JW_FLT_KV(json, "y", vertex.y);
            });
        });
        
        // Write boolean value
        JW_BOOL_KV(json, "filled", shape.filled);
    });

    // Print the resulting JSON
    printf("Modified JSON: %.*s\n", (int)json.length, json.data);

    // Cleanup
    StrDeinit(&shape.name);
    VecDeinit(&shape.vertices);
    StrDeinit(&json);
}
```

The example demonstrates:
1. Reading complex nested JSON structures
2. Writing JSON with proper formatting
3. Working with arrays and objects
4. Type-safe value handling
5. Proper memory management

## License

All files in this repo that are copyrighted by me are available under Apache 2.0 License if you're
using this for non-commercial usage and under GPLv3 if you're using this for any commercial use case.
I also reserve the right to make the licensing of copyrighted files less restrictive for any entity
I wish to do it for. This means if you're a commercial entity and if you have my explicit permission
you can use it under a license no more restrictive than GPLv3.

I intend to keep this library as open source and accessible as possible.

### Apache 2.0 (For Non-Commercial Use Case)

```
Copyright 2025 Siddharth Mishra

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
```

### GNU GPL 3.0 (For Commercial Use Case)

```
Copyright (C) 2025  Siddharth Mishra

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
```
