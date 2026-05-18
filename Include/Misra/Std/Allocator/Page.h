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

///
/// Per-type magic for `PageAllocator`. Stamped into
/// `Allocator.base.__magic` by `PageAllocatorInit*`. The page
/// implementation functions validate this exact value so a
/// `HeapAllocator` / `ArenaAllocator` / `PoolAllocator` reinterpreted
/// as a `PageAllocator *` is rejected at runtime as type-confusion.
///
#define PAGE_ALLOCATOR_MAGIC MAKE_NEW_MAGIC_VALUE("pageallc")

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
    typedef struct PageAllocator {
        Allocator base;
        size      cached_page_size;
    } PageAllocator;

    void *page_allocator_allocate(Allocator *self, size bytes, i8 zeroed);
    i8    page_allocator_resize(Allocator *self, void *ptr, size old_size, size new_size);
    void *page_allocator_remap(Allocator *self, void *ptr, size old_size, size new_size);

    ///
    /// Generic-dispatch deallocate stub. `PageAllocator` cannot recover
    /// the original allocation size from `ptr` alone (`munmap` /
    /// `VirtualFree` need the byte count). Calling
    /// `AllocatorFree(&page.base, ptr)` therefore aborts via
    /// `LOG_FATAL` -- route Page frees through `PageAllocatorFree`
    /// instead, which carries the explicit size.
    ///
    /// Consequence: `PageAllocator` is not suitable as the backing
    /// allocator for `Vec` / `Map` / `Str` (their `*Deinit` calls
    /// `AllocatorFree`). Wrap Page in a `HeapAllocator` for that.
    ///
    size page_allocator_deallocate(Allocator *self, void *ptr);

    ///
    /// Typed deallocator for a region previously returned by
    /// `AllocatorAlloc(&page.base, ...)` (or the `MisraScope` /
    /// `ScopeWith` macros over a PageAllocator). Bypasses the generic
    /// `AllocatorFree` dispatch because PageAllocator needs the byte
    /// count -- the kernel mapping API does.
    ///
    /// self[in,out] : PageAllocator that issued the allocation.
    /// ptr[in]      : Allocation pointer, or NULL.
    /// bytes[in]    : Original allocation size in bytes (the same value
    ///                that was passed to `AllocatorAlloc`).
    ///
    /// SUCCESS: Function returns; the kernel mapping is released.
    /// FAILURE: Aborts via `LOG_FATAL` on a NULL or type-confused
    ///          `self`. A NULL `ptr` is a no-op.
    ///
    /// TAGS: Allocator, Page, Deallocation
    ///
    void PageAllocatorFree(PageAllocator *self, void *ptr, size bytes);

    ///
    /// Page-level memory protection bits. The actual OS permissions are
    /// platform-mapped: POSIX uses `mprotect` with `PROT_*` combinations;
    /// Windows uses `VirtualProtect` with `PAGE_*`.
    ///
    /// FIELDS:
    /// - PAGE_PROT_NONE       : Reads, writes, and executes all trap.
    ///                          Use for guard pages or to make a freed
    ///                          region trap on any dangling-pointer access.
    /// - PAGE_PROT_READ       : Reads allowed, writes/executes trap.
    ///                          Use for after-init read-only tables.
    /// - PAGE_PROT_READ_WRITE : Reads and writes allowed. The default
    ///                          state for freshly-allocated memory.
    ///
    /// TAGS: Allocator, Page, Memory-Protection
    ///
    typedef enum PageProtection {
        PAGE_PROT_NONE       = 0,
        PAGE_PROT_READ       = 1,
        PAGE_PROT_READ_WRITE = 2,
    } PageProtection;

    ///
    /// Change the protection of a range of pages. `ptr` and `bytes` must
    /// be page-aligned and page-sized respectively; the typical pattern
    /// is to apply this to a region returned by
    /// `page_allocator_allocate` (which is always page-grain).
    ///
    /// ptr[in,out] : First byte of the region; must be page-aligned.
    /// bytes[in]   : Region size; must be a multiple of the OS page size.
    /// prot[in]    : New protection bits.
    ///
    /// SUCCESS : Returns true. The new protection is in effect for the
    ///           entire `[ptr, ptr+bytes)` range.
    /// FAILURE : Returns false and logs the failing syscall (`mprotect`
    ///           / `VirtualProtect`). The region's protection is
    ///           unchanged in the failure case.
    ///
    /// TAGS: Allocator, Page, Memory-Protection
    ///
    bool PageProtect(void *ptr, size bytes, PageProtection prot);

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
///     void *p = AllocatorAlloc(&page.base, 64 * 1024, true);
///     PageAllocatorFree(&page, p, 64 * 1024);
///
#define PageAllocatorInit()                                                                                            \
    ((PageAllocator) {                                                                                                 \
        .base =                                                                                                        \
            {.allocate    = page_allocator_allocate,                                                                   \
                   .resize      = page_allocator_resize,                                                                     \
                   .remap       = page_allocator_remap,                                                                      \
                   .deallocate  = page_allocator_deallocate,                                                                 \
                   .alignment   = 1,                                                                                         \
                   .effort      = ALLOCATOR_EFFORT_ONCE,                                                                     \
                   .retry_limit = 0,                                                                                         \
                   .__magic     = PAGE_ALLOCATOR_MAGIC},                                                                         \
        .cached_page_size = 0                                                                                          \
    })

///
/// Initialize a `PageAllocator` with a custom alignment floor. Page-backed
/// memory is naturally page-aligned, so requests below the page size are
/// rounded up. Stronger-than-page alignment is best-effort.
///
#define PageAllocatorInitAligned(N)                                                                                    \
    ((PageAllocator) {                                                                                                 \
        .base =                                                                                                        \
            {.allocate    = page_allocator_allocate,                                                                   \
                   .resize      = page_allocator_resize,                                                                     \
                   .remap       = page_allocator_remap,                                                                      \
                   .deallocate  = page_allocator_deallocate,                                                                 \
                   .alignment   = (N) ? (N) : 1,                                                                             \
                   .effort      = ALLOCATOR_EFFORT_ONCE,                                                                     \
                   .retry_limit = 0,                                                                                         \
                   .__magic     = PAGE_ALLOCATOR_MAGIC},                                                                         \
        .cached_page_size = 0                                                                                          \
    })

///
/// Teardown for `PageAllocator`. The allocator owns no per-instance
/// pages (each `allocate` is matched by a `deallocate` `munmap`), so
/// teardown just zeroes the struct - post-deinit dispatch then trips
/// `ValidateAllocator` on the zeroed `__magic` instead of silently
/// re-using the function-pointer table.
///
#define PageAllocatorDeinit(self) MemSet((self), 0, sizeof(*(self)))

#endif // MISRA_STD_ALLOCATOR_PAGE_H
