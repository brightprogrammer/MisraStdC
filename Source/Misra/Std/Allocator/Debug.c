/// file      : std/allocator/debug.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// DebugAllocator implementation. The struct is init-by-value -- it
/// owns its `heap` / `meta` / `page` allocators inline and lazily
/// binds the internal Map's allocator pointer to `&self->meta.base`
/// on first use (the compound literal doesn't know the struct's
/// final address yet).

#include <Misra/Std/Allocator/Debug.h>

#include <Misra/Std.h>
#include <Misra/Std/Container/Map.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Memory.h>
#include <Misra/Sys/Backtrace.h>

// Per-thread unique ID. Returns the TCB self-pointer via one register
// read on the platforms where the ABI exposes it directly -- no libc,
// no TLS bootstrap. Other platforms fall back to a TLS-marker byte
// whose address is per-thread.
//
// macOS: avoid `__thread`. On Darwin that triggers a `__tlv_bootstrap`
// reference against libSystem, which the Bin/-tools libc-diet gate
// (Mac CI) rejects -- the allowed-set is `__dyld_get_image_*` only.

#if PLATFORM_LINUX && (ARCHITECTURE_X86_64 || ARCHITECTURE_AARCH64)

u64 debug_current_tid(void) {
    u64 tp;
#    if ARCHITECTURE_X86_64
    __asm__ volatile("mov %%fs:0, %0"
                     : "=r"(tp));
#    else // aarch64
    __asm__ volatile("mrs %0, tpidr_el0"
                     : "=r"(tp));
#    endif
    return tp;
}

#elif PLATFORM_DARWIN && (ARCHITECTURE_X86_64 || ARCHITECTURE_AARCH64)

u64 debug_current_tid(void) {
    u64 tp;
#    if ARCHITECTURE_X86_64
    // macOS x86_64: thread-local-storage base is reachable via GS.
    __asm__ volatile("mov %%gs:0, %0"
                     : "=r"(tp));
#    else // aarch64
    // macOS aarch64: thread pointer in TPIDRRO_EL0 (read-only).
    __asm__ volatile("mrs %0, tpidrro_el0"
                     : "=r"(tp));
#    endif
    return tp;
}

#else

#    if defined(_MSC_VER)
#        define TLS_STORAGE __declspec(thread)
#    else
#        define TLS_STORAGE __thread
#    endif

static TLS_STORAGE u8 g_thread_marker;

u64 debug_current_tid(void) {
    return (u64)&g_thread_marker;
}

#endif

// ---------------------------------------------------------------------------
// Hash / compare for void* keys (extern: the Init macro stamps these
// into the embedded Map's compound literal).
// ---------------------------------------------------------------------------

u64 debug_ptr_hash(const void *data, u32 size) {
    (void)size;
    u64 x  = (u64)(*(void *const *)data);
    x     ^= x >> 30;
    x     *= 0xbf58476d1ce4e5b9ULL;
    x     ^= x >> 27;
    x     *= 0x94d049bb133111ebULL;
    x     ^= x >> 31;
    return x;
}

i32 debug_ptr_compare(const void *lhs, const void *rhs) {
    void *a = *(void *const *)lhs;
    void *b = *(void *const *)rhs;
    if ((u64)a < (u64)b)
        return -1;
    if ((u64)a > (u64)b)
        return 1;
    return 0;
}

// ---------------------------------------------------------------------------
// Canary helpers
// ---------------------------------------------------------------------------

static const u8 DEBUG_CANARY_PATTERN[4] = {0xc1, 0xd2, 0xe3, 0xf4};

static void debug_write_canary(u8 *trail, size n) {
    for (size i = 0; i < n; ++i) {
        trail[i] = DEBUG_CANARY_PATTERN[i & 3];
    }
}

static bool debug_check_canary(const u8 *trail, size n) {
    for (size i = 0; i < n; ++i) {
        if (trail[i] != DEBUG_CANARY_PATTERN[i & 3])
            return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// Validation + lazy Map allocator-pointer rebind. The Map's
// `.allocator` field is NULL inside the compound literal because the
// final struct address isn't known yet. We fix it up here on every
// entry, against the now-stable `self`.
// ---------------------------------------------------------------------------

static void debug_validate_self(const DebugAllocator *self) {
    if (!self) {
        LOG_FATAL("DebugAllocator: NULL self");
    }
    if (self->base.__magic != DEBUG_ALLOCATOR_MAGIC) {
        LOG_FATAL("type-confusion: allocator passed to debug_allocator_* is not a DebugAllocator");
    }
    if (!self->base.allocate || !self->base.resize || !self->base.remap || !self->base.deallocate) {
        LOG_FATAL("DebugAllocator: vtable function pointer is NULL");
    }
    if (self->base.alignment == 0 || (self->base.alignment & (self->base.alignment - 1)) != 0) {
        LOG_FATAL("DebugAllocator: alignment {} is not a positive power of two", (u64)self->base.alignment);
    }
    // Embedded backing allocators must themselves be sane. We don't
    // call their full validators here (they have side effects via
    // _validate_self in their own dispatch) but the magic check is
    // already strong enough to catch corruption.
    // Mask HEAP_MAGIC_VALIDATED_BIT before comparing -- HeapAllocator's
    // validator caches its deep-check result in that bit (see Heap.h).
    // The bit is part of the cache state, not the allocator's identity.
    if ((self->heap.base.__magic & ~HEAP_MAGIC_VALIDATED_BIT) != HEAP_ALLOCATOR_MAGIC) {
        LOG_FATAL("DebugAllocator: embedded heap has bad magic");
    }
    if ((self->meta.base.__magic & ~HEAP_MAGIC_VALIDATED_BIT) != HEAP_ALLOCATOR_MAGIC) {
        LOG_FATAL("DebugAllocator: embedded meta has bad magic");
    }
    if (self->page.base.__magic != PAGE_ALLOCATOR_MAGIC) {
        LOG_FATAL("DebugAllocator: embedded page has bad magic");
    }
    if (self->live.__magic != MAP_MAGIC) {
        LOG_FATAL("DebugAllocator: live map has bad magic");
    }
    if (self->freed.__magic != VEC_MAGIC) {
        LOG_FATAL("DebugAllocator: freed vec has bad magic");
    }
    // bytes_in_use must be consistent with live: if live is non-empty
    // bytes_in_use is non-zero; conversely zero live entries means
    // every byte has been returned.
    if (MapPairCount(&self->live) == 0 && self->bytes_in_use != 0) {
        LOG_FATAL("DebugAllocator: bytes_in_use {} with no live records", (u64)self->bytes_in_use);
    }
    u64 cur_tid = debug_current_tid();
    if (self->creator_tid != cur_tid) {
        LOG_FATAL(
            "DebugAllocator: cross-thread use detected (created on {x}, called from {x}). "
            "Each thread must use its own DebugAllocator instance.",
            self->creator_tid,
            cur_tid
        );
    }
    // intentional bypass: Debug allocator swap; no public MapSetAllocator mutator.
    // Cast away const to write through the lazy-bind fields; the mutation is
    // observably a no-op once bound (writes the same pointer on every entry
    // after the first). The underlying storage is the allocator's own map/vec
    // fields which it owns.
    if (!self->live.allocator) {
        ((DebugAllocator *)(void *)self)->live.allocator = ALLOCATOR_OF(&((DebugAllocator *)(void *)self)->meta);
    }
    if (!self->freed.allocator) {
        ((DebugAllocator *)(void *)self)->freed.allocator = ALLOCATOR_OF(&((DebugAllocator *)(void *)self)->meta);
    }
}

// ---------------------------------------------------------------------------
// Freed history. Unbounded Vec append on every successful free.
// On a free that isn't in the live map (the underlying Heap is about
// to LOG_FATAL with a double-free / foreign-ptr diagnostic), scan
// this Vec for the ptr to emit the original alloc + first-free
// traces before Heap aborts.
//
// Memory grows with lifetime free count. Set track_freed_history =
// false in workloads that allocate/free in the millions and don't
// want the bookkeeping cost.

static const DebugFreedEntry *debug_freed_find(const DebugAllocator *dbg, void *ptr) {
    for (u32 i = 0; i < VecLen(&dbg->freed); i++) {
        if (VecPtrAt(&dbg->freed, i)->ptr == ptr)
            return VecPtrAt(&dbg->freed, i);
    }
    return NULL;
}

// ---------------------------------------------------------------------------
// allocate / reallocate / deallocate
// ---------------------------------------------------------------------------

void *debug_allocator_allocate(DebugAllocator *self, size bytes, i8 zeroed) {
    debug_validate_self(self);
    if (!bytes)
        return NULL;

    size canary = self->config.detect_overflow ? self->config.canary_bytes : 0;
    size padded = bytes + canary;
    // force_page_backing routes through the embedded PageAllocator;
    // normal mode through the embedded HeapAllocator. Branching on the
    // typed call directly keeps both arms on the typed-dispatch path
    // (no upcast to `Allocator *`, no AllocatorAlloc_dyn).
    void *user_p = self->config.force_page_backing
                       ? AllocatorAlloc(&self->page, padded, zeroed)
                       : AllocatorAlloc(&self->heap, padded, zeroed);
    if (!user_p)
        return NULL;

    if (canary)
        debug_write_canary((u8 *)user_p + bytes, canary);

    DebugRecord rec;
    MemSet(&rec, 0, sizeof(rec));
    rec.requested_size = bytes;
    rec.padded_size    = padded;
    if (self->config.capture_traces && self->config.trace_depth > 0) {
        u32 depth = self->config.trace_depth;
        if (depth > DEBUG_ALLOCATOR_MAX_TRACE)
            depth = DEBUG_ALLOCATOR_MAX_TRACE;
        rec.alloc_trace_n = (u32)CaptureStackTrace(rec.alloc_trace, depth, 2);
    }

    if (!MapInsertR(&self->live, user_p, rec)) {
        if (self->config.force_page_backing) AllocatorFree(&self->page, user_p);
        else                                 AllocatorFree(&self->heap, user_p);
        LOG_ERROR("DebugAllocator: failed to record allocation in live map");
        return NULL;
    }
    self->bytes_in_use += (u64)bytes;
    return user_p;
}

static void debug_emit_trace(const StackFrame *frames, size count, Zstr label, Allocator *meta) {
    if (!count) {
        LOG_ERROR("    {} trace: (none captured)", label);
        return;
    }
    Str rendered = StrInit(meta);
#if !defined(LOG_NO_BACKTRACE) || !LOG_NO_BACKTRACE
    FormatStackTrace(&rendered, frames, count, meta);
#else
    // Same rationale as LogWrite's FATAL gate (see Log.c). Symbolicating
    // every leak/double-free trace costs ~10 s per call on macOS under
    // ASan because MachoCache re-parses Mach-O + dSYM + DWARF per
    // backtrace. The test harness (which is the only caller of this
    // path that opts into LOG_NO_BACKTRACE) only checks that
    // traces were captured (`trace_n > 0`), not that they symbolicate,
    // so emit raw IPs in this build.
    for (size i = 0; i < count; ++i) {
        StrAppendFmt(&rendered, "  #{} {x}\n", (u32)i, (u64)frames[i].ip);
    }
#endif
    LOG_ERROR("    {} trace:\n{}", label, rendered);
    StrDeinit(&rendered);
}

// Deallocate. Memory footprint of this allocator scales with the
// number of LIVE allocations only -- no per-lifetime growth from a
// "freed" map. Double-free / foreign / misaligned are caught here
// (when freed history has the original trace) and forwarded to the
// underlying HeapAllocator (which LOG_FATALs with its bitmap-state
// diagnostic for the cases freed history can't recognize).
size debug_allocator_deallocate(DebugAllocator *self, void *ptr) {
    debug_validate_self(self);
    if (!ptr)
        return 0;

    DebugRecord *live_rec = MapGetFirstPtr(&self->live, ptr);

    if (!live_rec) {
        // Pointer is not in our live set. Scan the freed-history Vec
        // and emit the original alloc + first-free traces if we have
        // them, then abort -- the freed-history hit is conclusive
        // evidence of a double-free.
        const DebugFreedEntry *fe = debug_freed_find(self, ptr);
        if (fe) {
            LOG_ERROR(
                "DebugAllocator: DOUBLE FREE of {x} (originally {} bytes); original alloc + first-free traces:",
                (u64)ptr,
                (u64)fe->requested_size
            );
            debug_emit_trace(fe->alloc_trace, fe->alloc_trace_n, "alloc", ALLOCATOR_OF(&self->meta));
            debug_emit_trace(fe->free_trace, fe->free_trace_n, "first-free", ALLOCATOR_OF(&self->meta));
            LOG_FATAL("DebugAllocator: double-free of {x}", (u64)ptr);
            return 0;
        }
        // No freed-history hit either: foreign pointer (or freed
        // history disabled). Forward to the underlying allocator and
        // let its state-machine diagnostic fire -- Heap LOG_FATALs
        // with "foreign ptr"; PageAllocator does the same via its
        // entries-table lookup.
        if (self->config.force_page_backing) AllocatorFree(&self->page, ptr);
        else                                 AllocatorFree(&self->heap, ptr);
        return 0;
    }

    if (self->config.detect_overflow && self->config.canary_bytes) {
        const u8 *trail = (const u8 *)ptr + live_rec->requested_size;
        if (!debug_check_canary(trail, self->config.canary_bytes)) {
            self->overflows += 1;
            LOG_ERROR(
                "DebugAllocator: BUFFER OVERFLOW past {x} ({} bytes requested)",
                (u64)ptr,
                (u64)live_rec->requested_size
            );
            debug_emit_trace(live_rec->alloc_trace, live_rec->alloc_trace_n, "alloc", ALLOCATOR_OF(&self->meta));
        }
    }

    // Append to freed history BEFORE removing from live -- captures
    // alloc_trace from live_rec and a fresh free_trace from this
    // call site. Unbounded; gated by track_freed_history config.
    if (self->config.track_freed_history) {
        DebugFreedEntry entry;
        entry.ptr            = ptr;
        entry.requested_size = live_rec->requested_size;
        entry.alloc_trace_n  = live_rec->alloc_trace_n;
        MemCopy(entry.alloc_trace, live_rec->alloc_trace, (size)live_rec->alloc_trace_n * sizeof(StackFrame));
        entry.free_trace_n = 0;
        if (self->config.capture_traces && self->config.trace_depth > 0) {
            u32 depth = self->config.trace_depth;
            if (depth > DEBUG_ALLOCATOR_MAX_TRACE)
                depth = DEBUG_ALLOCATOR_MAX_TRACE;
            entry.free_trace_n = (u32)CaptureStackTrace(entry.free_trace, depth, 3);
        }
        VecPushBack(&self->freed, entry);
    }

    size requested      = live_rec->requested_size;
    size padded         = live_rec->padded_size;
    self->bytes_in_use -= (u64)requested;
    MapRemoveFirst(&self->live, ptr);

    if (self->config.force_page_backing) {
        // Don't release the page. mprotect it PROT_NONE so any UAF
        // read/write traps with SIGSEGV at the moment of the bug.
        size page_size = PageAllocatorPageSize(&self->page);
        size rounded   = (padded + page_size - 1) & ~(page_size - 1);
        if (!PageProtect(ptr, rounded, PAGE_PROT_NONE)) {
            LOG_ERROR("DebugAllocator: PageProtect(PROT_NONE) failed on {x}", (u64)ptr);
        }
    } else {
        // The outer `if (force_page_backing)` already handled the
        // page-backed branch; here force_page_backing is false, so the
        // backing is unconditionally the embedded heap.
        AllocatorFree(&self->heap, ptr);
    }
    return requested;
}

// In-place resize: always refused. The debug allocator keeps a
// canary on every allocation + a live-map keyed by pointer; trying
// to resize in place would mean re-stamping the canary, updating
// the live-map entry's recorded size, and (in page-backed mode)
// potentially remapping pages -- none of that is "in place" in any
// useful sense. Refuse and force the caller through remap, which
// does the clean alloc-fresh + copy + free dance with full canary +
// live-map maintenance.
i8 debug_allocator_resize(DebugAllocator *self, void *ptr, size new_size) {
    debug_validate_self(self);
    (void)ptr;
    (void)new_size;
    return 0;
}

void *debug_allocator_remap(DebugAllocator *self, void *ptr, size new_size) {
    debug_validate_self(self);
    if (new_size == 0) {
        debug_allocator_deallocate(self, ptr);
        return NULL;
    }
    if (!ptr) {
        return debug_allocator_allocate(self, new_size, false);
    }
    // Look up the original requested size from the live map to bound
    // the copy. If ptr is not in the live map, forward to deallocate
    // which emits the double-free / foreign-ptr diagnostic and aborts.
    DebugRecord *rec = MapGetFirstPtr(&self->live, ptr);
    if (!rec) {
        debug_allocator_deallocate(self, ptr); // aborts
        return NULL;
    }
    size old_requested = rec->requested_size;
    // alloc-fresh + `MemCopy` + free, keeping canary + record
    // invariants simple. The cost in debug mode is fine.
    void *fresh = debug_allocator_allocate(self, new_size, false);
    if (!fresh)
        return NULL;
    size copy = old_requested < new_size ? old_requested : new_size;
    MemCopy(fresh, ptr, copy);
    debug_allocator_deallocate(self, ptr);
    return fresh;
}

// ---------------------------------------------------------------------------
// Deinit
// ---------------------------------------------------------------------------

void DebugAllocatorDeinit(DebugAllocator *self) {
    if (!self || self->base.__magic != DEBUG_ALLOCATOR_MAGIC)
        return;

    u64 cur_tid = debug_current_tid();
    if (self->creator_tid != cur_tid) {
        LOG_FATAL(
            "DebugAllocator: Deinit called from a different thread (created on {x}, called from {x}).",
            self->creator_tid,
            cur_tid
        );
    }

    // Report leaks for anything still in `live`.
    if (MapAllocator(&self->live) && MapPairCount(&self->live) > 0) {
        LOG_ERROR("DebugAllocator: {} live allocation(s) at deinit time:", (u64)MapPairCount(&self->live));
        MapForeachPairPtr(&self->live, key_ptr, val_ptr) {
            LOG_ERROR("  leaked {x} ({} bytes)", (u64)*key_ptr, (u64)val_ptr->requested_size);
            debug_emit_trace(val_ptr->alloc_trace, val_ptr->alloc_trace_n, "alloc", ALLOCATOR_OF(&self->meta));
        }
    }

    // Tear down the live map (bucket storage from `&self->meta`).
    // force_page_backing'd allocations are NOT released here -- they
    // remain mprotected and the pages stay mapped until process exit.
    // Acceptable: force_page_backing is opt-in for short-running tests
    // / fuzz, the memory cost is the documented trade-off.
    if (self->live.allocator)
        MapDeinit(&self->live);
    if (self->freed.allocator)
        VecDeinit(&self->freed);

    HeapAllocatorDeinit(&self->meta);
    HeapAllocatorDeinit(&self->heap);
    // PageAllocatorDeinit is deliberately NOT called. When
    // force_page_backing is true, allocs were never returned via
    // page_allocator_deallocate -- they were PROT_NONE'd in place as
    // UAF traps. Calling Deinit would unmap those traps. The
    // descriptor-table mmaps inside `self->page` are intentionally
    // leaked along with the trapped pages; this is the documented
    // trade-off of the mode. When force_page_backing is false
    // `self->page` was never used and has no live state to release.

    MemSet(self, 0, sizeof(*self));
}

// ---------------------------------------------------------------------------
// Public query / report API
// ---------------------------------------------------------------------------

size DebugAllocatorLiveCount(const DebugAllocator *self) {
    if (!self)
        return 0;
    return (size)MapPairCount(&self->live);
}

size DebugAllocatorLiveBytes(const DebugAllocator *self) {
    if (!self)
        return 0;
    return (size)self->bytes_in_use;
}

size DebugAllocatorOverflows(const DebugAllocator *self) {
    if (!self)
        return 0;
    return (size)self->overflows;
}

size DebugAllocatorFreedCount(const DebugAllocator *self) {
    if (!self)
        return 0;
    return VecLen(&self->freed);
}

void DebugAllocatorReportLeaks(DebugAllocator *self, Str *out) {
    if (!self || !out)
        return;
    if (MapPairCount(&self->live) == 0)
        return;

    StrAppendFmt(out, "DebugAllocator: {} live allocation(s):\n", (u64)MapPairCount(&self->live));
    MapForeachPairPtr(&self->live, key_ptr, val_ptr) {
        StrAppendFmt(out, "  leak: {x} ({} bytes)\n", (u64)*key_ptr, (u64)val_ptr->requested_size);
        if (val_ptr->alloc_trace_n > 0) {
#if !defined(LOG_NO_BACKTRACE) || !LOG_NO_BACKTRACE
            FormatStackTrace(out, val_ptr->alloc_trace, val_ptr->alloc_trace_n, ALLOCATOR_OF(&self->meta));
#else
            for (size i = 0; i < val_ptr->alloc_trace_n; ++i) {
                StrAppendFmt(out, "  #{} {x}\n", (u32)i, (u64)val_ptr->alloc_trace[i].ip);
            }
#endif
        }
    }
}
