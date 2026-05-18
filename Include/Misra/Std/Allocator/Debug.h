/// file      : std/allocator/debug.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// `DebugAllocator` is an init-by-value allocator that bolts leak +
/// canary overflow + alloc-site stack-trace tracking onto an
/// internally-owned `HeapAllocator`. It mirrors `HeapAllocator`'s
/// shape -- a struct literal you assign to a stack-resident value,
/// no `Create`/`Destroy` pointer dance, no globals.
///
///   - **Leak detection** -- every live allocation is tracked in an
///     embedded `Map(void*, DebugRecord)`. Anything still present at
///     `DebugAllocatorDeinit` time is reported with the captured
///     allocation stack trace. Memory cost is proportional to the
///     count of LIVE allocations, not lifetime allocations.
///   - **Double-free / foreign / misaligned / wrong-size frees** --
///     caught by the underlying `HeapAllocator`, which LOG_FATALs
///     with a backtrace. DebugAllocator used to keep a `freed` Map
///     for this and grew unboundedly under bignum-heavy workloads;
///     the new Heap's bitmap validation makes the freed Map
///     redundant.
///   - **Buffer-overflow detection** -- every allocation is padded
///     with `canary_bytes` of a sentinel pattern immediately after
///     the user region; the canary is verified on free.
///   - **Stack-trace capture** -- `Sys/Backtrace`'s `CaptureStackTrace`
///     records frames on the alloc site. Free-site traces are no
///     longer kept (Heap aborts at the free site with a fresh trace
///     when bad input is detected).
///   - **Use-after-free (force_page_backing)** -- when enabled,
///     allocations route through an internal `PageAllocator` and
///     freed regions are `mprotect(PROT_NONE)`'d so any UAF read or
///     write traps with SIGSEGV at the moment of the bug.
///
/// **Thread affinity (enforced at runtime)**: a DebugAllocator
/// instance is single-threaded. The thread that called
/// `DebugAllocatorInit` is the only thread allowed to call its
/// allocate / reallocate / deallocate / report / deinit entry points.
/// Cross-thread use trips `LOG_FATAL`. If a workload needs allocator
/// access from multiple threads, give each thread its own
/// DebugAllocator (pointers can flow across threads; only the
/// allocator calls are scoped).

#ifndef MISRA_STD_ALLOCATOR_DEBUG_H
#define MISRA_STD_ALLOCATOR_DEBUG_H

#include <Misra/Std/Allocator.h>
#include <Misra/Std/Allocator/Heap.h>
#include <Misra/Std/Allocator/Page.h>
#include <Misra/Std/Container/Map.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Sys/Backtrace.h>

///
/// Per-type magic for `DebugAllocator`.
///
#define DEBUG_ALLOCATOR_MAGIC MAKE_NEW_MAGIC_VALUE("dbgallc!")

#ifdef __cplusplus
extern "C" {
#endif

#define DEBUG_ALLOCATOR_MAX_TRACE 16

    ///
    /// Per-allocation bookkeeping record. Stored in the embedded
    /// `live` map only; freed allocations are not retained.
    ///
    typedef struct DebugRecord {
        size       requested_size;
        size       padded_size;
        u32        alloc_trace_n;
        StackFrame alloc_trace[DEBUG_ALLOCATOR_MAX_TRACE];
    } DebugRecord;

    typedef Map(void *, DebugRecord) DebugRecordMap;

    ///
    /// Runtime configuration. Use `DEBUG_ALLOCATOR_DEFAULTS` for a
    /// sensible all-checks-on baseline.
    ///
    typedef struct DebugAllocatorConfig {
        bool capture_traces;
        bool detect_overflow;
        ///
        /// Route every user allocation through the internal
        /// `PageAllocator` (one or more whole pages per alloc) and
        /// `PageProtect(PROT_NONE)` the region on free instead of
        /// returning it. Any dereference of a freed pointer traps with
        /// SIGSEGV. Costs at least one page per alloc, and freed
        /// regions are never reclaimed. Reserve for tests / fuzz
        /// harnesses.
        ///
        bool force_page_backing;
        u32  trace_depth;
        u32  canary_bytes;
    } DebugAllocatorConfig;

#define DEBUG_ALLOCATOR_DEFAULTS                                                                                       \
    ((DebugAllocatorConfig) {.capture_traces     = true,                                                               \
                             .detect_overflow    = true,                                                               \
                             .force_page_backing = false,                                                              \
                             .trace_depth        = 8,                                                                  \
                             .canary_bytes       = 16})

    ///
    /// `DebugAllocator` struct. Owns its `heap` / `meta` / `page`
    /// backing allocators inline; no external `parent` / `meta` to
    /// pass at init. Init-by-value via `DebugAllocatorInit()`.
    ///
    typedef struct DebugAllocator {
        Allocator            base;
        HeapAllocator        heap;
        HeapAllocator        meta;
        PageAllocator        page;
        DebugAllocatorConfig config;
        DebugRecordMap       live;
        u64                  overflows;
        u64                  bytes_in_use;
        u64                  creator_tid;
    } DebugAllocator;

    // Vtable functions, exposed because the Init macro stamps them
    // into `base`. Direct calls aren't recommended; use the
    // `Allocator *` returned by `ALLOCATOR_OF` instead.
    void *debug_allocator_allocate(Allocator *self, size bytes, i8 zeroed);
    i8    debug_allocator_resize(Allocator *self, void *ptr, size old_size, size new_size);
    void *debug_allocator_remap(Allocator *self, void *ptr, size old_size, size new_size);
    void  debug_allocator_deallocate(Allocator *self, void *ptr, size bytes);

    // Hash / compare callbacks for the embedded void*->DebugRecord
    // maps. Exposed so the Init macro can wire them into the Map
    // struct literals.
    u64 debug_ptr_hash(const void *data, u32 size);
    i32 debug_ptr_compare(const void *lhs, const void *rhs);

    // Stable per-thread ID via TLS-variable address. Captured by
    // `DebugAllocatorInit` for the cross-thread-use check.
    u64 debug_current_tid(void);

    ///
    /// Tear down a DebugAllocator. Iterates `live` first and emits a
    /// `LOG_ERROR` for each still-live allocation with its captured
    /// alloc trace. Releases all backing storage (`heap` / `meta` /
    /// `page`-managed pages) and clears the struct.
    ///
    void DebugAllocatorDeinit(DebugAllocator *self);

    /// Number of outstanding allocations (alloc minus free).
    size DebugAllocatorLiveCount(const DebugAllocator *self);
    /// Total user-requested bytes still outstanding.
    size DebugAllocatorLiveBytes(const DebugAllocator *self);
    /// Number of canary-corruption events caught.
    size DebugAllocatorOverflows(const DebugAllocator *self);
    /// Append a human-readable leak report to `out`.
    void DebugAllocatorReportLeaks(DebugAllocator *self, Str *out);

#ifdef __cplusplus
}
#endif

// ---------------------------------------------------------------------------
// Init macros. Construct a `DebugAllocator` value in the user's
// storage (typically a stack variable). The embedded Maps have
// `.allocator = NULL` at compound-literal time -- they're lazily
// bound to `&self->meta.base` on first use inside
// `debug_allocator_allocate` / `_deallocate`, because the compound
// literal doesn't know the struct's final address yet.
// ---------------------------------------------------------------------------

#define DEBUG_LIVE_LIT                                                                                                 \
    {.length            = 0,                                                                                           \
     .capacity          = 0,                                                                                           \
     .tombstones        = 0,                                                                                           \
     .key_copy_init     = NULL,                                                                                        \
     .key_copy_deinit   = NULL,                                                                                        \
     .value_copy_init   = NULL,                                                                                        \
     .value_copy_deinit = NULL,                                                                                        \
     .key_compare       = (GenericCompare)debug_ptr_compare,                                                           \
     .value_compare     = NULL,                                                                                        \
     .key_hash          = (GenericHash)debug_ptr_hash,                                                                 \
     .entries           = NULL,                                                                                        \
     .states            = NULL,                                                                                        \
     .policy            = MapPolicyLinear,                                                                             \
     .allocator         = NULL,                                                                                        \
     .__magic           = MAP_MAGIC}

#define DebugAllocatorInitWith(_cfg)                                                                                   \
    ((DebugAllocator) {                                                                                                \
        .base =                                                                                                        \
            {.allocate    = debug_allocator_allocate,                                                                  \
                   .resize      = debug_allocator_resize,                                                                    \
                   .remap       = debug_allocator_remap,                                                                     \
                   .deallocate  = debug_allocator_deallocate,                                                                \
                   .alignment   = 1,                                                                                         \
                   .effort      = ALLOCATOR_EFFORT_ONCE,                                                                     \
                   .retry_limit = 0,                                                                                         \
                   .__magic     = DEBUG_ALLOCATOR_MAGIC},                                                                        \
        .heap         = HeapAllocatorInit(),                                                                           \
        .meta         = HeapAllocatorInit(),                                                                           \
        .page         = PageAllocatorInit(),                                                                           \
        .config       = (_cfg),                                                                                        \
        .live         = DEBUG_LIVE_LIT,                                                                                \
        .overflows    = 0,                                                                                             \
        .bytes_in_use = 0,                                                                                             \
        .creator_tid  = debug_current_tid()                                                                            \
    })

#define DebugAllocatorInit() DebugAllocatorInitWith(DEBUG_ALLOCATOR_DEFAULTS)

#endif // MISRA_STD_ALLOCATOR_DEBUG_H
