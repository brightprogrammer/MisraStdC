/// file      : Misra/Sys/Proc.h
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
        SYS_PROC_STATUS_RUNNING,    // Process is still running
        SYS_PROC_STATUS_COMPLETED,  // Process completed normally
        SYS_PROC_STATUS_TERMINATED, // Process was terminated/killed
        SYS_PROC_STATUS_ERROR       // Error occurred while checking status
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
    } MisraProcessInfo_;
#endif

    ///
    /// Process handle. Layout is platform-conditional. Stack-declare with
    /// `ProcInit(path, argv, envp)`; don't poke fields directly. The
    /// `_pid` (POSIX) or `_pi.hProcess` (Windows) being zero / NULL means
    /// the init failed -- check with `ProcOk(&p)`.
    ///
    typedef struct Proc {
        int  _exit_code;
        bool _completed;
#if PLATFORM_WINDOWS
        MisraProcessInfo_ _pi;          // layout-compat with PROCESS_INFORMATION
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
    Proc proc_init(const char *path, char **argv, char **envp, Allocator *alloc);

#define ProcInit(...)                       MISRA_OVERLOAD(ProcInit, __VA_ARGS__)
#define ProcInit_3(path, argv, envp)        proc_init((path), (argv), (envp), MisraScope)
#define ProcInit_4(path, argv, envp, alloc) proc_init((path), (argv), (envp), ALLOCATOR_OF(alloc))

    ///
    /// Tear down a `Proc`. Closes any open fds / HANDLEs the parent held
    /// open for the child's stdin/stdout/stderr pipes. Does NOT terminate
    /// the child -- call `ProcTerminate` first if you want that. Safe on
    /// a zeroed (never-spawned) `Proc`.
    ///
    /// TAGS: Sys, Proc, Deinit
    ///
    void ProcDeinit(Proc *p);

    ///
    /// Returns true if `p` represents a successfully-spawned child.
    /// Returns false if `proc_init` failed (or `p` is NULL).
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
    /// SUCCESS : Returns `SYS_PROC_STATUS_COMPLETED` when the child
    ///           exits normally, or `SYS_PROC_STATUS_TERMINATED` when
    ///           the child was killed by a signal / external action.
    /// FAILURE : Returns `SYS_PROC_STATUS_ERROR`; the cause is logged.
    ///
    ProcStatus ProcWait(Proc *proc);

    ///
    /// Block for up to `timeout_ms` milliseconds waiting for the child.
    /// Pass 0 for infinite wait.
    ///
    /// SUCCESS : Returns `SYS_PROC_STATUS_COMPLETED`,
    ///           `SYS_PROC_STATUS_TERMINATED`, or
    ///           `SYS_PROC_STATUS_RUNNING` if the timeout elapsed first.
    /// FAILURE : Returns `SYS_PROC_STATUS_ERROR`; the cause is logged.
    ///
    ProcStatus ProcWaitFor(Proc *proc, u64 timeout_ms);

    ///
    /// Terminate the child process. Sends `SIGKILL` on POSIX,
    /// `TerminateProcess` on Windows.
    ///
    /// SUCCESS : Returns to the caller. The child has been signalled
    ///           (the kernel reaps it asynchronously; pair with
    ///           `ProcWait` if you need the exit status).
    /// FAILURE : Returns to the caller. Any kill error is logged but
    ///           not surfaced; this call is best-effort.
    ///
    void ProcTerminate(Proc *proc);

    ///
    /// Write raw data to the child's stdin pipe.
    ///
    /// SUCCESS: Returns number of bytes written.
    /// FAILURE: Returns -1.
    ///
    i32 ProcWriteToStdin(Proc *proc, Str *buf);

    ///
    /// Exit code of the child. Only meaningful after `ProcWait` /
    /// `ProcWaitFor` reports completion.
    ///
    /// SUCCESS : Returns the child's exit code.
    /// FAILURE : Returns -1 if the child has not yet exited or `proc`
    ///           is invalid.
    ///
    i32 ProcGetExitCode(Proc *proc);

    ///
    /// Blocking read from the child's stdout into `buf` (appended).
    ///
    /// SUCCESS : Returns the number of bytes appended to `buf` (>= 0).
    ///           Zero indicates EOF on the pipe.
    /// FAILURE : Returns -1. `buf` is left in its pre-call state; the
    ///           failure cause is logged.
    ///
    i32 ProcReadFromStdout(Proc *proc, Str *buf);

    ///
    /// Blocking read from the child's stderr into `buf` (appended).
    ///
    /// SUCCESS : Returns the number of bytes appended to `buf` (>= 0).
    ///           Zero indicates EOF on the pipe.
    /// FAILURE : Returns -1. `buf` is left in its pre-call state; the
    ///           failure cause is logged.
    ///
    i32 ProcReadFromStderr(Proc *proc, Str *buf);

    ///
    /// OS process ID of the child.
    ///
    /// SUCCESS : Returns a positive pid.
    /// FAILURE : Returns -1 if `proc` is invalid or not yet spawned.
    ///
    i32 ProcGetId(Proc *proc);

    ///
    /// Current status of the child without blocking. Useful for
    /// polling alongside `ProcWaitFor`.
    ///
    /// SUCCESS : Returns one of `SYS_PROC_STATUS_RUNNING`,
    ///           `SYS_PROC_STATUS_COMPLETED`, or
    ///           `SYS_PROC_STATUS_TERMINATED`.
    /// FAILURE : Returns `SYS_PROC_STATUS_ERROR` if `proc` is invalid
    ///           or the OS query failed (logged).
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
    Str *GetCurrentExecutablePath(Str *exe_path);

#define ProcReadFromStdoutFmt(p, ...)                                                                                  \
    do {                                                                                                               \
        Str UNPL(buf) = StrInit();                                                                                     \
        ProcReadFromStdout((p), &UNPL(buf));                                                                           \
        const char *UNPL(in) = UNPL(buf).data;                                                                         \
        StrReadFmt(UNPL(in), __VA_ARGS__);                                                                             \
        StrDeinit(&UNPL(buf));                                                                                         \
    } while (0)

#define ProcReadFromStderrFmt(p, ...)                                                                                  \
    do {                                                                                                               \
        Str UNPL(buf) = StrInit();                                                                                     \
        ProcReadFromStderr((p), &UNPL(buf));                                                                           \
        const char *UNPL(in) = UNPL(buf).data;                                                                         \
        StrReadFmt(UNPL(in), __VA_ARGS__);                                                                             \
        StrDeinit(&UNPL(buf));                                                                                         \
    } while (0)

#define ProcWriteToStdinFmt(p, ...)                                                                                    \
    do {                                                                                                               \
        Str UNPL(buf) = StrInit();                                                                                     \
        StrAppendFmt(&UNPL(buf), __VA_ARGS__);                                                                         \
        ProcWriteToStdin((p), &UNPL(buf));                                                                             \
        StrDeinit(&UNPL(buf));                                                                                         \
    } while (0)

#define ProcWriteToStdinFmtLn(p, ...)                                                                                  \
    do {                                                                                                               \
        Str UNPL(buf) = StrInit();                                                                                     \
        StrAppendFmt(&UNPL(buf), __VA_ARGS__);                                                                         \
        StrPushBack(&UNPL(buf), '\n');                                                                                 \
        ProcWriteToStdin((p), &UNPL(buf));                                                                             \
        StrDeinit(&UNPL(buf));                                                                                         \
    } while (0)

#ifdef __cplusplus
}
#endif

#endif // MISRA_SYS_PROC_H
