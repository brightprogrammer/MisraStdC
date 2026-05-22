/// file      : Proc.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// System functions for cross-platform process creation and interaction
///


// kill, readlink and usleep don't work without this
#define _DEFAULT_SOURCE

#include <Misra/Sys/Proc.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Sys.h>
#include <Misra/Std/Memory.h>
#include <Misra/Std/Log.h>
#include "../_Syscall.h"
#if PLATFORM_WINDOWS
#    include <windows.h>
#    include <tlhelp32.h>
#    include <psapi.h>
#    include <signal.h>
#    include <io.h>
#    define FILENO _fileno
#else
#    include <dirent.h>
#    include <pthread.h>
#    include <sys/stat.h>
#    include <sys/wait.h>
#    include <fcntl.h>
#    include <signal.h>
#    include <unistd.h>
#    if PLATFORM_DARWIN
#        include <mach-o/dyld.h>
#    endif
#    define FILENO fileno
#endif

// Sleep for `us` microseconds. Linux: direct nanosleep syscall.
// macOS / BSD: nanosleep from libSystem. Windows: kernel32 Sleep.
static inline void proc_sleep_us(u64 us) {
#if PLATFORM_WINDOWS
    Sleep((DWORD)(us / 1000));
#elif FEATURE_DIRECT_SYSCALL
    // struct __kernel_timespec is `long sec; long nsec;` on 64-bit Linux.
    struct {
        long sec;
        long nsec;
    } ts;
    ts.sec  = (long)(us / 1000000);
    ts.nsec = (long)((us % 1000000) * 1000);
    (void)misra_sys2(MISRA_SYS_nanosleep, (long)(u64)&ts, 0);
#else
    struct timespec ts = {(time_t)(us / 1000000), (long)((us % 1000000) * 1000)};
    nanosleep(&ts, NULL);
#endif
}

#if FEATURE_DIRECT_SYSCALL
// Linux: thin direct-syscall wrappers for the POSIX I/O / process
// primitives used below. macOS / BSD keep libSystem (Apple disallows
// direct user syscalls); Windows takes a different code path entirely.
//
// The aarch64 syscall table dropped the "legacy" variants (open,
// stat, fork, pipe, dup2, readlink, ...) so each wrapper handles
// the x86_64 vs aarch64 ABI divergence inline.

static inline long misra_proc_close(int fd) {
    return misra_sys1(MISRA_SYS_close, (long)fd);
}
static inline long misra_proc_read(int fd, void *buf, unsigned long n) {
    return misra_sys3(MISRA_SYS_read, (long)fd, (long)(u64)buf, (long)n);
}
static inline long misra_proc_write(int fd, const void *buf, unsigned long n) {
    return misra_sys3(MISRA_SYS_write, (long)fd, (long)(u64)buf, (long)n);
}
static inline long misra_proc_pipe(int fds[2]) {
#    if PLATFORM_DARWIN
    // Darwin pipe ignores its arg and returns fds in registers.
    return misra_darwin_pipe(fds);
#    elif ARCHITECTURE_X86_64
    return misra_sys1(MISRA_SYS_pipe, (long)(u64)fds);
#    else
    return misra_sys2(MISRA_SYS_pipe2, (long)(u64)fds, 0);
#    endif
}
static inline long misra_proc_dup2(int oldfd, int newfd) {
#    if PLATFORM_DARWIN || ARCHITECTURE_X86_64
    return misra_sys2(MISRA_SYS_dup2, (long)oldfd, (long)newfd);
#    else
    return misra_sys3(MISRA_SYS_dup3, (long)oldfd, (long)newfd, 0);
#    endif
}
static inline long misra_proc_fork(void) {
#    if PLATFORM_DARWIN || ARCHITECTURE_X86_64
    // Darwin has fork (#2); Linux x86_64 has fork (#57). Same shape:
    // returns 0 in child, pid in parent.
    return misra_sys0(MISRA_SYS_fork);
#    else
    // Linux aarch64: no SYS_fork. clone(SIGCHLD, NULL, NULL, NULL, NULL).
    // SIGCHLD = 17 on Linux.
    return misra_sys5(MISRA_SYS_clone, 17, 0, 0, 0, 0);
#    endif
}
static inline long misra_proc_execve(const char *path, char *const *argv, char *const *envp) {
    return misra_sys3(MISRA_SYS_execve, (long)(u64)path, (long)(u64)argv, (long)(u64)envp);
}
static inline long misra_proc_kill(int pid, int sig) {
    return misra_sys2(MISRA_SYS_kill, (long)pid, (long)sig);
}
static inline long misra_proc_readlink(const char *path, char *buf, unsigned long sz) {
#    if PLATFORM_DARWIN || ARCHITECTURE_X86_64
    return misra_sys3(MISRA_SYS_readlink, (long)(u64)path, (long)(u64)buf, (long)sz);
#    else
    // Linux aarch64: no SYS_readlink. AT_FDCWD = -100.
    return misra_sys4(MISRA_SYS_readlinkat, -100L, (long)(u64)path, (long)(u64)buf, (long)sz);
#    endif
}
static inline long misra_proc_waitpid(int pid, int *status, int options) {
    return misra_sys4(MISRA_SYS_wait4, (long)pid, (long)(u64)status, (long)options, 0);
}

// Macro shims so the existing POSIX call sites use our direct-syscall
// wrappers without per-line edits. Each returns the kernel's value
// (negative = -errno, otherwise success), and the callers already
// handle the "< 0" failure shape that POSIX wrappers expose. No outer
// cast: callers that bind the result get the implicit conversion
// (long -> int / pid_t / etc.), and callers that discard the result
// don't get `-Wunused-value` from a cast in expression-statement
// position.
#    define close(fd)                  misra_proc_close(fd)
#    define read(fd, buf, n)           misra_proc_read((fd), (buf), (n))
#    define write(fd, buf, n)          misra_proc_write((fd), (buf), (n))
#    define pipe(fds)                  misra_proc_pipe(fds)
#    define dup2(oldfd, newfd)         misra_proc_dup2((oldfd), (newfd))
#    define fork()                     misra_proc_fork()
#    define execve(p, a, e)            misra_proc_execve((p), (a), (e))
#    define kill(pid, sig)             misra_proc_kill((pid), (sig))
#    define readlink(p, b, n)          misra_proc_readlink((p), (b), (n))
#    define waitpid(pid, status, opts) misra_proc_waitpid((pid), (status), (opts))
#endif

#ifndef STDIN_FILENO
#    define STDIN_FILENO FILENO(stdin)
#endif
#ifndef STDOUT_FILENO
#    define STDOUT_FILENO FILENO(stdout)
#endif
#ifndef STDERR_FILENO
#    define STDERR_FILENO FILENO(stderr)
#endif

// struct Proc lives in <Misra/Sys/Proc.h> -- exposed so callers can
// stack-declare and initialise via ProcInit(...). Header field names
// are prefixed with `_` to flag them as implementation detail.

#define READ_END  0
#define WRITE_END 1

Proc proc_init(const char *filepath, char **argv, char **envp, Allocator *alloc) {
    Proc proc = {0};
#if PLATFORM_UNIX
    (void)alloc; // POSIX path doesn't need an allocator
    int stdin_pipe[2]  = {-1};
    int stdout_pipe[2] = {-1};
    int stderr_pipe[2] = {-1};

    // Capture the failing-call return so the error log can name the
    // errno without having to read libc's `errno` TLS slot. On
    // Linux+direct-syscall this is the kernel's -errno; on macOS
    // libSystem it's just -1 and ErrnoOf falls back to errno.
    long pipe_ret = pipe(stdin_pipe);
    if (pipe_ret == 0)
        pipe_ret = pipe(stdout_pipe);
    if (pipe_ret == 0)
        pipe_ret = pipe(stderr_pipe);
    if (pipe_ret < 0) {
        LOG_SYS_ERROR(ErrnoOf(pipe_ret), "pipe() failed");
        if (stdin_pipe[READ_END] >= 0)
            close(stdin_pipe[READ_END]);
        if (stdout_pipe[READ_END] >= 0)
            close(stdout_pipe[READ_END]);
        if (stderr_pipe[READ_END] >= 0)
            close(stderr_pipe[READ_END]);
        if (stdin_pipe[WRITE_END] >= 0)
            close(stdin_pipe[WRITE_END]);
        if (stdout_pipe[WRITE_END] >= 0)
            close(stdout_pipe[WRITE_END]);
        if (stderr_pipe[WRITE_END] >= 0)
            close(stderr_pipe[WRITE_END]);
        return proc; // _pid == 0 -> ProcOk() returns false
    }

    pid_t pid = fork();
    if (pid < 0) {
        LOG_SYS_ERROR(ErrnoOf(pid), "fork");
        close(stdin_pipe[READ_END]);
        close(stdout_pipe[READ_END]);
        close(stderr_pipe[READ_END]);
        close(stdin_pipe[WRITE_END]);
        close(stdout_pipe[WRITE_END]);
        close(stderr_pipe[WRITE_END]);
        return proc;
    }

    if (pid == 0) {
        // Child: wire stdin/stdout/stderr to pipe ends, then exec.
        dup2(stdin_pipe[READ_END], STDIN_FILENO);
        dup2(stdout_pipe[WRITE_END], STDOUT_FILENO);
        dup2(stderr_pipe[WRITE_END], STDERR_FILENO);
        close(stdin_pipe[WRITE_END]);
        close(stdout_pipe[READ_END]);
        close(stderr_pipe[READ_END]);

        long exec_ret = execve(filepath, argv, envp);

        // Only reached if execve failed.
        LOG_SYS_ERROR(ErrnoOf(exec_ret), "execve() failed");
        close(stdin_pipe[READ_END]);
        close(stdout_pipe[READ_END]);
        close(stderr_pipe[READ_END]);
        close(stdin_pipe[WRITE_END]);
        close(stdout_pipe[WRITE_END]);
        close(stderr_pipe[WRITE_END]);
        // Best-effort exit; we can't return from the child.
        return proc;
    }

    // Parent: close the ends only the child uses.
    close(stdin_pipe[READ_END]);
    close(stdout_pipe[WRITE_END]);
    close(stderr_pipe[WRITE_END]);

    proc._pid       = pid;
    proc._stdin_fd  = stdin_pipe[WRITE_END];
    proc._stdout_fd = stdout_pipe[READ_END];
    proc._stderr_fd = stderr_pipe[READ_END];
    return proc;
#else
    HANDLE              hStdinRead = NULL, hStdinWrite = NULL;
    HANDLE              hStdoutRead = NULL, hStdoutWrite = NULL;
    HANDLE              hStderrRead = NULL, hStderrWrite = NULL;
    SECURITY_ATTRIBUTES sa = {sizeof(sa), NULL, TRUE};

    if (!CreatePipe(&hStdinRead, &hStdinWrite, &sa, 0) || !CreatePipe(&hStdoutRead, &hStdoutWrite, &sa, 0) ||
        !CreatePipe(&hStderrRead, &hStderrWrite, &sa, 0)) {
        LOG_ERROR("CreatePipe failed (GetLastError={})", (i32)GetLastError());
        if (hStdinRead)
            CloseHandle(hStdinRead);
        if (hStdinWrite)
            CloseHandle(hStdinWrite);
        if (hStdoutRead)
            CloseHandle(hStdoutRead);
        if (hStdoutWrite)
            CloseHandle(hStdoutWrite);
        if (hStderrRead)
            CloseHandle(hStderrRead);
        if (hStderrWrite)
            CloseHandle(hStderrWrite);
        return proc;
    }

    SetHandleInformation(hStderrRead, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(hStdoutRead, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(hStdinWrite, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFO si = {sizeof(si)};
    si.dwFlags     = STARTF_USESTDHANDLES;
    si.hStdInput   = hStdinRead;
    si.hStdOutput  = hStdoutWrite;
    si.hStdError   = hStderrWrite;

    PROCESS_INFORMATION pi = {0};

    Str cmdline = StrInit(alloc);
    StrPushBackZstr(&cmdline, filepath);
    for (char **arg = argv + 1; *arg; ++arg) {
        StrPushBack(&cmdline, ' ');
        StrPushBackZstr(&cmdline, *arg);
    }

    if (!CreateProcessA(NULL, cmdline.data, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
        LOG_ERROR("CreateProcessA() failed (GetLastError={})", (i32)GetLastError());
        StrDeinit(&cmdline);
        CloseHandle(hStdinRead);
        CloseHandle(hStdinWrite);
        CloseHandle(hStdoutRead);
        CloseHandle(hStdoutWrite);
        CloseHandle(hStderrRead);
        CloseHandle(hStderrWrite);
        return proc;
    }
    StrDeinit(&cmdline);

    CloseHandle(hStdinRead);
    CloseHandle(hStdoutWrite);
    CloseHandle(hStderrWrite);

    // Copy PROCESS_INFORMATION fields into our layout-compatible
    // struct. We can't whole-struct-assign because the public header
    // declares `_pi` as MisraProcessInfo_ (so it doesn't have to
    // pull <windows.h>); the field shape matches but the types are
    // distinct at the C level.
    proc._pi.hProcess    = pi.hProcess;
    proc._pi.hThread     = pi.hThread;
    proc._pi.dwProcessId = (u32)pi.dwProcessId;
    proc._pi.dwThreadId  = (u32)pi.dwThreadId;
    proc._hStdinWrite    = hStdinWrite;
    proc._hStdoutRead    = hStdoutRead;
    proc._hStderrRead    = hStderrRead;

    return proc;
#endif
}

ProcStatus ProcWait(Proc *proc) {
    if (!proc) {
        LOG_FATAL("Invalid argument");
    }

#if PLATFORM_UNIX
    int  status;
    long wait_ret = waitpid(proc->_pid, &status, 0);
    if (wait_ret < 0) {
        LOG_SYS_ERROR(ErrnoOf(wait_ret), "Failed to wait for child process");
        return SYS_PROC_STATUS_ERROR;
    }

    proc->_completed = true;

    if (WIFEXITED(status)) {
        proc->_exit_code = WEXITSTATUS(status);
        return SYS_PROC_STATUS_COMPLETED;
    } else if (WIFSIGNALED(status)) {
        proc->_exit_code = 128 + WTERMSIG(status);
        return SYS_PROC_STATUS_TERMINATED;
    } else {
        proc->_exit_code = -1; // Unknown termination
        return SYS_PROC_STATUS_ERROR;
    }

#else
    if (WAIT_FAILED == WaitForSingleObject(proc->_pi.hProcess, INFINITE)) {
        // Win32: WaitForSingleObject uses GetLastError, not errno. Log
        // explicitly; LOG_SYS_ERROR's first arg is unused on Windows.
        LOG_ERROR("Failed to wait for child process (GetLastError={})", (i32)GetLastError());
        return SYS_PROC_STATUS_ERROR;
    }

    proc->_completed = true;

    DWORD code = 0;
    if (!GetExitCodeProcess(proc->_pi.hProcess, &code)) {
        proc->_exit_code = -1;
        return SYS_PROC_STATUS_ERROR;
    } else {
        proc->_exit_code = (i32)code;
        return SYS_PROC_STATUS_COMPLETED;
    }
#endif
}

ProcStatus ProcWaitFor(Proc *proc, u64 timeout_ms) {
    if (!proc) {
        LOG_FATAL("Invalid arguments");
    }

#if PLATFORM_WINDOWS
    DWORD wait_time = (timeout_ms == 0) ? INFINITE : (DWORD)timeout_ms;
    DWORD result    = WaitForSingleObject(proc->_pi.hProcess, wait_time);

    switch (result) {
        case WAIT_OBJECT_0 : {
            DWORD code_dw = 0;
            if (GetExitCodeProcess(proc->_pi.hProcess, &code_dw)) {
                proc->_exit_code = (int)code_dw;
                proc->_completed = true;
                return SYS_PROC_STATUS_COMPLETED;
            } else {
                return SYS_PROC_STATUS_ERROR;
            }
        }
        case WAIT_TIMEOUT :
            return SYS_PROC_STATUS_RUNNING;
        default :
            return SYS_PROC_STATUS_ERROR;
    }

#else
    int   status = 0;
    pid_t res;

    if (timeout_ms == 0) {
        // Infinite blocking wait
        res = waitpid(proc->_pid, &status, 0);
    } else {
        // Simulate timeout using polling
        u64       elapsed_ms        = 0;
        const u64 sleep_interval_ms = 10;

        while (elapsed_ms < timeout_ms) {
            res = waitpid(proc->_pid, &status, WNOHANG);
            if (res == -1) {
                return SYS_PROC_STATUS_ERROR;
            } else if (res == 0) {
                proc_sleep_us(sleep_interval_ms * 1000);
                elapsed_ms += sleep_interval_ms;
                continue;
            } else {
                break; // Process exited
            }
        }

        if (elapsed_ms >= timeout_ms) {
            return SYS_PROC_STATUS_RUNNING;
        }
    }

    if (res == proc->_pid) {
        proc->_completed = true;
        if (WIFEXITED(status)) {
            proc->_exit_code = WEXITSTATUS(status);
            return SYS_PROC_STATUS_COMPLETED;
        } else if (WIFSIGNALED(status)) {
            proc->_exit_code = 128 + WTERMSIG(status);
            return SYS_PROC_STATUS_TERMINATED;
        }
    }

    return SYS_PROC_STATUS_ERROR;
#endif
}

void ProcTerminate(Proc *proc) {
    if (!proc) {
        LOG_FATAL("Invalid argument");
    }

    if (proc->_completed) {
        return;
    }

#if PLATFORM_UNIX
    long kill_ret = kill(proc->_pid, SIGTERM);
    if (kill_ret < 0) {
        LOG_SYS_ERROR(ErrnoOf(kill_ret), "kill(pid, SIGTERM) failed");
    }

    // Now wait for it to exit and capture the exit code
    int  status;
    long wait_ret = waitpid(proc->_pid, &status, 0);
    if (wait_ret < 0) {
        LOG_SYS_ERROR(ErrnoOf(wait_ret), "waitpid after SIGTERM failed");
        return;
    }

    proc->_completed = true;

    if (WIFEXITED(status)) {
        proc->_exit_code = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        proc->_exit_code = 128 + WTERMSIG(status);
    } else {
        proc->_exit_code = -1; // Unknown
    }

#else
    if (!TerminateProcess(proc->_pi.hProcess, 1)) {
        LOG_ERROR("TerminateProcess failed (GetLastError={})", (i32)GetLastError());
        return;
    }

    // Wait for it to actually exit
    if (WAIT_FAILED == WaitForSingleObject(proc->_pi.hProcess, INFINITE)) {
        LOG_ERROR("WaitForSingleObject after TerminateProcess failed (GetLastError={})", (i32)GetLastError());
        return;
    }

    DWORD code = 0;
    if (!GetExitCodeProcess(proc->_pi.hProcess, &code)) {
        proc->_exit_code = -1;
    } else {
        proc->_exit_code = (i32)code;
    }

    proc->_completed = true;
#endif
}


void ProcDeinit(Proc *proc) {
    if (!proc) {
        return;
    }
    // Safe on a zeroed (never-spawned) Proc -- ProcOk gates the
    // child-side cleanup; we always zero-and-return at the end.
    if (ProcOk(proc)) {
        ProcTerminate(proc);
#if PLATFORM_UNIX
        close(proc->_stdin_fd);
        close(proc->_stdout_fd);
        close(proc->_stderr_fd);
#else
        CloseHandle(proc->_hStdinWrite);
        CloseHandle(proc->_hStdoutRead);
        CloseHandle(proc->_hStderrRead);
        CloseHandle(proc->_pi.hThread);
        CloseHandle(proc->_pi.hProcess);
#endif
    }
    MemSet(proc, 0, sizeof(*proc));
}

i32 ProcWriteToStdin(Proc *proc, Str *buf) {
    if (!proc || !buf) {
        LOG_FATAL("Invalid arguments");
    }

#if PLATFORM_UNIX
    return write(proc->_stdin_fd, buf->data, buf->length);
#else
    DWORD written = 0;
    if (!WriteFile(proc->_hStdinWrite, buf->data, buf->length, &written, NULL))
        return -1;
    return (int)written;
#endif
}

i32 sys_proc_read_internal(Proc *proc, Str *buf, bool is_stdout) {
    if (!proc || !buf) {
        LOG_FATAL("Invalid argument");
    }

    // Signed so the -1 error sentinel is honest and the final cast
    // to the function's i32 return type doesn't have to round-trip
    // through u64. A subprocess producing >2 GiB on a single stream
    // still saturates the i32 return -- documented limitation.
    i64  total_read   = 0;
    char tmpbuf[1024] = {0};

#if PLATFORM_UNIX
    i32 rfd = is_stdout ? proc->_stdout_fd : proc->_stderr_fd;

    // // Save original flags and switch to blocking
    // int flags = fcntl(rfd, F_GETFL, 0);
    // if (flags == -1) {
    //     LOG_SYS_ERROR("fcntl get failed");
    //     return -1;
    // }

    // // set to non-blocking read
    // if (flags & O_NONBLOCK) {
    //     if (fcntl(rfd, F_SETFL, flags & ~O_NONBLOCK) == -1) {
    //         LOG_SYS_ERROR("fcntl set blocking failed");
    //         return -1;
    //     }
    // }

    while (true) {
        ssize_t n = read(rfd, tmpbuf, 1023);
        if (n > 0) {
            StrPushBackCstr(buf, tmpbuf, n);
            total_read += n;
        } else if (n == 0) {
            // EOF
            break;
        } else {
            // Direct-syscall path returns -EINTR; libc returns -1 + sets errno.
#    if FEATURE_DIRECT_SYSCALL
            if (n == -EINTR)
                continue;
#    else
            if (Errno() == EINTR)
                continue;
#    endif
            LOG_SYS_ERROR(ErrnoOf(n), "read failed");
            total_read = -1;
            break;
        }
    }

    // // Restore non-blocking mode if it was set
    // if (flags & O_NONBLOCK) {
    //     if (fcntl(rfd, F_SETFL, flags) == -1) {
    //         LOG_SYS_ERROR("fcntl restore");
    //         return -1;
    //     }
    // }
#else
    while (true) {
        DWORD  available = 0;
        HANDLE rhandle   = is_stdout ? proc->_hStdoutRead : proc->_hStderrRead;

        if (!PeekNamedPipe(rhandle, NULL, 0, NULL, &available, NULL)) {
            LOG_ERROR("PeekNamedPipe failed (GetLastError={})", (i32)GetLastError());
            return -1;
        }

        if (available == 0) {
            // EOF or no data
            break;
        }

        DWORD bytes_read = 0;

        if (!ReadFile(rhandle, tmpbuf, 1023, &bytes_read, NULL)) {
            LOG_ERROR("ReadFile failed (GetLastError={})", (i32)GetLastError());
            return -1;
        }

        if (bytes_read == 0) {
            break;
        }

        StrPushBackCstr(buf, tmpbuf, bytes_read);
        total_read += bytes_read;
    }
#endif

    return (i32)total_read;
}

i32 ProcReadFromStdout(Proc *proc, Str *buf) {
    return sys_proc_read_internal(proc, buf, /* is stdout*/ true);
}

i32 ProcReadFromStderr(Proc *proc, Str *buf) {
    return sys_proc_read_internal(proc, buf, /* is stdout*/ false);
}

i32 ProcGetId(Proc *proc) {
    if (!proc) {
        LOG_FATAL("Invalid argument");
    }

#if PLATFORM_UNIX
    return proc->_pid;
#else
    return (i32)proc->_pi.dwProcessId;
#endif
}

i32 ProcIsRunning(Proc *proc) {
    if (!proc) {
        LOG_FATAL("Invalid argument");
    }

#if PLATFORM_UNIX
    int   status;
    pid_t result = waitpid(proc->_pid, &status, WNOHANG);
    if (result == 0) {
        return 1;  // Still running
    } else if (result == proc->_pid) {
        return 0;  // Exited
    } else {
        return -1; // Error
    }
#else
    DWORD code = 0;
    if (!GetExitCodeProcess(proc->_pi.hProcess, &code)) {
        return -1; // API error
    }
    return (code == STILL_ACTIVE) ? 1 : 0;
#endif
}

i32 ProcGetExitCode(Proc *proc) {
    if (!proc) {
        LOG_FATAL("Invalid argument");
    }

    if (!proc->_completed) {
        return -1; // Cannot get exit code if not completed
    }

#if PLATFORM_WINDOWS
    DWORD code;
    if (GetExitCodeProcess(proc->_pi.hProcess, &code)) {
        return (i32)code;
    }
    return -1;
#else
    return proc->_exit_code; // Already stored during wait
#endif
}

Str *GetCurrentExecutablePath(Str *exe_path) {
    ValidateStr(exe_path);
    Allocator *alloc = exe_path->allocator;

#if PLATFORM_WINDOWS
    char  buffer[MAX_PATH];
    DWORD len = GetModuleFileNameA(NULL, buffer, MAX_PATH);
    if (len == 0 || len >= MAX_PATH) {
        LOG_ERROR("Failed to get executable path or buffer too small");
        return NULL;
    }
    *exe_path = StrInitFromCstr(buffer, ZstrLen(buffer), alloc);
    return exe_path;
#else
    char buffer[4096]; // Large buffer for Unix paths

    // Try /proc/self/exe first (Linux)
    ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (len != -1) {
        buffer[len] = '\0';
        *exe_path   = StrInitFromCstr(buffer, ZstrLen(buffer), alloc);
        return exe_path;
    }

// Fallback for macOS.
//
// Apple's dlsym-style API for "what's my exe path" is
// _NSGetExecutablePath (libSystem). Replacement that fits in the
// allowed __dyld_* set: _dyld_get_image_name(0). Image index 0 is
// always the main executable Mach-O; the returned pointer lives in
// dyld's internal tables (don't free, don't outlive the process,
// which is fine for our copy-into-Str use here). Same call as
// Sys/Backtrace already makes per-frame.
#    if PLATFORM_DARWIN
    extern const char *_dyld_get_image_name(u32 image_index);
    const char        *exe = _dyld_get_image_name(0);
    if (exe) {
        *exe_path = StrInitFromCstr(exe, ZstrLen(exe), alloc);
        return exe_path;
    }
#    endif

    return NULL;
#endif
}
