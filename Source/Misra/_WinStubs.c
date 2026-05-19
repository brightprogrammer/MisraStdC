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
/// None of these stubs matter behaviourally for the Bin/ tools: they
/// never actually call sprintf, allocate giant stack frames, or check
/// stack canaries -- the symbols exist purely to satisfy the linker.

#include <Misra/Config.h>

#if PLATFORM_WINDOWS

// (1) Stack-protector cookies. clang-cl emits prologue/epilogue refs
//     to these for every function with a stack array, even with
//     /GS-. UCRT provides them. We provide minimal stubs: a fixed
//     cookie value (not security-meaningful since we're a libc-diet
//     project, not a hardened service), and an empty check function.
__attribute__((used)) unsigned long long __security_cookie = 0xBEEFC0DE12345678ULL;
__attribute__((used)) void               __security_check_cookie(unsigned long long cookie) {
    (void)cookie;
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

#endif // PLATFORM_WINDOWS
