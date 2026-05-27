/// file      : std/io/private.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Internal helpers for the formatted-I/O macros (`StrAppendFmt`,
/// `StrReadFmt`, `WriteFmt`, `ReadFmt`, ...). Declarations live here so
/// the public umbrella `Misra/Std/Io.h` stays focused on user-facing
/// macros and types; the public header pulls this in transitively.
/// User code should never call these functions directly.

#ifndef MISRA_STD_IO_PRIVATE_H
#define MISRA_STD_IO_PRIVATE_H

#include <Misra/Std/Container/Str.h>
#include <Misra/Std/File.h>
#include <Misra/Types.h>

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct TypeSpecificIO TypeSpecificIO;

    ///
    /// Implementation backend for `StrAppendFmt`. Appends the formatted
    /// result of `fmt` + `args` to the end of `o`; existing bytes are
    /// preserved.
    ///
    bool str_append_fmt(Str *o, Zstr fmt, TypeSpecificIO *args, u64 argc);

    ///
    /// Implementation backend for `StrWriteFmt`. Clears `o` first, then
    /// appends the formatted result.
    ///
    bool str_write_fmt(Str *o, Zstr fmt, TypeSpecificIO *args, u64 argc);

    ///
    /// Overwrite bytes of `o` starting at `offset` with the formatted
    /// result. Fails if the output would extend past `o->length`. Backs
    /// the `StrPatchFmt` macro.
    ///
    bool str_patch_fmt(Str *o, size offset, Zstr fmt, TypeSpecificIO *args, u64 argc);

    ///
    /// Write the result of expanding placeholders in `fmtstr` to `stream`.
    /// `append_newline` decides whether a `\n` is appended after the formatted
    /// payload. Called by the `WriteFmt` / `WriteFmtLn` / `FWriteFmt` macros.
    ///
    bool f_write_fmt(File *stream, Zstr fmtstr, TypeSpecificIO *argv, u64 argc, bool append_newline);

    ///
    /// Read placeholders from a NUL-terminated input string into the
    /// `TypeSpecificIO` argv slots. Returns a pointer inside `input` to the
    /// position just past the consumed text on success, `NULL` on no-match.
    /// Called by the `StrReadFmt` macro.
    ///
    Zstr str_read_fmt(Zstr input, Zstr fmtstr, TypeSpecificIO *argv, u64 argc);

    ///
    /// Read placeholders from `stream` into the `TypeSpecificIO` argv slots.
    /// On format-mismatch, attempts to roll back the underlying File so the
    /// caller sees no consumed data. Called by the `ReadFmt` / `FReadFmt` macros.
    ///
    void f_read_fmt(File *stream, Zstr fmtstr, TypeSpecificIO *argv, u64 argc);

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_IO_PRIVATE_H
