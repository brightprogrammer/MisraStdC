# MisraStdC

[![Build and Test](https://github.com/brightprogrammer/MisraStdC/actions/workflows/test.yml/badge.svg)](https://github.com/brightprogrammer/MisraStdC/actions/workflows/test.yml)

A modern, type-safe C library designed to make programming in C less painful and more productive. MisraStdC provides generic containers, string handling, and formatted I/O inspired by higher-level languages while maintaining C's performance and control.

## Features

- **Cross-platform compatibility**: Supports MSVC, GCC, and Clang
- **Type-safe generic containers**:
  - `Vec(T)`: Generic vector with strict type checking
  - `Str`: String handling (specialized `Vec(char)`)
  - `Map(K, V)`: Generic key-value hash-map storage (WIP)
  - `Int`: Custom big integer implementation (WIP)
- **Rust-style formatted I/O**:
  - `WriteFmt`, `ReadFmt`: Type-safe formatted standard I/O
  - `StrWriteFmt`, `StrReadFmt`: Type-safe formatted string operations
- **JSON parsing and serialization**
- **Memory safety** with proper initialization and cleanup functions

## Installation

### Prerequisites

- C compiler (GCC, Clang, or MSVC)
- [Meson](https://mesonbuild.com/) build system
- [Ninja](https://ninja-build.org/) build tool

### Building from Source

```bash
# Clone the repository with submodules
git clone --recursive https://github.com/brightprogrammer/MisraStdC.git
cd MisraStdC

# Configure the build
meson setup builddir

# Build the library
ninja -C builddir

# Run tests
ninja -C builddir test
```

### Build Options

For development with sanitizers (recommended for debugging):

```bash
meson setup builddir -Db_sanitize=address,undefined -Db_lundef=false
```

## Documentation

Comprehensive API documentation is available at [docs.brightprogrammer.in](https://docs.brightprogrammer.in).

## Examples

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

### Working with Complex Types

```c
#include <Misra.h>

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
    // Vector of complex types with resource management
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
    
    // Direct deletion (vector handles cleanup)
    // Since we provided ComplexTypeDeinit during initialization,
    // the vector will automatically call it when deleting items
    VecDelete(&objects, 0);  // ComplexTypeDeinit is called automatically
    
    // Cleanup
    VecDeinit(&objects);     // Calls ComplexTypeDeinit for each remaining element
}
```

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add some amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## License

This project is dedicated to the public domain under the [Unlicense](LICENSE.md).

This means you are free to:
- Use the code for any purpose
- Change the code in any way
- Share the code with anyone
- Distribute the code
- Sell the code or derivative works

No attribution is required. See the [LICENSE.md](LICENSE.md) file for details.
