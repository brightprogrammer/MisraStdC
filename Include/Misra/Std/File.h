/// file      : misra/std/file.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// File helper utilities

#ifndef MISRA_FILE_H
#define MISRA_FILE_H

#include <stddef.h>
#include <stdio.h>

// decompiler
#include <Misra/Std/Allocator.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Sys.h>
#include <Misra/Types.h>

/// Snake_case runtime helper. Users call `ReadCompleteFile(...)`; the
/// macro routes through `MISRA_OVERLOAD` to one of the per-arity forms
/// below, which forward to this function.
bool read_complete_file(
    const char *filename,
    char      **data,
    u64        *file_size,
    u64        *capacity,
    Allocator  *allocator
);

///
/// Read complete contents of a file at once.
///
/// Two forms via argument-count overload:
///
/// - `ReadCompleteFile(filename, data, file_size, capacity)` - inside a
///   `Scope` block; the buffer is allocated through `MisraScope`.
/// - `ReadCompleteFile(filename, data, file_size, capacity, allocator)`
///   - explicit allocator (typed handle or raw `Allocator *`).
///
/// The 4-arg form fails to compile outside any `Scope` block because
/// `MisraScope` is undeclared - the library does not accept a NULL
/// allocator anywhere.
///
/// filename[in]     : Name/path of file to be read.
/// data[in,out]     : Memory buffer where loaded file will be stored.
///                    The buffer is null-terminated for convenience.
/// file_size[out]   : Complete size of file in bytes.
/// capacity[in,out] : Current capacity of `*data`, updated on successful growth.
/// allocator[in,out]: Allocator responsible for `*data`.
///
/// SUCCESS : Returns true. `*data`, `*file_size`, and `*capacity` are updated.
/// FAILURE : Returns false on I/O or allocation failure. The buffer state
///           may be partially updated.
///
/// TAGS: Read, File, I/O, Utility, Allocator
///
#define ReadCompleteFile(...) MISRA_OVERLOAD(ReadCompleteFile, __VA_ARGS__)
#define ReadCompleteFile_4(filename, data, file_size, capacity)                                                        \
    read_complete_file((filename), (data), (file_size), (capacity), MisraScope)
#define ReadCompleteFile_5(filename, data, file_size, capacity, allocator)                                             \
    read_complete_file((filename), (data), (file_size), (capacity), (allocator))

#endif // MISRA_FILE_H
