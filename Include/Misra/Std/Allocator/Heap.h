/// file      : std/allocator/heap.h
/// author    : Generated during allocator refactor
/// This is free and unencumbered software released into the public domain.
///
/// Heap allocator descriptor helpers.

#ifndef MISRA_STD_ALLOCATOR_HEAP_H
#define MISRA_STD_ALLOCATOR_HEAP_H

#include <Misra/Std/Allocator.h>

#ifdef __cplusplus
extern "C" {
#endif

///
/// Create an allocator descriptor backed by the process heap.
/// The returned allocator uses `malloc`/`realloc`/`free` semantics, with
/// aligned allocation where required by the requested alignment.
///
/// SUCCESS: Returns a heap-backed allocator descriptor.
/// FAILURE: Function cannot fail.
///
/// TAGS: Allocator, Heap, Initialization, Memory
///
Allocator HeapAllocator(void);

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_ALLOCATOR_HEAP_H
