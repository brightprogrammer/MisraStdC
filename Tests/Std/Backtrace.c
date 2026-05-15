#include <Misra.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Sys/Backtrace.h>

#include <string.h>

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
    bool ok = rendered.length > 0 && strstr(rendered.data, "bt_capture_with_helper") != NULL &&
              strstr(rendered.data, "bt_capture_outer") != NULL;

    StrDeinit(&rendered);
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
    bool ok = out.length > 0 && strstr(out.data, "bt_capture_with_helper") != NULL;

    StrDeinit(&out);
    SymbolResolverDeinit(&res);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

int main(void) {
    WriteFmt("[INFO] Starting Backtrace tests\n\n");

    TestFunction tests[] = {
        test_backtrace_capture_non_empty,
        test_backtrace_format_resolves_helper,
        test_backtrace_format_with_shared_resolver,
    };

    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), NULL, 0, "Backtrace");
}
