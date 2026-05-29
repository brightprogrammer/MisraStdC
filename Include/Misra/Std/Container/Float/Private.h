/// file      : std/container/float/private.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Internal Float helpers used to implement the public generic API.

#ifndef MISRA_STD_CONTAINER_FLOAT_PRIVATE_H
#define MISRA_STD_CONTAINER_FLOAT_PRIVATE_H

#include "Type.h"

#ifdef __cplusplus
extern "C" {
#endif

    ///
    /// Snake_case runtime bodies behind the public `Float*` PascalCase API. Direct callers are
    /// the `_Generic` dispatch tables in `Convert.h`, `Compare.h`, and `Math.h`, plus the
    /// `OVERLOAD` helpers in `Init.h`. End-user code should never name these directly;
    /// the public surface holds the SUCCESS/FAILURE contracts and routes by operand type.
    /// The `*_with_error` variants signal lossy comparisons out-of-band via the `error` flag.
    ///
    /// TAGS: Float, Internal, Runtime, Dispatch
    ///
    Float float_from_u64(u64 value, Allocator *alloc);
    Float float_from_i64(i64 value, Allocator *alloc);
    Float float_from_int(const Int *value, Allocator *alloc);
    Float float_from_f32(float value, Allocator *alloc);
    Float float_from_f64(double value, Allocator *alloc);
    int   float_compare_with_error(const Float *lhs, const Float *rhs, bool *error);
    i32   float_compare(const void *lhs, const void *rhs);
    bool  float_add(Float *result, const Float *a, const Float *b);
    bool  float_sub(Float *result, const Float *a, const Float *b);
    bool  float_mul(Float *result, const Float *a, const Float *b);
    bool  float_div(Float *result, const Float *a, const Float *b, u64 precision);
    int   float_compare_int_with_error(const Float *lhs, const Int *rhs, bool *error);
    int   float_compare_int(const Float *lhs, const Int *rhs);
    int   float_compare_u64_with_error(const Float *lhs, u64 rhs, bool *error);
    int   float_compare_u64(const Float *lhs, u64 rhs);
    int   float_compare_i64_with_error(const Float *lhs, i64 rhs, bool *error);
    int   float_compare_i64(const Float *lhs, i64 rhs);
    int   float_compare_f32_with_error(const Float *lhs, float rhs, bool *error);
    int   float_compare_f32(const Float *lhs, float rhs);
    int   float_compare_f64_with_error(const Float *lhs, double rhs, bool *error);
    int   float_compare_f64(const Float *lhs, double rhs);
    bool  float_add_int(Float *result, const Float *a, const Int *b);
    bool  float_add_u64(Float *result, const Float *a, u64 b);
    bool  float_add_i64(Float *result, const Float *a, i64 b);
    bool  float_add_f32(Float *result, const Float *a, float b);
    bool  float_add_f64(Float *result, const Float *a, double b);
    bool  float_sub_int(Float *result, const Float *a, const Int *b);
    bool  float_sub_u64(Float *result, const Float *a, u64 b);
    bool  float_sub_i64(Float *result, const Float *a, i64 b);
    bool  float_sub_f32(Float *result, const Float *a, float b);
    bool  float_sub_f64(Float *result, const Float *a, double b);
    bool  float_mul_int(Float *result, const Float *a, const Int *b);
    bool  float_mul_u64(Float *result, const Float *a, u64 b);
    bool  float_mul_i64(Float *result, const Float *a, i64 b);
    bool  float_mul_f32(Float *result, const Float *a, float b);
    bool  float_mul_f64(Float *result, const Float *a, double b);
    bool  float_div_int(Float *result, const Float *a, const Int *b, u64 precision);
    bool  float_div_u64(Float *result, const Float *a, u64 b, u64 precision);
    bool  float_div_i64(Float *result, const Float *a, i64 b, u64 precision);
    bool  float_div_f32(Float *result, const Float *a, float b, u64 precision);
    bool  float_div_f64(Float *result, const Float *a, double b, u64 precision);

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_FLOAT_PRIVATE_H
