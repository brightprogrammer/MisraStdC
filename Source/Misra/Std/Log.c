/// file      : std/log.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Stateless logging implementation.
///
/// The logger holds no process state -- no init, no deinit, no globals,
/// no mutex. Each LogWrite call:
///
///   - Stack-creates a HeapAllocator for the message Str.
///   - Builds the full line (prefix + caller-supplied message + '\n')
///     via StrAppendFmt so all in-tree types (Str / Int / Float / etc.)
///     work as format args.
///   - Issues a single FileWrite to the appropriate File:
///       INFO  -> FileFromFd(1)  (normal output channel)
///       ERROR -> FileFromFd(2)  (diagnostic channel)
///       FATAL -> FileFromFd(2)  (diagnostic channel) + appends a
///                                captured stack trace before returning.
///   - The single FileWrite is per-call atomic for sub-PIPE_BUF
///     (4096) byte writes on POSIX, which our lines fit easily in.
///     This gives thread safety without a mutex on POSIX. Windows
///     console writes aren't strictly atomic but lines won't shred
///     in practice for typical lengths.
///
/// LOG_FATAL still needs to terminate; the macro calls Abort() right
/// after LogWrite returns.

#include <Misra/Std/Allocator/Heap.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Std/File.h>
#include <Misra/Std/Io.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Memory.h>
#include <Misra/Sys.h>
#include <Misra/Sys/Backtrace.h>

void LogWrite(LogMessageType type, Zstr tag, u64 line, Zstr msg) {
    if (!msg) {
        return;
    }
    if (!tag) {
        tag = "misra";
    }

    static Zstr NAMES[] = {
        [LOG_MESSAGE_TYPE_FATAL] = "FATAL",
        [LOG_MESSAGE_TYPE_ERROR] = "ERROR",
        [LOG_MESSAGE_TYPE_INFO]  = "INFO",
    };

    HeapAllocator h    = HeapAllocatorInit();
    Allocator    *a    = ALLOCATOR_OF(&h);
    Str           full = StrInit(a);
    StrAppendFmt(&full, "[{}] [{}:{}] {}\n", (Zstr)NAMES[type], (Zstr)tag, line, (Zstr)msg);

    File out = (type == LOG_MESSAGE_TYPE_INFO) ? FileFromFd(1) : FileFromFd(2);
    (void)FileWrite(&out, StrBegin(&full), StrLen(&full));

#if !defined(MISRA_LOG_NO_BACKTRACE) || !MISRA_LOG_NO_BACKTRACE
    if (type == LOG_MESSAGE_TYPE_FATAL) {
        // Append captured stack trace so the diagnostic carries the
        // call site context up to Abort(). Skip our own + LogWrite's
        // frame (1 frame).
        StackFrame frames[32];
        size       n     = CaptureStackTrace(frames, 32, 1);
        Str        trace = StrInit(a);
        FormatStackTrace(&trace, frames, n, a);
        (void)FileWrite(&out, StrBegin(&trace), StrLen(&trace));
        StrDeinit(&trace);
    }
#else
    // MISRA_LOG_NO_BACKTRACE: deadend test binaries opt out of the
    // FATAL backtrace because (a) they install an abort callback that
    // longjmps over the trace anyway, and (b) on macOS each backtrace
    // re-parses the binary's Mach-O + dSYM + DWARF (MachoCache lives
    // for the duration of one trace only), which dominates sanitised
    // test wall-clock at ~99% per measurement. The FATAL message
    // itself (with func:line) already identifies the abort point.
    (void)type;
#endif

    StrDeinit(&full);
    HeapAllocatorDeinit(&h);
}
