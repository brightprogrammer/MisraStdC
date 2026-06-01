/// file      : sys/proc.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Cross-platform child-process spawn and I/O. Stack-declared by value
/// via the `ProcInit(...)` macro; backend struct (POSIX fds vs Win32
/// PROCESS_INFORMATION + HANDLEs) is platform-conditional but the API
/// is uniform.

#ifndef MISRA_SYS_PROC_H
#define MISRA_SYS_PROC_H

#include <Misra/Std/Allocator.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Types.h>

// Deliberately NOT including <windows.h> from this public header --
// it would drag thousands of Win32 macros into every TU in the
// include chain and break unrelated code (e.g. Parsers/Pe.c's
// IMAGE_DEBUG_TYPE_CODEVIEW enum). Use layout-compatible opaque
// fields; Proc.c casts to the real Windows types inside the impl.
#if !PLATFORM_WINDOWS
#    include <sys/types.h> // pid_t
#endif

#ifdef __cplusplus
extern "C" {
#endif

    typedef u64 ProcId;

    typedef enum ProcStatus {
        PROC_STATUS_RUNNING,    // Process is still running
        PROC_STATUS_COMPLETED,  // Process completed normally
        PROC_STATUS_TERMINATED, // Process was terminated/killed
        PROC_STATUS_ERROR       // Error occurred while checking status
    } ProcStatus;

#if PLATFORM_WINDOWS
    // Layout-compatible with Win32 PROCESS_INFORMATION:
    //   typedef struct _PROCESS_INFORMATION {
    //       HANDLE hProcess;
    //       HANDLE hThread;
    //       DWORD  dwProcessId;
    //       DWORD  dwThreadId;
    //   } PROCESS_INFORMATION;
    // HANDLE is `void *`, DWORD is `unsigned long` (32-bit on Win64 -- u32).
    typedef struct {
        void *hProcess;
        void *hThread;
        u32   dwProcessId;
        u32   dwThreadId;
    } Win32ProcessInfo_;
#endif

    ///
    /// Process handle. Layout is platform-conditional. Stack-declare with
    /// `ProcInit(path, argv, envp)`; don't poke fields directly. The
    /// `_pid` (POSIX) or `_pi.hProcess` (Windows) being zero / NULL means
    /// the init failed -- check with `ProcOk(&p)`.
    ///
    /// TAGS: Proc, Type, API
    ///
    typedef struct Proc {
        int  _exit_code;
        bool _completed;
#if PLATFORM_WINDOWS
        Win32ProcessInfo_ _pi;          // layout-compat with PROCESS_INFORMATION
        void             *_hStdinWrite; // HANDLE
        void             *_hStdoutRead; // HANDLE
        void             *_hStderrRead; // HANDLE
#else
    pid_t _pid;
    int   _stdin_fd;  // write here to send to child stdin
    int   _stdout_fd; // read here from child stdout
    int   _stderr_fd; // read here from child error output
#endif
    } Proc;

    ///
    /// Spawn a child process. Returns a fully-formed `Proc` by value on
    /// success, or a zeroed-out `Proc` on failure (check with `ProcOk`).
    ///
    /// `alloc` is only used on Windows (for the command-line `Str` we
    /// pass to `CreateProcessA`); POSIX ignores it. Optional inside a
    /// `Scope` block.
    ///
    /// path[in]  : Path to the executable.
    /// argv[in]  : NULL-terminated argv array.
    /// envp[in]  : NULL-terminated envp array, or NULL to inherit parent's.
    /// alloc[in] : Allocator for the Windows cmdline buffer (optional).
    ///
    /// SUCCESS: Returns a `Proc` with pid/PROCESS_INFORMATION populated.
    /// FAILURE: Returns a zeroed `Proc` (`ProcOk(&p)` returns false).
    ///
    /// TAGS: Sys, Proc, Spawn
    ///
    Proc proc_init(Zstr path, char **argv, char **envp, Allocator *alloc);

#define ProcInit(...)                       OVERLOAD(ProcInit, __VA_ARGS__)
#define ProcInit_3(path, argv, envp)        proc_init((path), (argv), (envp), MisraScope)
#define ProcInit_4(path, argv, envp, alloc) proc_init((path), (argv), (envp), ALLOCATOR_OF(alloc))

    ///
    /// Tear down a `Proc`. Closes any open fds / HANDLEs the parent held
    /// open for the child's stdin/stdout/stderr pipes. Does NOT terminate
    /// the child -- call `ProcTerminate` first if you want that. Safe on
    /// a zeroed (never-spawned) `Proc`. No allocator argument: the
    /// allocator handed to `ProcInit` is used transiently during spawn
    /// (only on Windows, for the cmdline buffer) and is not retained.
    ///
    /// SUCCESS : Returns to the caller. `*p` is zeroed.
    /// FAILURE : Function cannot fail. NULL `p` is a no-op.
    ///
    /// TAGS: Sys, Proc, Deinit
    ///
    void ProcDeinit(Proc *p);

    ///
    /// Check whether `p` represents a successfully-spawned child.
    ///
    /// SUCCESS : Returns true when the child was spawned and the OS
    ///           handle / pid is valid.
    /// FAILURE : Returns false when `ProcInit` failed or `p` is NULL.
    ///           Cannot fail.
    ///
    /// TAGS: Proc, Query, State
    ///
    static inline bool ProcOk(const Proc *p) {
        if (!p) {
            return false;
        }
#if PLATFORM_WINDOWS
        return p->_pi.hProcess != (void *)0;
#else
    return p->_pid > 0;
#endif
    }

    ///
    /// Block until the child exits.
    ///
    /// SUCCESS : Returns `PROC_STATUS_COMPLETED` when the child
    ///           exits normally, or `PROC_STATUS_TERMINATED` when
    ///           the child was killed by a signal / external action.
    /// FAILURE : Returns `PROC_STATUS_ERROR`; the cause is logged.
    ///
    /// TAGS: Proc, Wait, API
    ///
    ProcStatus ProcWait(Proc *proc);

    ///
    /// Block for up to `timeout_ms` milliseconds waiting for the child.
    /// Pass 0 for infinite wait.
    ///
    /// SUCCESS : Returns `PROC_STATUS_COMPLETED`,
    ///           `PROC_STATUS_TERMINATED`, or
    ///           `PROC_STATUS_RUNNING` if the timeout elapsed first.
    /// FAILURE : Returns `PROC_STATUS_ERROR`; the cause is logged.
    ///
    /// TAGS: Proc, Wait, Timeout
    ///
    ProcStatus ProcWaitFor(Proc *proc, u64 timeout_ms);

    ///
    /// Terminate the child process. Sends `SIGTERM` on POSIX (giving
    /// the child a chance to clean up before exiting), calls
    /// `TerminateProcess` on Windows.
    ///
    /// SUCCESS : Returns to the caller. The child has been signalled
    ///           (the kernel reaps it asynchronously; pair with
    ///           `ProcWait` if you need the exit status).
    /// FAILURE : Returns to the caller. Any kill error is logged but
    ///           not surfaced; this call is best-effort.
    ///
    /// TAGS: Proc, Terminate, API
    ///
    void ProcTerminate(Proc *proc);

    ///
    /// Write raw data to the child's stdin pipe.
    ///
    /// SUCCESS: Returns number of bytes written.
    /// FAILURE: Returns -1.
    ///
    /// TAGS: Proc, Write, Stdio
    ///
    i32 ProcWriteToStdin(Proc *proc, const Str *buf);

    ///
    /// Exit code of the child. Only meaningful after `ProcWait` /
    /// `ProcWaitFor` reports completion.
    ///
    /// SUCCESS : Returns the child's exit code.
    /// FAILURE : Returns -1 if the child has not yet exited or `proc`
    ///           is invalid.
    ///
    /// TAGS: Proc, Get, ExitCode
    ///
    i32 ProcGetExitCode(Proc *proc);

    ///
    /// Blocking read from the child's stdout into `buf` (appended).
    ///
    /// SUCCESS : Returns the number of bytes appended to `buf` (>= 0).
    ///           Zero indicates EOF on the pipe.
    /// FAILURE : Returns -1. Any bytes read before the failing read are
    ///           still appended to `buf` (drain is incremental, not
    ///           transactional); the failure cause is logged.
    ///
    /// TAGS: Proc, Read, Stdio
    ///
    i32 ProcReadFromStdout(Proc *proc, Str *buf);

    ///
    /// Blocking read from the child's stderr into `buf` (appended).
    ///
    /// SUCCESS : Returns the number of bytes appended to `buf` (>= 0).
    ///           Zero indicates EOF on the pipe.
    /// FAILURE : Returns -1. Any bytes read before the failing read are
    ///           still appended to `buf` (drain is incremental, not
    ///           transactional); the failure cause is logged.
    ///
    /// TAGS: Proc, Read, Stdio
    ///
    i32 ProcReadFromStderr(Proc *proc, Str *buf);

    ///
    /// OS process ID of the child.
    ///
    /// SUCCESS : Returns a positive pid.
    /// FAILURE : Returns -1 if `proc` is invalid or not yet spawned.
    ///
    /// TAGS: Proc, Get, Identity
    ///
    i32 ProcGetId(Proc *proc);

    ///
    /// Current status of the child without blocking. Useful for
    /// polling alongside `ProcWaitFor`.
    ///
    /// SUCCESS : Returns one of `PROC_STATUS_RUNNING`,
    ///           `PROC_STATUS_COMPLETED`, or
    ///           `PROC_STATUS_TERMINATED`.
    /// FAILURE : Returns `PROC_STATUS_ERROR` if `proc` is invalid
    ///           or the OS query failed (logged).
    ///
    /// TAGS: Proc, Get, Status
    ///
    ProcStatus ProcGetStatus(Proc *proc);

    ///
    /// Resolve the path of the currently running executable. Appends
    /// into `exe_path` (caller initialises the Str).
    ///
    /// SUCCESS : Returns `exe_path` with the resolved path appended.
    /// FAILURE : Returns NULL. `exe_path` may have been partially
    ///           written; the cause is logged.
    ///
    /// TAGS: Proc, Get, Executable, Path
    ///
    Str *GetCurrentExecutablePath(Str *exe_path);

///
/// Blocking read of the child's stdout, then parse the captured bytes
/// with `StrReadFmt` using the supplied format directives. Convenience
/// wrapper around `ProcReadFromStdout` + `StrReadFmt` for the common
/// "drain the pipe, then scan it" pattern; the intermediate `Str` is
/// owned by the macro and released before return.
///
/// p[in]   : Process whose stdout should be drained.
/// ...[in] : `StrReadFmt`-style format string and output pointers.
///
/// SUCCESS : Returns to the caller. The stdout pipe was drained into a
///           transient `Str`, `StrReadFmt` parsed it into the supplied
///           outputs, and the transient buffer has been freed.
/// FAILURE : Returns to the caller. Underlying read/parse errors are
///           logged by the wrapped calls; outputs whose corresponding
///           directive did not match remain at their pre-call values.
///           The transient buffer is freed on every path.
///
/// TAGS: Proc, Read, Stdio, Fmt
///
#define ProcReadFromStdoutFmt(p, ...)                                                                                  \
    do {                                                                                                               \
        Str UNPL(buf) = StrInit();                                                                                     \
        ProcReadFromStdout((p), &UNPL(buf));                                                                           \
        Zstr UNPL(in) = StrBegin(&UNPL(buf));                                                                          \
        StrReadFmt(UNPL(in), __VA_ARGS__);                                                                             \
        StrDeinit(&UNPL(buf));                                                                                         \
    } while (0)

///
/// Blocking read of the child's stderr, then parse the captured bytes
/// with `StrReadFmt` using the supplied format directives. Mirror of
/// `ProcReadFromStdoutFmt` over the stderr pipe.
///
/// p[in]   : Process whose stderr should be drained.
/// ...[in] : `StrReadFmt`-style format string and output pointers.
///
/// SUCCESS : Returns to the caller. The stderr pipe was drained into a
///           transient `Str`, `StrReadFmt` parsed it into the supplied
///           outputs, and the transient buffer has been freed.
/// FAILURE : Returns to the caller. Underlying read/parse errors are
///           logged by the wrapped calls; outputs whose corresponding
///           directive did not match remain at their pre-call values.
///           The transient buffer is freed on every path.
///
/// TAGS: Proc, Read, Stdio, Fmt
///
#define ProcReadFromStderrFmt(p, ...)                                                                                  \
    do {                                                                                                               \
        Str UNPL(buf) = StrInit();                                                                                     \
        ProcReadFromStderr((p), &UNPL(buf));                                                                           \
        Zstr UNPL(in) = StrBegin(&UNPL(buf));                                                                          \
        StrReadFmt(UNPL(in), __VA_ARGS__);                                                                             \
        StrDeinit(&UNPL(buf));                                                                                         \
    } while (0)

///
/// Format a message with `StrAppendFmt` and write the result to the
/// child's stdin. Convenience wrapper for the common "build a line,
/// then push it" pattern; the intermediate `Str` is owned by the macro
/// and released before return.
///
/// p[in]   : Process whose stdin should receive the formatted bytes.
/// ...[in] : `StrAppendFmt`-style format string and arguments.
///
/// SUCCESS : Returns to the caller. The formatted bytes were assembled
///           in a transient `Str`, handed to `ProcWriteToStdin`, and
///           the transient buffer has been freed.
/// FAILURE : Returns to the caller. Underlying format/write errors are
///           logged by the wrapped calls; the transient buffer is
///           freed on every path.
///
/// TAGS: Proc, Write, Stdio, Fmt
///
#define ProcWriteToStdinFmt(p, ...)                                                                                    \
    do {                                                                                                               \
        Str UNPL(buf) = StrInit();                                                                                     \
        StrAppendFmt(&UNPL(buf), __VA_ARGS__);                                                                         \
        ProcWriteToStdin((p), &UNPL(buf));                                                                             \
        StrDeinit(&UNPL(buf));                                                                                         \
    } while (0)

///
/// Like `ProcWriteToStdinFmt`, but appends a trailing newline before
/// writing. Convenience wrapper for line-oriented child stdin protocols.
///
/// p[in]   : Process whose stdin should receive the formatted line.
/// ...[in] : `StrAppendFmt`-style format string and arguments.
///
/// SUCCESS : Returns to the caller. The formatted bytes plus a `'\n'`
///           terminator were assembled in a transient `Str`, handed to
///           `ProcWriteToStdin`, and the transient buffer has been
///           freed.
/// FAILURE : Returns to the caller. Underlying format/write errors are
///           logged by the wrapped calls; the transient buffer is
///           freed on every path.
///
/// TAGS: Proc, Write, Stdio, Fmt, Line
///
#define ProcWriteToStdinFmtLn(p, ...)                                                                                  \
    do {                                                                                                               \
        Str UNPL(buf) = StrInit();                                                                                     \
        StrAppendFmt(&UNPL(buf), __VA_ARGS__);                                                                         \
        StrPushBackR(&UNPL(buf), '\n');                                                                                 \
        ProcWriteToStdin((p), &UNPL(buf));                                                                             \
        StrDeinit(&UNPL(buf));                                                                                         \
    } while (0)

#ifdef __cplusplus
}
#endif

#endif // MISRA_SYS_PROC_H
