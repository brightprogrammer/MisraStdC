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
///   - **Freed history (for double-free forensics)** -- every
///     successful free appends an entry to an unbounded Vec.
///     When Heap detects a double-free and aborts, DebugAllocator
///     first scans the freed history for the original alloc +
///     first-free traces and emits them. Memory grows with lifetime
///     free count (~288 B per entry); workloads that don't want
///     the cost set `track_freed_history = false`.
///   - **Double-free / foreign / misaligned / wrong-size frees** --
///     detected and aborted by the underlying `HeapAllocator`'s
///     bitmap validation. DebugAllocator adds context (original
///     traces) from the freed history when available.
///   - **Buffer-overflow detection** -- every allocation is padded
///     with `canary_bytes` of a sentinel pattern immediately after
///     the user region; the canary is verified on free.
///   - **Stack-trace capture** -- `Sys/Backtrace`'s `CaptureStackTrace`
///     records frames on alloc + free sites.
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
#include <Misra/Std/Container/Vec.h>
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
    /// Live-allocation bookkeeping record. Stored in the embedded
    /// `live` map keyed by pointer.
    ///
    typedef struct DebugRecord {
        size       requested_size;
        size       padded_size;
        u32        alloc_trace_n;
        StackFrame alloc_trace[DEBUG_ALLOCATOR_MAX_TRACE];
    } DebugRecord;

    typedef Map(void *, DebugRecord) DebugRecordMap;

    ///
    /// Bounded-history record for a successfully-freed allocation.
    /// Kept in the `freed` Vec ring; scanned linearly when the next
    /// free can't find a live record, to emit the original alloc +
    /// first-free traces alongside Heap's double-free abort.
    ///
    typedef struct DebugFreedEntry {
        void      *ptr;
        size       requested_size;
        u32        alloc_trace_n;
        u32        free_trace_n;
        StackFrame alloc_trace[DEBUG_ALLOCATOR_MAX_TRACE];
        StackFrame free_trace[DEBUG_ALLOCATOR_MAX_TRACE];
    } DebugFreedEntry;

    typedef Vec(DebugFreedEntry) DebugFreedVec;

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
        ///
        /// Keep a record of every freed allocation (with traces) so
        /// double-free aborts can be diagnosed with the original
        /// alloc + first-free contexts. Memory grows with lifetime
        /// free count -- disable for stress workloads that do
        /// millions of allocs/frees.
        ///
        bool track_freed_history;
    } DebugAllocatorConfig;

#define DEBUG_ALLOCATOR_DEFAULTS                                                                                       \
    ((DebugAllocatorConfig) {.capture_traces      = true,                                                              \
                             .detect_overflow     = true,                                                              \
                             .force_page_backing  = false,                                                             \
                             .trace_depth         = 8,                                                                 \
                             .canary_bytes        = 16,                                                                \
                             .track_freed_history = true})

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
        DebugFreedVec        freed;
        u64                  overflows;
        u64                  bytes_in_use;
        u64                  creator_tid;
    } DebugAllocator;

    // Vtable functions, exposed because the Init macro stamps them
    // into `base`. Direct calls aren't recommended; use the
    // `Allocator *` returned by `ALLOCATOR_OF` instead.

    ///
    /// Allocate via the embedded heap (or page allocator when
    /// `force_page_backing` is set), pad with canary bytes when
    /// `detect_overflow` is set, and record the live entry in `live`
    /// with the captured allocation stack trace.
    ///
    /// SUCCESS: Returns a writable, alignment-correct pointer to at
    ///          least `bytes` user bytes. Zeroed when `zeroed` is
    ///          non-zero. The live map gains one entry; `bytes_in_use`
    ///          grows by the requested size.
    /// FAILURE: Returns NULL when the underlying heap / page allocator
    ///          fails or the live map cannot grow. Aborts via
    ///          `LOG_FATAL` on cross-thread misuse.
    ///
    /// TAGS: Allocator, Debug, Memory, Allocation
    ///
    void *debug_allocator_allocate(DebugAllocator *self, size bytes, i8 zeroed);

    ///
    /// In-place resize through the embedded heap. Verifies the canary
    /// for the existing allocation, attempts an in-place resize on the
    /// inner heap, and on success updates the live record's recorded
    /// size and re-stamps the canary at the new tail.
    ///
    /// SUCCESS: Returns 1 when the inner heap could resize in place.
    ///          The pointer stays valid for `new_size` bytes; live
    ///          bookkeeping reflects the new size.
    /// FAILURE: Returns 0 when the inner heap cannot resize without
    ///          relocating. Aborts via `LOG_FATAL` on cross-thread
    ///          misuse, canary corruption, or unknown `ptr`.
    ///
    /// TAGS: Allocator, Debug, Memory, InPlace
    ///
    i8    debug_allocator_resize(DebugAllocator *self, void *ptr, size new_size);

    ///
    /// Resize with relocation allowed. On a move, frees the old
    /// record (after canary verification + freed-history append),
    /// records a fresh live entry for the new pointer, and copies
    /// `min(old_size, new_size)` bytes across.
    ///
    /// SUCCESS: Returns the (possibly moved) pointer. The live map
    ///          gains a new entry (and loses the old one on a move);
    ///          `bytes_in_use` reflects the new size.
    /// FAILURE: Returns NULL when the inner heap cannot serve the
    ///          new size. Aborts via `LOG_FATAL` on cross-thread
    ///          misuse, canary corruption, or unknown `ptr`.
    ///
    /// TAGS: Allocator, Debug, Memory, Reallocation
    ///
    void *debug_allocator_remap(DebugAllocator *self, void *ptr, size new_size);

    ///
    /// Free an allocation. Verifies the canary (if enabled), looks up
    /// and removes the entry from `live`, appends an entry to `freed`
    /// (when `track_freed_history` is on) with the captured free-trace,
    /// then forwards to the inner heap / page allocator.
    ///
    /// SUCCESS: Returns the released user-byte count (what stats
    ///          accounting sees). `bytes_in_use` shrinks by the
    ///          recorded size.
    /// FAILURE: Aborts via `LOG_FATAL` on cross-thread misuse, canary
    ///          corruption, or when `ptr` is not in `live`. With
    ///          freed-history enabled, double-free aborts include the
    ///          original alloc + first-free traces.
    ///
    /// TAGS: Allocator, Debug, Memory, Deallocation
    ///
    size  debug_allocator_deallocate(DebugAllocator *self, void *ptr);

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
    /// `page`-managed pages) and clears the struct. Enforces the
    /// thread-affinity check -- calling from a thread other than the
    /// one that ran `DebugAllocatorInit` aborts via `LOG_FATAL`.
    ///
    /// self[in,out] : DebugAllocator instance, or NULL / uninitialised.
    ///
    /// SUCCESS: Function returns. Any still-live allocations have been
    ///          logged with their captured traces; `heap` / `meta` are
    ///          deinitialised; the struct is fully zeroed and cannot
    ///          be used until re-initialised. `force_page_backing`'d
    ///          regions remain mprotected and stay mapped until
    ///          process exit (documented trade-off).
    /// FAILURE: No action when `self` is NULL or `__magic` does not
    ///          match `DEBUG_ALLOCATOR_MAGIC`. Cross-thread call
    ///          aborts via `LOG_FATAL`.
    ///
    /// TAGS: Allocator, Debug, Cleanup
    ///
    void DebugAllocatorDeinit(DebugAllocator *self);

    ///
    /// Number of outstanding allocations (alloc minus free), read
    /// from the embedded `live` map.
    ///
    /// self[in] : DebugAllocator instance, or NULL.
    ///
    /// SUCCESS: Returns the live-allocation count. No state is touched.
    /// FAILURE: Returns 0 when `self` is NULL.
    ///
    /// TAGS: Allocator, Debug, Observability
    ///
    size DebugAllocatorLiveCount(const DebugAllocator *self);

    ///
    /// Total user-requested bytes still outstanding, summed by the
    /// allocate / deallocate hooks into `bytes_in_use`.
    ///
    /// self[in] : DebugAllocator instance, or NULL.
    ///
    /// SUCCESS: Returns the outstanding-bytes count. No state is touched.
    /// FAILURE: Returns 0 when `self` is NULL.
    ///
    /// TAGS: Allocator, Debug, Observability
    ///
    size DebugAllocatorLiveBytes(const DebugAllocator *self);

    ///
    /// Number of canary-corruption events caught by free-time canary
    /// verification since this DebugAllocator was initialised.
    ///
    /// self[in] : DebugAllocator instance, or NULL.
    ///
    /// SUCCESS: Returns the overflow counter. No state is touched.
    /// FAILURE: Returns 0 when `self` is NULL.
    ///
    /// TAGS: Allocator, Debug, Observability
    ///
    size DebugAllocatorOverflows(const DebugAllocator *self);

    ///
    /// Number of entries currently held in the freed-history ring
    /// (only populated when `track_freed_history` is enabled in the
    /// DebugAllocator's config). Each entry carries the original
    /// alloc + first-free stack traces used by the double-free
    /// diagnostic.
    ///
    /// self[in] : DebugAllocator instance, or NULL.
    ///
    /// SUCCESS: Returns the freed-history entry count.
    /// FAILURE: Returns 0 when `self` is NULL, or when freed-history
    ///          tracking is disabled.
    ///
    /// TAGS: Allocator, Debug, Observability
    ///
    size DebugAllocatorFreedCount(const DebugAllocator *self);

    ///
    /// Append a human-readable leak report to `out`. For each entry
    /// in `live`, appends one summary line plus the captured alloc
    /// stack trace (formatted via `FormatStackTrace` when backtraces
    /// are enabled, otherwise raw instruction pointers).
    ///
    /// self[in]  : DebugAllocator instance, or NULL.
    /// out[in,out] : `Str` the report is appended to; pre-existing
    ///               contents are preserved.
    ///
    /// SUCCESS: Function returns. When `live` is non-empty, `out`
    ///          has the report appended; when `live` is empty,
    ///          `out` is left unchanged.
    /// FAILURE: No action when `self` or `out` is NULL.
    ///
    /// TAGS: Allocator, Debug, Reporting
    ///
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

///
/// Compound-literal initializer for the embedded `live` map (a
/// `Map(void *, DebugRecord)`). Stamps the hash/compare callbacks
/// (`debug_ptr_hash` / `debug_ptr_compare`), the map magic, and the
/// linear-probe policy; leaves `.allocator = NULL` so the first use
/// inside `debug_allocator_allocate` / `_deallocate` can bind it to
/// the surrounding `DebugAllocator`'s `&self->meta.base` once the
/// final struct address is known. Used internally by
/// `DebugAllocatorInitWith` -- callers should not reach for it.
///
/// SUCCESS: Yields a struct-initializer expression suitable as the
///          `.live = DEBUG_LIVE_LIT` arm of a `DebugAllocator` compound
///          literal.
/// FAILURE: Macro cannot fail at expansion.
///
/// TAGS: Allocator, Debug, Init, Internal
///
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

///
/// Compound-literal initializer for the embedded `freed` Vec (a
/// `Vec(DebugFreedEntry)` storing the freed-history ring used to
/// reconstruct double-free contexts). Stamps the vec magic and
/// leaves `.allocator = NULL` so first-use lazily binds it to
/// `&self->meta.base`, same pattern as `DEBUG_LIVE_LIT`. Used
/// internally by `DebugAllocatorInitWith` -- callers should not
/// reach for it.
///
/// SUCCESS: Yields a struct-initializer expression suitable as the
///          `.freed = DEBUG_FREED_LIT` arm of a `DebugAllocator`
///          compound literal.
/// FAILURE: Macro cannot fail at expansion.
///
/// TAGS: Allocator, Debug, Init, Internal
///
#define DEBUG_FREED_LIT                                                                                                \
    {.length      = 0,                                                                                                 \
     .capacity    = 0,                                                                                                 \
     .copy_init   = NULL,                                                                                              \
     .copy_deinit = NULL,                                                                                              \
     .data        = NULL,                                                                                              \
     .allocator   = NULL,                                                                                              \
     .__magic     = VEC_MAGIC}

#define DebugAllocatorInitWith(_cfg)                                                                                   \
    ((DebugAllocator) {                                                                                                \
        .base =                                                                                                        \
            {.allocate    = (AllocatorAllocateFn)debug_allocator_allocate,                                          \
                   .resize      = (AllocatorResizeFn)debug_allocator_resize,                                                 \
                   .remap       = (AllocatorRemapFn)debug_allocator_remap,                                                   \
                   .deallocate  = (AllocatorDeallocateFn)debug_allocator_deallocate,                                         \
                   .alignment   = 1,                                                                                         \
                   .effort      = ALLOCATOR_EFFORT_ONCE,                                                                     \
                   .retry_limit = 0,                                                                                         \
                   .__magic     = DEBUG_ALLOCATOR_MAGIC},                                                                        \
        .heap         = HeapAllocatorInit(),                                                                           \
        .meta         = HeapAllocatorInit(),                                                                           \
        .page         = PageAllocatorInit(),                                                                           \
        .config       = (_cfg),                                                                                        \
        .live         = DEBUG_LIVE_LIT,                                                                                \
        .freed        = DEBUG_FREED_LIT,                                                                               \
        .overflows    = 0,                                                                                             \
        .bytes_in_use = 0,                                                                                             \
        .creator_tid  = debug_current_tid()                                                                            \
    })

#define DebugAllocatorInit() DebugAllocatorInitWith(DEBUG_ALLOCATOR_DEFAULTS)

#endif // MISRA_STD_ALLOCATOR_DEBUG_H
