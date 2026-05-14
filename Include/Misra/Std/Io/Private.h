/// file      : std/io/private.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Internal helpers for the formatted-I/O macros (`StrWriteFmt`,
/// `StrReadFmt`, `WriteFmt`, `ReadFmt`, ...). Declarations live here so
/// the public umbrella `Misra/Std/Io.h` stays focused on user-facing
/// macros and types; the public header pulls this in transitively.
/// User code should never call these functions directly.

#ifndef MISRA_STD_IO_PRIVATE_H
#define MISRA_STD_IO_PRIVATE_H

#include <stdio.h>

#include <Misra/Std/Container/Str.h>
#include <Misra/Types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct TypeSpecificIO TypeSpecificIO;

///
/// Append the result of expanding placeholders in `fmt` to `o`.
/// Called by the `StrWriteFmt` macro after it has built the `TypeSpecificIO`
/// argv array.
///
bool str_write_fmt(Str *o, const char *fmt, TypeSpecificIO *args, u64 argc);

///
/// Write the result of expanding placeholders in `fmtstr` to `stream`.
/// `append_newline` decides whether a `\n` is appended after the formatted
/// payload. Called by the `WriteFmt` / `WriteFmtLn` macros.
///
bool f_write_fmt(FILE *stream, const char *fmtstr, TypeSpecificIO *argv, u64 argc, bool append_newline);

///
/// Read placeholders from a NUL-terminated input string into the
/// `TypeSpecificIO` argv slots. Returns a pointer inside `input` to the
/// position just past the consumed text on success, `NULL` on no-match.
/// Called by the `StrReadFmt` macro.
///
const char *str_read_fmt(const char *input, const char *fmtstr, TypeSpecificIO *argv, u64 argc);

///
/// Read placeholders from `stream` into the `TypeSpecificIO` argv slots.
/// On format-mismatch, attempts to roll back the underlying stream so the
/// caller sees no consumed data. Called by the `ReadFmt` macro.
///
void f_read_fmt(FILE *stream, const char *fmtstr, TypeSpecificIO *argv, u64 argc);

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_IO_PRIVATE_H
