/// file      : std/allocator/_os.h  (INTERNAL)
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Internal-library shim over the OS page-management primitives
/// (mmap/munmap on POSIX, VirtualAlloc/VirtualFree on Windows). NOT
/// part of the public API. Used by allocator implementations
/// (HeapAllocator, ArenaAllocator, ...) so each allocator can talk to
/// the kernel directly and remain the single source of truth for its
/// own region tracking and retention policy.
///
/// The `_` prefix on the file name marks the header as not exported
/// from the install tree.

#ifndef MISRA_STD_ALLOCATOR_OS_H_INTERNAL
#define MISRA_STD_ALLOCATOR_OS_H_INTERNAL

#include <Misra/Types.h>

#ifdef __cplusplus
extern "C" {
#endif

    // Forward declaration. `Allocator` is the base struct every typed
    // allocator embeds at offset 0. _Os.h only needs to know it exists
    // and that `footprint_bytes` is reachable from a pointer to it;
    // <Misra/Std/Allocator.h> carries the full definition and is
    // included by the typed allocator's source TU.
    typedef struct Allocator Allocator;

    /// OS page size, in bytes. First call queries the platform (kernel32
    /// on Windows, compile-time constant on POSIX since we control the
    /// supported architecture/OS matrix); subsequent calls return the
    /// cached value. Thread-safety: the cached store uses a one-shot
    /// idempotent write -- callers may race on the first call but every
    /// observed value is the same.
    size os_page_size(void);

    /// mmap an anonymous, page-zeroed region of `bytes` bytes (which MUST
    /// be a multiple of `os_page_size()`). `owner->footprint_bytes` is
    /// bumped by `bytes` on success so footprint accounting moves in
    /// lock-step with the kernel call -- no per-site discipline
    /// required. Returns the region pointer or NULL on OS failure.
    /// The kernel zeros the region, so callers that need zeroed memory
    /// do not need to MemSet on top.
    void *os_page_map(Allocator *owner, size bytes);

    /// Release a region previously returned by `os_page_map`.
    /// `owner->footprint_bytes` is drawn down by `bytes`. The caller
    /// MUST pass the same `bytes` it requested -- the OS shim holds
    /// no per-region bookkeeping. Passing a wrong size or a stale
    /// pointer is undefined behaviour.
    void os_page_unmap(Allocator *owner, void *ptr, size bytes);

    /// Try to resize an existing `os_page_map` region in-place to
    /// `new_bytes` (must be page-multiple). `owner->footprint_bytes`
    /// is adjusted by `new_bytes - old_bytes` on success. The kernel
    /// may move the region; the returned pointer is the new address
    /// (possibly unchanged). Returns NULL if the resize cannot be
    /// honoured AT ALL on this platform / for this request -- callers
    /// MUST treat NULL as "fall back to alloc-new + MemCopy +
    /// unmap-old", not as a freed region. The old region remains
    /// valid on NULL return and footprint accounting is untouched.
    ///
    /// Implementation:
    ///   Linux direct-syscall : `mremap(..., MREMAP_MAYMOVE)`.
    ///   Darwin / Windows     : not supported, returns NULL. (XNU has
    ///                          no public mremap; Win32 has no analogue
    ///                          for resize-in-place of an mmapped
    ///                          region -- VirtualAlloc reservations
    ///                          can't be safely grown.)
    void *os_page_remap(Allocator *owner, void *ptr, size old_bytes, size new_bytes);

    /// Round `bytes` up to the next multiple of `os_page_size()`.
    /// Convenience for callers that compute mmap request sizes from
    /// arbitrary byte counts and need the exact rounded length back so
    /// the matching unmap can pass it.
    static inline size os_page_round_up(size bytes) {
        size ps = os_page_size();
        return (bytes + ps - 1u) & ~(ps - 1u);
    }

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_ALLOCATOR_OS_H_INTERNAL
