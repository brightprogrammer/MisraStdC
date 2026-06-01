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
/// allocate / resize / remap / deallocate / Deinit entry points.
/// Cross-thread use trips `LOG_FATAL`. The read-only query API
/// (`DebugAllocatorLiveCount` / `LiveBytes` / `Overflows` /
/// `FreedCount` / `ReportLeaks`) skips the affinity check so it
/// won't trip LOG_FATAL on cross-thread reads, but it provides no
/// synchronization against a concurrent allocate / free on the
/// owner thread -- callers that observe from a non-owner thread
/// must coordinate externally. If a workload needs mutating
/// allocator access from multiple threads, give each thread its own
/// DebugAllocator (pointers can flow across threads; only the
/// mutating calls are scoped).

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

///
/// All-checks-on `DebugAllocatorConfig` baseline. Use as the argument
/// to `DebugAllocatorInitWith` (or the default carried by
/// `DebugAllocatorInit`) when you want every diagnostic the allocator
/// offers: trace capture, canary overflow detection, freed-history
/// tracking. Page-backed UAF detection is left OFF because it makes
/// freed regions unreclaimable; opt in explicitly via
/// `DebugAllocatorInitWith` when you want it.
///
/// SUCCESS: Yields a `DebugAllocatorConfig` value with `capture_traces`,
///          `detect_overflow`, `track_freed_history` all `true`,
///          `force_page_backing` `false`, `trace_depth` = 8,
///          `canary_bytes` = 16.
/// FAILURE: Macro cannot fail.
///
/// TAGS: Allocator, Debug, Config
///
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
    // into `base`. Callers should reach them through the
    // `AllocatorAlloc` / `AllocatorResize` / `AllocatorRemap` /
    // `AllocatorFree` dispatch macros with a `DebugAllocator *` --
    // `_Generic` routes the typed pointer straight to these typed
    // entries, skipping the dyn-dispatch indirection.

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
    /// In-place resize: always refused. The debug allocator stamps a
    /// canary past every user region and keeps a live-map entry keyed
    /// by pointer; honouring an in-place resize would mean re-stamping
    /// the canary, rewriting the live record's recorded size, and (in
    /// page-backed mode) potentially remapping pages -- none of which
    /// is "in place" in any useful sense. Callers fall back to `remap`,
    /// which does the clean alloc-fresh + copy + free dance with full
    /// canary and live-map maintenance.
    ///
    /// SUCCESS: Never -- this entry point unconditionally returns 0.
    /// FAILURE: Returns 0 for every input. Aborts via `LOG_FATAL` on
    ///          cross-thread misuse (the self-validator's check fires
    ///          before the return).
    ///
    /// TAGS: Allocator, Debug, Memory, InPlace
    ///
    i8 debug_allocator_resize(DebugAllocator *self, void *ptr, size new_size);

    ///
    /// Resize with relocation allowed. Always relocates: allocates a
    /// fresh slot via the embedded heap (or page allocator under
    /// `force_page_backing`), copies `min(old_size, new_size)` bytes,
    /// then frees the old slot (which verifies the canary and appends
    /// to freed history). The live map loses the old pointer's entry
    /// and gains one for the new pointer.
    ///
    /// SUCCESS: Returns the new (moved) pointer. When `ptr` is NULL this
    ///          behaves like `debug_allocator_allocate(self, new_size,
    ///          true)` -- fresh allocations from a remap-NULL are zeroed
    ///          (same shape as the other typed allocators). When
    ///          `new_size == 0` the allocation is freed and NULL is
    ///          returned. The live map gains a new entry and loses the
    ///          old one; `bytes_in_use` reflects the new size.
    /// FAILURE: Returns NULL when the inner heap cannot serve the
    ///          new size (the old allocation is left untouched in
    ///          that case). Aborts via `LOG_FATAL` on cross-thread
    ///          misuse, canary corruption at the old pointer, or
    ///          when `ptr` is unknown to the live map.
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
    size debug_allocator_deallocate(DebugAllocator *self, void *ptr);

    ///
    /// `GenericHash` for `void *` keys. Exposed so the
    /// `DebugAllocatorInit` macro can stamp it into the embedded
    /// `live` map's compound literal -- a runtime helper cannot stand
    /// in for a struct-literal initializer.
    ///
    /// data[in] : Address of the `void *` key whose hash to compute.
    /// size[in] : Ignored; satisfies the `GenericHash` shape.
    ///
    /// SUCCESS: Returns a 64-bit avalanching hash of the pointer value.
    /// FAILURE: Cannot fail.
    ///
    /// TAGS: Allocator, Debug, Hash, Callback
    ///
    u64 debug_ptr_hash(const void *data, u32 size);

    ///
    /// `GenericCompare` for `void *` keys. Exposed for the same reason
    /// as `debug_ptr_hash` -- the Init macro needs a symbol it can name
    /// inside the embedded `live` map's compound literal.
    ///
    /// lhs[in] : Address of the left-hand `void *` key.
    /// rhs[in] : Address of the right-hand `void *` key.
    ///
    /// SUCCESS: Returns -1 / 0 / +1 reflecting the unsigned ordering of
    ///          the two pointer values.
    /// FAILURE: Cannot fail.
    ///
    /// TAGS: Allocator, Debug, Compare, Callback
    ///
    i32 debug_ptr_compare(const void *lhs, const void *rhs);

    ///
    /// Stable per-thread identifier. Read directly from the thread
    /// pointer register where the ABI exposes it (`%fs:0` on x86_64
    /// Linux, `tpidr_el0` on aarch64, ...) and falls back to the
    /// address of a TLS marker byte elsewhere. Captured by
    /// `DebugAllocatorInit` and re-checked at every entry point so a
    /// `DebugAllocator` aborts on cross-thread use instead of
    /// silently corrupting its embedded `live` / `freed` containers.
    ///
    /// SUCCESS: Returns the calling thread's identifier. Two calls
    ///          from the same thread always return the same value;
    ///          two calls from different threads always return
    ///          different values for as long as both threads are
    ///          live.
    /// FAILURE: Cannot fail.
    ///
    /// TAGS: Allocator, Debug, Thread, Identifier
    ///
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
     .__magic           = MAP_MAGIC | MAGIC_VALIDATED_BIT}

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
     .__magic     = VEC_MAGIC | MAGIC_VALIDATED_BIT}

///
/// Construct a `DebugAllocator` with caller-supplied `DebugAllocatorConfig`.
/// Use when you want to opt into `force_page_backing` or otherwise
/// tune trace depth / canary width / freed-history retention away
/// from the `DEBUG_ALLOCATOR_DEFAULTS` baseline. Captures the calling
/// thread's identifier at expansion so subsequent entry points can
/// enforce the single-threaded contract.
///
/// `_cfg` is evaluated once.
///
/// SUCCESS: Returns a fully-initialised `DebugAllocator` value with
///          the supplied config; the embedded `heap` / `meta` / `page`
///          backing allocators are initialised but lazy (no OS calls
///          yet); `creator_tid` is set to the calling thread.
/// FAILURE: Cannot fail at macro-expansion time.
///
/// TAGS: Allocator, Debug, Init
///
#define DebugAllocatorInitWith(_cfg)                                                                                   \
    ((DebugAllocator) {                                                                                                \
        .base =                                                                                                        \
            {.allocate        = (AllocatorAllocateFn)debug_allocator_allocate,                                         \
                   .resize          = (AllocatorResizeFn)debug_allocator_resize,                                             \
                   .remap           = (AllocatorRemapFn)debug_allocator_remap,                                               \
                   .deallocate      = (AllocatorDeallocateFn)debug_allocator_deallocate,                                     \
                   .alignment       = 1,                                                                                     \
                   .effort          = ALLOCATOR_EFFORT_ONCE,                                                                 \
                   .retry_limit     = 0,                                                                                     \
                   .__magic         = DEBUG_ALLOCATOR_MAGIC | MAGIC_VALIDATED_BIT,                                                                 \
                   .footprint_bytes = 0},                                                                                    \
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

///
/// Construct a `DebugAllocator` with `DEBUG_ALLOCATOR_DEFAULTS` (all
/// diagnostics on except page-backed UAF detection). The most common
/// entry point for callers that just want leak + canary + trace
/// tracking layered on top of `HeapAllocator`.
///
/// SUCCESS: Returns a fully-initialised `DebugAllocator` value with
///          the defaults baseline; same post-init state as
///          `DebugAllocatorInitWith(DEBUG_ALLOCATOR_DEFAULTS)`.
/// FAILURE: Cannot fail at macro-expansion time.
///
/// TAGS: Allocator, Debug, Init
///
#define DebugAllocatorInit() DebugAllocatorInitWith(DEBUG_ALLOCATOR_DEFAULTS)

#endif // MISRA_STD_ALLOCATOR_DEBUG_H
