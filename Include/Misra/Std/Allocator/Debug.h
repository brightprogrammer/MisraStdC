/// file      : std/allocator/debug.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// `DebugAllocator` wraps any backing `Allocator *` and bolts on:
///
///   - **Leak detection** — every live allocation is tracked in a
///     `Map(void*, DebugRecord)`. Anything still present at
///     `DebugAllocatorDestroy` time is reported with the captured
///     allocation stack trace.
///
///   - **Double-free detection** — when `retain_metadata` is on, freed
///     records move to a `freed` map instead of being dropped. A second
///     free of the same pointer is caught and logged with *both* the
///     original alloc trace and the previous free trace.
///
///   - **Buffer-overflow detection** — every allocation is padded with
///     `canary_bytes` of a sentinel pattern immediately after the user
///     region; the canary is verified on free.
///
///   - **Stack-trace capture** — `Sys/Backtrace`'s `CaptureStackTrace`
///     records frames on every alloc and free site, so leak / double-
///     free reports point at the real culprits.
///
/// Use it as a swap-in for any other allocator in tests and fuzz
/// harnesses. The backing allocator (`parent`) handles the actual user
/// allocations; a separate `meta` allocator backs the internal Map
/// storage so DebugAllocator's bookkeeping does not perturb the
/// stats / behaviour of the allocator being audited.

#ifndef MISRA_STD_ALLOCATOR_DEBUG_H
#define MISRA_STD_ALLOCATOR_DEBUG_H

#include <Misra/Std/Allocator.h>
#include <Misra/Std/Container/Str.h>

///
/// Per-type magic for `DebugAllocator`.
///
#define MISRA_DEBUG_ALLOCATOR_MAGIC MISRA_MAKE_NEW_MAGIC_VALUE("dbgallc!")

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct DebugAllocator DebugAllocator;

    ///
    /// Runtime configuration for a `DebugAllocator`. Pass to
    /// `DebugAllocatorCreateWith`. Use `DEBUG_ALLOCATOR_DEFAULTS` for a
    /// sensible all-checks-on baseline.
    ///
    /// FIELDS:
    /// - capture_traces  : Capture a stack trace at every alloc and
    ///                     free site. Pays the unwinder cost on every
    ///                     call but gives precise leak / double-free
    ///                     attribution.
    /// - detect_overflow : Pad each user allocation with `canary_bytes`
    ///                     of sentinel bytes after the buffer; verify
    ///                     the pattern on free. Off-by-one writes past
    ///                     the user buffer are caught at the next free.
    /// - retain_metadata : Keep freed records in a separate map. Lets
    ///                     us detect double-frees on memory we've
    ///                     already given back to the parent allocator.
    /// - trace_depth     : Number of stack frames to capture per site.
    ///                     Clamped to `DEBUG_ALLOCATOR_MAX_TRACE`.
    /// - canary_bytes    : Trailing canary length in bytes. 0 disables.
    ///
    /// TAGS: Allocator, Debug, Config
    ///
    typedef struct DebugAllocatorConfig {
        bool capture_traces;
        bool detect_overflow;
        bool retain_metadata;
        u32  trace_depth;
        u32  canary_bytes;
    } DebugAllocatorConfig;

#define DEBUG_ALLOCATOR_MAX_TRACE 16

#define DEBUG_ALLOCATOR_DEFAULTS                                                                                       \
    ((DebugAllocatorConfig) {                                                                                          \
        .capture_traces  = true,                                                                                       \
        .detect_overflow = true,                                                                                       \
        .retain_metadata = true,                                                                                       \
        .trace_depth     = 8,                                                                                          \
        .canary_bytes    = 16,                                                                                         \
    })

    ///
    /// Construct a `DebugAllocator` wrapping `parent`. Returns a pointer
    /// allocated through `meta_alloc`; the same `meta_alloc` must be
    /// passed back to `DebugAllocatorDestroy` so the handle and the
    /// internal maps can be released through the same allocator.
    ///
    /// parent[in]     : Backing allocator to audit (user allocations
    ///                  flow through it).
    /// meta_alloc[in] : Allocator for the DebugAllocator's own
    ///                  bookkeeping (live / freed maps, trace storage).
    ///                  Must outlive the returned DebugAllocator.
    ///
    /// SUCCESS : Returns a configured handle ready for use through
    ///           `ALLOCATOR_OF(handle)` like any other allocator.
    /// FAILURE : Returns NULL on allocation failure inside `meta_alloc`.
    ///
    /// TAGS: Allocator, Debug, Create
    ///
    DebugAllocator *DebugAllocatorCreate(Allocator *parent, Allocator *meta_alloc);

    ///
    /// Same as `DebugAllocatorCreate` but with a custom configuration.
    ///
    DebugAllocator *DebugAllocatorCreateWith(Allocator *parent, Allocator *meta_alloc, DebugAllocatorConfig config);

    ///
    /// Tear down a `DebugAllocator`. Iterates the live map first and
    /// emits a `LOG_ERROR` for every still-live allocation, including
    /// its captured alloc trace.
    ///
    /// self[in]       : Handle returned by `DebugAllocatorCreate*`.
    /// meta_alloc[in] : Same allocator that was passed at create time.
    ///
    /// SUCCESS : Function returns. The handle is invalid after this
    ///           call; leak count is available via the `live_at_destroy`
    ///           return prior to teardown if the caller called
    ///           `DebugAllocatorLiveCount` first.
    /// FAILURE : Function cannot fail.
    ///
    /// TAGS: Allocator, Debug, Destroy
    ///
    void DebugAllocatorDestroy(DebugAllocator *self, Allocator *meta_alloc);

    ///
    /// Number of allocations currently outstanding (alloc minus free).
    ///
    size DebugAllocatorLiveCount(const DebugAllocator *self);

    ///
    /// Total user-requested bytes currently outstanding.
    ///
    size DebugAllocatorLiveBytes(const DebugAllocator *self);

    ///
    /// Number of double-free events caught and reported so far.
    ///
    size DebugAllocatorDoubleFrees(const DebugAllocator *self);

    ///
    /// Number of canary-corruption events caught and reported so far.
    ///
    size DebugAllocatorOverflows(const DebugAllocator *self);

    ///
    /// Append a human-readable leak report to `out`. One block per
    /// still-live allocation with its alloc-site stack trace. Called
    /// implicitly by `DebugAllocatorDestroy`; exposed so callers can
    /// snapshot earlier.
    ///
    /// out[out]   : Str to append to.
    /// self[in]   : DebugAllocator handle.
    ///
    /// SUCCESS : `out` is updated; nothing is freed or modified on the
    ///           DebugAllocator.
    /// FAILURE : Function cannot fail.
    ///
    /// TAGS: Allocator, Debug, Report
    ///
    void DebugAllocatorReportLeaks(DebugAllocator *self, Str *out);

    void *debug_allocator_allocate(Allocator *self, size bytes, i8 zeroed);
    void *debug_allocator_reallocate(Allocator *self, void *ptr, size old_size, size new_size);
    void  debug_allocator_deallocate(Allocator *self, void *ptr, size bytes);

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_ALLOCATOR_DEBUG_H
