#include <Misra/Generics/TypeMatch.h>
#include <Misra/Std/Io.h>
#include <Misra/Types.h>

#include "../Util/TestRunner.h"

typedef struct {
    float x, y;
} Position2D;

// ---------------------------------------------------------------------------
// Match / When(T, bind) / Otherwise -- dispatch on a value's static type
// ---------------------------------------------------------------------------

bool test_match_dispatch(void) {
    int        i = 7;
    double     d = 2.5;
    Position2D p = {1.0f, 2.0f};

    int tag = -1;
    Match(i) {
        When(int, v) tag    = 0;
        When(double, v) tag = 1;
        Otherwise tag       = 9;
    }
    bool ok = tag == 0;

    Match(d) {
        When(int, v) tag    = 0;
        When(double, v) tag = 1;
        Otherwise tag       = 9;
    }
    ok = ok && tag == 1;

    // Unlisted type falls to Otherwise.
    Match(p) {
        When(int, v) tag    = 0;
        When(double, v) tag = 1;
        Otherwise tag       = 9;
    }
    ok = ok && tag == 9;
    return ok;
}

bool test_match_binds_value(void) {
    int    i  = 21;
    double d  = 1.5;
    int    ri = 0;
    double rd = 0.0;
    Match(i) {
        When(int, n) ri    = n * 2;
        When(double, x) rd = x;
    }
    Match(d) {
        When(int, n) ri    = n;
        When(double, x) rd = x * 2.0;
    }
    return ri == 42 && rd == 3.0;
}

bool test_match_rvalue(void) {
    int  i   = 4;
    bool hit = false;
    // Scrutinee is an rvalue; copied by value, then bound to `n`.
    Match(i + 1) {
        When(int, n) hit    = (n == 5);
        When(double, x) hit = false;
    }
    return hit;
}

bool test_match_arms_are_exclusive(void) {
    int i     = 7;
    int count = 0;
    Match(i) {
        When(int, v) count++;
        When(double, v) count++;
        Otherwise count++;
    }
    return count == 1;
}

bool test_match_otherwise(void) {
    Position2D p   = {1.0f, 2.0f};
    bool       hit = false;
    Match(p) {
        When(int, v) hit    = false;
        When(double, v) hit = false;
        Otherwise hit       = true;
    }
    return hit;
}

bool test_nested_match(void) {
    int    i  = 10;
    double d  = 3.5;
    int    oi = 0;
    double id = 0.0;
    Match(i) {
        When(int, outer) {
            Match(d) {
                When(int, x) id    = -1.0;
                When(double, x) id = x; // inner bind : double
            }
            oi = outer;                 // outer bind : int
        }
        When(double, v) oi = -1;
    }
    return oi == 10 && id == 3.5;
}

bool test_is_predicate(void) {
    int    i = 0;
    double d = 0.0;
    return Is(i, int) == 1 && Is(i, double) == 0 && Is(d, double) == 1 && Is(d, int) == 0;
}

// Deadend: a non-exhaustive match (no arm matches, no Otherwise) must abort.
bool deadend_match_nonexhaustive(void) {
    Position2D p = {1.0f, 2.0f};
    Match(p) {
        When(int, v)(void) v; // never matches Position2D, and there is no Otherwise
    }
    return true;              // unreachable -- the match aborts first
}

int main(void) {
    WriteFmt("[INFO] Starting Generics.TypeMatch tests\n\n");
    TestFunction tests[] = {
        test_match_dispatch,
        test_match_binds_value,
        test_match_rvalue,
        test_match_arms_are_exclusive,
        test_match_otherwise,
        test_nested_match,
        test_is_predicate,
    };
    TestFunction deadend_tests[] = {
        deadend_match_nonexhaustive,
    };
    int total   = sizeof(tests) / sizeof(tests[0]);
    int deadend = sizeof(deadend_tests) / sizeof(deadend_tests[0]);
    return run_test_suite(tests, total, deadend_tests, deadend, "Generics.TypeMatch");
}
