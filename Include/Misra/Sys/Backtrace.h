/// file      : Backtrace.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Stack-trace capture + formatting, built on top of `Sys/SymbolResolver`
/// (the in-tree dladdr replacement) and a frame-pointer-based walker.
/// Does **not** depend on libc `backtrace()`, libgcc unwinder, or
/// `dladdr` — both phases are pure-Misra code reading ELF + procmaps.
///
/// Two stages, as before:
///
///   `CaptureStackTrace(out, max, skip)` — fast: walks saved-FP chain,
///       writes raw instruction pointers. Constant-time per frame.
///
///   `FormatStackTrace(...)` — slow: resolves each IP to
///       `module!symbol+offset` via a `SymbolResolver` and renders the
///       result into a `Str`.
///
/// The capture path needs frame pointers, i.e. the program (and any
/// code we unwind through) must be built with `-fno-omit-frame-pointer`.
/// DWARF CFI unwinding for `-fomit-frame-pointer` builds is in
/// FUTURE-PLANS.md.

#ifndef MISRA_SYS_BACKTRACE_H
#define MISRA_SYS_BACKTRACE_H

#include <Misra/Std/Allocator.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Sys/SymbolResolver.h>
#include <Misra/Types.h>

///
/// A single captured stack frame. Just the IP — symbol resolution
/// happens later in `FormatStackTrace`.
///
typedef struct StackFrame {
    void *ip;
} StackFrame;

///
/// Capture up to `max_frames` frames of the caller's stack via the
/// saved-FP chain. `skip_frames` discards the topmost frames (caller's
/// own wrappers). `CaptureStackTrace`'s own frame is *always* skipped.
///
/// out[out]         : Frame array to populate.
/// max_frames[in]   : Capacity of `out`.
/// skip_frames[in]  : Number of caller-side wrappers to discard.
///
/// SUCCESS : Returns number of frames written (<= max_frames).
/// FAILURE : Returns 0 if frame pointers are unavailable (build with
///           `-fomit-frame-pointer`) or the stack is structurally
///           invalid. `out` is left untouched on failure.
///
/// TAGS: Sys, Backtrace, Unwind
///
size CaptureStackTrace(StackFrame *out, size max_frames, size skip_frames);

///
/// Resolve each frame to `module!symbol+offset` and append a multi-
/// line trace to `out`. A fresh `SymbolResolver` is created internally
/// for each call — convenient for one-off uses, but if you format many
/// traces in a loop, share a resolver via `FormatStackTraceWith`.
///
/// out[out]    : Str to append to.
/// frames[in]  : Frames captured by `CaptureStackTrace`.
/// count[in]   : Number of valid frames.
/// alloc[in]   : Allocator backing the resolver and any scratch.
///
/// SUCCESS : Output appended.
/// FAILURE : Function does not fail; unresolved frames emit as
///           `#N 0x<ip>`.
///
/// TAGS: Sys, Backtrace, Format
///
void FormatStackTrace(Str *out, const StackFrame *frames, size count, Allocator *alloc);

///
/// Same as `FormatStackTrace` but reuses a caller-owned resolver.
/// Cheaper when formatting many traces.
///
/// out[out]       : Str to append to.
/// frames[in]     : Frames captured by `CaptureStackTrace`.
/// count[in]      : Number of valid frames.
/// resolver[in,out] : Resolver to use. Cache may grow.
///
/// SUCCESS : Output appended.
/// FAILURE : Function does not fail.
///
/// TAGS: Sys, Backtrace, Format
///
void FormatStackTraceWith(Str *out, const StackFrame *frames, size count, SymbolResolver *resolver);

#endif // MISRA_SYS_BACKTRACE_H
