/// file      : std/container/float/math.h
/// author    : Generated following Misra project patterns
/// This is free and unencumbered software released into the public domain.
///
/// Arithmetic helpers for Float.

#ifndef MISRA_STD_CONTAINER_FLOAT_MATH_H
#define MISRA_STD_CONTAINER_FLOAT_MATH_H

#include "Type.h"

#ifdef __cplusplus
extern "C" {
#endif

void FloatNegate(Float *value);
void FloatAbs(Float *value);
void (FloatAdd)(Float *result, Float *a, Float *b);
void FloatAddInt(Float *result, Float *a, Int *b);
void FloatAddU64(Float *result, Float *a, u64 b);
void FloatAddI64(Float *result, Float *a, i64 b);
void FloatAddF32(Float *result, Float *a, float b);
void FloatAddF64(Float *result, Float *a, double b);
void (FloatSub)(Float *result, Float *a, Float *b);
void FloatSubInt(Float *result, Float *a, Int *b);
void FloatSubU64(Float *result, Float *a, u64 b);
void FloatSubI64(Float *result, Float *a, i64 b);
void FloatSubF32(Float *result, Float *a, float b);
void FloatSubF64(Float *result, Float *a, double b);
void (FloatMul)(Float *result, Float *a, Float *b);
void FloatMulInt(Float *result, Float *a, Int *b);
void FloatMulU64(Float *result, Float *a, u64 b);
void FloatMulI64(Float *result, Float *a, i64 b);
void FloatMulF32(Float *result, Float *a, float b);
void FloatMulF64(Float *result, Float *a, double b);
void (FloatDiv)(Float *result, Float *a, Float *b, u64 precision);
void FloatDivInt(Float *result, Float *a, Int *b, u64 precision);
void FloatDivU64(Float *result, Float *a, u64 b, u64 precision);
void FloatDivI64(Float *result, Float *a, i64 b, u64 precision);
void FloatDivF32(Float *result, Float *a, float b, u64 precision);
void FloatDivF64(Float *result, Float *a, double b, u64 precision);

static inline void FloatAddConstFloat(Float *result, Float *a, const Float *b) {
    FloatAdd(result, a, (Float *)b);
}

static inline void FloatAddValueFloat(Float *result, Float *a, Float b) {
    FloatAdd(result, a, &b);
}

static inline void FloatAddConstInt(Float *result, Float *a, const Int *b) {
    FloatAddInt(result, a, (Int *)b);
}

static inline void FloatAddValueInt(Float *result, Float *a, Int b) {
    FloatAddInt(result, a, &b);
}

static inline void FloatSubConstFloat(Float *result, Float *a, const Float *b) {
    FloatSub(result, a, (Float *)b);
}

static inline void FloatSubValueFloat(Float *result, Float *a, Float b) {
    FloatSub(result, a, &b);
}

static inline void FloatSubConstInt(Float *result, Float *a, const Int *b) {
    FloatSubInt(result, a, (Int *)b);
}

static inline void FloatSubValueInt(Float *result, Float *a, Int b) {
    FloatSubInt(result, a, &b);
}

static inline void FloatMulConstFloat(Float *result, Float *a, const Float *b) {
    FloatMul(result, a, (Float *)b);
}

static inline void FloatMulValueFloat(Float *result, Float *a, Float b) {
    FloatMul(result, a, &b);
}

static inline void FloatMulConstInt(Float *result, Float *a, const Int *b) {
    FloatMulInt(result, a, (Int *)b);
}

static inline void FloatMulValueInt(Float *result, Float *a, Int b) {
    FloatMulInt(result, a, &b);
}

static inline void FloatDivConstFloat(Float *result, Float *a, const Float *b, u64 precision) {
    FloatDiv(result, a, (Float *)b, precision);
}

static inline void FloatDivValueFloat(Float *result, Float *a, Float b, u64 precision) {
    FloatDiv(result, a, &b, precision);
}

static inline void FloatDivConstInt(Float *result, Float *a, const Int *b, u64 precision) {
    FloatDivInt(result, a, (Int *)b, precision);
}

static inline void FloatDivValueInt(Float *result, Float *a, Int b, u64 precision) {
    FloatDivInt(result, a, &b, precision);
}

#ifndef __cplusplus
#    define MISRA_FLOAT_ADD_DISPATCH(rhs)                                                                              \
        _Generic(                                                                                                      \
            (rhs),                                                                                                     \
            Float: FloatAddValueFloat,                                                                                 \
            Float *: FloatAdd,                                                                                         \
            const Float *: FloatAddConstFloat,                                                                         \
            Int: FloatAddValueInt,                                                                                     \
            Int *: FloatAddInt,                                                                                        \
            const Int *: FloatAddConstInt,                                                                             \
            unsigned char: FloatAddU64,                                                                                \
            unsigned short: FloatAddU64,                                                                               \
            unsigned int: FloatAddU64,                                                                                 \
            unsigned long: FloatAddU64,                                                                                \
            unsigned long long: FloatAddU64,                                                                           \
            signed char: FloatAddI64,                                                                                  \
            signed short: FloatAddI64,                                                                                 \
            signed int: FloatAddI64,                                                                                   \
            signed long: FloatAddI64,                                                                                  \
            signed long long: FloatAddI64,                                                                             \
            float: FloatAddF32,                                                                                        \
            double: FloatAddF64                                                                                        \
        )

#    define MISRA_FLOAT_SUB_DISPATCH(rhs)                                                                              \
        _Generic(                                                                                                      \
            (rhs),                                                                                                     \
            Float: FloatSubValueFloat,                                                                                 \
            Float *: FloatSub,                                                                                         \
            const Float *: FloatSubConstFloat,                                                                         \
            Int: FloatSubValueInt,                                                                                     \
            Int *: FloatSubInt,                                                                                        \
            const Int *: FloatSubConstInt,                                                                             \
            unsigned char: FloatSubU64,                                                                                \
            unsigned short: FloatSubU64,                                                                               \
            unsigned int: FloatSubU64,                                                                                 \
            unsigned long: FloatSubU64,                                                                                \
            unsigned long long: FloatSubU64,                                                                           \
            signed char: FloatSubI64,                                                                                  \
            signed short: FloatSubI64,                                                                                 \
            signed int: FloatSubI64,                                                                                   \
            signed long: FloatSubI64,                                                                                  \
            signed long long: FloatSubI64,                                                                             \
            float: FloatSubF32,                                                                                        \
            double: FloatSubF64                                                                                        \
        )

#    define MISRA_FLOAT_MUL_DISPATCH(rhs)                                                                              \
        _Generic(                                                                                                      \
            (rhs),                                                                                                     \
            Float: FloatMulValueFloat,                                                                                 \
            Float *: FloatMul,                                                                                         \
            const Float *: FloatMulConstFloat,                                                                         \
            Int: FloatMulValueInt,                                                                                     \
            Int *: FloatMulInt,                                                                                        \
            const Int *: FloatMulConstInt,                                                                             \
            unsigned char: FloatMulU64,                                                                                \
            unsigned short: FloatMulU64,                                                                               \
            unsigned int: FloatMulU64,                                                                                 \
            unsigned long: FloatMulU64,                                                                                \
            unsigned long long: FloatMulU64,                                                                           \
            signed char: FloatMulI64,                                                                                  \
            signed short: FloatMulI64,                                                                                 \
            signed int: FloatMulI64,                                                                                   \
            signed long: FloatMulI64,                                                                                  \
            signed long long: FloatMulI64,                                                                             \
            float: FloatMulF32,                                                                                        \
            double: FloatMulF64                                                                                        \
        )

#    define MISRA_FLOAT_DIV_DISPATCH(rhs)                                                                              \
        _Generic(                                                                                                      \
            (rhs),                                                                                                     \
            Float: FloatDivValueFloat,                                                                                 \
            Float *: FloatDiv,                                                                                         \
            const Float *: FloatDivConstFloat,                                                                         \
            Int: FloatDivValueInt,                                                                                     \
            Int *: FloatDivInt,                                                                                        \
            const Int *: FloatDivConstInt,                                                                             \
            unsigned char: FloatDivU64,                                                                                \
            unsigned short: FloatDivU64,                                                                               \
            unsigned int: FloatDivU64,                                                                                 \
            unsigned long: FloatDivU64,                                                                                \
            unsigned long long: FloatDivU64,                                                                           \
            signed char: FloatDivI64,                                                                                  \
            signed short: FloatDivI64,                                                                                 \
            signed int: FloatDivI64,                                                                                   \
            signed long: FloatDivI64,                                                                                  \
            signed long long: FloatDivI64,                                                                             \
            float: FloatDivF32,                                                                                        \
            double: FloatDivF64                                                                                        \
        )

#    define FloatAdd(result, a, b) MISRA_FLOAT_ADD_DISPATCH(b)((result), (a), (b))
#    define FloatSub(result, a, b) MISRA_FLOAT_SUB_DISPATCH(b)((result), (a), (b))
#    define FloatMul(result, a, b) MISRA_FLOAT_MUL_DISPATCH(b)((result), (a), (b))
#    define FloatDiv(result, a, b, precision) MISRA_FLOAT_DIV_DISPATCH(b)((result), (a), (b), (precision))
#endif

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_FLOAT_MATH_H
