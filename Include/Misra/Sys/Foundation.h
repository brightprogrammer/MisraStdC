/// file      : sys/foundation.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Foundation system primitives -- always built (in `Sys.c`), independent of
/// the optional `Sys/*` features. Split out of `Misra/Sys.h` so the umbrella
/// stays a pure header-include manifest. `LOG_FATAL` / `LOG_SYS_*` depend on
/// `ProcId`, `ErrnoOf`, and `StrError` declared here.

#ifndef MISRA_SYS_FOUNDATION_H
#define MISRA_SYS_FOUNDATION_H

#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Sys/Errno.h>
#include <Misra/Types.h>

// `ProcId` is part of the foundation because `LOG_FATAL` formats it
// into the log message. `ProcGetCurrentId` (declared below) lives
// in `Sys.c`, also foundation. The full process-spawning API in
// `Sys/Proc.h` is the optional `sys_proc` feature - C11 lets the
// typedef appear in both files.
typedef u64 ProcId;

#ifndef ERROR_STR_MAX_LENGTH
#    define ERROR_STR_MAX_LENGTH 128
#endif

///
/// Convert the return value of a system call into an errno-style i32.
///
/// On the Linux/Darwin direct-syscall path the kernel returns `-errno`
/// directly in the return register, so `ErrnoOf(ret)` is just `-ret`.
/// On libc-bound fallback paths the failing wrapper returns -1 and
/// stores the code in the thread-local `errno`; `ErrnoOf(ret)` ignores
/// the return value and reads errno.
///
/// Use this everywhere you'd previously have read `errno` after a
/// system call. Lets `LOG_SYS_ERROR(ErrnoOf(ret), "...")` work
/// uniformly across platforms and -- critically -- avoids touching
/// `__errno_location` on the direct-syscall path.
///
/// ret[in] : Return value from a system call. Negative on the
///           direct-syscall path means `-errno`; -1 on the libc
///           fallback means "errno was set".
///
/// SUCCESS : Returns the errno code (>= 0) corresponding to `ret`.
/// FAILURE : Function cannot fail.
///
/// TAGS: System, Errno
///
static inline i32 ErrnoOf(long ret) {
#if FEATURE_DIRECT_SYSCALL
    return (i32)(-ret);
#else
    (void)ret;
    return Errno();
#endif
}

///
/// Platform-independent method to get current process Id. Foundation
/// API: provided by `Sys.c` (always built), unlike the rest of the
/// process-spawning functions in `Sys/Proc.h` which live in the optional
/// `sys_proc` feature.
///
/// SUCCESS : Returns current process ID.
/// FAILURE : Function cannot fail - always returns valid ID.
///
/// TAGS: System, Process
///
ProcId ProcGetCurrentId(void);

///
/// Read an environment variable. Returns the value of environment
/// variable `name`, or NULL if not set. Implementation calls into the
/// platform env-lookup API so consumers don't have to pull in libc
/// headers for the prototype.
///
/// name[in] : NUL-terminated environment variable name.
///
/// SUCCESS : Returns a pointer to the value string (process-owned).
/// FAILURE : Returns NULL when `name` is NULL, when the variable is
///           not set, or when the platform has no envp source linked
///           (freestanding Windows, Darwin with no caller-captured
///           envp).
///
/// TAGS: Environment
///
Zstr EnvGet(Zstr name);

///
/// Get last error using an error number.
///
/// eno[in]      : Unique error number descriptor.
/// err_str[out] : Error string will be stored in this.
///
/// SUCCESS : Returns `err_str` with the error description appended.
/// FAILURE : Aborts via `ValidateStr` if `err_str` is NULL or
///           uninitialised (`*err_str` is left in its pre-call state).
///
/// TAGS: System, Error, String
///
Str *StrError(i32 eno, Str *err_str);

///
/// Function pointer type for Abort callback.
/// This allows custom handling of abort situations (e.g., for testing).
///
typedef void (*AbortCallback)(void);

///
/// Register a custom callback function invoked when `Abort` runs.
/// If no callback is registered, `Abort` traps directly.
///
/// callback[in] : Function to call when Abort is invoked, or NULL to reset to default.
///
/// SUCCESS : Callback is registered.
/// FAILURE : Function cannot fail.
///
/// TAGS: System, Testing, Callback
///
void OnAbort(AbortCallback callback);

///
/// Custom abort function that can be redirected for testing purposes.
/// Traps directly via the architecture's native trap instruction. If
/// a callback is registered via `OnAbort`, it runs first; control then
/// falls through to the trap (a callback that wants to short-circuit
/// `Abort` must `longjmp` or `exit` itself).
///
/// SUCCESS : Function does not return. Any registered callback runs
///           first, then the hardware trap fires.
/// FAILURE : Function cannot fail.
///
/// TAGS: System, Testing, Control
///
void Abort(void);

#endif // MISRA_SYS_FOUNDATION_H
