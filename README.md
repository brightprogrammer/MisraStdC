# MisraStdC

[![Linux Build](https://github.com/brightprogrammer/MisraStdC/actions/workflows/test-linux.yml/badge.svg?branch=master)](https://github.com/brightprogrammer/MisraStdC/actions/workflows/test-linux.yml)
[![macOS Build](https://github.com/brightprogrammer/MisraStdC/actions/workflows/test-macos.yml/badge.svg?branch=master)](https://github.com/brightprogrammer/MisraStdC/actions/workflows/test-macos.yml)
[![Windows Build](https://github.com/brightprogrammer/MisraStdC/actions/workflows/test-windows-msvc.yml/badge.svg?branch=master)](https://github.com/brightprogrammer/MisraStdC/actions/workflows/test-windows-msvc.yml)

A modern C11 library designed to make programming in C less painful and more productive, written in pure C. MisraStdC provides generic containers, string handling, and formatted I/O inspired by higher-level languages while maintaining C's performance and control.

> **Disclaimer:** This library is **not** related to the MISRA C standard or guidelines. The name "MisraStdC" comes from the author's name, Siddharth Mishra, who is commonly known as "Misra" among friends.

## Table of Contents

- [Features](#features)
- [Requirements](#requirements)
- [Installation](#installation)
- [Documentation](#documentation)
- [Concenpts](#concepts)
- [Examples](#examples)
  - [Vector Container (Vec)](#vector-container-vec)
  - [String Operations (Str)](#string-operations-str)
  - [Formatted I/O](#formatted-io)
  - [JSON Parsing and Writing](#json-parsing-and-writing)
  - [Working with Complex Types](#working-with-complex-types)
  - [Parent Child Process](#parent-child-process)
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

## Concepts

### Initialization

If an object type provides an `Init()` method or macro, then that must necessarily be used. Some objects
employ tricks to detect whether object is initialized properly or is corrupted at runtime. These checks
are performed everytime during a function call. While this adds a bit of overhead to the function calls,
it does make sure that everything's working as expected. There's no computation involved, and just a few
comparision checks.

Similar to initialization, all objects must be deinitialized at the end of their life cycle.

### Copy/Move Semantics

There are two types of insertion methods into a container.

- Insertion of l-value
- Insertion of r-value

While the naming is a bit ambiguous, this is what I came up at the time of need. By default
all unmarked functions/macros follow l-value semantics.

#### L-Value Insertion

Functions/macros marked with ___L___ suffix follow this behavior. Functions/macros marked with ___L___
will make sure that there will always exist only one copy of data being inserted. If the container you're
inserting an item to, makes it's own copy of items, the inserted l-value remains as it is, because
unique ownership is maintained. If however the container does not create it's own copies, because `copy_init`
method is not set, then it'll take ownership by calling `memset(lval, 0, sizeof(lval))` on given l-value `lval`.

This is to explicitly state that the object must always have single ownership.

#### R-Value Insertion

Unlike l-value insertion, here the functions/macros don't care about who owns what. You just insert
and forget about ownership. There can be multiple owners, there can be a single one, we believe the user
knows what they're doing.

#### Example use case(s)

This strict set of functions/macros to declare ownership transfers in code is to
better annotate the transfer locations. Usually these are not very clear when reading code.
One example use case of l-value semantics is when you're creating objects in a for-loop in
a temporary variable (for eg: receivng from a stream) and then inserting those directly into a
container for storage.

NOTE: The container will take ownership only if no `copy_init` is set!!

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
        WriteFmtLn("[{}] = {}\n", idx, val);
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
    VecForeach(&parts, str, {
        WriteFmtLn("Part: {}\n", str);
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
    StrWriteFmt(&output, "Count: {}, Name: {}\n", count, name);  // Pass values directly, not pointers
    
    // Format with alignment and hex
    u32 hex_val = 0xDEADBEEF;
    StrWriteFmt(&output, "Hex: {X}\n", hex_val);
    
    // Read formatted input
    const char* input = "Count: 42, Name: Test";
    int read_count;
    Str read_name = StrInit();
    
    // For reading, we pass the variables directly
    StrReadFmt(input, "Count: {}, Name: {}", read_count, read_name);  // No & operator needed
    
    // Multiple value types
    float pi = 3.14159f;
    u64 big_num = 123456789ULL;
    StrWriteFmt(&output, "Float: {.2f}, Integer: {}, Hex: {x}\n", pi, big_num, big_num);
    
    // String formatting
    Str hello = StrInitFromZstr("Hello");
    StrWriteFmt(&output, "String: {}\n", hello);  // Pass Str directly
    
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
    WriteFmtLn("Modified JSON: {}", json);

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

### Parent Child Process

The library also provides a way to create child processes in a cross-platform manner. I've
also added a method to write to `stdin` and read from `stdout` and `stderr` for each child process.
Refer to the following example, also present in `Bin/SubProcComm.c`.

```c
#include <Misra.h>

// this program was verifed to work when executed with /bin/head
// the prgram writes something to child process and expect's the same thing echoed back
// so it can be verified that we got the same content
// executed like : Build/SubProcComm /bin/head -n 1
int main(int argc, char** argv, char** envp) {
    // create a new child process
    SysProc* proc = SysProcCreate(argv[1], argv + 1, envp);

    // write something to it's stdout
    SysProcWriteToStdinFmtLn(proc, "value = {}", 42);

    // retrieve back the value
    i32 val = 0;
    SysProcReadFromStdoutFmt(proc, "value = {}", val);

    // write the retrieved value to stdout (parent, not child)
    WriteFmtLn("got value = {}", val);

    // wait for program to exit for 1 second
    SysProcWaitFor(proc, 1000);

    // finally terminate
    SysProcDestroy(proc);
}
```

## Format Specifiers

The library supports Rust-style format strings with placeholders in the form `{}` or `{pecifier}`.

### Important: Understanding Supported Argument Format

The macro-tricks use `_Generic` for compile-time type specific io dispatching. Here's what works and what doesn't:

#### ❌ What Doesn't Work

```c
// String literals (array types like char[6] not handled by _Generic)
StrWriteFmt(&output, "Hello, {}!", "world");  // ERROR: char[6] not in _Generic cases

// Any char array types are not handled
char buffer[20] = "Hello";                    // Type: char[20] 
StrWriteFmt(&output, "Message: {}", buffer);  // ERROR: char[20] not in _Generic cases

const char name[] = "Alice";                  // Type: const char[6]
StrWriteFmt(&output, "Name: {}", name);       // ERROR: const char[6] not in _Generic cases
```

#### ✅ What Works Perfectly

```c
// const char* variables (pointer type matches _Generic case)
const char* title = "Mr.";
const char* surname = "Smith";
StrWriteFmt(&output, "{} {}", title, surname);  // ✅ Works great!

// char* variables (pointer type matches _Generic case)
char* dynamic_str = malloc(50);
strcpy(dynamic_str, "Dynamic");
StrWriteFmt(&output, "Value: {}", dynamic_str);      // ✅ Works perfectly!

// Str objects (library's string type)
Str greeting = StrInitFromZstr("Welcome");
StrWriteFmt(&output, "Message: {}", greeting);

// Primitive types (all handled by _Generic)
int number = 42;
float pi = 3.14f;
StrWriteFmt(&output, "Number: {}, Pi: {.2}", number, pi);
```

#### 💡 Best Practices

```c
// For constant strings, use const char* pointers:
const char* program_name = "MyApp";      // ✅ const char* works perfectly!
const char* version = "1.0.0";           // ✅ char* pointer types work!

// For dynamic strings, use Str objects or char* pointers:
Str user_input = StrInit();
StrReadFmt(input_line, "Name: {}", user_input);

char* allocated = malloc(100);
strcpy(allocated, "Dynamic content");
StrWriteFmt(&message, "Content: {}", allocated);     // ✅ char* works!

// For function parameters accepting strings:
void log_message(const char* msg) {                      // ✅ const char* parameter
    StrWriteFmt(&log_output, "[LOG] {}", msg);       // ✅ Works perfectly!
}

void process_buffer(char* buffer) {                      // ✅ char* parameter  
    StrWriteFmt(&output, "Processing: {}", buffer);  // ✅ Works great!
}
```

#### 🔧 Technical Explanation

The macro-tricks use `_Generic` which only handles these specific types:
- `const char*` ✅
- `char*` ✅  
- `Str` ✅
- Primitive types (`int`, `float`, `u32`, etc.) ✅

But **NOT** array types like:
- `char[6]` (from `"hello"`) ❌
- `char[20]` (from `char buffer[20]`) ❌
- `const char[10]` (from `const char arr[] = "test"`) ❌

The compiler knows these array types perfectly, but `_Generic` doesn't have cases for every possible array size.

### Basic Usage

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

This is also used to control the endianness of raw data read or written.

#### Width

Specifies the minimum field width. The value is padded with spaces if it's shorter than this width:

```c
{}    // Minimum width of 5 characters, right-aligned
{<5}   // Minimum width of 5 characters, left-aligned
{^5}   // Minimum width of 5 characters, center-aligned
```

### Endianness

The endianness specified is used to convert the read data to native endian after reading
in specified endianness format.

| Specifier | Description |
|-----------|-------------|
| `<` | Little Endian (Least significant byte first) |
| `>` | Big Endian (Most significant byte first, default) |
| `^` | Native Endian (Same as host endianness) |

Much like how alignment is specified, width of data read can also be specified in bytes.

```c
{4}    // Read/Write 4 bytes in big endian order.
{>4}   // Read/Write 4 bytes in big endian order.
{<2}   // Read/Write 2 bytes in little endian order.
{^8}   // Read/Write 8 bytes in native endian
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
| `r` | Raw data reading or writing | `\x7fELF` (magic bytes of an elf file) |
| `e` | Scientific notation (lowercase) | `1.235e+02` |
| `E` | Scientific notation (uppercase) | `1.235E+02` |
| `s` | Read a string in single quotes or double quotes, or a single word | `"this is a string"`, `'this as well'`, `this not a string` |

#### Precision

For floating-point values, specifies the number of decimal places:

```c
{.2}   // Two decimal places
{.0}   // No decimal places
{.10}  // Ten decimal places
```

Precision is ignored if specified when reading/writing raw data.

### Format Examples

#### Basic Formatting

```c
// Correct usage with const char*
const char* greeting = "Hello";
const char* subject = "world";
StrWriteFmt(&output, "{}, {}!", greeting, subject);  // "Hello, world!"

// Escaped braces
StrWriteFmt(&output, "{{Hello}}");  // "{Hello}"
```

#### String Formatting

```c
const char* str = "Hello";  // const char* variable

// Basic string
StrWriteFmt(&output, "{}", str);  // "Hello"

// String with width and alignment
StrWriteFmt(&output, "{>10}", str);  // "     Hello"
StrWriteFmt(&output, "{<10}", str);  // "Hello     "
StrWriteFmt(&output, "{^10}", str);  // "  Hello   "
```

#### Integer Formatting

```c
i32 val = 42;

// Default decimal
StrWriteFmt(&output, "{}", val);  // "42"

// Hexadecimal
u32 hex_val = 0xDEADBEEF;
StrWriteFmt(&output, "{}", hex_val);  // "0xdeadbeef"
StrWriteFmt(&output, "{}", hex_val);  // "0xDEADBEEF"

// Binary
u8 bin_val = 0xA5;  // 10100101 in binary
StrWriteFmt(&output, "{}", bin_val);  // "0b10100101"

// Octal
u16 oct_val = 0777;
StrWriteFmt(&output, "{}", oct_val);  // "0o777"

// Width and alignment with numbers
StrWriteFmt(&output, "{}", val);   // "   42" (right-aligned)
StrWriteFmt(&output, "{<5}", val);  // "42   " (left-aligned)
StrWriteFmt(&output, "{^5}", val);  // " 42  " (center-aligned)
```

#### Character Formatting

The character format specifiers (`c`, `a`, `A`) work with integer types, treating them as character data:

```c
// Single character (u8)
u8 upper_char = 'M';
u8 lower_char = 'm';

StrWriteFmt(&output, "{}", upper_char);  // "M" (preserve case)
StrWriteFmt(&output, "{}", upper_char);  // "m" (force lowercase)
StrWriteFmt(&output, "{}", lower_char);  // "M" (force uppercase)

// Multi-byte integers (interpreted as character sequences)
u16 u16_value = ('A' << 8) | 'B'; // "AB" in big-endian
StrWriteFmt(&output, "{}", u16_value);  // "AB" (preserve case)
StrWriteFmt(&output, "{}", u16_value);  // "ab" (force lowercase)
StrWriteFmt(&output, "{}", u16_value);  // "AB" (force uppercase)

// Works with u32 and u64 as well, treating them as byte sequences
u32 u32_value = ('H' << 24) | ('i' << 16) | ('!' << 8) | '!';
StrWriteFmt(&output, "{}", u32_value);  // "Hi!!" (preserve case)
StrWriteFmt(&output, "{}", u32_value);  // "hi!!" (force lowercase)
StrWriteFmt(&output, "{}", u32_value);  // "HI!!" (force uppercase)
```

#### String Case Formatting

Character format specifiers also work with strings:

```c
const char* mixed_case = "MiXeD CaSe";

StrWriteFmt(&output, "{}", mixed_case);  // "MiXeD CaSe" (preserve case)
StrWriteFmt(&output, "{}", mixed_case);  // "mixed case" (force lowercase)
StrWriteFmt(&output, "{}", mixed_case);  // "MIXED CASE" (force uppercase)

// Also works with Str objects
Str s = StrInitFromZstr("Hello World");
StrWriteFmt(&output, "{}", s);  // "hello world"
StrWriteFmt(&output, "{}", s);  // "HELLO WORLD"
```

#### Floating-Point Formatting

```c
f64 pi = 3.14159265359;

// Default precision (6 decimal places)
StrWriteFmt(&output, "{}", pi);  // "3.141593"

// Custom precision
StrWriteFmt(&output, "{.2}", pi);   // "3.14"
StrWriteFmt(&output, "{.0}", pi);   // "3"
StrWriteFmt(&output, "{.10}", pi);  // "3.1415926536"

// Scientific notation
StrWriteFmt(&output, "{}", 123.456);  // "1.235e+02"
StrWriteFmt(&output, "{}", 123.456);  // "1.235E+02"

// Custom precision with scientific notation
StrWriteFmt(&output, "{.3e}", 123.456);  // "1.235e+02"

// Special values
f64 pos_inf = INFINITY;
f64 neg_inf = -INFINITY;
f64 nan_val = NAN;
StrWriteFmt(&output, "{}", pos_inf);  // "inf"
StrWriteFmt(&output, "{}", neg_inf);  // "-inf"
StrWriteFmt(&output, "{}", nan_val);  // "nan"
```

### Reading Values

The library also supports parsing values from strings using the same format specifier syntax:

```c
// Reading integers
i32 num = 0;
StrReadFmt("42", "{}", num);  // num = 42

// Reading hexadecimal (auto-detected with 0x prefix)
u32 hex_val = 0;
StrReadFmt("0xdeadbeef", "{}", hex_val);  // hex_val = 0xdeadbeef

// Reading binary (auto-detected with 0b prefix)
i8 bin_val = 0;
StrReadFmt("0b101010", "{}", bin_val);  // bin_val = 42

// Reading octal (auto-detected with 0o prefix)
i32 oct_val = 0;
StrReadFmt("0o755", "{}", oct_val);  // oct_val = 493

// Reading floating point
f64 value = 0.0;
StrReadFmt("3.14159", "{}", value);  // value = 3.14159

// Reading scientific notation
StrReadFmt("1.23e4", "{}", value);  // value = 12300.0

// Reading strings
Str name = StrInit();
StrReadFmt("Alice", "{}", name);  // name = "Alice"

// Reading quoted strings
StrReadFmt("\"Hello, World!\"", "{}", name);  // name = "Hello, World!"

// Reading multiple values
i32 count = 0;
Str user = StrInit();
StrReadFmt("Count: 42, Name: Alice", "Count: {}, Name: {}", count, user);
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


