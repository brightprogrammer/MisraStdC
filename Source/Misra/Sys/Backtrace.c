/// file      : Backtrace.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Frame-pointer-based stack walker + SymbolResolver-driven formatter.
///
/// On x86-64 and aarch64 the ABI guarantees (with -fno-omit-frame-pointer)
/// that at every active call site:
///
///   [fp + 0] = saved FP of caller
///   [fp + 8] = saved return address (the IP in the caller)
///
/// We walk the chain by chasing `[fp + 0]` until it stops increasing
/// monotonically (the stack grows down, so each saved FP must live at
/// a higher address than the previous one).

#include <Misra/Sys/Backtrace.h>

#include <Misra/Std.h>
#include <Misra/Std/Log.h>

#include <stdint.h>

// ---------------------------------------------------------------------------
// Capture
// ---------------------------------------------------------------------------

enum {
    BACKTRACE_MAX_WALK = 256, // hard cap on walk depth to guard against loops
};

size CaptureStackTrace(StackFrame *out, size max_frames, size skip_frames) {
    if (!out || max_frames == 0) {
        return 0;
    }

    // Start one frame up so we don't include our own.
    void **fp = (void **)__builtin_frame_address(0);
    if (!fp) {
        return 0;
    }

    size   captured = 0;
    size   depth    = 0;
    void **prev_fp  = NULL;
    while (fp && depth < BACKTRACE_MAX_WALK) {
        // Sanity: fp must be 8-byte aligned and strictly above the
        // previous fp (stack grows downward).
        if ((uintptr_t)fp & 0x7u)
            break;
        if (prev_fp && (uintptr_t)fp <= (uintptr_t)prev_fp)
            break;

        void *saved_fp = fp[0];
        void *ret_addr = fp[1];
        if (!ret_addr) {
            break;
        }

        // `fp[1]` is the return address into our *caller*, so depth 0
        // already represents the caller (not us). Skip only what the
        // caller asked for.
        if (depth >= skip_frames) {
            if (captured >= max_frames)
                break;
            out[captured].ip  = ret_addr;
            captured         += 1;
        }

        prev_fp  = fp;
        fp       = (void **)saved_fp;
        depth   += 1;
    }

    return captured;
}

// ---------------------------------------------------------------------------
// Format
// ---------------------------------------------------------------------------

static const char *basename_of(const char *path) {
    if (!path)
        return "?";
    const char *slash = path;
    for (const char *p = path; *p; ++p) {
        if (*p == '/')
            slash = p + 1;
    }
    return slash;
}

static void emit_resolved_line(Str *out, u32 idx, const ResolvedSymbol *r, void *ip) {
    if (r->symbol_name) {
        const char *mod = basename_of(r->module_path);
        StrWriteFmt(out, "  #{} {}!{}+{x} [{x}]", idx, mod, r->symbol_name, r->offset, (u64)(uintptr_t)ip);
    } else if (r->module_path) {
        const char *mod = basename_of(r->module_path);
        StrWriteFmt(out, "  #{} {}+{x} [{x}]", idx, mod, r->offset, (u64)(uintptr_t)ip);
    } else {
        StrWriteFmt(out, "  #{} {x}", idx, (u64)(uintptr_t)ip);
    }

    if (r->source_file) {
        const char *file = basename_of(r->source_file);
        if (r->source_line > 0) {
            StrWriteFmt(out, " ({}:{})", file, r->source_line);
        } else {
            StrWriteFmt(out, " ({})", file);
        }
    }
    StrPushBack(out, '\n');
}

void FormatStackTraceWith(Str *out, const StackFrame *frames, size count, SymbolResolver *resolver) {
    if (!out || !frames || !resolver)
        return;
    for (size i = 0; i < count; ++i) {
        ResolvedSymbol r;
        if (SymbolResolverResolve(resolver, frames[i].ip, &r)) {
            emit_resolved_line(out, (u32)i, &r, frames[i].ip);
        } else {
            StrWriteFmt(out, "  #{} {x}\n", (u32)i, (u64)(uintptr_t)frames[i].ip);
        }
    }
}

void FormatStackTrace(Str *out, const StackFrame *frames, size count, Allocator *alloc) {
    if (!out || !frames || !alloc)
        return;
    SymbolResolver res;
    if (!SymbolResolverInit(&res, alloc)) {
        // Fall back to plain IP dump.
        for (size i = 0; i < count; ++i) {
            StrWriteFmt(out, "  #{} {x}\n", (u32)i, (u64)(uintptr_t)frames[i].ip);
        }
        return;
    }
    FormatStackTraceWith(out, frames, count, &res);
    SymbolResolverDeinit(&res);
}
