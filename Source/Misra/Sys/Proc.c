/// file      : Proc.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// System functions for cross-platform process creation and interaction
///


// kill, readlink and usleep don't work without this
#define _DEFAULT_SOURCE

#include <Misra/Sys/Proc.h>
#include <Misra/Sys.h>
#include <Misra/Std/Memory.h>
#include <Misra/Std/Log.h>

#ifdef _WIN32
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
#    ifdef __APPLE__
#        include <mach-o/dyld.h>
#    endif
#    define FILENO fileno
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

struct Proc {
    int  exit_code;
    bool completed;
#if defined(__APPLE__) || defined(__linux__)
    pid_t pid;
    int   stdin_fd;  // write here to send to child stdin
    int   stdout_fd; // read here from child stdout
    int   stderr_fd; // read here from child error output
#elif defined(_WIN32)
    PROCESS_INFORMATION pi;
    HANDLE              hStdinWrite;
    HANDLE              hStdoutRead;
    HANDLE              hStderrRead;
#else
#    error "Unsupported OS for Proc"
#endif
};

#define READ_END  0
#define WRITE_END 1

Proc *ProcCreate(const char *filepath, char **argv, char **envp, Allocator *alloc) {
    if (!alloc) {
        LOG_FATAL("ProcCreate requires an allocator");
    }
#if defined(__APPLE__) || defined(__linux__)
    int stdin_pipe[2]  = {-1};
    int stdout_pipe[2] = {-1};
    int stderr_pipe[2] = {-1};

    if (pipe(stdin_pipe) == -1 || pipe(stdout_pipe) == -1 || pipe(stderr_pipe) == -1) {
        LOG_SYS_ERROR("pipe() failed");
        if (stdin_pipe[READ_END])
            close(stdin_pipe[READ_END]);
        if (stdout_pipe[READ_END])
            close(stdout_pipe[READ_END]);
        if (stderr_pipe[READ_END])
            close(stderr_pipe[READ_END]);
        if (stdin_pipe[WRITE_END])
            close(stdin_pipe[WRITE_END]);
        if (stdout_pipe[WRITE_END])
            close(stdout_pipe[WRITE_END]);
        if (stderr_pipe[WRITE_END])
            close(stderr_pipe[WRITE_END]);
        return NULL;
    }

    pid_t pid = fork();
    if (pid < 0) {
        LOG_SYS_ERROR("fork");

        close(stdin_pipe[READ_END]);
        close(stdout_pipe[READ_END]);
        close(stderr_pipe[READ_END]);
        close(stdin_pipe[WRITE_END]);
        close(stdout_pipe[WRITE_END]);
        close(stderr_pipe[WRITE_END]);

        return NULL;
    }

    if (pid == 0) {
        // parent will write to WRITE_END of stdin_pipe
        // parent will read from READ_END of stdout_pipe
        // parent will read from READ_END of stderr_pipe
        dup2(stdin_pipe[READ_END], STDIN_FILENO);    // child's stdin is READ_END of pipe
        dup2(stdout_pipe[WRITE_END], STDOUT_FILENO); // child's stdout is WRITE_END of pipe
        dup2(stderr_pipe[WRITE_END], STDERR_FILENO); // child's stderr is WRITE_END of pipe

        // child does not write to WRITE_END of stdin_pipe, child uses READ_ENd
        // child does not read from READ_END of stdout_pipe, child uses WRITE_END
        // child does not read from READ_END of stderr_pipe, child uses WRITE_END
        close(stdin_pipe[WRITE_END]);
        close(stdout_pipe[READ_END]);
        close(stderr_pipe[READ_END]);

        // execute child process
        execve(filepath, argv, envp);

        // if continues then this is bad, execve failed
        LOG_SYS_ERROR("execve() failed");

        close(stdin_pipe[READ_END]);
        close(stdout_pipe[READ_END]);
        close(stderr_pipe[READ_END]);
        close(stdin_pipe[WRITE_END]);
        close(stdout_pipe[WRITE_END]);
        close(stderr_pipe[WRITE_END]);

        return NULL;
    }

    // parent won't read from READ_END of stdin_pipe, parent uses WRITE_END
    // parent won't write to WRITE_END of stdout_pipe, parent uses READ_END
    // parent won't write to WRITE_END of stderr_pipe, parent uses READ_END
    close(stdin_pipe[READ_END]);
    close(stdout_pipe[WRITE_END]);
    close(stderr_pipe[WRITE_END]);

    Proc *proc = (Proc *)AllocatorAlloc(alloc, sizeof(Proc), true);

    if (!proc) {
        close(stdin_pipe[WRITE_END]);
        close(stdout_pipe[READ_END]);
        close(stderr_pipe[READ_END]);
        LOG_ERROR("Failed to allocate process handle");
        return NULL;
    }
    proc->pid       = pid;
    proc->stdin_fd  = stdin_pipe[WRITE_END];
    proc->stdout_fd = stdout_pipe[READ_END];
    proc->stderr_fd = stderr_pipe[READ_END];

    return proc;
#else
    HANDLE              hStdinRead = NULL, hStdinWrite = NULL;
    HANDLE              hStdoutRead = NULL, hStdoutWrite = NULL;
    HANDLE              hStderrRead = NULL, hStderrWrite = NULL;
    SECURITY_ATTRIBUTES sa = {sizeof(sa), NULL, TRUE};

    // Create pipes
    if (!CreatePipe(&hStdinRead, &hStdinWrite, &sa, 0) || !CreatePipe(&hStdoutRead, &hStdoutWrite, &sa, 0) ||
        !CreatePipe(&hStderrRead, &hStderrWrite, &sa, 0)) {
        LOG_SYS_ERROR("CreatePipe failed");

        // Clean up only those handles that were successfully created
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

        return NULL;
    }

    SetHandleInformation(hStderrRead, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(hStdoutRead, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(hStdinWrite, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFO si = {sizeof(si)};
    si.dwFlags     = STARTF_USESTDHANDLES;
    si.hStdInput   = hStdinRead;   // child will read from read end of stdin pipe
    si.hStdOutput  = hStdoutWrite; // child will write to write of stdout pipe
    si.hStdError   = hStderrWrite; // child will write to write end of stderr pipe

    PROCESS_INFORMATION pi = {0};

    // Build command line
    Str cmdline = StrInit(alloc);
    StrPushBackZstr(&cmdline, filepath);
    for (char **arg = argv + 1; *arg; ++arg) {
        StrPushBack(&cmdline, ' ');
        StrPushBackZstr(&cmdline, *arg);
    }

    if (!CreateProcessA(NULL, cmdline.data, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
        LOG_SYS_ERROR("CreateProcessA() failed");

        StrDeinit(&cmdline);
        CloseHandle(hStdinRead);
        CloseHandle(hStdinWrite);
        CloseHandle(hStdoutRead);
        CloseHandle(hStdoutWrite);
        CloseHandle(hStderrRead);
        CloseHandle(hStderrWrite);

        return NULL;
    }
    StrDeinit(&cmdline);

    CloseHandle(hStdinRead);   // parent won't read from child's stdin, will write to it
    CloseHandle(hStdoutWrite); // parent won't write to child's stdout, will read from it
    CloseHandle(hStderrWrite); // parent won't write to child's stderr, will read from it

    Proc *proc = (Proc *)AllocatorAlloc(alloc, sizeof(Proc), true);

    if (!proc) {
        CloseHandle(hStdinWrite);
        CloseHandle(hStdoutRead);
        CloseHandle(hStderrRead);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        LOG_ERROR("Failed to allocate process handle");
        return NULL;
    }
    proc->pi          = pi;
    proc->hStdinWrite = hStdinWrite;
    proc->hStdoutRead = hStdoutRead;
    proc->hStderrRead = hStderrRead;

    return proc;
#endif
}

ProcStatus ProcWait(Proc *proc) {
    if (!proc) {
        LOG_FATAL("Invalid argument");
    }

#if defined(__APPLE__) || defined(__linux__)
    int status;
    if (-1 == waitpid(proc->pid, &status, 0)) {
        LOG_SYS_ERROR("Failed to wait for child process");
        return SYS_PROC_STATUS_ERROR;
    }

    proc->completed = true;

    if (WIFEXITED(status)) {
        proc->exit_code = WEXITSTATUS(status);
        return SYS_PROC_STATUS_COMPLETED;
    } else if (WIFSIGNALED(status)) {
        proc->exit_code = 128 + WTERMSIG(status);
        return SYS_PROC_STATUS_TERMINATED;
    } else {
        proc->exit_code = -1; // Unknown termination
        return SYS_PROC_STATUS_ERROR;
    }

#else
    if (WAIT_FAILED == WaitForSingleObject(proc->pi.hProcess, INFINITE)) {
        LOG_SYS_ERROR("Failed to wait for child process");
        return SYS_PROC_STATUS_ERROR;
    }

    proc->completed = true;

    DWORD code = 0;
    if (!GetExitCodeProcess(proc->pi.hProcess, &code)) {
        proc->exit_code = -1;
        return SYS_PROC_STATUS_ERROR;
    } else {
        proc->exit_code = (i32)code;
        return SYS_PROC_STATUS_COMPLETED;
    }
#endif
}

ProcStatus ProcWaitFor(Proc *proc, u64 timeout_ms) {
    if (!proc) {
        LOG_FATAL("Invalid arguments");
    }

#if defined(_WIN32)
    DWORD wait_time = (timeout_ms == 0) ? INFINITE : (DWORD)timeout_ms;
    DWORD result    = WaitForSingleObject(proc->pi.hProcess, wait_time);

    switch (result) {
        case WAIT_OBJECT_0 : {
            DWORD code_dw = 0;
            if (GetExitCodeProcess(proc->pi.hProcess, &code_dw)) {
                proc->exit_code = (int)code_dw;
                proc->completed = true;
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
        res = waitpid(proc->pid, &status, 0);
    } else {
        // Simulate timeout using polling
        u64       elapsed_ms        = 0;
        const u64 sleep_interval_ms = 10;

        while (elapsed_ms < timeout_ms) {
            res = waitpid(proc->pid, &status, WNOHANG);
            if (res == -1) {
                return SYS_PROC_STATUS_ERROR;
            } else if (res == 0) {
                usleep(sleep_interval_ms * 1000);
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

    if (res == proc->pid) {
        proc->completed = true;
        if (WIFEXITED(status)) {
            proc->exit_code = WEXITSTATUS(status);
            return SYS_PROC_STATUS_COMPLETED;
        } else if (WIFSIGNALED(status)) {
            proc->exit_code = 128 + WTERMSIG(status);
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

    if (proc->completed) {
        return;
    }

#if defined(__APPLE__) || defined(__linux__)
    if (-1 == kill(proc->pid, SIGTERM)) {
        LOG_SYS_ERROR("kill(pid, SIGTERM) failed");
    }

    // Now wait for it to exit and capture the exit code
    int status;
    if (-1 == waitpid(proc->pid, &status, 0)) {
        LOG_SYS_ERROR("waitpid after SIGTERM failed");
        return;
    }

    proc->completed = true;

    if (WIFEXITED(status)) {
        proc->exit_code = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        proc->exit_code = 128 + WTERMSIG(status);
    } else {
        proc->exit_code = -1; // Unknown
    }

#else
    if (!TerminateProcess(proc->pi.hProcess, 1)) {
        LOG_SYS_ERROR("TerminateProcess failed");
        return;
    }

    // Wait for it to actually exit
    if (WAIT_FAILED == WaitForSingleObject(proc->pi.hProcess, INFINITE)) {
        LOG_SYS_ERROR("WaitForSingleObject after TerminateProcess failed");
        return;
    }

    DWORD code = 0;
    if (!GetExitCodeProcess(proc->pi.hProcess, &code)) {
        proc->exit_code = -1;
    } else {
        proc->exit_code = (i32)code;
    }

    proc->completed = true;
#endif
}


void ProcDestroy(Proc *proc, Allocator *alloc) {
    if (!proc) {
        LOG_FATAL("Invalid argument");
    }
    if (!alloc) {
        LOG_FATAL("ProcDestroy requires the allocator that created the handle");
    }
    ProcTerminate(proc);
#if defined(__APPLE__) || defined(__linux__)
    close(proc->stdin_fd);
    close(proc->stdout_fd);
    close(proc->stderr_fd);
#else
    CloseHandle(proc->hStdinWrite);
    CloseHandle(proc->hStdoutRead);
    CloseHandle(proc->hStderrRead);
    CloseHandle(proc->pi.hThread);
    CloseHandle(proc->pi.hProcess);
#endif
    AllocatorFree(alloc, proc, sizeof(Proc));
}

i32 ProcWriteToStdin(Proc *proc, Str *buf) {
    if (!proc || !buf) {
        LOG_FATAL("Invalid arguments");
    }

#if defined(__APPLE__) || defined(__linux__)
    return write(proc->stdin_fd, buf->data, buf->length);
#else
    DWORD written = 0;
    if (!WriteFile(proc->hStdinWrite, buf->data, buf->length, &written, NULL))
        return -1;
    return (int)written;
#endif
}

i32 sys_proc_read_internal(Proc *proc, Str *buf, bool is_stdout) {
    if (!proc || !buf) {
        LOG_FATAL("Invalid argument");
    }

    u64  total_read   = 0;
    char tmpbuf[1024] = {0};

#if defined(__APPLE__) || defined(__linux__)
    i32 rfd = is_stdout ? proc->stdout_fd : proc->stderr_fd;

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
            if (errno == EINTR)
                continue; // retry on signal
            LOG_SYS_ERROR("read failed");
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
        HANDLE rhandle   = is_stdout ? proc->hStdoutRead : proc->hStderrRead;

        if (!PeekNamedPipe(rhandle, NULL, 0, NULL, &available, NULL)) {
            LOG_SYS_ERROR("PeekNamedPipe failed");
            return -1;
        }

        if (available == 0) {
            // EOF or no data
            break;
        }

        DWORD bytes_read = 0;

        if (!ReadFile(rhandle, tmpbuf, 1023, &bytes_read, NULL)) {
            LOG_SYS_ERROR("ReadFile failed");
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

#if defined(__APPLE__) || defined(__linux__)
    return proc->pid;
#else
    return (i32)proc->pi.dwProcessId;
#endif
}

i32 ProcIsRunning(Proc *proc) {
    if (!proc) {
        LOG_FATAL("Invalid argument");
    }

#if defined(__APPLE__) || defined(__linux__)
    int   status;
    pid_t result = waitpid(proc->pid, &status, WNOHANG);
    if (result == 0) {
        return 1;  // Still running
    } else if (result == proc->pid) {
        return 0;  // Exited
    } else {
        return -1; // Error
    }
#else
    DWORD code = 0;
    if (!GetExitCodeProcess(proc->pi.hProcess, &code)) {
        return -1; // API error
    }
    return (code == STILL_ACTIVE) ? 1 : 0;
#endif
}

i32 ProcGetExitCode(Proc *proc) {
    if (!proc) {
        LOG_FATAL("Invalid argument");
    }

    if (!proc->completed) {
        return -1; // Cannot get exit code if not completed
    }

#if defined(_WIN32)
    DWORD code;
    if (GetExitCodeProcess(proc->pi.hProcess, &code)) {
        return (i32)code;
    }
    return -1;
#else
    return proc->exit_code; // Already stored during wait
#endif
}

Str *GetCurrentExecutablePath(Str *exe_path) {
    ValidateStr(exe_path);
    Allocator *alloc = exe_path->allocator;

#ifdef _WIN32
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

// Fallback for macOS and other Unix systems
#    ifdef __APPLE__
    // macOS specific method
    u32 bsize = sizeof(buffer);
    if (_NSGetExecutablePath(buffer, &bsize) == 0) {
        *exe_path = StrInitFromCstr(buffer, ZstrLen(buffer), alloc);
        return exe_path;
    }
#    endif

    return NULL;
#endif
}
