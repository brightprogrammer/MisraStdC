#include <Misra/Std/Log.h>
#include <Misra/Std/Utility/Iter.h>
#include <Misra/Types.h>

#include "../Util/TestRunner.h"

typedef Iter(const u8) ByteIter;

static ByteIter from(const u8 *p, size n) {
    ByteIter it = {.data = p, .length = n, .pos = 0, .alignment = 1, .dir = 1};
    return it;
}

static ByteIter from_rev(const u8 *p, size n) {
    ByteIter it = {.data = p, .length = n, .pos = n - 1, .alignment = 1, .dir = -1};
    return it;
}

bool test_iter_remaining_forward(void) {
    const u8 buf[3] = {1, 2, 3};
    ByteIter it     = from(buf, 3);
    if (IterRemainingLength(&it) != 3) {
        return false;
    }
    u8 v;
    IterRead(&it, &v);
    if (IterRemainingLength(&it) != 2) {
        return false;
    }
    IterRead(&it, &v);
    IterRead(&it, &v);
    return IterRemainingLength(&it) == 0;
}

bool test_iter_remaining_reverse(void) {
    const u8 buf[3] = {1, 2, 3};
    ByteIter it     = from_rev(buf, 3);
    if (IterRemainingLength(&it) != 3) {
        return false;
    }
    u8 v;
    IterRead(&it, &v);
    if (IterRemainingLength(&it) != 2) {
        return false;
    }
    IterRead(&it, &v);
    IterRead(&it, &v);
    return IterRemainingLength(&it) == 0;
}

bool test_iter_read_forward(void) {
    const u8 buf[3] = {10, 20, 30};
    ByteIter it     = from(buf, 3);
    u8       v      = 0;
    if (!IterRead(&it, &v) || v != 10) {
        return false;
    }
    if (!IterRead(&it, &v) || v != 20) {
        return false;
    }
    if (!IterRead(&it, &v) || v != 30) {
        return false;
    }
    return !IterRead(&it, &v); // EOF
}

bool test_iter_read_reverse(void) {
    const u8 buf[3] = {10, 20, 30};
    ByteIter it     = from_rev(buf, 3);
    u8       v      = 0;
    if (!IterRead(&it, &v) || v != 30) {
        return false;
    }
    if (!IterRead(&it, &v) || v != 20) {
        return false;
    }
    if (!IterRead(&it, &v) || v != 10) {
        return false;
    }
    return !IterRead(&it, &v); // past start
}

bool test_iter_read_eof_leaves_state(void) {
    const u8 buf[1] = {7};
    ByteIter it     = from(buf, 1);
    u8       v      = 0;
    IterRead(&it, &v); // consume sole element
    size pos_before = it.pos;
    u8   sentinel   = 0xAA;
    v               = sentinel;
    if (IterRead(&it, &v)) {
        return false; // should have failed
    }
    // Propagating: pos unchanged, *out not written
    return it.pos == pos_before && v == sentinel;
}

bool test_iter_peek_in_range(void) {
    const u8 buf[4] = {5, 6, 7, 8};
    ByteIter it     = from(buf, 4);
    IterMove(&it, 2); // pos == 2
    u8 v;
    if (!IterPeekAt(&it, 0, &v) || v != 7) {
        return false;
    }
    if (!IterPeekAt(&it, 1, &v) || v != 8) {
        return false;
    }
    if (!IterPeekAt(&it, -2, &v) || v != 5) {
        return false;
    }
    return it.pos == 2; // peek does not advance
}

bool test_iter_peek_out_of_range(void) {
    const u8 buf[2] = {1, 2};
    ByteIter it     = from(buf, 2);
    u8       v      = 0xAA;
    if (IterPeekAt(&it, 2, &v)) {
        return false; // out of range
    }
    if (v != 0xAA) {
        return false; // *out untouched
    }
    if (IterPeekAt(&it, -1, &v)) {
        return false; // negative out of range
    }
    return v == 0xAA;
}

bool test_iter_move_forward_basic(void) {
    const u8 buf[5] = {0};
    ByteIter it     = from(buf, 5);
    if (!IterMove(&it, 3) || it.pos != 3) {
        return false;
    }
    if (!IterMove(&it, -2) || it.pos != 1) {
        return false;
    }
    return true;
}

bool test_iter_move_forward_to_exhausted(void) {
    const u8 buf[3] = {0};
    ByteIter it     = from(buf, 3);
    if (!IterMove(&it, 3) || it.pos != 3) {
        return false;
    }
    return IterRemainingLength(&it) == 0;
}

bool test_iter_move_forward_overflow(void) {
    const u8 buf[3] = {0};
    ByteIter it     = from(buf, 3);
    size     before = it.pos;
    if (IterMove(&it, 4)) {
        return false; // 4 > length
    }
    return it.pos == before;
}

bool test_iter_move_forward_underflow(void) {
    const u8 buf[3] = {0};
    ByteIter it     = from(buf, 3);
    if (IterMove(&it, -1)) {
        return false;
    }
    return it.pos == 0;
}

bool test_iter_move_reverse_basic(void) {
    const u8 buf[5] = {0};
    ByteIter it     = from_rev(buf, 5);
    // start at pos=4
    if (!IterMove(&it, 2) || it.pos != 2) {
        return false;
    }
    // step backward in reverse direction (n=-1, effective +1)
    if (!IterMove(&it, -1) || it.pos != 3) {
        return false;
    }
    return true;
}

bool test_iter_move_reverse_to_past_start(void) {
    const u8 buf[3] = {0};
    ByteIter it     = from_rev(buf, 3);
    // pos=2, dir=-1, move by 3 lands on sentinel pos=-1
    if (!IterMove(&it, 3) || it.pos != (size)-1) {
        return false;
    }
    return IterRemainingLength(&it) == 0;
}

bool test_iter_move_reverse_overflow(void) {
    const u8 buf[3] = {0};
    ByteIter it     = from_rev(buf, 3);
    // pos=2, dir=-1, move by 4 would land at pos=-2 — invalid
    size before = it.pos;
    if (IterMove(&it, 4)) {
        return false;
    }
    return it.pos == before;
}

bool test_iter_next_prev(void) {
    const u8 buf[3] = {0};
    ByteIter it     = from(buf, 3);
    if (!IterNext(&it) || it.pos != 1) {
        return false;
    }
    if (!IterNext(&it) || it.pos != 2) {
        return false;
    }
    if (!IterPrev(&it) || it.pos != 1) {
        return false;
    }
    if (!IterPrev(&it) || it.pos != 0) {
        return false;
    }
    return !IterPrev(&it); // underflow
}

int main(void) {
    WriteFmt("[INFO] Starting Iter tests\n\n");
    TestFunction tests[] = {
        test_iter_remaining_forward,
        test_iter_remaining_reverse,
        test_iter_read_forward,
        test_iter_read_reverse,
        test_iter_read_eof_leaves_state,
        test_iter_peek_in_range,
        test_iter_peek_out_of_range,
        test_iter_move_forward_basic,
        test_iter_move_forward_to_exhausted,
        test_iter_move_forward_overflow,
        test_iter_move_forward_underflow,
        test_iter_move_reverse_basic,
        test_iter_move_reverse_to_past_start,
        test_iter_move_reverse_overflow,
        test_iter_next_prev,
    };
    int total = sizeof(tests) / sizeof(tests[0]);
    return run_test_suite(tests, total, NULL, 0, "Iter");
}
