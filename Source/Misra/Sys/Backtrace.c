/// file      : Backtrace.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Dual-platform stack-trace capture + formatter.
///
/// Linux path: pure-Misra. Walks `__builtin_frame_address(0)` for
/// capture; routes each IP through `Sys/SymbolResolver` (`/proc/self/maps`
/// + our ELF + DWARF parsers) for formatting. No libc backtrace, no
/// dladdr.
///
/// Windows path: wraps `CaptureStackBackTrace` and dbghelp's
/// `SymFromAddr` / `SymGetLineFromAddr64`. The in-tree PE+PDB
/// equivalents are deferred (FUTURE-PLANS).

#include <Misra/Sys/Backtrace.h>

#include <Misra/Std.h>
#include <Misra/Std/Log.h>

#include <stdint.h>

// ---------------------------------------------------------------------------
// Helpers used by both backends
// ---------------------------------------------------------------------------

static const char *basename_of(const char *path) {
    if (!path)
        return "?";
    const char *slash = path;
    for (const char *p = path; *p; ++p) {
        if (*p == '/' || *p == '\\')
            slash = p + 1;
    }
    return slash;
}

// ---------------------------------------------------------------------------
// Windows backend
// ---------------------------------------------------------------------------

#ifdef _WIN32

#    include <windows.h>
#    include <dbghelp.h>

// dbghelp.lib must be on the link line; meson.build adds it.

static bool g_dbghelp_initialized = false;

static void ensure_dbghelp(void) {
    if (g_dbghelp_initialized)
        return;
    SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);
    if (SymInitialize(GetCurrentProcess(), NULL, TRUE)) {
        g_dbghelp_initialized = true;
    }
}

size CaptureStackTrace(StackFrame *out, size max_frames, size skip_frames) {
    if (!out || max_frames == 0)
        return 0;

    enum {
        SCRATCH_MAX = 256
    };
    void *raw[SCRATCH_MAX];
    ULONG cap = (ULONG)max_frames;
    if (cap > SCRATCH_MAX)
        cap = SCRATCH_MAX;
    // The +1 accounts for this function's own frame.
    ULONG n = CaptureStackBackTrace((DWORD)(skip_frames + 1), cap, raw, NULL);
    for (ULONG i = 0; i < n; ++i) {
        out[i].ip = raw[i];
    }
    return (size)n;
}

void FormatStackTrace(Str *out, const StackFrame *frames, size count, Allocator *alloc) {
    (void)alloc;
    if (!out || !frames)
        return;
    ensure_dbghelp();
    HANDLE proc = GetCurrentProcess();

    enum {
        MAX_NAME = 512
    };
    // SYMBOL_INFO contains ULONG64 fields, so its backing storage
    // needs 8-byte alignment. A plain `char[]` on the stack is not
    // guaranteed that — the resulting misaligned access trips a
    // Windows 0xC0000005. Back it with a ULONG64 array instead.
    ULONG64      sym_buf[(sizeof(SYMBOL_INFO) + MAX_NAME + sizeof(ULONG64) - 1) / sizeof(ULONG64)];
    SYMBOL_INFO *sym = (SYMBOL_INFO *)sym_buf;
    MemSet(sym, 0, sizeof(*sym));
    sym->SizeOfStruct = sizeof(SYMBOL_INFO);
    sym->MaxNameLen   = MAX_NAME;

    IMAGEHLP_LINE64 line;
    MemSet(&line, 0, sizeof(line));
    line.SizeOfStruct = sizeof(line);

    for (size i = 0; i < count; ++i) {
        DWORD64 ip      = (DWORD64)(uintptr_t)frames[i].ip;
        DWORD64 sym_off = 0;
        bool    has_sym = g_dbghelp_initialized && SymFromAddr(proc, ip, &sym_off, sym);

        if (has_sym) {
            // Cast the trailing flexible-array `CHAR Name[1]` to a
            // proper `const char *` so our `_Generic`-based IOFMT
            // picks the Zstr case instead of a fallback single-char.
            StrWriteFmt(
                out, "  #{} {}+{x} [{x}]", (u32)i, (const char *)sym->Name, (u64)sym_off, (u64)ip);
        } else {
            StrWriteFmt(out, "  #{} {x}", (u32)i, (u64)ip);
        }

        DWORD line_disp = 0;
        if (g_dbghelp_initialized && SymGetLineFromAddr64(proc, ip, &line_disp, &line) && line.FileName) {
            const char *fname = basename_of(line.FileName);
            StrWriteFmt(out, " ({}:{})", fname, (u32)line.LineNumber);
        }
        StrPushBack(out, '\n');
    }
}

#elif defined(__GNUC__) || defined(__clang__)

// ---------------------------------------------------------------------------
// Linux / GCC + Clang backend
// ---------------------------------------------------------------------------

enum {
    BACKTRACE_MAX_WALK = 256, // hard cap on walk depth to guard against loops
};

size CaptureStackTrace(StackFrame *out, size max_frames, size skip_frames) {
    if (!out || max_frames == 0) {
        return 0;
    }

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

#else
#    error "Sys/Backtrace requires Windows or GCC/Clang frame-pointer builtins"
#endif
