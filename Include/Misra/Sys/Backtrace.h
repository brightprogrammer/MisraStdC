/// file      : Backtrace.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Stack-trace capture + formatting. Two platform backends:
///
///   - **Linux / GCC + Clang**: pure-Misra implementation. Capture
///     walks saved-FP chain via `__builtin_frame_address`. Format
///     pumps each IP through `Sys/SymbolResolver`, which reads our
///     own `/proc/self/maps` parser and our own ELF + DWARF parsers.
///     No libc `backtrace()`, no libgcc unwinder, no `dladdr`.
///
///   - **Windows / MSVC**: wraps `CaptureStackBackTrace` (kernel32)
///     and dbghelp's `SymFromAddr` / `SymGetLineFromAddr64`. Same
///     API surface; behind the scenes it uses the platform's system
///     libraries because there is no `/proc/self/maps` and PE+PDB
///     parsing is a separate effort tracked in FUTURE-PLANS.
///
/// The Linux capture path needs frame pointers, i.e. the program
/// (and any code we unwind through) must be built with
/// `-fno-omit-frame-pointer`. The Windows path uses SEH-based
/// unwinding from `CaptureStackBackTrace` and has no such
/// requirement.

#ifndef MISRA_SYS_BACKTRACE_H
#define MISRA_SYS_BACKTRACE_H

#include <Misra/Std/Allocator.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Types.h>

// SymbolResolver only exists on the in-tree (Linux) path.
#if MISRA_HAVE_SYS_SYMRESOLVE
#    include <Misra/Sys/SymbolResolver.h>
#endif

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

#if MISRA_HAVE_SYS_SYMRESOLVE
///
/// Same as `FormatStackTrace` but reuses a caller-owned resolver.
/// Cheaper when formatting many traces. Available only when the
/// in-tree `SymbolResolver` is compiled in (Linux today). On Windows
/// dbghelp does its own caching internally, so this variant is
/// neither offered nor needed.
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
#endif

#endif // MISRA_SYS_BACKTRACE_H
