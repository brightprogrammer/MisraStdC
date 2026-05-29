#include <Misra/Std/Container/Buf.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Utility/Iter.h>
#include <Misra/Types.h>

#include "../Util/TestRunner.h"

// Each test exhausts the iter (or constructs an out-of-range op) and
// then calls a `*Must*` variant -- which must abort. The `return true`
// tail is unreachable in a passing run; the harness's abort callback
// is what records success.

bool deadend_must_read_eof(void) {
    const u8 buf[1] = {1};
    BufIter  it     = BufIterFromMemory(buf, 1);
    u8       v;
    IterRead(&it, &v);
    IterMustRead(&it, &v);
    return true;
}

bool deadend_must_peek_out_of_range(void) {
    const u8 buf[1] = {1};
    BufIter  it     = BufIterFromMemory(buf, 1);
    u8       v;
    IterMustPeekAt(&it, 1, &v);
    return true;
}

bool deadend_must_move_overflow(void) {
    const u8 buf[3] = {0};
    BufIter  it     = BufIterFromMemory(buf, 3);
    IterMustMove(&it, 4);
    return true;
}

bool deadend_must_next_eof(void) {
    const u8 buf[1] = {0};
    BufIter  it     = BufIterFromMemory(buf, 1);
    u8       v;
    IterRead(&it, &v);
    IterMustNext(&it);
    return true;
}

bool deadend_must_prev_underflow(void) {
    const u8 buf[3] = {0};
    BufIter  it     = BufIterFromMemory(buf, 3);
    IterMustPrev(&it);
    return true;
}

int main(void) {
    WriteFmt("[INFO] Starting Iter.Deadend tests\n\n");
    TestFunction tests[] = {
        deadend_must_read_eof,
        deadend_must_peek_out_of_range,
        deadend_must_move_overflow,
        deadend_must_next_eof,
        deadend_must_prev_underflow,
    };
    int total = sizeof(tests) / sizeof(tests[0]);
    return run_test_suite(NULL, 0, tests, total, "Iter.Deadend");
}
