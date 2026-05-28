/// file      : std/container/int/private.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Internal Int helpers used to implement the public generic API.

#ifndef MISRA_STD_CONTAINER_INT_PRIVATE_H
#define MISRA_STD_CONTAINER_INT_PRIVATE_H

#include "Type.h"

#ifdef __cplusplus
extern "C" {
#endif

    ///
    /// Snake_case runtime bodies behind the public `Int*` PascalCase API. Direct callers are
    /// the `_Generic` dispatch tables in `Convert.h`, `Compare.h`, and `Math.h`, plus the
    /// `OVERLOAD` helpers in `Init.h`. End-user code should never name these directly;
    /// the public surface holds the SUCCESS/FAILURE contracts and routes by argument type.
    /// `int_compare` is also exposed under its own header for `Map`/`Vec` callback use.
    ///
    /// TAGS: Int, Internal, Runtime, Dispatch
    ///
    Int  int_from_u64(u64 value, Allocator *alloc);
    Int  int_from_i64(i64 value, Allocator *alloc);
    int  int_compare(const Int *lhs, const Int *rhs);
    bool int_add(Int *result, const Int *a, const Int *b);
    bool int_sub(Int *result, const Int *a, const Int *b);
    bool int_mul(Int *result, const Int *a, const Int *b);
    bool int_pow(Int *result, const Int *base, const Int *exponent);
    bool int_div(Int *result, const Int *dividend, const Int *divisor);
    bool int_div_exact(Int *result, const Int *dividend, const Int *divisor);
    bool int_mod(Int *result, const Int *dividend, const Int *divisor);
    bool int_div_mod(Int *quotient, Int *remainder, const Int *dividend, const Int *divisor);
    bool int_pow_mod(Int *result, const Int *base, const Int *exponent, const Int *modulus);
    int  int_compare_u64(const Int *lhs, u64 rhs);
    int  int_compare_i64(const Int *lhs, i64 rhs);
    bool int_add_u64(Int *result, const Int *value, u64 addend);
    bool int_add_i64(Int *result, const Int *value, i64 addend);
    bool int_sub_u64(Int *result, const Int *value, u64 subtrahend);
    bool int_sub_i64(Int *result, const Int *value, i64 subtrahend);
    bool int_mul_u64(Int *result, const Int *value, u64 factor);
    bool int_mul_i64(Int *result, const Int *value, i64 factor);
    bool int_pow_u64(Int *result, const Int *base, u64 exponent);
    bool int_pow_i64(Int *result, const Int *base, i64 exponent);
    bool int_pow_u64_mod(Int *result, const Int *base, u64 exponent, const Int *modulus);
    bool int_pow_i64_mod(Int *result, const Int *base, i64 exponent, const Int *modulus);
    bool int_div_u64(Int *result, const Int *dividend, u64 divisor);
    bool int_div_i64(Int *result, const Int *dividend, i64 divisor);
    bool int_div_exact_u64(Int *result, const Int *dividend, u64 divisor);
    bool int_div_exact_i64(Int *result, const Int *dividend, i64 divisor);
    bool int_mod_u64_into(Int *result, const Int *dividend, u64 divisor);
    bool int_mod_i64_into(Int *result, const Int *dividend, i64 divisor);
    bool int_div_mod_u64(Int *quotient, Int *remainder, const Int *dividend, u64 divisor);
    bool int_div_mod_i64(Int *quotient, Int *remainder, const Int *dividend, i64 divisor);
    u64  int_div_u64_rem(Int *quotient, const Int *dividend, u64 divisor);
    u64  int_mod_u64(const Int *value, u64 modulus);

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_INT_PRIVATE_H
