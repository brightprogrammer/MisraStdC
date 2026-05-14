/// file      : std/allocator/default.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// `DefaultAllocator` is the recommended general-purpose allocator type
/// for code that does not have a more specific requirement. It is a
/// plain type alias for `HeapAllocator` - a per-descriptor binned heap
/// built on top of `PageAllocator`. There is NO global instance; every
/// `DefaultAllocator` is user-owned, stack- or heap-resident, and must
/// be paired with `DefaultAllocatorDeinit` (or `HeapAllocatorDeinit`)
/// on teardown.
///
/// USAGE - direct construction:
///   DefaultAllocator alloc = DefaultAllocatorInit();
///   Vec(int) v = VecInit(&alloc);
///   ...
///   VecDeinit(&v);
///   DefaultAllocatorDeinit(&alloc);
///
/// USAGE - inside a Scope:
///   Scope(lifetime, DefaultAllocator) {
///       Vec(int) v = VecInit();
///       ...
///   }   // allocator destroyed automatically
///
/// Because `DefaultAllocator` is a type alias, every `HeapAllocator*`
/// API works identically on a `DefaultAllocator*` value.

#ifndef MISRA_STD_ALLOCATOR_DEFAULT_H
#define MISRA_STD_ALLOCATOR_DEFAULT_H

#include <Misra/Std/Allocator/Heap.h>

#ifdef __cplusplus
extern "C" {
#endif

    typedef HeapAllocator DefaultAllocator;

#ifdef __cplusplus
}
#endif

#define DefaultAllocatorInit()      HeapAllocatorInit()
#define DefaultAllocatorDeinit(ptr) HeapAllocatorDeinit(ptr)

#endif // MISRA_STD_ALLOCATOR_DEFAULT_H
