#include <Misra/Std/Log.h>
#include <Misra/Std/Utility/StrIter.h>
#include <Misra/Types.h>

#include "../../Util/TestRunner.h"

// StrIterFromZstr takes the length from ZstrLen and excludes the NUL:
// the iterator walks exactly the visible characters, alignment 1, forward.
bool test_striter_from_zstr_length_and_read(void) {
    StrIter si = StrIterFromZstr("abc");
    if (StrIterRemainingLength(&si) != 3) {
        return false;
    }
    char c = 0;
    if (!StrIterRead(&si, &c) || c != 'a') {
        return false;
    }
    if (!StrIterRead(&si, &c) || c != 'b') {
        return false;
    }
    if (!StrIterRead(&si, &c) || c != 'c') {
        return false;
    }
    // The NUL terminator is not part of the range: a fourth read fails.
    return !StrIterRead(&si, &c);
}

// StrIterFromCstr caps the range at the caller-supplied length, ignoring
// bytes that sit past it in the backing buffer.
bool test_striter_from_cstr_caps_length(void) {
    StrIter si = StrIterFromCstr("abcd", 2);
    if (StrIterRemainingLength(&si) != 2) {
        return false;
    }
    char c = 0;
    if (!StrIterRead(&si, &c) || c != 'a') {
        return false;
    }
    if (!StrIterRead(&si, &c) || c != 'b') {
        return false;
    }
    // 'c'/'d' are past the capped length even though they are in memory.
    return !StrIterRead(&si, &c);
}

// StrIterPeek reads the current character without advancing the cursor.
bool test_striter_peek_does_not_advance(void) {
    StrIter si = StrIterFromZstr("xy");
    char    c  = 0;
    if (!StrIterPeek(&si, &c) || c != 'x') {
        return false;
    }
    return StrIterIndex(&si) == 0;
}

// StrIterPeekNext looks one character ahead in iteration order without
// advancing. On a forward iter that is the byte after the cursor.
bool test_striter_peek_next_forward(void) {
    StrIter si = StrIterFromZstr("abc");
    char    c  = 0;
    if (!StrIterPeekNext(&si, &c) || c != 'b') {
        return false;
    }
    return StrIterIndex(&si) == 0;
}

// StrIterPeekPrev looks one character behind in iteration order.
bool test_striter_peek_prev_forward(void) {
    StrIter si = StrIterFromZstr("abc");
    StrIterMustNext(&si); // -> 'b'
    char c = 0;
    if (!StrIterPeekPrev(&si, &c) || c != 'a') {
        return false;
    }
    return StrIterIndex(&si) == 1;
}

// At the last character nothing lies ahead: PeekNext fails and leaves
// *out untouched.
bool test_striter_peek_next_at_last_fails(void) {
    StrIter si = StrIterFromZstr("ab");
    StrIterMustNext(&si); // -> 'b' (last)
    char c = 0x7F;
    if (StrIterPeekNext(&si, &c)) {
        return false;
    }
    return c == 0x7F;
}

// At the first character nothing lies behind: PeekPrev fails and leaves
// *out untouched.
bool test_striter_peek_prev_at_first_fails(void) {
    StrIter si = StrIterFromZstr("ab");
    char    c  = 0x7F;
    if (StrIterPeekPrev(&si, &c)) {
        return false;
    }
    return c == 0x7F;
}

// StrIterPeekAt reads at a signed iteration-direction offset without
// advancing the cursor.
bool test_striter_peek_at_offset(void) {
    StrIter si = StrIterFromZstr("abcd");
    char    c  = 0;
    if (!StrIterPeekAt(&si, 2, &c) || c != 'c') {
        return false;
    }
    return StrIterIndex(&si) == 0;
}

// StrIterDataAt is index-addressed and well-defined one past the end.
bool test_striter_data_at_bounds(void) {
    StrIter si = StrIterFromZstr("abc");
    if (*StrIterDataAt(&si, 0) != 'a') {
        return false;
    }
    if (*StrIterDataAt(&si, 2) != 'c') {
        return false;
    }
    // The one-past-end pointer equals begin + length.
    return StrIterDataAt(&si, 3) == StrIterDataAt(&si, 0) + 3;
}

int main(void) {
    WriteFmt("[INFO] Starting StrIter tests\n\n");
    TestFunction tests[] = {
        test_striter_from_zstr_length_and_read,
        test_striter_from_cstr_caps_length,
        test_striter_peek_does_not_advance,
        test_striter_peek_next_forward,
        test_striter_peek_prev_forward,
        test_striter_peek_next_at_last_fails,
        test_striter_peek_prev_at_first_fails,
        test_striter_peek_at_offset,
        test_striter_data_at_bounds,
    };
    int total = sizeof(tests) / sizeof(tests[0]);
    return run_test_suite(tests, total, NULL, 0, "StrIter");
}
