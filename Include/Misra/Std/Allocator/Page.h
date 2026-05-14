/// file      : std/allocator/page.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Page-granular allocator. Allocations come straight from the operating
/// system via `mmap` on POSIX and `VirtualAlloc` on Windows, so this
/// allocator does not touch libc heap functions. State is inline; the
/// only "state" is a cached page size and the user's struct lives wherever
/// the user declared it (typically the stack).

#ifndef MISRA_STD_ALLOCATOR_PAGE_H
#define MISRA_STD_ALLOCATOR_PAGE_H

#include <Misra/Std/Allocator.h>

#ifdef __cplusplus
extern "C" {
#endif

    ///
    /// Typed page-backed allocator. Carries `Allocator base` at offset 0 so
    /// `(Allocator *)&page` is well-defined. The body of an allocation routes
    /// through `mmap`/`VirtualAlloc` directly - no per-instance state needs
    /// to be allocated.
    ///
    /// FIELDS:
    /// - base             : Generic allocator base (function pointers, alignment, ...).
    /// - cached_page_size : Lazily-cached system page size in bytes, 0 until first query.
    ///
    /// TAGS: Allocator, Page, Memory
    ///
    typedef struct {
        Allocator base;
        size      cached_page_size;
    } PageAllocator;

    void *page_allocator_allocate(Allocator *self, size bytes, bool zeroed);
    void *page_allocator_reallocate(Allocator *self, void *ptr, size old_size, size new_size);
    void  page_allocator_deallocate(Allocator *self, void *ptr, size bytes);

    ///
    /// Query the system page size in bytes through a `PageAllocator`. The
    /// result is cached inside the allocator instance after the first call.
    ///
    /// self[in,out] : PageAllocator handle (may be NULL for a one-shot query).
    ///
    /// SUCCESS: Returns the OS page size in bytes (typically 4096 on x86_64).
    /// FAILURE: Returns `4096` when the OS query fails.
    ///
    /// TAGS: Allocator, Page, Query
    ///
    size PageAllocatorPageSize(PageAllocator *self);

#ifdef __cplusplus
}
#endif

///
/// Initialize a `PageAllocator` with default settings (alignment = 1, the
/// natural page-grain alignment of `mmap`/`VirtualAlloc`). Use as a
/// designated-initializer:
///
///     PageAllocator page = PageAllocatorInit();
///     Vec(int) v = VecInit(&page);
///
#define PageAllocatorInit()                                                                                            \
    ((PageAllocator) {.base             = {.allocate    = page_allocator_allocate,                                     \
                                           .reallocate  = page_allocator_reallocate,                                   \
                                           .deallocate  = page_allocator_deallocate,                                   \
                                           .alignment   = 1,                                                           \
                                           .effort      = ALLOCATOR_EFFORT_ONCE,                                       \
                                           .retry_limit = 0,                                                           \
                                           .flags       = 0},                                                          \
                      .cached_page_size = 0})

///
/// Initialize a `PageAllocator` with a custom alignment floor. Page-backed
/// memory is naturally page-aligned, so requests below the page size are
/// rounded up. Stronger-than-page alignment is best-effort.
///
#define PageAllocatorInitAligned(N)                                                                                    \
    ((PageAllocator) {.base             = {.allocate    = page_allocator_allocate,                                     \
                                           .reallocate  = page_allocator_reallocate,                                   \
                                           .deallocate  = page_allocator_deallocate,                                   \
                                           .alignment   = (N) ? (N) : 1,                                               \
                                           .effort      = ALLOCATOR_EFFORT_ONCE,                                       \
                                           .retry_limit = 0,                                                           \
                                           .flags       = 0},                                                          \
                      .cached_page_size = 0})

///
/// Teardown for `PageAllocator`. Stateless; expands to a no-op. Provided for
/// API symmetry with the stateful allocators.
///
#define PageAllocatorDeinit(self) ((void)(self))

#endif // MISRA_STD_ALLOCATOR_PAGE_H
