/// file      : Source/Misra/_WinStubs.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Windows-only UCRT + compiler-runtime stubs for the freestanding
/// build. clang-cl emits implicit references to several symbols that
/// normally live in UCRT / vcruntime. With /NODEFAULTLIB dropping
/// those, we have to provide our own.
///
/// These definitions are kept OUT of libmisra_std.a deliberately: the
/// test binaries and FuzzHarness keep UCRT, and UCRT also defines
/// these symbols (e.g. __security_cookie in msvcrtd.lib's
/// gs_cookie.obj). If both were in the link line we'd get
/// 'duplicate symbol' errors. Meson adds this file as a per-target
/// source only for freestanding Bin/ executables.
///
/// Most of these stubs are behaviourally inert -- the symbols exist
/// only to satisfy the linker (sprintf is never called, stack frames
/// never exceed the chkstk threshold, getenv is unused on Windows).
/// The exception is the stack-protector trio (__security_cookie,
/// __security_init_cookie, __security_check_cookie): those are real
/// implementations -- the cookie is seeded from BCryptGenRandom at
/// process start and the check function aborts on mismatch. See
/// section (1) for the full design.

#include <Misra/Config.h>

#if PLATFORM_WINDOWS

// (1) Stack-protector cookies. clang-cl emits prologue/epilogue refs
//     to these for every function with a stack array. The cookie is
//     XORed with the saved return address on entry, the result is
//     stored above the locals, and on exit __security_check_cookie
//     is called with the stored value -- a mismatch means the return
//     address was overwritten (stack-overflow exploit attempt) and
//     the program must die before the return runs.
//
//     We supply the same trio UCRT does: the cookie itself, the
//     check function, and the init routine that seeds the cookie
//     from BCryptGenRandom at process start. __security_init_cookie
//     is called from _StartWin.c's misra_start before main runs.
//
//     The cookie's top 16 bits are forced to zero per the MSVC ABI:
//     a stack-canary value that fits in the kernel-pointer range
//     would be indistinguishable from a swept-stack pointer write,
//     so the layout reserves the high range to catch that pattern.

// BCrypt forward decls. <bcrypt.h> pulls thousands of Win32 typedefs
// we don't need; declare just BCryptGenRandom by hand.
typedef long           NTSTATUS;
typedef unsigned char *PUCHAR;
typedef unsigned long  ULONG;
typedef void          *BCRYPT_ALG_HANDLE;
#    define DECLSPEC __declspec(dllimport)
#    define WINAPI   __stdcall
DECLSPEC NTSTATUS WINAPI BCryptGenRandom(BCRYPT_ALG_HANDLE hAlgorithm, PUCHAR pbBuffer, ULONG cbBuffer, ULONG dwFlags);
#    define MISRA_BCRYPT_USE_SYSTEM_PREFERRED_RNG 0x00000002UL

// kernel32 entropy-mix sources for the BCrypt-failed fallback. None
// are CSPRNG-quality individually; XORed together they at least vary
// per-run.
DECLSPEC unsigned long long WINAPI GetTickCount64(void);
DECLSPEC unsigned long WINAPI      GetCurrentProcessId(void);
DECLSPEC unsigned long WINAPI      GetCurrentThreadId(void);
DECLSPEC void WINAPI               ExitProcess(unsigned long uExitCode);

// Initial value is a fixed nonzero sentinel with top 16 bits clear
// so even an unitialised __security_cookie (init skipped somehow)
// is still discriminable from common overflow patterns. Real seeding
// happens in __security_init_cookie below.
__attribute__((used)) unsigned long long __security_cookie = 0x00002B992DDFA232ULL;

// no_stack_protector: this function itself runs canary instrumentation
// in normal compilation, and we don't want it consuming the
// pre-seeded value. Excluding it closes the bootstrap loop.
__attribute__((no_stack_protector, used)) void __security_init_cookie(void) {
    unsigned long long cookie = 0;
    NTSTATUS           s      = BCryptGenRandom(
        ((BCRYPT_ALG_HANDLE)0),
        (PUCHAR)&cookie,
        (ULONG)sizeof cookie,
        MISRA_BCRYPT_USE_SYSTEM_PREFERRED_RNG
    );
    if (s < 0) {
        // BCrypt unavailable -- fall back to ASLR + PID + TID + tick.
        // Not CSPRNG-quality, but varies per-run and across machines.
        cookie  = (unsigned long long)&__security_cookie;
        cookie ^= (unsigned long long)GetCurrentProcessId() << 32;
        cookie ^= (unsigned long long)GetCurrentThreadId();
        cookie ^= GetTickCount64();
    }
    cookie &= 0x0000FFFFFFFFFFFFULL;    // top-16 zero per MSVC ABI
    if (cookie == 0) {
        cookie = 0x00002B992DDFA232ULL; // astronomically unlikely; defensive
    }
    __security_cookie = cookie;
}

// On canary mismatch, abort with STATUS_STACK_BUFFER_OVERRUN -- the
// NT status code Windows itself uses for this class of failure, so
// debuggers and crash reporters categorise the abort correctly.
// no_stack_protector + noreturn: don't want a canary check on the
// path that handles a canary failure.
__attribute__((no_stack_protector, used, noreturn)) static void misra_security_failure(void) {
    ExitProcess(0xC0000409UL);
    __builtin_unreachable();
}

__attribute__((no_stack_protector, used)) void __security_check_cookie(unsigned long long cookie) {
    if (cookie != __security_cookie) {
        misra_security_failure();
    }
}

// (2) Stack-probe. clang-cl emits __chkstk in prologues with frames
//     bigger than the probe threshold (default 4 KiB). UCRT/ntdll
//     provide a routine that touches every guard page so the OS
//     demand-faults them in. Our runtime never has frames that large
//     (Allocator handles big buffers), so a no-op is fine. Same
//     approach as the Mac __chkstk_darwin stub.
__attribute__((naked, used)) void __chkstk(void) {
    __asm__("ret");
}

// (3) Floating-point indicator. MSVC C-runtime convention: every
//     module that uses FP references _fltused. UCRT defines as
//     int=0x9875. We do the same.
__attribute__((used)) int _fltused = 0x9875;

// (4) UCRT <stdio.h> inline-function fallout. <stdio.h> has __inline
//     defs of sprintf/snprintf/_vsnprintf_l that bottom out at
//     __stdio_common_vsprintf (dllimport). Any TU that includes
//     <stdio.h> (even transitively, via Misra/Std/Log.h) instantiates
//     the inlines, so the linker sees calls to it even though our
//     code never actually calls sprintf. Provide a stub that returns
//     -1 (sprintf's encoding-error convention).
//
//     dllimport convention: the linker looks for __imp_<symbol>
//     as a function pointer. Provide that.
static long long misra_stdio_common_vsprintf_stub(
    unsigned long long options,
    char              *buf,
    unsigned long long bufsize,
    const char        *format,
    void              *locale,
    void             **arglist
) {
    (void)options;
    (void)buf;
    (void)bufsize;
    (void)format;
    (void)locale;
    (void)arglist;
    return -1;
}
__attribute__((used)) void *__imp___stdio_common_vsprintf = (void *)misra_stdio_common_vsprintf_stub;

// (5) UCRT `getenv`. `Sys.c`'s `EnvGet` calls `getenv` on Windows;
//     non-freestanding builds resolve it from UCRT via dllimport.
//     Freestanding can't pull UCRT, so we provide a stub that returns
//     NULL -- matching the Darwin `EnvGet` behaviour (no env access
//     without libc). Same `__imp_<name>` indirect-call convention as
//     `__stdio_common_vsprintf` above.
static char *misra_getenv_stub(const char *name) {
    (void)name;
    return ((char *)0);
}
__attribute__((used)) void *__imp_getenv = (void *)misra_getenv_stub;

#endif // PLATFORM_WINDOWS
