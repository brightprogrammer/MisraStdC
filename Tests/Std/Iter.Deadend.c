#include <Misra/Std/Log.h>
#include <Misra/Std/Utility/Iter.h>
#include <Misra/Types.h>

#include "../Util/TestRunner.h"

typedef Iter(const u8) ByteIter;

static ByteIter from(const u8 *p, size n) {
    ByteIter it = {.data = p, .length = n, .pos = 0, .alignment = 1, .dir = 1};
    return it;
}

bool deadend_must_read_eof(void) {
    const u8 buf[1] = {1};
    ByteIter it     = from(buf, 1);
    u8       v;
    IterRead(&it, &v); // consume sole element
    // Now exhausted - this must abort.
    IterMustRead(&it, &v);
    return true; // unreachable
}

bool deadend_must_peek_out_of_range(void) {
    const u8 buf[1] = {1};
    ByteIter it     = from(buf, 1);
    u8       v;
    // pos=0, length=1: peek at +1 is out of range.
    IterMustPeekAt(&it, 1, &v);
    return true; // unreachable
}

bool deadend_must_move_overflow(void) {
    const u8 buf[3] = {0};
    ByteIter it     = from(buf, 3);
    // length=3, dir=+1: move by 4 lands past end.
    IterMustMove(&it, 4);
    return true; // unreachable
}

bool deadend_must_next_eof(void) {
    const u8 buf[1] = {0};
    ByteIter it     = from(buf, 1);
    u8       v;
    IterRead(&it, &v); // pos=1, exhausted
    IterMustNext(&it);
    return true;       // unreachable
}

bool deadend_must_prev_underflow(void) {
    const u8 buf[3] = {0};
    ByteIter it     = from(buf, 3);
    // pos=0, dir=+1: prev would land at -1.
    IterMustPrev(&it);
    return true; // unreachable
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
