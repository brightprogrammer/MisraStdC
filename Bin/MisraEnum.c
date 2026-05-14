/// file      : bin/misraenum.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Tool takes in JSON specification of a C enum and then emits C code for the
/// corresponding enum type, along with functions to convert to and from string
/// value.
///
/// Sample JSON format:
///
/// {
///   "name": "CKeyword",
///   "to_from_str": true,
///   "invalid_enum" : { "name": "C_KEYWORD_UNKNOWN", "str": "unknown", "value": 0},
///   "entries": [
///     { "name": "C_KEYWORD_ALIGNAS", "str": "alignas" },
///     { "name": "C_KEYWORD_ALIGNOF", "str": "alignof" },
///     { "name": "C_KEYWORD_AUTO", "str": "auto" },
///     { "name": "C_KEYWORD_BOOL", "str": "bool" }
///   ]
/// }
///
///

#include <Misra.h>
#include <Misra/Std/Allocator/Default.h>

typedef struct EnumEntry {
    Str name;
    Str str;
    i64 value;
} EnumEntry;

typedef Vec(EnumEntry) EnumEntries;

// Walk a JSON object; for each key invoke `handler` with `key_str` (Str *) and
// `value_si` (StrIter*) bound. `si` is advanced past the closing `}`.
// We unroll the JR_OBJ macro here because the header version uses StrInit() with
// no allocator and we cannot modify the header.
static StrIter parse_object_keys(StrIter si, Allocator *alloc, void (*on_key)(Str *key, StrIter *value_si, void *ctx), void *ctx) {
    if (!StrIterRemainingLength(&si)) {
        return si;
    }
    StrIter saved_si = si;
    si               = JSkipWhitespace(si);
    if (StrIterPeek(&si) != '{') {
        LOG_ERROR("Invalid object start. Expected '{'.");
        return saved_si;
    }
    StrIterNext(&si);
    si = JSkipWhitespace(si);

    bool expect_comma = false;
    while (StrIterPeek(&si) && StrIterPeek(&si) != '}') {
        if (expect_comma) {
            if (StrIterPeek(&si) != ',') {
                LOG_ERROR("Expected ',' between key/value pairs.");
                return saved_si;
            }
            StrIterNext(&si);
            si = JSkipWhitespace(si);
        }

        Str key = StrInit(alloc);
        StrIter read_si = JReadString(si, &key);
        if (read_si.pos == si.pos) {
            LOG_ERROR("Failed to read key.");
            StrDeinit(&key);
            return saved_si;
        }
        si = read_si;
        si = JSkipWhitespace(si);
        if (StrIterPeek(&si) != ':') {
            LOG_ERROR("Expected ':' after key.");
            StrDeinit(&key);
            return saved_si;
        }
        StrIterNext(&si);
        si = JSkipWhitespace(si);

        StrIter si_before = si;
        on_key(&key, &si, ctx);
        if (si.pos == si_before.pos) {
            // user didn't consume; skip
            si = JSkipValue(si);
        }
        StrDeinit(&key);
        si = JSkipWhitespace(si);
        expect_comma = true;
    }
    if (StrIterPeek(&si) == '}') {
        StrIterNext(&si);
    }
    return si;
}

// Walk a JSON array; call `on_value(value_si, ctx)` for each entry.
static StrIter parse_array_values(StrIter si, void (*on_value)(StrIter *value_si, void *ctx), void *ctx) {
    if (!StrIterRemainingLength(&si)) {
        return si;
    }
    StrIter saved_si = si;
    si               = JSkipWhitespace(si);
    if (StrIterPeek(&si) != '[') {
        LOG_ERROR("Invalid array start. Expected '['.");
        return saved_si;
    }
    StrIterNext(&si);
    si = JSkipWhitespace(si);

    bool expect_comma = false;
    while (StrIterPeek(&si) && StrIterPeek(&si) != ']') {
        if (expect_comma) {
            if (StrIterPeek(&si) != ',') {
                LOG_ERROR("Expected ',' between array values.");
                return saved_si;
            }
            StrIterNext(&si);
            si = JSkipWhitespace(si);
        }
        StrIter si_before = si;
        on_value(&si, ctx);
        if (si.pos == si_before.pos) {
            si = JSkipValue(si);
        }
        si = JSkipWhitespace(si);
        expect_comma = true;
    }
    if (StrIterPeek(&si) == ']') {
        StrIterNext(&si);
    }
    return si;
}

typedef struct InvalidEnumCtx {
    EnumEntry *e;
    Allocator *alloc;
} InvalidEnumCtx;

static void invalid_enum_on_key(Str *key, StrIter *value_si, void *vctx) {
    InvalidEnumCtx *ctx = (InvalidEnumCtx *)vctx;
    if (!StrCmpZstr(key, "name")) {
        Str s = StrInit(ctx->alloc);
        *value_si = JReadString(*value_si, &s);
        StrDeinit(&ctx->e->name);
        ctx->e->name = s;
    } else if (!StrCmpZstr(key, "value")) {
        i64 v = 0;
        *value_si = JReadInteger(*value_si, &v);
        ctx->e->value = v;
    } else if (!StrCmpZstr(key, "str")) {
        Str s = StrInit(ctx->alloc);
        *value_si = JReadString(*value_si, &s);
        StrDeinit(&ctx->e->str);
        ctx->e->str = s;
    }
}

typedef struct EntryCtx {
    EnumEntries *entries;
    Allocator   *alloc;
    bool         to_from_str;
    i64         *last_value;
} EntryCtx;

static void entry_on_key(Str *key, StrIter *value_si, void *vctx) {
    EnumEntry *e = (EnumEntry *)vctx;
    if (!StrCmpZstr(key, "name")) {
        Str s = StrInit(e->name.allocator);
        *value_si = JReadString(*value_si, &s);
        StrDeinit(&e->name);
        e->name = s;
    } else if (!StrCmpZstr(key, "value")) {
        i64 v = 0;
        *value_si = JReadInteger(*value_si, &v);
        e->value = v;
    } else if (!StrCmpZstr(key, "str")) {
        Str s = StrInit(e->str.allocator);
        *value_si = JReadString(*value_si, &s);
        StrDeinit(&e->str);
        e->str = s;
    }
}

static void entries_on_value(StrIter *value_si, void *vctx) {
    EntryCtx *ctx = (EntryCtx *)vctx;
    EnumEntry e   = {0};
    e.name        = StrInit(ctx->alloc);
    e.str         = StrInit(ctx->alloc);
    *value_si     = parse_object_keys(*value_si, ctx->alloc, entry_on_key, &e);

    if (!e.name.length) {
        LOG_ERROR("Invalid enum entry in 'entries' array. Entry without name.");
        abort();
    }

    if (!e.value) {
        e.value = (*ctx->last_value)++;
    } else {
        *ctx->last_value = e.value;
    }

    if (ctx->to_from_str && !e.str.length) {
        LOG_ERROR("to_from_str is set to true but str value not provided for enum {}", e.name);
        abort();
    }

    VecPushBack(ctx->entries, e);
}

typedef struct TopCtx {
    Allocator  *alloc;
    Str        *enum_name;
    bool       *to_from_str;
    EnumEntry  *invalid_enum;
    EnumEntries *entries;
    i64        *last_value;
} TopCtx;

static void top_on_key(Str *key, StrIter *value_si, void *vctx) {
    TopCtx *ctx = (TopCtx *)vctx;
    if (!StrCmpZstr(key, "name")) {
        Str s = StrInit(ctx->alloc);
        *value_si = JReadString(*value_si, &s);
        StrDeinit(ctx->enum_name);
        *ctx->enum_name = s;
    } else if (!StrCmpZstr(key, "to_from_str")) {
        bool b = false;
        *value_si = JReadBool(*value_si, &b);
        *ctx->to_from_str = b;
    } else if (!StrCmpZstr(key, "invalid_enum")) {
        InvalidEnumCtx ic = {.e = ctx->invalid_enum, .alloc = ctx->alloc};
        *value_si = parse_object_keys(*value_si, ctx->alloc, invalid_enum_on_key, &ic);
        if (ctx->invalid_enum->name.length) {
            *ctx->last_value = ctx->invalid_enum->value;
        }
    } else if (!StrCmpZstr(key, "entries")) {
        EntryCtx ec = {.entries = ctx->entries, .alloc = ctx->alloc, .to_from_str = *ctx->to_from_str, .last_value = ctx->last_value};
        *value_si = parse_array_values(*value_si, entries_on_value, &ec);
    }
}

int main(int argc, char **argv) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    LogInit(false, &alloc.base);

    if (argc < 2 || argc > 3) {
        FWriteFmtLn(stderr, "USAGE : {} <enum-json-spec> [output-file-name]", argc == 0 ? "MisraEnum" : argv[0]);
        return 1;
    }

    const char *input_filename  = argv[1];
    const char *output_filename = NULL;
    if (argc == 3) {
        output_filename = argv[2];
    }

    Str code = StrInit(&alloc);
    ReadCompleteFile(input_filename, &code.data, &code.length, &code.capacity, &alloc.base);

    EnumEntries entries = VecInit(&alloc);

    StrIter json        = StrIterFromStr(code);
    Str     enum_name   = StrInit(&alloc);
    bool    to_from_str = false;

    EnumEntry invalid_enum = {0};
    invalid_enum.name      = StrInit(&alloc);
    invalid_enum.str       = StrInit(&alloc);

    i64 last_value = 0;

    TopCtx top = {
        .alloc        = &alloc.base,
        .enum_name    = &enum_name,
        .to_from_str  = &to_from_str,
        .invalid_enum = &invalid_enum,
        .entries      = &entries,
        .last_value   = &last_value,
    };
    json = parse_object_keys(json, &alloc.base, top_on_key, &top);

    StrClear(&code);

    // Use a temporary variable for enum name
    StrWriteFmt(&code, "typedef enum {} {{\n", enum_name.data);

    // last value starts with invalid enum's value
    if (invalid_enum.name.length) {
        last_value = invalid_enum.name.length ? invalid_enum.value : VecFirst(&entries).value;
        StrWriteFmt(&code, "    {} = {},\n", invalid_enum.name.data, invalid_enum.value);
    }

    // Use VecForeach for iterating over entries
    VecForeach(&entries, e) {
        if (last_value == e.value - 1) {
            StrWriteFmt(&code, "    {},\n", e.name.data);
        } else {
            StrWriteFmt(&code, "    {} = {},\n", e.name.data, e.value);
        }
        last_value = e.value;
    };

    StrWriteFmt(&code, "}} {};\n", enum_name.data);

    if (to_from_str) {
        // Store string literals in temporary variables
        const char *funcHeader =
            "///\n"
            "/// Converts given zero-terminated string to {} enum value.\n"
            "///\n"
            "/// zstr[in] : String to be converted back to corresponding enum.\n"
            "///\n"
            "/// SUCCESS : Value of enum\n"
            "/// FAILURE : 0\n"
            "///\n"
            "{} {}FromZstr(const char* zstr) {{\n"
            "    if(!zstr) {{\n"
            "        LOG_ERROR(\"Invalid string provided. Cannot convert to enum.\");\n"
            "        return {};\n"
            "    }}\n";

        // Prepare the return value for invalid enum
        const char *invalidEnumName = "0";
        if (invalid_enum.name.length) {
            invalidEnumName = invalid_enum.name.data;
        }

        StrWriteFmt(&code, funcHeader, enum_name.data, enum_name.data, enum_name.data, invalidEnumName);

        // Use VecForeach for iterating over entries
        VecForeach(&entries, e) {
            const char *compareTemplate = "    if(ZstrCompareN(\"{}\", zstr, {}) == 0) {{return {};}}\n";
            // Store the length in a variable to avoid taking address of rvalue
            unsigned long long strLength = (unsigned long long)e.str.length;
            StrWriteFmt(&code, compareTemplate, e.str.data, strLength, e.name.data);
        };

        const char *returnTemplate = "    return {};\n}}\n";
        StrWriteFmt(&code, returnTemplate, invalidEnumName);

        const char *toZstrHeader =
            "///\n"
            "/// Converts given enum to {} zero-terminated string.\n"
            "///\n"
            "/// e[in] : String to be converted back to corresponding enum.\n"
            "///\n"
            "/// SUCCESS : A zero-terminated char pointer representing corresponding string value of enum\n"
            "/// FAILURE : NULL\n"
            "///\n"
            "const char* {}ToZstr({} e) {{\n"
            "    switch(e) {{\n";

        StrWriteFmt(&code, toZstrHeader, enum_name.data, enum_name.data, enum_name.data);

        // Use VecForeach for iterating over entries
        VecForeach(&entries, e) {
            const char *caseTemplate = "        case {} : {{return \"{}\";}}\n";
            StrWriteFmt(&code, caseTemplate, e.name.data, e.str.data);
        };

        const char *defaultTemplate =
            "        default: break;\n"
            "    }}\n"
            "    return \"{}\";\n"
            "}}\n";

        // Use a static string for NULL to avoid taking address of string literal
        const char *nullStrValue = "NULL";
        const char *nullStr      = nullStrValue;
        if (invalid_enum.str.data) {
            nullStr = invalid_enum.str.data;
        }

        StrWriteFmt(&code, defaultTemplate, nullStr);
    }

    if (output_filename) {
        FILE *f = fopen(output_filename, "w");
        fwrite(code.data, 1, code.length, f);
        fclose(f);
    } else {
        puts(code.data);
    }

    StrDeinit(&invalid_enum.name);
    StrDeinit(&invalid_enum.str);
    StrDeinit(&enum_name);
    StrDeinit(&code);

    VecForeach(&entries, e) {
        StrDeinit(&e.name);
        StrDeinit(&e.str);
    };
    VecDeinit(&entries);

    LogDeinit();
    DefaultAllocatorDeinit(&alloc);
    return 0;
}
