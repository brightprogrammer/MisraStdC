/// file      : std/container/float/private.h
/// author    : Generated following Misra project patterns
/// This is free and unencumbered software released into the public domain.
///
/// Internal Float helpers used to implement the public generic API.

#ifndef MISRA_STD_CONTAINER_FLOAT_PRIVATE_H
#define MISRA_STD_CONTAINER_FLOAT_PRIVATE_H

#include "Type.h"
#include <Misra/Std/Container/Int/Private.h>

///
/// Empty-Float struct literal (numeric value 0) bound to a raw allocator
/// pointer. Equivalent to the public `FloatInit(typed_alloc_ptr)` macro
/// but takes a raw `Allocator *` directly. Used by library .c code that
/// already holds an allocator pointer it wants to pass through.
///
#define float_init_alloc(alloc_ptr)                                                                                    \
    ((Float) {.negative = false, .significand = int_init_alloc(alloc_ptr), .exponent = 0})

#ifdef __cplusplus
extern "C" {
#endif

    Float float_from_u64(u64 value, Allocator *alloc);
    Float float_from_i64(i64 value, Allocator *alloc);
    Float float_from_int(Int *value, Allocator *alloc);
    Float float_from_f32(float value, Allocator *alloc);
    Float float_from_f64(double value, Allocator *alloc);
    int   float_compare_with_error(Float *lhs, Float *rhs, bool *error);
    int   float_compare(Float *lhs, Float *rhs);
    bool  float_add(Float *result, Float *a, Float *b);
    bool  float_sub(Float *result, Float *a, Float *b);
    bool  float_mul(Float *result, Float *a, Float *b);
    bool  float_div(Float *result, Float *a, Float *b, u64 precision);
    int   float_compare_int_with_error(Float *lhs, Int *rhs, bool *error);
    int   float_compare_int(Float *lhs, Int *rhs);
    int   float_compare_u64_with_error(Float *lhs, u64 rhs, bool *error);
    int   float_compare_u64(Float *lhs, u64 rhs);
    int   float_compare_i64_with_error(Float *lhs, i64 rhs, bool *error);
    int   float_compare_i64(Float *lhs, i64 rhs);
    int   float_compare_f32_with_error(Float *lhs, float rhs, bool *error);
    int   float_compare_f32(Float *lhs, float rhs);
    int   float_compare_f64_with_error(Float *lhs, double rhs, bool *error);
    int   float_compare_f64(Float *lhs, double rhs);
    bool  float_add_int(Float *result, Float *a, Int *b);
    bool  float_add_u64(Float *result, Float *a, u64 b);
    bool  float_add_i64(Float *result, Float *a, i64 b);
    bool  float_add_f32(Float *result, Float *a, float b);
    bool  float_add_f64(Float *result, Float *a, double b);
    bool  float_sub_int(Float *result, Float *a, Int *b);
    bool  float_sub_u64(Float *result, Float *a, u64 b);
    bool  float_sub_i64(Float *result, Float *a, i64 b);
    bool  float_sub_f32(Float *result, Float *a, float b);
    bool  float_sub_f64(Float *result, Float *a, double b);
    bool  float_mul_int(Float *result, Float *a, Int *b);
    bool  float_mul_u64(Float *result, Float *a, u64 b);
    bool  float_mul_i64(Float *result, Float *a, i64 b);
    bool  float_mul_f32(Float *result, Float *a, float b);
    bool  float_mul_f64(Float *result, Float *a, double b);
    bool  float_div_int(Float *result, Float *a, Int *b, u64 precision);
    bool  float_div_u64(Float *result, Float *a, u64 b, u64 precision);
    bool  float_div_i64(Float *result, Float *a, i64 b, u64 precision);
    bool  float_div_f32(Float *result, Float *a, float b, u64 precision);
    bool  float_div_f64(Float *result, Float *a, double b, u64 precision);

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_FLOAT_PRIVATE_H
