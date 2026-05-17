/// file      : Source/Misra/_StartLinux.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Linux/ELF process entry point. Replaces glibc's `crt1.o` +
/// `__libc_start_main` shim with a tiny `_start` that pulls argc/argv
/// off the kernel-supplied stack, calls `main`, and exits the process
/// via the `exit_group` syscall.
///
/// Compiled only on Linux + x86_64 / aarch64 -- macOS uses libSystem's
/// entry shim (different ABI), Windows uses `mainCRTStartup` (entirely
/// different runtime). On those platforms this file is omitted from
/// the build and the CRT-provided entry runs unchanged.
///
/// Pair this with `-nostartfiles` at link time so the CRT's own
/// entry object (`crt1.o`) isn't pulled in alongside ours. Linker
/// keeps libc itself linked -- we still need it for the few
/// libc-shaped pieces that survive elsewhere (Proc.c fork/execve,
/// Debug.c when alloc_debug is on, etc.).
///
/// What we lose by skipping the CRT entry:
///   - C++ static dtors via `__cxa_finalize` (we have no C++).
///   - `_init`/`_fini` section invocation (we don't use them).
///   - `atexit()` handlers (no library code registers any; user
///     code that wants this should restore the CRT entry).
///   - `__gmon_start__` profiling hook (gprof; not used).
///
/// What we still get:
///   - argc / argv from the kernel-provided stack frame.
///   - A correct exit-status syscall.

#include "_Syscall.h"

#if defined(__linux__) && (defined(__x86_64__) || defined(__aarch64__))

// Naked entry point. The kernel jumps here with a freshly-set-up
// stack: SP points at argc, followed by argv[0..argc-1], a NULL
// terminator, envp[...], NULL, then the auxv pairs and string data.
// We grab argc + argv, hand them to `main`, then issue exit_group with
// main's return value. ABI registers (rdi/rsi on x86_64, x0/x1 on
// aarch64) are loaded directly; no C prologue is generated because of
// the `naked` attribute.
__attribute__((naked, used, noreturn)) void _start(void) {
#    if defined(__x86_64__)
    __asm__(
        // SysV AMD64 ABI: clear rbp so unwinders treat _start as the
        // root frame, load argc into rdi, argv into rsi, force the
        // stack alignment the C ABI wants at a call boundary (the
        // kernel hands us 8-aligned, not 16), then call main. After
        // main returns, exit_group via syscall 231.
        "xor %ebp, %ebp\n"
        "mov (%rsp), %rdi\n"  // argc (1st arg)
        "lea 8(%rsp), %rsi\n" // argv (2nd arg)
        "and $-16, %rsp\n"    // 16-byte stack align for the call
        "call main\n"
        "mov %eax, %edi\n"    // exit status -> 1st syscall arg
        "mov $231, %eax\n"    // SYS_exit_group
        "syscall\n"
        "ud2\n"               // unreachable; trap if syscall returns
    );
#    elif defined(__aarch64__)
    __asm__(
        // AArch64 ABI: zero frame-pointer + link-register so backtraces
        // bottom out cleanly. Load argc into x0, argv into x1, call
        // main, then SYS_exit_group (94) with w0 carrying the exit
        // status.
        "mov x29, #0\n"
        "mov x30, #0\n"
        "ldr x0, [sp]\n"   // argc (1st arg)
        "add x1, sp, #8\n" // argv (2nd arg)
        "bl main\n"
        "mov w8, #94\n"    // SYS_exit_group
        "svc #0\n"
        "brk #0\n"         // unreachable
    );
#    endif
}

#endif // __linux__ && (__x86_64__ || __aarch64__)
