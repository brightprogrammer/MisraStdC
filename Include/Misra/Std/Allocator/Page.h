/// file      : std/allocator/page.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Page-granular allocator. Allocations come straight from the operating
/// system via `mmap` on POSIX and `VirtualAlloc` on Windows, so this
/// allocator does not touch libc heap functions. Per-allocator state is
/// inline; the only "state" is a cached page size and a small descriptor
/// table tracking (ptr, mmap-byte-length) for every live region so that
/// free can recover the kernel-needed byte count without the caller
/// supplying it.

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
    /// Per-live-allocation descriptor. The byte count is the *rounded*
    /// mmap length (a multiple of the OS page size), which is what
    /// `munmap` / `VirtualFree` need. The user pointer is the mmap'd
    /// base address.
    ///
    typedef struct PageEntry {
        void *ptr;
        size  bytes;
    } PageEntry;

    ///
    /// Typed page-backed allocator. Carries `Allocator base` at offset 0 so
    /// `(Allocator *)&page` is well-defined.
    ///
    /// FIELDS:
    /// - base                : Generic allocator base (function pointers, alignment, ...).
    /// - cached_page_size    : Lazily-cached system page size in bytes, 0 until first query.
    /// - entries             : Descriptor array for live mmap'd regions, sorted by `ptr`
    ///                         ascending; managed via raw page_map/page_unmap calls (not
    ///                         through the public Allocator dispatch -- would recurse).
    /// - len                 : Number of live entries.
    /// - cap                 : Capacity of `entries` (geometric growth).
    /// - entries_bytes       : Rounded mmap length of the `entries` table itself,
    ///                         retained so PageAllocatorDeinit can unmap it.
    /// - free_entries        : Retained (user-freed but not yet returned to the OS)
    ///                         descriptor array, sorted by `bytes` ascending so the
    ///                         alloc-side exact-size match is a binary search.
    ///                         Page-level release policy is: once mmap'd, regions are
    ///                         kept until `PageAllocatorDeinit`; deallocate moves an
    ///                         entry from `entries[]` to `free_entries[]`, allocate
    ///                         first searches `free_entries[]` for an exact-size match
    ///                         before going to the kernel. The freed mmap regions
    ///                         themselves are never written into (allocator data lives
    ///                         in this sibling table only -- see CODING-CONVENTIONS:
    ///                         user-handed memory is opaque even after reclaim).
    ///                         Double-free detection on this table reduces to "ptr is
    ///                         missing from entries[]"; the error message is the
    ///                         combined "foreign or already-freed", same as before.
    /// - free_len            : Number of retained entries.
    /// - free_cap            : Capacity of `free_entries` (geometric growth, separate
    ///                         from `cap` since the two tables grow independently).
    /// - free_entries_bytes  : Rounded mmap length of the `free_entries` table itself.
    ///
    /// TAGS: Allocator, Page, Memory
    ///
    typedef struct PageAllocator {
        Allocator  base;
        size       cached_page_size;
        PageEntry *entries;
        u32        len;
        u32        cap;
        size       entries_bytes;
        PageEntry *free_entries;
        u32        free_len;
        u32        free_cap;
        size       free_entries_bytes;
    } PageAllocator;

    void *page_allocator_allocate(Allocator *self, size bytes, i8 zeroed);
    i8    page_allocator_resize(Allocator *self, void *ptr, size new_size);
    void *page_allocator_remap(Allocator *self, void *ptr, size new_size);

    ///
    /// Free a region previously returned by
    /// `AllocatorAlloc(&page.base, ...)`. The byte count to munmap is
    /// recovered from the allocator's internal descriptor table -- the
    /// caller does NOT pass a size.
    ///
    /// SUCCESS: Returns the rounded mmap length that was released
    ///          (page-aligned, what stats accounting sees). The kernel
    ///          mapping is gone.
    /// FAILURE: Aborts via `LOG_FATAL` when `ptr` is foreign to this
    ///          allocator or has already been freed.
    ///
    size page_allocator_deallocate(Allocator *self, void *ptr);

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
    /// is to apply this to a region returned by `PageAllocator` allocation
    /// (which is always page-grain).
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

    ///
    /// Tear down a `PageAllocator`. Any region still tracked in `entries`
    /// (a caller leak, e.g. forgot to `AllocatorFree`) is munmapped so
    /// the kernel doesn't keep the mapping around. The descriptor table
    /// itself is then released and the struct is zeroed -- post-deinit
    /// dispatch trips `ValidateAllocator` on the zeroed `__magic`.
    ///
    /// self[in,out] : PageAllocator instance, or NULL.
    ///
    /// SUCCESS: Function returns. Every live mapping owned by this
    ///          allocator has been released; the struct is fully
    ///          zeroed and cannot be used until re-initialised.
    /// FAILURE: No action when `self` is NULL.
    ///
    /// TAGS: Allocator, Page, Cleanup
    ///
    void PageAllocatorDeinit(PageAllocator *self);

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
///     AllocatorFree(&page.base, p);
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
        .cached_page_size   = 0,                                                                                       \
        .entries            = NULL,                                                                                    \
        .len                = 0,                                                                                       \
        .cap                = 0,                                                                                       \
        .entries_bytes      = 0,                                                                                       \
        .free_entries       = NULL,                                                                                    \
        .free_len           = 0,                                                                                       \
        .free_cap           = 0,                                                                                       \
        .free_entries_bytes = 0,                                                                                       \
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
        .cached_page_size   = 0,                                                                                       \
        .entries            = NULL,                                                                                    \
        .len                = 0,                                                                                       \
        .cap                = 0,                                                                                       \
        .entries_bytes      = 0,                                                                                       \
        .free_entries       = NULL,                                                                                    \
        .free_len           = 0,                                                                                       \
        .free_cap           = 0,                                                                                       \
        .free_entries_bytes = 0,                                                                                       \
    })

#endif // MISRA_STD_ALLOCATOR_PAGE_H
