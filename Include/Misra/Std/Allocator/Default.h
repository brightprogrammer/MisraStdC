/// file      : std/allocator/default.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// `DefaultAllocator` is the recommended general-purpose allocator
/// type for code that does not have a more specific requirement. It
/// is a plain type alias for `HeapAllocator` -- a per-descriptor
/// binned heap built on top of `PageAllocator`. There is NO global
/// instance; every `DefaultAllocator` is user-owned, stack- or
/// heap-resident, and must be paired with `DefaultAllocatorDeinit`
/// on teardown.
///
/// USAGE - direct construction:
///   DefaultAllocator alloc = DefaultAllocatorInit();
///   Vec(int) v = VecInit(&alloc);
///   ...
///   VecDeinit(&v);
///   DefaultAllocatorDeinit(&alloc);
///
/// **MSan / ASan-style mode (`MISRA_DEFAULT_ALLOC_DEBUG`)**: when the
/// build flag is set, `DefaultAllocator` becomes a `DebugAllocator`
/// instead. The struct shape (init-by-value, ALLOCATOR_OF, deinit)
/// is identical, so existing call sites work unchanged -- they just
/// get leak / double-free / canary-overflow / stack-trace tracking
/// for free. With `DEBUG_ALLOCATOR_DEFAULTS` baseline; pair with
/// `MISRA_DEFAULT_ALLOC_DEBUG_PAGE_BACKED=true` (separate meson
/// option, requires `default_alloc_debug=true`) to additionally
/// route every allocation through whole pages and PROT_NONE the
/// region on free -- any UAF read/write then traps with SIGSEGV at
/// the moment of the bug. Much higher memory cost (freed pages are
/// never reclaimed), see DebugAllocator docs.

#ifndef MISRA_STD_ALLOCATOR_DEFAULT_H
#define MISRA_STD_ALLOCATOR_DEFAULT_H

#include <Misra/Std/Allocator/Heap.h>
#include <Misra/Types.h>

#if MISRA_DEFAULT_ALLOC_DEBUG
#    include <Misra/Std/Allocator/Debug.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if MISRA_DEFAULT_ALLOC_DEBUG
    typedef DebugAllocator DefaultAllocator;
#else
typedef HeapAllocator DefaultAllocator;
#endif

#ifdef __cplusplus
}
#endif

#if MISRA_DEFAULT_ALLOC_DEBUG
#    if MISRA_DEFAULT_ALLOC_DEBUG_PAGE_BACKED
// Layer page-backed UAF detection on top of the default-debug config:
// every alloc consumes whole pages, every free PROT_NONE's the region.
// Compound-literal form so each DefaultAllocator gets its own config copy.
#        define DefaultAllocatorInit()                                                                                 \
            DebugAllocatorInitWith(((DebugAllocatorConfig) {.capture_traces     = true,                                \
                                                            .detect_overflow    = true,                                \
                                                            .retain_metadata    = true,                                \
                                                            .force_page_backing = true,                                \
                                                            .trace_depth        = 8,                                   \
                                                            .canary_bytes       = 16}))
#    else
#        define DefaultAllocatorInit() DebugAllocatorInit()
#    endif
#    define DefaultAllocatorDeinit(ptr) DebugAllocatorDeinit(ptr)
#else
#    define DefaultAllocatorInit()      HeapAllocatorInit()
#    define DefaultAllocatorDeinit(ptr) HeapAllocatorDeinit(ptr)
#endif

#endif // MISRA_STD_ALLOCATOR_DEFAULT_H
