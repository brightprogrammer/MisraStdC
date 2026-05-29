/// file      : Tests/Util/TestRunner.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Test utilities for running potentially failing tests using a
/// hand-rolled register save/restore pair (`SetJmp` / `LongJmp`).
///
/// We do NOT use libc's `setjmp` / `longjmp` because clang-cl on
/// Windows x86_64 maps `setjmp` (the macro in <setjmp.h>) to the
/// `_setjmpex` intrinsic. `_setjmpex` is SEH-aware: `longjmp` then
/// goes through `RtlUnwindEx`, which iterates each intermediate
/// frame's RUNTIME_FUNCTION entry. Any noreturn-shaped helper on the
/// unwind path (notably `Abort` -> trap intrinsic) trips
/// STATUS_BAD_STACK (0xC0000028) the moment RtlUnwindEx reaches it.
///
/// The asm pair below saves only callee-saved scalars (no SEH
/// metadata, no unwind handlers). Output is identical to a normal
/// function return, so the abort callback can longjmp back into the
/// test runner without dragging unwinder semantics in.

#include "TestRunner.h"
#include <Misra/Sys.h>
#include <Misra/Std.h>

// JmpBuf layout per platform. Each slot is 8 bytes.
//
// The hand-rolled asm pair below only works with toolchains that
// support `__attribute__((naked))` + GNU-style inline asm: GCC,
// Clang, and clang-cl on Windows. Plain MSVC (`cl.exe`) understands
// neither, so the MSVC branch falls back to libc `setjmp`/`longjmp`
// -- those bind to MSVC's `_setjmpex` intrinsic, but cl.exe's SEH
// metadata for noreturn frames doesn't trip the unwinder the way
// clang-cl's does (the original Windows STATUS_BAD_STACK failure),
// so this is fine in practice.
//
// AArch64 (Linux, Darwin) saves x19-x28 + fp + lr + sp.
#if defined(_MSC_VER) && !defined(__clang__)
#    include <setjmp.h>
typedef jmp_buf JmpBuf;
#    define SetJmp(env)       setjmp(env)
#    define LongJmp(env, val) longjmp((env), (val))
#elif (PLATFORM_LINUX || PLATFORM_DARWIN || PLATFORM_WINDOWS) && ARCHITECTURE_X86_64
typedef u64 JmpBuf[10]; // rbx, rbp, r12-r15, rdi, rsi, rsp, rip
#elif (PLATFORM_LINUX || PLATFORM_DARWIN) && ARCHITECTURE_AARCH64
typedef u64 JmpBuf[13]; // x19-x28, x29 (fp), x30 (lr), sp
#else
#    error "TestRunner: no SetJmp/LongJmp impl for this platform/arch"
#endif

#if PLATFORM_WINDOWS && ARCHITECTURE_X86_64 && defined(__clang__)
// Win64 ABI: 1st arg in RCX, 2nd arg in RDX. Callee-saved scalars:
// RBX, RBP, RDI, RSI, R12-R15, RSP. XMM6-XMM15 are also callee-saved
// per Win64 but the deadend test path doesn't touch them in ways that
// need preserving.
__attribute__((naked, used)) static int SetJmp(JmpBuf env) {
    __asm__(
        "movq %rbx,  0(%rcx)\n"
        "movq %rbp,  8(%rcx)\n"
        "movq %r12, 16(%rcx)\n"
        "movq %r13, 24(%rcx)\n"
        "movq %r14, 32(%rcx)\n"
        "movq %r15, 40(%rcx)\n"
        "movq %rdi, 48(%rcx)\n"
        "movq %rsi, 56(%rcx)\n"
        "leaq 8(%rsp), %rax\n" // caller's rsp (skip our return addr)
        "movq %rax, 64(%rcx)\n"
        "movq (%rsp), %rax\n"  // caller's rip
        "movq %rax, 72(%rcx)\n"
        "xorl %eax, %eax\n"    // return 0 on the SetJmp path
        "ret\n"
    );
}

__attribute__((naked, used, noreturn)) static void LongJmp(JmpBuf env, int val) {
    __asm__(
        "movq  0(%rcx), %rbx\n"
        "movq  8(%rcx), %rbp\n"
        "movq 16(%rcx), %r12\n"
        "movq 24(%rcx), %r13\n"
        "movq 32(%rcx), %r14\n"
        "movq 40(%rcx), %r15\n"
        "movq 48(%rcx), %rdi\n"
        "movq 56(%rcx), %rsi\n"
        "movq 64(%rcx), %rsp\n"
        "movl %edx, %eax\n" // return val, normalised so 0 -> 1
        "testl %eax, %eax\n"
        "jnz 1f\n"
        "incl %eax\n"
        "1: jmpq *72(%rcx)\n"
    );
}
#elif PLATFORM_LINUX && ARCHITECTURE_X86_64
// SysV AMD64: 1st arg in RDI, 2nd arg in RSI.
__attribute__((naked, used)) static int SetJmp(JmpBuf env) {
    __asm__(
        "movq %rbx,  0(%rdi)\n"
        "movq %rbp,  8(%rdi)\n"
        "movq %r12, 16(%rdi)\n"
        "movq %r13, 24(%rdi)\n"
        "movq %r14, 32(%rdi)\n"
        "movq %r15, 40(%rdi)\n"
        "leaq 8(%rsp), %rax\n"
        "movq %rax, 48(%rdi)\n"
        "movq (%rsp), %rax\n"
        "movq %rax, 56(%rdi)\n"
        "xorl %eax, %eax\n"
        "ret\n"
    );
}

__attribute__((naked, used, noreturn)) static void LongJmp(JmpBuf env, int val) {
    __asm__(
        "movq  0(%rdi), %rbx\n"
        "movq  8(%rdi), %rbp\n"
        "movq 16(%rdi), %r12\n"
        "movq 24(%rdi), %r13\n"
        "movq 32(%rdi), %r14\n"
        "movq 40(%rdi), %r15\n"
        "movq 48(%rdi), %rsp\n"
        "movl %esi, %eax\n"
        "testl %eax, %eax\n"
        "jnz 1f\n"
        "incl %eax\n"
        "1: jmpq *56(%rdi)\n"
    );
}
#elif PLATFORM_DARWIN && ARCHITECTURE_X86_64
// Darwin SysV AMD64: same arg regs as Linux. Mach-O symbol prefixing
// is handled by the assembler; no special action needed for a static.
__attribute__((naked, used)) static int SetJmp(JmpBuf env) {
    __asm__(
        "movq %rbx,  0(%rdi)\n"
        "movq %rbp,  8(%rdi)\n"
        "movq %r12, 16(%rdi)\n"
        "movq %r13, 24(%rdi)\n"
        "movq %r14, 32(%rdi)\n"
        "movq %r15, 40(%rdi)\n"
        "leaq 8(%rsp), %rax\n"
        "movq %rax, 48(%rdi)\n"
        "movq (%rsp), %rax\n"
        "movq %rax, 56(%rdi)\n"
        "xorl %eax, %eax\n"
        "ret\n"
    );
}

__attribute__((naked, used, noreturn)) static void LongJmp(JmpBuf env, int val) {
    __asm__(
        "movq  0(%rdi), %rbx\n"
        "movq  8(%rdi), %rbp\n"
        "movq 16(%rdi), %r12\n"
        "movq 24(%rdi), %r13\n"
        "movq 32(%rdi), %r14\n"
        "movq 40(%rdi), %r15\n"
        "movq 48(%rdi), %rsp\n"
        "movl %esi, %eax\n"
        "testl %eax, %eax\n"
        "jnz 1f\n"
        "incl %eax\n"
        "1: jmpq *56(%rdi)\n"
    );
}
#elif (PLATFORM_LINUX || PLATFORM_DARWIN) && ARCHITECTURE_AARCH64
// AAPCS64: 1st arg in X0, 2nd arg in X1. Callee-saved scalars:
// X19-X28, X29 (fp), X30 (lr). Plus SP.
__attribute__((naked, used)) static int SetJmp(JmpBuf env) {
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

__attribute__((naked, used, noreturn)) static void LongJmp(JmpBuf env, int val) {
    __asm__(
        "ldp x19, x20, [x0,  #0]\n"
        "ldp x21, x22, [x0, #16]\n"
        "ldp x23, x24, [x0, #32]\n"
        "ldp x25, x26, [x0, #48]\n"
        "ldp x27, x28, [x0, #64]\n"
        "ldp x29, x30, [x0, #80]\n"
        "ldr x2,       [x0, #96]\n"
        "mov sp, x2\n"
        "mov w0, w1\n"
        "cbnz w0, 1f\n"
        "mov w0, #1\n"
        "1: ret\n"
    );
}
#endif

// Global jump buffer for capturing aborts
static JmpBuf g_test_abort_jmp;

// Callback installed via `OnAbort` (Misra/Sys.h). When LOG_FATAL fires
// inside a deadend test, `Abort()` invokes this hook instead of running
// the trap intrinsic, and we unwind back into `test_deadend` via the
// hand-rolled register pair below.
static void test_abort_handler(void) {
    LongJmp(g_test_abort_jmp, 1);
}

// Run a specific test using the SetJmp/LongJmp pair below to capture
// LOG_FATAL aborts.
// `expect_failure=true`  -> pass iff the test aborts.
// `expect_failure=false` -> pass iff the test returns true without aborting.
// Both paths go through setjmp: a `false`-expectation that nevertheless
// aborts must unwind back here too, otherwise the abort handler would
// longjmp to an uninitialised buffer.
bool test_deadend(TestFunction test_func, bool expect_failure) {
    if (!test_func) {
        WriteFmt("[ERROR] test_deadend: NULL test function provided\n");
        return false;
    }

    // Install the abort capture handler for the duration of this call.
    OnAbort(test_abort_handler);

    bool test_result = false;
    if (SetJmp(g_test_abort_jmp) == 0) {
        // First entry: run the test.
        bool returned = test_func();
        if (expect_failure) {
            WriteFmt("    [Unexpected success: Test completed without abort]\n");
            test_result = false;    // Expected abort, got clean return.
        } else {
            WriteFmt("    [Success: Test completed normally]\n");
            test_result = returned; // Caller's bool is the verdict.
        }
    } else {
        // Re-entry via longjmp: the test triggered LOG_FATAL.
        if (expect_failure) {
            WriteFmt("    [Expected failure: Test aborted as expected]\n");
            test_result = true; // Abort was the contract.
        } else {
            WriteFmt("    [Unexpected failure: Test aborted unexpectedly]\n");
            test_result = false;
        }
    }

    // Restore the default abort path before returning.
    OnAbort(NULL);

    return test_result;
}

/// Run an array of simple tests
int simple_test_driver(TestFunction *tests, int count) {
    if (!tests) {
        WriteFmt("[ERROR] simple_test_driver: NULL tests array provided\n");
        return count; // All tests failed
    }

    int passed = 0;
    int failed = 0;

    // Run all tests and accumulate results
    for (int i = 0; i < count; i++) {
        WriteFmt("[TEST {}/{}] ", i + 1, count);
        bool result = tests[i]();
        if (result) {
            WriteFmt("[PASS]\n\n");
            passed++;
        } else {
            WriteFmt("[FAIL]\n\n");
            failed++;
        }
    }

    // Print summary
    WriteFmt("[SUMMARY] Total: {}, Passed: {}, Failed: {}\n", count, passed, failed);

    return failed;
}

/// Run an array of deadend tests (all expecting failure)
int deadend_test_driver(TestFunction *tests, int count) {
    if (!tests) {
        WriteFmt("[ERROR] deadend_test_driver: NULL tests array provided\n");
        return count; // All tests failed
    }

    WriteFmt("\n[INFO] Testing deadend scenarios\n\n");

    int passed = 0;
    int failed = 0;

    // Run all deadend tests (expecting failure)
    for (int i = 0; i < count; i++) {
        WriteFmt("[TEST {}/{}] ", i + 1, count);
        bool result = test_deadend(tests[i], true); // All deadend tests expect failure
        if (result) {
            WriteFmt("[PASS]\n\n");
            passed++;
        } else {
            WriteFmt("[FAIL]\n\n");
            failed++;
        }
    }

    // Print summary
    WriteFmt("[SUMMARY] Deadend tests - Total: {}, Passed: {}, Failed: {}\n", count, passed, failed);

    return failed;
}

/// Main test driver - handles everything: normal tests and deadend tests
int run_test_suite(
    TestFunction *normal_tests,
    int           normal_count,
    TestFunction *deadend_tests,
    int           deadend_count,
    Zstr          test_name
) {
    WriteFmt("[INFO] Starting {} tests\n\n", test_name ? test_name : "Test Suite");

    int total_failed = 0;

    // Run normal tests if any
    if (normal_tests && normal_count > 0) {
        int failed    = simple_test_driver(normal_tests, normal_count);
        total_failed += failed;
    }

    // Run deadend tests if any
    if (deadend_tests && deadend_count > 0) {
        int deadend_failed  = deadend_test_driver(deadend_tests, deadend_count);
        total_failed       += deadend_failed;
    }

    // Print final summary
    WriteFmt(
        "\n[FINAL SUMMARY] {} - Normal: {} tests, Deadend: {} tests, Total Failed: {}\n",
        test_name ? test_name : "Test Suite",
        normal_count,
        deadend_count,
        total_failed
    );

    // Return non-zero exit code if any test failed
    return total_failed > 0 ? 1 : 0;
}
