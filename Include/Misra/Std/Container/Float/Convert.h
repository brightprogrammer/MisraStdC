/// file      : std/container/float/convert.h
/// author    : Generated following Misra project patterns
/// This is free and unencumbered software released into the public domain.
///
/// Conversion helpers for Float.

#ifndef MISRA_STD_CONTAINER_FLOAT_CONVERT_H
#define MISRA_STD_CONTAINER_FLOAT_CONVERT_H

#include "Type.h"
#include <Misra/Std/Container/Str.h>

#ifdef __cplusplus
extern "C" {
#endif

Float FloatFromU64(u64 value);
Float FloatFromI64(i64 value);
Float FloatFromInt(Int *value);
bool  FloatToInt(Int *result, Float *value);
Float FloatFromStr(const char *text);
Str   FloatToStr(Float *value);

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_FLOAT_CONVERT_H
