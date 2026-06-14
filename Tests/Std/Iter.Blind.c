#include <Misra/Std/Container/Buf.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Utility/Iter.h>
#include <Misra/Types.h>

#include "../Util/TestRunner.h"

// No public reverse-iter constructor yet; open-coded (mirrors Iter.c).
static BufIter from_rev(const u8 *p, size n) {
    BufIter it = {.data = p, .length = n, .pos = n - 1, .alignment = 1, .dir = -1};
    return it;
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
    WriteFmt("[INFO] Starting Iter.Blind tests\n\n");
    TestFunction tests[] = {
        test_remaining_reverse_pos_eq_length_is_zero,
    };
    int total = sizeof(tests) / sizeof(tests[0]);
    return run_test_suite(tests, total, NULL, 0, "Iter.Blind");
}
