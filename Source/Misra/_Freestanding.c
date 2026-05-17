/// file      : Source/Misra/_Freestanding.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Compiler- and harness-emitted symbols a libc-free build still needs.
/// Two symbol families live here, each with its own platform gate:
///
///   mem* family  (memcpy / memset / memmove / memcmp / bzero)
///       Linux + Darwin, x86_64 + aarch64.
///       The compiler emits implicit calls to these for struct copies,
///       large array initialisations, comparisons, and overlapping
///       moves -- you can't avoid them in C without -fno-builtin-* on
///       every TU. We forward to Misra's already-in-tree MemCopy /
///       MemSet / MemCompare / MemMove (byte loops in Std/Memory.c).
///
///       On Linux these resolve the `-nostdlib` freestanding link.
///       On Darwin the linker prefers our static-lib definition over
///       libSystem's dylib export (two-level namespace, static archive
///       processed before the implicit libSystem dylib), so even
///       though libSystem is still present in the Mach-O LCs, calls
///       to memcpy/memset/etc. bind to our copies and never reach it.
///       `bzero` is included because clang emits direct calls to it
///       on Darwin for zero-init patterns.
///
///   setjmp / longjmp
///       Linux only -- used by Tests/Util/TestRunner.c to catch
///       LOG_FATAL aborts in deadend tests. Hand-written register
///       save/restore per ABI; no libc, no compiler intrinsic. The
///       jmp_buf layout is project-internal (not glibc-compatible).
///       Mac test runner still uses libSystem's _setjmp -- separate
///       follow-up if we want it gone.

#include <Misra/Std/Memory.h>
#include <Misra/Types.h>

#if (defined(__linux__) || defined(__APPLE__)) && \
    (defined(__x86_64__) || defined(__aarch64__))

// Darwin's <string.h> macro-expands memcpy/memmove/memset to
// __builtin___memcpy_chk(...) under FORTIFY_SOURCE -- which is on by
// default on Mac regardless of -U_FORTIFY_SOURCE, because the macros
// gate on _USE_FORTIFY_LEVEL which is set elsewhere in <sys/cdefs.h>.
// <strings.h> does the same to bzero. Misra/Types.h pulls <string.h>
// in transitively, so by the time the preprocessor reaches these
// definitions the names we want to define have been textually replaced
// by builtin invocations.
//
// Undef them right before the defs. The macros only mattered for
// callers (where they added size-check wrappers); our linkable symbols
// (_memcpy/_memmove/_memset/_bzero) are resolved by symbol name at link
// time, not by macro state at the call site. Compiler-emitted intrinsic
// calls bind to our symbols regardless of which header callers saw.
#undef memcpy
#undef memmove
#undef memset
#undef memcmp
#undef bzero

// ---------------------------------------------------------------------------
// mem* family -- thin forwarders to the in-tree byte-loop implementations
// in Std/Memory.c. Plain extern (not static) so the linker can resolve
// compiler-emitted intrinsic calls against them. Marked `used` so LTO /
// unused-function passes don't strip them when no user code references
// them directly.
// ---------------------------------------------------------------------------

__attribute__((used)) void *memcpy(void *dst, const void *src, unsigned long n) {
    MemCopy(dst, src, (size)n);
    return dst;
}

__attribute__((used)) void *memmove(void *dst, const void *src, unsigned long n) {
    MemMove(dst, src, (size)n);
    return dst;
}

__attribute__((used)) void *memset(void *dst, int c, unsigned long n) {
    MemSet(dst, c, (size)n);
    return dst;
}

__attribute__((used)) int memcmp(const void *a, const void *b, unsigned long n) {
    return (int)MemCompare(a, b, (size)n);
}

// Darwin clang emits direct `bzero` calls for some zero-init patterns
// (esp. small struct zeroing), unlike Linux clang/gcc which always go
// through memset. Provide it on both for safety -- it's a one-liner.
__attribute__((used)) void bzero(void *dst, unsigned long n) {
    MemSet(dst, 0, (size)n);
}

// ---------------------------------------------------------------------------
// setjmp / longjmp -- callee-saved register snapshot + restore. The
// jmp_buf layout is project-internal (Tests/Util/TestRunner.c). Both
// `_setjmp` and `setjmp` resolve to the same code because glibc's
// `setjmp` adds signal-mask save/restore on top of `_setjmp`, but the
// project test harness only needs the bare control-flow form.
//
// Linux-only: on Darwin the test runner currently still links
// libSystem's _setjmp/_longjmp. Symbol mangling differs (Mach-O wants
// `_setjmp` as the asm symbol -- which is what naked C declarations
// produce -- but Darwin's libSystem also exports a 16-byte-aligned
// jmp_buf that wouldn't match our 56-byte layout) so a separate Mac
// trampoline + alignment shim is needed if we ever pursue it.
// ---------------------------------------------------------------------------

#endif // mem* gate

#if defined(__linux__) && (defined(__x86_64__) || defined(__aarch64__))

#    if defined(__x86_64__)

// Layout (each slot 8 bytes):
//   0: rbx   1: rbp   2: r12   3: r13   4: r14   5: r15   6: rsp   7: rip
__attribute__((naked, used)) int setjmp(void *env) {
    __asm__(
        "mov %rbx,  0(%rdi)\n" // save callee-saved regs
        "mov %rbp,  8(%rdi)\n"
        "mov %r12, 16(%rdi)\n"
        "mov %r13, 24(%rdi)\n"
        "mov %r14, 32(%rdi)\n"
        "mov %r15, 40(%rdi)\n"
        "lea 8(%rsp), %rax\n" // saved SP = caller's SP (skip our return addr)
        "mov %rax, 48(%rdi)\n"
        "mov (%rsp), %rax\n"  // saved return address
        "mov %rax, 56(%rdi)\n"
        "xor %eax, %eax\n"    // return 0 on the setjmp path
        "ret\n"
    );
}
__attribute__((naked, used)) int _setjmp(void *env) {
    __asm__(
        "mov %rbx,  0(%rdi)\n"
        "mov %rbp,  8(%rdi)\n"
        "mov %r12, 16(%rdi)\n"
        "mov %r13, 24(%rdi)\n"
        "mov %r14, 32(%rdi)\n"
        "mov %r15, 40(%rdi)\n"
        "lea 8(%rsp), %rax\n"
        "mov %rax, 48(%rdi)\n"
        "mov (%rsp), %rax\n"
        "mov %rax, 56(%rdi)\n"
        "xor %eax, %eax\n"
        "ret\n"
    );
}

__attribute__((naked, used, noreturn)) void longjmp(void *env, int val) {
    __asm__(
        "mov  0(%rdi), %rbx\n" // restore callee-saved
        "mov  8(%rdi), %rbp\n"
        "mov 16(%rdi), %r12\n"
        "mov 24(%rdi), %r13\n"
        "mov 32(%rdi), %r14\n"
        "mov 40(%rdi), %r15\n"
        "mov 48(%rdi), %rsp\n"
        "mov %esi, %eax\n" // return val, normalised so 0 -> 1
        "test %eax, %eax\n"
        "jnz 1f\n"
        "inc %eax\n"
        "1: jmp *56(%rdi)\n"
    );
}

#    else  // __aarch64__

// Layout (each slot 8 bytes):
//   0..9: x19..x28   10: x29 (fp)   11: x30 (lr)   12: sp
__attribute__((naked, used)) int setjmp(void *env) {
    __asm__(
        "stp x19, x20, [x0,  #0]\n"
        "stp x21, x22, [x0, #16]\n"
        "stp x23, x24, [x0, #32]\n"
        "stp x25, x26, [x0, #48]\n"
        "stp x27, x28, [x0, #64]\n"
        "stp x29, x30, [x0, #80]\n"
        "mov x1, sp\n"
        "str x1,       [x0, #96]\n"
        "mov w0, #0\n" // return 0 on the setjmp path
        "ret\n"
    );
}
__attribute__((naked, used)) int _setjmp(void *env) {
    __asm__(
        "stp x19, x20, [x0,  #0]\n"
        "stp x21, x22, [x0, #16]\n"
        "stp x23, x24, [x0, #32]\n"
        "stp x25, x26, [x0, #48]\n"
        "stp x27, x28, [x0, #64]\n"
        "stp x29, x30, [x0, #80]\n"
        "mov x1, sp\n"
        "str x1,       [x0, #96]\n"
        "mov w0, #0\n"
        "ret\n"
    );
}

__attribute__((naked, used, noreturn)) void longjmp(void *env, int val) {
    __asm__(
        "ldp x19, x20, [x0,  #0]\n"
        "ldp x21, x22, [x0, #16]\n"
        "ldp x23, x24, [x0, #32]\n"
        "ldp x25, x26, [x0, #48]\n"
        "ldp x27, x28, [x0, #64]\n"
        "ldp x29, x30, [x0, #80]\n"
        "ldr x2,       [x0, #96]\n"
        "mov sp, x2\n"
        "mov w0, w1\n" // return val, normalised so 0 -> 1
        "cbnz w0, 1f\n"
        "mov w0, #1\n"
        "1: ret\n"
    );
}

#    endif // arch

#endif     // __linux__ && (__x86_64__ || __aarch64__)
