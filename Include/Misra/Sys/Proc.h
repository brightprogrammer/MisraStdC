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

#ifdef _WIN32
#    ifndef WIN32_LEAN_AND_MEAN
#        define WIN32_LEAN_AND_MEAN
#    endif
#    include <windows.h>
#else
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

///
/// Process handle. Layout is platform-conditional. Stack-declare with
/// `ProcInit(path, argv, envp)`; don't poke fields directly. The
/// `_pid` (POSIX) or `_pi.hProcess` (Windows) being zero / NULL means
/// the init failed -- check with `ProcOk(&p)`.
///
typedef struct Proc {
    int  _exit_code;
    bool _completed;
#ifdef _WIN32
    PROCESS_INFORMATION _pi;
    HANDLE              _hStdinWrite;
    HANDLE              _hStdoutRead;
    HANDLE              _hStderrRead;
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

#define ProcInit(...)                          MISRA_OVERLOAD(ProcInit, __VA_ARGS__)
#define ProcInit_3(path, argv, envp)           proc_init((path), (argv), (envp), MisraScope)
#define ProcInit_4(path, argv, envp, alloc)    proc_init((path), (argv), (envp), ALLOCATOR_OF(alloc))

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
#ifdef _WIN32
    return p->_pi.hProcess != NULL;
#else
    return p->_pid > 0;
#endif
}

///
/// Block until the child exits.
///
/// RETURNS: `SYS_PROC_STATUS_COMPLETED`, `SYS_PROC_STATUS_TERMINATED`,
///          or `SYS_PROC_STATUS_ERROR`.
///
ProcStatus ProcWait(Proc *proc);

///
/// Block for up to `timeout_ms` milliseconds waiting for the child.
/// Pass 0 for infinite wait. Returns current status; if the child is
/// still running after the timeout, returns `SYS_PROC_STATUS_RUNNING`.
///
ProcStatus ProcWaitFor(Proc *proc, u64 timeout_ms);

///
/// Terminate the child process.
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
/// Exit code of the child. Only meaningful after `ProcWait`.
///
i32 ProcGetExitCode(Proc *proc);

///
/// Blocking read from child's stdout into `buf` (appended).
///
i32 ProcReadFromStdout(Proc *proc, Str *buf);

///
/// Blocking read from child's stderr into `buf` (appended).
///
i32 ProcReadFromStderr(Proc *proc, Str *buf);

///
/// Returns the OS process ID of the child, or -1 if `proc` is invalid.
///
i32 ProcGetId(Proc *proc);

///
/// Returns the current status of the child.
///
ProcStatus ProcGetStatus(Proc *proc);

///
/// Get the path to the current executable.
///
Str *GetCurrentExecutablePath(Str *exe_path);

#define ProcReadFromStdoutFmt(p, ...)                                                                                  \
    do {                                                                                                               \
        Str b_ = StrInit();                                                                                            \
        ProcReadFromStdout((p), &b_);                                                                                  \
        const char *in_ = b_.data;                                                                                     \
        StrReadFmt(in_, __VA_ARGS__);                                                                                  \
        StrDeinit(&b_);                                                                                                \
    } while (0)

#define ProcReadFromStderrFmt(p, ...)                                                                                  \
    do {                                                                                                               \
        Str b_ = StrInit();                                                                                            \
        ProcReadFromStderr((p), &b_);                                                                                  \
        const char *in_ = b_.data;                                                                                     \
        StrReadFmt(in_, __VA_ARGS__);                                                                                  \
        StrDeinit(&b_);                                                                                                \
    } while (0)

#define ProcWriteToStdinFmt(p, ...)                                                                                    \
    do {                                                                                                               \
        Str b_ = StrInit();                                                                                            \
        StrWriteFmt(&b_, __VA_ARGS__);                                                                                 \
        ProcWriteToStdin((p), &b_);                                                                                    \
        StrDeinit(&b_);                                                                                                \
    } while (0)

#define ProcWriteToStdinFmtLn(p, ...)                                                                                  \
    do {                                                                                                               \
        Str b_ = StrInit();                                                                                            \
        StrWriteFmt(&b_, __VA_ARGS__);                                                                                 \
        StrPushBack(&b_, '\n');                                                                                        \
        ProcWriteToStdin((p), &b_);                                                                                    \
        StrDeinit(&b_);                                                                                                \
    } while (0)

#ifdef __cplusplus
}
#endif

#endif // MISRA_SYS_PROC_H
