/// file      : std/container/int/convert.h
/// author    : Generated following Misra project patterns
/// This is free and unencumbered software released into the public domain.
///
/// Conversion helpers for Int.

#ifndef MISRA_STD_CONTAINER_INT_CONVERT_H
#define MISRA_STD_CONTAINER_INT_CONVERT_H

#include "Type.h"
#include <Misra/Std/Container/Str.h>

#ifdef __cplusplus
extern "C" {
#endif

Int IntFromU64(u64 value);
u64 IntToU64(Int *value);
Int IntFromBytesLE(const u8 *bytes, u64 len);
u64 IntToBytesLE(Int *value, u8 *bytes, u64 max_len);
Int IntFromBytesBE(const u8 *bytes, u64 len);
u64 IntToBytesBE(Int *value, u8 *bytes, u64 max_len);
Int IntFromStrRadix(const char *digits, u8 radix);
Str IntToStrRadix(Int *value, u8 radix, bool uppercase);
Int IntFromStr(const char *decimal);
Str IntToStr(Int *value);
Int IntFromBinary(const char *binary);
Str IntToBinary(Int *value);
Int IntFromOctStr(const char *octal);
Str IntToOctStr(Int *value);
Int IntFromHexStr(const char *hex);
Str IntToHexStr(Int *value);

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_INT_CONVERT_H
