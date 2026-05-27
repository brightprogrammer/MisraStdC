/// file      : Backtrace.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Stack-trace capture + formatter, three platform backends, each
/// offering raw and Vec shapes via the PascalCase macros in
/// Backtrace.h.
///
/// Shape design:
///   - Capture walks emit each frame through a sink callback. The
///     walker itself never allocates; only the sink decides what to do
///     with each frame. Raw form's sink writes into a fixed buffer;
///     Vec form's sink pushes into a `StackFrames` and reports OOM
///     back through the sink's return value.
///   - The FP-based walkers (Linux + macOS) are `always_inline`
///     helpers so `__builtin_frame_address(0)` evaluates relative to
///     the public function it's inlined into, not the helper itself.
///   - Format walks are straight loops over `(frames, count)`; the
///     raw form takes those directly, the Vec form unpacks `.data` /
///     `.length` and forwards.

#include <Misra/Sys/Backtrace.h>

#include <Misra/Std.h>
#include <Misra/Std/Log.h>
// Backtrace.h forward-declares SymbolResolver to keep the include
// footprint small (so files like Bin/ElfInfo.c that maintain their
// own ELF enum vocabulary aren't poisoned by Parsers/Elf.h transitively).
// The implementation needs the full definition.
#if FEATURE_SYS_SYMRESOLVE
#    include <Misra/Sys/SymbolResolver.h>
#endif

// ---------------------------------------------------------------------------
// Sink callback shared across all backends.
// ---------------------------------------------------------------------------

typedef bool (*StackFrameSinkFn)(void *user, void *ip);

typedef struct RawSinkCtx {
    StackFrame *out;
    size        max;
    size        n;
} RawSinkCtx;

typedef struct VecSinkCtx {
    StackFrames *vec;
    bool         oom;
} VecSinkCtx;

static bool raw_sink(void *user, void *ip) {
    RawSinkCtx *s = (RawSinkCtx *)user;
    if (s->n >= s->max)
        return false;
    s->out[s->n++].ip = ip;
    return true;
}

static bool vec_sink(void *user, void *ip) {
    VecSinkCtx *s = (VecSinkCtx *)user;
    StackFrame  f = {.ip = ip};
    if (!VecPushBackR(s->vec, f)) {
        s->oom = true;
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// Helpers used by formatters
// ---------------------------------------------------------------------------

static Zstr basename_of(Zstr path) {
    if (!path)
        return "?";
    Zstr slash = path;
    for (Zstr p = path; *p; ++p) {
        if (*p == '/' || *p == '\\')
            slash = p + 1;
    }
    return slash;
}

// ---------------------------------------------------------------------------
// Windows backend
// ---------------------------------------------------------------------------

#if PLATFORM_WINDOWS

#    include <windows.h>
#    include <dbghelp.h>
#    if FEATURE_PARSER_PDB
#        include <Misra/Sys/PdbCache.h>
#    endif

static bool g_dbghelp_initialized = false;

static void ensure_dbghelp(void) {
    if (g_dbghelp_initialized)
        return;
    SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);
    if (SymInitialize(GetCurrentProcess(), NULL, TRUE))
        g_dbghelp_initialized = true;
}

#    if FEATURE_PARSER_PDB
// Find the loaded module for `ip` via the Windows loader. No
// allocation inside.
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
    *out_base = (u64)mod;
    return true;
}
#    endif

// Sink-driven capture wrapper around CaptureStackBackTrace. The
// kernel32 call gives us a raw array of IPs; we hand each off to the
// sink, stopping early if it returns false.
static size win_capture_walk(size skip_frames, StackFrameSinkFn sink, void *user) {
    enum {
        SCRATCH_MAX = 256
    };
    void *raw[SCRATCH_MAX];
    // Add 1 to skip win_capture_walk's own frame.
    ULONG n       = CaptureStackBackTrace((DWORD)(skip_frames + 1), SCRATCH_MAX, raw, NULL);
    size  emitted = 0;
    for (ULONG i = 0; i < n; ++i) {
        if (!sink(user, raw[i]))
            break;
        ++emitted;
    }
    return emitted;
}

size capture_stack_trace_raw(StackFrame *out, size max_frames, size skip_frames) {
    if (!out || max_frames == 0)
        return 0;
    RawSinkCtx s = {out, max_frames, 0};
    win_capture_walk(skip_frames + 1, raw_sink, &s);
    return s.n;
}

bool capture_stack_trace_vec(StackFrames *out, size skip_frames) {
    if (!out || !out->allocator)
        return false;
    VecSinkCtx s = {out, false};
    win_capture_walk(skip_frames + 1, vec_sink, &s);
    return !s.oom;
}

// Shared formatter body (raw view: pointer + length).
static void format_walk_win(Str *out, const StackFrame *frames, size count, Allocator *alloc) {
    ensure_dbghelp();
    HANDLE proc = GetCurrentProcess();

    enum {
        MAX_NAME = 512
    };
    ULONG64      sym_buf[(sizeof(SYMBOL_INFO) + MAX_NAME + sizeof(ULONG64) - 1) / sizeof(ULONG64)];
    SYMBOL_INFO *sym = (SYMBOL_INFO *)sym_buf;
    MemSet(sym, 0, sizeof(*sym));
    sym->SizeOfStruct = sizeof(SYMBOL_INFO);
    sym->MaxNameLen   = MAX_NAME;

    IMAGEHLP_LINE64 line;
    MemSet(&line, 0, sizeof(line));
    line.SizeOfStruct = sizeof(line);

#    if FEATURE_PARSER_PDB
    PdbCache pdb_cache;
    bool     pdb_cache_ok = alloc && PdbCacheInit(&pdb_cache, alloc);
#    else
    (void)alloc;
#    endif

    for (size i = 0; i < count; ++i) {
        DWORD64     ip       = (DWORD64)(u64)frames[i].ip;
        bool        named    = false;
        Zstr sym_name = NULL;
        u32         sym_off  = 0;

#    if FEATURE_PARSER_PDB
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
                sym_name = (Zstr)sym->Name;
                sym_off  = (u32)d_off;
                named    = true;
            }
        }

        if (named) {
            StrAppendFmt(out, "  #{} {}+{x} [{x}]", (u32)i, sym_name, (u64)sym_off, (u64)ip);
        } else {
            StrAppendFmt(out, "  #{} {x}", (u32)i, (u64)ip);
        }

        DWORD line_disp = 0;
        if (g_dbghelp_initialized && SymGetLineFromAddr64(proc, ip, &line_disp, &line) && line.FileName) {
            Zstr fname = basename_of(line.FileName);
            StrAppendFmt(out, " ({}:{})", fname, (u32)line.LineNumber);
        }
        StrPushBackR(out, '\n');
    }

#    if FEATURE_PARSER_PDB
    if (pdb_cache_ok)
        PdbCacheDeinit(&pdb_cache);
#    endif
}

void format_stack_trace_raw(Str *out, const StackFrame *frames, size count, Allocator *alloc) {
    if (!out || !frames)
        return;
    format_walk_win(out, frames, count, alloc);
}

void format_stack_trace_vec(Str *out, const StackFrames *frames, Allocator *alloc) {
    if (!out || !frames)
        return;
    format_walk_win(out, frames->data, frames->length, alloc);
}

// ---------------------------------------------------------------------------
// macOS / Darwin backend
// ---------------------------------------------------------------------------
//
// Capture path: same saved-FP walk as the Linux backend below
// (frame pointers are the norm on x86_64 / arm64 Darwin builds).
// Format path: per-IP we ask dyld for the loaded image's path + slide
// and route through the in-tree MachoCache, which handles the binary
// + dSYM + DWARF chain.
//
// We deliberately do NOT `#include <mach-o/dyld.h>` / `<mach-o/loader.h>`
// here -- they transitively pull in `<stdbool.h>`, which `#define bool
// _Bool` and breaks Misra's "bool comes from Types.h, always means i8"
// invariant. Following the `Parsers/Elf` pattern, we restate the
// handful of dyld / Mach-O declarations we actually need.

#elif PLATFORM_DARWIN

// dyld public API surface (libdyld.dylib, ABI-stable).
extern u32         _dyld_image_count(void);
extern const void *_dyld_get_image_header(u32 image_index);
extern Zstr _dyld_get_image_name(u32 image_index);
extern i64         _dyld_get_image_vmaddr_slide(u32 image_index);

// Mach-O constants we look at.
enum {
    MACHO_MH_MAGIC_64   = 0xFEEDFACFu,
    MACHO_LC_SEGMENT_64 = 0x19,
};

// Mach-O 64-bit header layout (mirrors `struct mach_header_64`).
typedef struct MachoHeader64 {
    u32 magic;
    u32 cputype;
    u32 cpusubtype;
    u32 filetype;
    u32 ncmds;
    u32 sizeofcmds;
    u32 flags;
    u32 reserved;
} MachoHeader64;

// Load command head: every LC_* record starts with this 8-byte tuple.
typedef struct MachoLoadCommandHdr {
    u32 cmd;
    u32 cmdsize;
} MachoLoadCommandHdr;

// `segment_command_64` prefix -- we only read fields up through vmsize.
typedef struct MachoSegmentCommand64 {
    u32  cmd;
    u32  cmdsize;
    char segname[16];
    u64  vmaddr;
    u64  vmsize;
    u64  fileoff;
    u64  filesize;
    // trailing maxprot/initprot/nsects/flags not read
} MachoSegmentCommand64;

#    if FEATURE_PARSER_MACHO
#        include <Misra/Sys/MachoCache.h>
#    endif

enum {
    BACKTRACE_MAX_WALK = 256,
};

// always_inline so __builtin_frame_address(0) resolves to the wrapping
// public function's frame pointer rather than this helper's.
static __attribute__((always_inline)) inline size fp_walk(size skip_frames, StackFrameSinkFn sink, void *user) {
    void **fp = (void **)__builtin_frame_address(0);
    if (!fp)
        return 0;
    size   captured = 0;
    size   depth    = 0;
    void **prev_fp  = NULL;
    while (fp && depth < BACKTRACE_MAX_WALK) {
        if ((u64)fp & 0x7u)
            break;
        if (prev_fp && (u64)fp <= (u64)prev_fp)
            break;
        void *saved_fp = fp[0];
        void *ret_addr = fp[1];
        if (!ret_addr)
            break;
        if (depth >= skip_frames) {
            if (!sink(user, ret_addr))
                break;
            ++captured;
        }
        prev_fp = fp;
        fp      = (void **)saved_fp;
        ++depth;
    }
    return captured;
}

size capture_stack_trace_raw(StackFrame *out, size max_frames, size skip_frames) {
    if (!out || max_frames == 0)
        return 0;
    RawSinkCtx s = {out, max_frames, 0};
    fp_walk(skip_frames, raw_sink, &s);
    return s.n;
}

bool capture_stack_trace_vec(StackFrames *out, size skip_frames) {
    if (!out || !out->allocator)
        return false;
    VecSinkCtx s = {out, false};
    fp_walk(skip_frames, vec_sink, &s);
    return !s.oom;
}

#    if FEATURE_PARSER_MACHO
static bool dyld_image_for_ip(void *ip, Zstr *out_path, u64 *out_slide) {
    u64 ipx = (u64)ip;
    u32 n   = _dyld_image_count();
    for (u32 i = 0; i < n; ++i) {
        const MachoHeader64 *h = (const MachoHeader64 *)_dyld_get_image_header(i);
        if (!h || h->magic != MACHO_MH_MAGIC_64)
            continue;
        i64       slide = _dyld_get_image_vmaddr_slide(i);
        const u8 *cmd_p = (const u8 *)h + sizeof(MachoHeader64);
        for (u32 c = 0; c < h->ncmds; ++c) {
            const MachoLoadCommandHdr *lc = (const MachoLoadCommandHdr *)cmd_p;
            if (lc->cmd == MACHO_LC_SEGMENT_64) {
                const MachoSegmentCommand64 *seg = (const MachoSegmentCommand64 *)cmd_p;
                u64                          lo  = (u64)(seg->vmaddr + (u64)slide);
                u64                          hi  = lo + (u64)seg->vmsize;
                if (ipx >= lo && ipx < hi) {
                    *out_path  = _dyld_get_image_name(i);
                    *out_slide = (u64)slide;
                    return true;
                }
            }
            cmd_p += lc->cmdsize;
        }
    }
    return false;
}
#    endif

static void format_walk_mac(Str *out, const StackFrame *frames, size count, Allocator *alloc) {
#    if FEATURE_PARSER_MACHO
    MachoCache cache;
    bool       cache_ok = alloc && MachoCacheInit(&cache, alloc);
#    else
    (void)alloc;
#    endif

    for (size i = 0; i < count; ++i) {
        u64         ip       = (u64)frames[i].ip;
        Zstr sym_name = NULL;
        u32         sym_off  = 0;
        Zstr mod_path = NULL;
        bool        named    = false;

#    if FEATURE_PARSER_MACHO
        u64 slide = 0;
        if (cache_ok && dyld_image_for_ip(frames[i].ip, &mod_path, &slide)) {
            if (MachoCacheResolve(&cache, mod_path, slide, ip, &sym_name, &sym_off)) {
                named = true;
            }
        }
#    endif

        if (named) {
            Zstr mod = basename_of(mod_path);
            StrAppendFmt(out, "  #{} {}!{}+{x} [{x}]\n", (u32)i, mod, sym_name, (u64)sym_off, ip);
        } else if (mod_path) {
            Zstr mod = basename_of(mod_path);
            StrAppendFmt(out, "  #{} {}+? [{x}]\n", (u32)i, mod, ip);
        } else {
            StrAppendFmt(out, "  #{} {x}\n", (u32)i, ip);
        }
    }

#    if FEATURE_PARSER_MACHO
    if (cache_ok)
        MachoCacheDeinit(&cache);
#    endif
}

void format_stack_trace_raw(Str *out, const StackFrame *frames, size count, Allocator *alloc) {
    if (!out || !frames || !alloc)
        return;
    format_walk_mac(out, frames, count, alloc);
}

void format_stack_trace_vec(Str *out, const StackFrames *frames, Allocator *alloc) {
    if (!out || !frames || !alloc)
        return;
    format_walk_mac(out, frames->data, frames->length, alloc);
}

// ---------------------------------------------------------------------------
// Linux / GCC + Clang backend
// ---------------------------------------------------------------------------

#elif defined(__GNUC__) || defined(__clang__)

enum {
    BACKTRACE_MAX_WALK = 256,
};

// always_inline so __builtin_frame_address(0) inside this helper
// resolves to the public function's frame pointer after inlining.
static __attribute__((always_inline)) inline size fp_walk(size skip_frames, StackFrameSinkFn sink, void *user) {
    void **fp = (void **)__builtin_frame_address(0);
    if (!fp)
        return 0;
    size   captured = 0;
    size   depth    = 0;
    void **prev_fp  = NULL;
    while (fp && depth < BACKTRACE_MAX_WALK) {
        if ((u64)fp & 0x7u)
            break;
        if (prev_fp && (u64)fp <= (u64)prev_fp)
            break;
        void *saved_fp = fp[0];
        void *ret_addr = fp[1];
        if (!ret_addr)
            break;
        if (depth >= skip_frames) {
            if (!sink(user, ret_addr))
                break;
            ++captured;
        }
        prev_fp = fp;
        fp      = (void **)saved_fp;
        ++depth;
    }
    return captured;
}

size capture_stack_trace_raw(StackFrame *out, size max_frames, size skip_frames) {
    if (!out || max_frames == 0)
        return 0;
    RawSinkCtx s = {out, max_frames, 0};
    fp_walk(skip_frames, raw_sink, &s);
    return s.n;
}

bool capture_stack_trace_vec(StackFrames *out, size skip_frames) {
    if (!out || !out->allocator)
        return false;
    VecSinkCtx s = {out, false};
    fp_walk(skip_frames, vec_sink, &s);
    return !s.oom;
}

static void emit_resolved_line(Str *out, u32 idx, const ResolvedSymbol *r, void *ip) {
    if (r->symbol_name) {
        Zstr mod = basename_of(r->module_path);
        StrAppendFmt(out, "  #{} {}!{}+{x} [{x}]", idx, mod, r->symbol_name, r->offset, (u64)ip);
    } else if (r->module_path) {
        Zstr mod = basename_of(r->module_path);
        StrAppendFmt(out, "  #{} {}+{x} [{x}]", idx, mod, r->offset, (u64)ip);
    } else {
        StrAppendFmt(out, "  #{} {x}", idx, (u64)ip);
    }
    if (r->source_file) {
        Zstr file = basename_of(r->source_file);
        if (r->source_line > 0) {
            StrAppendFmt(out, " ({}:{})", file, r->source_line);
        } else {
            StrAppendFmt(out, " ({})", file);
        }
    }
    StrPushBackR(out, '\n');
}

static void format_walk_with(Str *out, const StackFrame *frames, size count, SymbolResolver *resolver) {
    for (size i = 0; i < count; ++i) {
        ResolvedSymbol r;
        if (SymbolResolverResolve(resolver, frames[i].ip, &r)) {
            emit_resolved_line(out, (u32)i, &r, frames[i].ip);
        } else {
            StrAppendFmt(out, "  #{} {x}\n", (u32)i, (u64)frames[i].ip);
        }
    }
}

static void format_walk_alloc(Str *out, const StackFrame *frames, size count, Allocator *alloc) {
    SymbolResolver res;
    if (!SymbolResolverInit(&res, alloc)) {
        for (size i = 0; i < count; ++i) {
            StrAppendFmt(out, "  #{} {x}\n", (u32)i, (u64)frames[i].ip);
        }
        return;
    }
    format_walk_with(out, frames, count, &res);
    SymbolResolverDeinit(&res);
}

void format_stack_trace_with_raw(Str *out, const StackFrame *frames, size count, SymbolResolver *resolver) {
    if (!out || !frames || !resolver)
        return;
    format_walk_with(out, frames, count, resolver);
}

void format_stack_trace_with_vec(Str *out, const StackFrames *frames, SymbolResolver *resolver) {
    if (!out || !frames || !resolver)
        return;
    format_walk_with(out, frames->data, frames->length, resolver);
}

void format_stack_trace_raw(Str *out, const StackFrame *frames, size count, Allocator *alloc) {
    if (!out || !frames || !alloc)
        return;
    format_walk_alloc(out, frames, count, alloc);
}

void format_stack_trace_vec(Str *out, const StackFrames *frames, Allocator *alloc) {
    if (!out || !frames || !alloc)
        return;
    format_walk_alloc(out, frames->data, frames->length, alloc);
}

// ---------------------------------------------------------------------------
// CFI-based unwinder (Linux x86-64 only in v1)
// ---------------------------------------------------------------------------
//
// x86-64 SysV ABI: DWARF register numbering puts RSP at 7, RBP at 6,
// and the return-address pseudo-register at 16. We track RSP and RBP
// across frames; other GPRs aren't needed to walk the call stack.

#    if FEATURE_SYS_SYMRESOLVE && FEATURE_PARSER_DWARF && ARCHITECTURE_X86_64

enum {
    DWARF_REG_RBP = 6,
    DWARF_REG_RSP = 7,
};

static bool read_u64_at(u64 addr, u64 *out) {
    if (addr == 0)
        return false;
    if (addr & 0x7)
        return false;
    *out = *(volatile u64 *)(u64)addr;
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
            return false;
    }
    *out_cfa = base + (u64)cfa->offset;
    return true;
}

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
        case DWARF_REG_RULE_REGISTER :
        case DWARF_REG_RULE_EXPRESSION :
        default :
            return false;
    }
}

// CFI walk wrapped to use the same sink pattern as fp_walk. Inlined
// for the same reason: the inline-asm RSP/RBP reads + the
// __builtin_return_address(0) call need to evaluate in the wrapping
// public function's frame.
static __attribute__((always_inline)) inline size
    cfi_walk(size skip_frames, SymbolResolver *resolver, StackFrameSinkFn sink, void *user) {
    u64 rsp, rbp;
    __asm__ volatile("movq %%rsp, %0"
                     : "=r"(rsp));
    __asm__ volatile("movq %%rbp, %0"
                     : "=r"(rbp));
    u64 rip  = (u64)__builtin_return_address(0);
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
            if (!sink(user, (void *)(u64)rip))
                break;
            ++captured;
        }
        ++emitted;

        const DwarfCfi *cfi         = NULL;
        const DwarfFde *fde         = NULL;
        u64             module_base = 0;
        if (!SymbolResolverFindFde(resolver, (void *)(u64)rip, &cfi, &fde, &module_base))
            break;
        u64 file_relative = rip - module_base;

        DwarfUnwindRow row;
        if (!DwarfCfiBuildRow(cfi, fde, file_relative, &row))
            break;

        u64 cfa = 0;
        if (!apply_cfa(&row.cfa, rsp, rbp, &cfa))
            break;

        u64                 saved_rip = 0;
        const DwarfRegRule *ra_rule   = &row.regs[row.return_address_register];
        if (!apply_reg_rule(ra_rule, cfa, rip, &saved_rip))
            break;

        u64                 saved_rbp = rbp;
        const DwarfRegRule *rbp_rule  = &row.regs[DWARF_REG_RBP];
        if (rbp_rule->kind == DWARF_REG_RULE_OFFSET) {
            (void)read_u64_at(cfa + (u64)rbp_rule->offset, &saved_rbp);
        }

        rip = saved_rip;
        rsp = cfa;
        rbp = saved_rbp;
    }
    return captured;
}

size capture_stack_trace_cfi_raw(StackFrame *out, size max_frames, size skip_frames, SymbolResolver *resolver) {
    if (!out || max_frames == 0 || !resolver)
        return 0;
    RawSinkCtx s = {out, max_frames, 0};
    cfi_walk(skip_frames, resolver, raw_sink, &s);
    return s.n;
}

bool capture_stack_trace_cfi_vec(StackFrames *out, size skip_frames, SymbolResolver *resolver) {
    if (!out || !out->allocator || !resolver)
        return false;
    VecSinkCtx s = {out, false};
    cfi_walk(skip_frames, resolver, vec_sink, &s);
    return !s.oom;
}

#    elif FEATURE_SYS_SYMRESOLVE && FEATURE_PARSER_DWARF
// CFI walker not yet implemented for this architecture (only x86-64
// in v1). aarch64 follows a very similar pattern and is in
// FUTURE-PLANS.
size capture_stack_trace_cfi_raw(StackFrame *out, size max_frames, size skip_frames, SymbolResolver *resolver) {
    (void)out;
    (void)max_frames;
    (void)skip_frames;
    (void)resolver;
    return 0;
}
bool capture_stack_trace_cfi_vec(StackFrames *out, size skip_frames, SymbolResolver *resolver) {
    (void)out;
    (void)skip_frames;
    (void)resolver;
    return false;
}
#    endif

#else
#    error "Sys/Backtrace requires Windows, macOS, or GCC/Clang frame-pointer builtins"
#endif
