/// file      : std/argparse.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Command-line argument parser. Declarative, type-inferred, no globals.
///
/// One schema, three outputs: parser, error messages, auto `--help`.
/// Inspired by Rust's clap (builder mode); the type-dispatch trick is
/// borrowed from the MisraStdC IOFMT system.
///
/// Five registration verbs cover the whole CLI vocabulary:
///
///   `ArgRequired`   -- value option that must appear (e.g. `--listen X`)
///   `ArgOptional`   -- value option that may appear
///   `ArgFlag`       -- boolean toggle, no value (`--verbose`)
///   `ArgCount`      -- repeatable flag, increments a counter (`-vvv`)
///   `ArgPositional` -- positional argument, slot order = registration order
///
/// Value targets are typed via `_Generic` -- pass `&u32_var` and the
/// parser knows it has to convert to u32; pass `&str_ptr` and it stores
/// the raw string. Supported target types: `const char **`, `char **`,
/// `bool *`, `u8/u16/u32/u64 *`, `i8/i16/i32/i64 *`, `f32/f64 *`,
/// `Str *`. Unknown targets trip `ARG_KIND_INVALID` and `ArgParseRun`
/// refuses to start.
///
/// Three value forms are accepted on the command line:
///
///   `--long VAL`     -- next argv token is the value
///   `--long=VAL`     -- inline value (the form to use when VAL starts with `-`)
///   `-s VAL`         -- short form, next argv token is the value
///
/// `--` ends option parsing; every token after it is positional.
///
/// `--help` / `-h` is registered automatically and prints an
/// auto-generated usage table. No `--version` in v1 (callers can
/// register their own with `ArgFlag` if they want).

#ifndef MISRA_STD_ARGPARSE_H
#define MISRA_STD_ARGPARSE_H

#include <Misra/Std/Allocator.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Container/Vec.h>
#include <Misra/Types.h>

#ifdef __cplusplus
extern "C" {
#endif

    ///
    /// Type tag for a target variable, computed at compile time via
    /// `_Generic`. Drives both value-string parsing and the metavar /
    /// error-message wording.
    ///
    typedef enum ArgKind {
        ARG_KIND_INVALID = 0, // unknown target type -- registration fails
        ARG_KIND_ZSTR,        // `const char **` / `char **`
        ARG_KIND_STR,         // `Str *`
        ARG_KIND_BOOL,        // `bool *`
        ARG_KIND_U8,
        ARG_KIND_U16,
        ARG_KIND_U32,
        ARG_KIND_U64,
        ARG_KIND_I8,
        ARG_KIND_I16,
        ARG_KIND_I32,
        ARG_KIND_I64,
        ARG_KIND_F32,
        ARG_KIND_F64,
    } ArgKind;

    ///
    /// What kind of slot this is (value option, flag, count, positional).
    /// Registration verbs map 1:1 to this enum.
    ///
    typedef enum ArgRole {
        ARG_ROLE_REQUIRED,   // value option, required
        ARG_ROLE_OPTIONAL,   // value option, optional
        ARG_ROLE_FLAG,       // no value, presence => true (bool target)
        ARG_ROLE_COUNT,      // no value, increments per occurrence (integer target)
        ARG_ROLE_POSITIONAL, // value, positional slot (always required)
    } ArgRole;

    ///
    /// `ArgParseRun` return value. Lets the caller distinguish
    /// "continue main" from "help printed, exit 0" from "error printed,
    /// exit 1" without overloading bool.
    ///
    typedef enum ArgRun {
        ARG_RUN_OK,    // parse succeeded, continue
        ARG_RUN_HELP,  // `--help` printed; caller should exit 0
        ARG_RUN_ERROR, // bad input, error printed; caller should exit non-zero
    } ArgRun;

    ///
    /// Per-target tag produced by the `ARG_TARGET` macro. Carries the
    /// kind enum and an opaque pointer to the variable the parser will
    /// write into. Generated at compile time -- caller never builds
    /// this by hand.
    ///
    typedef struct ArgTarget {
        ArgKind kind;
        void   *target;
    } ArgTarget;

    ///
    /// Internal registration record. One entry per `ArgRequired` /
    /// `ArgOptional` / `ArgFlag` / `ArgCount` / `ArgPositional` call.
    /// Lives in the `ArgParse.specs` Vec; populated at registration
    /// time, mutated during `ArgParseRun`.
    ///
    typedef struct ArgSpec {
        const char *short_name; // "-l" or NULL; ignored for positionals
        const char *long_name;  // "--listen" for options; metavar (e.g. "hostname") for positionals
        const char *help;       // one-line description for `--help`
        ArgRole     role;
        ArgKind     kind;
        void       *target;
        bool        seen; // set during parse so missing-required can be checked
    } ArgSpec;

    typedef Vec(ArgSpec) ArgSpecs;

    ///
    /// Parser state. Stack-allocate, init once, register verbs, run.
    ///
    /// `name`  -- shown as the program name in `--help` and errors.
    /// `about` -- one-line description at the top of `--help`. May be NULL.
    /// `alloc` -- where the `specs` Vec and any owned strings come from.
    ///
    typedef struct ArgParse {
        Allocator  *alloc;
        const char *name;
        const char *about;
        ArgSpecs    specs;
    } ArgParse;

    ///
    /// Create a parser. Registers `-h` / `--help` automatically. The
    /// `name` and `about` pointers are borrowed -- they must outlive
    /// the parser (string literals are the typical case).
    ///
    /// SUCCESS: Returns an initialized parser.
    /// FAILURE: Aborts via `LOG_FATAL` on allocator OOM.
    ///
    ArgParse ArgParseInit(const char *name, const char *about, Allocator *alloc);

    ///
    /// Release the spec Vec. Safe on a fully-initialised parser; not
    /// safe on a zero-initialised one.
    ///
    void ArgParseDeinit(ArgParse *self);

    ///
    /// Walk `argv` and populate every registered target. Prints errors
    /// or `--help` straight to the diagnostic channel; caller only
    /// needs to inspect the return value.
    ///
    /// argv[0] is treated as the program path (skipped during parsing).
    ///
    /// SUCCESS : Returns `ARG_RUN_OK`; every registered target is set.
    /// FAILURE : Returns `ARG_RUN_HELP` (help printed) or
    ///           `ARG_RUN_ERROR` (parse error logged + usage hint).
    ///
    ArgRun ArgParseRun(ArgParse *self, int argc, char **argv);

    ///
    /// Internal registration entry point. Called by the `ArgRequired` /
    /// `ArgOptional` / `ArgFlag` / `ArgCount` / `ArgPositional` macros
    /// after `_Generic` dispatch has tagged the target. Public so the
    /// macros can resolve it from caller TUs; not intended for direct
    /// use.
    ///
    void arg_register(
        ArgParse   *self,
        ArgRole     role,
        const char *short_name,
        const char *long_name,
        const char *help,
        ArgTarget   target
    );

    ///
    /// Type-aware target tag generator. Inspects the pointee type of
    /// `t` via `_Generic` and produces an `ArgTarget` carrying the
    /// `ArgKind` plus the raw void pointer. Unknown target types fall
    /// through to `ARG_KIND_INVALID` and `arg_register` refuses the
    /// registration with a `LOG_FATAL` so the bug surfaces at startup
    /// rather than at parse time.
    ///
    /// Note on `i8`: `bool` is `typedef i8 bool` in `Misra/Types.h`, so
    /// `bool *` and `i8 *` collide at the C type level and `_Generic`
    /// can't disambiguate them. We map this single shared pointer type
    /// to `ARG_KIND_BOOL` (the cli-useful one). Anyone needing a tiny
    /// signed integer from the command line should reach for `i16`.
    ///
    /// TAGS: Macro, TypeDispatch, Generic, ArgParse
    ///
#define ARG_TARGET(t)                                                                                                  \
    _Generic(                                                                                                          \
        (t),                                                                                                           \
        const char **: ((ArgTarget) {ARG_KIND_ZSTR, (void *)(t)}),                                                     \
        char **: ((ArgTarget) {ARG_KIND_ZSTR, (void *)(t)}),                                                           \
        Str *: ((ArgTarget) {ARG_KIND_STR, (void *)(t)}),                                                              \
        bool *: ((ArgTarget) {ARG_KIND_BOOL, (void *)(t)}),                                                            \
        u8 *: ((ArgTarget) {ARG_KIND_U8, (void *)(t)}),                                                                \
        u16 *: ((ArgTarget) {ARG_KIND_U16, (void *)(t)}),                                                              \
        u32 *: ((ArgTarget) {ARG_KIND_U32, (void *)(t)}),                                                              \
        u64 *: ((ArgTarget) {ARG_KIND_U64, (void *)(t)}),                                                              \
        i16 *: ((ArgTarget) {ARG_KIND_I16, (void *)(t)}),                                                              \
        i32 *: ((ArgTarget) {ARG_KIND_I32, (void *)(t)}),                                                              \
        i64 *: ((ArgTarget) {ARG_KIND_I64, (void *)(t)}),                                                              \
        f32 *: ((ArgTarget) {ARG_KIND_F32, (void *)(t)}),                                                              \
        f64 *: ((ArgTarget) {ARG_KIND_F64, (void *)(t)}),                                                              \
        default: ((ArgTarget) {ARG_KIND_INVALID, NULL})                                                                \
    )

    ///
    /// Register a required value option (`--listen X` / `-l X` /
    /// `--listen=X`). The target type is inferred from `target`. After
    /// `ArgParseRun` returns `ARG_RUN_OK` the target is guaranteed to
    /// have been written.
    ///
    /// USAGE:
    ///   const char *listen = NULL;
    ///   ArgRequired(&p, "-l", "--listen", &listen, "host:port to listen on");
    ///
#define ArgRequired(parser, short_, long_, target, help_)                                                              \
    arg_register((parser), ARG_ROLE_REQUIRED, (short_), (long_), (help_), ARG_TARGET(target))

    ///
    /// Register an optional value option. Target keeps its prior value
    /// if the option doesn't appear -- initialise with the default you
    /// want.
    ///
    /// USAGE:
    ///   u32 timeout = 30;
    ///   ArgOptional(&p, NULL, "--timeout", &timeout, "connection timeout in seconds");
    ///
#define ArgOptional(parser, short_, long_, target, help_)                                                              \
    arg_register((parser), ARG_ROLE_OPTIONAL, (short_), (long_), (help_), ARG_TARGET(target))

    ///
    /// Register a boolean toggle. No value consumed; presence sets the
    /// target to `true`. Target must be `bool *`.
    ///
    /// USAGE:
    ///   bool verbose = false;
    ///   ArgFlag(&p, "-v", "--verbose", &verbose, "verbose logging");
    ///
#define ArgFlag(parser, short_, long_, target, help_)                                                                  \
    arg_register((parser), ARG_ROLE_FLAG, (short_), (long_), (help_), ARG_TARGET(target))

    ///
    /// Register a repeat counter. No value consumed; the target is
    /// incremented per occurrence. `-vvv` counts as three. Target must
    /// be one of the unsigned integer pointer types.
    ///
    /// USAGE:
    ///   u32 verbose = 0;
    ///   ArgCount(&p, "-v", "--verbose", &verbose, "verbose logging (repeatable)");
    ///
#define ArgCount(parser, short_, long_, target, help_)                                                                 \
    arg_register((parser), ARG_ROLE_COUNT, (short_), (long_), (help_), ARG_TARGET(target))

    ///
    /// Register a positional argument. Slot order on the command line
    /// matches registration order. Always required.
    ///
    /// `name` is both the help-text label and the metavar shown in
    /// usage output (rendered as `<NAME>`). It is not parsed for `--`
    /// prefixes -- positionals never start with `-`.
    ///
    /// USAGE:
    ///   const char *hostname = NULL;
    ///   ArgPositional(&p, "hostname", &hostname, "name to resolve");
    ///
#define ArgPositional(parser, name, target, help_)                                                                     \
    arg_register((parser), ARG_ROLE_POSITIONAL, NULL, (name), (help_), ARG_TARGET(target))

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_ARGPARSE_H
