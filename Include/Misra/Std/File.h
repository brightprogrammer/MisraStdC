/// file      : misra/std/file.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// copyright : Copyright (c) 2024, Siddharth Mishra, All rights reserved.
///
/// File helper utilities

#ifndef MISRA_FILE_H
#define MISRA_FILE_H

#include <stddef.h>
#include <stdint.h>

// decompiler
#include <Misra/Std/Container/Str.h>
#include <Misra/Sys.h>
#include <Misra/Types.h>

///
/// Read complete contents of file at once.
///
/// Pointer returned is malloc'd and hence must be freed after use.
/// The returned pointer can also be reused by providing pointer to it
/// in `data` parameter.
///
/// `realloc` is called on `*data` in order to expand it's size.
/// If `*capacity` exceeds the size of file to be loaded, then no reallocation
/// is performed. This means the provided buffer will automatically be expanded
/// if required.
///
/// The returned buffer is null-terminated just-in-case.
///
/// The implementation and API is designed in such a way that it can be used
/// with containers like Vec and Str.
///
/// filename[in]     : Name/path of file to be read.
/// data[in,out]     : Memory buffer where loaded file will be stored.
/// file_size[out]   : Complete size of file in bytes will be stored here.
/// capacity[in,out] : Hints towards current capacity of `data` buffer.
///                    New capacity of `data` buffer is automatically stored here if
///                    realloc is performed.
///
/// SUCCESS : true
/// FAILURE : false
///
bool ReadCompleteFile(const char *filename, char **data, size *file_size, size *capacity);

#endif // MISRA_FILE_H
