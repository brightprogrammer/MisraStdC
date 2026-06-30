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

// --- iter_peek_index: reverse past-start sentinel handling (34:17, 35:13) ---
// From the reverse sentinel (pos == (size)-1, cur == -1) a peek of +1 must
// resolve to index 0. If `dir == -1` is mutated to `!=`, or `cur = -1`
// becomes `cur = 42`, the computed base is wrong and the peek fails.
bool test_it_peek_from_reverse_sentinel(void) {
    const u8 buf[3] = {10, 20, 30};
    BufIter  it     = from_rev(buf, 3);
    // Step past the start so pos becomes the (size)-1 sentinel.
    IterMove(&it, 3);
    if (IterIndex(&it) != (size)-1) {
        return false;
    }
    u8 v = 0;
    // cur == -1, n == 1 -> target index 0 (value 10).
    if (!IterPeekAt(&it, 1, &v) || v != 10) {
        return false;
    }
    // cur == -1, n == 3 -> target index 2 (value 30).
    return IterPeekAt(&it, 3, &v) && v == 30;
}

// --- iter_peek_index: reverse non-sentinel uses real pos (34:34) ---
// A reverse iter NOT at the sentinel (pos == 2) must peek from pos, not -1.
// Mutating `pos == (size)-1` to `!=` forces the `cur = -1` branch here.
bool test_it_peek_reverse_nonsentinel_uses_pos(void) {
    const u8 buf[3] = {10, 20, 30};
    BufIter  it     = from_rev(buf, 3);
    if (IterIndex(&it) != 2) {
        return false;
    }
    u8 v = 0;
    // cur == 2, n == 0 -> index 2 (value 30).
    if (!IterPeekAt(&it, 0, &v) || v != 30) {
        return false;
    }
    // cur == 2, n == -2 -> index 0 (value 10).
    return IterPeekAt(&it, -2, &v) && v == 10;
}

// --- iter_try_move: reverse move out of the sentinel (48:17, 49:13) ---
// From the sentinel (cur == -1), moving by n == -1 (delta +1) lands on
// index 0. Mutating `dir == -1` to `!=`, or `cur = -1` to `cur = 42`,
// makes the base wrong and the move is rejected.
bool test_it_move_out_of_reverse_sentinel(void) {
    const u8 buf[3] = {10, 20, 30};
    BufIter  it     = from_rev(buf, 3);
    IterMove(&it, 3); // -> sentinel
    if (IterIndex(&it) != (size)-1) {
        return false;
    }
    // cur == -1, n == -1 -> new_pos 0.
    if (!IterMove(&it, -1) || IterIndex(&it) != 0) {
        return false;
    }
    u8 v = 0;
    return IterRead(&it, &v) && v == 10;
}

// --- iter_try_move reverse upper bound is length-1 (64:37) ---
// Moving to exactly new_pos == length-1 must succeed. `>` mutated to `>=`
// would reject the top valid position.
bool test_it_move_reverse_to_max_position(void) {
    const u8 buf[3] = {10, 20, 30};
    BufIter  it     = from_rev(buf, 3);
    IterMove(&it, 3); // -> sentinel (cur == -1)
    // cur == -1, n == -3 -> delta +3 -> new_pos 2 == length-1 (top valid pos).
    if (!IterMove(&it, -3) || IterIndex(&it) != 2) {
        return false;
    }
    u8 v = 0;
    return IterRead(&it, &v) && v == 30;
}

// --- iter_try_move reverse rejects new_pos == length (64:55) ---
// The reverse upper bound is length-1, so landing on new_pos == length must
// be rejected. `length - 1` mutated to `length + 1` would wrongly accept it.
bool test_it_move_reverse_rejects_length(void) {
    const u8 buf[3] = {10, 20, 30};
    BufIter  it     = from_rev(buf, 3); // pos == 2
    size     before = IterIndex(&it);
    // cur == 2, n == -1 -> delta +1 -> new_pos 3 == length: out of range.
    if (IterMove(&it, -1)) {
        return false;
    }
    return IterIndex(&it) == before;
}

// --- iter_try_move reverse new_pos == 0 is a real index, not sentinel (67:28) ---
// Landing on new_pos == 0 must store pos 0, not the (size)-1 sentinel.
// `new_pos < 0` mutated to `<= 0` would store the sentinel instead.
bool test_it_move_reverse_to_index_zero(void) {
    const u8 buf[3] = {10, 20, 30};
    BufIter  it     = from_rev(buf, 3);
    IterMove(&it, 3); // -> sentinel
    // cur == -1, n == -1 -> new_pos 0: a real index, not the sentinel.
    if (!IterMove(&it, -1) || IterIndex(&it) != 0) {
        return false;
    }
    // Sentinel would read as exhausted; index 0 still has one element.
    return IterRemainingLength(&it) == 1;
}

// --- validate_iter accepts valid forward / reverse iters (80:17, 80:33) ---
// A structurally valid iter must not abort. Mutating either `!=` to `==`
// makes the validator abort on a valid iter (dir == -1 or dir == 1).
bool test_it_validate_accepts_forward(void) {
    const u8 buf[2] = {1, 2};
    BufIter  it     = BufIterFromMemory(buf, 2);
    ValidateIter(&it);
    return true;
}

bool test_it_validate_accepts_reverse(void) {
    const u8 buf[2] = {1, 2};
    BufIter  it     = from_rev(buf, 2);
    ValidateIter(&it);
    return true;
}

// --- remaining_length_iter reverse boundary: pos == length is exhausted (20:21) ---
// In the dir == -1 branch `mi->pos < IterLength(mi)` guards the "still
// has elements" case. A reverse iter whose `pos` has been forced equal
// to `length` (via IterTruncate) is past-the-end: remaining must be 0.
// Mutating `<` to `<=` makes `pos == length` pass the guard and return
// `pos + 1` (== length + 1 == 3 here) instead of 0.
bool test_remaining_reverse_pos_eq_length_is_zero(void) {
    const u8 buf[3] = {10, 20, 30};
    BufIter  it     = from_rev(buf, 3); // dir == -1, pos == 2, length == 3
    // length := pos + 0 == 2, so now pos (2) == length (2).
    IterTruncate(&it, 0);
    if (IterIndex(&it) != 2) {
        return false;
    }
    if (IterLength(&it) != 2) {
        return false;
    }
    // Original returns 0 (pos not < length); mutant returns pos + 1 == 3.
    return IterRemainingLength(&it) == 0;
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
        test_it_peek_from_reverse_sentinel,
        test_it_peek_reverse_nonsentinel_uses_pos,
        test_it_move_out_of_reverse_sentinel,
        test_it_move_reverse_to_max_position,
        test_it_move_reverse_rejects_length,
        test_it_move_reverse_to_index_zero,
        test_it_validate_accepts_forward,
        test_it_validate_accepts_reverse,
        test_remaining_reverse_pos_eq_length_is_zero,
    };
    int total = sizeof(tests) / sizeof(tests[0]);
    return run_test_suite(tests, total, NULL, 0, "Iter");
}
