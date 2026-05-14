/// file      : sys.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Portable system functions

#ifndef MISRA_SYS_H
#define MISRA_SYS_H

#include <Misra/Std/Container/Str.h>
#include <errno.h>

#include <Misra/Sys/Dir.h>
#include <Misra/Sys/Mutex.h>
#include <Misra/Sys/Proc.h>

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
SysProcId SysGetCurrentProcessId(void);

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
Str *SysGetEnv(const char *name, Str *value);

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
Str *SysStrError(i32 eno, Str *err_str);

///
/// Function pointer type for SysAbort callback.
/// This allows custom handling of abort situations (e.g., for testing).
///
typedef void (*SysAbortCallback)(void);

///
/// Set a custom callback function for SysAbort.
/// If no callback is set, SysAbort will call the standard abort() function.
///
/// callback[in] : Function to call when SysAbort is invoked, or NULL to reset to default.
///
/// SUCCESS : Callback is set.
/// FAILURE : Function cannot fail.
///
/// TAGS: System, Testing, Callback
///
void SysSetAbortCallback(SysAbortCallback callback);

///
/// Custom abort function that can be redirected for testing purposes.
/// By default, this calls the standard abort() function.
/// If a callback is set via SysSetAbortCallback, it calls the callback instead.
///
/// SUCCESS : Function does not return (either aborts or calls callback).
/// FAILURE : Function cannot fail.
///
/// TAGS: System, Testing, Control
///
void SysAbort(void);

#endif // MISRA_SYS_H
