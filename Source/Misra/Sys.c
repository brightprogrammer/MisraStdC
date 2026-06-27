/// file      : sys.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Cross-platform system primitives that don't fit any narrower
/// namespace: the errno -> Str describer (`StrError`), the abort
/// callback hook (`OnAbort` / `Abort`), `ProcGetCurrentId`, and
/// `EnvGet` (Linux-via-captured-`envp`, Windows-via-UCRT, Darwin-NULL).
/// Everything here forwards either to a syscall or to a Win32 entry
/// point; no libc surface leaks back through the API.

#include <Misra/Config.h>

#if PLATFORM_WINDOWS
#    include <windows.h>
#endif

#include <Misra/Std/Allocator.h>
#include <Misra/Std.h>
#include <Misra/Std/Log.h>
#include <Misra/Sys.h>
#include "_Syscall.h"

// In-tree errno -> short description table. Covers the POSIX errnos
// we actually surface in Sys / Std / Parsers; anything else falls
// through to a generic "Unknown error" with the numeric value.
//
// Values are POSIX-standardized for the common cases (EPERM=1,
// ENOENT=2, ...) and identical across Linux / macOS / *BSD. Windows
// (UCRT) maps most of them to the same values; the ones it diverges
// on don't matter for our call sites today.
static Zstr errno_description(i32 eno) {
    switch (eno) {
#ifdef EPERM
        case EPERM :
            return "Operation not permitted";
#endif
#ifdef ENOENT
        case ENOENT :
            return "No such file or directory";
#endif
#ifdef ESRCH
        case ESRCH :
            return "No such process";
#endif
#ifdef EINTR
        case EINTR :
            return "Interrupted system call";
#endif
#ifdef EIO
        case EIO :
            return "I/O error";
#endif
#ifdef ENXIO
        case ENXIO :
            return "No such device or address";
#endif
#ifdef E2BIG
        case E2BIG :
            return "Argument list too long";
#endif
#ifdef ENOEXEC
        case ENOEXEC :
            return "Exec format error";
#endif
#ifdef EBADF
        case EBADF :
            return "Bad file descriptor";
#endif
#ifdef ECHILD
        case ECHILD :
            return "No child processes";
#endif
#ifdef EAGAIN
        case EAGAIN :
            return "Resource temporarily unavailable";
#endif
#ifdef ENOMEM
        case ENOMEM :
            return "Cannot allocate memory";
#endif
#ifdef EACCES
        case EACCES :
            return "Permission denied";
#endif
#ifdef EFAULT
        case EFAULT :
            return "Bad address";
#endif
#ifdef EBUSY
        case EBUSY :
            return "Device or resource busy";
#endif
#ifdef EEXIST
        case EEXIST :
            return "File exists";
#endif
#ifdef EXDEV
        case EXDEV :
            return "Invalid cross-device link";
#endif
#ifdef ENODEV
        case ENODEV :
            return "No such device";
#endif
#ifdef ENOTDIR
        case ENOTDIR :
            return "Not a directory";
#endif
#ifdef EISDIR
        case EISDIR :
            return "Is a directory";
#endif
#ifdef EINVAL
        case EINVAL :
            return "Invalid argument";
#endif
#ifdef ENFILE
        case ENFILE :
            return "Too many open files in system";
#endif
#ifdef EMFILE
        case EMFILE :
            return "Too many open files";
#endif
#ifdef ENOTTY
        case ENOTTY :
            return "Inappropriate ioctl for device";
#endif
#ifdef EFBIG
        case EFBIG :
            return "File too large";
#endif
#ifdef ENOSPC
        case ENOSPC :
            return "No space left on device";
#endif
#ifdef ESPIPE
        case ESPIPE :
            return "Illegal seek";
#endif
#ifdef EROFS
        case EROFS :
            return "Read-only file system";
#endif
#ifdef EMLINK
        case EMLINK :
            return "Too many links";
#endif
#ifdef EPIPE
        case EPIPE :
            return "Broken pipe";
#endif
#ifdef EDOM
        case EDOM :
            return "Numerical argument out of domain";
#endif
#ifdef ERANGE
        case ERANGE :
            return "Numerical result out of range";
#endif
#ifdef ENAMETOOLONG
        case ENAMETOOLONG :
            return "File name too long";
#endif
#ifdef ENOTEMPTY
        case ENOTEMPTY :
            return "Directory not empty";
#endif
#ifdef ELOOP
        case ELOOP :
            return "Too many levels of symbolic links";
#endif
#ifdef EOVERFLOW
        case EOVERFLOW :
            return "Value too large for defined data type";
#endif
#ifdef ECONNREFUSED
        case ECONNREFUSED :
            return "Connection refused";
#endif
#ifdef ECONNRESET
        case ECONNRESET :
            return "Connection reset by peer";
#endif
#ifdef ECONNABORTED
        case ECONNABORTED :
            return "Software caused connection abort";
#endif
#ifdef EISCONN
        case EISCONN :
            return "Transport endpoint is already connected";
#endif
#ifdef ENOTCONN
        case ENOTCONN :
            return "Transport endpoint is not connected";
#endif
#ifdef ENETUNREACH
        case ENETUNREACH :
            return "Network is unreachable";
#endif
#ifdef EHOSTUNREACH
        case EHOSTUNREACH :
            return "No route to host";
#endif
#ifdef ETIMEDOUT
        case ETIMEDOUT :
            return "Connection timed out";
#endif
#ifdef EADDRINUSE
        case EADDRINUSE :
            return "Address already in use";
#endif
#ifdef EADDRNOTAVAIL
        case EADDRNOTAVAIL :
            return "Cannot assign requested address";
#endif
#ifdef EAFNOSUPPORT
        case EAFNOSUPPORT :
            return "Address family not supported";
#endif
#ifdef EPROTONOSUPPORT
        case EPROTONOSUPPORT :
            return "Protocol not supported";
#endif
        default :
            return "Unknown error";
    }
}

// Renders "<description> (errno <n>)" straight into `err_str`, reusing
// the buffer the caller already provided. `LOG_SYS_*` hands us a
// 256-byte `StrInitStack` scratch (allocator NULL by design); the errno
// text always fits, so no growth -- and therefore no allocator -- is
// needed. Writing into a fresh `StrInit(StrAllocator(err_str))` instead
// would inherit that NULL allocator into a zero-capacity Str and abort
// on the first byte via `reserve_vec`, regardless of platform; it only
// stayed hidden because the errno paths that reach here are rare.
Str *StrError(i32 eno, Str *err_str) {
    ValidateStr(err_str);
    StrClear(err_str);
    StrAppendFmt(err_str, "{} (errno {})", errno_description(eno), eno);
    return err_str;
}

// Optional pre-`Abort` hook. NULL = no hook; `Abort` falls straight
// through to the hardware trap. Set by `OnAbort` -- the test harness
// uses it to longjmp out of intentional aborts in deadend fixtures.
static AbortCallback g_abort_callback = NULL;

void OnAbort(AbortCallback callback) {
    g_abort_callback = callback;
}

void Abort(void) {
    if (g_abort_callback) {
        g_abort_callback();
        // Fall through if the callback returned -- a callback that
        // wants to short-circuit Abort should longjmp / exit itself.
    }
    // Emit the architecture's native trap instruction directly. No libc
    // (`abort`, `raise`), no compiler intrinsic that isn't universal
    // (`__builtin_trap` is GCC/Clang only). The hardware fault is
    // handled by the OS the same way it would handle abort()'s SIGABRT:
    //   - Linux / macOS: kernel delivers SIGILL/SIGTRAP, default
    //     handler terminates the process and writes a core file.
    //   - Windows: structured-exception EXCEPTION_ILLEGAL_INSTRUCTION /
    //     EXCEPTION_BREAKPOINT, default top-level handler terminates.
#if defined(_MSC_VER)
    // MSVC and clang-cl. __debugbreak is recognized as a compiler
    // intrinsic without needing <intrin.h>; emits `int 3` on x86 and
    // `brk` on ARM/ARM64. EXCEPTION_BREAKPOINT terminates the process
    // when no debugger is attached.
    __debugbreak();
#elif ARCHITECTURE_X86_64 || ARCHITECTURE_X86_32
    __asm__ volatile("ud2");
#elif ARCHITECTURE_AARCH64
    __asm__ volatile("brk #0");
#elif ARCHITECTURE_ARM32
    __asm__ volatile("udf #0");
#else
    // Last-resort for unknown arches: NULL deref. Generates SIGSEGV
    // on POSIX, EXCEPTION_ACCESS_VIOLATION on Windows.
    *(volatile int *)0 = 0;
#endif
}

ProcId ProcGetCurrentId(void) {
#if PLATFORM_WINDOWS
    // kernel32.dll, not libc.
    return (ProcId)GetCurrentProcessId();
#elif FEATURE_DIRECT_SYSCALL
    // Linux + Darwin (XNU): direct syscall. Kernel guarantees getpid never fails.
    return (ProcId)direct_sys0(MISRA_SYS_getpid);
#else
#    error "ProcGetCurrentId: unsupported platform/architecture (no direct-syscall path)"
#endif
}

// `EnvGet`: read an environment variable. The strategy depends on what
// startup machinery the platform gives us:
//
//   - Linux x86_64/aarch64: `_StartLinux.c` captures `envp` from the
//     kernel-supplied stack into `envp_global` (defined here) before
//     calling `main`. We walk it directly -- no libc touch.
//   - Windows (MSVC / clang-cl): non-freestanding builds link UCRT
//     and pull `getenv` via a dllimport-matched prototype. The
//     freestanding Bin/ tools (no UCRT) get a stub from
//     `_WinStubs.c` that returns NULL -- so `EnvGet` on freestanding
//     Windows behaves like Darwin.
//   - Darwin: do NOT reference `getenv`. The libc-diet gate on Mac
//     forbids any unexpected libSystem ref in Bin/ tools, and
//     `_getenv` is not in the allowed set. Callers that need env
//     vars on Darwin must capture them at startup themselves; for
//     now `EnvGet` returns NULL on Darwin.

#if PLATFORM_LINUX && (ARCHITECTURE_X86_64 || ARCHITECTURE_AARCH64)
// Owned here so the symbol is always defined in libmisra_std.a. Set
// by `_StartLinux.c`'s `linux_start_c` trampoline before `main`.
// Targets that don't link our `_start` -- e.g. `Tests/Dwarf.Stripped`,
// which provides its own entry -- leave it NULL, and `EnvGet` returns
// NULL safely.
char **envp_global = NULL;
#elif PLATFORM_WINDOWS
__declspec(dllimport) extern char *__cdecl getenv(Zstr name);
#endif

Zstr EnvGet(Zstr name) {
    if (!name) {
        return NULL;
    }
#if PLATFORM_LINUX && (ARCHITECTURE_X86_64 || ARCHITECTURE_AARCH64)
    if (!envp_global) {
        return NULL;
    }
    // Match `name` against the prefix of each entry up to '='.
    for (char **e = envp_global; *e; ++e) {
        Zstr entry = *e;
        Zstr n     = name;
        while (*n && *entry == *n) {
            ++entry;
            ++n;
        }
        if (*n == 0 && *entry == '=') {
            return entry + 1;
        }
    }
    return NULL;
#elif PLATFORM_WINDOWS
    return (Zstr)getenv(name);
#elif PLATFORM_DARWIN
    // No libSystem `_getenv` reference -- callers must capture envp
    // themselves.
    (void)name;
    return NULL;
#else
#    error "EnvGet: unsupported platform (Linux x86_64/aarch64, Darwin, Windows only)"
#endif
}
