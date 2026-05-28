/// file      : sys.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Portable system functions

#ifndef MISRA_SYS_H
#define MISRA_SYS_H

#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Sys/Errno.h>
#include <Misra/Sys/Mutex.h>

// `ProcId` is part of the foundation because `LOG_FATAL` formats it
// into the log message. `ProcGetCurrentId` (declared below) lives
// in `Sys.c`, also foundation. The full process-spawning API in
// `Sys/Proc.h` is the optional `sys_proc` feature - C11 lets the
// typedef appear in both files.
typedef u64 ProcId;

#if FEATURE_SYS_DIR
#    include <Misra/Sys/Dir.h>
#endif

#if FEATURE_SYS_PROC
#    include <Misra/Sys/Proc.h>
#endif

#if FEATURE_SYS_SOCKET
#    include <Misra/Sys/Socket.h>
#endif

#if FEATURE_SYS_PROCMAPS
#    include <Misra/Sys/ProcMaps.h>
#endif

#if FEATURE_SYS_DNS
#    include <Misra/Sys/Dns.h>
#endif

// Sys/SymbolResolver.h, Sys/Backtrace.h, Sys/PdbCache.h, and
// Sys/MachoCache.h are NOT pulled through the umbrella because they
// transit Parsers/Elf.h / Parsers/Pdb.h / Parsers/Macho.h, whose
// public-API names can collide with downstream TUs that carry their
// own ELF / PDB / Mach-O constants. Include those headers directly
// when you want the resolver, the backtrace formatter, or the
// PE+PDB / Mach-O symbol caches.

#ifndef SYS_ERROR_STR_MAX_LENGTH
#    define SYS_ERROR_STR_MAX_LENGTH 128
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
/// SUCCESS : Returns the errno code (>= 0) corresponding to `ret`.
/// FAILURE : Function cannot fail.
///
/// USAGE:
///   long pid = misra_sys0(MISRA_SYS_fork);
///   if (pid < 0) {
///       LOG_SYS_ERROR(ErrnoOf(pid), "fork failed");
///   }
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
/// By default, this traps directly via the architecture's native trap
/// instruction. If a callback is registered via `OnAbort`, it is
/// invoked instead.
///
/// SUCCESS : Function does not return (either aborts or calls callback).
/// FAILURE : Function cannot fail.
///
/// TAGS: System, Testing, Control
///
void Abort(void);

#endif // MISRA_SYS_H
