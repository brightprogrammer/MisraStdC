# MisraStdC

[![Linux Build](https://github.com/brightprogrammer/MisraStdC/actions/workflows/test-linux.yml/badge.svg?branch=master)](https://github.com/brightprogrammer/MisraStdC/actions/workflows/test-linux.yml)
[![macOS Build](https://github.com/brightprogrammer/MisraStdC/actions/workflows/test-macos.yml/badge.svg?branch=master)](https://github.com/brightprogrammer/MisraStdC/actions/workflows/test-macos.yml)
[![Windows Build](https://github.com/brightprogrammer/MisraStdC/actions/workflows/test-windows-msvc.yml/badge.svg?branch=master)](https://github.com/brightprogrammer/MisraStdC/actions/workflows/test-windows-msvc.yml)
[![Fuzzing](https://github.com/brightprogrammer/MisraStdC/actions/workflows/fuzz.yml/badge.svg?branch=master)](https://github.com/brightprogrammer/MisraStdC/actions/workflows/fuzz.yml)

> ~~While this was really fun to do, I realize there will always be some limitations of this library. If you just wanna have some fun with macros, go ahead and try this library, otherwise I think maintaining it is a real PITA. There will always exist a use-case that's just too covoluted to handle. Ciao Ciao!~~ Let's give it another shot!

A modern C11 library designed to make programming in C less painful and more productive, written in pure C. MisraStdC provides generic containers, string handling, and formatted I/O inspired by higher-level languages while maintaining C's performance and control.

> **Disclaimer:** This library is **not** related to the MISRA C standard or guidelines. The name "MisraStdC" comes from the author's name, Siddharth Mishra, who is commonly known as "Misra" among friends.

## Table of Contents

- [Features](#features)
- [Requirements](#requirements)
- [Installation](#installation)
- [Documentation](#documentation)
- [Concepts](#concepts)
- [Examples](#examples)
  - [Vector Container (Vec)](#vector-container-vec)
  - [String Operations (Str)](#string-operations-str)
  - [Directed Graph Container (Graph)](#directed-graph-container-graph)
  - [Formatted I/O](#formatted-io)
  - [JSON Parsing and Writing](#json-parsing-and-writing)
  - [Working with Complex Types](#working-with-complex-types)
  - [Parent Child Process](#parent-child-process)
- [Format Specifiers](#format-specifiers)
- [Contributing](#contributing)
- [License](#license)

## Features

- **Cross-platform compatibility**: Supports MSVC, GCC, and Clang
- **Macro-based generic containers and utilities**:
  - `Vec(T)`: Generic vector backed by shared `GenericVec` runtime helpers
  - `List(T)`: Generic doubly linked list backed by shared `GenericList` runtime helpers
  - `Graph(T)`: Generic directed graph with stable node ids, predecessor/successor traversal, and deferred destructive mutation
  - `Str` / `Strs`: Predefined `typedef`s for `Vec(char)` and `Vec(Str)`
  - `BitVec`: Packed bit container for boolean-style storage
  - `Int`: Arbitrary-precision unsigned integer API backed by `BitVec`, with byte and radix-string conversion, arithmetic, roots, gcd/lcm, primality helpers, and modular arithmetic
  - `Float`: Arbitrary-precision decimal floating-point API backed by `Int`, with string conversion and exact add/sub/mul plus precision-based division
  - `Iter(T)` and `Pair(xT, yT)`: Generic utility macros
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
git clone --recursive https://git.anvielabs.com/bp/MisraStdC.git
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

The prose guides include a dedicated graph guide covering node handles, predecessor traversal, deferred deletion, and analysis-oriented usage patterns.

## Concepts

### How Generic Templating Works

MisraStdC uses the C preprocessor plus a shared runtime layer rather than code generation. The template-style APIs are
macros:

- `Vec(T)`, `List(T)`, `Graph(T)`, `Iter(T)`, and `Pair(xT, yT)` expand to anonymous structs.
- Macros like `VecInsertR`, `VecAt`, `VecDeinit`, and `ListDeinit` infer the element type and `sizeof(...)` at the
  call site, then forward to generic runtime helpers in `Source/`.
- `Str` and `Strs` are ordinary `typedef`s built on top of `Vec(char)` and `Vec(Str)`.
- `BitVec` is a dedicated concrete type, not `Vec(bool)`.
- `Int` is a concrete bigint type with `BitVec` storage inside it, but its public API is intentionally
  integer-oriented: initialization, radix and byte conversion, comparison, arithmetic, roots, power-of-two
  inspection, and modular number-theory helpers are exposed directly, while raw bitvector operations remain on
  `BitVec`.
- `Float` is a concrete decimal floating-point type built on top of `Int`, storing a sign, an integer significand,
  and a base-10 exponent for exact decimal arithmetic.

That model has a few practical consequences:

- Each `Vec(T)` or `List(T)` expansion is a distinct anonymous type. If you need a reusable type across declarations,
  create a `typedef` first.
- `VecInsertL` and similar `...L` forms expect an l-value expression, not a pointer. They can transfer ownership by
  zeroing the source object when no deep-copy callback is configured.
- If a nested macro type contains commas, wrap it in `T(...)` so the outer macro sees it as a single argument.

```c
typedef Vec(int) IntVec;
typedef Vec(T(Pair(i32, Str))) PairVec;
typedef List(Str) StrList;
```

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
method is not set, then it'll take ownership by zeroing the source l-value after insertion (effectively
`memset(&lval, 0, sizeof(lval))`).

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

typedef Vec(int) IntVec;

int compare_ints(const void* a, const void* b) {
    return *(const int*)a - *(const int*)b;
}

int main(void) {
    // Initialize a reusable vector typedef
    IntVec numbers = VecInit();
    
    // Pre-allocate space for better performance
    VecReserve(&numbers, 10);
    
    // Insert elements (ownership transfer for l-values)
    int val = 42;
    VecInsertL(&numbers, val, 0);       // val is now owned by vector
    VecInsertR(&numbers, 10, 0);        // Insert at front
    VecInsertR(&numbers, 30, 1);        // Insert in middle
    
    // Access elements safely
    int first = VecAt(&numbers, 0);                // Get by value
    int* first_ptr = VecPtrAt(&numbers, 0);       // Get by pointer
    int last = VecLast(&numbers);                 // Last element
    
    // Batch operations
    int items[] = {15, 25, 35};
    VecInsertRangeR(&numbers, items, VecLen(&numbers), 3);
    
    // Sort the vector
    VecSort(&numbers, compare_ints);
    
    // Different iteration patterns
    VecForeachIdx(&numbers, current, idx) {
        WriteFmtLn("[{}] = {}", idx, current);
    }
    
    // Modify elements in-place
    VecForeachPtr(&numbers, current) {
        *current *= 2;
    }
    
    // Memory management
    VecTryReduceSpace(&numbers);  // Optimize memory usage
    u64 size = VecSize(&numbers);  // Size in bytes
    
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

int main(void) {
    // Str is a typedef specialization of Vec(char)
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
    VecForeach(&parts, part) {
        WriteFmtLn("Part: {}", part);
    }
    
    // Cleanup
    StrDeinit(&text);
    StrDeinit(&hello);
    StrDeinit(&world);
    StrDeinit(&csv);
    
    // Cleanup split results
    VecForeachPtr(&parts, part) {
        StrDeinit(part);
    }
    VecDeinit(&parts);
}
```

### Directed Graph Container (Graph)

```c
#include <Misra.h>

typedef Graph(Str) NameGraph;

int main(void) {
    NameGraph graph = GraphInitWithDeepCopy(NULL, StrDeinit);

    GraphNodeId alpha = GraphAddNodeR(&graph, StrZ("Alpha"));
    GraphNodeId beta  = GraphAddNodeR(&graph, StrZ("Beta"));
    GraphNodeId gamma = GraphAddNodeR(&graph, StrZ("Gamma"));

    GraphAddEdge(&graph, alpha, beta);
    GraphAddEdge(&graph, beta, gamma);

    GraphForeachNode(&graph, node) {
        WriteFmtLn("{}: out={}, in={}",
                   GraphNodeData(&graph, node),
                   GraphOutDegree(&graph, GraphNodeGetId(node)),
                   GraphInDegree(&graph, GraphNodeGetId(node)));
    }

    GraphDeinit(&graph);
}
```

`Graph(T)` is meant for analysis-heavy work such as reachability, control-flow, and dependency traversal. For graph-owned names, prefer `Graph(Str)` plus `GraphInitWithDeepCopy(NULL, StrDeinit)` and insert inline owned strings with `GraphAddNodeR(..., StrZ("..."))` as shown above.

`Graph(const char *)` is still valid, but only when every stored pointer refers to memory that outlives the graph, such as string literals, interned names, or externally owned stable storage. It is a borrowed-pointer graph, not a copying string graph.

This is separate from the formatted-I/O caveat later in this README: `WriteFmtLn("{}", GraphNodeData(&graph, node))` is fine here because `GraphNodeData(...)` yields a `Str` or `const char *` value. What does not work is passing a raw string-literal or `char[]` array expression directly to the formatter.

In real named-node workloads you usually pair the graph with a side `Map(name -> node_id)`. The in-depth guide on the docs site covers that full model: node ids and handles, predecessor traversal, deferred deletion, and side-state patterns.

### Formatted I/O

```c
#include <Misra.h>

int main(void) {
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
    const char* cursor = "Count: 42, Name: Test";
    int read_count = 0;
    Str read_name = StrInit();
    
    // StrReadFmt advances the input cursor on success
    StrReadFmt(cursor, "Count: {}, Name: {}", read_count, read_name);
    
    // Multiple value types
    float pi = 3.14159f;
    u64 big_num = 123456789ULL;
    StrWriteFmt(&output, "Float: {.2}, Integer: {}, Hex: {x}\n", pi, big_num, big_num);
    
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

typedef Vec(Point) PointVec;

typedef struct Shape {
    Str name;
    Point position;
    PointVec vertices;
    bool filled;
} Shape;

int main(void) {
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
    VecForeachPtr(&shape.vertices, vertex) {
        vertex->y += 1.0;  // Move all points up by 1
    }

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

typedef Vec(int) IntVec;

// Complex type with owned resources
typedef struct {
    int id;
    IntVec data;
} ComplexType;

typedef Vec(ComplexType) ComplexVec;

// Copy initialization for deep copying
bool ComplexTypeCopyInit(ComplexType* dst, const ComplexType* src) {
    dst->id = src->id;
    dst->data = VecInit();
    
    // Copy all elements from source vector
    VecForeachIdx(&src->data, val, idx) {
        VecInsertR(&dst->data, val, idx);
    }
    return true;
}

// Proper cleanup of owned resources
void ComplexTypeDeinit(ComplexType* ct) {
    VecDeinit(&ct->data);
}

int main(void) {
    // Vector of complex types with resource management
    ComplexVec objects = VecInitWithDeepCopy(ComplexTypeCopyInit, ComplexTypeDeinit);
    
    // Create and insert items
    ComplexType item = {
        .id = 1,
        .data = VecInit()
    };
    VecInsertR(&item.data, 42, 0);
    VecInsertR(&item.data, 43, 1);
    
    // Insert with ownership transfer
    VecInsertL(&objects, item, 0);   // item is now owned by vector
    
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

The library supports placeholders of the form `{}` or `{[alignment][width][.precision][flags]}`.

### Important: Supported Argument Types

The formatting macros use `_Generic` for compile-time dispatch through `IOFMT(...)`, so the exact C type matters.

#### What Doesn't Work

```c
// String literals are arrays, not pointers
StrWriteFmt(&output, "Hello, {}!", "world");

// Plain char arrays are also distinct array types
char buffer[20] = "Hello";
StrWriteFmt(&output, "Message: {}", buffer);

const char name[] = "Alice";
StrWriteFmt(&output, "Name: {}", name);
```

#### What Works

```c
const char* title = "Mr.";
const char* surname = "Smith";
StrWriteFmt(&output, "{} {}", title, surname);

char* dynamic_str = malloc(50);
strcpy(dynamic_str, "Dynamic");
StrWriteFmt(&output, "Value: {}", dynamic_str);

Str greeting = StrInitFromZstr("Welcome");
StrWriteFmt(&output, "Message: {}", greeting);

int number = 42;
float pi = 3.14f;
StrWriteFmt(&output, "Number: {}, Pi: {.2}", number, pi);
```

#### Best Practices

```c
// Bind string constants to pointer variables
const char* program_name = "MyApp";
const char* version = "1.0.0";

// StrReadFmt expects the input itself to be an assignable cursor variable
const char* input_line = "Name: Alice";
Str user_input = StrInit();
StrReadFmt(input_line, "Name: {}", user_input);

char* allocated = malloc(100);
strcpy(allocated, "Dynamic content");
StrWriteFmt(&message, "Content: {}", allocated);

void log_message(const char* msg) {
    StrWriteFmt(&log_output, "[LOG] {}", msg);
}

void process_buffer(char* buffer) {
    StrWriteFmt(&output, "Processing: {}", buffer);
}
```

#### Technical Explanation

`_Generic` currently has cases for:

- `const char*`
- `char*`
- `Str`
- `Float`
- `Int`
- `BitVec`
- primitive integer and floating-point types
- `char`

It does not automatically treat array types as pointer cases, so `char[6]`, `char[20]`, `const char[10]`, and similar
types must be bound to `char*` or `const char*` variables first.

That typing rule is separate from pointer lifetime. Once a value has pointer type, formatting it is fine only if the pointed-to storage is still valid for the duration of the call.

### Basic Usage

If no specifier is provided, default formatting is used.

### Format Specifier Options

#### Alignment

Controls text alignment within a field width:

| Specifier | Description |
|-----------|-------------|
| `<` | Left-aligned (pad on the right) |
| `>` | Right-aligned (pad on the left, default) |
| `^` | Center-aligned (pad on both sides) |

#### Width

Specifies the minimum field width for text formatting:

```c
{5}    // Minimum width of 5 characters, right-aligned
{<5}   // Minimum width of 5 characters, left-aligned
{^5}   // Minimum width of 5 characters, center-aligned
```

#### Endianness and Raw I/O

When the `r` flag is present, alignment controls endianness and width controls raw byte count:

| Specifier | Description |
|-----------|-------------|
| `<` | Little Endian |
| `>` | Big Endian (default) |
| `^` | Native Endian |

```c
{4r}    // Read or write 4 bytes in big-endian order
{>4r}   // Same as above
{<2r}   // Read or write 2 bytes in little-endian order
{^8r}   // Read or write 8 bytes in native-endian order
```

#### Type Flags

| Flag | Description | Example Output |
|------|-------------|----------------|
| `x` | Hexadecimal format (lowercase) | `0xdeadbeef` |
| `X` | Hexadecimal format (uppercase) | `0xDEADBEEF` |
| `b` | Binary format | `0b10100101` |
| `o` | Octal format | `0o777` |
| `c` | Character formatting, preserve case | Raw character bytes |
| `a` | Character formatting, force lowercase | Lowercased text |
| `A` | Character formatting, force uppercase | Uppercased text |
| `r` | Raw data read or write | raw bytes |
| `e` | Scientific notation (lowercase) | `1.235e+02` |
| `E` | Scientific notation (uppercase) | `1.235E+02` |
| `s` | Read a quoted string or a single word | `"hello world"` |

#### Precision

For floating-point values, precision controls decimal places:

```c
{.2}   // Two decimal places
{.0}   // No decimal places
{.10}  // Ten decimal places
```

Precision is ignored for raw I/O.

### Format Examples

#### Basic Formatting

```c
const char* greeting = "Hello";
const char* subject = "world";
StrWriteFmt(&output, "{}, {}!", greeting, subject);  // "Hello, world!"

StrWriteFmt(&output, "{{Hello}}");  // "{Hello}"
```

#### String Formatting

```c
const char* str = "Hello";

StrWriteFmt(&output, "{}", str);       // "Hello"
StrWriteFmt(&output, "{>10}", str);    // "     Hello"
StrWriteFmt(&output, "{<10}", str);    // "Hello     "
StrWriteFmt(&output, "{^10}", str);    // "  Hello   "
```

#### Integer Formatting

```c
i32 val = 42;
u32 hex_val = 0xDEADBEEF;
u8 bin_val = 0xA5;
u16 oct_val = 0777;

StrWriteFmt(&output, "{}", val);       // "42"
StrWriteFmt(&output, "{x}", hex_val);  // "0xdeadbeef"
StrWriteFmt(&output, "{X}", hex_val);  // "0xDEADBEEF"
StrWriteFmt(&output, "{b}", bin_val);  // "0b10100101"
StrWriteFmt(&output, "{o}", oct_val);  // "0o777"

StrWriteFmt(&output, "{5}", val);      // "   42"
StrWriteFmt(&output, "{<5}", val);     // "42   "
StrWriteFmt(&output, "{^5}", val);     // " 42  "

Int big = IntFromHexStr("deadbeef");
StrWriteFmt(&output, "{x}", big);      // "deadbeef"
StrWriteFmt(&output, "{b}", big);      // "11011110101011011011111011101111"
```

#### Character and Case Formatting

```c
u8 upper_char = 'M';
u8 lower_char = 'm';

StrWriteFmt(&output, "{c}", upper_char);  // "M"
StrWriteFmt(&output, "{a}", upper_char);  // "m"
StrWriteFmt(&output, "{A}", lower_char);  // "M"

u16 u16_value = ('A' << 8) | 'B';
StrWriteFmt(&output, "{c}", u16_value);   // "AB"
StrWriteFmt(&output, "{a}", u16_value);   // "ab"
StrWriteFmt(&output, "{A}", u16_value);   // "AB"

const char* mixed_case = "MiXeD CaSe";
StrWriteFmt(&output, "{c}", mixed_case);  // "MiXeD CaSe"
StrWriteFmt(&output, "{a}", mixed_case);  // "mixed case"
StrWriteFmt(&output, "{A}", mixed_case);  // "MIXED CASE"

Str s = StrInitFromZstr("Hello World");
StrWriteFmt(&output, "{a}", s);           // "hello world"
StrWriteFmt(&output, "{A}", s);           // "HELLO WORLD"
```

#### Floating-Point Formatting

```c
f64 pi = 3.14159265359;

StrWriteFmt(&output, "{}", pi);         // "3.141593"
StrWriteFmt(&output, "{.2}", pi);       // "3.14"
StrWriteFmt(&output, "{.0}", pi);       // "3"
StrWriteFmt(&output, "{.10}", pi);      // "3.1415926536"

StrWriteFmt(&output, "{e}", 123.456);   // "1.235e+02"
StrWriteFmt(&output, "{E}", 123.456);   // "1.235E+02"
StrWriteFmt(&output, "{.3e}", 123.456); // "1.235e+02"

f64 pos_inf = INFINITY;
f64 neg_inf = -INFINITY;
f64 nan_val = NAN;
StrWriteFmt(&output, "{}", pos_inf);    // "inf"
StrWriteFmt(&output, "{}", neg_inf);    // "-inf"
StrWriteFmt(&output, "{}", nan_val);    // "nan"

Float big_float = FloatFromStr("12345.67");
StrWriteFmt(&output, "{}", big_float);   // "12345.67"
StrWriteFmt(&output, "{e}", big_float);  // "1.234567e+04"
StrWriteFmt(&output, "{.3}", big_float); // "12345.670"
```

### Reading Values

`StrReadFmt` updates the input cursor variable on success, so pass an assignable pointer variable rather than a string
literal expression.

```c
const char* cursor = "42";
i32 num = 0;
StrReadFmt(cursor, "{}", num);  // num = 42

cursor = "0xdeadbeef";
u32 hex_val = 0;
StrReadFmt(cursor, "{}", hex_val);  // hex_val = 0xdeadbeef

cursor = "0b101010";
i8 bin_val = 0;
StrReadFmt(cursor, "{}", bin_val);  // bin_val = 42

cursor = "0o755";
i32 oct_val = 0;
StrReadFmt(cursor, "{}", oct_val);  // oct_val = 493

cursor = "3.14159";
f64 value = 0.0;
StrReadFmt(cursor, "{}", value);  // value = 3.14159

cursor = "deadbeef";
Int big = IntInit();
StrReadFmt(cursor, "{x}", big);   // big = 0xdeadbeef

cursor = "1.23e4";
StrReadFmt(cursor, "{}", value);  // value = 12300.0

cursor = "1.234567e+04";
Float big_float = FloatInit();
StrReadFmt(cursor, "{e}", big_float);  // big_float = 12345.67

cursor = "Alice";
Str name = StrInit();
StrReadFmt(cursor, "{}", name);  // name = "Alice"

cursor = "\"Hello, World!\"";
StrReadFmt(cursor, "{s}", name);  // name = "Hello, World!"

cursor = "Count: 42, Name: Alice";
i32 count = 0;
Str user = StrInit();
StrReadFmt(cursor, "Count: {}, Name: {}", count, user);
// count = 42, user = "Alice"
```

### Available I/O Functions

- `StrWriteFmt(&str, format, ...)`: Append formatted output to a string
- `StrReadFmt(input, format, ...)`: Parse values from a cursor variable and advance it on success
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
