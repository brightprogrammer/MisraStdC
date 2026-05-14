/// file      : std/allocator/page.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Page-granular allocator. Allocations come straight from the operating
/// system via `mmap` on POSIX and `VirtualAlloc` on Windows, so this
/// allocator does not touch libc heap functions.

#ifndef MISRA_STD_ALLOCATOR_PAGE_H
#define MISRA_STD_ALLOCATOR_PAGE_H

#include <Misra/Std/Allocator.h>

#ifdef __cplusplus
extern "C" {
#endif

    ///
    /// Create an allocator descriptor backed by direct OS page mapping.
    /// All allocations are rounded up to a multiple of the system page size
    /// and zeroed by the kernel before being handed back. No libc heap
    /// functions are invoked.
    ///
    /// SUCCESS: Returns a page-backed allocator descriptor.
    /// FAILURE: Function cannot fail.
    ///
    /// TAGS: Allocator, Page, Initialization, Memory
    ///
    Allocator PageAllocator(void);

    ///
    /// Create a page-backed allocator with a custom alignment floor.
    /// Allocations always honor at least page-size alignment - which is
    /// what `mmap` / `VirtualAlloc` guarantee. Requests for alignment
    /// stronger than the page size are not honored by this allocator: a
    /// future allocator backend will offer that capability by over-mapping
    /// and trimming.
    ///
    /// alignment[in] : Required minimum alignment in bytes.
    ///
    /// SUCCESS: Returns a page-backed allocator descriptor with the requested
    ///          minimum alignment.
    /// FAILURE: Function cannot fail.
    ///
    /// TAGS: Allocator, Page, Aligned, Initialization
    ///
    Allocator PageAllocatorAligned(size alignment);

    ///
    /// Query the system page size in bytes. Cached after first call so
    /// repeated lookups are cheap.
    ///
    /// SUCCESS: Returns the OS page size (typically 4096 on x86_64).
    /// FAILURE: Returns `4096` if the platform query fails.
    ///
    /// TAGS: Allocator, Page, Query, Memory
    ///
    size PageAllocatorPageSize(void);

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_ALLOCATOR_PAGE_H
