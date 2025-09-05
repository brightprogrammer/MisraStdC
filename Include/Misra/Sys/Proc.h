/// file      : Proc.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// System functions for cross-platform process creation and interaction
///

#ifndef MISRA_SYS_PROC_H
#define MISRA_SYS_PROC_H

#include <Misra/Types.h>
#include <Misra/Std/Container/Str.h>

///
/// Opaque platform-dependent handle storing process information.
///
typedef struct SysProc SysProc;

typedef u64 SysProcId;

typedef enum SysProcStatus {
    SYS_PROC_STATUS_RUNNING,    // Process is still running
    SYS_PROC_STATUS_COMPLETED,  // Process completed normally
    SYS_PROC_STATUS_TERMINATED, // Process was terminated/killed
    SYS_PROC_STATUS_ERROR       // Error occurred while checking status
} SysProcStatus;

///
/// Create a new process
///
/// path[in] : Path to executable to be executed.
/// argv[in] : NULL terminated array containing zero-terminated strings.
/// envp[in] : NULL terminated array containing zero-terminated strings.
///
/// SUCCESS: New created `SysProc` object opaque handle.
/// FAILURE: NULL.
///
SysProc* SysProcCreate(const char* path, char** argv, char** envp);

///
/// Block the parent process and wait for provided child process to finish execution.
///
/// proc[in] : Child process handle to wait for.
///
/// SUCCESS: Return
/// FAILURE: Abort with log message.
///
void SysProcWait(SysProc* proc);

///
/// Block the parent process and wait for provided child process to finish execution.
/// This will wait for given time, otherwise will continue execution to parent without
/// closing the child process. Parent may then call terminate if they need the process to end.
///
/// proc[in]    : Child process handle to wait for.
/// timeout[in] : Timeout in milliseconds, 0 for infinite wait.
///
/// SUCCESS: Return
/// FAILURE: Abort with log message.
///
SysProcStatus SysProcWaitFor(SysProc* proc, u64 timeout);

///
/// Terminate the child process
///
/// proc[in] : Child process handle to terminate.
///
/// SUCCESS: Return
/// FAILURE: Abort with log message.
///
void SysProcTerminate(SysProc* proc);

///
/// Terminate the child process and then destroy given `SysProc` structure.
///
/// proc[in] : Child process handle to terminate and destroy.
///
/// SUCCESS: Return
/// FAILURE: Abort with log message.
///
void SysProcDestroy(SysProc* proc);

///
/// Write given raw data to `stdin` of child process.
///
/// proc[in] : Child process handle to write to.
/// buf[in]  : Pointer to `Str` object containing data to be written.
///
/// SUCCESS: Return number of bytes written.
/// FAILURE: Return -1
///
i32 SysProcWriteToStdin(SysProc* proc, Str* buf);

///
/// Get exit code of given child process.
///
/// SUCCESS: Exit code of given child process.
/// FAILURE: Abort with a log message.
///
i32 SysProcGetExitCode(SysProc* proc);

///
/// Perform a blocking read from `stdout` of child process to provided buffer.
///
/// proc[in] : Child process handle to read from.
/// buf[out] : Pointer to `Str` object where the read data will be appended.
///
/// SUCCESS: Return number of bytes read.
/// FAILURE: Return -1
///
i32 SysProcReadFromStdout(SysProc* proc, Str* buf);

///
/// Perform a blocking read from `stderr` of child process to provided buffer.
///
/// proc[in] : Child process handle to read from.
/// buf[out] : Pointer to `Str` object where the read data will be appended.
///
/// SUCCESS: Return number of bytes read.
/// FAILURE: Return -1
///
i32 SysProcReadFromStderr(SysProc* proc, Str* buf);

///
/// Get the OS-specific process ID of a child process.
///
/// proc[in] : Child process handle.
///
/// SUCCESS: Returns the process ID of the child process.
/// FAILURE: Returns -1
///
i32 SysProcGetId(SysProc* proc);

///
/// Check if a child process is still running.
///
/// proc[in] : Child process handle.
///
SysProcStatus SysProcGetStatus(SysProc* proc);

///
/// Platform independent method to get current process Id.
///
/// SUCCESS : Returns current process ID.
/// FAILURE : Function cannot fail - always returns valid ID.
///
/// TAGS: System, Process
///
SysProcId SysGetCurrentProcessId(void);

///
/// Get the path to the current executable.
///
/// exe_path[out] : Str object to store the executable path.
///
/// SUCCESS : Returns initialized Str object with executable path.
/// FAILURE : Returns NULL if path cannot be determined.
///
/// TAGS: System, Process, Path
///
Str* SysGetCurrentExecutablePath(Str* exe_path);

#define SysProcReadFromStdoutFmt(p, ...)                                                                               \
    do {                                                                                                               \
        Str b_ = StrInit();                                                                                            \
        SysProcReadFromStdout((p), &b_);                                                                               \
        const char* in_ = b_.data;                                                                                     \
        StrReadFmt(in_, __VA_ARGS__);                                                                                  \
        StrDeinit(&b_);                                                                                                \
    } while (0)

#define SysProcReadFromStderrFmt(p, ...)                                                                               \
    do {                                                                                                               \
        Str b_ = StrInit();                                                                                            \
        SysProcReadFromStderr((p), &b_);                                                                               \
        const char* in_ = b_.data;                                                                                     \
        StrReadFmt(in_, __VA_ARGS__);                                                                                  \
        StrDeinit(&b_);                                                                                                \
    } while (0)

#define SysProcWriteToStdinFmt(p, ...)                                                                                 \
    do {                                                                                                               \
        Str b_ = StrInit();                                                                                            \
        StrWriteFmt(&b_, __VA_ARGS__);                                                                                 \
        SysProcWriteToStdin((p), &b_);                                                                                 \
        StrDeinit(&b_);                                                                                                \
    } while (0)

#define SysProcWriteToStdinFmtLn(p, ...)                                                                               \
    do {                                                                                                               \
        Str b_ = StrInit();                                                                                            \
        StrWriteFmt(&b_, __VA_ARGS__);                                                                                 \
        StrPushBack(&b_, '\n');                                                                                        \
        SysProcWriteToStdin((p), &b_);                                                                                 \
        StrDeinit(&b_);                                                                                                \
    } while (0)

#endif // MISRA_SYS_PROC_H
