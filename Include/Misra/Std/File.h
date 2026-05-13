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

///
/// Read complete contents of a file at once.
///
/// This API supports two forms:
///
/// - `ReadCompleteFile(filename, data, file_size, capacity)`
/// - `ReadCompleteFile(filename, data, file_size, capacity, allocator)`
///
/// If `*data == NULL` and no allocator is supplied, `DefaultAllocator()` is used.
/// If `*data == NULL` and an allocator is supplied, that allocator is used.
/// If `*data != NULL`, the allocator responsible for that buffer must be supplied.
/// Omitting it in that case is a caller contract violation.
///
/// The returned buffer is null-terminated for convenience.
///
/// filename[in]     : Name/path of file to be read.
/// data[in,out]     : Memory buffer where loaded file will be stored.
/// file_size[out]   : Complete size of file in bytes will be stored here.
/// capacity[in,out] : Current capacity of `*data`, updated on successful growth.
/// allocator[in,out]: Optional allocator responsible for `*data`.
///
/// SUCCESS : true
/// FAILURE : false
///
/// TAGS: Read, File, I/O, Utility, Allocator
///
bool ReadCompleteFileEx(
    const char *filename,
    char      **data,
    u64        *file_size,
    u64        *capacity,
    Allocator  *allocator
);

///
/// Read complete contents of a file at once.
///
/// This macro dispatches to `ReadCompleteFileEx(...)` and supports both:
///
/// - `ReadCompleteFile(filename, data, file_size, capacity)`
/// - `ReadCompleteFile(filename, data, file_size, capacity, allocator)`
///
/// If `*data` already owns memory, the allocator argument is required.
///
/// SUCCESS : Returns true and updates `data`, `file_size`, and `capacity`.
/// FAILURE : Returns false on I/O or allocation failure.
///
/// TAGS: Read, File, I/O, Utility, Allocator, Macro
///
#define READ_COMPLETE_FILE_HAS_ARGS_IMPL(_1, _2, _3, _4, _5, count, ...) count
#define READ_COMPLETE_FILE_HAS_ARGS(...) READ_COMPLETE_FILE_HAS_ARGS_IMPL(__VA_ARGS__, 5, 4, 3, 2, 1, 0)
#define ReadCompleteFile(...) CONCAT(ReadCompleteFile_, READ_COMPLETE_FILE_HAS_ARGS(__VA_ARGS__))(__VA_ARGS__)
#define ReadCompleteFile_4(filename, data, file_size, capacity)                                                         \
    ReadCompleteFileEx((filename), (data), (file_size), (capacity), NULL)
#define ReadCompleteFile_5(filename, data, file_size, capacity, allocator)                                              \
    ReadCompleteFileEx((filename), (data), (file_size), (capacity), (allocator))

#endif // MISRA_FILE_H
