#include <Misra/Std/Container/Buf.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Utility/Iter.h>
#include <Misra/Types.h>

#include "../Util/TestRunner.h"

// No public reverse-iter constructor yet; open-coded until a
// BufIterFromMemoryRev (or similar) is added.
static BufIter from_rev(const u8 *p, size n) {
    BufIter it = {.data = p, .length = n, .pos = n - 1, .alignment = 1, .dir = -1};
    return it;
}

bool test_iter_remaining_forward(void) {
    const u8 buf[3] = {1, 2, 3};
    BufIter  it     = BufIterFromMemory(buf, 3);
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
    BufIter  it     = from_rev(buf, 3);
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
    BufIter  it     = BufIterFromMemory(buf, 3);
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
    // Fourth read must fail: cursor sits at EOF after the three above.
    return !IterRead(&it, &v);
}

bool test_iter_read_reverse(void) {
    const u8 buf[3] = {10, 20, 30};
    BufIter  it     = from_rev(buf, 3);
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
    // Fourth read must fail: cursor has stepped past the start sentinel.
    return !IterRead(&it, &v);
}

bool test_iter_read_eof_leaves_state(void) {
    const u8 buf[1] = {7};
    BufIter  it     = BufIterFromMemory(buf, 1);
    u8       v      = 0;
    IterRead(&it, &v);
    size pos_before = IterIndex(&it);
    u8   sentinel   = 0xAA;
    v               = sentinel;
    if (IterRead(&it, &v)) {
        return false;
    }
    // Failed read must leave both `pos` and `*out` untouched.
    return IterIndex(&it) == pos_before && v == sentinel;
}

bool test_iter_peek_in_range(void) {
    const u8 buf[4] = {5, 6, 7, 8};
    BufIter  it     = BufIterFromMemory(buf, 4);
    IterMove(&it, 2);
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
    // Peeks must not advance the cursor.
    return IterIndex(&it) == 2;
}

bool test_iter_peek_out_of_range(void) {
    const u8 buf[2] = {1, 2};
    BufIter  it     = BufIterFromMemory(buf, 2);
    u8       v      = 0xAA;
    if (IterPeekAt(&it, 2, &v)) {
        return false;
    }
    if (v != 0xAA) {
        return false;
    }
    if (IterPeekAt(&it, -1, &v)) {
        return false;
    }
    // Out-of-range peeks (positive and negative) must leave *out untouched.
    return v == 0xAA;
}

bool test_iter_move_forward_basic(void) {
    const u8 buf[5] = {0};
    BufIter  it     = BufIterFromMemory(buf, 5);
    if (!IterMove(&it, 3) || IterIndex(&it) != 3) {
        return false;
    }
    if (!IterMove(&it, -2) || IterIndex(&it) != 1) {
        return false;
    }
    return true;
}

bool test_iter_move_forward_to_exhausted(void) {
    const u8 buf[3] = {0};
    BufIter  it     = BufIterFromMemory(buf, 3);
    if (!IterMove(&it, 3) || IterIndex(&it) != 3) {
        return false;
    }
    return IterRemainingLength(&it) == 0;
}

bool test_iter_move_forward_overflow(void) {
    const u8 buf[3] = {0};
    BufIter  it     = BufIterFromMemory(buf, 3);
    size     before = IterIndex(&it);
    if (IterMove(&it, 4)) {
        return false;
    }
    return IterIndex(&it) == before;
}

bool test_iter_move_forward_underflow(void) {
    const u8 buf[3] = {0};
    BufIter  it     = BufIterFromMemory(buf, 3);
    if (IterMove(&it, -1)) {
        return false;
    }
    return IterIndex(&it) == 0;
}

bool test_iter_move_reverse_basic(void) {
    const u8 buf[5] = {0};
    BufIter  it     = from_rev(buf, 5);
    // start at pos=4
    if (!IterMove(&it, 2) || IterIndex(&it) != 2) {
        return false;
    }
    // step backward in reverse direction (n=-1, effective +1)
    if (!IterMove(&it, -1) || IterIndex(&it) != 3) {
        return false;
    }
    return true;
}

bool test_iter_move_reverse_to_past_start(void) {
    const u8 buf[3] = {0};
    BufIter  it     = from_rev(buf, 3);
    // pos=2, dir=-1, move by 3 lands on sentinel pos=-1
    if (!IterMove(&it, 3) || IterIndex(&it) != (size)-1) {
        return false;
    }
    return IterRemainingLength(&it) == 0;
}

bool test_iter_move_reverse_overflow(void) {
    const u8 buf[3] = {0};
    BufIter  it     = from_rev(buf, 3);
    // pos=2, dir=-1, move by 4 would land at pos=-2 — invalid
    size before = IterIndex(&it);
    if (IterMove(&it, 4)) {
        return false;
    }
    return IterIndex(&it) == before;
}

bool test_iter_next_prev(void) {
    const u8 buf[3] = {0};
    BufIter  it     = BufIterFromMemory(buf, 3);
    if (!IterNext(&it) || IterIndex(&it) != 1) {
        return false;
    }
    if (!IterNext(&it) || IterIndex(&it) != 2) {
        return false;
    }
    if (!IterPrev(&it) || IterIndex(&it) != 1) {
        return false;
    }
    if (!IterPrev(&it) || IterIndex(&it) != 0) {
        return false;
    }
    // Cursor sits at the start; one more prev must underflow.
    return !IterPrev(&it);
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
