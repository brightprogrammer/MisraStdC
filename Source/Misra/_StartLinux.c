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

extern int main(int argc, char **argv);

// envp captured at process start. Lives here so `Sys.c`'s `EnvGet`
// can walk it without ever calling `getenv` / touching libc.
//
// The kernel-supplied stack at `_start` is:
//   sp:                    argc
//   sp + 8..8+8*argc:      argv[0..argc-1]
//   sp + 8+8*argc:         NULL  (argv terminator)
//   sp + 8+8*argc+8 ...:   envp[0..N-1]
//   ...                    NULL  (envp terminator)
//   ...                    auxv pairs
//
// `envp = argv + argc + 1`. The strings themselves live in a region
// the kernel maps above the initial stack; pointers remain valid for
// the lifetime of the process.
char **misra_envp = 0;

__attribute__((used, noreturn)) static void misra_start_c(long *kernel_sp) {
    int    argc = (int)kernel_sp[0];
    char **argv = (char **)(kernel_sp + 1);
    misra_envp  = argv + argc + 1;
    int rc      = main(argc, argv);
    (void)misra_sys1(MISRA_SYS_exit_group, rc);
    __builtin_unreachable();
}

// Naked entry point. The kernel jumps here with a freshly-set-up
// stack as described above. We hand the raw SP to `misra_start_c`,
// which decodes argc/argv/envp and dispatches to `main`.
__attribute__((naked, used, noreturn)) void _start(void) {
#    if defined(__x86_64__)
    __asm__(
        "xor %ebp, %ebp\n"
        "mov %rsp, %rdi\n" // kernel_sp (1st arg)
        "and $-16, %rsp\n" // 16-byte stack align for the C call
        "call misra_start_c\n"
        "ud2\n"            // unreachable
    );
#    elif defined(__aarch64__)
    __asm__(
        "mov x29, #0\n"
        "mov x30, #0\n"
        "mov x0, sp\n" // kernel_sp (1st arg)
        "bl misra_start_c\n"
        "brk #0\n"     // unreachable
    );
#    endif
}

#endif // __linux__ && (__x86_64__ || __aarch64__)
