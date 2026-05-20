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

#if PLATFORM_LINUX && (defined(__x86_64__) || defined(__aarch64__))

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
// Defined in `Sys.c` so the symbol is always present in libmisra_std.a.
// We just write to it here before calling `main`.
extern char **misra_envp;

// __stack_chk_guard lives in _Freestanding.c (weak). Seeded from
// AT_RANDOM by init_stack_canary below before any canary-using
// function runs. With this in place, the per-process canary is
// kernel-CSPRNG-quality random and a return-address-clobbering
// buffer overflow is detected on function exit (via
// __stack_chk_fail in _Freestanding.c -> LogWrite + Abort).
extern unsigned long __stack_chk_guard;

// Linux ELF aux-vector tag for "16 bytes of kernel-CSPRNG entropy
// at this address". See <elf.h> AT_RANDOM. Hardcoded here so the
// freestanding build doesn't include a libc header for one value.
#    define MISRA_AT_RANDOM 25

// `no_stack_protector` is critical: every other function with
// stack instrumentation reads __stack_chk_guard in its prologue,
// and we haven't seeded it yet. If init_stack_canary itself had
// canary instrumentation, the read on entry would consume the
// guard's pre-seed value (0). Excluding this one function from
// instrumentation closes the bootstrap loop.
//
// The byte loop intentionally avoids memcpy: in some libc-diet
// configs memcpy is itself instrumented, and the canary check on
// memcpy's exit would race against our seeding.
__attribute__((no_stack_protector, used)) static void init_stack_canary(char **envp) {
    while (*envp) {
        envp += 1;
    }
    unsigned long *auxv = (unsigned long *)(envp + 1);
    for (; *auxv != 0; auxv += 2) {
        if (auxv[0] == MISRA_AT_RANDOM) {
            const unsigned char *src = (const unsigned char *)auxv[1];
            unsigned char       *dst = (unsigned char *)&__stack_chk_guard;
            for (unsigned long i = 0; i < sizeof __stack_chk_guard; i += 1) {
                dst[i] = src[i];
            }
            return;
        }
    }
    // AT_RANDOM absent (very old kernel; should not happen on any
    // supported Misra target). Fall back to an ASLR-derived value
    // so the canary is at least unpredictable across runs even
    // though it's not CSPRNG-strong.
    __stack_chk_guard = (unsigned long)&__stack_chk_guard ^ 0xdeadbeefcafef00dUL;
}

__attribute__((used, noreturn)) static void misra_start_c(long *kernel_sp) {
    int    argc = (int)kernel_sp[0];
    char **argv = (char **)(kernel_sp + 1);
    misra_envp  = argv + argc + 1;
    init_stack_canary(misra_envp);
    int rc = main(argc, argv);
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

#endif // PLATFORM_LINUX && (__x86_64__ || __aarch64__)
