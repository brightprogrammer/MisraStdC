/// file      : std/container/int/private.h
/// author    : Generated following Misra project patterns
/// This is free and unencumbered software released into the public domain.
///
/// Internal Int helpers used to implement the public generic API.

#ifndef MISRA_STD_CONTAINER_INT_PRIVATE_H
#define MISRA_STD_CONTAINER_INT_PRIVATE_H

#include "Type.h"

#ifdef __cplusplus
extern "C" {
#endif

    Int  int_from_u64(u64 value, Allocator *alloc);
    Int  int_from_i64(i64 value, Allocator *alloc);
    int  int_compare(Int *lhs, Int *rhs);
    bool int_add(Int *result, Int *a, Int *b);
    bool int_sub(Int *result, Int *a, Int *b);
    bool int_mul(Int *result, Int *a, Int *b);
    bool int_pow(Int *result, Int *base, Int *exponent);
    bool int_div(Int *result, Int *dividend, Int *divisor);
    bool int_div_exact(Int *result, Int *dividend, Int *divisor);
    bool int_mod(Int *result, Int *dividend, Int *divisor);
    bool int_div_mod(Int *quotient, Int *remainder, Int *dividend, Int *divisor);
    bool int_pow_mod(Int *result, Int *base, Int *exponent, Int *modulus);
    int  int_compare_u64(Int *lhs, u64 rhs);
    int  int_compare_i64(Int *lhs, i64 rhs);
    bool int_add_u64(Int *result, Int *value, u64 addend);
    bool int_add_i64(Int *result, Int *value, i64 addend);
    bool int_sub_u64(Int *result, Int *value, u64 subtrahend);
    bool int_sub_i64(Int *result, Int *value, i64 subtrahend);
    bool int_mul_u64(Int *result, Int *value, u64 factor);
    bool int_mul_i64(Int *result, Int *value, i64 factor);
    bool int_pow_u64(Int *result, Int *base, u64 exponent);
    bool int_pow_i64(Int *result, Int *base, i64 exponent);
    bool int_pow_u64_mod(Int *result, Int *base, u64 exponent, Int *modulus);
    bool int_pow_i64_mod(Int *result, Int *base, i64 exponent, Int *modulus);
    bool int_div_u64(Int *result, Int *dividend, u64 divisor);
    bool int_div_i64(Int *result, Int *dividend, i64 divisor);
    bool int_div_exact_u64(Int *result, Int *dividend, u64 divisor);
    bool int_div_exact_i64(Int *result, Int *dividend, i64 divisor);
    bool int_mod_u64_into(Int *result, Int *dividend, u64 divisor);
    bool int_mod_i64_into(Int *result, Int *dividend, i64 divisor);
    bool int_div_mod_u64(Int *quotient, Int *remainder, Int *dividend, u64 divisor);
    bool int_div_mod_i64(Int *quotient, Int *remainder, Int *dividend, i64 divisor);
    u64  int_div_u64_rem(Int *quotient, Int *dividend, u64 divisor);
    u64  int_mod_u64(Int *value, u64 modulus);

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_INT_PRIVATE_H
