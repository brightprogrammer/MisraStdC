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

// --- deadend: validate_iter aborts on a bad direction (80:17, 80:33) ---
// dir == 0 is structurally invalid; the validator must abort. Mutating
// either `!=` to `==` makes the abort condition false, so the abort is
// skipped and this deadend test fails.
bool deadend_it_validate_bad_dir(void) {
    const u8 buf[2] = {1, 2};
    BufIter  it     = {.data = buf, .length = 2, .pos = 0, .alignment = 1, .dir = 0};
    ValidateIter(&it);
    return true;
}

// --- deadend: validate_iter aborts on zero alignment ---
bool deadend_it_validate_zero_alignment(void) {
    const u8 buf[2] = {1, 2};
    BufIter  it     = {.data = buf, .length = 2, .pos = 0, .alignment = 0, .dir = 1};
    ValidateIter(&it);
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
        deadend_it_validate_bad_dir,
        deadend_it_validate_zero_alignment,
    };
    int total = sizeof(tests) / sizeof(tests[0]);
    return run_test_suite(NULL, 0, tests, total, "Iter.Deadend");
}
