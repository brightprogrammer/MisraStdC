/// file      : std/allocator/debug.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// DebugAllocator implementation. Per-allocation records are kept in
/// `Map(void*, DebugRecord)` instances backed by a separate "meta"
/// allocator so that the bookkeeping does not perturb the parent
/// allocator being audited. Each record carries the requested size,
/// alloc/free stack traces, and (optionally) a copy of the alloc-site
/// canary pattern for verification on free.

#include <Misra/Std/Allocator/Debug.h>

#include <Misra/Std.h>
#include <Misra/Std/Container/Map.h>
#include <Misra/Std/Log.h>
#include <Misra/Sys/Backtrace.h>

#include <stdint.h>

// ---------------------------------------------------------------------------
// Per-allocation record
// ---------------------------------------------------------------------------

typedef struct DebugRecord {
    size       requested_size;
    size       padded_size; // requested + canary_bytes
    u32        alloc_trace_n;
    u32        free_trace_n;
    bool       freed;
    StackFrame alloc_trace[DEBUG_ALLOCATOR_MAX_TRACE];
    StackFrame free_trace[DEBUG_ALLOCATOR_MAX_TRACE];
} DebugRecord;

typedef Map(void *, DebugRecord) DebugRecordMap;

// ---------------------------------------------------------------------------
// DebugAllocator state
// ---------------------------------------------------------------------------

struct DebugAllocator {
    Allocator            base;
    Allocator           *parent;
    Allocator           *meta;
    DebugAllocatorConfig config;
    DebugRecordMap       live;
    DebugRecordMap       freed;
    bool                 freed_map_initialized;
    u64                  double_frees;
    u64                  overflows;
    u64                  bytes_in_use;
};

// ---------------------------------------------------------------------------
// Hash / compare for void* keys
// ---------------------------------------------------------------------------

static u64 debug_ptr_hash(const void *data, u32 size) {
    (void)size;
    // splitmix-style scramble on the pointer's integer representation.
    u64 x  = (u64)(uintptr_t)(*(void *const *)data);
    x     ^= x >> 30;
    x     *= 0xbf58476d1ce4e5b9ULL;
    x     ^= x >> 27;
    x     *= 0x94d049bb133111ebULL;
    x     ^= x >> 31;
    return x;
}

static i32 debug_ptr_compare(const void *lhs, const void *rhs) {
    void *a = *(void *const *)lhs;
    void *b = *(void *const *)rhs;
    if ((uintptr_t)a < (uintptr_t)b)
        return -1;
    if ((uintptr_t)a > (uintptr_t)b)
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
        if (trail[i] != DEBUG_CANARY_PATTERN[i & 3]) {
            return false;
        }
    }
    return true;
}

// ---------------------------------------------------------------------------
// Validation
// ---------------------------------------------------------------------------

static DebugAllocator *debug_validate_self(const Allocator *self) {
    if (!self || self->__magic != MISRA_DEBUG_ALLOCATOR_MAGIC) {
        LOG_FATAL("type-confusion: allocator passed to debug_allocator_* is not a DebugAllocator");
    }
    return (DebugAllocator *)self;
}

// ---------------------------------------------------------------------------
// allocate / reallocate / deallocate
// ---------------------------------------------------------------------------

void *debug_allocator_allocate(Allocator *self, size bytes, i8 zeroed) {
    DebugAllocator *dbg = debug_validate_self(self);
    if (!bytes) {
        return NULL;
    }

    size  canary = dbg->config.detect_overflow ? dbg->config.canary_bytes : 0;
    size  padded = bytes + canary;
    void *user_p = AllocatorAlloc(dbg->parent, padded, zeroed);
    if (!user_p) {
        return NULL;
    }

    if (canary) {
        debug_write_canary((u8 *)user_p + bytes, canary);
    }

    DebugRecord rec;
    MemSet(&rec, 0, sizeof(rec));
    rec.requested_size = bytes;
    rec.padded_size    = padded;
    if (dbg->config.capture_traces && dbg->config.trace_depth > 0) {
        u32 depth = dbg->config.trace_depth;
        if (depth > DEBUG_ALLOCATOR_MAX_TRACE)
            depth = DEBUG_ALLOCATOR_MAX_TRACE;
        rec.alloc_trace_n = (u32)CaptureStackTrace(rec.alloc_trace, depth, 2);
    }

    if (!MapInsertR(&dbg->live, user_p, rec)) {
        AllocatorFree(dbg->parent, user_p, padded);
        LOG_ERROR("DebugAllocator: failed to record allocation in live map");
        return NULL;
    }

    dbg->bytes_in_use += (u64)bytes;
    return user_p;
}

static void debug_emit_trace(const StackFrame *frames, size count, const char *label, Allocator *meta) {
    if (!count) {
        LOG_ERROR("    {} trace: (none captured)", label);
        return;
    }
    Str rendered = StrInit(meta);
    FormatStackTrace(&rendered, frames, count, meta);
    LOG_ERROR("    {} trace:\n{}", label, rendered);
    StrDeinit(&rendered);
}

static void debug_record_free_trace(DebugAllocator *dbg, DebugRecord *rec) {
    if (dbg->config.capture_traces && dbg->config.trace_depth > 0) {
        u32 depth = dbg->config.trace_depth;
        if (depth > DEBUG_ALLOCATOR_MAX_TRACE)
            depth = DEBUG_ALLOCATOR_MAX_TRACE;
        rec->free_trace_n = (u32)CaptureStackTrace(rec->free_trace, depth, 3);
    }
    rec->freed = true;
}

void debug_allocator_deallocate(Allocator *self, void *ptr, size bytes) {
    DebugAllocator *dbg = debug_validate_self(self);
    if (!ptr) {
        return;
    }

    DebugRecord *live_rec = MapGetFirstPtr(&dbg->live, ptr);
    if (!live_rec) {
        if (dbg->config.retain_metadata && dbg->freed_map_initialized) {
            DebugRecord *freed_rec = MapGetFirstPtr(&dbg->freed, ptr);
            if (freed_rec) {
                dbg->double_frees += 1;
                LOG_ERROR(
                    "DebugAllocator: DOUBLE FREE of {x} (originally {} bytes)",
                    (u64)(uintptr_t)ptr,
                    (u64)freed_rec->requested_size
                );
                debug_emit_trace(freed_rec->alloc_trace, freed_rec->alloc_trace_n, "alloc", dbg->meta);
                debug_emit_trace(freed_rec->free_trace, freed_rec->free_trace_n, "first-free", dbg->meta);
                return;
            }
        }
        LOG_ERROR("DebugAllocator: free of unknown pointer {x}", (u64)(uintptr_t)ptr);
        return;
    }

    if (bytes && bytes != live_rec->requested_size) {
        LOG_ERROR(
            "DebugAllocator: size mismatch on free of {x} (claimed {} bytes, "
            "tracked {} bytes)",
            (u64)(uintptr_t)ptr,
            (u64)bytes,
            (u64)live_rec->requested_size
        );
    }

    if (dbg->config.detect_overflow && dbg->config.canary_bytes) {
        const u8 *trail = (const u8 *)ptr + live_rec->requested_size;
        if (!debug_check_canary(trail, dbg->config.canary_bytes)) {
            dbg->overflows += 1;
            LOG_ERROR(
                "DebugAllocator: BUFFER OVERFLOW past {x} ({} bytes "
                "requested)",
                (u64)(uintptr_t)ptr,
                (u64)live_rec->requested_size
            );
            debug_emit_trace(live_rec->alloc_trace, live_rec->alloc_trace_n, "alloc", dbg->meta);
        }
    }

    DebugRecord record = *live_rec;
    debug_record_free_trace(dbg, &record);

    dbg->bytes_in_use -= (u64)live_rec->requested_size;

    size padded = live_rec->padded_size;
    MapRemoveFirst(&dbg->live, ptr);

    if (dbg->config.retain_metadata) {
        if (!dbg->freed_map_initialized) {
            DebugRecordMap fresh       = MapInit(debug_ptr_hash, debug_ptr_compare, dbg->meta);
            dbg->freed                 = fresh;
            dbg->freed_map_initialized = true;
        }
        MapInsertR(&dbg->freed, ptr, record);
    }

    AllocatorFree(dbg->parent, ptr, padded);
}

void *debug_allocator_reallocate(Allocator *self, void *ptr, size old_size, size new_size) {
    DebugAllocator *dbg = debug_validate_self(self);

    if (new_size == 0) {
        debug_allocator_deallocate(self, ptr, old_size);
        return NULL;
    }
    if (!ptr) {
        return debug_allocator_allocate(self, new_size, false);
    }

    // We always alloc fresh + copy + free, even when the parent could
    // realloc in place — keeping the canary and metadata invariants
    // straightforward is worth the cost in debug mode.
    void *fresh = debug_allocator_allocate(self, new_size, false);
    if (!fresh) {
        return NULL;
    }
    size copy = old_size < new_size ? old_size : new_size;
    MemCopy(fresh, ptr, copy);
    debug_allocator_deallocate(self, ptr, old_size);
    (void)dbg;
    return fresh;
}

// ---------------------------------------------------------------------------
// Create / Destroy
// ---------------------------------------------------------------------------

DebugAllocator *DebugAllocatorCreateWith(Allocator *parent, Allocator *meta_alloc, DebugAllocatorConfig config) {
    if (!parent || !meta_alloc) {
        LOG_ERROR("DebugAllocatorCreate: parent and meta_alloc are required");
        return NULL;
    }

    DebugAllocator *dbg = (DebugAllocator *)AllocatorAlloc(meta_alloc, sizeof(DebugAllocator), true);
    if (!dbg) {
        LOG_ERROR("DebugAllocatorCreate: handle allocation failed");
        return NULL;
    }

    dbg->base = (Allocator) {
        .allocate    = debug_allocator_allocate,
        .reallocate  = debug_allocator_reallocate,
        .deallocate  = debug_allocator_deallocate,
        .alignment   = parent->alignment,
        .effort      = parent->effort,
        .retry_limit = parent->retry_limit,
        .__magic     = MISRA_DEBUG_ALLOCATOR_MAGIC,
    };
    dbg->parent                = parent;
    dbg->meta                  = meta_alloc;
    dbg->config                = config;
    dbg->freed_map_initialized = false;
    dbg->double_frees          = 0;
    dbg->overflows             = 0;
    dbg->bytes_in_use          = 0;
    {
        DebugRecordMap fresh = MapInit(debug_ptr_hash, debug_ptr_compare, meta_alloc);
        dbg->live            = fresh;
    }

    if (config.trace_depth > DEBUG_ALLOCATOR_MAX_TRACE) {
        dbg->config.trace_depth = DEBUG_ALLOCATOR_MAX_TRACE;
    }

    return dbg;
}

DebugAllocator *DebugAllocatorCreate(Allocator *parent, Allocator *meta_alloc) {
    return DebugAllocatorCreateWith(parent, meta_alloc, DEBUG_ALLOCATOR_DEFAULTS);
}

void DebugAllocatorDestroy(DebugAllocator *self, Allocator *meta_alloc) {
    if (!self) {
        return;
    }

    // Report any leaks before tearing down.
    size live_n = (size)self->live.length;
    if (live_n) {
        LOG_ERROR("DebugAllocator: {} live allocation(s) at destroy time:", (u64)live_n);
        MapForeachPairPtr(&self->live, k_ptr, v_ptr) {
            void              *user_p = *k_ptr;
            const DebugRecord *rec    = v_ptr;
            LOG_ERROR("  leaked {x} ({} bytes)", (u64)(uintptr_t)user_p, (u64)rec->requested_size);
            debug_emit_trace(rec->alloc_trace, rec->alloc_trace_n, "alloc", self->meta);
        }
    }

    MapDeinit(&self->live);
    if (self->freed_map_initialized) {
        MapDeinit(&self->freed);
    }

    MemSet(self, 0, sizeof(*self));
    AllocatorFree(meta_alloc, self, sizeof(DebugAllocator));
}

// ---------------------------------------------------------------------------
// Read-side helpers
// ---------------------------------------------------------------------------

size DebugAllocatorLiveCount(const DebugAllocator *self) {
    if (!self)
        return 0;
    return (size)self->live.length;
}

size DebugAllocatorLiveBytes(const DebugAllocator *self) {
    if (!self)
        return 0;
    return (size)self->bytes_in_use;
}

size DebugAllocatorDoubleFrees(const DebugAllocator *self) {
    if (!self)
        return 0;
    return (size)self->double_frees;
}

size DebugAllocatorOverflows(const DebugAllocator *self) {
    if (!self)
        return 0;
    return (size)self->overflows;
}

void DebugAllocatorReportLeaks(DebugAllocator *self, Str *out) {
    if (!self || !out) {
        return;
    }
    MapForeachPairPtr(&self->live, k_ptr, v_ptr) {
        void              *user_p = *k_ptr;
        const DebugRecord *rec    = v_ptr;
        StrWriteFmt(out, "leaked {x} ({} bytes)\n", (u64)(uintptr_t)user_p, (u64)rec->requested_size);
        if (rec->alloc_trace_n) {
            FormatStackTrace(out, rec->alloc_trace, rec->alloc_trace_n, self->meta);
        }
    }
}
