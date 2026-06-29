/// file : tests/std/int.leak.c
///
/// Leak-guard tests for Int: route allocations through an explicit
/// DebugAllocator and assert DebugAllocatorLiveCount(&dbg) == 0 after full
/// cleanup, to KILL mutation survivors that remove internal *Deinit calls on
/// input-reachable branches (the leak-only proposals). Distinct contract from
/// the value-correctness tests in the sibling Int.* files -- do NOT duplicate.
///
/// Each test drives a specific public function down a SUCCESS-path branch that
/// frees one or more internal temporaries before returning. With every Int the
/// test owns released, a surviving internal allocation (a removed *Deinit) shows
/// up as a non-zero DebugAllocator live count / bytes -> the mutant is killed.

#include <Misra.h>
#include <Misra/Std/Allocator/Debug.h>
#include <Misra/Std/Container/Int.h>

#include "../Util/TestRunner.h"

#define LEAK_CLEAN(dbg) (DebugAllocatorLiveCount(&(dbg)) == 0 && DebugAllocatorLiveBytes(&(dbg)) == 0)

// Leak-only config: live-count tracking with NO per-alloc stack-trace capture,
// canary, or freed-history. Bignum ops allocate thousands of limbs; the default
// 8-deep backtrace-per-alloc makes the leak suite crawl under mull. LiveCount /
// LiveBytes are still tracked, so leak detection is unchanged.
#define LEAK_CFG                                                                                                       \
    ((DebugAllocatorConfig) {.capture_traces = false, .detect_overflow = false, .track_freed_history = false})

// ---------------------------------------------------------------------------
// Core arithmetic: int_mul / int_add / int_sub success-path replace+deinit
// ---------------------------------------------------------------------------

bool test_mul_nonzero_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    Int x = IntFrom(123456u, a);
    Int y = IntFrom(7891011u, a);
    Int r = IntInit(a);

    bool ok = IntMul(&r, &x, &y);
    ok      = ok && IntToU64(&r) == 123456ull * 7891011ull;

    IntDeinit(&x);
    IntDeinit(&y);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_mul_zero_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    Int x = IntFrom(0u, a);
    Int y = IntFrom(7891011u, a);
    Int r = IntFrom(42u, a); // r starts non-empty so the internal deinit matters

    bool ok = IntMul(&r, &x, &y);
    ok      = ok && IntIsZero(&r);

    IntDeinit(&x);
    IntDeinit(&y);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_add_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    Int x = IntFrom(0xFFFFFFFFull, a);
    Int y = IntFrom(0xFFFFFFFFull, a);
    Int r = IntFrom(7u, a);

    bool ok = IntAdd(&r, &x, &y);
    ok      = ok && IntToU64(&r) == 0xFFFFFFFFull + 0xFFFFFFFFull;

    IntDeinit(&x);
    IntDeinit(&y);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_sub_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    Int x = IntFrom(1000000u, a);
    Int y = IntFrom(999983u, a);
    Int r = IntFrom(7u, a);

    bool ok = IntSub(&r, &x, &y);
    ok      = ok && IntToU64(&r) == 17u;

    IntDeinit(&x);
    IntDeinit(&y);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_add_u64_in_place_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    // IntAdd with a u64 operand drives int_add_u64 -> int_add_u64_in_place,
    // which frees lhs/rhs temporaries on success (lines 276/277).
    Int x = IntFrom(500u, a);
    Int r = IntFrom(7u, a);

    bool ok = IntAdd(&r, &x, 250u);
    ok      = ok && IntToU64(&r) == 750u;

    IntDeinit(&x);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_mul_u64_in_place_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    // IntMul with a u64 operand drives int_mul_u64 -> int_mul_u64_in_place,
    // which frees lhs/rhs temporaries on success (lines 251/252).
    Int x = IntFrom(500u, a);
    Int r = IntFrom(7u, a);

    bool ok = IntMul(&r, &x, 13u);
    ok      = ok && IntToU64(&r) == 6500u;

    IntDeinit(&x);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_sub_u64_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    Int x = IntFrom(1000u, a);
    Int r = IntFrom(7u, a);

    bool ok = IntSub(&r, &x, 250u); // int_sub_u64 frees rhs on success (1187)
    ok      = ok && IntToU64(&r) == 750u;

    IntDeinit(&x);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// ---------------------------------------------------------------------------
// Division family: int_div_mod success path + scalar wrappers
// ---------------------------------------------------------------------------

bool test_div_mod_ge_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    // dividend >= divisor drives the long-division branch (lines 1378..1411,
    // 1420..1423 replace/deinit).
    Int dvd = IntFrom(1000003u, a);
    Int dvs = IntFrom(101u, a);
    Int q   = IntFrom(7u, a);
    Int r   = IntFrom(9u, a);

    bool ok = IntDivMod(&q, &r, &dvd, &dvs);
    ok      = ok && IntToU64(&q) == 1000003u / 101u && IntToU64(&r) == 1000003u % 101u;

    IntDeinit(&dvd);
    IntDeinit(&dvs);
    IntDeinit(&q);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_div_mod_lt_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    // dividend < divisor drives the else branch (line 1413: deinit+reinit q).
    Int dvd = IntFrom(50u, a);
    Int dvs = IntFrom(101u, a);
    Int q   = IntFrom(7u, a);
    Int r   = IntFrom(9u, a);

    bool ok = IntDivMod(&q, &r, &dvd, &dvs);
    ok      = ok && IntIsZero(&q) && IntToU64(&r) == 50u;

    IntDeinit(&dvd);
    IntDeinit(&dvs);
    IntDeinit(&q);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_div_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    Int dvd = IntFrom(1000003u, a);
    Int dvs = IntFrom(101u, a);
    Int r   = IntFrom(7u, a);

    bool ok = IntDiv(&r, &dvd, &dvs); // int_div frees remainder on success (1445)
    ok      = ok && IntToU64(&r) == 1000003u / 101u;

    IntDeinit(&dvd);
    IntDeinit(&dvs);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_div_exact_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    Int dvd = IntFrom(1000u, a);
    Int dvs = IntFrom(8u, a);
    Int r   = IntFrom(7u, a);

    bool ok = IntDivExact(&r, &dvd, &dvs); // frees remainder on success (1474)
    ok      = ok && IntToU64(&r) == 125u;

    IntDeinit(&dvd);
    IntDeinit(&dvs);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_div_scalar_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    // int_div_u64 / int_div_exact_u64 / int_div_i64 etc. free divisor_value
    // on success (1488, 1514, 1501, 1527).
    Int dvd = IntFrom(1000u, a);
    Int r1  = IntFrom(7u, a);
    Int r2  = IntFrom(7u, a);
    Int r3  = IntFrom(7u, a);

    bool ok = IntDiv(&r1, &dvd, 8u);
    ok      = ok && IntDivExact(&r2, &dvd, 8u);
    ok      = ok && IntDiv(&r3, &dvd, (i64)8);
    ok      = ok && IntToU64(&r1) == 125u && IntToU64(&r2) == 125u && IntToU64(&r3) == 125u;

    IntDeinit(&dvd);
    IntDeinit(&r1);
    IntDeinit(&r2);
    IntDeinit(&r3);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_div_mod_scalar_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    // int_div_mod_u64 / int_div_mod_i64 free divisor_value on success
    // (1540, 1553); int_mod_u64_into / int_mod_i64_into free quotient (1607/1615).
    Int dvd = IntFrom(1003u, a);
    Int q   = IntFrom(7u, a);
    Int r   = IntFrom(7u, a);
    Int m1  = IntFrom(7u, a);
    Int m2  = IntFrom(7u, a);

    bool ok = IntDivMod(&q, &r, &dvd, 100u);
    ok      = ok && IntMod(&m1, &dvd, 100u);
    ok      = ok && IntMod(&m2, &dvd, (i64)100);
    ok      = ok && IntToU64(&q) == 10u && IntToU64(&r) == 3u;
    ok      = ok && IntToU64(&m1) == 3u && IntToU64(&m2) == 3u;

    IntDeinit(&dvd);
    IntDeinit(&q);
    IntDeinit(&r);
    IntDeinit(&m1);
    IntDeinit(&m2);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_mod_int_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    Int dvd = IntFrom(1003u, a);
    Int dvs = IntFrom(100u, a);
    Int r   = IntFrom(7u, a);

    bool ok = IntMod(&r, &dvd, &dvs); // int_mod frees quotient on success (1598)
    ok      = ok && IntToU64(&r) == 3u;

    IntDeinit(&dvd);
    IntDeinit(&dvs);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// ---------------------------------------------------------------------------
// to_str: int_div_u64_rem + int_try_to_str_radix loop deinits
// ---------------------------------------------------------------------------

bool test_to_str_radix_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    // Drives int_try_to_str_radix loop (760, 772) and int_div_u64_rem
    // (1583/1584) and int_mod_u64 (1630).
    Int  v   = IntFrom(1234567u, a);
    Str  s   = IntToStrRadix(&v, 10, false, a);
    bool ok  = StrLen(&s) == 7;
    u64  rem = 0;
    {
        Int q = IntInit(a);
        rem   = int_div_u64_rem(&q, &v, 1000u);
        IntDeinit(&q);
    }
    ok = ok && rem == 567u;
    ok = ok && int_mod_u64(&v, 100u) == 67u;

    StrDeinit(&s);
    IntDeinit(&v);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// ---------------------------------------------------------------------------
// pow: int_pow_u64 squaring loop deinits
// ---------------------------------------------------------------------------

bool test_pow_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    // exponent 13 = 0b1101 hits both the multiply-acc branch (1317) and the
    // square-current branch (1331), and final current/acc cleanup (1336).
    Int base = IntFrom(3u, a);
    Int r    = IntFrom(7u, a);

    bool ok = IntPow(&r, &base, 13u);
    ok      = ok && IntToU64(&r) == 1594323u; // 3^13

    IntDeinit(&base);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// ---------------------------------------------------------------------------
// GCD / LCM
// ---------------------------------------------------------------------------

bool test_gcd_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    // Euclid loop frees x each iteration (1657), final y cleanup (1663).
    Int x = IntFrom(123456u, a);
    Int y = IntFrom(7890u, a);
    Int r = IntFrom(7u, a);

    bool ok = IntGCD(&r, &x, &y);
    ok      = ok && IntToU64(&r) == 6u;

    IntDeinit(&x);
    IntDeinit(&y);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_lcm_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    // Success path frees gcd + quotient (1689/1690).
    Int x = IntFrom(21u, a);
    Int y = IntFrom(6u, a);
    Int r = IntFrom(7u, a);

    bool ok = IntLCM(&r, &x, &y);
    ok      = ok && IntToU64(&r) == 42u;

    IntDeinit(&x);
    IntDeinit(&y);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// ---------------------------------------------------------------------------
// Roots: IntRootRem binary-search loop, IntRoot, perfect-square/power
// ---------------------------------------------------------------------------

bool test_root_rem_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    // Non-trivial cube root drives the bisection loop: low/high/mid/mid_pow
    // deinits (1813,1819,1832,1837,1838), the final block (1855..1858).
    Int v    = IntFrom(1000000u, a);
    Int root = IntFrom(7u, a);
    Int rem  = IntFrom(9u, a);

    bool ok = IntRootRem(&root, &rem, &v, 3); // 100^3 = 1e6
    ok      = ok && IntToU64(&root) == 100u && IntIsZero(&rem);

    IntDeinit(&v);
    IntDeinit(&root);
    IntDeinit(&rem);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_root_rem_inexact_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    // Inexact root exercises the cmp>0 branch (1819/1822/1832 high update).
    Int v    = IntFrom(1000u, a);
    Int root = IntFrom(7u, a);
    Int rem  = IntFrom(9u, a);

    bool ok = IntRootRem(&root, &rem, &v, 3); // cbrt(1000)=10 exact actually
    ok      = ok && IntToU64(&root) == 10u && IntIsZero(&rem);

    // and a genuinely inexact one
    Int v2 = IntFrom(999u, a);
    ok     = ok && IntRootRem(&root, &rem, &v2, 3);
    ok     = ok && IntToU64(&root) == 9u && IntToU64(&rem) == 999u - 729u;

    IntDeinit(&v);
    IntDeinit(&v2);
    IntDeinit(&root);
    IntDeinit(&rem);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_root_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    Int v = IntFrom(1000000u, a);
    Int r = IntFrom(7u, a);

    bool ok = IntRoot(&r, &v, 3); // frees remainder on success (1877)
    ok      = ok && IntToU64(&r) == 100u;

    IntDeinit(&v);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_is_perfect_square_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    // Frees root + remainder on success (1904/1905).
    Int sq  = IntFrom(123201u, a); // 351^2
    Int nsq = IntFrom(123202u, a);

    bool ok = IntIsPerfectSquare(&sq) && !IntIsPerfectSquare(&nsq);

    IntDeinit(&sq);
    IntDeinit(&nsq);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_is_perfect_power_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    // Loop frees root + remainder each degree (1934/1935).
    Int pw  = IntFrom(7776u, a); // 6^5
    Int npw = IntFrom(7777u, a);

    bool ok = IntIsPerfectPower(&pw) && !IntIsPerfectPower(&npw);

    IntDeinit(&pw);
    IntDeinit(&npw);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// ---------------------------------------------------------------------------
// Jacobi
// ---------------------------------------------------------------------------

bool test_jacobi_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    // Drives the main loop (swap, even-stripping 1972, inner deinits) and both
    // tail branches: result-1 path (1998) and nn!=1 -> 0 path requires gcd!=1.
    Int x  = IntFrom(1001u, a);
    Int n  = IntFrom(9907u, a); // prime
    int jr = 0;

    bool ok = IntTryJacobi(&jr, &x, &n);

    // a case where gcd(a,n) != 1 -> the *out=0 branch (frees nn at 1998)
    Int x2  = IntFrom(15u, a);
    Int n2  = IntFrom(9u, a); // odd, gcd(15,9)=3
    int jr2 = 0;
    ok      = ok && IntTryJacobi(&jr2, &x2, &n2) && jr2 == 0;

    IntDeinit(&x);
    IntDeinit(&n);
    IntDeinit(&x2);
    IntDeinit(&n2);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// ---------------------------------------------------------------------------
// Modular arithmetic: add/sub/mul/div/square/pow_mod
// ---------------------------------------------------------------------------

bool test_mod_add_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    Int x = IntFrom(12345u, a);
    Int y = IntFrom(67890u, a);
    Int m = IntFrom(1009u, a);
    Int r = IntFrom(7u, a);

    bool ok = IntModAdd(&r, &x, &y, &m); // frees ar/br/sum on success (2042-2044)
    ok      = ok && IntToU64(&r) == (12345u % 1009u + 67890u % 1009u) % 1009u;

    IntDeinit(&x);
    IntDeinit(&y);
    IntDeinit(&m);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_mod_sub_ge_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    // ar >= br branch (2098/2099 frees ar/br).
    Int x = IntFrom(900u, a);
    Int y = IntFrom(100u, a);
    Int m = IntFrom(1009u, a);
    Int r = IntFrom(7u, a);

    bool ok = IntModSub(&r, &x, &y, &m);
    ok      = ok && IntToU64(&r) == 800u;

    IntDeinit(&x);
    IntDeinit(&y);
    IntDeinit(&m);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_mod_sub_lt_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    // ar < br branch: nonzero diff -> modulus-diff (2095 frees diff).
    Int x = IntFrom(100u, a);
    Int y = IntFrom(900u, a);
    Int m = IntFrom(1009u, a);
    Int r = IntFrom(7u, a);

    bool ok = IntModSub(&r, &x, &y, &m);
    ok      = ok && IntToU64(&r) == 1009u - 800u;

    IntDeinit(&x);
    IntDeinit(&y);
    IntDeinit(&m);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_mod_sub_zero_diff_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    // ar < br but diff is a multiple of modulus -> diff==0 inner branch
    // (still frees diff at 2095). x=0,y=m gives ar=0,br=0 actually; use
    // x=0, y=1009*2 reduced -> br=0; to make br>ar with diff==0 mod m we need
    // ar<br after reduction with (br-ar)%m==0 -> impossible unless equal.
    // Instead exercise the ar<br with diff==0 by x=0 mod m, y=0 mod m won't.
    // Use the generic ar<br path already covered; here cover equal-after-mod.
    Int x = IntFrom(5u, a);
    Int y = IntFrom(5u, a);
    Int m = IntFrom(1009u, a);
    Int r = IntFrom(7u, a);

    bool ok = IntModSub(&r, &x, &y, &m);
    ok      = ok && IntIsZero(&r);

    IntDeinit(&x);
    IntDeinit(&y);
    IntDeinit(&m);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_mod_mul_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    Int x = IntFrom(12345u, a);
    Int y = IntFrom(67890u, a);
    Int m = IntFrom(1009u, a);
    Int r = IntFrom(7u, a);

    bool ok = IntModMul(&r, &x, &y, &m); // frees ar/br/prod (2126-2128)
    ok      = ok && IntToU64(&r) == (12345ull * 67890ull) % 1009ull;

    IntDeinit(&x);
    IntDeinit(&y);
    IntDeinit(&m);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_square_mod_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    Int x = IntFrom(12345u, a);
    Int m = IntFrom(1009u, a);
    Int r = IntFrom(7u, a);

    bool ok = IntSquareMod(&r, &x, &m);
    ok      = ok && IntToU64(&r) == (12345ull * 12345ull) % 1009ull;

    IntDeinit(&x);
    IntDeinit(&m);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_mod_div_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    // IntModDiv frees inverse on success (2160).
    Int x = IntFrom(42u, a);
    Int y = IntFrom(5u, a);
    Int m = IntFrom(1009u, a);
    Int r = IntFrom(7u, a);

    bool ok = IntModDiv(&r, &x, &y, &m);

    IntDeinit(&x);
    IntDeinit(&y);
    IntDeinit(&m);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_pow_u64_mod_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    // int_pow_u64_mod: multiply-acc (2201) and square (2215), final cleanup
    // (2220 frees base_mod).
    Int base = IntFrom(7u, a);
    Int m    = IntFrom(1000000007u, a);
    Int r    = IntFrom(9u, a);

    bool ok = IntPowMod(&r, &base, 13u, &m);
    ok      = ok && IntToU64(&r) == 96889010407ull % 1000000007ull; // 7^13 mod m

    IntDeinit(&base);
    IntDeinit(&m);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_pow_mod_integer_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    // int_pow_mod (Int exponent) main loop + final base_mod/exp cleanup.
    Int base = IntFrom(7u, a);
    Int exp  = IntFrom(13u, a);
    Int m    = IntFrom(1000000007u, a);
    Int r    = IntFrom(9u, a);

    bool ok = IntPowMod(&r, &base, &exp, &m);
    ok      = ok && IntToU64(&r) == 96889010407ull % 1000000007ull;

    IntDeinit(&base);
    IntDeinit(&exp);
    IntDeinit(&m);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// ---------------------------------------------------------------------------
// IntModInv: extended-Euclid loop frees q/rem/q_new_t each iteration, t/r
// reassignment chain, and final positive-result branch.
// ---------------------------------------------------------------------------

bool test_mod_inv_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    Int v = IntFrom(17u, a);
    Int m = IntFrom(3120u, a); // gcd(17,3120)=1
    Int r = IntFrom(7u, a);

    bool ok = IntModInv(&r, &v, &m);
    ok      = ok && (IntToU64(&r) * 17u) % 3120u == 1u;

    IntDeinit(&v);
    IntDeinit(&m);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_mod_inv_negative_t_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    // A case where the Bezout coefficient t is negative, driving the
    // t.negative && !zero branch (modulus - mag_mod, frees positive/mag_mod
    // and mag_mod at 2414).
    Int v = IntFrom(3u, a);
    Int m = IntFrom(7u, a); // inverse of 3 mod 7 is 5
    Int r = IntFrom(9u, a);

    bool ok = IntModInv(&r, &v, &m);
    ok      = ok && IntToU64(&r) == 5u;

    IntDeinit(&v);
    IntDeinit(&m);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_mod_inv_no_solution_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    // gcd != 1 -> r != one, the if(IntEQ(&r,&one)) block is skipped and ok stays
    // false; the function still frees reduced/r/new_r/one/t/new_t at the tail
    // (2419-2424). All those carry allocations.
    Int v = IntFrom(6u, a);
    Int m = IntFrom(9u, a);           // gcd(6,9)=3, no inverse
    Int r = IntFrom(7u, a);

    bool ok = !IntModInv(&r, &v, &m); // returns false

    IntDeinit(&v);
    IntDeinit(&m);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// ---------------------------------------------------------------------------
// IntModSqrt: the p%4==3 fast path, the Tonelli-Shanks loop, zero residue,
// modulus==2, no-solution (jacobi != 1).
// ---------------------------------------------------------------------------

bool test_mod_sqrt_p3mod4_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    // modulus 7 (7 % 4 == 3) drives the fast path: frees exponent + a (2488/2489).
    Int v = IntFrom(2u, a); // 2 is QR mod 7 (3^2=2)
    Int m = IntFrom(7u, a);
    Int r = IntFrom(9u, a);

    bool ok = IntModSqrt(&r, &v, &m);
    u64  rv = IntToU64(&r);
    ok      = ok && (rv * rv) % 7u == 2u;

    IntDeinit(&v);
    IntDeinit(&m);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_mod_sqrt_tonelli_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    // modulus 17 (17 % 4 == 1) drives the Tonelli-Shanks block: q/z/c/t/r,
    // the witness-search loop, the t-loop with t_power/b/b_sq, and the success
    // int_replace path (2724-2735). 2 is a QR mod 17 (6^2=36=2).
    Int v = IntFrom(2u, a);
    Int m = IntFrom(17u, a);
    Int r = IntFrom(9u, a);

    bool ok = IntModSqrt(&r, &v, &m);
    u64  rv = IntToU64(&r);
    ok      = ok && (rv * rv) % 17u == 2u;

    IntDeinit(&v);
    IntDeinit(&m);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_mod_sqrt_tonelli_larger_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    // modulus 41 (41 % 4 == 1, 41-1 = 40 = 8*5 so m=3 levels) drives the inner
    // b-squaring loop (2662) and the c/t/b reassignments more deeply.
    Int v = IntFrom(10u, a); // 10 is QR mod 41 (16^2=256=256-246=10)
    Int m = IntFrom(41u, a);
    Int r = IntFrom(9u, a);

    bool ok = IntModSqrt(&r, &v, &m);
    u64  rv = IntToU64(&r);
    ok      = ok && (rv * rv) % 41u == 10u;

    IntDeinit(&v);
    IntDeinit(&m);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_mod_sqrt_zero_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    // value % modulus == 0 -> zero branch frees a (2449).
    Int v = IntFrom(0u, a);
    Int m = IntFrom(17u, a);
    Int r = IntFrom(9u, a);

    bool ok = IntModSqrt(&r, &v, &m) && IntIsZero(&r);

    IntDeinit(&v);
    IntDeinit(&m);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_mod_sqrt_mod2_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    // modulus 2 path: int_replace(result, &a) (no extra deinit, but reaches
    // the compare-u64==2 branch); a is moved into result.
    Int v = IntFrom(1u, a);
    Int m = IntFrom(2u, a);
    Int r = IntFrom(9u, a);

    bool ok = IntModSqrt(&r, &v, &m) && IntToU64(&r) == 1u;

    IntDeinit(&v);
    IntDeinit(&m);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_mod_sqrt_no_solution_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    // jacobi != 1 -> non-residue, frees a and returns false (2473's deinit at
    // the early non-residue return). 3 is a non-residue mod 7.
    Int v = IntFrom(3u, a);
    Int m = IntFrom(7u, a);
    Int r = IntFrom(9u, a);

    bool ok = !IntModSqrt(&r, &v, &m);

    IntDeinit(&v);
    IntDeinit(&m);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_mod_sqrt_composite_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    // composite (non-prime) odd modulus -> !prime branch frees a (2465).
    Int v = IntFrom(2u, a);
    Int m = IntFrom(9u, a); // odd composite
    Int r = IntFrom(9u, a);

    bool ok = !IntModSqrt(&r, &v, &m);

    IntDeinit(&v);
    IntDeinit(&m);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_mod_sqrt_even_modulus_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    // even modulus (>2) -> IntIsEven branch frees a (2465).
    Int v = IntFrom(3u, a);
    Int m = IntFrom(8u, a);
    Int r = IntFrom(9u, a);

    bool ok = !IntModSqrt(&r, &v, &m);

    IntDeinit(&v);
    IntDeinit(&m);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// ---------------------------------------------------------------------------
// Primality: IntIsProbablePrime Miller-Rabin loop + IntNextPrime
// ---------------------------------------------------------------------------

bool test_is_probable_prime_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    // Large-ish prime drives the full Miller-Rabin loop: base/x deinits in the
    // base>=value continue (2807/2808), the x==1||n-1 continue (2820/2821), the
    // witness inner loop (2839), per-iteration base/x (2853/2854), and the d /
    // n_minus_one tail (2860/2861).
    Int prime     = IntFrom(1000003u, a); // prime
    Int composite = IntFrom(1000005u, a); // composite

    bool ok = IntIsProbablePrime(&prime) && !IntIsProbablePrime(&composite);

    IntDeinit(&prime);
    IntDeinit(&composite);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_is_probable_prime_witness_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    // A strong-pseudoprime-ish composite that forces the witness inner loop
    // (squaring x repeatedly) before failing. 2047 = 23*89 passes base 2 (it's
    // a 2-SPRP) but fails on another base -> exercises x squaring (2839).
    Int n = IntFrom(2047u, a);

    bool ok = !IntIsProbablePrime(&n);

    IntDeinit(&n);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_next_prime_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    // Drives candidate-stepping loop (frees candidate on success at 2924, and
    // the +2 stepping at 2918). Starting from an even-ish composite.
    Int v = IntFrom(1000000u, a);
    Int r = IntFrom(7u, a);

    bool ok = IntNextPrime(&r, &v);
    ok      = ok && IntToU64(&r) == 1000003u;

    IntDeinit(&v);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_next_prime_small_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    // value <= 1 -> two path (2879 int_replace) and the candidate<=2 path
    // (2902/2903 frees candidate then replaces with two).
    Int v0 = IntFrom(0u, a);
    Int r0 = IntFrom(7u, a);
    Int v1 = IntFrom(2u, a);
    Int r1 = IntFrom(7u, a);

    bool ok = IntNextPrime(&r0, &v0) && IntToU64(&r0) == 2u;
    ok      = ok && IntNextPrime(&r1, &v1) && IntToU64(&r1) == 3u;

    IntDeinit(&v0);
    IntDeinit(&r0);
    IntDeinit(&v1);
    IntDeinit(&r1);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// ---------------------------------------------------------------------------
// from_bytes_be: int_add_u64_in_place via the shift+add loop
// ---------------------------------------------------------------------------

bool test_from_bytes_be_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);

    u8  bytes[] = {0x01, 0x02, 0x03, 0x04};
    Int v       = IntFromBytesBE(bytes, sizeof(bytes), &dbg);

    bool ok = IntToU64(&v) == 0x01020304u;

    IntDeinit(&v);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// ---------------------------------------------------------------------------
// Follow-up kills: success-path internal *Deinit on branches the pilot's tests
// did not reach. Each drives a public function with a *pre-populated* output
// Int (so the internal IntDeinit on the result handle frees a live buffer) or
// a deeper algorithmic path (Tonelli-Shanks inner squaring loop).
// ---------------------------------------------------------------------------

// int_try_from_str_radix_impl line 360: IntDeinit(out) before *out = result on
// the success path. The pilot only ever parses into a fresh Int; a non-empty
// out leaks its old buffer if the deinit is removed.
bool test_from_str_nonempty_out_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    Int out = IntFrom(987654321u, a); // pre-populated: holds a live buffer

    bool ok = IntTryFromStr(&out, "123456789");
    ok      = ok && IntToU64(&out) == 123456789u;

    IntDeinit(&out);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// int_mul line 1245: IntDeinit(result) before *result = acc on the non-zero
// product path. test_mul_nonzero uses a fresh (empty) result; a pre-populated
// result leaks its old buffer if the deinit is removed.
bool test_mul_nonzero_nonempty_result_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    Int x = IntFrom(123456u, a);
    Int y = IntFrom(7891011u, a);
    Int r = IntFrom(0xDEADBEEFu, a); // pre-populated: holds a live buffer

    bool ok = IntMul(&r, &x, &y);
    ok      = ok && IntToU64(&r) == 123456ull * 7891011ull;

    IntDeinit(&x);
    IntDeinit(&y);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// int_div_exact_i64 line 1527: IntDeinit(&divisor_value) on the success path.
// The pilot's test_div_scalar exercises int_div_i64 (the inexact i64 wrapper)
// but never int_div_exact_i64, so this divisor_value free went untested.
bool test_div_exact_i64_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    Int dvd = IntFrom(1000u, a);
    Int r   = IntFrom(7u, a);

    bool ok = IntDivExact(&r, &dvd, (i64)8);
    ok      = ok && IntToU64(&r) == 125u;

    IntDeinit(&dvd);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// IntModSqrt Tonelli-Shanks line 2662: IntDeinit(&b) before b = square in the
// inner b-squaring loop (for j+i+1 < m). Reached only when the level count m is
// large enough that the loop body runs (m - i - 1 >= 1). Drive several QRs
// across moduli with deep 2-adic valuation of (p-1) so the inner loop executes.
bool test_mod_sqrt_tonelli_inner_loop_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    // moduli with p % 4 == 1 (forces the Tonelli-Shanks block) and a high power
    // of two dividing p-1: 17 (16=2^4), 97 (96=2^5*3), 193 (192=2^6*3),
    // 257 (256=2^8). Sample a spread of residues across [1, p-1] per modulus --
    // enough to drive the inner squaring loop and every cleanup branch (the leak
    // oracle), not an exhaustive correctness sweep.
    u64       moduli[] = {17u, 97u, 193u, 257u};
    const u64 samples  = 12u;
    bool      ok       = true;

    for (u64 mi = 0; mi < sizeof(moduli) / sizeof(moduli[0]) && ok; mi++) {
        u64 p = moduli[mi];
        for (u64 si = 0; si < samples && ok; si++) {
            u64 vv = 1u + (si * (p - 2u)) / (samples - 1u);
            Int v  = IntFrom(vv, a);
            Int m  = IntFrom(p, a);
            Int r  = IntFrom(9u, a);

            bool found = IntModSqrt(&r, &v, &m);
            if (found) {
                u64 rv = IntToU64(&r);
                ok     = ok && (rv * rv) % p == vv;
            }

            IntDeinit(&v);
            IntDeinit(&m);
            IntDeinit(&r);
        }
    }

    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

int main(void) {
    TestFunction tests[] = {
        test_mul_nonzero_no_leak,
        test_mul_zero_no_leak,
        test_add_no_leak,
        test_sub_no_leak,
        test_add_u64_in_place_no_leak,
        test_mul_u64_in_place_no_leak,
        test_sub_u64_no_leak,
        test_div_mod_ge_no_leak,
        test_div_mod_lt_no_leak,
        test_div_no_leak,
        test_div_exact_no_leak,
        test_div_scalar_no_leak,
        test_div_mod_scalar_no_leak,
        test_mod_int_no_leak,
        test_to_str_radix_no_leak,
        test_pow_no_leak,
        test_gcd_no_leak,
        test_lcm_no_leak,
        test_root_rem_no_leak,
        test_root_rem_inexact_no_leak,
        test_root_no_leak,
        test_is_perfect_square_no_leak,
        test_is_perfect_power_no_leak,
        test_jacobi_no_leak,
        test_mod_add_no_leak,
        test_mod_sub_ge_no_leak,
        test_mod_sub_lt_no_leak,
        test_mod_sub_zero_diff_no_leak,
        test_mod_mul_no_leak,
        test_square_mod_no_leak,
        test_mod_div_no_leak,
        test_pow_u64_mod_no_leak,
        test_pow_mod_integer_no_leak,
        test_mod_inv_no_leak,
        test_mod_inv_negative_t_no_leak,
        test_mod_inv_no_solution_no_leak,
        test_mod_sqrt_p3mod4_no_leak,
        test_mod_sqrt_tonelli_no_leak,
        test_mod_sqrt_tonelli_larger_no_leak,
        test_mod_sqrt_zero_no_leak,
        test_mod_sqrt_mod2_no_leak,
        test_mod_sqrt_no_solution_no_leak,
        test_mod_sqrt_composite_no_leak,
        test_mod_sqrt_even_modulus_no_leak,
        test_is_probable_prime_no_leak,
        test_is_probable_prime_witness_no_leak,
        test_next_prime_no_leak,
        test_next_prime_small_no_leak,
        test_from_bytes_be_no_leak,
        test_from_str_nonempty_out_no_leak,
        test_mul_nonzero_nonempty_result_no_leak,
        test_div_exact_i64_no_leak,
        test_mod_sqrt_tonelli_inner_loop_no_leak,
    };
    TestFunction deadend_tests[] = {
        0,
    };
    (void)deadend_tests;
    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), deadend_tests, 0, "Int.Leak");
}
