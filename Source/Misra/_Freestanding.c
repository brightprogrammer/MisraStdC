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

#include <Misra/Std/Log.h>
#include <Misra/Std/Memory.h>
#include <Misra/Types.h>

#if (PLATFORM_LINUX || PLATFORM_DARWIN || PLATFORM_WINDOWS) && (ARCHITECTURE_X86_64 || ARCHITECTURE_AARCH64)

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
#    undef memcpy
#    undef memmove
#    undef memset
#    undef memcmp
#    undef bzero

// ---------------------------------------------------------------------------
// mem* family -- thin forwarders to the in-tree byte-loop implementations
// in Std/Memory.c. Plain extern (not static) so the linker can resolve
// compiler-emitted intrinsic calls against them. Marked `used` so LTO /
// unused-function passes don't strip them when no user code references
// them directly.
// ---------------------------------------------------------------------------

// Use __SIZE_TYPE__ (clang/gcc builtin -- expands to whatever the
// platform's size_t is) rather than 'unsigned long'. On Linux/Mac
// x86_64 these are the same (LP64). On Windows x86_64 size_t is
// 'unsigned long long' (LLP64) and 'unsigned long' is 32-bit, so a
// declaration mismatch fights with vcruntime_string.h's declaration
// and the linker / compiler refuses with 'conflicting types'.
typedef __SIZE_TYPE__ misra_freestanding_size_t;

__attribute__((used)) void *memcpy(void *dst, const void *src, misra_freestanding_size_t n) {
    MemCopy(dst, src, (size)n);
    return dst;
}

__attribute__((used)) void *memmove(void *dst, const void *src, misra_freestanding_size_t n) {
    MemMove(dst, src, (size)n);
    return dst;
}

__attribute__((used)) void *memset(void *dst, int c, misra_freestanding_size_t n) {
    MemSet(dst, c, (size)n);
    return dst;
}

__attribute__((used)) int memcmp(const void *a, const void *b, misra_freestanding_size_t n) {
    return (int)MemCompare(a, b, (size)n);
}

// Darwin clang emits direct `bzero` calls for some zero-init patterns
// (esp. small struct zeroing), unlike Linux clang/gcc which always go
// through memset. Provide it on both for safety -- it's a one-liner.
// (Skip on Windows: clang-cl doesn't emit bzero, and Windows headers
// don't declare it.)
#    if !PLATFORM_WINDOWS
__attribute__((used)) void bzero(void *dst, misra_freestanding_size_t n) {
    MemSet(dst, 0, (size)n);
}
#    endif

// ---------------------------------------------------------------------------
// Stack-protector helpers -- __stack_chk_guard / __stack_chk_fail.
//
// gcc/clang emit a canary load + store in every function with a
// stack array, and a canary check + branch on exit; on mismatch the
// generated code tail-calls __stack_chk_fail (noreturn). The
// per-process canary value lives in __stack_chk_guard, normally
// supplied by libc and seeded from kernel entropy at process start.
//
// In a libc-diet build there is no libc to supply either symbol.
// Rather than disabling the instrumentation, we provide both
// in-tree: the guard is seeded from AT_RANDOM in _StartLinux.c
// before main runs, and the fail path aborts via LOG_FATAL.
//
// Both symbols are weak. Platform resolution rules differ:
//
//   Linux ELF: strong-beats-weak. Consumers linking libc get libc's
//   strong __stack_chk_guard / __stack_chk_fail (libc seeds its own
//   slot from AT_RANDOM at process start); our weak versions are
//   dead. Libc-diet links (-nostdlib) get only our versions, and
//   _StartLinux.c seeds OUR __stack_chk_guard from AT_RANDOM before
//   main runs.
//
//   Darwin Mach-O: opposite mechanic. Static archives are searched
//   before dylibs, and two-level namespacing pins libSystem's
//   internal __stack_chk_guard references to libSystem's own slot
//   at libSystem's build time. So OUR weak symbol wins for the
//   binary's instrumentation -- two slots co-exist at runtime, ours
//   and libSystem's. libSystem's startup-init constructor (run by
//   dyld before main) seeds __stack_chk_guard by symbol name through
//   dyld's flat-namespace lookup, which finds the binary's exported
//   __stack_chk_guard -- which is ours. The kernel-entropy write
//   lands in our storage. Verified empirically: every process run
//   reads a fresh random value here, and a buffer overflow triggers
//   our __stack_chk_fail correctly.
//
// Windows clang-cl uses a different mechanism (__security_cookie /
// __security_check_cookie) handled in _WinStubs.c, with our own
// __security_init_cookie seeding from BCryptGenRandom -- there's no
// dyld-cooperation on the libc-diet Windows path (no libSystem
// equivalent linked).
#    if !PLATFORM_WINDOWS
// Type is `unsigned long` to match what gcc/clang's canary
// instrumentation expects on every supported Misra target
// (LP64 -- 8 bytes on x86_64 and aarch64 Linux/Mac). Don't use
// size_t here: glibc / libSystem's strong defs are unsigned long,
// and a type mismatch on the weak symbol would surface as a link
// warning when the strong def is around.
__attribute__((weak, used)) unsigned long __stack_chk_guard;

// LogWrite + Abort, not LOG_FATAL. The macro builds a HeapAllocator
// + Str through the format pipeline, and from a corrupted-stack
// state any of those touches could segfault on already-bad memory.
// LogWrite takes a const-string message directly -- no allocations,
// no formatting, no shared state.
__attribute__((weak, used, noreturn)) void __stack_chk_fail(void) {
    LogWrite(
        LOG_MESSAGE_TYPE_FATAL,
        "__stack_chk_fail",
        0,
        "stack-protector: canary corrupted -- buffer overflow detected"
    );
    Abort();
    // Abort() is not declared noreturn in Misra/Std/Log.h (it's a
    // function pointer indirection through OnAbort in some
    // builds, so the compiler can't prove termination). Give the
    // compiler the noreturn guarantee explicitly so this whole
    // function compiles clean as noreturn.
    __builtin_unreachable();
}
#    endif

// Windows-specific UCRT + compiler-runtime stubs (__security_cookie,
// __chkstk, _fltused, __imp___stdio_common_vsprintf) live in
// _WinStubs.c -- a separate TU that's linked only into freestanding
// Bin/ tools (via per-target sources in meson.build), NOT into
// libmisra_std.a. Otherwise the test bins and FuzzHarness, which keep
// UCRT and pull __security_cookie from msvcrtd.lib(gs_cookie.obj),
// would see our def too and fail with 'duplicate symbol'.

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

// ---------------------------------------------------------------------------
// __chkstk_darwin -- Mac-only large-frame stack probe shim.
//
// Clang on Darwin emits an implicit call to ___chkstk_darwin in the
// prologue of any function whose stack frame exceeds ~4 KiB. The real
// libSystem implementation walks the frame in 4-KiB chunks, touching
// each page so the kernel demand-faults it in before the function
// actually uses it -- a safeguard for OS-default 8 MiB stacks where
// the guard page sits right below the live SP.
//
// For a libc-diet Mac build we'd otherwise have to drag libSystem in
// just for this probe. The Misra runtime never allocates the kind of
// gigantic-on-stack buffers where the probe matters (large allocations
// go through Allocator); and even when it did, the kernel's standard
// stack handling tolerates an unprobed grow as long as we're not
// jumping over the guard page. Replace with a do-nothing trampoline.
//
// Important: this is a STUB -- functions with > stack-size frames
// would still fault. Acceptable here because no in-tree code goes
// near that limit; if it ever did the right answer is to refactor
// that frame, not to re-add the probe.
//
// Mach-O symbol mangling: the C name we want to export is
// "___chkstk_darwin" (three underscores) which is the Mach-O encoding
// of "__chkstk_darwin" (two underscores) at C level. asm() rename
// pins the symbol.
#if PLATFORM_DARWIN && (ARCHITECTURE_X86_64 || ARCHITECTURE_AARCH64)
__attribute__((naked, used)) void __chkstk_darwin(void) __asm__("___chkstk_darwin");
__attribute__((naked, used)) void __chkstk_darwin(void) {
#    if ARCHITECTURE_X86_64
    __asm__("ret\n");
#    else // ARCHITECTURE_AARCH64
    __asm__("ret\n");
#    endif
}
#endif

#if PLATFORM_LINUX && (ARCHITECTURE_X86_64 || ARCHITECTURE_AARCH64)

#    if ARCHITECTURE_X86_64

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

#    else  // ARCHITECTURE_AARCH64

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

#endif     // PLATFORM_LINUX && (ARCHITECTURE_X86_64 || ARCHITECTURE_AARCH64)
