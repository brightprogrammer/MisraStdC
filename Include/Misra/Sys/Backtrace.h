/// file      : sys/backtrace.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Stack-trace capture + formatting. Three platform backends:
///
///   - **Linux / GCC + Clang**: capture walks the saved frame-pointer
///     chain via `__builtin_frame_address`, so there is no third-party
///     unwinder dependency and the walk works inside crash handlers.
///     Format pumps each IP through `Sys/SymbolResolver`, which reads
///     `/proc/self/maps` via `Sys/ProcMaps` and the in-tree ELF + DWARF
///     parsers to recover names from `.symtab` (including statics).
///
///   - **macOS / Darwin**: same FP walk for capture; formatting routes
///     per-IP through `Sys/MachoCache`, which uses dyld to locate the
///     loaded image, then the in-tree Mach-O + dSYM + DWARF chain for
///     symbol resolution.
///
///   - **Windows / MSVC**: capture uses `CaptureStackBackTrace`
///     (kernel32) as the kernel boundary for stack collection. Format
///     tries the in-tree PE + PDB chain first (`Sys/PdbCache`), which
///     resolves names directly from the PDB stream tables; when no
///     PDB is locatable next to the module, format falls back to the
///     OS-supplied symbol API (dbghelp) so frames at least name the
///     module.
///
/// Each capture / format function has two callable shapes via
/// `OVERLOAD`:
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
// Backtrace, where downstream code that carries its own ELF enum
// vocabulary would collide. Callers who actually construct a
// SymbolResolver should include the full header themselves.
#if FEATURE_SYS_SYMRESOLVE
typedef struct SymbolResolver SymbolResolver;
#endif

///
/// A single captured stack frame. Just the IP -- symbol resolution
/// happens later in `FormatStackTrace`.
///
/// TAGS: Backtrace, Type, Frame
///
typedef struct StackFrame {
    void *ip;
} StackFrame;

///
/// Vec-flavoured handle for stack frames. The Vec form of every
/// public capture / format function consumes this.
///
/// TAGS: Backtrace, Type, Frame
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
/// TAGS: Backtrace, Capture, Trace, Stack
///
size capture_stack_trace_raw(StackFrame *out, size max_frames, size skip_frames);

///
/// Vec capture: grows `out` through its own allocator. Convenient but
/// allocates; do not use from the debug allocator or a crash handler.
///
/// SUCCESS : Returns true; `out` contains the captured frames.
/// FAILURE : Returns false on allocator OOM during the walk.
///
/// TAGS: Backtrace, Capture, Trace, Stack
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
/// TAGS: Backtrace, Capture, Trace, CFI, Stack
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
/// FAILURE : Function cannot fail. Allocator OOM while growing `*out`
///           aborts via `LOG_FATAL` (Str's standard must-succeed
///           contract).
///
/// TAGS: Backtrace, Format, Trace, Stack
///
void format_stack_trace_raw(Str *out, const StackFrame *frames, size count, Allocator *alloc);

///
/// Vec formatter: same as raw but consumes a `StackFrames *`.
///
/// SUCCESS : Returns to the caller; `out` is appended to.
/// FAILURE : Function cannot fail. Allocator OOM while growing `*out`
///           aborts via `LOG_FATAL` (Str's standard must-succeed
///           contract).
///
/// TAGS: Backtrace, Format, Trace, Stack
///
void format_stack_trace_vec(Str *out, const StackFrames *frames, Allocator *alloc);

#if FEATURE_SYS_SYMRESOLVE
///
/// Resolver-sharing variants -- cheaper when formatting many traces in
/// a loop. Linux only (only platform with an in-tree SymbolResolver).
///
/// SUCCESS : Returns to the caller; `out` is appended to using
///           `resolver`'s already-built symbol tables.
/// FAILURE : Function cannot fail. Allocator OOM while growing `*out`
///           aborts via `LOG_FATAL` (Str's standard must-succeed
///           contract).
///
/// TAGS: Backtrace, Format, Trace, Symbol, Stack
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
/// SUCCESS : Raw form returns the number of frames written into `out`
///           (`0..max`). Vec form returns `true`; `*out_vec` is grown
///           and populated with the captured frames.
/// FAILURE : Raw form returns `0` when no frame can be captured (e.g.
///           unwind library refuses, or `max == 0`). Vec form returns
///           `false` on allocator failure while growing `*out_vec`;
///           the vec is left in whatever partial state the failed
///           growth produced.
///
/// TAGS: Sys, Backtrace, Unwind
///
#define CaptureStackTrace(...)              OVERLOAD(CaptureStackTrace, __VA_ARGS__)
#define CaptureStackTrace_3(out, max, skip) capture_stack_trace_raw((out), (max), (skip))
#define CaptureStackTrace_2(out, skip)      capture_stack_trace_vec((out), (skip))

#if FEATURE_SYS_SYMRESOLVE && FEATURE_PARSER_DWARF
///
/// CFI-based capture. Two shapes:
///
///   `CaptureStackTraceCfi(out, max, skip, resolver)` -- raw, returns `size`
///   `CaptureStackTraceCfi(out_vec, skip, resolver)`  -- vec, returns `bool`
///
/// SUCCESS : Raw form returns the number of CFI-walked frames written
///           into `out` (`0..max`); `resolver` is reused for symbol
///           lookup. Vec form returns `true`; `*out_vec` is grown and
///           populated with the captured frames.
/// FAILURE : Raw form returns `0` when the CFI walker cannot make
///           progress (missing `.eh_frame` / `.debug_frame`, corrupt
///           tables, or `max == 0`). Vec form returns `false` on
///           allocator failure while growing `*out_vec`.
///
/// TAGS: Sys, Backtrace, CFI, Unwind
///
#    define CaptureStackTraceCfi(...)                   OVERLOAD(CaptureStackTraceCfi, __VA_ARGS__)
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
/// SUCCESS : Returns to the caller; `*out_str` has been appended with
///           one line per frame, resolved where the symbol resolver
///           could, and as `#N 0x<ip>` otherwise.
/// FAILURE : Function cannot fail. Allocator OOM while growing
///           `*out_str` aborts via `LOG_FATAL` (Str's standard
///           must-succeed contract).
///
/// TAGS: Sys, Backtrace, Format
///
#define FormatStackTrace(...)                         OVERLOAD(FormatStackTrace, __VA_ARGS__)
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
/// SUCCESS : Returns to the caller; `*out_str` has been appended with
///           one line per frame using `resolver` for symbol lookup,
///           falling back to `#N 0x<ip>` when the resolver cannot
///           name the frame.
/// FAILURE : Function cannot fail. Allocator OOM while growing
///           `*out_str` aborts via `LOG_FATAL`.
///
/// TAGS: Sys, Backtrace, Format
///
#    define FormatStackTraceWith(...)                       OVERLOAD(FormatStackTraceWith, __VA_ARGS__)
#    define FormatStackTraceWith_4(out, frames, count, res) format_stack_trace_with_raw((out), (frames), (count), (res))
#    define FormatStackTraceWith_3(out, frames, res)        format_stack_trace_with_vec((out), (frames), (res))
#endif

#endif // MISRA_SYS_BACKTRACE_H
