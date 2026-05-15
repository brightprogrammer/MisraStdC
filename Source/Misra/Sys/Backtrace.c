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
#    if MISRA_HAVE_PARSER_PDB
#        include <Misra/Std/Allocator/Default.h>
#        include <Misra/Sys/PdbCache.h>
#    endif

// dbghelp.lib must be on the link line; meson.build adds it. We use
// dbghelp as a fallback when the in-tree PE+PDB chain can't satisfy a
// resolve (PDB missing from disk, GUID mismatch, etc.).

static bool g_dbghelp_initialized = false;

static void ensure_dbghelp(void) {
    if (g_dbghelp_initialized)
        return;
    SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);
    if (SymInitialize(GetCurrentProcess(), NULL, TRUE)) {
        g_dbghelp_initialized = true;
    }
}

#    if MISRA_HAVE_PARSER_PDB
// Given a runtime IP, find the loaded module's HMODULE and a usable
// path-on-disk for it. Both come straight from the Windows loader.
static bool win_module_for_ip(void *ip, char *out_path, size out_path_size, u64 *out_base) {
    HMODULE mod = NULL;
    if (!GetModuleHandleExA(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            (LPCSTR)ip,
            &mod
        )) {
        return false;
    }
    DWORD n = GetModuleFileNameA(mod, out_path, (DWORD)out_path_size);
    if (n == 0 || n >= out_path_size)
        return false;
    *out_base = (u64)(uintptr_t)mod;
    return true;
}
#    endif

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

#    if MISRA_HAVE_PARSER_PDB
    PdbCache pdb_cache;
    bool     pdb_cache_ok = alloc && PdbCacheInit(&pdb_cache, alloc);
#    else
    (void)alloc;
#    endif

    for (size i = 0; i < count; ++i) {
        DWORD64     ip       = (DWORD64)(uintptr_t)frames[i].ip;
        bool        named    = false;
        const char *sym_name = NULL;
        u32         sym_off  = 0;

#    if MISRA_HAVE_PARSER_PDB
        // First-class path: locate the PE on disk via the Windows
        // loader, then route through the in-tree PE+PDB chain. If any
        // step fails we cascade to dbghelp below -- never silently
        // drop a frame.
        if (pdb_cache_ok) {
            char module_path[MAX_PATH];
            u64  module_base = 0;
            if (win_module_for_ip(frames[i].ip, module_path, sizeof(module_path), &module_base)) {
                if (PdbCacheResolve(&pdb_cache, module_path, module_base, (u64)ip, &sym_name, &sym_off)) {
                    named = true;
                }
            }
        }
#    endif

        if (!named && g_dbghelp_initialized) {
            DWORD64 d_off = 0;
            if (SymFromAddr(proc, ip, &d_off, sym)) {
                // Cast the trailing flexible-array `CHAR Name[1]` to a
                // proper `const char *` so our `_Generic`-based IOFMT
                // picks the Zstr case instead of a fallback single-char.
                sym_name = (const char *)sym->Name;
                sym_off  = (u32)d_off;
                named    = true;
            }
        }

        if (named) {
            StrWriteFmt(out, "  #{} {}+{x} [{x}]", (u32)i, sym_name, (u64)sym_off, (u64)ip);
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

#    if MISRA_HAVE_PARSER_PDB
    if (pdb_cache_ok)
        PdbCacheDeinit(&pdb_cache);
#    endif
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

// ---------------------------------------------------------------------------
// CFI-based unwinder (Linux x86-64 only in v1)
// ---------------------------------------------------------------------------
//
// x86-64 SysV ABI: DWARF register numbering puts RSP at 7, RBP at 6,
// and the return-address pseudo-register at 16. We track RSP and RBP
// across frames; other GPRs aren't needed to walk the call stack.

#    if MISRA_HAVE_SYS_SYMRESOLVE && MISRA_HAVE_PARSER_DWARF && defined(__x86_64__)

enum {
    DWARF_REG_RBP = 6,
    DWARF_REG_RSP = 7,
};

// Read u64 from a runtime address. Returns false if `addr` looks
// bogus (we can't probe page-mappings inline; this stays a NULL +
// alignment check). A wild address can still crash; see FUTURE-PLANS
// for a sigsetjmp-guarded read.
static bool read_u64_at(u64 addr, u64 *out) {
    if (addr == 0)
        return false;
    if (addr & 0x7)
        return false;
    *out = *(volatile u64 *)(uintptr_t)addr;
    return true;
}

static bool apply_cfa(const DwarfCfaRule *cfa, u64 rsp, u64 rbp, u64 *out_cfa) {
    if (cfa->kind != DWARF_CFA_RULE_REG_OFFSET)
        return false;
    u64 base;
    switch (cfa->reg) {
        case DWARF_REG_RSP :
            base = rsp;
            break;
        case DWARF_REG_RBP :
            base = rbp;
            break;
        default :
            return false; // other CFA bases not tracked in v1
    }
    *out_cfa = base + (u64)cfa->offset;
    return true;
}

// Apply a single register's rule to find its previous-frame value.
// `cfa` is the already-computed CFA. `cur_reg_value` is the register's
// value in the current frame (for SAME_VALUE and as a fallback).
static bool apply_reg_rule(const DwarfRegRule *r, u64 cfa, u64 cur_reg_value, u64 *out) {
    switch (r->kind) {
        case DWARF_REG_RULE_OFFSET :
            return read_u64_at(cfa + (u64)r->offset, out);
        case DWARF_REG_RULE_VAL_OFFSET :
            *out = cfa + (u64)r->offset;
            return true;
        case DWARF_REG_RULE_SAME_VALUE :
            *out = cur_reg_value;
            return true;
        case DWARF_REG_RULE_UNDEFINED :
        case DWARF_REG_RULE_REGISTER :   // would need to track all regs to honour this
        case DWARF_REG_RULE_EXPRESSION : // expression evaluator deferred
        default :
            return false;
    }
}

size CaptureStackTraceCfi(StackFrame *out, size max_frames, size skip_frames, SymbolResolver *resolver) {
    if (!out || max_frames == 0 || !resolver)
        return 0;

    // Snapshot RSP / RBP at the very top of this function. RIP is
    // the return-into-caller address obtained via the compiler
    // intrinsic. RSP at our entry points just past the return-address
    // slot the CALL pushed; the caller's CFA is therefore (our RSP) +
    // 8 — i.e. their original SP before the CALL adjusted nothing
    // else.
    u64 rsp, rbp;
    __asm__ volatile("movq %%rsp, %0"
                     : "=r"(rsp));
    __asm__ volatile("movq %%rbp, %0"
                     : "=r"(rbp));
    u64 rip  = (u64)(uintptr_t)__builtin_return_address(0);
    rsp     += 8;

    enum {
        HARD_CAP = 256
    };
    size captured = 0;
    size emitted  = 0;

    for (size depth = 0; depth < HARD_CAP; ++depth) {
        if (rip == 0)
            break;

        if (emitted >= skip_frames) {
            if (captured >= max_frames)
                break;
            out[captured++].ip = (void *)(uintptr_t)rip;
        }
        ++emitted;

        const DwarfCfi *cfi         = NULL;
        const DwarfFde *fde         = NULL;
        u64             module_base = 0;
        if (!SymbolResolverFindFde(resolver, (void *)(uintptr_t)rip, &cfi, &fde, &module_base)) {
            break;
        }
        u64 file_relative = rip - module_base;

        DwarfUnwindRow row;
        if (!DwarfCfiBuildRow(cfi, fde, file_relative, &row))
            break;

        u64 cfa = 0;
        if (!apply_cfa(&row.cfa, rsp, rbp, &cfa))
            break;

        // Saved RIP at the return-address-register's offset off CFA.
        u64                 saved_rip = 0;
        const DwarfRegRule *ra_rule   = &row.regs[row.return_address_register];
        if (!apply_reg_rule(ra_rule, cfa, rip, &saved_rip))
            break;

        // Saved RBP may or may not be tracked; we only need it if a
        // later frame's CFA is RBP-based. Update opportunistically.
        u64                 saved_rbp = rbp;
        const DwarfRegRule *rbp_rule  = &row.regs[DWARF_REG_RBP];
        if (rbp_rule->kind == DWARF_REG_RULE_OFFSET) {
            (void)read_u64_at(cfa + (u64)rbp_rule->offset, &saved_rbp);
        }

        // Advance: the previous frame's SP is the CFA we just used.
        rip = saved_rip;
        rsp = cfa;
        rbp = saved_rbp;
    }

    return captured;
}

#    elif MISRA_HAVE_SYS_SYMRESOLVE && MISRA_HAVE_PARSER_DWARF
size CaptureStackTraceCfi(StackFrame *out, size max_frames, size skip_frames, SymbolResolver *resolver) {
    // CFI walker not yet implemented for this architecture (only x86-64
    // in v1). aarch64 follows a very similar pattern and is in
    // FUTURE-PLANS.
    (void)out;
    (void)max_frames;
    (void)skip_frames;
    (void)resolver;
    return 0;
}
#    endif

#else
#    error "Sys/Backtrace requires Windows or GCC/Clang frame-pointer builtins"
#endif
