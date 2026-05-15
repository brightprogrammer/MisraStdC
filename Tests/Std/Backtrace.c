#include <Misra.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Sys/Backtrace.h>


#include "../Util/TestRunner.h"

// Named helpers so we can verify a specific symbol shows up in the
// captured trace.
static __attribute__((noinline)) size bt_capture_with_helper(StackFrame *frames, size max) {
    return CaptureStackTrace(frames, max, 0);
}

static __attribute__((noinline)) size bt_capture_outer(StackFrame *frames, size max) {
    return bt_capture_with_helper(frames, max);
}

bool test_backtrace_capture_non_empty(void) {
    StackFrame frames[32];
    size       n = bt_capture_outer(frames, 32);
    return n >= 2 && n <= 32; // at least helper + outer
}

bool test_backtrace_format_resolves_helper(void) {
    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    StackFrame frames[32];
    size       n = bt_capture_outer(frames, 32);

    Str rendered = StrInit(alloc_base);
    FormatStackTrace(&rendered, frames, n, alloc_base);

    // We expect both helper names to appear in the rendered trace.
    bool ok = rendered.length > 0 && ZstrFindSubstring(rendered.data, "bt_capture_with_helper") != NULL &&
              ZstrFindSubstring(rendered.data, "bt_capture_outer") != NULL;

    // With -gdwarf-4 the source location should be in there too.
    ok = ok && ZstrFindSubstring(rendered.data, "Backtrace.c") != NULL;

    StrDeinit(&rendered);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Vec-form versions of the capture helpers; same call shape as the
// raw helpers above, just routed through the StackFrames overload.
static __attribute__((noinline)) bool bt_vec_capture_with_helper(StackFrames *out) {
    return CaptureStackTrace(out, 0);
}

static __attribute__((noinline)) bool bt_vec_capture_outer(StackFrames *out) {
    return bt_vec_capture_with_helper(out);
}

// Vec-form capture + format end to end: same nested-helper shape as
// the raw test, so the rendered output should contain
// bt_vec_capture_with_helper + bt_vec_capture_outer.
bool test_backtrace_vec_form_resolves_helper(void) {
    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    StackFrames frames = VecInitT(frames, alloc_base);
    bool        ok     = bt_vec_capture_outer(&frames);
    ok                 = ok && frames.length >= 2;

    Str rendered = StrInit(alloc_base);
    FormatStackTrace(&rendered, &frames, alloc_base);

    ok = ok && rendered.length > 0;
    ok = ok && ZstrFindSubstring(rendered.data, "bt_vec_capture_with_helper") != NULL;
    ok = ok && ZstrFindSubstring(rendered.data, "bt_vec_capture_outer") != NULL;

    StrDeinit(&rendered);
    VecDeinit(&frames);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

bool test_backtrace_format_with_shared_resolver(void) {
    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    SymbolResolver res;
    if (!SymbolResolverInit(&res, alloc_base)) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    StackFrame frames[16];
    size       n = bt_capture_outer(frames, 16);

    Str out = StrInit(alloc_base);
    FormatStackTraceWith(&out, frames, n, &res);

    // Should also contain the helper name through the shared resolver.
    bool ok = out.length > 0 && ZstrFindSubstring(out.data, "bt_capture_with_helper") != NULL;

    StrDeinit(&out);
    SymbolResolverDeinit(&res);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

#if MISRA_HAVE_PARSER_DWARF && defined(__x86_64__)

// Same nested-call shape as the FP-walk tests, but routes through the
// CFI walker. The walker does not depend on -fno-omit-frame-pointer;
// here we still get FP frames (test build flag), but the unwinder is
// driven by .eh_frame rules in either case.
static __attribute__((noinline)) size cfi_capture_inner(SymbolResolver *r, StackFrame *frames, size max) {
    return CaptureStackTraceCfi(frames, max, 0, r);
}

static __attribute__((noinline)) size cfi_capture_outer(SymbolResolver *r, StackFrame *frames, size max) {
    return cfi_capture_inner(r, frames, max);
}

bool test_backtrace_cfi_walks_multi_frame(void) {
    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    SymbolResolver res;
    if (!SymbolResolverInit(&res, alloc_base)) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    StackFrame frames[32];
    size       n = cfi_capture_outer(&res, frames, 32);

    Str rendered = StrInit(alloc_base);
    FormatStackTraceWith(&rendered, frames, n, &res);

    // The two named helpers must show up: the CFI walker successfully
    // unwound across at least two real-code frames.
    bool ok = n >= 2;
    ok      = ok && rendered.length > 0;
    ok      = ok && ZstrFindSubstring(rendered.data, "cfi_capture_inner") != NULL;
    ok      = ok && ZstrFindSubstring(rendered.data, "cfi_capture_outer") != NULL;

    StrDeinit(&rendered);
    SymbolResolverDeinit(&res);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// FP-walk and CFI-walk should agree on a healthy fraction of IPs. The
// two captures happen at almost-identical PCs (one line apart in the
// same function), so the deeper frames — anything outside the leaf —
// must match exactly. We accept the leaf differing: each capture path
// snapshots at a slightly different PC inside its respective capture
// function.
static __attribute__((noinline)) size fp_capture_inner(StackFrame *frames, size max) {
    return CaptureStackTrace(frames, max, 0);
}

static __attribute__((noinline)) void cfi_vs_fp_inner(
    SymbolResolver *r,
    StackFrame     *fp_frames,
    size           *fp_n,
    StackFrame     *cfi_frames,
    size           *cfi_n,
    size            max
) {
    *fp_n  = fp_capture_inner(fp_frames, max);
    *cfi_n = CaptureStackTraceCfi(cfi_frames, max, 0, r);
}

bool test_backtrace_cfi_agrees_with_fp(void) {
    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    SymbolResolver res;
    if (!SymbolResolverInit(&res, alloc_base)) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    StackFrame fp[32], cfi[32];
    size       fp_n = 0, cfi_n = 0;
    cfi_vs_fp_inner(&res, fp, &fp_n, cfi, &cfi_n, 32);

    // Both must capture at least the leaf + test_* + run_test_suite.
    bool ok = fp_n >= 3 && cfi_n >= 3;

    // Skip frame 0 in each (different capture functions); compare the
    // shorter of the remaining tails. They should match exactly for at
    // least two frames (the rest may differ at the bottom where the
    // test runner unwinds into libc or __libc_start_main differently).
    if (ok) {
        size matches = 0;
        size tail    = (fp_n < cfi_n ? fp_n : cfi_n) - 1;
        for (size i = 1; i <= tail; ++i) {
            if (fp[i].ip == cfi[i].ip)
                ++matches;
        }
        ok = matches >= 2;
    }

    SymbolResolverDeinit(&res);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

#endif // MISRA_HAVE_PARSER_DWARF && __x86_64__

int main(void) {
    WriteFmt("[INFO] Starting Backtrace tests\n\n");

    TestFunction tests[] = {
        test_backtrace_capture_non_empty,
        test_backtrace_format_resolves_helper,
        test_backtrace_vec_form_resolves_helper,
        test_backtrace_format_with_shared_resolver,
#if MISRA_HAVE_PARSER_DWARF && defined(__x86_64__)
        test_backtrace_cfi_walks_multi_frame,
        test_backtrace_cfi_agrees_with_fp,
#endif
    };

    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), NULL, 0, "Backtrace");
}
