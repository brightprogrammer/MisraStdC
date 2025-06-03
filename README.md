# MisraStdC

[![Linux Build](https://github.com/brightprogrammer/MisraStdC/actions/workflows/test-linux.yml/badge.svg?branch=master)](https://github.com/brightprogrammer/MisraStdC/actions/workflows/test-linux.yml)
[![macOS Build](https://github.com/brightprogrammer/MisraStdC/actions/workflows/test-macos.yml/badge.svg?branch=master)](https://github.com/brightprogrammer/MisraStdC/actions/workflows/test-macos.yml)
[![Windows Build](https://github.com/brightprogrammer/MisraStdC/actions/workflows/test-windows.yml/badge.svg?branch=master)](https://github.com/brightprogrammer/MisraStdC/actions/workflows/test-windows.yml)

A modern C11 library designed to make programming in C less painful and more productive, written in pure C. MisraStdC provides generic containers, string handling, and formatted I/O inspired by higher-level languages while maintaining C's performance and control.

> **Disclaimer:** This library is **not** related to the MISRA C standard or guidelines. The name "MisraStdC" comes from the author's name, Siddharth Mishra, who is commonly known as "Misra" among friends.

## Table of Contents

- [Features](#features)
- [Requirements](#requirements)
- [Installation](#installation)
- [Documentation](#documentation)
- [Examples](#examples)
  - [Vector Container (Vec)](#vector-container-vec)
  - [String Operations (Str)](#string-operations-str)
  - [Formatted I/O](#formatted-io)
  - [JSON Parsing and Writing](#json-parsing-and-writing)
  - [Working with Complex Types](#working-with-complex-types)
- [Format Specifiers](#format-specifiers)
- [Contributing](#contributing)
- [License](#license)

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

## Requirements

- **C11 compatible compiler**:
  - **GCC**: Version 4.9 or newer
  - **Clang**: Version 3.4 or newer
  - **MSVC**: Visual Studio 2019 or newer
  
- **Build System**:
  - [Meson](https://mesonbuild.com/) build system (version 0.60.0 or newer)
  - [Ninja](https://ninja-build.org/) build tool

## Installation

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

## Format Specifiers

The library supports Rust-style format strings with placeholders in the form `{}` or `{:specifier}`. Arguments are passed using the `FMT()` macro, which automatically determines the correct type handling.

### Basic Usage

```c
Str output = StrInit();

// Basic placeholder
StrWriteFmt(&output, "Hello, {}!", FMT("world"));  // "Hello, world!"

// Multiple placeholders
int count = 42;
const char* name = "Alice";
StrWriteFmt(&output, "Count: {}, Name: {}", FMT(count), FMT(name));
```

If no specifier is provided (just `{}`), default formatting is used.

### Format Specifier Options

Format specifiers can include the following components, which can be combined:

#### Alignment

Controls text alignment within a field width:

| Specifier | Description |
|-----------|-------------|
| `<` | Left-aligned (pad on the right) |
| `>` | Right-aligned (pad on the left) - default |
| `^` | Center-aligned (pad on both sides) |

#### Width

Specifies the minimum field width. The value is padded with spaces if it's shorter than this width:

```c
{:5}    // Minimum width of 5 characters, right-aligned
{:<5}   // Minimum width of 5 characters, left-aligned
{:^5}   // Minimum width of 5 characters, center-aligned
```

#### Type Specifiers

Specifies the output format for the value:

| Specifier | Description | Example Output |
|-----------|-------------|----------------|
| `x` | Hexadecimal format (lowercase) | `0xdeadbeef` |
| `X` | Hexadecimal format (uppercase) | `0xDEADBEEF` |
| `b` | Binary format | `0b10100101` |
| `o` | Octal format | `0o777` |
| `c` | Character format (preserve case) | Raw character bytes |
| `a` | Character format (force lowercase) | Converts characters to lowercase |
| `A` | Character format (force uppercase) | Converts characters to uppercase |
| `e` | Scientific notation (lowercase) | `1.235e+02` |
| `E` | Scientific notation (uppercase) | `1.235E+02` |

#### Precision

For floating-point values, specifies the number of decimal places:

```c
{:.2}   // Two decimal places
{:.0}   // No decimal places
{:.10}  // Ten decimal places
```

### Format Examples

#### Basic Formatting

```c
// Simple placeholder
StrWriteFmt(&output, "Hello, {}!", FMT("world"));  // "Hello, world!"

// Escaped braces
StrWriteFmt(&output, "{{Hello}}");  // "{Hello}"
```

#### String Formatting

```c
const char* str = "Hello";

// Basic string
StrWriteFmt(&output, "{}", FMT(str));  // "Hello"

// String with width and alignment
StrWriteFmt(&output, "{:>10}", FMT(str));  // "     Hello"
StrWriteFmt(&output, "{:<10}", FMT(str));  // "Hello     "
StrWriteFmt(&output, "{:^10}", FMT(str));  // "  Hello   "
```

#### Integer Formatting

```c
i32 val = 42;

// Default decimal
StrWriteFmt(&output, "{}", FMT(val));  // "42"

// Hexadecimal
u32 hex_val = 0xDEADBEEF;
StrWriteFmt(&output, "{:x}", FMT(hex_val));  // "0xdeadbeef"
StrWriteFmt(&output, "{:X}", FMT(hex_val));  // "0xDEADBEEF"

// Binary
u8 bin_val = 0xA5;  // 10100101 in binary
StrWriteFmt(&output, "{:b}", FMT(bin_val));  // "0b10100101"

// Octal
u16 oct_val = 0777;
StrWriteFmt(&output, "{:o}", FMT(oct_val));  // "0o777"

// Width and alignment with numbers
StrWriteFmt(&output, "{:5}", FMT(val));   // "   42" (right-aligned)
StrWriteFmt(&output, "{:<5}", FMT(val));  // "42   " (left-aligned)
StrWriteFmt(&output, "{:^5}", FMT(val));  // " 42  " (center-aligned)
```

#### Character Formatting

The character format specifiers (`c`, `a`, `A`) work with integer types, treating them as character data:

```c
// Single character (u8)
u8 upper_char = 'M';
u8 lower_char = 'm';

StrWriteFmt(&output, "{:c}", FMT(upper_char));  // "M" (preserve case)
StrWriteFmt(&output, "{:a}", FMT(upper_char));  // "m" (force lowercase)
StrWriteFmt(&output, "{:A}", FMT(lower_char));  // "M" (force uppercase)

// Multi-byte integers (interpreted as character sequences)
u16 u16_value = ('A' << 8) | 'B'; // "AB" in big-endian
StrWriteFmt(&output, "{:c}", FMT(u16_value));  // "AB" (preserve case)
StrWriteFmt(&output, "{:a}", FMT(u16_value));  // "ab" (force lowercase)
StrWriteFmt(&output, "{:A}", FMT(u16_value));  // "AB" (force uppercase)

// Works with u32 and u64 as well, treating them as byte sequences
u32 u32_value = ('H' << 24) | ('i' << 16) | ('!' << 8) | '!';
StrWriteFmt(&output, "{:c}", FMT(u32_value));  // "Hi!!" (preserve case)
StrWriteFmt(&output, "{:a}", FMT(u32_value));  // "hi!!" (force lowercase)
StrWriteFmt(&output, "{:A}", FMT(u32_value));  // "HI!!" (force uppercase)
```

#### String Case Formatting

Character format specifiers also work with strings:

```c
const char* mixed_case = "MiXeD CaSe";

StrWriteFmt(&output, "{:c}", FMT(mixed_case));  // "MiXeD CaSe" (preserve case)
StrWriteFmt(&output, "{:a}", FMT(mixed_case));  // "mixed case" (force lowercase)
StrWriteFmt(&output, "{:A}", FMT(mixed_case));  // "MIXED CASE" (force uppercase)

// Also works with Str objects
Str s = StrInitFromZstr("Hello World");
StrWriteFmt(&output, "{:a}", FMT(s));  // "hello world"
StrWriteFmt(&output, "{:A}", FMT(s));  // "HELLO WORLD"
```

#### Floating-Point Formatting

```c
f64 pi = 3.14159265359;

// Default precision (6 decimal places)
StrWriteFmt(&output, "{}", FMT(pi));  // "3.141593"

// Custom precision
StrWriteFmt(&output, "{:.2}", FMT(pi));   // "3.14"
StrWriteFmt(&output, "{:.0}", FMT(pi));   // "3"
StrWriteFmt(&output, "{:.10}", FMT(pi));  // "3.1415926536"

// Scientific notation
StrWriteFmt(&output, "{:e}", FMT(123.456));  // "1.235e+02"
StrWriteFmt(&output, "{:E}", FMT(123.456));  // "1.235E+02"

// Custom precision with scientific notation
StrWriteFmt(&output, "{:.3e}", FMT(123.456));  // "1.235e+02"

// Special values
f64 pos_inf = INFINITY;
f64 neg_inf = -INFINITY;
f64 nan_val = NAN;
StrWriteFmt(&output, "{}", FMT(pos_inf));  // "inf"
StrWriteFmt(&output, "{}", FMT(neg_inf));  // "-inf"
StrWriteFmt(&output, "{}", FMT(nan_val));  // "nan"
```

### Reading Values

The library also supports parsing values from strings using the same format specifier syntax:

```c
// Reading integers
i32 num = 0;
StrReadFmt("42", "{}", FMT(num));  // num = 42

// Reading hexadecimal (auto-detected with 0x prefix)
u32 hex_val = 0;
StrReadFmt("0xdeadbeef", "{}", FMT(hex_val));  // hex_val = 0xdeadbeef

// Reading binary (auto-detected with 0b prefix)
i8 bin_val = 0;
StrReadFmt("0b101010", "{}", FMT(bin_val));  // bin_val = 42

// Reading octal (auto-detected with 0o prefix)
i32 oct_val = 0;
StrReadFmt("0o755", "{}", FMT(oct_val));  // oct_val = 493

// Reading floating point
f64 value = 0.0;
StrReadFmt("3.14159", "{}", FMT(value));  // value = 3.14159

// Reading scientific notation
StrReadFmt("1.23e4", "{}", FMT(value));  // value = 12300.0

// Reading strings
Str name = StrInit();
StrReadFmt("Alice", "{}", FMT(name));  // name = "Alice"

// Reading quoted strings
StrReadFmt("\"Hello, World!\"", "{}", FMT(name));  // name = "Hello, World!"

// Reading multiple values
i32 count = 0;
Str user = StrInit();
StrReadFmt("Count: 42, Name: Alice", "Count: {}, Name: {}", FMT(count), FMT(user));
// count = 42, user = "Alice"
```

### Available I/O Functions

The library provides several I/O functions for formatted reading and writing:

- `StrWriteFmt(&str, format, ...)`: Append formatted output to a string
- `StrReadFmt(input, format, ...)`: Parse values from a string
- `FWriteFmt(file, format, ...)`: Write formatted output to a file
- `FWriteFmtLn(file, format, ...)`: Write formatted output to a file with a newline
- `FReadFmt(file, format, ...)`: Read formatted input from a file
- `WriteFmt(format, ...)`: Write formatted output to stdout
- `WriteFmtLn(format, ...)`: Write formatted output to stdout with a newline
- `ReadFmt(format, ...)`: Read formatted input from stdin

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


