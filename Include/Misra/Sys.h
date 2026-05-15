/// file      : sys.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Portable system functions

#ifndef MISRA_SYS_H
#define MISRA_SYS_H

#include <Misra/Std/Container/Str.h>
#include <errno.h>

#include <Misra/Sys/Mutex.h>

// `ProcId` is part of the foundation because `LOG_FATAL` formats it
// into the log message. `ProcGetCurrentId` (declared below) lives
// in `Sys.c`, also foundation. The full process-spawning API in
// `Sys/Proc.h` is the optional `sys_proc` feature - C11 lets the
// typedef appear in both files.
typedef u64 ProcId;

#if MISRA_HAVE_SYS_DIR
#    include <Misra/Sys/Dir.h>
#endif

#if MISRA_HAVE_SYS_PROC
#    include <Misra/Sys/Proc.h>
#endif

#ifndef SYS_ERROR_STR_MAX_LENGTH
#    define SYS_ERROR_STR_MAX_LENGTH 128
#endif

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
/// Get environment value value in a `Str` object.
/// Object must be destroyed after use.
///
/// name[in]   : Name of environment variable.
/// value[out] : Value of environment variable.
///
/// SUCCESS : `Str` object containing value of environment variable.
/// FAILURE : Returns NULL if variable not found.
///
/// TAGS: System, Environment, Memory
///
Str *GetEnv(const char *name, Str *value);

///
/// Get last error using an error number.
///
/// eno[in]      : Unique error number descriptor.
/// err_str[out] : Error string will be stored in this.
///
/// SUCCESS : Error string describing last error.
/// FAILURE : Returns NULL if `err_str` is NULL.
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
/// Set a custom callback function for Abort.
/// If no callback is set, Abort will call the standard abort() function.
///
/// callback[in] : Function to call when Abort is invoked, or NULL to reset to default.
///
/// SUCCESS : Callback is set.
/// FAILURE : Function cannot fail.
///
/// TAGS: System, Testing, Callback
///
void SetAbortCallback(AbortCallback callback);

///
/// Custom abort function that can be redirected for testing purposes.
/// By default, this calls the standard abort() function.
/// If a callback is set via SetAbortCallback, it calls the callback instead.
///
/// SUCCESS : Function does not return (either aborts or calls callback).
/// FAILURE : Function cannot fail.
///
/// TAGS: System, Testing, Control
///
void Abort(void);

#endif // MISRA_SYS_H
