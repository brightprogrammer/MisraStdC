#include <Misra/Generics.h>
#include <Misra/Std/Io.h>
#include <Misra/Types.h>

#include "../Util/TestRunner.h"

// Self-contained variants -- no global registration; two variants sharing a
// payload type do not collide.
Variant(Number, int, double);
Variant(Cell, int, char);
Variant(Tri, int, float, char);

// ---------------------------------------------------------------------------
// Variant construction + Match / When(N, T, bind) / Otherwise
// ---------------------------------------------------------------------------

bool test_variant_construct_and_match(void) {
    Number n   = Number_from_int(42);
    bool   hit = false;
    Match(n) {
        When(Number, int, v) hit    = (v == 42);
        When(Number, double, v) hit = false;
    }

    Number m  = Number_from_double(2.5);
    bool   hd = false;
    Match(m) {
        When(Number, int, v) hd    = false;
        When(Number, double, v) hd = (v == 2.5);
    }
    return hit && hd;
}

bool test_variant_binds_typed_payload(void) {
    Number a  = Number_from_int(10);
    Number b  = Number_from_double(1.5);
    int    ai = 0;
    double bd = 0.0;
    Match(a) {
        When(Number, int, n) ai    = n * 2; // n : int
        When(Number, double, r) bd = r;
    }
    Match(b) {
        When(Number, int, n) ai    = n;
        When(Number, double, r) bd = r * 2.0; // r : double
    }
    return ai == 20 && bd == 3.0;
}

// value in, value out -- no pointers
static Number twice(Number n) {
    Match(n) {
        When(Number, int, v) return Number_from_int(v * 2);
        When(Number, double, v) return Number_from_double(v * 2.0);
    }
    return n;
}

bool test_variant_by_value_flow(void) {
    Number r  = twice(Number_from_int(21));
    bool   ok = false;
    Match(r) {
        When(Number, int, v) ok    = (v == 42);
        When(Number, double, v) ok = false;
    }
    return ok;
}

bool test_variant_rvalue_and_otherwise(void) {
    bool ok = false;
    // Match directly on a function-return rvalue.
    Match(twice(Number_from_double(1.5))) {
        When(Number, int, v) ok    = false;
        When(Number, double, v) ok = (v == 3.0);
        Otherwise ok               = false;
    }
    return ok;
}

bool test_two_variants_independent(void) {
    Cell c  = Cell_from_char('Z');
    bool ok = false;
    Match(c) {
        When(Cell, int, v) ok  = false;
        When(Cell, char, v) ok = (v == 'Z');
    }
    return ok;
}

bool test_variant_three_payloads(void) {
    Tri  vals[] = {Tri_from_int(7), Tri_from_float(1.5f), Tri_from_char('A')};
    int  seen   = 0;
    bool ok     = true;
    for (int k = 0; k < 3; k++) {
        Match(vals[k]) {
            When(Tri, int, v) ok = ok && (v == 7), seen |= 1;
            When(Tri, float, v) ok = ok && (v == 1.5f), seen |= 2;
            When(Tri, char, v) ok = ok && (v == 'A'), seen |= 4;
        }
    }
    return ok && seen == 7;
}

bool test_variant_arms_are_exclusive(void) {
    Number n     = Number_from_int(7);
    int    count = 0;
    Match(n) {
        When(Number, int, v) count++;
        When(Number, double, v) count++;
        Otherwise count++;
    }
    return count == 1;
}

// Deadend: a non-exhaustive variant match aborts rather than fall through.
bool deadend_variant_nonexhaustive(void) {
    Number n = Number_from_double(9.0); // holds double, only int handled, no Otherwise
    Match(n) {
        When(Number, int, v)(void) v;
    }
    return true; // unreachable -- the match aborts first
}

int main(void) {
    WriteFmt("[INFO] Starting Generics.Variant tests\n\n");
    TestFunction tests[] = {
        test_variant_construct_and_match,
        test_variant_binds_typed_payload,
        test_variant_by_value_flow,
        test_variant_rvalue_and_otherwise,
        test_two_variants_independent,
        test_variant_three_payloads,
        test_variant_arms_are_exclusive,
    };
    TestFunction deadend_tests[] = {
        deadend_variant_nonexhaustive,
    };
    int total   = sizeof(tests) / sizeof(tests[0]);
    int deadend = sizeof(deadend_tests) / sizeof(deadend_tests[0]);
    return run_test_suite(tests, total, deadend_tests, deadend, "Generics.Variant");
}
