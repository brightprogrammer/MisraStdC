/// file      : Backtrace.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Stack-trace capture + formatting. Three platform backends:
///
///   - **Linux / GCC + Clang**: pure-Misra implementation. Capture
///     walks saved-FP chain via `__builtin_frame_address`. Format
///     pumps each IP through `Sys/SymbolResolver`, which reads our
///     own `/proc/self/maps` parser and our own ELF + DWARF parsers.
///     No libc `backtrace()`, no libgcc unwinder, no `dladdr`.
///
///   - **macOS / Darwin**: pure-Misra. Same FP walk for capture;
///     formatting routes per-IP through `Sys/MachoCache`, which uses
///     dyld to locate the loaded image, then the in-tree Mach-O +
///     dSYM + DWARF chain for symbol resolution.
///
///   - **Windows / MSVC**: wraps `CaptureStackBackTrace` (kernel32)
///     for capture. Format tries the in-tree PE + PDB chain first
///     (`Sys/PdbCache`), falling back to dbghelp's `SymFromAddr` /
///     `SymGetLineFromAddr64` when the PDB can't be located.
///
/// Each capture / format function has two callable shapes via
/// `MISRA_OVERLOAD`:
///
///   - **Raw**: caller passes a fixed `StackFrame *` buffer (typically
///     stack-allocated). No allocations happen anywhere in the walk
///     itself, so this form is safe from crash handlers and is what
///     the debug allocator uses to record allocation sites without
///     recursing back into itself.
///
///   - **Vec**: caller passes a `StackFrames *` (a `Vec(StackFrame)`),
///     which grows through its own allocator as the walk emits frames.
///     Convenient for application code that doesn't have a crash-path
///     constraint.

#ifndef MISRA_SYS_BACKTRACE_H
#define MISRA_SYS_BACKTRACE_H

#include <Misra/Std/Allocator.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Container/Vec.h>
#include <Misra/Types.h>

// SymbolResolver only exists on the in-tree (Linux) path. Forward-
// declare here -- the FormatStackTraceWith APIs only take a pointer,
// so including the full header would pull `<Misra/Parsers/Elf.h>`
// (and its enum names) transitively into every TU that uses
// Backtrace, including files like `Bin/ElfInfo.c` that maintain
// their own ELF enum vocabulary. Callers who actually construct a
// SymbolResolver should include the full header themselves.
#if FEATURE_SYS_SYMRESOLVE
typedef struct SymbolResolver SymbolResolver;
#endif

///
/// A single captured stack frame. Just the IP -- symbol resolution
/// happens later in `FormatStackTrace`.
///
typedef struct StackFrame {
    void *ip;
} StackFrame;

///
/// Vec-flavoured handle for stack frames. The Vec form of every
/// public capture / format function consumes this.
///
typedef Vec(StackFrame) StackFrames;

// ---------------------------------------------------------------------------
// Implementation functions (snake_case). The PascalCase macros below
// are the supported entry points; these signatures exist as the actual
// linkable symbols so the macros can resolve.
// ---------------------------------------------------------------------------

///
/// Raw capture: fixed buffer, no allocation. Safe inside crash
/// handlers and inside the debug allocator itself.
///
/// out[out]         : Buffer (caller-owned, usually stack).
/// max_frames[in]   : Capacity of `out`.
/// skip_frames[in]  : Caller-side wrappers to discard.
///
/// SUCCESS : Returns number of frames written (<= max_frames).
/// FAILURE : Returns 0 if FPs are unavailable (e.g. `-fomit-frame-pointer`
///           builds on the FP-walking backends) or the stack is invalid.
///
size capture_stack_trace_raw(StackFrame *out, size max_frames, size skip_frames);

///
/// Vec capture: grows `out` through its own allocator. Convenient but
/// allocates; do not use from the debug allocator or a crash handler.
///
/// SUCCESS : Returns true; `out` contains the captured frames.
/// FAILURE : Returns false on allocator OOM during the walk.
///
bool capture_stack_trace_vec(StackFrames *out, size skip_frames);

#if FEATURE_SYS_SYMRESOLVE && FEATURE_PARSER_DWARF
///
/// CFI-based capture (Linux x86-64). Same shape as the FP-walk
/// variants but routes through DWARF `.eh_frame` rules, so it works
/// on `-fomit-frame-pointer` builds.
///
/// SUCCESS : `_raw` returns number of frames written (<= max_frames);
///           `_vec` returns true and grows `out` through its allocator.
/// FAILURE : `_raw` returns 0; `_vec` returns false. Triggered by an
///           unwind step that the CFI rules can't resolve or an OOM
///           inside `_vec`.
///
size capture_stack_trace_cfi_raw(StackFrame *out, size max_frames, size skip_frames, SymbolResolver *resolver);
bool capture_stack_trace_cfi_vec(StackFrames *out, size skip_frames, SymbolResolver *resolver);
#endif

///
/// Raw formatter: walks `(frames, count)`, appends one line per frame
/// to `out`. Creates a fresh `SymbolResolver` internally per call.
///
/// SUCCESS : Returns to the caller; `out` is appended to (one line per
///           input frame). Unresolved frames render as `#N 0x<ip>`.
/// FAILURE : Cannot fail. Allocator OOM during the append is logged via
///           the underlying StrAppendFmt and silently truncates; the
///           formatter never reports failure to the caller.
///
void format_stack_trace_raw(Str *out, const StackFrame *frames, size count, Allocator *alloc);

///
/// Vec formatter: same as raw but consumes a `StackFrames *`.
///
/// SUCCESS : Returns to the caller; `out` is appended to.
/// FAILURE : Cannot fail. See `format_stack_trace_raw` for the OOM
///           behaviour.
///
void format_stack_trace_vec(Str *out, const StackFrames *frames, Allocator *alloc);

#if FEATURE_SYS_SYMRESOLVE
///
/// Resolver-sharing variants -- cheaper when formatting many traces in
/// a loop. Linux only (only platform with an in-tree SymbolResolver).
///
/// SUCCESS : Returns to the caller; `out` is appended to using
///           `resolver`'s already-built symbol tables.
/// FAILURE : Cannot fail. OOM during append is logged + silently
///           truncates; the formatter never reports failure.
///
void format_stack_trace_with_raw(Str *out, const StackFrame *frames, size count, SymbolResolver *resolver);
void format_stack_trace_with_vec(Str *out, const StackFrames *frames, SymbolResolver *resolver);
#endif

// ---------------------------------------------------------------------------
// Public macros: dispatch by arg count. The 3-arg / 4-arg forms call
// the raw implementations; the 2-arg / 3-arg forms call the vec ones.
//
// NOTE: the two forms have different return types -- raw capture
// returns `size`, vec capture returns `bool`. The macro is just a
// name dispatcher, so use the return type matching the form you call.
// ---------------------------------------------------------------------------

///
/// Capture up to `max_frames` stack frames into `out`. Two shapes:
///
///   `CaptureStackTrace(out, max, skip)`  -- raw, returns `size`
///   `CaptureStackTrace(out_vec, skip)`   -- vec, returns `bool`
///
/// `CaptureStackTrace`'s own frame is *always* skipped on top of
/// whatever `skip` discards.
///
/// TAGS: Sys, Backtrace, Unwind
///
#define CaptureStackTrace(...)              MISRA_OVERLOAD(CaptureStackTrace, __VA_ARGS__)
#define CaptureStackTrace_3(out, max, skip) capture_stack_trace_raw((out), (max), (skip))
#define CaptureStackTrace_2(out, skip)      capture_stack_trace_vec((out), (skip))

#if FEATURE_SYS_SYMRESOLVE && FEATURE_PARSER_DWARF
///
/// CFI-based capture. Two shapes:
///
///   `CaptureStackTraceCfi(out, max, skip, resolver)` -- raw, returns `size`
///   `CaptureStackTraceCfi(out_vec, skip, resolver)`  -- vec, returns `bool`
///
/// TAGS: Sys, Backtrace, CFI, Unwind
///
#    define CaptureStackTraceCfi(...)                   MISRA_OVERLOAD(CaptureStackTraceCfi, __VA_ARGS__)
#    define CaptureStackTraceCfi_4(out, max, skip, res) capture_stack_trace_cfi_raw((out), (max), (skip), (res))
#    define CaptureStackTraceCfi_3(out, skip, res)      capture_stack_trace_cfi_vec((out), (skip), (res))
#endif

///
/// Format a captured trace. Two shapes:
///
///   `FormatStackTrace(out_str, frames, count, alloc)` -- raw
///   `FormatStackTrace(out_str, frames_vec, alloc)`    -- vec
///
/// Output is appended; never fails -- unresolved frames emit as
/// `#N 0x<ip>`.
///
/// TAGS: Sys, Backtrace, Format
///
#define FormatStackTrace(...)                         MISRA_OVERLOAD(FormatStackTrace, __VA_ARGS__)
#define FormatStackTrace_4(out, frames, count, alloc) format_stack_trace_raw((out), (frames), (count), (alloc))
#define FormatStackTrace_3(out, frames, alloc)        format_stack_trace_vec((out), (frames), (alloc))

#if FEATURE_SYS_SYMRESOLVE
///
/// Same as `FormatStackTrace` but reuses a caller-owned resolver.
/// Two shapes:
///
///   `FormatStackTraceWith(out_str, frames, count, resolver)` -- raw
///   `FormatStackTraceWith(out_str, frames_vec, resolver)`    -- vec
///
/// TAGS: Sys, Backtrace, Format
///
#    define FormatStackTraceWith(...)                       MISRA_OVERLOAD(FormatStackTraceWith, __VA_ARGS__)
#    define FormatStackTraceWith_4(out, frames, count, res) format_stack_trace_with_raw((out), (frames), (count), (res))
#    define FormatStackTraceWith_3(out, frames, res)        format_stack_trace_with_vec((out), (frames), (res))
#endif

#endif // MISRA_SYS_BACKTRACE_H
