/// file      : std/argparse.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Implementation of the ArgParse command-line parser. See ArgParse.h
/// for the public API surface and the design notes.

#include <Misra/Std/ArgParse.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Std/File.h>
#include <Misra/Std/Io.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Memory.h>

/* ------------------------------------------------------------------ */
/* Small helpers                                                       */
/* ------------------------------------------------------------------ */

// Human-readable type name used in invalid-value error messages. Lines
// up 1:1 with the ArgKind enum; new entries get a label here.
static Zstr arg_kind_label(ArgKind k) {
    switch (k) {
        case ARG_KIND_ZSTR :
            return "string";
        case ARG_KIND_STR :
            return "string";
        case ARG_KIND_BOOL :
            return "bool";
        case ARG_KIND_U8 :
            return "u8";
        case ARG_KIND_U16 :
            return "u16";
        case ARG_KIND_U32 :
            return "u32";
        case ARG_KIND_U64 :
            return "u64";
        case ARG_KIND_I8 :
            return "i8";
        case ARG_KIND_I16 :
            return "i16";
        case ARG_KIND_I32 :
            return "i32";
        case ARG_KIND_I64 :
            return "i64";
        case ARG_KIND_F32 :
            return "f32";
        case ARG_KIND_F64 :
            return "f64";
        default :
            return "invalid";
    }
}

static bool zstr_eq(Zstr a, Zstr b) {
    return a && b && ZstrCompare(a, b) == 0;
}

// Decimal integer parse that requires the entire input to be consumed.
// Returns false on empty input, non-digit content, or trailing junk.
// Signed range checked against the caller-supplied lo/hi.
static bool parse_signed(Zstr s, i64 lo, i64 hi, i64 *out) {
    if (!s || !*s)
        return false;
    Zstr end = NULL;
    i64  v   = ZstrToI64(s, &end);
    if (!end || end == s || *end != '\0')
        return false;
    if (v < lo || v > hi)
        return false;
    *out = v;
    return true;
}

// Same shape, unsigned. ZstrToI64 doesn't span the unsigned-64 range
// past 0x7fffffffffffffff so we walk decimal digits ourselves.
static bool parse_unsigned(Zstr s, u64 hi, u64 *out) {
    if (!s || !*s)
        return false;
    Zstr p = s;
    u64  v = 0;
    while (*p >= '0' && *p <= '9') {
        u64 d = (u64)(*p - '0');
        if (v > (~(u64)0 - d) / 10)
            return false; // would overflow u64
        v = v * 10 + d;
        ++p;
    }
    if (p == s || *p != '\0')
        return false;
    if (v > hi)
        return false;
    *out = v;
    return true;
}

static bool parse_bool(Zstr s, bool *out) {
    if (zstr_eq(s, "true") || zstr_eq(s, "1") || zstr_eq(s, "yes") || zstr_eq(s, "on")) {
        *out = true;
        return true;
    }
    if (zstr_eq(s, "false") || zstr_eq(s, "0") || zstr_eq(s, "no") || zstr_eq(s, "off")) {
        *out = false;
        return true;
    }
    return false;
}

static bool parse_f64_full(Zstr s, f64 *out) {
    if (!s || !*s)
        return false;
    Zstr end = NULL;
    f64  v   = ZstrToF64(s, &end);
    if (!end || end == s || *end != '\0')
        return false;
    *out = v;
    return true;
}

// Write a parsed value into target according to its kind. The string
// `value` is the raw token from argv (or the `=value` slice). Returns
// false on type-mismatch / out-of-range so the caller can emit a
// "invalid value 'X' for --flag: expected <type>" message.
static bool store_value(ArgKind kind, void *target, Zstr value) {
    switch (kind) {
        case ARG_KIND_ZSTR : {
            *(Zstr *)target = value;
            return true;
        }
        case ARG_KIND_STR : {
            Str *s = (Str *)target;
            StrClear(s);
            StrPushBackZstr(s, value);
            return true;
        }
        case ARG_KIND_BOOL :
            return parse_bool(value, (bool *)target);
        case ARG_KIND_U8 : {
            u64 v = 0;
            if (!parse_unsigned(value, 0xFFu, &v))
                return false;
            *(u8 *)target = (u8)v;
            return true;
        }
        case ARG_KIND_U16 : {
            u64 v = 0;
            if (!parse_unsigned(value, 0xFFFFu, &v))
                return false;
            *(u16 *)target = (u16)v;
            return true;
        }
        case ARG_KIND_U32 : {
            u64 v = 0;
            if (!parse_unsigned(value, 0xFFFFFFFFu, &v))
                return false;
            *(u32 *)target = (u32)v;
            return true;
        }
        case ARG_KIND_U64 : {
            u64 v = 0;
            if (!parse_unsigned(value, ~(u64)0, &v))
                return false;
            *(u64 *)target = v;
            return true;
        }
        case ARG_KIND_I8 : {
            i64 v = 0;
            if (!parse_signed(value, -128, 127, &v))
                return false;
            *(i8 *)target = (i8)v;
            return true;
        }
        case ARG_KIND_I16 : {
            i64 v = 0;
            if (!parse_signed(value, -32768, 32767, &v))
                return false;
            *(i16 *)target = (i16)v;
            return true;
        }
        case ARG_KIND_I32 : {
            i64 v = 0;
            if (!parse_signed(value, -2147483647 - 1, 2147483647, &v))
                return false;
            *(i32 *)target = (i32)v;
            return true;
        }
        case ARG_KIND_I64 : {
            i64 v = 0;
            if (!parse_signed(value, (i64)0x8000000000000000ULL, (i64)0x7FFFFFFFFFFFFFFFULL, &v))
                return false;
            *(i64 *)target = v;
            return true;
        }
        case ARG_KIND_F32 : {
            f64 v = 0;
            if (!parse_f64_full(value, &v))
                return false;
            *(f32 *)target = (f32)v;
            return true;
        }
        case ARG_KIND_F64 :
            return parse_f64_full(value, (f64 *)target);
        default :
            return false;
    }
}

// Increment a counter target (ArgCount role). Counters are restricted
// to unsigned-int kinds at registration time; signed counters would
// give surprising wrap-around semantics for repeated flags.
static bool count_bump(ArgKind kind, void *target) {
    switch (kind) {
        case ARG_KIND_U8 :
            *(u8 *)target = (u8)(*(u8 *)target + 1);
            return true;
        case ARG_KIND_U16 :
            *(u16 *)target = (u16)(*(u16 *)target + 1);
            return true;
        case ARG_KIND_U32 :
            *(u32 *)target = *(u32 *)target + 1u;
            return true;
        case ARG_KIND_U64 :
            *(u64 *)target = *(u64 *)target + 1u;
            return true;
        default :
            return false;
    }
}

/* ------------------------------------------------------------------ */
/* Spec lookup                                                         */
/* ------------------------------------------------------------------ */

static ArgSpec *find_long(ArgParse *self, Zstr long_name) {
    VecForeachPtr(&self->specs, sp) {
        if (sp->role == ARG_ROLE_POSITIONAL)
            continue;
        if (zstr_eq(sp->long_name, long_name))
            return sp;
    }
    return NULL;
}

static ArgSpec *find_short(ArgParse *self, Zstr short_name) {
    VecForeachPtr(&self->specs, sp) {
        if (sp->role == ARG_ROLE_POSITIONAL)
            continue;
        if (zstr_eq(sp->short_name, short_name))
            return sp;
    }
    return NULL;
}

/* ------------------------------------------------------------------ */
/* Help printing                                                       */
/* ------------------------------------------------------------------ */

// Build the metavar for a value option. Either the explicit override
// stashed in long_name post-`=` (we don't support that yet) or the
// long-flag name upper-cased and hyphens swapped for underscores. For
// short-only options without a long form we fall back to "VALUE".
static void append_metavar(Str *out, const ArgSpec *sp) {
    Zstr src = sp->long_name;
    if (!src) {
        StrPushBackZstr(out, "VALUE");
        return;
    }
    // skip leading "--"
    if (src[0] == '-' && src[1] == '-')
        src += 2;
    while (*src) {
        char c = *src++;
        if (c == '-')
            c = '_';
        if (c >= 'a' && c <= 'z')
            c = (char)(c - 'a' + 'A');
        StrPushBack(out, c);
    }
}

// Build the "  -l, --listen <LISTEN>" left column for one spec. Returns
// the visible width so the caller can pad to a shared right margin.
static u64 spec_format_left(const ArgSpec *sp, Str *out) {
    u64 start = out->length;
    StrPushBackZstr(out, "  ");
    if (sp->role == ARG_ROLE_POSITIONAL) {
        StrPushBack(out, '<');
        StrPushBackZstr(out, sp->long_name);
        StrPushBack(out, '>');
    } else {
        if (sp->short_name) {
            StrPushBackZstr(out, sp->short_name);
            if (sp->long_name)
                StrPushBackZstr(out, ", ");
        } else {
            StrPushBackZstr(out, "    ");
        }
        if (sp->long_name)
            StrPushBackZstr(out, sp->long_name);
        if (sp->role == ARG_ROLE_REQUIRED || sp->role == ARG_ROLE_OPTIONAL) {
            StrPushBack(out, ' ');
            StrPushBack(out, '<');
            append_metavar(out, sp);
            StrPushBack(out, '>');
        }
    }
    return out->length - start;
}

static void print_help(ArgParse *self) {
    File err = FileStderr();

    if (self->about) {
        FWriteFmtLn(&err, "{} -- {}", self->name, self->about);
        FWriteFmtLn(&err, "");
    } else {
        FWriteFmtLn(&err, "{}", self->name);
        FWriteFmtLn(&err, "");
    }

    // Usage line: "usage: name [OPTIONS] --req <REQ>... <POS>..."
    Str usage = StrInit(self->alloc);
    StrPushBackZstr(&usage, "usage: ");
    StrPushBackZstr(&usage, self->name);

    bool any_option = false;
    VecForeachPtr(&self->specs, sp) {
        if (sp->role == ARG_ROLE_OPTIONAL || sp->role == ARG_ROLE_FLAG || sp->role == ARG_ROLE_COUNT) {
            any_option = true;
            break;
        }
    }
    if (any_option)
        StrPushBackZstr(&usage, " [OPTIONS]");

    VecForeachPtr(&self->specs, sp) {
        if (sp->role != ARG_ROLE_REQUIRED)
            continue;
        StrPushBack(&usage, ' ');
        if (sp->long_name)
            StrPushBackZstr(&usage, sp->long_name);
        else if (sp->short_name)
            StrPushBackZstr(&usage, sp->short_name);
        StrPushBackZstr(&usage, " <");
        append_metavar(&usage, sp);
        StrPushBack(&usage, '>');
    }
    VecForeachPtr(&self->specs, sp) {
        if (sp->role != ARG_ROLE_POSITIONAL)
            continue;
        StrPushBackZstr(&usage, " <");
        StrPushBackZstr(&usage, sp->long_name);
        StrPushBack(&usage, '>');
    }

    FWriteFmtLn(&err, "{}", usage);
    StrDeinit(&usage);
    FWriteFmtLn(&err, "");

    // Two-pass: compute max left-column width, then re-emit with
    // padding. We allocate one Str per spec to remember its left side
    // so we don't have to format it twice.
    Str left_col[64];
    u64 left_w[64];
    u64 max_w   = 0;
    u64 n_specs = self->specs.length;
    if (n_specs > 64)
        n_specs = 64;
    for (u64 i = 0; i < n_specs; ++i) {
        left_col[i] = StrInit(self->alloc);
        left_w[i]   = spec_format_left(&self->specs.data[i], &left_col[i]);
        if (left_w[i] > max_w)
            max_w = left_w[i];
    }

    // Print positionals first, then options.
    bool printed_positional_header = false;
    bool printed_options_header    = false;
    for (u64 i = 0; i < n_specs; ++i) {
        ArgSpec *sp = &self->specs.data[i];
        if (sp->role != ARG_ROLE_POSITIONAL)
            continue;
        if (!printed_positional_header) {
            FWriteFmtLn(&err, "positional arguments:");
            printed_positional_header = true;
        }
        FWriteFmt(&err, "{}", left_col[i]);
        for (u64 pad = left_w[i]; pad < max_w + 2; ++pad)
            FWriteFmt(&err, " ");
        FWriteFmtLn(&err, "{}", sp->help ? sp->help : "");
    }
    if (printed_positional_header)
        FWriteFmtLn(&err, "");

    for (u64 i = 0; i < n_specs; ++i) {
        ArgSpec *sp = &self->specs.data[i];
        if (sp->role == ARG_ROLE_POSITIONAL)
            continue;
        if (!printed_options_header) {
            FWriteFmtLn(&err, "options:");
            printed_options_header = true;
        }
        FWriteFmt(&err, "{}", left_col[i]);
        for (u64 pad = left_w[i]; pad < max_w + 2; ++pad)
            FWriteFmt(&err, " ");
        FWriteFmtLn(&err, "{}", sp->help ? sp->help : "");
    }

    for (u64 i = 0; i < n_specs; ++i) {
        StrDeinit(&left_col[i]);
    }
}

/* ------------------------------------------------------------------ */
/* Registration                                                        */
/* ------------------------------------------------------------------ */

void arg_register(
    ArgParse   *self,
    ArgRole     role,
    Zstr short_name,
    Zstr long_name,
    Zstr help,
    ArgTarget   target
) {
    if (!self)
        LOG_FATAL("arg_register: NULL parser");
    if (target.kind == ARG_KIND_INVALID) {
        LOG_FATAL(
            "arg_register: unsupported target type for '{}'",
            long_name ? long_name : (short_name ? short_name : "<unnamed>")
        );
    }
    if (role == ARG_ROLE_FLAG && target.kind != ARG_KIND_BOOL) {
        LOG_FATAL("arg_register: ArgFlag '{}' requires a bool* target", long_name ? long_name : short_name);
    }
    if (role == ARG_ROLE_COUNT && target.kind != ARG_KIND_U8 && target.kind != ARG_KIND_U16 &&
        target.kind != ARG_KIND_U32 && target.kind != ARG_KIND_U64) {
        LOG_FATAL("arg_register: ArgCount '{}' requires an unsigned-int* target", long_name ? long_name : short_name);
    }
    if (role == ARG_ROLE_POSITIONAL && !long_name) {
        LOG_FATAL("arg_register: ArgPositional needs a non-NULL name");
    }
    if (role != ARG_ROLE_POSITIONAL && !short_name && !long_name) {
        LOG_FATAL("arg_register: option needs at least one of short/long name");
    }

    ArgSpec sp    = {0};
    sp.short_name = short_name;
    sp.long_name  = long_name;
    sp.help       = help;
    sp.role       = role;
    sp.kind       = target.kind;
    sp.target     = target.target;
    sp.seen       = false;
    VecPushBack(&self->specs, sp);
}

/* ------------------------------------------------------------------ */
/* Lifecycle + run                                                     */
/* ------------------------------------------------------------------ */

ArgParse arg_parse_init(Zstr name, Zstr about, Allocator *alloc) {
    if (!name)
        LOG_FATAL("ArgParseInit: name is required");
    if (!alloc)
        LOG_FATAL("ArgParseInit: allocator is required");
    ArgParse p = {0};
    p.alloc    = alloc;
    p.name     = name;
    p.about    = about;
    p.specs    = VecInitT(p.specs, alloc);
    return p;
}

void ArgParseDeinit(ArgParse *self) {
    if (!self)
        return;
    VecDeinit(&self->specs);
}

// Walk a "-s" / "--long" / "--long=VAL" token and dispatch. Returns
// `ARG_RUN_OK` to continue, `ARG_RUN_HELP` if this token was --help,
// `ARG_RUN_ERROR` on failure (after printing the error).
static ArgRun handle_option_token(
    ArgParse   *self,
    Zstr tok,  // the current argv[i] token
    int        *i_io, // walked forward by 1 when we consume a value
    int         argc,
    char      **argv,
    File       *err
) {
    bool        is_long  = (tok[0] == '-' && tok[1] == '-');
    Zstr eq       = NULL;
    Zstr flag     = tok;
    Zstr inline_v = NULL;

    if (is_long) {
        eq = ZstrFindChar(tok, '=');
        if (eq) {
            inline_v = eq + 1;
            // Copy flag part (without the = sign) into a temp buffer
            // so the lookup uses a clean string.
            // Buffer big enough for any sane flag name.
            static char flagbuf[128];
            u64         n = (u64)(eq - tok);
            if (n >= sizeof(flagbuf)) {
                FWriteFmtLn(err, "{}: flag name too long: {}", self->name, tok);
                return ARG_RUN_ERROR;
            }
            MemCopy(flagbuf, tok, n);
            flagbuf[n] = '\0';
            flag       = flagbuf;
        }
    }

    ArgSpec *sp = is_long ? find_long(self, flag) : find_short(self, flag);
    if (!sp) {
        FWriteFmtLn(err, "{}: unknown option: {}", self->name, flag);
        FWriteFmtLn(err, "run with --help for usage");
        return ARG_RUN_ERROR;
    }

    // --help / -h short-circuit straight to help, before we apply any
    // side effects (so partial parses don't pollute caller state).
    if (zstr_eq(sp->long_name, "--help")) {
        print_help(self);
        return ARG_RUN_HELP;
    }

    switch (sp->role) {
        case ARG_ROLE_FLAG : {
            if (inline_v) {
                FWriteFmtLn(err, "{}: flag {} does not take a value", self->name, flag);
                return ARG_RUN_ERROR;
            }
            *(bool *)sp->target = true;
            sp->seen            = true;
            return ARG_RUN_OK;
        }
        case ARG_ROLE_COUNT : {
            if (inline_v) {
                FWriteFmtLn(err, "{}: counter {} does not take a value", self->name, flag);
                return ARG_RUN_ERROR;
            }
            count_bump(sp->kind, sp->target);
            sp->seen = true;
            return ARG_RUN_OK;
        }
        case ARG_ROLE_REQUIRED :
        case ARG_ROLE_OPTIONAL : {
            Zstr val = inline_v;
            if (!val) {
                if (*i_io + 1 >= argc) {
                    FWriteFmtLn(err, "{}: option {} requires a value", self->name, flag);
                    return ARG_RUN_ERROR;
                }
                *i_io += 1;
                val    = argv[*i_io];
            }
            if (!store_value(sp->kind, sp->target, val)) {
                FWriteFmtLn(
                    err,
                    "{}: invalid value '{}' for {}: expected {}",
                    self->name,
                    val,
                    flag,
                    arg_kind_label(sp->kind)
                );
                return ARG_RUN_ERROR;
            }
            sp->seen = true;
            return ARG_RUN_OK;
        }
        case ARG_ROLE_POSITIONAL :
        default :
            FWriteFmtLn(err, "{}: internal error: positional matched as option", self->name);
            return ARG_RUN_ERROR;
    }
}

// Bundled-short-flag form: -vvv counts as three uses of -v, and only
// when -v is a Flag or a Count. -lFOO (stuck value) is intentionally
// not supported in v1.
static ArgRun handle_short_bundle(ArgParse *self, Zstr tok, File *err) {
    // tok looks like "-XYZ..."; verify every char maps to a Flag/Count.
    for (Zstr p = tok + 1; *p; ++p) {
        char     buf[3] = {'-', *p, 0};
        ArgSpec *sp     = find_short(self, (Zstr)buf);
        if (!sp) {
            FWriteFmtLn(err, "{}: unknown option: {}", self->name, (Zstr)buf);
            return ARG_RUN_ERROR;
        }
        if (sp->role == ARG_ROLE_FLAG) {
            *(bool *)sp->target = true;
            sp->seen            = true;
        } else if (sp->role == ARG_ROLE_COUNT) {
            count_bump(sp->kind, sp->target);
            sp->seen = true;
        } else {
            FWriteFmtLn(err, "{}: option {} requires a value, can't bundle", self->name, (Zstr)buf);
            return ARG_RUN_ERROR;
        }
    }
    return ARG_RUN_OK;
}

ArgRun ArgParseRun(ArgParse *self, int argc, char **argv) {
    if (!self || argc < 0 || (argc > 0 && !argv)) {
        LOG_ERROR("ArgParseRun: bad arguments");
        return ARG_RUN_ERROR;
    }

    // Register the auto --help spec once. We could do this in Init but
    // doing it here lets the user inspect specs.length between Init
    // and Run without seeing the synthetic entry first.
    bool already_has_help = false;
    VecForeachPtr(&self->specs, sp) {
        if (zstr_eq(sp->long_name, "--help")) {
            already_has_help = true;
            break;
        }
    }
    if (!already_has_help) {
        // Help has no target; we short-circuit before any store_value.
        ArgSpec help    = {0};
        help.short_name = "-h";
        help.long_name  = "--help";
        help.help       = "print this help";
        help.role       = ARG_ROLE_FLAG;
        help.kind       = ARG_KIND_BOOL;
        help.target     = NULL;
        VecPushBack(&self->specs, help);
    }

    File err = FileStderr();

    bool rest_positional = false;
    u64  next_positional = 0;

    // Count positional slots up front so "too many" errors are
    // accurate even on the first overflow token.
    u64 n_positionals = 0;
    VecForeachPtr(&self->specs, sp) {
        if (sp->role == ARG_ROLE_POSITIONAL)
            ++n_positionals;
    }

    for (int i = 1; i < argc; ++i) {
        Zstr tok = argv[i];

        if (!rest_positional) {
            if (zstr_eq(tok, "--")) {
                rest_positional = true;
                continue;
            }
            if (tok[0] == '-' && tok[1] == '-' && tok[2] != '\0') {
                ArgRun r = handle_option_token(self, tok, &i, argc, argv, &err);
                if (r != ARG_RUN_OK)
                    return r;
                continue;
            }
            if (tok[0] == '-' && tok[1] != '\0' && tok[1] != '-') {
                // Single-char short flag: lookup as "-X". If the
                // remainder is non-empty (-vvv) try the bundled form
                // (only valid for Flag/Count). Otherwise it's a normal
                // short option.
                if (tok[2] != '\0') {
                    char     two[3] = {'-', tok[1], 0};
                    ArgSpec *first  = find_short(self, (Zstr)two);
                    if (first && (first->role == ARG_ROLE_FLAG || first->role == ARG_ROLE_COUNT)) {
                        ArgRun r = handle_short_bundle(self, tok, &err);
                        if (r != ARG_RUN_OK)
                            return r;
                        continue;
                    }
                    FWriteFmtLn(
                        &err,
                        "{}: short value option '{}' cannot be bundled; use form '{} VAL' or '{}=VAL'",
                        self->name,
                        (Zstr)two,
                        (Zstr)two,
                        (Zstr)two
                    );
                    return ARG_RUN_ERROR;
                }
                ArgRun r = handle_option_token(self, tok, &i, argc, argv, &err);
                if (r != ARG_RUN_OK)
                    return r;
                continue;
            }
        }

        // Positional (either consumed naturally or forced by "--").
        if (next_positional >= n_positionals) {
            FWriteFmtLn(&err, "{}: unexpected positional argument: {}", self->name, tok);
            return ARG_RUN_ERROR;
        }
        // Find the n-th positional spec.
        u64      seen = 0;
        ArgSpec *pos  = NULL;
        VecForeachPtr(&self->specs, sp) {
            if (sp->role != ARG_ROLE_POSITIONAL)
                continue;
            if (seen == next_positional) {
                pos = sp;
                break;
            }
            ++seen;
        }
        if (!pos) {
            FWriteFmtLn(&err, "{}: internal error: positional slot {} missing", self->name, (u64)next_positional);
            return ARG_RUN_ERROR;
        }
        if (!store_value(pos->kind, pos->target, tok)) {
            FWriteFmtLn(
                &err,
                "{}: invalid value '{}' for <{}>: expected {}",
                self->name,
                tok,
                pos->long_name,
                arg_kind_label(pos->kind)
            );
            return ARG_RUN_ERROR;
        }
        pos->seen = true;
        ++next_positional;
    }

    // Validate required / positional were all set.
    VecForeachPtr(&self->specs, sp) {
        if ((sp->role == ARG_ROLE_REQUIRED || sp->role == ARG_ROLE_POSITIONAL) && !sp->seen) {
            if (sp->role == ARG_ROLE_POSITIONAL) {
                FWriteFmtLn(&err, "{}: missing required positional argument <{}>", self->name, sp->long_name);
            } else {
                FWriteFmtLn(
                    &err,
                    "{}: missing required option {}",
                    self->name,
                    sp->long_name ? sp->long_name : sp->short_name
                );
            }
            FWriteFmtLn(&err, "run with --help for usage");
            return ARG_RUN_ERROR;
        }
    }

    return ARG_RUN_OK;
}
